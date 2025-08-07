#include "DSRuntimeWidget.h"
#include "../Pawns/DSPawn.h"
#include "../Actors/DSRuntimeManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "DatasmithTest/Actors/DSLightSyncer.h"
#include "HAL/IConsoleManager.h"

// Logging category for this widget
DEFINE_LOG_CATEGORY_STATIC(LogDSRuntimeWidget, Log, All);

UDSRuntimeWidget::UDSRuntimeWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Constructor delegates to parent class initialization
}

void UDSRuntimeWidget::NativeConstruct()
{
    // Call parent implementation first
    Super::NativeConstruct();

    UE_LOG(LogDSRuntimeWidget, Log, TEXT("DSRuntimeWidget: Native construct started"));

    // Locate and cache references to game objects we'll be working with
    if (!FindGameComponents())
    {
        LogError(TEXT("Failed to find required game components during widget construction"));
    }

    // Set up all dropdown menus with their available options
    InitializeComboBoxes();

    // === TEXT INPUT FIELD EVENT BINDING ===
    // Bind event handlers for numeric input fields that control various parameters
    if (MaxSpeedTextBox)
    {
        MaxSpeedTextBox->OnTextCommitted.AddDynamic(this, &UDSRuntimeWidget::OnMaxSpeedCommitted);
    }

    if (ChordToleranceTextBox)
    {
        ChordToleranceTextBox->OnTextCommitted.AddDynamic(this, &UDSRuntimeWidget::OnChordToleranceCommitted);
    }

    if (MaxEdgeLengthTextBox)
    {
        MaxEdgeLengthTextBox->OnTextCommitted.AddDynamic(this, &UDSRuntimeWidget::OnMaxEdgeLengthCommitted);
    }

    if (NormalToleranceTextBox)
    {
        NormalToleranceTextBox->OnTextCommitted.AddDynamic(this, &UDSRuntimeWidget::OnNormalToleranceCommitted);
    }

    // === DROPDOWN COMBO BOX EVENT BINDING ===
    // Set up event handlers for all dropdown selection changes
    if (StitchingTechniqueComboBox)
    {
        StitchingTechniqueComboBox->OnSelectionChanged.AddDynamic(this, &UDSRuntimeWidget::OnStitchingTechniqueChanged);
    }

    if (HierarchyMethodComboBox)
    {
        HierarchyMethodComboBox->OnSelectionChanged.AddDynamic(this, &UDSRuntimeWidget::OnHierarchyMethodChanged);
    }

    if (CollisionEnabledComboBox)
    {
        CollisionEnabledComboBox->OnSelectionChanged.AddDynamic(this, &UDSRuntimeWidget::OnCollisionEnabledChanged);
    }

    if (CollisionTraceFlagComboBox)
    {
        CollisionTraceFlagComboBox->OnSelectionChanged.AddDynamic(this, &UDSRuntimeWidget::OnCollisionTraceFlagChanged);
    }

    if (DirectLinkSourceComboBox)
    {
        DirectLinkSourceComboBox->OnSelectionChanged.AddDynamic(this, &UDSRuntimeWidget::OnDirectLinkSourceChanged);
    }

    // === CHECKBOX EVENT BINDING ===
    // Bind metadata import checkbox
    if (ImportMetadataCheckBox)
    {
        ImportMetadataCheckBox->OnCheckStateChanged.AddDynamic(this, &UDSRuntimeWidget::OnImportMetadataChanged);
    }

    // Bind all raytracing feature toggle checkboxes
    if (RaytracingShadowsCheckBox)
    {
        RaytracingShadowsCheckBox->OnCheckStateChanged.AddDynamic(this, &UDSRuntimeWidget::OnRaytracingShadowsChanged);
    }

    if (RaytracingAmbientOcclusionCheckBox)
    {
        RaytracingAmbientOcclusionCheckBox->OnCheckStateChanged.AddDynamic(this, &UDSRuntimeWidget::OnRaytracingAmbientOcclusionChanged);
    }

    if (RaytracingGlobalIlluminationCheckBox)
    {
        RaytracingGlobalIlluminationCheckBox->OnCheckStateChanged.AddDynamic(this, &UDSRuntimeWidget::OnRaytracingGlobalIlluminationChanged);
    }

    if (RaytracingReflectionsCheckBox)
    {
        RaytracingReflectionsCheckBox->OnCheckStateChanged.AddDynamic(this, &UDSRuntimeWidget::OnRaytracingReflectionsChanged);
    }

    // === BUTTON EVENT BINDING ===
    // Connect all action buttons to their respective handlers
    if (UpdateDirectLinkButton)
    {
        UpdateDirectLinkButton->OnClicked.AddDynamic(this, &UDSRuntimeWidget::OnUpdateDirectLinkClicked);
    }

    if (RefreshSourcesButton)
    {
        RefreshSourcesButton->OnClicked.AddDynamic(this, &UDSRuntimeWidget::OnRefreshSourcesClicked);
    }

    if (ApplySettingsButton)
    {
        ApplySettingsButton->OnClicked.AddDynamic(this, &UDSRuntimeWidget::OnApplySettingsClicked);
    }

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UDSRuntimeWidget::OnCloseClicked);
    }

    if (SyncLightButton)
    {
        SyncLightButton->OnClicked.AddDynamic(this, &UDSRuntimeWidget::OnLightSyncPressed);
    }

    // Load current values from game objects and populate all UI controls
    RefreshAllValues();

    UE_LOG(LogDSRuntimeWidget, Log, TEXT("DSRuntimeWidget: Native construct completed"));
}

