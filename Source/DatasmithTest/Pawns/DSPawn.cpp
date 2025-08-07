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