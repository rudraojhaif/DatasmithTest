// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is provided for educational and reference purposes only.
// Redistribution, modification, or use of this code in any commercial or private
// product is strictly prohibited without explicit written permission from the author.
//
// Unauthorized use in any software or plugin distributed to end-users,
// whether open-source or commercial, is not allowed.
//
// Contact: rudraojhaif@gmail.com for licensing inquiries.
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

/**
 * @brief Constructor - enables ticking for processing queued TCP data
 */
ADSLightSyncer::ADSLightSyncer()
{
	PrimaryActorTick.bCanEverTick = true; // Enable ticking to process queued data from TCP
}

/**
 * @brief Called when the actor begins play
 * 
 * Optionally starts the TCP listener immediately. You can also call
 * StartTcpListener() manually from Blueprint or C++.
 */
void ADSLightSyncer::BeginPlay()
{
	Super::BeginPlay();
	
	// Optionally start listening immediately when the game starts
	// StartTcpListener();
}

/**
 * @brief Called when the actor ends play
 * 
 * Ensures proper cleanup of TCP connections and spawned lights
 */
void ADSLightSyncer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTcpListener();
	ClearExistingLights(); // Clean up any spawned lights
	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Called every frame to process queued TCP data
 * 
 * Processes any JSON light data received via TCP in the game thread
 * to ensure thread-safe light spawning and manipulation.
 */
void ADSLightSyncer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ProcessQueuedData(); // Process any queued light data from TCP
}

/**
 * @brief Starts the TCP listener on the specified port
 * 
 * Creates a TCP socket listener that waits for connections from Rhino.
 * When Rhino sends light data, it's queued for processing in the game thread.
 */
void ADSLightSyncer::StartTcpListener()
{
	if (bIsListening)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCP Listener is already running on port %d"), ListeningPort);
		return;
	}

	// Get the platform-specific socket subsystem
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get socket subsystem"));
		return;
	}

	// Create endpoint to listen on any IP address on the specified port
	FIPv4Endpoint Endpoint(FIPv4Address::Any, ListeningPort);

	// Create TCP listener socket
	TcpListener = MakeShareable(new FTcpListener(Endpoint));

	// Bind connection accepted delegate to our handler function
	TcpListener->OnConnectionAccepted().BindUObject(this, &ADSLightSyncer::HandleConnectionAccepted);

	// Start listening for connections
	if (TcpListener->IsActive())
	{
		bIsListening = true;
		UE_LOG(LogTemp, Log, TEXT("Started TCP listener on port %d - Ready to receive light data from Rhino"), ListeningPort);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start TCP listener on port %d"), ListeningPort);
		TcpListener = nullptr;
	}
}

/**
 * @brief Stops the TCP listener
 * 
 * Gracefully shuts down the TCP listener and cleans up resources.
 */
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
	UE_LOG(LogTemp, Log, TEXT("Stopped TCP listener - No longer receiving light data"));
}

/**
 * @brief Handles incoming TCP connections from Rhino
 * 
 * Called automatically when Rhino connects to send light data.
 * Reads the JSON data in a background thread and queues it for processing.
 * 
 * @param Socket The connected socket from Rhino
 * @param Endpoint The endpoint information of the connection
 * @return True if connection was handled successfully
 */
bool ADSLightSyncer::HandleConnectionAccepted(FSocket* Socket, const FIPv4Endpoint& Endpoint)
{
	if (!Socket)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("TCP connection accepted from Rhino at %s"), *Endpoint.ToString());

	// Handle the connection in a background thread to avoid blocking the game thread
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Socket]()
	{
		// Configure socket for data reception
		Socket->SetNonBlocking(false);
		int32 BufferSize = 65536;
		Socket->SetReceiveBufferSize(BufferSize, BufferSize);

		// Read incoming JSON data from Rhino
		TArray<uint8> ReceivedData;
		uint8 Buffer[4096];
		int32 BytesRead = 0;

		// Keep reading until we get all data or connection closes
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

		// Close the socket after receiving data
		Socket->Close();

		// Convert received binary data to string
		if (ReceivedData.Num() > 0)
		{
			FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			
			// Ensure proper null termination for string conversion
			if (ReceivedData.Last() != 0)
			{
				ReceivedString = FString(ReceivedData.Num(), UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			}

			UE_LOG(LogTemp, Log, TEXT("Received light data JSON from Rhino: %s"), *ReceivedString);

			// Queue the JSON data for processing in the game thread
			// This ensures thread-safe light manipulation
			IncomingDataQueue.Enqueue(ReceivedString);
		}
	});

	return true;
}

