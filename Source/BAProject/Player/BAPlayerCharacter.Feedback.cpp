#include "Player/BAPlayerCharacter.h"

#include "GameFramework/PlayerController.h"

void ABAPlayerCharacter::PlayDamageReactionForceFeedback(
	const EBADamageReactionType DamageReactionType,
	const bool bGuarding,
	const bool bGuardBreak) const
{
	if (bGuardBreak)
	{
		PlayConfiguredForceFeedback(GuardBreakForceFeedbackIntensity, GuardBreakForceFeedbackDuration);
		return;
	}

	if (bGuarding)
	{
		PlayConfiguredForceFeedback(GuardHitForceFeedbackIntensity, GuardHitForceFeedbackDuration);
		return;
	}

	switch (DamageReactionType)
	{
	case EBADamageReactionType::KnockDown:
		PlayConfiguredForceFeedback(KnockDownForceFeedbackIntensity, KnockDownForceFeedbackDuration);
		break;
	case EBADamageReactionType::LargeHitReact:
		PlayConfiguredForceFeedback(LargeHitReactForceFeedbackIntensity, LargeHitReactForceFeedbackDuration);
		break;
	case EBADamageReactionType::HitReact:
	default:
		PlayConfiguredForceFeedback(HitReactForceFeedbackIntensity, HitReactForceFeedbackDuration);
		break;
	}
}

void ABAPlayerCharacter::PlayPerfectGuardForceFeedback() const
{
	PlayConfiguredForceFeedback(PerfectGuardForceFeedbackIntensity, PerfectGuardForceFeedbackDuration);
}

void ABAPlayerCharacter::PlayGuardStartForceFeedback() const
{
	PlayConfiguredForceFeedback(GuardStartForceFeedbackIntensity, GuardStartForceFeedbackDuration);
}

void ABAPlayerCharacter::PlayConfiguredForceFeedback(const float Intensity, const float Duration) const
{
	if (!bEnableGamepadForceFeedback || Intensity <= 0.f || Duration <= 0.f)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->PlayDynamicForceFeedback(
		FMath::Clamp(Intensity, 0.f, 1.f),
		Duration,
		true,
		true,
		true,
		true,
		EDynamicForceFeedbackAction::Start);
}
