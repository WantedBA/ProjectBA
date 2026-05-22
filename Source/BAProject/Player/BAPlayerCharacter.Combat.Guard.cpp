#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Component/ActionAnimationComponent.h"
#include "Component/StatComponent.h"
#include "Engine/Engine.h"

namespace
{
	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율도 장비별 정책으로 옮겨야 한다.
	constexpr float PerfectGuardStaminaCostMultiplier = 0.5f;
}

bool ABAPlayerCharacter::TryStartGuard()
{
	bGuardInputHeld = true;

	if (!CanAcceptActionInput() || !ActionComponent)
	{
		return false;
	}

	SetGuardWindowActive(false);
	SetPerfectGuardWindowActive(false);

	if (!ActionComponent->TryStartAction(EActionCommand::Guard))
	{
		return false;
	}

	if (ActionComponent->GetActiveActionType() != EActionType::Guard)
	{
		return false;
	}

	ConfigureGuardMontageSections();
	SetBAPlayerState(EBAPlayerState::Guarding);
	SetCombatMode(EPlayerCombatMode::Block);
	return true;
}

void ABAPlayerCharacter::StopGuard()
{
	bGuardInputHeld = false;

	if (!ActionComponent)
	{
		return;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	const bool bGuardActionRunning = ActionComponent->GetActiveActionType() == EActionType::Guard;
	const bool bGuardWindowActive = GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking;
	const bool bGuardBroken = GuardState == EGuardState::GuardBroken;
	SetGuardWindowActive(false);
	if (bGuardBroken)
	{
		ActionComponent->SetGuardState(EGuardState::None);
	}

	if (bGuardActionRunning)
	{
		// 실제 가드 판정이 열린 뒤의 해제만 End 섹션으로 보낸다.
		// Start 중 키를 떼는 스팸 입력은 아직 가드가 성립하지 않았으므로 몽타주를 끊는다.
		if ((bGuardWindowActive || bGuardBroken) && RequestGuardMontageEnd())
		{
			SetBAPlayerState(EBAPlayerState::None);
			SetCombatMode(EPlayerCombatMode::None);
			return;
		}

		ActionComponent->CompleteCurrentAction();
	}

	if (!bGuardWindowActive && !bGuardBroken)
	{
		return;
	}

	if (IsDamageReacting())
	{
		return;
	}

	SetBAPlayerState(EBAPlayerState::None);
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::ConfigureGuardMontageSections()
{
	if (!ActionAnimationComponent)
	{
		return;
	}

	ActionAnimationComponent->SetActiveMontageNextSection(GuardStartSection, GuardLoopSection);
	ActionAnimationComponent->SetActiveMontageNextSection(GuardLoopSection, GuardLoopSection);
}

bool ABAPlayerCharacter::RequestGuardMontageEnd()
{
	if (!ActionAnimationComponent)
	{
		return false;
	}

	ActionAnimationComponent->SetActiveMontageNextSection(GuardStartSection, GuardEndSection);
	ActionAnimationComponent->SetActiveMontageNextSection(GuardLoopSection, GuardEndSection);
	return ActionAnimationComponent->JumpActiveMontageToSection(GuardEndSection);
}

void ABAPlayerCharacter::ConsumePerfectGuardStaminaCost()
{
	if (!ActionComponent)
	{
		return;
	}

	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율 적용도 장비 정책에서 계산해야 한다.
	ActionComponent->ConsumeActiveActionStaminaCost(GetPerfectGuardStaminaCostMultiplier());
}

void ABAPlayerCharacter::SetGuardWindowActive(const bool bActive)
{
	if (!ActionComponent)
	{
		SetPerfectGuardWindowActive(false);
		return;
	}

	if (bActive)
	{
		if (ActionComponent->GetActiveActionType() == EActionType::Guard && !IsDamageReacting())
		{
			ActionComponent->SetGuardState(EGuardState::Guarding);
		}
		return;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	if (GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking)
	{
		ActionComponent->SetGuardState(EGuardState::None);
	}
	SetPerfectGuardWindowActive(false);
}

void ABAPlayerCharacter::SetPerfectGuardWindowActive(const bool bActive)
{
	if (bActive && (!ActionComponent
			|| (ActionComponent->GetGuardState() != EGuardState::Guarding
				&& ActionComponent->GetGuardState() != EGuardState::Blocking)))
	{
		return;
	}

	bPerfectGuardWindowActive = bActive;
}

bool ABAPlayerCharacter::IsPerfectGuardWindowActive() const
{
	return bPerfectGuardWindowActive;
}

float ABAPlayerCharacter::GetGuardAbsorptionMultiplier() const
{
	return 1 - StatComponent->GetGuardDamageReductionRate();
}

float ABAPlayerCharacter::GetPerfectGuardStaminaCostMultiplier() const
{
	return PerfectGuardStaminaCostMultiplier;
}

bool ABAPlayerCharacter::ConsumeGuardStaminaForDamage()
{
	if (!ActionComponent)
	{
		return false;
	}

	const bool bConsumedStamina = ActionComponent->ConsumeActiveActionStaminaCost();
	ActionComponent->SetGuardState(bConsumedStamina ? EGuardState::Blocking : EGuardState::GuardBroken);
	return bConsumedStamina;
}

bool ABAPlayerCharacter::ShouldResumeGuardAfterGuardHit() const
{
	return bGuardInputHeld && IsAlive() && !IsOnLadder();
}

bool ABAPlayerCharacter::ResumeGuardAfterGuardHit()
{
	if (!TryStartGuard())
	{
		return false;
	}

	if (ActionAnimationComponent)
	{
		ActionAnimationComponent->JumpActiveMontageToSection(GuardLoopSection);
	}

	SetGuardWindowActive(true);
	SetPerfectGuardWindowActive(false);
	if (ActionComponent)
	{
		ActionComponent->SetGuardState(EGuardState::Guarding);
	}
	SetBAPlayerState(EBAPlayerState::Guarding);
	SetCombatMode(EPlayerCombatMode::Block);
	return true;
}

void ABAPlayerCharacter::ShowGuardJudgementDebugMessage(const FString& Message, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color, Message);
	}
#endif
}

void ABAPlayerCharacter::HandlePerfectGuardSucceeded(const FHitResult& HitResult, AActor* DamageCauser)
{
	ConsumePerfectGuardStaminaCost();
	ShowGuardJudgementDebugMessage(TEXT("Perfect Guard"), FColor::Cyan);
	K2_OnPerfectGuardSucceeded(HitResult, DamageCauser);

	if (ACharacterBase* DamageCauserCharacter = Cast<ACharacterBase>(DamageCauser))
	{
		DamageCauserCharacter->OnAttackPerfectGuarded.Broadcast(this, HitResult);
	}

	// 슬로우모션은 아직 합의되지 않은 스펙이라 플레이어 가드 성공 처리 쪽에 위치만 남기고 비활성화한다.
	// if (UWorld* World = GetWorld())
	// {
	// 	if (UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>())
	// 	{
	// 		TimeSubsystem->ApplySlowMotion(TimeDilation, Duration);
	// 	}
	// }
}
