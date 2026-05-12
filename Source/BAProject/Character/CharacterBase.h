#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterBase.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Alive		UMETA(DisplayName = "Alive"),
	Invincible	UMETA(DisplayName = "Invincible"),	// i-frame 등 무적 상태
	Dead		UMETA(DisplayName = "Dead"),
};

UCLASS()
class BAPROJECT_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void Attack();
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	FORCEINLINE bool IsAlive() const { 	return CharacterState != ECharacterState::Dead; }
	FORCEINLINE bool IsInvincible() const { return CharacterState == ECharacterState::Invincible; }
	FORCEINLINE bool CanReceiveDamage() const { return IsAlive() && IsInvincible() == false; }
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	ECharacterState CharacterState;

	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser);
	virtual void OnDeath();
};
