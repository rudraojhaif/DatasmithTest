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
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DatasmithRuntime.h"
#include "DatasmithRuntimeBlueprintLibrary.h"
#include "DSRuntimeManager.generated.h"

// Forward declarations
class ADatasmithRuntimeActor;
class UDirectLinkProxy;

/**
 * ADSRuntimeManager - Manages Datasmith runtime imports and DirectLink connections
 * 
 * This actor provides a centralized manager for handling Datasmith content importing
 * at runtime with configurable transform, collision, and tessellation settings.
 * It also manages DirectLink connections for live updates from external applications.
 * 
 * Key Features:
 * - Runtime spawning of Datasmith content
 * - Configurable import options (tessellation, collision, hierarchy)
 * - DirectLink connection management for live updates
 * - Blueprint-accessible interface for runtime configuration
 */
UCLASS(BlueprintType, Blueprintable, Category = "Datasmith")
class DATASMITHTEST_API ADSRuntimeManager : public AActor
{
    GENERATED_BODY()

public:
    ADSRuntimeManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // Core component references
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> DefaultRootComponent;

    // Datasmith runtime actor reference - using weak pointer for safety
    UPROPERTY()
    TWeakObjectPtr<ADatasmithRuntimeActor> DatasmithRuntimeActorRef;

    // DirectLink proxy reference - using weak pointer for safety
    UPROPERTY()
    TWeakObjectPtr<UDirectLinkProxy> DirectLinkProxyRef;

    // Import Options - Tessellation Settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Tessellation", 
              meta = (AllowPrivateAccess = "true", ClampMin = "0.001", ClampMax = "10.0"))
    float ChordTolerance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Tessellation", 
              meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "100.0"))
    float MaxEdgeLength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Tessellation", 
              meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "90.0"))
    float NormalTolerance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Tessellation", 
              meta = (AllowPrivateAccess = "true"))
    EDatasmithCADStitchingTechnique StitchingTechnique = EDatasmithCADStitchingTechnique::StitchingSew;

    // Import Options - Hierarchy Settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Hierarchy", 
              meta = (AllowPrivateAccess = "true"))
    EBuildHierarchyMethod HierarchyMethod = EBuildHierarchyMethod::Unfiltered;

    // Import Options - Collision Settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Collision", 
              meta = (AllowPrivateAccess = "true"))
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::NoCollision;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Collision", 
              meta = (AllowPrivateAccess = "true"))
    TEnumAsByte<ECollisionTraceFlag> CollisionTraceFlag = CTF_UseComplexAsSimple;

    // Import Options - Metadata Settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import Options|Metadata", 
              meta = (AllowPrivateAccess = "true"))
    bool bImportMetaData = true;

    // DirectLink Settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DirectLink", 
              meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 DirectLinkSourceIndex = 0;

public:
    // Core functionality
    /**
     * Performs a DirectLink update with the current proxy and sources
     * @return True if the update was successful, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith|DirectLink")
    bool UpdateDirectLinkConnection();

    /**
     * Initializes or reinitializes the Datasmith runtime actor with current settings
     * @return True if initialization was successful
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith|Runtime")
    bool InitializeDatasmithActor();

    /**
     * Refreshes the DirectLink proxy reference
     * @return True if proxy was successfully obtained
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith|DirectLink")
    bool RefreshDirectLinkProxy();

    /**
     * Gets the number of available DirectLink sources
     * @return Number of available sources, -1 if proxy is invalid
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|DirectLink")
    int32 GetAvailableSourceCount() const;

    // Tessellation Settings - Getters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Tessellation")
    float GetChordTolerance() const { return ChordTolerance; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Tessellation")
    float GetMaxEdgeLength() const { return MaxEdgeLength; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Tessellation")
    float GetNormalTolerance() const { return NormalTolerance; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Tessellation")
    EDatasmithCADStitchingTechnique GetStitchingTechnique() const { return StitchingTechnique; }

    // Tessellation Settings - Setters
    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Tessellation")
    void SetChordTolerance(float InChordTolerance);

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Tessellation")
    void SetMaxEdgeLength(float InMaxEdgeLength);

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Tessellation")
    void SetNormalTolerance(float InNormalTolerance);

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Tessellation")
    void SetStitchingTechnique(EDatasmithCADStitchingTechnique InStitchingTechnique);

    // Hierarchy Settings - Getters/Setters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Hierarchy")
    EBuildHierarchyMethod GetHierarchyMethod() const { return HierarchyMethod; }

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Hierarchy")
    void SetHierarchyMethod(EBuildHierarchyMethod InHierarchyMethod);

    // Collision Settings - Getters/Setters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Collision")
    ECollisionEnabled::Type GetCollisionEnabled() const { return CollisionEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Collision")
    void SetCollisionEnabled(ECollisionEnabled::Type InCollisionEnabled);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Collision")
    ECollisionTraceFlag GetCollisionTraceFlag() const { return CollisionTraceFlag; }

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Collision")
    void SetCollisionTraceFlag(ECollisionTraceFlag InCollisionTraceFlag);

    // Metadata Settings - Getters/Setters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|Import Options|Metadata")
    bool GetImportMetadata() const { return bImportMetaData; }

    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options|Metadata")
    void SetImportMetadata(bool bInImportMetadata);

    // DirectLink Settings - Getters/Setters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Datasmith|DirectLink")
    int32 GetDirectLinkSourceIndex() const { return DirectLinkSourceIndex; }

    UFUNCTION(BlueprintCallable, Category = "Datasmith|DirectLink")
    void SetDirectLinkSourceIndex(int32 InSourceIndex);

    /**
     * Applies current import options to the Datasmith actor
     * @return True if options were successfully applied
     */
    UFUNCTION(BlueprintCallable, Category = "Datasmith|Import Options")
    bool ApplyImportOptions();

private:
    /**
     * Validates that all required components and references are valid
     * @return True if all components are valid and ready for use
     */
    bool ValidateComponents() const;

    /**
     * Creates the import options structure from current settings
     * @return Configured import options structure
     */
    FDatasmithRuntimeImportOptions CreateImportOptionsFromSettings() const;

    /**
     * Logs current configuration state for debugging purposes
     */
    void LogCurrentConfiguration() const;
};