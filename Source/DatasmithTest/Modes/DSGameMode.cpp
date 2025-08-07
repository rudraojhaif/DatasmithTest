#include "DSGameMode.h"
#include "../Controllers/DSPlayerController.h"
#include "../Pawns/DSPawn.h"
#include "../Actors/DSRuntimeManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DatasmithTest/Actors/DSLightSyncer.h"

// Logging category for this game mode
DEFINE_LOG_CATEGORY_STATIC(LogDSGameMode, Log, All);

ADSGameMode::ADSGameMode()
{
    // Set default classes for this game mode
    DefaultPawnClass = ADSPawn::StaticClass();
    PlayerControllerClass = ADSPlayerController::StaticClass();
    
    // Set the DSRuntimeManager class reference
    DSRuntimeManagerClass = ADSRuntimeManager::StaticClass();

    // Set the DSlightSyncer class ref
    DSLightSyncerClass = ADSLightSyncer::StaticClass();
    
    // Initialize default spawn transform (at origin)
    DSRuntimeManagerSpawnTransform = FTransform::Identity;

    UE_LOG(LogDSGameMode, Log, TEXT("DSGameMode: Initialized with DSPawn and DSPlayerController"));
}

void ADSGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogDSGameMode, Log, TEXT("DSGameMode: BeginPlay started"));

    // Ensure a DSRuntimeManager exists in the world
    if (bAutoSpawnDSRuntimeManager)
    {
        EnsureDSRuntimeManagerExists();
    }

    // Ensure a DSLightSyncer exists in the world
    if (bAutoSpawnDSLightSyncer)
    {
        EnsureDSLightSyncerExists();
    }

    UE_LOG(LogDSGameMode, Log, TEXT("DSGameMode: BeginPlay completed"));
}

bool ADSGameMode::EnsureDSRuntimeManagerExists()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Invalid world reference when checking for DSRuntimeManager"));
        return false;
    }

    // Check if a DSRuntimeManager already exists
    for (TActorIterator<ADSRuntimeManager> ActorItr(World); ActorItr; ++ActorItr)
    {
        UE_LOG(LogDSGameMode, Log, TEXT("DSRuntimeManager already exists: %s"), *ActorItr->GetName());
        return true; // Found existing manager
    }

    // No manager found, spawn one if we have a valid class
    if (!DSRuntimeManagerClass)
    {
        UE_LOG(LogDSGameMode, Warning, TEXT("No DSRuntimeManagerClass set, cannot auto-spawn manager"));
        return false;
    }

    ADSRuntimeManager* NewManager = SpawnDSRuntimeManager(DSRuntimeManagerSpawnTransform);
    return IsValid(NewManager);
}

bool ADSGameMode::EnsureDSLightSyncerExists()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Invalid world reference when checking for DSLightSyncer"));
        return false;
    }

    // Check if a DSLightSyncer already exists
    for (TActorIterator<ADSLightSyncer> ActorItr(World); ActorItr; ++ActorItr)
    {
        UE_LOG(LogDSGameMode, Log, TEXT("DSLightSyncer already exists: %s"), *ActorItr->GetName());
        return true; // Found existing syncer
    }

    // No syncer found, spawn one if we have a valid class
    if (!DSLightSyncerClass)
    {
        UE_LOG(LogDSGameMode, Warning, TEXT("No DSLightSyncerClass set, cannot auto-spawn syncer"));
        return false;
    }

    ADSLightSyncer* NewSyncer = SpawnDSLightSyncer(FTransform::Identity);
    return IsValid(NewSyncer);
}


ADSRuntimeManager* ADSGameMode::GetDSRuntimeManager() const
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return nullptr;
    }

    // Find and return the first DSRuntimeManager in the world
    for (TActorIterator<ADSRuntimeManager> ActorItr(World); ActorItr; ++ActorItr)
    {
        return *ActorItr;
    }

    return nullptr;
}

ADSRuntimeManager* ADSGameMode::SpawnDSRuntimeManager(const FTransform& SpawnTransform)
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Cannot spawn DSRuntimeManager - invalid world reference"));
        return nullptr;
    }

    if (!DSRuntimeManagerClass)
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Cannot spawn DSRuntimeManager - no class specified"));
        return nullptr;
    }

    // Set up spawn parameters
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bNoFail = true;

    // Spawn the manager
    ADSRuntimeManager* NewManager = World->SpawnActor<ADSRuntimeManager>(
        DSRuntimeManagerClass,
        SpawnTransform,
        SpawnParams
    );

    if (IsValid(NewManager))
    {
        UE_LOG(LogDSGameMode, Log, TEXT("Successfully spawned DSRuntimeManager: %s"), *NewManager->GetName());
    }
    else
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Failed to spawn DSRuntimeManager"));
    }

    return NewManager;
}

ADSLightSyncer* ADSGameMode::GetDSLightSyncer() const
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return nullptr;
    }

    // Find and return the first DSLightSyncer in the world
    for (TActorIterator<ADSLightSyncer> ActorItr(World); ActorItr; ++ActorItr)
    {
        return *ActorItr;
    }

    return nullptr;
}


ADSLightSyncer* ADSGameMode::SpawnDSLightSyncer(const FTransform& SpawnTransform)
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Cannot spawn DSLightSyncer - invalid world reference"));
        return nullptr;
    }

    if (!DSLightSyncerClass)
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Cannot spawn DSLightSyncer - no class specified"));
        return nullptr;
    }

    // Set up spawn parameters
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bNoFail = true;

    // Spawn the syncer
    ADSLightSyncer* NewSyncer = World->SpawnActor<ADSLightSyncer>(
        DSLightSyncerClass,
        SpawnTransform,
        SpawnParams
    );

    if (IsValid(NewSyncer))
    {
        UE_LOG(LogDSGameMode, Log, TEXT("Successfully spawned DSLightSyncer: %s"), *NewSyncer->GetName());
    }
    else
    {
        UE_LOG(LogDSGameMode, Error, TEXT("Failed to spawn DSLightSyncer"));
    }

    return NewSyncer;
}
