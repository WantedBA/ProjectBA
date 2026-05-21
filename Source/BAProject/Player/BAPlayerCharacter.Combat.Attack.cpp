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
			CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack());
			NextActionAnimationTid = FirstLightAttackMontageTid;
			StartAttack(FirstLightAttackMontage);
		}
		else if (InActionCommand == EActionCommand::HeavyAttack)
		{
			CombatComponent->SetAttackData(WeaponRadius, StatComponent->GetAttack() * 1.5);
			NextActionAnimationTid = FirstHeavyAttackMontageTid;
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
		NowActionAnimationTid = 0;
	}
	
}

void ABAPlayerCharacter::StartAttack(UAnimMontage* InAnimMontage)
{
	NowActionAnimationTid = NextActionAnimationTid;
	NextActionAnimationTid = 0;
	NextAttackMontage = nullptr;
	
	SetBAPlayerState(EBAPlayerState::Attacking);
	CombatComponent->ExecuteAttack(InAnimMontage, StatComponent->GetAttackSpeed());
	
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded MontageEnded;
		MontageEnded.BindUObject(this, &ABAPlayerCharacter::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(MontageEnded, InAnimMontage);
	}
}

void ABAPlayerCharacter::SetNextCombo(EActionCommand InActionCommand)
{
	static const UBATableManager* TableManager = UBATableManager::Get(this);
	
	const FActionAnimationDataRow* NowActionAnimationData = 
		TableManager->FindActionAnimationData(NowActionAnimationTid);
	if (!NowActionAnimationData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::SetNextCombo] Failed to find now action animation data for tid: %d"), NowActionAnimationTid);
		return;
	}
	
	// 현재 액션과 입력 커맨드로 다음 액션 탐색
	if (InActionCommand == EActionCommand::LightAttack)
	{
		NextActionAnimationTid = NowActionAnimationData->NextComboLAnimationTid;
	}
	else if (InActionCommand == EActionCommand::HeavyAttack)
	{
		NextActionAnimationTid = NowActionAnimationData->NextComboRAnimationTid;
	}
	
	// 다음 콤보가 없는 경우
	if (NextActionAnimationTid == 0)
	{
		return;
	}
	
	const FActionAnimationDataRow* NextAttackActionAnimationData = 
		TableManager->FindActionAnimationData(NextActionAnimationTid);
	
	if (!NextAttackActionAnimationData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABAPlayerCharacter::SetNextCombo] Failed to find next combo animation data for tid: %d, now: %d"), NextActionAnimationTid, NowActionAnimationTid);
		return;
	}
	
	// TODO 비동기 로딩으로 변경
	NextAttackMontage = NextAttackActionAnimationData->Montage.LoadSynchronous();
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

