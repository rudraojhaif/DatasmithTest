#include "DSPlayerController.h"
#include "../Widgets/DSRuntimeWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

// Logging category for this player controller
DEFINE_LOG_CATEGORY_STATIC(LogDSPlayerController, Log, All);

ADSPlayerController::ADSPlayerController()
{
    // Enable input for this player controller
    bShowMouseCursor = false;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    // Set default widget class
    DSRuntimeWidgetClass = UDSRuntimeWidget::StaticClass();

    UE_LOG(LogDSPlayerController, Log, TEXT("DSPlayerController: Initialized"));
}

void ADSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogDSPlayerController, Log, TEXT("DSPlayerController: BeginPlay started"));

    // Create the widget if auto-create is enabled
    if (bAutoCreateWidget)
    {
        EnsureWidgetExists();
    }

    UE_LOG(LogDSPlayerController, Log, TEXT("DSPlayerController: BeginPlay completed"));
}

void ADSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Bind the Settings action to toggle widget
    if (InputComponent)
    {
        InputComponent->BindAction("Settings", IE_Pressed, this, &ADSPlayerController::OnToggleWidget);
        UE_LOG(LogDSPlayerController, Log, TEXT("Bound Settings action to toggle widget"));
    }
    else
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("InputComponent is null - cannot bind input actions"));
    }
}

bool ADSPlayerController::EnsureWidgetExists()
{
    // Widget already exists
    if (IsValid(DSRuntimeWidgetInstance))
    {
        return true;
    }

    // No widget class specified
    if (!DSRuntimeWidgetClass)
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Cannot create widget - no DSRuntimeWidgetClass specified"));
        return false;
    }

    return CreateDSRuntimeWidget();
}

void ADSPlayerController::OnToggleWidget()
{
    UE_LOG(LogDSPlayerController, Log, TEXT("Toggle widget input triggered"));

    // Ensure widget exists before toggling
    if (EnsureWidgetExists())
    {
        ToggleDSRuntimeWidget();
    }
    else
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Failed to toggle widget - widget creation failed"));
    }
}

bool ADSPlayerController::ShowDSRuntimeWidget()
{
    if (!EnsureWidgetExists())
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Cannot show widget - widget creation failed"));
        return false;
    }

    DSRuntimeWidgetInstance->ShowWidget();
    UE_LOG(LogDSPlayerController, Log, TEXT("Showed DSRuntimeWidget"));
    return true;
}

void ADSPlayerController::HideDSRuntimeWidget()
{
    if (IsValid(DSRuntimeWidgetInstance))
    {
        DSRuntimeWidgetInstance->HideWidget();
        UE_LOG(LogDSPlayerController, Log, TEXT("Hid DSRuntimeWidget"));
    }
}

void ADSPlayerController::ToggleDSRuntimeWidget()
{
    if (!EnsureWidgetExists())
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Cannot toggle widget - widget creation failed"));
        return;
    }

    DSRuntimeWidgetInstance->ToggleWidget();
    UE_LOG(LogDSPlayerController, Log, TEXT("Toggled DSRuntimeWidget"));
}

bool ADSPlayerController::CreateDSRuntimeWidget()
{
    // Destroy existing widget first
    if (IsValid(DSRuntimeWidgetInstance))
    {
        DestroyDSRuntimeWidget();
    }

    if (!DSRuntimeWidgetClass)
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Cannot create widget - no widget class specified"));
        return false;
    }

    // Create the widget
    DSRuntimeWidgetInstance = CreateWidget<UDSRuntimeWidget>(this, DSRuntimeWidgetClass);
    
    if (!IsValid(DSRuntimeWidgetInstance))
    {
        UE_LOG(LogDSPlayerController, Error, TEXT("Failed to create DSRuntimeWidget instance"));
        return false;
    }

    // Add to viewport but keep it hidden initially
    DSRuntimeWidgetInstance->AddToViewport(100); // High Z-order to appear on top
    DSRuntimeWidgetInstance->SetVisibility(ESlateVisibility::Hidden);

    UE_LOG(LogDSPlayerController, Log, TEXT("Successfully created DSRuntimeWidget"));
    return true;
}

void ADSPlayerController::DestroyDSRuntimeWidget()
{
    if (IsValid(DSRuntimeWidgetInstance))
    {
        DSRuntimeWidgetInstance->RemoveFromParent();
        DSRuntimeWidgetInstance = nullptr;
        UE_LOG(LogDSPlayerController, Log, TEXT("Destroyed DSRuntimeWidget"));
    }
}