#include "Player/BAPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Component/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Constants/BAProjectConstant.h"

/*
 * Death Policy Summary
 *
 * 사망 처리는 "사망 판정", "사망 상태 정리", "마지막 자세 고정", "무기 드롭"을 분리해서 다룬다.
 * HP가 0이 되면 StatComponent가 OnDeath를 호출하고, 이 파일은 피격 타입에 따라 바로 죽을지
 * 피격 리액션을 끝까지 재생한 뒤 죽을지 결정한다.
 *
 * 1. 진입 흐름
 * - OnDeath는 DropWeaponAndDie만 호출한다. 실제 분기는 DropWeaponAndDie 안에서 처리한다.
 * - Hit 사망은 바로 사망 상태를 정리하고 사망 몽타주를 재생한다.
 * - LargeHit 사망은 사망 상태만 먼저 잠그고, Movement disable은 사망 몽타주 blend-out 이후로 늦춘다.
 * - KnockDown/Airborne 사망은 별도 사망 몽타주를 재생하지 않는다. 이미 재생 중인 피격 리액션을 끝까지 사용한다.
 * - FinishDeferredDeath는 지연 사망을 마무리하는 공개 진입점이다. 애니메이션 노티파이나 blend-out 콜백에서 같은 흐름을 재사용한다.
 *
 * 2. 몽타주 선택
 * - DeathMontages는 일반 HitReact 사망에 사용된다.
 * - LargeHitDeathMontages는 LargeHitReact 사망에 사용된다.
 * - 방향별 몽타주가 없으면 대각선은 인접한 축 방향으로 보정하고, 마지막으로 Any를 찾는다.
 * - KnockDown/Airborne용 사망 몽타주 맵은 없다. 해당 자세는 KnockDownReactMontages의 마지막 프레임에서 고정된다.
 *
 * 3. 상태 정리
 * - PrepareDeathState는 가드, 액션, 전투 모드, 피격 상태를 정리하고 BAPlayerState를 Dead로 바꾼다.
 * - StopMontagesForDeath는 새 사망 몽타주를 재생해야 하는 경우에만 기존 몽타주를 끊는다.
 * - LargeHit 사망 몽타주는 RootMotionFromMontagesOnly로 재생하고, 고정 후 이동 컴포넌트를 끈다.
 * - 지연 사망에서는 기존 피격 리액션 몽타주를 끊지 않는다. 먼저 마지막 프레임을 고정한 뒤 사망 상태만 정리한다.
 *
 * 4. 마지막 자세 고정
 * - FreezeMontageAtFinalFrame은 지정한 몽타주를 마지막 프레임 직전으로 이동시킨 뒤 Pause하고, Mesh 애니메이션을 멈춘다.
 * - Hit/LargeHit 사망 몽타주는 blend-out이 시작될 때 고정한다. end 시점까지 기다리면 idle 자세가 섞일 수 있다.
 * - KnockDown/Airborne 사망은 피격 리액션 몽타주의 blend-out이 시작될 때 고정한다.
 * - DeathPoseFreezeFrameOffset을 키우면 마지막 프레임보다 더 앞에서 멈추고, 줄이면 더 끝 프레임에 가깝게 멈춘다.
 *
 * 5. 무기 드롭
 * - bDropWeaponOnDeath가 꺼져 있으면 사망해도 무기를 떨어뜨리지 않는다.
 * - HitReact 사망은 사망 몽타주 시작 전에 무기를 떨어뜨린다.
 * - LargeHit/KnockDown/Airborne 같은 지연 사망은 마지막 자세를 고정한 뒤 무기를 떨어뜨린다.
 * - 떨어진 무기는 WorldStatic만 막는다. Pawn, 적, 다른 물리 오브젝트는 무시해서 메쉬에 끼지 않게 한다.
 * - mass/damping 값은 무기의 무게감을 바꾸고, impulse 값은 사망 순간 튀는 방향을 바꾼다.
 */
namespace
{
	constexpr float DeathPoseFreezeFrameOffset = 0.001f;

	UAnimMontage* FindConfiguredDeathMontage(
		const TMap<EActionDirection, TObjectPtr<UAnimMontage>>& Montages,
		const EActionDirection Direction)
	{
		if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(Direction))
		{
			return Montage->Get();
		}

