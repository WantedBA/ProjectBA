#pragma once

#include "CoreMinimal.h"
#include "Combat/BADamageTypes.h"
#include "GameFramework/Character.h"
#include "Tables/ActionEnums.h"
#include "CharacterBase.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Alive		UMETA(DisplayName = "Alive"),
	Invincible	UMETA(DisplayName = "Invincible"),	// i-frame 등 무적 상태
	Dead		UMETA(DisplayName = "Dead"),
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttackPerfectGuardedDelegate, AActor* /*GuardingActor*/, const FHitResult& /*HitResult*/);

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

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetInvincible(bool bNewInvincible);
	
	// 무기 메쉬와 소켓 설정 시 해당 클래스에서 override 필요
	virtual class UStaticMeshComponent* GetWeaponMesh() const {return nullptr;}

	virtual bool IsGuardingAgainstDamage(const FVector& DamageDirection) const { return false; }
	virtual bool IsPerfectGuardWindowActive() const { return false; }

	// 이 캐릭터의 공격이 다른 캐릭터의 퍼펙트 가드에 막혔을 때 공격자 쪽 반응을 연결한다.
	FOnAttackPerfectGuardedDelegate OnAttackPerfectGuarded;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	ECharacterState CharacterState;

	static EBADamageReactionType ResolveDamageReactionType(FDamageEvent const& DamageEvent);
	static FVector ResolveDamageDirection(
		const AActor& DamagedActor,
		FDamageEvent const& DamageEvent,
		const AActor* DamageCauser);
	static FHitResult ResolveDamageHitResult(FDamageEvent const& DamageEvent);
	static EActionDirection ResolveHitDirection(const AActor& DamagedActor, const FVector& DamageDirection);

	virtual void OnDamaged(
		float FinalDamage,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser);
	
	virtual void OnDeath();
};
