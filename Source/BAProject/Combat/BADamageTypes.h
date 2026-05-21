// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
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
 * BA 전투 시스템에서 TakeDamage 경계로 넘길 수 있는 프로젝트 전용 피해 이벤트.
 *
 * FPointDamageEvent는 HitResult와 ShotDirection을 이미 제공하므로, 단순한 타격 지점/방향 전달에는 충분하다.
 * 그래도 이 타입을 남기는 이유는 HitReact/LargeHitReact/KnockDown 같은 BA 전용 피격 반응 메타데이터가
 * UE 기본 FDamageEvent 계열에는 없기 때문이다. 공격자가 피격 반응 강도만 런타임으로 지정해야 할 때
 * DamageReactionType을 TakeDamage 경계로 함께 넘기기 위한 최소 확장점으로 사용한다.
 *
 * DamageDirection은 충돌면 법선이 아니라 공격자에서 피격자 쪽으로 향하는 게임플레이 방향이다.
 * 가드 각도, 피격 방향 몽타주, 넉백 방향처럼 “어느 방향에서 맞았는가”를 판단할 때 사용한다.
 * HitResult는 시각 연출 위치나 fallback 정보가 필요할 때만 사용하고, 표면 법선 기반 판정과 혼동하지 않는다.
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

	// CombatComponent가 피해 적용 시점에 확인한 피해자 가드 상태다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage|Guard")
	bool bVictimGuarding = false;

	// 피해자의 가드 몽타주 퍼펙트 윈도우 안에서 성립한 가드인지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage|Guard")
	bool bVictimPerfectGuard = false;

	virtual int32 GetTypeID() const override
	{
		return FBADamageEvent::ClassID;
	}

	virtual bool IsOfType(const int32 InID) const override
	{
		return InID == FBADamageEvent::ClassID || FDamageEvent::IsOfType(InID);
	}
};
