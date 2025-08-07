#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "DatasmithRuntime.h"
#include "DSRuntimeWidget.generated.h"

// Forward declarations
class ADSPawn;
class ADSRuntimeManager;
class ADSLightSyncer;
/**
 * UDSRuntimeWidget - Main configuration widget for Datasmith Runtime settings
 * 
 * This widget provides a comprehensive interface for configuring:
 * - DSPawn movement speed
 * - Datasmith tessellation settings
 * - Import hierarchy options
 * - Collision settings
 * - DirectLink connection management
 * - Raytracing graphics settings
 * 
 * The widget automatically finds and connects to the current DSPawn and DSRuntimeManager
 * instances in the world when initialized.
 */
UCLASS(BlueprintType, Blueprintable)
class DATASMITHTEST_API UDSRuntimeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UDSRuntimeWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // === UI Components ===
    
    // Pawn Movement Controls
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> MaxSpeedTextBox;

    // Tessellation Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ChordToleranceTextBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> MaxEdgeLengthTextBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> NormalToleranceTextBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> StitchingTechniqueComboBox;

    // Hierarchy Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> HierarchyMethodComboBox;

    // Collision Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> CollisionEnabledComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> CollisionTraceFlagComboBox;

    // Metadata Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> ImportMetadataCheckBox;

    // Raytracing Graphics Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> RaytracingShadowsCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> RaytracingAmbientOcclusionCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> RaytracingGlobalIlluminationCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> RaytracingReflectionsCheckBox;

    // DirectLink Settings
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> DirectLinkSourceComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AvailableSourcesTextBlock;

    // Control Buttons
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> UpdateDirectLinkButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RefreshSourcesButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplySettingsButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CloseButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SyncLightButton;

private:
    // === Component References ===
    
    /** Weak reference to the current DSPawn in the world */
    UPROPERTY()
    TWeakObjectPtr<ADSPawn> CurrentDSPawn;

    /** Weak reference to the current DSRuntimeManager in the world */
    UPROPERTY()
    TWeakObjectPtr<ADSRuntimeManager> CurrentDSRuntimeManager;

    /** Weak reference to the current LightSyncer in the world */
    UPROPERTY()
    TWeakObjectPtr<ADSLightSyncer> CurrentDSLightSyncer;
    
    /** Flag to prevent recursive updates when setting values */
    bool bIsUpdatingValues = false;

    // === Raytracing Settings Storage ===
    
    /** Current raytracing shadows state */
    bool bRaytracingShadowsEnabled = true;

    /** Current raytracing ambient occlusion state */
    bool bRaytracingAmbientOcclusionEnabled = true;

    /** Current raytracing global illumination state */
    bool bRaytracingGlobalIlluminationEnabled = true;

    /** Current raytracing reflections state */
    bool bRaytracingReflectionsEnabled = true;

public:
    // === Public Interface ===

    /**
     * Shows the widget and refreshes all values from current game objects
     */
    UFUNCTION(BlueprintCallable, Category = "DS Runtime Widget")
    void ShowWidget();

    /**
     * Hides the widget and releases focus back to the game
     */
    UFUNCTION(BlueprintCallable, Category = "DS Runtime Widget")
    void HideWidget();

    /**
     * Toggles widget visibility
     */
    UFUNCTION(BlueprintCallable, Category = "DS Runtime Widget")
    void ToggleWidget();