		return nullptr;
	}

	UAnimMontage* FindDeathMontageForDirection(
		const TMap<EActionDirection, TObjectPtr<UAnimMontage>>& Montages,
		const EActionDirection Direction)
	{
		if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, Direction))
		{
			return Montage;
		}

		switch (Direction)
		{
		case EActionDirection::ForwardLeft:
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Forward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Left))
			{
				return Montage;
			}
			break;
		case EActionDirection::ForwardRight:
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Forward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Right))
			{
				return Montage;
			}
			break;
		case EActionDirection::BackwardLeft:
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Backward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Left))
			{
				return Montage;
			}
			break;
		case EActionDirection::BackwardRight:
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Backward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredDeathMontage(Montages, EActionDirection::Right))
			{
				return Montage;
			}
			break;
		default:
			break;
		}

		return FindConfiguredDeathMontage(Montages, EActionDirection::Any);
	}
}

void ABAPlayerCharacter::OnDeath()
{
	DropWeaponAndDie();
}

void ABAPlayerCharacter::DropWeaponAndDie()
{
	if (ShouldDeferDeathUntilDamageReaction())
	{
		StartDeferredDamageReactionDeath();
		return;
	}

	FinalizeDropWeaponAndDie(LastDamageHitDirection);
}

bool ABAPlayerCharacter::ShouldDeferDeathUntilDamageReaction() const
{
	return !bDeathFinalizationDeferred
		&& LastDamageReactionType == EBADamageReactionType::KnockDown;
}

bool ABAPlayerCharacter::ShouldDeferMovementDisableForDeathMontage() const
{
	return LastDamageReactionType == EBADamageReactionType::LargeHitReact;
}

void ABAPlayerCharacter::StartDeferredDamageReactionDeath()
{
	bDeathFinalizationDeferred = true;
	CancelCurrentActionForDamageReaction();
	ApplyDamageReactionKnockback(
		LastDamageReactionType,
		LastDamageDirection,
		LastDamageHitDirection,
		false,
		false);
	PlayDamageReactionAnimation(
		LastDamageReactionType,
		LastDamageHitDirection,
		false,
		false);
}

void ABAPlayerCharacter::FinishDeferredDeath(const EActionDirection /*DeathDirection*/)
{
	if (!bDeathFinalizationDeferred)
	{
		return;
	}

	bDeathFinalizationDeferred = false;
	FinalizeDeferredDamageReactionDeath();
}

void ABAPlayerCharacter::FinalizeDeferredDamageReactionDeath()
{
	FreezeMontageAtFinalFrame(ActiveDamageReactionMontage);

	Super::OnDeath();
	PrepareDeathState();

	if (!ShouldDropWeaponImmediatelyOnDeath())
	{
		DropWeaponForDeath();
	}

	Respawn();
}

void ABAPlayerCharacter::FinalizeDropWeaponAndDie(const EActionDirection DeathDirection)
{
	const bool bDeferMovementDisable = ShouldDeferMovementDisableForDeathMontage();
	if (bDeferMovementDisable)
	{
		CharacterState = ECharacterState::Dead;
		bDeathMovementDisableDeferred = true;
	}
	else
	{
		Super::OnDeath();
	}

	PrepareDeathState();
	StopMontagesForDeath();
	if (bDeferMovementDisable)
	{
		ApplyDeathMontageRootMotionMode();
		bDeathMovementDisableDeferred = true;
	}

	if (ShouldDropWeaponImmediatelyOnDeath())
	{
		DropWeaponForDeath();
	}
	PlayDeathMontage(DeathDirection);
	if (!ActiveDeathMontage)
	{
		FinalizeDeathAfterMontage();
	}
}

void ABAPlayerCharacter::FinalizeDeathAfterMontage()
{
	RestoreDeathMontageRootMotionMode();

	if (!bDeathMovementDisableDeferred)
	{
		Respawn();
		return;
	}

	bDeathMovementDisableDeferred = false;
	Super::OnDeath();

	if (!ShouldDropWeaponImmediatelyOnDeath())
	{
		DropWeaponForDeath();
	}

	Respawn();
}

