#include "BAPlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Component/ActionComponent.h"
#include "Component/CombatComponent.h"
#include "Component/PlayerWeaponVFX.h"
#include "Component/StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tables/ActionRows.h"
#include "Tables/BATableManager.h"

void ABAPlayerCharacter::ClearAttackRuntimeState()
{
	NowComboTransitionTid = 0;
	NextComboTransitionTid = 0;
	NextAttackMontage = nullptr;
	NextAttackActionType = EActionType::None;
	ActiveAttackMontage = nullptr;
	ActiveAttackPlaybackId = 0;
}

bool ABAPlayerCharacter::IsActiveAttackMontagePlaying() const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	return AnimInstance && ActiveAttackMontage && AnimInstance->Montage_IsPlaying(ActiveAttackMontage);
}

// 공격 입력 진입점
void ABAPlayerCharacter::TryAttack(EActionCommand InActionCommand)
{
	UE_LOG(LogTemp, Log, TEXT("Player TryAttack Command: %hhd"), InActionCommand);

	if (BAPlayerState == EBAPlayerState::Attacking && !IsActiveAttackMontagePlaying())
	{
		ClearAttackRuntimeState();
		SetBAPlayerState(EBAPlayerState::None);
	}

	switch (BAPlayerState)
	{
	// 공격 커맨드 무시
	case EBAPlayerState::Dead:
	case EBAPlayerState::HitReacting:
	case EBAPlayerState::KnockedDown:
	case EBAPlayerState::Respawning:
		return;
		break;
	// 다음 공격 저장
	case EBAPlayerState::Attacking:
	case EBAPlayerState::DodgeRolling:
		SetNextCombo(InActionCommand);
		break;
	// 공격 바로 실행
	case EBAPlayerState::Guarding:
	case EBAPlayerState::Moving:
	case EBAPlayerState::None:
	default:
		if (InActionCommand == EActionCommand::LightAttack)
		{
			CancelGuardForActionInterrupt();
			SetNextCombo(InActionCommand);
			if (NextAttackMontage)
			{
				StartAttack(NextAttackMontage);
			}
		}
		else if (InActionCommand == EActionCommand::HeavyAttack)
		{
			CancelGuardForActionInterrupt();
			SetNextCombo(InActionCommand);
			if (NextAttackMontage)
			{
				StartAttack(NextAttackMontage);
			}
		}
	}
}

void ABAPlayerCharacter::ChargeAttackStart()
{
	bIsBeforeCharge = true;
}

void ABAPlayerCharacter::ChargeLoopStart(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// 루프를 돌 적당한 애니메이션이 없어서 그냥 일시정지로 구현함
	// 몽타주에 설정된 AN_ChargeStart에서 호출됨
	bIsBeforeCharge = false;
	
	if (bIsChargeInputCompleted)
	{
		// 이 함수가 호출되기 전에 이미 우클릭이 끝난 상태
		bIsChargeInputCompleted = false;
		return;
	}
	
	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}
	UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (!Montage)
	{
		Montage = AnimInstance->GetCurrentActiveMontage();
		UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::ChargeLoopStart] AnimNotify에서 넘겨준 몽타주가 비어 있습니다."))
	}

	if (Montage)
	{
		bIsCharging = true;
		AnimInstance->Montage_Pause(Montage);
		PausedMontage = Montage;
		

		
		GetWorldTimerManager().ClearTimer(ChargeAttackTimerHandle);
		GetWorldTimerManager().SetTimer(
			ChargeAttackTimerHandle, this, &ABAPlayerCharacter::ChargeAttackCompleted, MaxChargeTime, false);
	}
}

void ABAPlayerCharacter::ChargeAttackCompleted()
{
	if (!bIsCharging)
	{
		if (bIsBeforeCharge)
		{
			bIsChargeInputCompleted = true;
		}
		return;
	}
	
	bIsCharging = false;
	
	if (!PausedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("PausedMontage is nullptr when ChargeAttackCompleted is called"));
		return;
	}
	
	// 차징 시간
	float FinalChargeTime = GetWorldTimerManager().GetTimerElapsed(ChargeAttackTimerHandle);
	UE_LOG(LogTemp, Log, TEXT("ChargeAttackCompleted - FinalChargeTime: %f"), FinalChargeTime);
	
	// 차징 공격 대미지 설정
	UBATableManager* TableManager = UBATableManager::Get(this);
	const FComboTransitionRow* NowCombo = TableManager->FindComboTransition(NowComboTransitionTid);
	CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack() * NowCombo->DamageCoefficient 
		* (1.f + FinalChargeTime));
	
	GetMesh()->GetAnimInstance()->Montage_Resume(PausedMontage);
	StopChargeEffect();
}

void ABAPlayerCharacter::StopChargeEffect()
{
	GetWorldTimerManager().ClearTimer(ChargeAttackTimerHandle);
	
	// 차징 중 중간에 끊기거나, 정상적으로 차징이 완료되어 후딜 실행 중일 때
	// ChargeLoopStart 뒷부분에 차징 중 표시할 이펙트 작성하고, 여기서 중단하면 됩니다
	PausedMontage = nullptr;
}

