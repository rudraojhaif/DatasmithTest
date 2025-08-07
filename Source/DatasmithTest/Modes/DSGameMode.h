#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DSGameMode.generated.h"

// Forward declarations
class ADSPlayerController;
class ADSPawn;
class ADSRuntimeManager;
class ADSLightSyncer;
/**
 * ADSGameMode - Custom game mode for Datasmith runtime testing
 * 
 * This game mode sets up the appropriate default classes for:
 * - Player Controller (DSPlayerController for input handling)
 * - Default Pawn (DSPawn for editor-style movement)
 * - HUD class for UI management
 * 
 * The game mode also automatically spawns a DSRuntimeManager instance
 * if one doesn't exist in the world when the game starts.
 */
UCLASS(BlueprintType, Blueprintable)
class DATASMITHTEST_API ADSGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADSGameMode();

protected:
    virtual void BeginPlay() override;

    /**
     * Class reference for the DSRuntimeManager to spawn if none exists
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Datasmith")
    TSubclassOf<ADSRuntimeManager> DSRuntimeManagerClass;

    /**
     * Class reference for the DSLightSyncer to spawn if none exists
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Datasmith")
    TSubclassOf<ADSLightSyncer> DSLightSyncerClass;
    
    /**
     * Transform to use when spawning the DSRuntimeManager
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Datasmith")
    FTransform DSRuntimeManagerSpawnTransform;

    /**
     * Whether to automatically spawn a DSRuntimeManager if none exists in the world
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Datasmith")
    bool bAutoSpawnDSRuntimeManager = true;

    /**
     * Whether to automatically spawn a DSLightSyncer if none exists in the world
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Datasmith")
    bool bAutoSpawnDSLightSyncer = true;

private:
    /**
     * Spawns a DSRuntimeManager if none exists and auto-spawn is enabled
     * @return True if a manager was spawned or already exists
     */
    bool EnsureDSRuntimeManagerExists();

    /**
     * Spawns a DSLightSyncer if none exists and auto-spawn is enabled
     * @return True if a actor was spawned or already exists
     */
    bool EnsureDSLightSyncerExists();

public:
    /**
     * Gets the DSRuntimeManager instance in the world
     * @return Pointer to DSRuntimeManager, or nullptr if none exists
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith")
    ADSRuntimeManager* GetDSRuntimeManager() const;

    /**
     * Manually spawns a DSRuntimeManager at the specified transform
     * @param SpawnTransform The transform to spawn the manager at
     * @return The spawned DSRuntimeManager, or nullptr if spawn failed
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith")
    ADSRuntimeManager* SpawnDSRuntimeManager(const FTransform& SpawnTransform);


    /**
     * Gets the DSLightSyncer instance in the world
     * @return Pointer to DSLightSyncer, or nullptr if none exists
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith")
    ADSLightSyncer* GetDSLightSyncer() const;
    
    /**
     * Manually spawns a DSLightSyncer at the specified transform
     * @param SpawnTransform The transform to spawn the syncer at
     * @return The spawned DSLightSyncer, or nullptr if spawn failed
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith")
    ADSLightSyncer* SpawnDSLightSyncer(const FTransform& SpawnTransform);
};