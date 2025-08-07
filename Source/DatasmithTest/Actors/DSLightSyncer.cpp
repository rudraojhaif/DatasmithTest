#include "DSLightSyncer.h"

#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Components/SpotLightComponent.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Json.h"

ADSLightSyncer::ADSLightSyncer()
{
	PrimaryActorTick.bCanEverTick = true; // Enable ticking to process queued data
}

void ADSLightSyncer::BeginPlay()
{
	Super::BeginPlay();
	
	// Optionally start listening immediately
	// StartTcpListener();
}

void ADSLightSyncer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTcpListener();
	Super::EndPlay(EndPlayReason);
}

void ADSLightSyncer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ProcessQueuedData();
}

void ADSLightSyncer::StartTcpListener()
{
	if (bIsListening)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCP Listener is already running"));
		return;
	}

	// Get socket subsystem
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get socket subsystem"));
		return;
	}

	// Create endpoint
	FIPv4Endpoint Endpoint(FIPv4Address::Any, ListeningPort);

	// Create TCP listener
	TcpListener = MakeShareable(new FTcpListener(Endpoint));

	// Set connection accepted delegate
	TcpListener->OnConnectionAccepted().BindUObject(this, &ADSLightSyncer::HandleConnectionAccepted);

	// Start listening
	if (TcpListener->IsActive())
	{
		bIsListening = true;
		UE_LOG(LogTemp, Log, TEXT("Started TCP listener on port %d"), ListeningPort);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start TCP listener on port %d"), ListeningPort);
		TcpListener = nullptr;
	}
}

void ADSLightSyncer::StopTcpListener()
{
	if (!bIsListening)
	{
		return;
	}

	if (TcpListener.IsValid())
	{
		TcpListener->Stop();
		TcpListener = nullptr;
	}

	bIsListening = false;
	UE_LOG(LogTemp, Log, TEXT("Stopped TCP listener"));
}

bool ADSLightSyncer::HandleConnectionAccepted(FSocket* Socket, const FIPv4Endpoint& Endpoint)
{
	if (!Socket)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("TCP connection accepted from %s"), *Endpoint.ToString());

	// Handle the connection in a separate thread
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Socket]()
	{
		// Set socket to blocking mode with timeout
		Socket->SetNonBlocking(false);
		int32 BufferSize = 65536;
		Socket->SetReceiveBufferSize(BufferSize, BufferSize);

		// Read data
		TArray<uint8> ReceivedData;
		uint8 Buffer[4096];
		int32 BytesRead = 0;

		// Keep reading until we get all data or timeout
		while (Socket->Recv(Buffer, sizeof(Buffer), BytesRead))
		{
			if (BytesRead > 0)
			{
				ReceivedData.Append(Buffer, BytesRead);
			}
			else
			{
				break; // No more data or connection closed
			}
		}

		// Close the socket
		Socket->Close();

		// Convert received data to string
		if (ReceivedData.Num() > 0)
		{
			FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			
			// Add null terminator if not present
			if (ReceivedData.Last() != 0)
			{
				ReceivedString = FString(ReceivedData.Num(), UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			}

			UE_LOG(LogTemp, Log, TEXT("Received JSON data: %s"), *ReceivedString);

			// Queue the data for processing in the game thread
			IncomingDataQueue.Enqueue(ReceivedString);
		}
	});

	return true;
}

void ADSLightSyncer::ProcessQueuedData()
{
	FString JsonData;
	while (IncomingDataQueue.Dequeue(JsonData))
	{
		ProcessReceivedLightData(JsonData);
	}
}

void ADSLightSyncer::ProcessReceivedLightData(const FString& JsonData)
{
	// Parse the JSON data
	FRhinoLightData LightData = ParseJsonLightData(JsonData);
	
	if (!LightData.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse received light data"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Processing light event: %s with %d lights"), 
		*LightData.EventType, LightData.LightCount);

	// Spawn lights from the received data
	SpawnLightsFromJsonData(LightData);
}