void ABAPlayerCharacter::PrepareDeathState()
{
	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	ResetKnockDownRecovery();
	ResetLandingRecovery();
	PlayDeathCameraShake();
	DamageReactionState = EPlayerDamageReactionState::None;
	ActiveDamageReactionMontage = nullptr;
	ActiveDamageReactionPlaybackId = 0;
	ActiveDeathMontage = nullptr;
	bDeathFinalizationDeferred = false;
	bDeathMovementDisableDeferred = false;
	bWeaponDroppedForDeath = false;
	bGuardInputHeld = false;
	SetGuardWindowActive(false);

	if (ActionComponent)
	{
		ActionComponent->CancelCurrentAction();
		ActionComponent->SetGuardState(EGuardState::None);
	}

	if (CombatComponent)
	{
		CombatComponent->CheckHitEnd();
	}

	SetCombatMode(EPlayerCombatMode::None);
	SetBAPlayerState(EBAPlayerState::Dead);
}

void ABAPlayerCharacter::StopMontagesForDeath()
{
	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->bPauseAnims = false;
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(FMath::Max(0.f, DeathMontageStopBlendOut));
		}
	}
}

void ABAPlayerCharacter::ApplyDeathMontageRootMotionMode()
{
	if (bDeathRootMotionModeOverridden)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	PreviousDeathRootMotionMode = AnimInstance->RootMotionMode;
	DeathRootMotionModeAnimInstance = AnimInstance;
	bDeathRootMotionModeOverridden = true;
	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
}

void ABAPlayerCharacter::RestoreDeathMontageRootMotionMode()
{
	if (!bDeathRootMotionModeOverridden)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = DeathRootMotionModeAnimInstance.Get())
	{
		AnimInstance->SetRootMotionMode(PreviousDeathRootMotionMode);
	}

	DeathRootMotionModeAnimInstance = nullptr;
	bDeathRootMotionModeOverridden = false;
}

bool ABAPlayerCharacter::ShouldDropWeaponOnDeath() const
{
	return CanDropWeaponForDeath();
}

bool ABAPlayerCharacter::ShouldDropWeaponImmediatelyOnDeath() const
{
	return LastDamageReactionType == EBADamageReactionType::HitReact;
}

bool ABAPlayerCharacter::CanDropWeaponForDeath() const
{
	return bDropWeaponOnDeath
		&& !bWeaponDroppedForDeath
		&& WeaponMeshComponent
		&& WeaponMeshComponent->GetStaticMesh()
		&& !WeaponMeshComponent->IsSimulatingPhysics();
}

void ABAPlayerCharacter::DropWeaponForDeath()
{
	if (!ShouldDropWeaponOnDeath())
	{
		return;
	}

	DetachWeaponForDeath();
	ApplyDroppedWeaponPhysics();
	bWeaponDroppedForDeath = true;
}

void ABAPlayerCharacter::DetachWeaponForDeath()
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	WeaponMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void ABAPlayerCharacter::ApplyDroppedWeaponPhysics()
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	ConfigureDroppedWeaponCollision();
	ConfigureDroppedWeaponWeight();

	WeaponMeshComponent->SetSimulatePhysics(true);
	WeaponMeshComponent->SetEnableGravity(true);
	WeaponMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	WeaponMeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	WeaponMeshComponent->WakeAllRigidBodies();

	const FVector DropImpulse = CalculateDeathWeaponDropImpulse();
	if (!DropImpulse.IsNearlyZero())
	{
		WeaponMeshComponent->AddImpulse(DropImpulse, NAME_None, false);
	}

	if (!DroppedWeaponAngularImpulse.IsNearlyZero())
	{
		WeaponMeshComponent->AddAngularImpulseInDegrees(DroppedWeaponAngularImpulse, NAME_None, false);
	}
}

void ABAPlayerCharacter::ConfigureDroppedWeaponCollision()
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
	WeaponMeshComponent->SetNotifyRigidBodyCollision(false);
}