void UDSRuntimeWidget::NativeDestruct()
{
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("DSRuntimeWidget: Native destruct"));

    // Clean up our cached references to prevent dangling pointers
    CurrentDSPawn.Reset();
    CurrentDSRuntimeManager.Reset();
    CurrentDSLightSyncer.Reset();

    // Call parent cleanup
    Super::NativeDestruct();
}

void UDSRuntimeWidget::ShowWidget()
{
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Showing DSRuntimeWidget"));

    // Refresh our references to game objects in case they've changed
    FindGameComponents();
    
    // Update all UI elements with current values before showing
    RefreshAllValues();

    // Make the widget visible to the user
    SetVisibility(ESlateVisibility::Visible);
    
    // Configure input handling to allow both game and UI interaction
    if (APlayerController* PC = GetOwningPlayer())
    {
        // Set up hybrid input mode that allows both game controls and UI interaction
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        // Enable mouse cursor for UI interaction
        PC->bShowMouseCursor = true;
    }
}

void UDSRuntimeWidget::HideWidget()
{
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Hiding DSRuntimeWidget"));

    // Make the widget invisible
    SetVisibility(ESlateVisibility::Hidden);

    // Restore game-only input mode when hiding UI
    if (APlayerController* PC = GetOwningPlayer())
    {
        // Switch back to game-only input (no UI interaction)
        PC->SetInputMode(FInputModeGameOnly());
        // Hide mouse cursor since we're back in game mode
        PC->bShowMouseCursor = false;
    }
}

void UDSRuntimeWidget::ToggleWidget()
{
    // Check current visibility state and toggle accordingly
    if (GetVisibility() == ESlateVisibility::Visible)
    {
        HideWidget();
    }
    else
    {
        ShowWidget();
    }
}

bool UDSRuntimeWidget::FindGameComponents()
{
    // Get the current world context
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        LogError(TEXT("Invalid world reference when finding game components"));
        return false;
    }

    bool bFoundAll = true;

    // === FIND PLAYER'S PAWN ===
    // Look for the current player's pawn and verify it's our custom DSPawn type
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
    {
        // Try to cast the possessed pawn to our specific DSPawn type
        if (ADSPawn* DSPawn = Cast<ADSPawn>(PC->GetPawn()))
        {
            // Cache the reference for later use
            CurrentDSPawn = DSPawn;
            UE_LOG(LogDSRuntimeWidget, Log, TEXT("Found DSPawn: %s"), *DSPawn->GetName());
        }
        else
        {
            // Player has a pawn but it's not our expected type
            LogWarning(TEXT("Player pawn is not a DSPawn"));
            bFoundAll = false;
        }
    }
    else
    {
        // No player controller found in the world
        LogWarning(TEXT("No player controller found"));
        bFoundAll = false;
    }

    // === FIND DATASMITH RUNTIME MANAGER ===
    // Search the world for our runtime manager actor
    for (TActorIterator<ADSRuntimeManager> ActorItr(World); ActorItr; ++ActorItr)
    {
        // Cache reference to the first manager we find
        CurrentDSRuntimeManager = *ActorItr;
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Found DSRuntimeManager: %s"), *ActorItr->GetName());
        break; // Use the first one found (should only be one per level)
    }

    // Check if we successfully found the runtime manager
    if (!CurrentDSRuntimeManager.IsValid())
    {
        LogWarning(TEXT("No DSRuntimeManager found in world"));
        bFoundAll = false;
    }

    // === FIND Light Syncer ===
    // Search the world for our light syncer actor
    for (TActorIterator<ADSLightSyncer> ActorItr(World); ActorItr; ++ActorItr)
    {
        // Cache reference to the first actor we find
        CurrentDSLightSyncer = *ActorItr;
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Found Light syncer: %s"), *ActorItr->GetName());
        break; // Use the first one found (should only be one per level)
    }

    // Check if we successfully found the light syncer
    if (!CurrentDSLightSyncer.IsValid())
    {
        LogWarning(TEXT("No Light Syncer found in world"));
        bFoundAll = false;
    }
    
    return bFoundAll;
}