void ABAPlayerCharacter::OnAttackMontageEnded(
	UAnimMontage* AnimMontage,
	const bool /*bInterrupted*/,
	const int32 PlaybackId)
{
	// 차징 공격 중간에 외부에서 끊긴 경우
	if (bIsCharging)
	{
		StopChargeEffect();
		bIsCharging = false;
	}

	if (PlaybackId != ActiveAttackPlaybackId || AnimMontage != ActiveAttackMontage)
	{
		return;
	}

	ClearAttackRuntimeState();
	if (BAPlayerState == EBAPlayerState::Attacking)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
	
	// 공격 몽타주가 중간에 끊긴 경우
	if (bArg && BAPlayerState != EBAPlayerState::Attacking)
	{
		NowComboTransitionTid = 0;
	}

	// Collision의 NotifyEnd가 호출되지 않았을 수 있음
	if (PlayerWeaponVFX)
	{
		PlayerWeaponVFX->DeactivateTrailNiagara();
	}
}

void ABAPlayerCharacter::StartAttack(UAnimMontage* InAnimMontage)
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager || !InAnimMontage || NextComboTransitionTid == 0)
	{
		ClearAttackRuntimeState();
		return;
	}
	
	NowComboTransitionTid = NextComboTransitionTid;
	NextComboTransitionTid = 0;
	NextAttackMontage = nullptr;
	const EActionType AttackActionType = NextAttackActionType;
	NextAttackActionType = EActionType::None;
	
	const FComboTransitionRow* NowCombo = TableManager->FindComboTransition(NowComboTransitionTid);
	if (!NowCombo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find combo transition row with tid: %d"), NowComboTransitionTid);
		ClearAttackRuntimeState();
		return;
	}
	
	// 대미지 설정
	CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack() * NowCombo->DamageCoefficient);
	
	// 재생 속도 : 테이블에 정의된 몽타주 재생 속도 * 공격 속도
	const float MontagePlayRate = NowCombo->PlayRate * StatComponent->GetAttackSpeed();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		ClearAttackRuntimeState();
		return;
	}

	const int32 PlaybackId = NextAttackPlaybackId++;
	ActiveAttackPlaybackId = PlaybackId;
	ActiveAttackMontage = InAnimMontage;
	const float PlayedDuration = PlayAnimMontage(InAnimMontage, MontagePlayRate);
	if (PlayedDuration <= 0.f)
	{
		ClearAttackRuntimeState();
		if (BAPlayerState == EBAPlayerState::Attacking)
		{
			SetBAPlayerState(EBAPlayerState::None);
		}
		return;
	}

	SetBAPlayerState(EBAPlayerState::Attacking);

	// 스태미나 소모
	if (ActionComponent)
	{
		ActionComponent->ConsumeActionStartStaminaCostByType(AttackActionType);
	}

	FOnMontageEnded MontageEnded;
	MontageEnded.BindUObject(this, &ABAPlayerCharacter::OnAttackMontageEnded, PlaybackId);
	AnimInstance->Montage_SetEndDelegate(MontageEnded, InAnimMontage);
}

void ABAPlayerCharacter::SetNextCombo(EActionCommand InActionCommand)
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return;
	}

	NextComboTransitionTid = 0;
	NextAttackMontage = nullptr;
	NextAttackActionType = EActionType::None;
	
	// 현재 실행 중인 액션이 없을 경우 기본값으로 세팅
	if (!NowComboTransitionTid)
	{
		if (InActionCommand == EActionCommand::LightAttack)
		{
			NextComboTransitionTid = FirstLComboTransitionTid;
		}
		else if (InActionCommand == EActionCommand::HeavyAttack)
		{
			NextComboTransitionTid = FirstRComboTransitionTid;
		}
	}
	else
	{
		// 현재 실행 중인 액션
		const FComboTransitionRow* NowComboTransition = 
			TableManager->FindComboTransition(NowComboTransitionTid);
		if (!NowComboTransition)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::SetNextCombo] Failed to find now action animation data for tid: %d"), NowComboTransitionTid);
			return;
		}
		
		// 현재 액션과 입력 커맨드로 다음 액션 탐색
		if (InActionCommand == EActionCommand::LightAttack)
		{
			NextComboTransitionTid = NowComboTransition->NextOnL;
		}
		else if (InActionCommand == EActionCommand::HeavyAttack)
		{
			NextComboTransitionTid = NowComboTransition->NextOnR;
		}
		
		// 다음 콤보가 없는 경우
		if (NextComboTransitionTid == 0)
		{
			return;
		}
	}
	
	const FComboTransitionRow* NextComboTransition = 
		TableManager->FindComboTransition(NextComboTransitionTid);
	
	if (!NextComboTransition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::SetNextCombo] Failed to find next combo animation data for tid: %d, now: %d"), NextComboTransitionTid, NowComboTransitionTid);
		return;
	}
	
	// 스태미나 소모값 확인을 위한 정보 저장
	if (InActionCommand == EActionCommand::LightAttack)
	{
		NextAttackActionType = EActionType::LightAttack;
	}
	else if (InActionCommand == EActionCommand::HeavyAttack)
	{
		NextAttackActionType = EActionType::HeavyAttack;
	}
	
	// TODO 비동기 로딩으로 변경
	NextAttackMontage = NextComboTransition->Montage.LoadSynchronous();
}

void ABAPlayerCharacter::OnNextComboCheck()
{
	if (NextAttackMontage)
	{
		StartAttack(NextAttackMontage);
	}
	else
	{
		return;
	}
}