void ABAPlayerCharacter::ConfigureDroppedWeaponWeight()
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	if (DroppedWeaponMassKg > 0.f)
	{
		WeaponMeshComponent->SetMassOverrideInKg(NAME_None, DroppedWeaponMassKg, true);
	}

	WeaponMeshComponent->SetLinearDamping(DroppedWeaponLinearDamping);
	WeaponMeshComponent->SetAngularDamping(DroppedWeaponAngularDamping);
	WeaponMeshComponent->SetPhysicsMaxAngularVelocityInDegrees(DroppedWeaponMaxAngularSpeedDeg);
}

FVector ABAPlayerCharacter::CalculateDeathWeaponDropImpulse() const
{
	return GetActorForwardVector() * DroppedWeaponForwardImpulse
		+ GetActorRightVector() * DroppedWeaponRightImpulse
		+ FVector::UpVector * DroppedWeaponUpwardImpulse;
}

const TMap<EActionDirection, TObjectPtr<UAnimMontage>>* ABAPlayerCharacter::GetDeathMontageMapForDamageReaction(
	const EBADamageReactionType DamageReactionType) const
{
	switch (DamageReactionType)
	{
	case EBADamageReactionType::LargeHitReact:
		return &LargeHitDeathMontages;
	case EBADamageReactionType::HitReact:
	default:
		return &DeathMontages;
	}
}

UAnimMontage* ABAPlayerCharacter::SelectDeathMontage(const EActionDirection HitDirection) const
{
	const TMap<EActionDirection, TObjectPtr<UAnimMontage>>* DeathMontageMap =
		GetDeathMontageMapForDamageReaction(LastDamageReactionType);
	if (DeathMontageMap)
	{
		if (UAnimMontage* Montage = FindDeathMontageForDirection(*DeathMontageMap, HitDirection))
		{
			return Montage;
		}
	}

	return DeathMontageMap != &DeathMontages
		? FindConfiguredDeathMontage(DeathMontages, EActionDirection::Any)
		: nullptr;
}

void ABAPlayerCharacter::PlayDeathMontage(const EActionDirection DeathDirection)
{
	ActiveDeathMontage = SelectDeathMontage(DeathDirection);
	if (!ActiveDeathMontage)
	{
		return;
	}

	const float MontageDuration = PlayAnimMontage(ActiveDeathMontage);
	if (MontageDuration <= 0.f)
	{
		ActiveDeathMontage = nullptr;
		return;
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			FOnMontageBlendingOutStarted DeathMontageBlendingOut;
			DeathMontageBlendingOut.BindUObject(this, &ABAPlayerCharacter::HandleDeathMontageBlendingOut);
			AnimInstance->Montage_SetBlendingOutDelegate(DeathMontageBlendingOut, ActiveDeathMontage);

			FOnMontageEnded DeathMontageEnded;
			DeathMontageEnded.BindUObject(this, &ABAPlayerCharacter::HandleDeathMontageEnded);
			AnimInstance->Montage_SetEndDelegate(DeathMontageEnded, ActiveDeathMontage);
		}
	}

	K2_OnDeathMontageStarted(DeathDirection, ActiveDeathMontage);
}

void ABAPlayerCharacter::FreezeMontageAtFinalFrame(UAnimMontage* MontageToPause)
{
	if (!MontageToPause)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			const float FreezePosition = FMath::Max(0.f, MontageToPause->GetPlayLength() - DeathPoseFreezeFrameOffset);
			AnimInstance->Montage_SetPosition(MontageToPause, FreezePosition);
			AnimInstance->Montage_Pause(MontageToPause);
		}

		MeshComponent->bPauseAnims = true;
	}
}

void ABAPlayerCharacter::HandleDeathMontageBlendingOut(UAnimMontage* Montage, const bool bInterrupted)
{
	if (Montage != ActiveDeathMontage || bInterrupted)
	{
		return;
	}

	FreezeMontageAtFinalFrame(Montage);
	FinalizeDeathAfterMontage();
}

void ABAPlayerCharacter::HandleDeathMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	if (Montage != ActiveDeathMontage)
	{
		return;
	}

	FinalizeDeathAfterMontage();
	ActiveDeathMontage = nullptr;
	K2_OnDeathMontageEnded(bInterrupted);
}