void UDSRuntimeWidget::InitializeComboBoxes()
{
    UE_LOG(LogDSRuntimeWidget, VeryVerbose, TEXT("Initializing combo boxes"));

    // Set up all dropdown menus with their available options
    PopulateStitchingTechniqueComboBox();
    PopulateHierarchyMethodComboBox();
    PopulateCollisionEnabledComboBox();
    PopulateCollisionTraceFlagComboBox();
    // Update DirectLink sources list (may change dynamically)
    RefreshDirectLinkSources();
}

void UDSRuntimeWidget::PopulateStitchingTechniqueComboBox()
{
    // Early exit if combo box doesn't exist
    if (!StitchingTechniqueComboBox)
    {
        return;
    }

    // Clear any existing options first
    StitchingTechniqueComboBox->ClearOptions();
    // Add all available stitching technique options
    StitchingTechniqueComboBox->AddOption(TEXT("Stitching Sew"));
    StitchingTechniqueComboBox->AddOption(TEXT("Stitching Heal"));
    StitchingTechniqueComboBox->AddOption(TEXT("Stitching None"));
}

void UDSRuntimeWidget::PopulateHierarchyMethodComboBox()
{
    // Early exit if combo box doesn't exist
    if (!HierarchyMethodComboBox)
    {
        return;
    }

    // Clear existing options and populate with hierarchy method choices
    HierarchyMethodComboBox->ClearOptions();
    HierarchyMethodComboBox->AddOption(TEXT("Unfiltered"));
    HierarchyMethodComboBox->AddOption(TEXT("Simplified"));
    HierarchyMethodComboBox->AddOption(TEXT("None"));
}

void UDSRuntimeWidget::PopulateCollisionEnabledComboBox()
{
    // Early exit if combo box doesn't exist
    if (!CollisionEnabledComboBox)
    {
        return;
    }

    // Set up collision type options
    CollisionEnabledComboBox->ClearOptions();
    CollisionEnabledComboBox->AddOption(TEXT("No Collision"));
    CollisionEnabledComboBox->AddOption(TEXT("Query Only"));
    CollisionEnabledComboBox->AddOption(TEXT("Physics Only"));
    CollisionEnabledComboBox->AddOption(TEXT("Query and Physics"));
}

void UDSRuntimeWidget::PopulateCollisionTraceFlagComboBox()
{
    // Early exit if combo box doesn't exist
    if (!CollisionTraceFlagComboBox)
    {
        return;
    }

    // Set up collision trace flag options
    CollisionTraceFlagComboBox->ClearOptions();
    CollisionTraceFlagComboBox->AddOption(TEXT("Use Default"));
    CollisionTraceFlagComboBox->AddOption(TEXT("Use Simple as Complex"));
    CollisionTraceFlagComboBox->AddOption(TEXT("Use Complex as Simple"));
}

void UDSRuntimeWidget::RefreshDirectLinkSources()
{
    // Ensure we have both UI element and manager reference
    if (!DirectLinkSourceComboBox || !CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    // Clear existing source list
    DirectLinkSourceComboBox->ClearOptions();

    // Query the runtime manager for available DirectLink sources
    const int32 SourceCount = CurrentDSRuntimeManager->GetAvailableSourceCount();
    
    if (SourceCount > 0)
    {
        // Add numbered source options for each available source
        for (int32 i = 0; i < SourceCount; ++i)
        {
            DirectLinkSourceComboBox->AddOption(FString::Printf(TEXT("Source %d"), i));
        }
    }
    else
    {
        // Show placeholder when no sources are available
        DirectLinkSourceComboBox->AddOption(TEXT("No Sources Available"));
    }

    // Update the source count display text
    if (AvailableSourcesTextBlock)
    {
        // Ensure we don't show negative source counts
        AvailableSourcesTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Available Sources: %d"), FMath::Max(0, SourceCount))));
    }
}

void UDSRuntimeWidget::RefreshAllValues()
{
    // Prevent recursive updates that could cause infinite loops
    if (bIsUpdatingValues)
    {
        return;
    }

    // Set flag to prevent recursive calls during value updates
    bIsUpdatingValues = true;

    // Update all UI sections with current values from game objects
    RefreshPawnValues();
    RefreshDatasmithValues();
    RefreshRaytracingValues();
    RefreshDirectLinkSources();

    // Clear the update flag
    bIsUpdatingValues = false;
}

void UDSRuntimeWidget::RefreshPawnValues()
{
    // Ensure we have valid references before proceeding
    if (!CurrentDSPawn.IsValid() || !MaxSpeedTextBox)
    {
        return;
    }

    // Get current max speed from the pawn and display it in the text box
    const float CurrentMaxSpeed = CurrentDSPawn->GetMaxSpeed();
    MaxSpeedTextBox->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), CurrentMaxSpeed)));
}

