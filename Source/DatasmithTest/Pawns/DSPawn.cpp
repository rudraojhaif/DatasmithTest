#include "DSPawn.h"
#include "Components/InputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ADSPawn::ADSPawn()
{
    // Initialize with a default speed
    SetMaxSpeed(1000.0f);
}

void ADSPawn::SetMaxSpeed(float NewMaxSpeed)
{
    if (UFloatingPawnMovement* MovementComp = GetPawnMovement())
    {
        // Ensure speed is non-negative
        MovementComp->MaxSpeed = FMath::Max(0.0f, NewMaxSpeed);
    }
}

float ADSPawn::GetMaxSpeed() const
{
    if (const UFloatingPawnMovement* MovementComp = GetPawnMovement())
    {
        return MovementComp->MaxSpeed;
    }
    return 0.0f;
}

UFloatingPawnMovement* ADSPawn::GetPawnMovement() const
{
    // Cache the movement component with proper type casting
    return Cast<UFloatingPawnMovement>(GetMovementComponent());
}