/**
 * @brief Processes queued light data in the game thread
 * 
 * Called every frame to process any JSON light data received from Rhino.
 * Ensures all light operations happen in the game thread for thread safety.
 */
void ADSLightSyncer::ProcessQueuedData()
{
	FString JsonData;
	while (IncomingDataQueue.Dequeue(JsonData))
	{
		ProcessReceivedLightData(JsonData);
	}
}

/**
 * @brief Processes received light data from Rhino
 * 
 * Parses the JSON light data and spawns/updates lights in the Unreal scene.
 * 
 * @param JsonData The JSON string containing light information from Rhino
 */
void ADSLightSyncer::ProcessReceivedLightData(const FString& JsonData)
{
	// Parse the JSON data from Rhino
	FRhinoLightData LightData = ParseJsonLightData(JsonData);
	
	if (!LightData.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse received light data from Rhino"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Processing light event from Rhino: %s with %d lights"), 
		*LightData.EventType, LightData.LightCount);

	// Spawn/update lights from the received data
	SpawnLightsFromJsonData(LightData);
}

/**
 * @brief Parses JSON light data received from Rhino
 * 
 * Converts the simplified JSON structure from Rhino into Unreal-compatible data structures.
 * Handles the streamlined format that includes rotation directly.
 * 
 * @param JsonData The JSON string from Rhino
 * @return Parsed light data structure
 */
ADSLightSyncer::FRhinoLightData ADSLightSyncer::ParseJsonLightData(const FString& JsonData)
{
	FRhinoLightData Result;
	Result.bIsValid = false;

	// Parse the JSON string into an object
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON data from Rhino"));
		return Result;
	}

	// Extract main fields from simplified JSON structure
	Result.EventType = JsonObject->GetStringField(TEXT("event"));
	Result.LightCount = JsonObject->GetIntegerField(TEXT("lightCount"));

	UE_LOG(LogTemp, Log, TEXT("Parsing Rhino event: %s with %d lights"), *Result.EventType, Result.LightCount);

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

	// Validate that we parsed the expected number of lights
	Result.bIsValid = (Result.Lights.Num() == Result.LightCount);
	
	if (Result.bIsValid)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully parsed %d lights from Rhino"), Result.Lights.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Light count mismatch: expected %d, parsed %d"), Result.LightCount, Result.Lights.Num());
	}

	return Result;
}

/**
 * @brief Parses individual light data from JSON
 * 
 * Extracts position, rotation, intensity, color and spotlight parameters
 * from the simplified JSON structure sent by Rhino.
 * 
 * @param LightObject JSON object containing a single light's data
 * @return Parsed light data structure
 */