void UDSRuntimeWidget::RefreshDatasmithValues()
{
    // Early exit if we don't have a valid runtime manager
    if (!CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    // === UPDATE TESSELLATION PARAMETER VALUES ===
    // These control how CAD geometry is converted to mesh data
    if (ChordToleranceTextBox)
    {
        ChordToleranceTextBox->SetText(FText::FromString(FString::Printf(TEXT("%.3f"), CurrentDSRuntimeManager->GetChordTolerance())));
    }

    if (MaxEdgeLengthTextBox)
    {
        MaxEdgeLengthTextBox->SetText(FText::FromString(FString::Printf(TEXT("%.3f"), CurrentDSRuntimeManager->GetMaxEdgeLength())));
    }

    if (NormalToleranceTextBox)
    {
        NormalToleranceTextBox->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), CurrentDSRuntimeManager->GetNormalTolerance())));
    }

    // === UPDATE GEOMETRY PROCESSING OPTIONS ===
    if (StitchingTechniqueComboBox)
    {
        // Convert enum value to display string and set selection
        StitchingTechniqueComboBox->SetSelectedOption(StitchingTechniqueToString(CurrentDSRuntimeManager->GetStitchingTechnique()));
    }

    // === UPDATE HIERARCHY BUILDING SETTINGS ===
    if (HierarchyMethodComboBox)
    {
        // Convert enum to string and update selection
        HierarchyMethodComboBox->SetSelectedOption(HierarchyMethodToString(CurrentDSRuntimeManager->GetHierarchyMethod()));
    }

    // === UPDATE COLLISION DETECTION SETTINGS ===
    if (CollisionEnabledComboBox)
    {
        // Convert collision type enum to display string
        CollisionEnabledComboBox->SetSelectedOption(CollisionEnabledToString(CurrentDSRuntimeManager->GetCollisionEnabled()));
    }

    if (CollisionTraceFlagComboBox)
    {
        // Convert trace flag enum to display string
        CollisionTraceFlagComboBox->SetSelectedOption(CollisionTraceFlagToString(CurrentDSRuntimeManager->GetCollisionTraceFlag()));
    }

    // === UPDATE METADATA IMPORT SETTINGS ===
    if (ImportMetadataCheckBox)
    {
        // Set checkbox state based on current metadata import setting
        ImportMetadataCheckBox->SetIsChecked(CurrentDSRuntimeManager->GetImportMetadata());
    }

    // === UPDATE DIRECTLINK CONNECTION SETTINGS ===
    if (DirectLinkSourceComboBox)
    {
        // Get current source index and format it as display string
        const int32 CurrentIndex = CurrentDSRuntimeManager->GetDirectLinkSourceIndex();
        const FString SelectedOption = FString::Printf(TEXT("Source %d"), CurrentIndex);
        DirectLinkSourceComboBox->SetSelectedOption(SelectedOption);
    }
}

void UDSRuntimeWidget::RefreshRaytracingValues()
{
    // Update all raytracing checkboxes with their current enabled states
    if (RaytracingShadowsCheckBox)
    {
        RaytracingShadowsCheckBox->SetIsChecked(bRaytracingShadowsEnabled);
    }

    if (RaytracingAmbientOcclusionCheckBox)
    {
        RaytracingAmbientOcclusionCheckBox->SetIsChecked(bRaytracingAmbientOcclusionEnabled);
    }

    if (RaytracingGlobalIlluminationCheckBox)
    {
        RaytracingGlobalIlluminationCheckBox->SetIsChecked(bRaytracingGlobalIlluminationEnabled);
    }

    if (RaytracingReflectionsCheckBox)
    {
        RaytracingReflectionsCheckBox->SetIsChecked(bRaytracingReflectionsEnabled);
    }
}

