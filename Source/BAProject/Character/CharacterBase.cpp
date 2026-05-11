#include "Character/CharacterBase.h"

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

	OnDamaged(FinalDamage, DamageCauser);
	return FinalDamage;
}

void ACharacterBase::OnDamaged(float FinalDamage, AActor* DamageCauser)
{
	// TODO: 실제 HP/스태미너 반영 및 사망 판정은 StatComponent에서 처리
}

void ACharacterBase::OnDeath()
{
	CharacterState = ECharacterState::Dead;
}
