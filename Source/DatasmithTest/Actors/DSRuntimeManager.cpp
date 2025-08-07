#include "DSRuntimeManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// Logging category for this class
DEFINE_LOG_CATEGORY_STATIC(LogDSRuntimeManager, Log, All);

ADSRuntimeManager::ADSRuntimeManager()
{
    // Set this actor to never tick for performance
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // Initialize root component for proper actor placement and hierarchy
    DefaultRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRootComponent"));
    RootComponent = DefaultRootComponent;

    // Set default spawn collision handling method
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void ADSRuntimeManager::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogDSRuntimeManager, Log, TEXT("DSRuntimeManager starting initialization..."));

    // Initialize DirectLink proxy first
    if (!RefreshDirectLinkProxy())
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("Failed to initialize DirectLink proxy during BeginPlay"));
    }

    // Initialize Datasmith actor
    if (!InitializeDatasmithActor())
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("Failed to initialize Datasmith actor during BeginPlay"));
        return;
    }

    // Apply initial import options
    ApplyImportOptions();

    // Log current configuration for debugging
    LogCurrentConfiguration();

    UE_LOG(LogDSRuntimeManager, Log, TEXT("DSRuntimeManager initialization completed successfully"));
}

void ADSRuntimeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogDSRuntimeManager, Log, TEXT("DSRuntimeManager ending play..."));

    // Clean up references
    DatasmithRuntimeActorRef.Reset();
    DirectLinkProxyRef.Reset();

    Super::EndPlay(EndPlayReason);
}

bool ADSRuntimeManager::UpdateDirectLinkConnection()
{
    // Validate DirectLink proxy
    if (!DirectLinkProxyRef.IsValid())
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("DirectLink proxy is invalid, attempting to refresh..."));
        if (!RefreshDirectLinkProxy())
        {
            UE_LOG(LogDSRuntimeManager, Error, TEXT("UpdateDirectLinkConnection failed - unable to obtain DirectLink proxy"));
            return false;
        }
    }

    // Validate Datasmith actor - DO NOT recreate, just check and log if missing
    if (!DatasmithRuntimeActorRef.IsValid())
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("UpdateDirectLinkConnection failed - Datasmith runtime actor is not present. Actor should be created during BeginPlay."));
        return false;
    }

    // Get available sources
    const int32 AvailableSourceCount = GetAvailableSourceCount();
    if (AvailableSourceCount <= 0)
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("No DirectLink sources available"));
        return false;
    }

    // Validate source index
    if (DirectLinkSourceIndex >= AvailableSourceCount)
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("DirectLink source index %d is out of range (available: %d), clamping to 0"), 
               DirectLinkSourceIndex, AvailableSourceCount);
        DirectLinkSourceIndex = 0;
    }

    // Apply current import options before connecting
    if (!ApplyImportOptions())
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("Failed to apply import options before DirectLink connection"));
    }

    // Attempt to open connection
    const bool bConnectionSuccess = DatasmithRuntimeActorRef->OpenConnectionWithIndex(DirectLinkSourceIndex);
    
    if (bConnectionSuccess)
    {
        UE_LOG(LogDSRuntimeManager, Log, TEXT("Successfully opened DirectLink connection with source index %d"), DirectLinkSourceIndex);
    }
    else
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("Failed to open DirectLink connection with source index %d"), DirectLinkSourceIndex);
    }

    return bConnectionSuccess;
}

bool ADSRuntimeManager::InitializeDatasmithActor()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("Cannot initialize Datasmith actor - invalid world reference"));
        return false;
    }

    // Check if actor already exists - only create once
    if (DatasmithRuntimeActorRef.IsValid())
    {
        UE_LOG(LogDSRuntimeManager, Log, TEXT("Datasmith runtime actor already exists, skipping creation"));
        return true;
    }

    // Spawn new Datasmith runtime actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Use identity transform - let the manager handle positioning
    const FTransform SpawnTransform(FQuat::Identity, FVector::ZeroVector, FVector::OneVector);

    ADatasmithRuntimeActor* NewDatasmithActor = World->SpawnActor<ADatasmithRuntimeActor>(
        ADatasmithRuntimeActor::StaticClass(),
        SpawnTransform,
        SpawnParams
    );

    if (!IsValid(NewDatasmithActor))
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("Failed to spawn Datasmith runtime actor"));
        return false;
    }

    // Store weak reference
    DatasmithRuntimeActorRef = NewDatasmithActor;
    
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Successfully spawned Datasmith runtime actor"));
    return true;
}

bool ADSRuntimeManager::RefreshDirectLinkProxy()
{
    UDirectLinkProxy* NewProxy = UDatasmithRuntimeLibrary::GetDirectLinkProxy();
    if (!IsValid(NewProxy))
    {
        UE_LOG(LogDSRuntimeManager, Error, TEXT("Failed to obtain DirectLink proxy from Datasmith Runtime Library"));
        DirectLinkProxyRef.Reset();
        return false;
    }

    DirectLinkProxyRef = NewProxy;
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Successfully refreshed DirectLink proxy"));
    return true;
}

int32 ADSRuntimeManager::GetAvailableSourceCount() const
{
    if (!DirectLinkProxyRef.IsValid())
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("Cannot get source count - DirectLink proxy is invalid"));
        return -1;
    }

    // Get list of sources to determine count
    DirectLinkProxyRef->GetListOfSources();
    
    // Note: The actual implementation would depend on how GetListOfSources returns data
    // This is a placeholder - you may need to modify based on actual API
    return DirectLinkProxyRef->GetListOfSources().Num();
}

