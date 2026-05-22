#include "BAPlayerCharacter.h"
#include "Component/CombatComponent.h"
#include "Component/StatComponent.h"
#include "Tables/ActionRows.h"
#include "Tables/BATableManager.h"


// 공격 입력 진입점
void ABAPlayerCharacter::TryAttack(EActionCommand InActionCommand)
{
	UE_LOG(LogTemp, Log, TEXT("Player TryAttack Command: %hhd"), InActionCommand);

	switch (BAPlayerState)
	{
	// 공격 커맨드 무시
	case EBAPlayerState::Dead:
	case EBAPlayerState::HitReacting:
	case EBAPlayerState::KnockedDown:
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
			CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack());
			NextComboTransitionTid = FirstLComboTransitionTid;
			StartAttack(FirstLightAttackMontage);
		}
		else if (InActionCommand == EActionCommand::HeavyAttack)
		{
			CancelGuardForActionInterrupt();
			CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack() * 1.5);
			NextComboTransitionTid = FirstRComboTransitionTid;
			StartAttack(FirstHeavyAttackMontage);
		}
	}
}

void ABAPlayerCharacter::OnAttackMontageEnded(UAnimMontage* AnimMontage, bool bArg)
{
	// 정상 종료되었을 경우
	if (!bArg)
	{
		SetBAPlayerState(EBAPlayerState::None);
		NowComboTransitionTid = 0;
	}
}

void ABAPlayerCharacter::StartAttack(UAnimMontage* InAnimMontage)
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	
	NowComboTransitionTid = NextComboTransitionTid;
	NextComboTransitionTid = 0;
	NextAttackMontage = nullptr;
	
	const FComboTransitionRow* NowCombo = TableManager->FindComboTransition(NowComboTransitionTid);
	if (!NowCombo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find combo transition row with tid: %d"), NowComboTransitionTid);
		return;
	}
	
	// 재생 속도 : 테이블에 정의된 몽타주 재생 속도 * 공격 속도
	const float MontagePlayRate = NowCombo->PlayRate * StatComponent->GetAttackSpeed();
	CombatComponent->ExecuteAttack(InAnimMontage, MontagePlayRate);
	
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		SetBAPlayerState(EBAPlayerState::Attacking);
		
		FOnMontageEnded MontageEnded;
		MontageEnded.BindUObject(this, &ABAPlayerCharacter::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(MontageEnded, InAnimMontage);
	}
}

void ABAPlayerCharacter::SetNextCombo(EActionCommand InActionCommand)
{
	static const UBATableManager* TableManager = UBATableManager::Get(this);
	
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
	
	const FComboTransitionRow* NextComboTransition = 
		TableManager->FindComboTransition(NextComboTransitionTid);
	
	if (!NextComboTransition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::SetNextCombo] Failed to find next combo animation data for tid: %d, now: %d"), NextComboTransitionTid, NowComboTransitionTid);
		return;
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