ADSLightSyncer::FLightData ADSLightSyncer::ParseJsonLight(const TSharedPtr<FJsonObject>& LightObject)
{
	FLightData Result;
	Result.bIsValid = false;

	if (!LightObject.IsValid())
	{
		return Result;
	}

	// Parse light type (Point, Directional, Spot)
	Result.LightType = LightObject->GetStringField(TEXT("type"));

	// Parse location from Rhino (coordinates are in meters)
	const TSharedPtr<FJsonObject>* LocationObject;
	if (LightObject->TryGetObjectField(TEXT("location"), LocationObject) && LocationObject->IsValid())
	{
		// Rhino coordinates to Unreal coordinates conversion
		// Rhino: X=Right, Y=Forward, Z=Up
		// Unreal: X=Forward, Y=Right, Z=Up
		// So we swap X and Y from Rhino to get proper Unreal coordinates
		float RhinoX = (*LocationObject)->GetNumberField(TEXT("x"));
		float RhinoY = (*LocationObject)->GetNumberField(TEXT("y"));
		float RhinoZ = (*LocationObject)->GetNumberField(TEXT("z"));
		
		// Convert from meters to Unreal units (1m = 100 Unreal units)
		// and apply coordinate system conversion
		Result.Location = FVector(
			RhinoY * 100.0f,  // Unreal X = Rhino Y
			RhinoX * 100.0f,  // Unreal Y = Rhino X  
			RhinoZ * 100.0f   // Unreal Z = Rhino Z
		);
	}

	// Parse rotation directly from Rhino (no complex conversion needed)
	const TSharedPtr<FJsonObject>* RotationObject;
	if (LightObject->TryGetObjectField(TEXT("rotation"), RotationObject) && RotationObject->IsValid())
	{
		// Get rotation values in degrees from Rhino
		float Pitch = (*RotationObject)->GetNumberField(TEXT("pitch"));
		float Yaw = (*RotationObject)->GetNumberField(TEXT("yaw"));
		float Roll = (*RotationObject)->GetNumberField(TEXT("roll"));
		
		// Direct rotation conversion from Rhino to Unreal
		// Rhino and Unreal both use: Pitch (X-axis), Yaw (Z-axis), Roll (Y-axis)
		// However, coordinate system differences require adjustment:
		// - Rhino: X=Right, Y=Forward, Z=Up
		// - Unreal: X=Forward, Y=Right, Z=Up
		Result.Rotation = FRotator(
			Pitch,   // Keep pitch as-is
			Yaw ,     // Subtract 90 degrees to align coordinate systems
			Roll     // Keep roll as-is
		);
		
		UE_LOG(LogTemp, Log, TEXT("Parsed rotation from JSON - Pitch: %.3f, Yaw: %.3f, Roll: %.3f"), 
			Pitch, Yaw, Roll);
		UE_LOG(LogTemp, Log, TEXT("Applied rotation to Unreal - Pitch: %.3f, Yaw: %.3f, Roll: %.3f"), 
			Result.Rotation.Pitch, Result.Rotation.Yaw, Result.Rotation.Roll);
	}

	// Parse intensity value
	Result.Intensity = LightObject->GetNumberField(TEXT("intensity"));

	// Parse RGB color values (0-255 range from Rhino)
	const TSharedPtr<FJsonObject>* ColorObject;
	if (LightObject->TryGetObjectField(TEXT("color"), ColorObject) && ColorObject->IsValid())
	{
		int32 R = (*ColorObject)->GetIntegerField(TEXT("r"));
		int32 G = (*ColorObject)->GetIntegerField(TEXT("g"));
		int32 B = (*ColorObject)->GetIntegerField(TEXT("b"));
		
		// Convert from 0-255 range to 0-1 range for Unreal
		Result.Color = FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);
	}

	// Parse spotlight parameters if present
	const TSharedPtr<FJsonObject>* SpotLightObject;
	if (LightObject->TryGetObjectField(TEXT("spotLight"), SpotLightObject) && SpotLightObject->IsValid())
	{
		Result.InnerAngle = (*SpotLightObject)->GetNumberField(TEXT("innerAngle"));
		Result.OuterAngle = (*SpotLightObject)->GetNumberField(TEXT("outerAngle"));
	}
	else
	{
		// Default spotlight values for non-spot lights
		Result.InnerAngle = 0.0f;
		Result.OuterAngle = 45.0f;
	}

	Result.bIsValid = true;
	UE_LOG(LogTemp, VeryVerbose, TEXT("Parsed %s light at location %s with rotation %s"), 
		*Result.LightType, *Result.Location.ToString(), *Result.Rotation.ToString());

	return Result;
}

/**
 * @brief Spawns light actors in Unreal from Rhino data
 * 
 * Clears existing lights and creates new ones based on the received data.
 * Handles Point, Directional, and Spot light types with proper conversion
 * of intensity values and spotlight parameters.
 * 
 * @param LightData The parsed light data from Rhino
 */