// === Event Handlers - Text Input ===

void UDSRuntimeWidget::OnMaxSpeedCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // Prevent handling during programmatic updates or if pawn is invalid
    if (bIsUpdatingValues || !CurrentDSPawn.IsValid())
    {
        return;
    }

    float NewMaxSpeed;
    // Validate the input text as a valid float value
    if (ValidateFloatInput(Text, NewMaxSpeed))
    {
        // Apply the new speed value to the pawn
        CurrentDSPawn->SetMaxSpeed(NewMaxSpeed);
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set pawn max speed to: %.2f"), NewMaxSpeed);
    }
    else
    {
        // Input was invalid, revert the display to the current actual value
        LogWarning(TEXT("Invalid max speed input, reverting to current value"));
        RefreshPawnValues(); // Revert to current value
    }
}

void UDSRuntimeWidget::OnChordToleranceCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // Skip processing during programmatic updates or if manager is invalid
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    float NewValue;
    // Validate and parse the input value
    if (ValidateFloatInput(Text, NewValue))
    {
        // Apply the new chord tolerance to the runtime manager
        CurrentDSRuntimeManager->SetChordTolerance(NewValue);
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set chord tolerance to: %.3f"), NewValue);
    }
    else
    {
        // Invalid input, restore the current value in the UI
        LogWarning(TEXT("Invalid chord tolerance input, reverting to current value"));
        RefreshDatasmithValues();
    }
}

void UDSRuntimeWidget::OnMaxEdgeLengthCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // Guard against recursive updates and invalid manager state
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    float NewValue;
    // Parse and validate the input
    if (ValidateFloatInput(Text, NewValue))
    {
        // Set the new maximum edge length parameter
        CurrentDSRuntimeManager->SetMaxEdgeLength(NewValue);
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set max edge length to: %.3f"), NewValue);
    }
    else
    {
        // Revert to current value on invalid input
        LogWarning(TEXT("Invalid max edge length input, reverting to current value"));
        RefreshDatasmithValues();
    }
}

void UDSRuntimeWidget::OnNormalToleranceCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // Skip if we're in the middle of updating or manager is invalid
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    float NewValue;
    // Validate the entered tolerance value
    if (ValidateFloatInput(Text, NewValue))
    {
        // Update the normal tolerance setting
        CurrentDSRuntimeManager->SetNormalTolerance(NewValue);
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set normal tolerance to: %.2f"), NewValue);
    }
    else
    {
        // Invalid input - reset display to current actual value
        LogWarning(TEXT("Invalid normal tolerance input, reverting to current value"));
        RefreshDatasmithValues();
    }
}

// === Event Handlers - Combo Boxes ===

void UDSRuntimeWidget::OnStitchingTechniqueChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // Ignore programmatic selection changes and updates during value refresh
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid() || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    // Convert the selected string back to the corresponding enum value
    const EDatasmithCADStitchingTechnique NewTechnique = StringToStitchingTechnique(SelectedItem);
    // Apply the new stitching technique setting
    CurrentDSRuntimeManager->SetStitchingTechnique(NewTechnique);
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set stitching technique to: %s"), *SelectedItem);
}

void UDSRuntimeWidget::OnHierarchyMethodChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // Skip programmatic changes and updates during refresh cycles
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid() || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    // Convert display string to enum value
    const EBuildHierarchyMethod NewMethod = StringToHierarchyMethod(SelectedItem);
    // Update the hierarchy building method
    CurrentDSRuntimeManager->SetHierarchyMethod(NewMethod);
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set hierarchy method to: %s"), *SelectedItem);
}

void UDSRuntimeWidget::OnCollisionEnabledChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // Filter out programmatic selection updates
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid() || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    // Parse the collision type from the selected string
    const ECollisionEnabled::Type NewCollision = StringToCollisionEnabled(SelectedItem);
    // Apply the new collision detection setting
    CurrentDSRuntimeManager->SetCollisionEnabled(NewCollision);
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set collision enabled to: %s"), *SelectedItem);
}

void UDSRuntimeWidget::OnCollisionTraceFlagChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // Only process user-initiated selection changes
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid() || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    // Convert string selection to enum value
    const ECollisionTraceFlag NewTraceFlag = StringToCollisionTraceFlag(SelectedItem);
    // Update the collision trace flag setting
    CurrentDSRuntimeManager->SetCollisionTraceFlag(NewTraceFlag);
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set collision trace flag to: %s"), *SelectedItem);
}

