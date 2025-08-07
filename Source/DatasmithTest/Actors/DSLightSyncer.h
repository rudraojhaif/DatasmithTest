#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Networking.h"
#include "DSLightSyncer.generated.h"

// Forward declaration
class FTcpListener;

UCLASS()
class DATASMITHTEST_API ADSLightSyncer : public AActor
{
    GENERATED_BODY()
    
public: 
    ADSLightSyncer();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Structure to hold parsed light data
    struct FLightData
    {
       bool bIsValid = false;
       FString LightType;
       FVector Location = FVector::ZeroVector;
       FRotator Rotation = FRotator::ZeroRotator;
       float Intensity = 1.0f;
       FLinearColor Color = FLinearColor::White;
       float InnerAngle = 0.0f;  // For spot lights
       float OuterAngle = 45.0f; // For spot lights
    };

    // Structure to hold JSON light data from Rhino
    struct FRhinoLightData
    {
        FString EventType;
        FString Timestamp;
        int32 LightCount = 0;
        TArray<FLightData> Lights;
        bool bIsValid = false;
    };

public:
    // Path to the light synchronization file
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Sync")
    FString LightFilePath = TEXT("C:/ProgramData/RhinoLightSync/Lights.txt");

    // TCP listening port
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Sync")
    int32 ListeningPort = 5173;

    // Function to load lights from file and spawn them in the scene
    UFUNCTION(BlueprintCallable, Category = "Light Sync")
    void LoadAndSpawnLights();

    // Function to clear all spawned lights
    UFUNCTION(BlueprintCallable, Category = "Light Sync")
    void ClearExistingLights();

    // Function to start TCP listener
    UFUNCTION(BlueprintCallable, Category = "Light Sync")
    void StartTcpListener();

    // Function to stop TCP listener
    UFUNCTION(BlueprintCallable, Category = "Light Sync")
    void StopTcpListener();

    // Function to process received JSON data (called from game thread)
    UFUNCTION(BlueprintCallable, Category = "Light Sync")
    void ProcessReceivedLightData(const FString& JsonData);

private:
    // Array to keep track of spawned light actors
    UPROPERTY()
    TArray<AActor*> SpawnedLights;

    // TCP Listener
    TSharedPtr<FTcpListener> TcpListener;
    bool bIsListening = false;

    // Thread-safe queue for incoming data
    TQueue<FString, EQueueMode::Mpsc> IncomingDataQueue;

    // Helper functions for parsing
    FLightData ParseLightLine(const FString& Line);
    FVector ParseVectorString(const FString& VectorStr);
    FRotator ParseRotationString(const FString& RotationStr);
    FLinearColor ParseColorString(const FString& ColorStr);

    // JSON parsing functions
    FRhinoLightData ParseJsonLightData(const FString& JsonData);
    FLightData ParseJsonLight(const TSharedPtr<FJsonObject>& LightObject);

    // Spawn lights from JSON data
    void SpawnLightsFromJsonData(const FRhinoLightData& LightData);

    // TCP connection handling
    bool HandleConnectionAccepted(FSocket* Socket, const FIPv4Endpoint& Endpoint);

    // Process queued data in game thread
    void ProcessQueuedData();

public:
    // Tick function to process queued data
    virtual void Tick(float DeltaTime) override;
};