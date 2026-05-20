#include "Character/CharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	EActionDirection ResolveHitDirection(const AActor& DamagedActor, const FVector& DamageDirection)
	{
		FVector SourceDirection = -DamageDirection;
		SourceDirection.Z = 0.f;
		if (!SourceDirection.Normalize())
		{
			return EActionDirection::Any;
		}

		FVector Forward = DamagedActor.GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();

		FVector Right = DamagedActor.GetActorRightVector();
		Right.Z = 0.f;
		Right.Normalize();

		const float ForwardDot = FVector::DotProduct(Forward, SourceDirection);
		const float RightDot = FVector::DotProduct(Right, SourceDirection);
		const float Angle = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

		// 정면을 0도로 두고 45도 간격으로 8방향 피격 섹터를 나눈다.
		if (Angle >= -22.5f && Angle < 22.5f)
		{
			return EActionDirection::Forward;
		}
		if (Angle >= 22.5f && Angle < 67.5f)
		{
			return EActionDirection::ForwardRight;
		}
		if (Angle >= 67.5f && Angle < 112.5f)
		{
			return EActionDirection::Right;
		}
		if (Angle >= 112.5f && Angle < 157.5f)
		{
			return EActionDirection::BackwardRight;
		}
		if (Angle >= 157.5f || Angle < -157.5f)
		{
			return EActionDirection::Backward;
		}
		if (Angle >= -157.5f && Angle < -112.5f)
		{
			return EActionDirection::BackwardLeft;
		}
		if (Angle >= -112.5f && Angle < -67.5f)
		{
			return EActionDirection::Left;
		}

		return EActionDirection::ForwardLeft;
	}
}

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	CharacterState = ECharacterState::Alive;
}

void ACharacterBase::Attack()
{
}

float ACharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!CanReceiveDamage())
	{
		return 0.0f;
	}

	const float FinalDamage = FMath::Max(0.0f, DamageAmount);
	if (FinalDamage <= 0.0f)
	{
		return 0.0f;
	}

	OnDamaged(BuildDamageContext(FinalDamage, DamageEvent, EventInstigator, DamageCauser));
	return FinalDamage;
}

void ACharacterBase::SetInvincible(const bool bNewInvincible)
{
	if (!IsAlive())
	{
		return;
	}

	CharacterState = bNewInvincible ? ECharacterState::Invincible : ECharacterState::Alive;
}

FBACharacterDamageContext ACharacterBase::BuildDamageContext(
	const float FinalDamage,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser) const
{
	FBACharacterDamageContext DamageContext;
	DamageContext.FinalDamage = FinalDamage;
	DamageContext.DamageCauser = DamageCauser;
	DamageContext.EventInstigator = EventInstigator;

	// 프로젝트 전용 이벤트는 피격 강도와 히트 위치까지 포함한다.
	if (DamageEvent.IsOfType(FBADamageEvent::ClassID))
	{
		const FBADamageEvent& BADamageEvent = static_cast<const FBADamageEvent&>(DamageEvent);
		DamageContext.DamageReactionType = BADamageEvent.DamageReactionType;
		DamageContext.DamageDirection = BADamageEvent.DamageDirection.GetSafeNormal();
		DamageContext.HitResult = BADamageEvent.HitResult;
	}
	else if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// 외부 시스템에서 들어온 PointDamage도 방향 기반 피격 반응에 연결한다.
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		DamageContext.DamageDirection = PointDamageEvent.ShotDirection.GetSafeNormal();
		DamageContext.HitResult = PointDamageEvent.HitInfo;
	}

	if (DamageContext.DamageDirection.IsNearlyZero() && DamageCauser)
	{
		// 최소한의 방향 정보가 필요하므로 DamageCauser 위치를 fallback으로 사용한다.
		DamageContext.DamageDirection = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	}

	DamageContext.HitDirection = ResolveHitDirection(*this, DamageContext.DamageDirection);
	return DamageContext;
}

void ACharacterBase::OnDamaged(const FBACharacterDamageContext& DamageContext)
{
	// TODO: 실제 HP/스태미너 반영 및 사망 판정은 StatComponent에서 처리
	
}

void ACharacterBase::OnDeath()
{
	CharacterState = ECharacterState::Dead;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// 사망 후 파생 클래스별 연출 중에도 기본 이동은 즉시 멈춘다.
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}