void UDSRuntimeWidget::OnDirectLinkSourceChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // Ignore updates during refresh and programmatic changes
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid() || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    // Parse the source index from "Source X" format string
    FString IndexString;
    if (SelectedItem.Split(TEXT(" "), nullptr, &IndexString))
    {
        int32 NewIndex;
        // Validate the parsed index value
        if (ValidateIntInput(FText::FromString(IndexString), NewIndex))
        {
            // Set the new DirectLink source index
            CurrentDSRuntimeManager->SetDirectLinkSourceIndex(NewIndex);
            UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set DirectLink source index to: %d"), NewIndex);
        }
    }
}

// === Event Handlers - Check Boxes ===

void UDSRuntimeWidget::OnImportMetadataChanged(bool bIsChecked)
{
    // Skip processing during value refresh cycles
    if (bIsUpdatingValues || !CurrentDSRuntimeManager.IsValid())
    {
        return;
    }

    // Update the metadata import flag in the runtime manager
    CurrentDSRuntimeManager->SetImportMetadata(bIsChecked);
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set import metadata to: %s"), bIsChecked ? TEXT("true") : TEXT("false"));
}

void UDSRuntimeWidget::OnRaytracingShadowsChanged(bool bIsChecked)
{
    // Avoid recursive updates during value refresh
    if (bIsUpdatingValues)
    {
        return;
    }

    // Update local state and apply the setting to the rendering system
    bRaytracingShadowsEnabled = bIsChecked;
    ApplyRaytracingShadowsSetting();
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set raytracing shadows to: %s"), bIsChecked ? TEXT("enabled") : TEXT("disabled"));
}

void UDSRuntimeWidget::OnRaytracingAmbientOcclusionChanged(bool bIsChecked)
{
    // Skip during programmatic updates
    if (bIsUpdatingValues)
    {
        return;
    }

    // Store the new state and apply it to the render settings
    bRaytracingAmbientOcclusionEnabled = bIsChecked;
    ApplyRaytracingAmbientOcclusionSetting();
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set raytracing ambient occlusion to: %s"), bIsChecked ? TEXT("enabled") : TEXT("disabled"));
}

void UDSRuntimeWidget::OnRaytracingGlobalIlluminationChanged(bool bIsChecked)
{
    // Only process user-initiated changes
    if (bIsUpdatingValues)
    {
        return;
    }

    // Update local flag and apply to rendering pipeline
    bRaytracingGlobalIlluminationEnabled = bIsChecked;
    ApplyRaytracingGlobalIlluminationSetting();
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set raytracing global illumination to: %s"), bIsChecked ? TEXT("enabled") : TEXT("disabled"));
}

void UDSRuntimeWidget::OnRaytracingReflectionsChanged(bool bIsChecked)
{
    // Guard against recursive updates
    if (bIsUpdatingValues)
    {
        return;
    }

    // Save the new setting and configure the rendering system
    bRaytracingReflectionsEnabled = bIsChecked;
    ApplyRaytracingReflectionsSetting();
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Set raytracing reflections to: %s"), bIsChecked ? TEXT("enabled") : TEXT("disabled"));
}

// === Event Handlers - Buttons ===

void UDSRuntimeWidget::OnUpdateDirectLinkClicked()
{
    // Ensure we have a valid runtime manager before proceeding
    if (!CurrentDSRuntimeManager.IsValid())
    {
        LogError(TEXT("Cannot update DirectLink - no DSRuntimeManager found"));
        return;
    }

    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Updating DirectLink connection..."));
    // Attempt to refresh the DirectLink connection
    const bool bSuccess = CurrentDSRuntimeManager->UpdateDirectLinkConnection();
    
    // Log the result of the update operation
    if (bSuccess)
    {
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("DirectLink connection updated successfully"));
    }
    else
    {
        LogError(TEXT("Failed to update DirectLink connection"));
    }
}

void UDSRuntimeWidget::OnRefreshSourcesClicked()
{
    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Refreshing DirectLink sources..."));
    // Query the runtime manager for updated source list and refresh the dropdown
    RefreshDirectLinkSources();
}