bool ADSRuntimeManager::ApplyImportOptions()
{
    if (!DatasmithRuntimeActorRef.IsValid())
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("Cannot apply import options - Datasmith actor is invalid"));
        return false;
    }

    // Create import options from current settings
    const FDatasmithRuntimeImportOptions ImportOptions = CreateImportOptionsFromSettings();
    
    // Apply to the Datasmith actor
    DatasmithRuntimeActorRef->ImportOptions = ImportOptions;
    
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Applied import options to Datasmith runtime actor"));
    return true;
}

// Tessellation Setters with validation
void ADSRuntimeManager::SetChordTolerance(float InChordTolerance)
{
    ChordTolerance = FMath::Clamp(InChordTolerance, 0.001f, 10.0f);
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Chord tolerance set to %f"), ChordTolerance);
}

void ADSRuntimeManager::SetMaxEdgeLength(float InMaxEdgeLength)
{
    MaxEdgeLength = FMath::Max(InMaxEdgeLength, 0.0f);
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Max edge length set to %f"), MaxEdgeLength);
}

void ADSRuntimeManager::SetNormalTolerance(float InNormalTolerance)
{
    NormalTolerance = FMath::Clamp(InNormalTolerance, 0.1f, 90.0f);
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Normal tolerance set to %f"), NormalTolerance);
}

void ADSRuntimeManager::SetStitchingTechnique(EDatasmithCADStitchingTechnique InStitchingTechnique)
{
    StitchingTechnique = InStitchingTechnique;
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Stitching technique set to %d"), (int32)StitchingTechnique);
}

// Hierarchy Setters
void ADSRuntimeManager::SetHierarchyMethod(EBuildHierarchyMethod InHierarchyMethod)
{
    HierarchyMethod = InHierarchyMethod;
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Hierarchy method set to %d"), (int32)HierarchyMethod);
}

// Collision Setters
void ADSRuntimeManager::SetCollisionEnabled(ECollisionEnabled::Type InCollisionEnabled)
{
    CollisionEnabled = InCollisionEnabled;
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Collision enabled set to %d"), (int32)CollisionEnabled);
}

void ADSRuntimeManager::SetCollisionTraceFlag(ECollisionTraceFlag InCollisionTraceFlag)
{
    CollisionTraceFlag = InCollisionTraceFlag;
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Collision trace flag set to %d"), (int32)CollisionTraceFlag);
}

// Metadata Setters
void ADSRuntimeManager::SetImportMetadata(bool bInImportMetadata)
{
    bImportMetaData = bInImportMetadata;
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("Import metadata set to %s"), bImportMetaData ? TEXT("true") : TEXT("false"));
}

// DirectLink Setters
void ADSRuntimeManager::SetDirectLinkSourceIndex(int32 InSourceIndex)
{
    const int32 AvailableSourceCount = GetAvailableSourceCount();
    if (AvailableSourceCount > 0)
    {
        DirectLinkSourceIndex = FMath::Clamp(InSourceIndex, 0, AvailableSourceCount - 1);
    }
    else
    {
        DirectLinkSourceIndex = FMath::Max(InSourceIndex, 0);
    }
    
    UE_LOG(LogDSRuntimeManager, VeryVerbose, TEXT("DirectLink source index set to %d"), DirectLinkSourceIndex);
}

bool ADSRuntimeManager::ValidateComponents() const
{
    const bool bProxyValid = DirectLinkProxyRef.IsValid();
    const bool bActorValid = DatasmithRuntimeActorRef.IsValid();
    const bool bWorldValid = IsValid(GetWorld());

    if (!bProxyValid)
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("DirectLink proxy is invalid"));
    }
    if (!bActorValid)
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("Datasmith runtime actor is invalid"));
    }
    if (!bWorldValid)
    {
        UE_LOG(LogDSRuntimeManager, Warning, TEXT("World reference is invalid"));
    }

    return bProxyValid && bActorValid && bWorldValid;
}

FDatasmithRuntimeImportOptions ADSRuntimeManager::CreateImportOptionsFromSettings() const
{
    FDatasmithRuntimeImportOptions ImportOptions;

    // Configure tessellation options
    ImportOptions.TessellationOptions = FDatasmithTessellationOptions{
        ChordTolerance,
        MaxEdgeLength,
        NormalTolerance,
        StitchingTechnique
    };

    // Configure other options
    ImportOptions.BuildHierarchy = HierarchyMethod;
    ImportOptions.BuildCollisions = CollisionEnabled;
    ImportOptions.CollisionType = CollisionTraceFlag;
    ImportOptions.bImportMetaData = bImportMetaData;

    return ImportOptions;
}

void ADSRuntimeManager::LogCurrentConfiguration() const
{
    UE_LOG(LogDSRuntimeManager, Log, TEXT("=== DSRuntimeManager Configuration ==="));
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Tessellation - Chord Tolerance: %f"), ChordTolerance);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Tessellation - Max Edge Length: %f"), MaxEdgeLength);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Tessellation - Normal Tolerance: %f"), NormalTolerance);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Tessellation - Stitching Technique: %d"), (int32)StitchingTechnique);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Hierarchy Method: %d"), (int32)HierarchyMethod);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Collision Enabled: %d"), (int32)CollisionEnabled.GetValue());
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Collision Trace Flag: %d"), (int32)CollisionTraceFlag.GetValue());
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Import Metadata: %s"), bImportMetaData ? TEXT("true") : TEXT("false"));
    UE_LOG(LogDSRuntimeManager, Log, TEXT("DirectLink Source Index: %d"), DirectLinkSourceIndex);
    UE_LOG(LogDSRuntimeManager, Log, TEXT("Available Sources: %d"), GetAvailableSourceCount());
    UE_LOG(LogDSRuntimeManager, Log, TEXT("====================================="));
}