private:
    // === Initialization Methods ===

    /**
     * Finds and caches references to DSPawn and DSRuntimeManager in the current world
     * @return True if both components were found successfully
     */
    bool FindGameComponents();

    /**
     * Initializes all combo boxes with their respective enum values
     */
    void InitializeComboBoxes();

    /**
     * Populates the stitching technique combo box with available options
     */
    void PopulateStitchingTechniqueComboBox();

    /**
     * Populates the hierarchy method combo box with available options
     */
    void PopulateHierarchyMethodComboBox();

    /**
     * Populates the collision enabled combo box with available options
     */
    void PopulateCollisionEnabledComboBox();

    /**
     * Populates the collision trace flag combo box with available options
     */
    void PopulateCollisionTraceFlagComboBox();

    /**
     * Refreshes the DirectLink source combo box with current available sources
     */
    void RefreshDirectLinkSources();

    // === Value Update Methods ===

    /**
     * Refreshes all widget values from the current game components
     */
    void RefreshAllValues();

    /**
     * Updates pawn-related values in the widget
     */
    void RefreshPawnValues();

    /**
     * Updates Datasmith-related values in the widget
     */
    void RefreshDatasmithValues();

    /**
     * Updates raytracing-related values in the widget
     */
    void RefreshRaytracingValues();

    // === Event Handlers - Text Input ===

    /**
     * Called when the max speed text box is committed (Enter pressed or focus lost)
     */
    UFUNCTION()
    void OnMaxSpeedCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /**
     * Called when the chord tolerance text box is committed
     */
    UFUNCTION()
    void OnChordToleranceCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /**
     * Called when the max edge length text box is committed
     */
    UFUNCTION()
    void OnMaxEdgeLengthCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /**
     * Called when the normal tolerance text box is committed
     */
    UFUNCTION()
    void OnNormalToleranceCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    // === Event Handlers - Combo Boxes ===

    /**
     * Called when the stitching technique combo box selection changes
     */
    UFUNCTION()
    void OnStitchingTechniqueChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /**
     * Called when the hierarchy method combo box selection changes
     */
    UFUNCTION()
    void OnHierarchyMethodChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /**
     * Called when the collision enabled combo box selection changes
     */
    UFUNCTION()
    void OnCollisionEnabledChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /**
     * Called when the collision trace flag combo box selection changes
     */
    UFUNCTION()
    void OnCollisionTraceFlagChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /**
     * Called when the DirectLink source combo box selection changes
     */
    UFUNCTION()
    void OnDirectLinkSourceChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    // === Event Handlers - Check Boxes ===

    /**
     * Called when the import metadata checkbox state changes
     */
    UFUNCTION()
    void OnImportMetadataChanged(bool bIsChecked);

    /**
     * Called when the raytracing shadows checkbox state changes
     */
    UFUNCTION()
    void OnRaytracingShadowsChanged(bool bIsChecked);

    /**
     * Called when the raytracing ambient occlusion checkbox state changes
     */
    UFUNCTION()
    void OnRaytracingAmbientOcclusionChanged(bool bIsChecked);

    /**
     * Called when the raytracing global illumination checkbox state changes
     */
    UFUNCTION()
    void OnRaytracingGlobalIlluminationChanged(bool bIsChecked);

    /**
     * Called when the raytracing reflections checkbox state changes
     */
    UFUNCTION()
    void OnRaytracingReflectionsChanged(bool bIsChecked);

    // === Event Handlers - Buttons ===

    /**
     * Called when the Update DirectLink button is clicked
     */
    UFUNCTION()
    void OnUpdateDirectLinkClicked();

    /**
     * Called when the Refresh Sources button is clicked
     */
    UFUNCTION()
    void OnRefreshSourcesClicked();

    /**
     * Called when the Apply Settings button is clicked
     */
    UFUNCTION()
    void OnApplySettingsClicked();

    /**
     * Called when the Close button is clicked
     */
    UFUNCTION()
    void OnCloseClicked();
    
    /**
     * Called when Light sync is pressed
     */
    UFUNCTION()
    void OnLightSyncPressed();

    // === Utility Methods ===

    /**
     * Converts enum value to display string for stitching technique
     */
    FString StitchingTechniqueToString(EDatasmithCADStitchingTechnique Technique) const;

    /**
     * Converts display string to enum value for stitching technique
     */
    EDatasmithCADStitchingTechnique StringToStitchingTechnique(const FString& String) const;

    /**
     * Converts enum value to display string for hierarchy method
     */
    FString HierarchyMethodToString(EBuildHierarchyMethod Method) const;

    /**
     * Converts display string to enum value for hierarchy method
     */
    EBuildHierarchyMethod StringToHierarchyMethod(const FString& String) const;

    /**
     * Converts enum value to display string for collision enabled
     */
    FString CollisionEnabledToString(ECollisionEnabled::Type CollisionType) const;

    /**
     * Converts display string to enum value for collision enabled
     */
    ECollisionEnabled::Type StringToCollisionEnabled(const FString& String) const;

    /**
     * Converts enum value to display string for collision trace flag
     */
    FString CollisionTraceFlagToString(ECollisionTraceFlag TraceFlag) const;

    /**
     * Converts display string to enum value for collision trace flag
     */
    ECollisionTraceFlag StringToCollisionTraceFlag(const FString& String) const;

    /**
     * Validates and parses a float value from text input
     * @param Text The input text to parse
     * @param OutValue The parsed float value (only valid if return is true)
     * @return True if parsing was successful
     */
    bool ValidateFloatInput(const FText& Text, float& OutValue) const;

    /**
     * Validates and parses an integer value from text input
     * @param Text The input text to parse
     * @param OutValue The parsed integer value (only valid if return is true)
     * @return True if parsing was successful
     */
    bool ValidateIntInput(const FText& Text, int32& OutValue) const;

    /**
     * Logs an error message with consistent formatting
     */
    void LogError(const FString& Message) const;

    /**
     * Logs a warning message with consistent formatting
     */
    void LogWarning(const FString& Message) const;

    // === Raytracing Utility Methods ===

    /**
     * Applies the current raytracing shadows setting
     */
    void ApplyRaytracingShadowsSetting();

    /**
     * Applies the current raytracing ambient occlusion setting
     */
    void ApplyRaytracingAmbientOcclusionSetting();

    /**
     * Applies the current raytracing global illumination setting
     */
    void ApplyRaytracingGlobalIlluminationSetting();

    /**
     * Applies the current raytracing reflections setting
     */
    void ApplyRaytracingReflectionsSetting();
};