void UDSRuntimeWidget::OnApplySettingsClicked()
{
    // Verify we have a valid runtime manager reference
    if (!CurrentDSRuntimeManager.IsValid())
    {
        LogError(TEXT("Cannot apply settings - no DSRuntimeManager found"));
        return;
    }

    UE_LOG(LogDSRuntimeWidget, Log, TEXT("Applying import settings..."));
    // Tell the runtime manager to apply all current import option settings
    const bool bSuccess = CurrentDSRuntimeManager->ApplyImportOptions();
    
    // Report the outcome of the settings application
    if (bSuccess)
    {
        UE_LOG(LogDSRuntimeWidget, Log, TEXT("Import settings applied successfully"));
    }
    else
    {
        LogError(TEXT("Failed to apply import settings"));
    }
}

void UDSRuntimeWidget::OnCloseClicked()
{
    // Simply hide the widget when close button is pressed
    HideWidget();
}

void UDSRuntimeWidget::OnLightSyncPressed()
{
    if (!CurrentDSLightSyncer.IsValid())
    {
        LogError(TEXT("Cannot update lightsync - no DSLightSyncer found"));
        return;
    }

    CurrentDSLightSyncer->StartTcpListener();
}

// === Utility Methods - Enum Conversions ===

FString UDSRuntimeWidget::StitchingTechniqueToString(EDatasmithCADStitchingTechnique Technique) const
{
    // Convert stitching technique enum values to user-friendly display strings
    switch (Technique)
    {
    case EDatasmithCADStitchingTechnique::StitchingSew:
        return TEXT("Stitching Sew");
    case EDatasmithCADStitchingTechnique::StitchingHeal:
        return TEXT("Stitching Heal");
    case EDatasmithCADStitchingTechnique::StitchingNone:
        return TEXT("Stitching None");
    default:
        // Fall back to default value if enum is unrecognized
        return TEXT("Stitching Sew");
    }
}

EDatasmithCADStitchingTechnique UDSRuntimeWidget::StringToStitchingTechnique(const FString& String) const
{
    // Parse display strings back to their corresponding enum values
    if (String == TEXT("Stitching Heal"))
    {
        return EDatasmithCADStitchingTechnique::StitchingHeal;
    }
    if (String == TEXT("Stitching None"))
    {
        return EDatasmithCADStitchingTechnique::StitchingNone;
    }
    // Default to StitchingSew for any unrecognized strings
    return EDatasmithCADStitchingTechnique::StitchingSew;
}

FString UDSRuntimeWidget::HierarchyMethodToString(EBuildHierarchyMethod Method) const
{
    // Convert hierarchy method enum to display string
    switch (Method)
    {
    case EBuildHierarchyMethod::None:
        return TEXT("None");
    case EBuildHierarchyMethod::Unfiltered:
        return TEXT("Unfiltered");
    case EBuildHierarchyMethod::Simplified:
        return TEXT("Simplified");
    default:
        // Default to Unfiltered if enum value is unexpected
        return TEXT("Unfiltered");
    }
}

EBuildHierarchyMethod UDSRuntimeWidget::StringToHierarchyMethod(const FString& String) const
{
    // Convert display strings back to hierarchy method enum values
    if (String == TEXT("None"))
    {
        return EBuildHierarchyMethod::None;
    }
    if (String == TEXT("Simplified"))
    {
        return EBuildHierarchyMethod::Simplified;
    }
    // Default to Unfiltered for unrecognized strings
    return EBuildHierarchyMethod::Unfiltered;
}

FString UDSRuntimeWidget::CollisionEnabledToString(ECollisionEnabled::Type CollisionType) const
{
    // Convert collision enabled enum to user-readable string
    switch (CollisionType)
    {
    case ECollisionEnabled::NoCollision:
        return TEXT("No Collision");
    case ECollisionEnabled::QueryOnly:
        return TEXT("Query Only");
    case ECollisionEnabled::PhysicsOnly:
        return TEXT("Physics Only");
    case ECollisionEnabled::QueryAndPhysics:
        return TEXT("Query and Physics");
    default:
        // Fall back to no collision for unexpected values
        return TEXT("No Collision");
    }
}

ECollisionEnabled::Type UDSRuntimeWidget::StringToCollisionEnabled(const FString& String) const
{
    // Parse collision type strings back to enum values
    if (String == TEXT("Query Only"))
    {
        return ECollisionEnabled::QueryOnly;
    }
    else if (String == TEXT("Physics Only"))
    {
        return ECollisionEnabled::PhysicsOnly;
    }
    else if (String == TEXT("Query and Physics"))
    {
        return ECollisionEnabled::QueryAndPhysics;
    }
    // Default to no collision for unrecognized strings
    return ECollisionEnabled::NoCollision;
}