ADSLightSyncer::FRhinoLightData ADSLightSyncer::ParseJsonLightData(const FString& JsonData)
{
	FRhinoLightData Result;
	Result.bIsValid = false;

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON data"));
		return Result;
	}

	// Extract main fields
	Result.EventType = JsonObject->GetStringField(TEXT("event"));
	Result.Timestamp = JsonObject->GetStringField(TEXT("timestamp"));
	Result.LightCount = JsonObject->GetIntegerField(TEXT("lightCount"));

	// Parse lights array
	const TArray<TSharedPtr<FJsonValue>>* LightsArray;
	if (JsonObject->TryGetArrayField(TEXT("lights"), LightsArray))
	{
		for (const auto& LightValue : *LightsArray)
		{
			TSharedPtr<FJsonObject> LightObject = LightValue->AsObject();
			if (LightObject.IsValid())
			{
				FLightData LightData = ParseJsonLight(LightObject);
				if (LightData.bIsValid)
				{
					Result.Lights.Add(LightData);
				}
			}
		}
	}

	Result.bIsValid = (Result.Lights.Num() == Result.LightCount);
	return Result;
}

ADSLightSyncer::FLightData ADSLightSyncer::ParseJsonLight(const TSharedPtr<FJsonObject>& LightObject)
{
	FLightData Result;
	Result.bIsValid = false;

	if (!LightObject.IsValid())
	{
		return Result;
	}

	// Parse light type
	Result.LightType = LightObject->GetStringField(TEXT("type"));

	// Parse location
	const TSharedPtr<FJsonObject>* LocationObject;
	if (LightObject->TryGetObjectField(TEXT("location"), LocationObject) && LocationObject->IsValid())
	{
		float X = (*LocationObject)->GetNumberField(TEXT("y"));
		float Y = (*LocationObject)->GetNumberField(TEXT("x"));// opposite for rhino
		float Z = (*LocationObject)->GetNumberField(TEXT("z"));
		
		// Convert from Rhino units to UE4 units (cm to UE units)
		Result.Location = FVector(X * 0.1f, Y * 0.1f, Z * 0.1f);
	}

	// Parse direction and convert to rotation
	const TSharedPtr<FJsonObject>* DirectionObject;
	if (LightObject->TryGetObjectField(TEXT("direction"), DirectionObject) && DirectionObject->IsValid())
	{
		float X = (*DirectionObject)->GetNumberField(TEXT("y"));
		float Y = (*DirectionObject)->GetNumberField(TEXT("x"));
		float Z = (*DirectionObject)->GetNumberField(TEXT("z"));
		
		FVector Direction(X, Y, Z);
		Direction.Normalize();
		
		// Convert direction vector to rotation
		Result.Rotation = Direction.Rotation();
	}

	// Parse intensity
	Result.Intensity = LightObject->GetNumberField(TEXT("intensity"));

	// Parse color
	const TSharedPtr<FJsonObject>* ColorObject;
	if (LightObject->TryGetObjectField(TEXT("color"), ColorObject) && ColorObject->IsValid())
	{
		int32 R = (*ColorObject)->GetIntegerField(TEXT("r"));
		int32 G = (*ColorObject)->GetIntegerField(TEXT("g"));
		int32 B = (*ColorObject)->GetIntegerField(TEXT("b"));
		
		Result.Color = FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);
	}

	// Parse spot light angles if present
	const TSharedPtr<FJsonObject>* SpotLightObject;
	if (LightObject->TryGetObjectField(TEXT("spotLight"), SpotLightObject) && SpotLightObject->IsValid())
	{
		Result.InnerAngle = (*SpotLightObject)->GetNumberField(TEXT("innerAngle"));
		Result.OuterAngle = (*SpotLightObject)->GetNumberField(TEXT("outerAngle"));
	}

	Result.bIsValid = true;
	return Result;
}

