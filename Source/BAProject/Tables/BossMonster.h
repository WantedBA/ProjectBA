// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "BossMonster.generated.h"

/** 보스 패턴 실행 분기 타입. 행의 PatternType(int32) 값에 대응. */
UENUM(BlueprintType)
enum class EBossPatternType : uint8
{
	Normal      = 0,   // 일반 패턴 (103 차징콤보·106 후방·107 측면 포함)
	ChargeAoE   = 1,   // 104 차징 광역
	ParryStance = 2    // 105 패리 스탠스
};

/** 패턴 발동에 요구되는 플레이어 위치 구역. 행의 RequiredZone(int32) 값에 대응. */
UENUM(BlueprintType)
enum class EBossPatternZone : uint8
{
	Any   = 0,   // 위치 무관
	Front = 1,   // 정면
	Side  = 2,   // 좌/우 측면 (107)
	Back  = 3    // 후방 (106)
};

/**
 * BossMonster.xlsx 의 각 시트 (1/2/3 StageBossAttack).
 * Utility AI 패턴 선택용 — A.식별/타입  B.선택 조건 게이트  C.가중치  D.실행 파라미터.
 */
USTRUCT(BlueprintType, meta = (BASheet = "1StageBossAttack"))
struct BAPROJECT_API F1StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	// --- A. 식별 / 타입 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Tid = 0;

	// EBossPatternType: 0=Normal, 1=ChargeAoE(104), 2=ParryStance(105)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 PatternType = 0;

	// 강공 여부 (0/1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 IsStrongAttack = 0;

	// --- B. 선택 조건 게이트 (전부 만족해야 후보) ---
	// 보스 HP%가 [MinHPPercent, MaxHPPercent] 범위일 때만. 기본 0~100
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 MinHPPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 MaxHPPercent = 100;

	// 플레이어 거리 범위. 0 = 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float MinDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float MaxDistance = 0.0f;

	// EBossPatternZone: 0=Any, 1=Front, 2=Side(107), 3=Back(106)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 RequiredZone = 0;

	// 보스전당 최대 사용 횟수. 0 = 무제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 MaxUseCount = 0;

	// --- C. 선택 가중치 (Utility 점수) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float BaseWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float ScoreMultiplier = 1.0f;

	// 재사용 대기 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float CoolTime = 0.0f;

	// --- D. 실행 파라미터 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	FString MontagePath;

	// 이동 목표 거리 (MoveToRange 도달 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float IdealRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Attack = 0;

	// 피격 반응 타입 — 0=HitReact, 1=LargeHitReact, 2=KnockDown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 DamageReactionType = 0;

	// 콤보로 이어질 다음 패턴 Tid (0 = 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 NextComboTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	float ComboTransitionTime = 0.2f;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

USTRUCT(BlueprintType, meta = (BASheet = "2StageBossAttack"))
struct BAPROJECT_API F2StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	// --- A. 식별 / 타입 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 PatternType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 IsStrongAttack = 0;

	// --- B. 선택 조건 게이트 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 MinHPPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 MaxHPPercent = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float MinDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float MaxDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 RequiredZone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 MaxUseCount = 0;

	// --- C. 선택 가중치 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float BaseWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float ScoreMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float CoolTime = 0.0f;

	// --- D. 실행 파라미터 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	FString MontagePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float IdealRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Attack = 0;

	// 피격 반응 타입 — 0=HitReact, 1=LargeHitReact, 2=KnockDown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 DamageReactionType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 NextComboTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	float ComboTransitionTime = 0.2f;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

USTRUCT(BlueprintType, meta = (BASheet = "3StageBossAttack"))
struct BAPROJECT_API F3StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	// --- A. 식별 / 타입 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 PatternType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 IsStrongAttack = 0;

	// --- B. 선택 조건 게이트 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 MinHPPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 MaxHPPercent = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float MinDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float MaxDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 RequiredZone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 MaxUseCount = 0;

	// --- C. 선택 가중치 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float BaseWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float ScoreMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float CoolTime = 0.0f;

	// --- D. 실행 파라미터 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	FString MontagePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float IdealRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Attack = 0;

	// 피격 반응 타입 — 0=HitReact, 1=LargeHitReact, 2=KnockDown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 DamageReactionType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 NextComboTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	float ComboTransitionTime = 0.2f;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};
