#include "Player/BAPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

void ABAPlayerCharacter::LockMovementForCutscene()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_None);
	}
}

void ABAPlayerCharacter::UnlockMovementForCutscene()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}