void ADSLightSyncer::SpawnLightsFromJsonData(const FRhinoLightData& LightData)
{
	// Clear existing lights first to avoid duplicates
	ClearExistingLights();

	UE_LOG(LogTemp, Log, TEXT("Spawning %d lights from Rhino event: %s"), LightData.Lights.Num(), *LightData.EventType);

	// Spawn each light based on its type
	for (int32 LightIndex = 0; LightIndex < LightData.Lights.Num(); LightIndex++)
	{
		const FLightData& Light = LightData.Lights[LightIndex];
		AActor* SpawnedLightActor = nullptr;
		
		if (Light.LightType == TEXT("Point"))
		{
			// Create Point Light
			APointLight* PointLightActor = GetWorld()->SpawnActor<APointLight>(
				APointLight::StaticClass(), 
				Light.Location, 
				FRotator::ZeroRotator
			);
			
			if (PointLightActor)
			{
				// Configure point light properties
				// Multiply intensity for better visibility in Unreal
				PointLightActor->GetLightComponent()->SetIntensity(Light.Intensity * 1000.0f);
				PointLightActor->GetLightComponent()->SetLightColor(Light.Color);
				PointLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				PointLightActor->GetLightComponent()->SetWorldRotation(Light.Rotation);
				SpawnedLightActor = PointLightActor;
				
				UE_LOG(LogTemp, Log, TEXT("Created Point Light %d at %s"), LightIndex, *Light.Location.ToString());
			}
		}
		else if (Light.LightType == TEXT("Directional"))
		{
			// Create Directional Light (like sun light)
			ADirectionalLight* DirLightActor = GetWorld()->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(), 
				Light.Location,
				FRotator::ZeroRotator
			);
			
			if (DirLightActor)
			{
				// Configure directional light properties
				// Use lower multiplier for directional lights as they affect entire scene
				DirLightActor->GetLightComponent()->SetIntensity(Light.Intensity * 10.0f);
				DirLightActor->GetLightComponent()->SetLightColor(Light.Color);
				DirLightActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
				DirLightActor->GetLightComponent()->SetWorldRotation(Light.Rotation);
				SpawnedLightActor = DirLightActor;
				
				UE_LOG(LogTemp, Log, TEXT("Created Directional Light %d with intensity %.2f and rotation %s"), 
					LightIndex, Light.Intensity * 10.0f, *Light.Rotation.ToString());
			}
		}
		else if (Light.LightType == TEXT("Spot"))
		{
			// Create Spot Light with cone angles
			ASpotLight* SpotLightActor = GetWorld()->SpawnActor<ASpotLight>(
				ASpotLight::StaticClass(), 
				Light.Location, 
				Light.Rotation
			);
			
			if (SpotLightActor)
			{
				USpotLightComponent* SpotComponent = SpotLightActor->SpotLightComponent;
				
				// Configure spotlight properties
				SpotComponent->SetIntensity(Light.Intensity * 1000.0f);
				SpotComponent->SetLightColor(Light.Color);
				SpotComponent->SetMobility(EComponentMobility::Movable);
				SpotComponent->SetWorldRotation(Light.Rotation);
				// Set spotlight cone angles from Rhino data
				SpotComponent->SetInnerConeAngle(Light.InnerAngle);
				SpotComponent->SetOuterConeAngle(Light.OuterAngle);
				
				SpawnedLightActor = SpotLightActor;
				
				UE_LOG(LogTemp, Log, TEXT("Created Spot Light %d at %s with inner angle %.1f° and outer angle %.1f°"), 
					LightIndex, *Light.Location.ToString(), Light.InnerAngle, Light.OuterAngle);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unknown light type '%s' at index %d"), *Light.LightType, LightIndex);
			continue;
		}
		
		// Add successfully created light to our tracking array
		if (SpawnedLightActor)
		{
			SpawnedLights.Add(SpawnedLightActor);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn %s light at index %d"), *Light.LightType, LightIndex);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("Light synchronization completed. Successfully spawned %d/%d lights from Rhino"), 
		SpawnedLights.Num(), LightData.Lights.Num());
}

// Legacy file-based functions (kept for backwards compatibility)
/**
 * @brief Legacy function to load lights from file
 * 
 * This function maintains compatibility with the original file-based workflow.
 * It reads light data from a text file instead of TCP.
 */
void ADSLightSyncer::LoadAndSpawnLights()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting legacy light sync from file: %s"), *LightFilePath);

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
		
		// Parse the line using the legacy format
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
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Legacy light sync completed. Spawned %d lights."), SpawnedLights.Num());
}

/**
 * @brief Destroys all previously spawned lights
 * 
 * Cleans up all light actors created by this syncer to prepare for new lights.
 * Called before spawning new lights to avoid duplicates.
 */
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
	
	// Clear the tracking array
	SpawnedLights.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("Cleared %d existing lights"), SpawnedLights.Num());
}

/**
 * @brief Legacy function to parse light data from text file format
 * 
 * Parses the old text-based format for backwards compatibility.
 * New implementations should use the JSON-based TCP approach.
 */
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
	Result.Location.X = Location.Y; 
	Result.Location.Y = Location.X;
	Result.Location.Z = Location.Z;
	
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

// Legacy parsing helper functions (kept for file-based compatibility)
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
	float Roll = FCString::Atof(*RotationParts[1].TrimStartAndEnd());
	float Pitch = FCString::Atof(*RotationParts[0].TrimStartAndEnd());
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