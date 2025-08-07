#pragma once

#include "GameFramework/DefaultPawn.h"
#include "DSPawn.generated.h"


// Forward dec
class UFloatingPawnMovement;
/**
 * DSPawn - Simple editor-style movement pawn inheriting from DefaultPawn
 * Inherits all the basic movement functionality from DefaultPawn
 */
UCLASS()
class DATASMITHTEST_API ADSPawn : public ADefaultPawn
{
    GENERATED_BODY()

public:
    ADSPawn();

    /**
     * Sets the maximum movement speed for this pawn
     * @param NewMaxSpeed The new maximum speed to set (must be positive)
     */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetMaxSpeed(float NewMaxSpeed);

    /**
     * Gets the current maximum movement speed of this pawn
     * @return The current maximum speed
     */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    float GetMaxSpeed() const;

protected:
    /**
     * Gets the pawn's movement component, cast to UFloatingPawnMovement
     * @return Pointer to the movement component, or nullptr if invalid
     */
    UFloatingPawnMovement* GetPawnMovement() const;
};