FString UDSRuntimeWidget::CollisionTraceFlagToString(ECollisionTraceFlag TraceFlag) const
{
    // Convert collision trace flag enum to display string
    switch (TraceFlag)
    {
    case CTF_UseDefault:
        return TEXT("Use Default");
    case CTF_UseSimpleAsComplex:
        return TEXT("Use Simple as Complex");
    case CTF_UseComplexAsSimple:
        return TEXT("Use Complex as Simple");
    default:
        // Default to standard trace flag behavior
        return TEXT("Use Default");
    }
}

ECollisionTraceFlag UDSRuntimeWidget::StringToCollisionTraceFlag(const FString& String) const
{
    // Convert display strings back to trace flag enum values
    if (String == TEXT("Use Simple as Complex"))
    {
        return CTF_UseSimpleAsComplex;
    }
    else if (String == TEXT("Use Complex as Simple"))
    {
        return CTF_UseComplexAsSimple;
    }
    // Default to using engine default trace behavior
    return CTF_UseDefault;
}

// === Utility Methods - Validation ===

bool UDSRuntimeWidget::ValidateFloatInput(const FText& Text, float& OutValue) const
{
    // Clean up the input string by removing leading/trailing whitespace
    const FString InputString = Text.ToString().TrimStartAndEnd();
    
    // Reject empty input strings
    if (InputString.IsEmpty())
    {
        return false;
    }

    // Check if the string represents a valid numeric value
    if (InputString.IsNumeric())
    {
        // Convert the string to a float value
        OutValue = FCString::Atof(*InputString);
        return true;
    }

    // Input string is not a valid number
    return false;
}

bool UDSRuntimeWidget::ValidateIntInput(const FText& Text, int32& OutValue) const
{
    // Clean up input by trimming whitespace
    const FString InputString = Text.ToString().TrimStartAndEnd();
    
    // Reject empty strings
    if (InputString.IsEmpty())
    {
        return false;
    }

    // Verify the string contains only numeric characters
    if (InputString.IsNumeric())
    {
        // Parse the string as an integer
        OutValue = FCString::Atoi(*InputString);
        return true;
    }

    // String is not a valid integer
    return false;
}

void UDSRuntimeWidget::LogError(const FString& Message) const
{
    // Standardized error logging with widget identification
    UE_LOG(LogDSRuntimeWidget, Error, TEXT("DSRuntimeWidget Error: %s"), *Message);
}

void UDSRuntimeWidget::LogWarning(const FString& Message) const
{
    // Standardized warning logging with widget identification
    UE_LOG(LogDSRuntimeWidget, Warning, TEXT("DSRuntimeWidget Warning: %s"), *Message);
}

// === Raytracing Utility Methods ===

void UDSRuntimeWidget::ApplyRaytracingShadowsSetting()
{
    // Get reference to the raytracing shadows console variable
    static IConsoleVariable* CVarRayTracingShadows = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Shadows"));

    // Apply the setting if the console variable exists
    if (CVarRayTracingShadows)
    {
        // Set console variable value: 1 for enabled, 0 for disabled
        CVarRayTracingShadows->Set(bRaytracingShadowsEnabled ? 1 : 0, ECVF_SetByCode);
    }
}

void UDSRuntimeWidget::ApplyRaytracingAmbientOcclusionSetting()
{
    // Find the ambient occlusion raytracing console variable
    static IConsoleVariable* CVarRayTracingAO = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.AmbientOcclusion"));

    // Update the console variable if it exists
    if (CVarRayTracingAO)
    {
        // Enable or disable raytraced ambient occlusion
        CVarRayTracingAO->Set(bRaytracingAmbientOcclusionEnabled ? 1 : 0, ECVF_SetByCode);
    }
}

void UDSRuntimeWidget::ApplyRaytracingGlobalIlluminationSetting()
{
    // Access the dynamic global illumination method console variable
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
    if (CVar)
    {
        // Set GI method: 1 for raytraced, 0 for traditional methods
        CVar->Set(bRaytracingGlobalIlluminationEnabled ? 1 : 0, ECVF_SetByConsole);
    }
}

void UDSRuntimeWidget::ApplyRaytracingReflectionsSetting()
{
    // Get the Lumen reflections console variable (used for raytraced reflections)
    static IConsoleVariable* CVarRayTracingReflections = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.Allow"));
    
    // Apply the reflections setting if the console variable is available
    if (CVarRayTracingReflections)
    {
        // Enable or disable raytraced reflections through Lumen
        CVarRayTracingReflections->Set(bRaytracingReflectionsEnabled ? 1 : 0, ECVF_SetByCode);
    }
}