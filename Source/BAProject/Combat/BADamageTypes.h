// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Tables/ActionEnums.h"
#include "BADamageTypes.generated.h"

UENUM(BlueprintType)
enum class EBADamageReactionType : uint8
{
	// 일반 피격 반응: 이동 x
	HitReact,

	// 강공격이나 큰 패턴의 피격 반응: 두세걸음 이동 o
	LargeHitReact,

	// 장거리 넉백/비행 피격 반응.
	// 재생 중 추가 피격을 막아, 날아가는 도중 연속으로 맞는 상황을 방지한다.
	KnockDown
};

/**
 * BA 전투 시스템에서 TakeDamage 경계로 넘기는 피해 이벤트.
 *
 * DamageDirection은 공격 판정 원점에서 피격자 쪽으로 향하는 월드 방향이다.
 * 피격자 기준 앞/뒤/좌/우 방향은 CharacterBase가 이 값을 캐릭터 로컬 방향으로 변환해 계산한다.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FBADamageEvent : public FDamageEvent
{
	GENERATED_BODY()

	static const int32 ClassID = 1001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	EBADamageReactionType DamageReactionType = EBADamageReactionType::HitReact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FVector DamageDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FHitResult HitResult;

	virtual int32 GetTypeID() const override
	{
		return FBADamageEvent::ClassID;
	}

	virtual bool IsOfType(const int32 InID) const override
	{
		return InID == FBADamageEvent::ClassID || FDamageEvent::IsOfType(InID);
	}
};

/**
 * CharacterBase 이후의 프로젝트 내부 표준 피해 컨텍스트.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FBACharacterDamageContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float FinalDamage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	EBADamageReactionType DamageReactionType = EBADamageReactionType::HitReact;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FVector DamageDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	EActionDirection HitDirection = EActionDirection::Any;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	TObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	TObjectPtr<AController> EventInstigator = nullptr;
};
