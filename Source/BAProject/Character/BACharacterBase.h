#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BACharacterBase.generated.h"

UENUM(BlueprintType)
enum class EBACharacterState : uint8
{
	Alive		UMETA(DisplayName = "Alive"),
	Invincible	UMETA(DisplayName = "Invincible"),	// i-frame 등 무적 상태
	Dead		UMETA(DisplayName = "Dead"),
};

UCLASS()
class BAPROJECT_API ABACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ABACharacterBase();

	virtual void Attack();
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	FORCEINLINE bool IsAlive() const { 	return CharacterState != EBACharacterState::Dead; }
	FORCEINLINE bool IsInvincible() const { return CharacterState == EBACharacterState::Invincible; }
	FORCEINLINE bool CanReceiveDamage() const { return IsAlive() && IsInvincible() == false; }
	FORCEINLINE EBACharacterState GetCharacterState() const { return CharacterState; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	EBACharacterState CharacterState;

	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser);
	virtual void OnDeath();
};
