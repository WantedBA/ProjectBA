#include "Character/CharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

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

	OnDamaged(FinalDamage, DamageEvent, EventInstigator, DamageCauser);
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

EBADamageReactionType ACharacterBase::ResolveDamageReactionType(FDamageEvent const& DamageEvent)
{
	if (DamageEvent.IsOfType(FBADamageEvent::ClassID))
	{
		const FBADamageEvent& BADamageEvent = static_cast<const FBADamageEvent&>(DamageEvent);
		return BADamageEvent.DamageReactionType;
	}

	return EBADamageReactionType::HitReact;
}

FVector ACharacterBase::ResolveDamageDirection(
	const AActor& DamagedActor,
	FDamageEvent const& DamageEvent,
	const AActor* DamageCauser)
{
	FVector DamageDirection = FVector::ZeroVector;

	if (DamageEvent.IsOfType(FBADamageEvent::ClassID))
	{
		const FBADamageEvent& BADamageEvent = static_cast<const FBADamageEvent&>(DamageEvent);
		DamageDirection = BADamageEvent.DamageDirection.GetSafeNormal();
	}
	else if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		DamageDirection = PointDamageEvent.ShotDirection.GetSafeNormal();
	}

	if (DamageDirection.IsNearlyZero() && DamageCauser)
	{
		DamageDirection = (DamagedActor.GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	}

	return DamageDirection;
}

FHitResult ACharacterBase::ResolveDamageHitResult(FDamageEvent const& DamageEvent)
{
	if (DamageEvent.IsOfType(FBADamageEvent::ClassID))
	{
		const FBADamageEvent& BADamageEvent = static_cast<const FBADamageEvent&>(DamageEvent);
		return BADamageEvent.HitResult;
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		return PointDamageEvent.HitInfo;
	}

	return FHitResult();
}

EActionDirection ACharacterBase::ResolveHitDirection(const AActor& DamagedActor, const FVector& DamageDirection)
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

void ACharacterBase::OnDamaged(
	const float FinalDamage,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
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
