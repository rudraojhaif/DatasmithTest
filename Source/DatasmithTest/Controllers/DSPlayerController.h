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
#include "GameFramework/PlayerController.h"
#include "DSPlayerController.generated.h"

// Forward declarations
class UDSRuntimeWidget;

/**
 * ADSPlayerController - Player controller for Datasmith runtime testing
 * 
 * This player controller handles:
 * - Enhanced Input system setup for escape key
 * - Creation and management of the DSRuntimeWidget
 * - Toggling between game and UI input modes
 * 
 * Key Features:
 * - Escape key input action to toggle the configuration widget
 * - Automatic widget creation and management
 * - Proper input mode switching for UI interaction
 */
UCLASS(BlueprintType, Blueprintable)
class DATASMITHTEST_API ADSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ADSPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // === Widget Management ===

    /**
     * Class reference for the DSRuntimeWidget to create
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UDSRuntimeWidget> DSRuntimeWidgetClass;

    /**
     * Instance of the DSRuntimeWidget
     */
    UPROPERTY()
    TObjectPtr<UDSRuntimeWidget> DSRuntimeWidgetInstance;

    /**
     * Whether to automatically create the widget on BeginPlay
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    bool bAutoCreateWidget = true;

private:
    /**
     * Creates the DSRuntimeWidget instance if it doesn't exist
     * @return True if widget was created or already exists
     */
    bool EnsureWidgetExists();

public:
    // === Input Handlers ===

    /**
     * Called when the toggle widget input action is triggered (Escape key pressed)
     */
    UFUNCTION()
    void OnToggleWidget();

    // === Widget Interface ===

    /**
     * Gets the current DSRuntimeWidget instance
     * @return Pointer to the widget, or nullptr if it doesn't exist
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
    UDSRuntimeWidget* GetDSRuntimeWidget() const { return DSRuntimeWidgetInstance; }

    /**
     * Shows the DSRuntimeWidget
     * @return True if the widget was shown successfully
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool ShowDSRuntimeWidget();

    /**
     * Hides the DSRuntimeWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideDSRuntimeWidget();

    /**
     * Toggles the visibility of the DSRuntimeWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleDSRuntimeWidget();

    /**
     * Manually creates the DSRuntimeWidget
     * @return True if creation was successful
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool CreateDSRuntimeWidget();

    /**
     * Destroys the current DSRuntimeWidget instance
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void DestroyDSRuntimeWidget();
};