void ADSLightSyncer::SpawnLightsFromJsonData(const FRhinoLightData& LightData)
{
	// Clear existing lights first
	ClearExistingLights();

	// Spawn each light
	for (const FLightData& Light : LightData.Lights)
	{
		AActor* SpawnedLightActor = nullptr;
		
		if (Light.LightType == TEXT("Point"))
		{
			APointLight* PointLightActor = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), Light.Location, Light.Rotation);
			if (PointLightActor)
			{
				PointLightActor->GetLightComponent()->SetIntensity(Light.Intensity * 1000.0f);
				PointLightActor->GetLightComponent()->SetLightColor(Light.Color);
				PointLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				SpawnedLightActor = PointLightActor;
			}
		}
		else if (Light.LightType == TEXT("Directional"))
		{
			ADirectionalLight* DirLightActor = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Light.Location, Light.Rotation);
			if (DirLightActor)
			{
				DirLightActor->GetLightComponent()->SetIntensity(Light.Intensity * 10.0f);
				DirLightActor->GetLightComponent()->SetLightColor(Light.Color);
				DirLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				SpawnedLightActor = DirLightActor;
			}
		}
		else if (Light.LightType == TEXT("Spot"))
		{
			ASpotLight* SpotLightActor = GetWorld()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Light.Location, Light.Rotation);
			if (SpotLightActor)
			{
				USpotLightComponent* SpotComponent = SpotLightActor->SpotLightComponent;
				SpotComponent->SetIntensity(Light.Intensity * 1000.0f);
				SpotComponent->SetLightColor(Light.Color);
				SpotComponent->SetMobility(EComponentMobility::Movable);
				
				// Set spot light angles
				SpotComponent->SetInnerConeAngle(Light.InnerAngle);
				SpotComponent->SetOuterConeAngle(Light.OuterAngle);
				
				SpawnedLightActor = SpotLightActor;
			}
		}
		
		if (SpawnedLightActor)
		{
			SpawnedLights.Add(SpawnedLightActor);
			UE_LOG(LogTemp, Log, TEXT("Spawned %s light at %s"), 
				*Light.LightType, *Light.Location.ToString());
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("Light sync completed from JSON. Spawned %d lights for event: %s"), 
		SpawnedLights.Num(), *LightData.EventType);
}

// Keep existing file-based functions
void ADSLightSyncer::LoadAndSpawnLights()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting light sync from: %s"), *LightFilePath);

	// Clear any existing lights before spawning new ones
	ClearExistingLights();
	
	// Read the file content
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *LightFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load light file: %s"), *LightFilePath);
		return;
	}
	
	// Split file content into lines
	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);
	
	// Process each line
	for (const FString& Line : Lines)
	{
		// Skip comments and empty lines
		if (Line.StartsWith(TEXT("#")) || Line.IsEmpty())
		{
			continue;
		}
		
		// Parse the line using a more flexible approach
		FLightData ParsedLight = ParseLightLine(Line);
		if (!ParsedLight.bIsValid)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to parse line: %s"), *Line);
			continue;
		}
		
		// Create the appropriate light actor based on type
		AActor* SpawnedLightActor = nullptr;
		
		if (ParsedLight.LightType == TEXT("Point"))
		{
			APointLight* PointLightActor = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), ParsedLight.Location, ParsedLight.Rotation);
			if (PointLightActor)
			{
				PointLightActor->GetLightComponent()->SetIntensity(ParsedLight.Intensity * 1000.0f);
				PointLightActor->GetLightComponent()->SetLightColor(ParsedLight.Color);
				PointLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				SpawnedLightActor = PointLightActor;
			}
		}
		else if (ParsedLight.LightType == TEXT("Directional"))
		{
			ADirectionalLight* DirLightActor = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), ParsedLight.Location, ParsedLight.Rotation);
			if (DirLightActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("Created directional light with intensity: %f"), ParsedLight.Intensity);
				DirLightActor->GetLightComponent()->SetIntensity(ParsedLight.Intensity * 10.0f);
				DirLightActor->GetLightComponent()->SetLightColor(ParsedLight.Color);
				DirLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				SpawnedLightActor = DirLightActor;
			}
		}
		else if (ParsedLight.LightType == TEXT("Spot"))
		{
			ASpotLight* SpotLightActor = GetWorld()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), ParsedLight.Location, ParsedLight.Rotation);
			if (SpotLightActor)
			{
				USpotLightComponent* SpotComponent = SpotLightActor->SpotLightComponent;
				SpotComponent->SetIntensity(ParsedLight.Intensity * 1000.0f);
				SpotComponent->SetLightColor(ParsedLight.Color);
				SpotComponent->SetMobility(EComponentMobility::Movable);
				
				// Set spot light angles
				SpotComponent->SetInnerConeAngle(ParsedLight.InnerAngle);
				SpotComponent->SetOuterConeAngle(ParsedLight.OuterAngle);
				
				UE_LOG(LogTemp, Log, TEXT("Set spot light angles - Inner: %f°, Outer: %f°"), ParsedLight.InnerAngle, ParsedLight.OuterAngle);
				
				SpawnedLightActor = SpotLightActor;
			}
		}
		
		if (SpawnedLightActor)
		{
			// Set light position and rotation
			SpawnedLightActor->SetActorLocation(ParsedLight.Location);
			SpawnedLightActor->SetActorRotation(ParsedLight.Rotation);
			
			// Add to spawned lights array for cleanup
			SpawnedLights.Add(SpawnedLightActor);
			
			UE_LOG(LogTemp, Log, TEXT("Created %s light at location: %s with color: %s"), 
				*ParsedLight.LightType, 
				*ParsedLight.Location.ToString(),
				*ParsedLight.Color.ToString());
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Light sync completed. Spawned %d lights."), SpawnedLights.Num());
}

void ADSLightSyncer::ClearExistingLights()
{
	// Destroy all previously spawned lights
	for (AActor* Light : SpawnedLights)
	{
		if (IsValid(Light))
		{
			Light->Destroy();
		}
	}
	
	// Clear the array
	SpawnedLights.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("Cleared existing lights"));
}

ADSLightSyncer::FLightData ADSLightSyncer::ParseLightLine(const FString& Line)
{
	FLightData Result;
	Result.bIsValid = false;
	
	// Split the line into components by spaces, but be careful with parentheses
	TArray<FString> Components;
	FString CurrentComponent;
	bool bInParentheses = false;
	
	for (int32 i = 0; i < Line.Len(); i++)
	{
		TCHAR Char = Line[i];
		
		if (Char == '(')
		{
			bInParentheses = true;
			CurrentComponent += Char;
		}
		else if (Char == ')')
		{
			bInParentheses = false;
			CurrentComponent += Char;
		}
		else if (Char == ' ' && !bInParentheses)
		{
			if (!CurrentComponent.IsEmpty())
			{
				Components.Add(CurrentComponent.TrimStartAndEnd());
				CurrentComponent.Empty();
			}
		}
		else
		{
			CurrentComponent += Char;
		}
	}
	
	// Add the last component
	if (!CurrentComponent.IsEmpty())
	{
		Components.Add(CurrentComponent.TrimStartAndEnd());
	}
	
	// Expected format: Type Location Rotation Intensity Color [InnerAngle OuterAngle]
	if (Components.Num() < 5)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough components in line. Found %d, expected at least 5"), Components.Num());
		return Result;
	}
	
	// Parse light type
	Result.LightType = Components[0];
	
	// Parse location: (x,y,z)
	FVector Location = ParseVectorString(Components[1]);
	if (Location == FVector::ZeroVector && Components[1] != TEXT("(0,0,0)"))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse location: %s"), *Components[1]);
		return Result;
	}
	
	// Convert to FVector (Rhino to UE4 coordinate conversion)
	Result.Location.X = Location.X * 0.1f; // Convert cm to UE units
	Result.Location.Y = Location.Y * 0.1f;
	Result.Location.Z = Location.Z * 0.1f;
	
	// Parse rotation: (pitch°, yaw°, roll°)
	Result.Rotation = ParseRotationString(Components[2]);
	
	// Parse intensity
	Result.Intensity = FCString::Atof(*Components[3]);
	
	// Parse color: RGB(r,g,b)
	Result.Color = ParseColorString(Components[4]);
	
	// Parse spot light angles if present (for spot lights)
	if (Result.LightType == TEXT("Spot") && Components.Num() >= 7)
	{
		// Remove degree symbol and parse
		FString InnerAngleStr = Components[5].Replace(TEXT("°"), TEXT(""));
		FString OuterAngleStr = Components[6].Replace(TEXT("°"), TEXT(""));
		
		Result.InnerAngle = FCString::Atof(*InnerAngleStr);
		Result.OuterAngle = FCString::Atof(*OuterAngleStr);
	}
	else
	{
		// Default values for non-spot lights
		Result.InnerAngle = 0.0f;
		Result.OuterAngle = 45.0f;
	}
	
	Result.bIsValid = true;
	return Result;
}

FVector ADSLightSyncer::ParseVectorString(const FString& VectorStr)
{
	// Parse vector format: (x,y,z)
	FString CleanStr = VectorStr;
	CleanStr = CleanStr.Replace(TEXT("("), TEXT(""));
	CleanStr = CleanStr.Replace(TEXT(")"), TEXT(""));
	
	TArray<FString> VectorParts;
	CleanStr.ParseIntoArray(VectorParts, TEXT(","));
	
	if (VectorParts.Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid vector format: %s"), *VectorStr);
		return FVector::ZeroVector;
	}
	
	float X = FCString::Atof(*VectorParts[0].TrimStartAndEnd());
	float Y = FCString::Atof(*VectorParts[1].TrimStartAndEnd());
	float Z = FCString::Atof(*VectorParts[2].TrimStartAndEnd());
	
	return FVector(X, Y, Z);
}

FRotator ADSLightSyncer::ParseRotationString(const FString& RotationStr)
{
	// Parse rotation format: (pitch°, yaw°, roll°) to FRotator
	FString CleanStr = RotationStr;
	CleanStr = CleanStr.Replace(TEXT("("), TEXT(""));
	CleanStr = CleanStr.Replace(TEXT(")"), TEXT(""));
	CleanStr = CleanStr.Replace(TEXT("°"), TEXT(""));
	
	TArray<FString> RotationParts;
	CleanStr.ParseIntoArray(RotationParts, TEXT(","));
	
	if (RotationParts.Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid rotation format: %s"), *RotationStr);
		return FRotator::ZeroRotator;
	}
	
	// Convert Rhino rotation to UE4 rotation (may need adjustment based on coordinate system)
	float Roll = FCString::Atof(*RotationParts[0].TrimStartAndEnd());
	float Pitch = FCString::Atof(*RotationParts[1].TrimStartAndEnd());
	float Yaw = FCString::Atof(*RotationParts[2].TrimStartAndEnd());
	
	return FRotator(Pitch, Yaw, Roll);
}

FLinearColor ADSLightSyncer::ParseColorString(const FString& ColorStr)
{
	// Parse color format: RGB(r,g,b)
	FString CleanStr = ColorStr;
	CleanStr = CleanStr.Replace(TEXT("RGB("), TEXT(""));
	CleanStr = CleanStr.Replace(TEXT(")"), TEXT(""));
	
	TArray<FString> ColorParts;
	CleanStr.ParseIntoArray(ColorParts, TEXT(","));
	
	if (ColorParts.Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid color format: %s, using white"), *ColorStr);
		return FLinearColor::White;
	}
	
	float R = FCString::Atof(*ColorParts[0].TrimStartAndEnd()) / 255.0f;
	float G = FCString::Atof(*ColorParts[1].TrimStartAndEnd()) / 255.0f;
	float B = FCString::Atof(*ColorParts[2].TrimStartAndEnd()) / 255.0f;
	
	return FLinearColor(R, G, B, 1.0f);
}