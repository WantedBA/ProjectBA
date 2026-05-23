#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyBase.h"
#include "Tables/BossMonster.h"
#include "Quest/QuestActivatable.h"
#include "Boss.generated.h"

class UStaticMeshComponent;

// 보스 공격 패턴 1개의 런타임 데이터. BossMonster 테이블 행을 LoadBossPatterns에서 매핑.
USTRUCT(BlueprintType)
struct BAPROJECT_API FBossAttackData
{
	GENERATED_BODY()

	// --- A. 식별 / 타입 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Tid = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBossPatternType PatternType = EBossPatternType::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsStrongAttack = false;

	// --- B. 선택 조건 게이트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MinHPPercent = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MaxHPPercent = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MinDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MaxDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBossPatternZone RequiredZone = EBossPatternZone::Any;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MaxUseCount = 0;

	// --- C. 선택 가중치 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BaseWeight = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ScoreMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CoolTime = 0.0f;

	// --- D. 실행 파라미터 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimMontage> PatternMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float IdealRange = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Attack = 0;

	// 피격 반응 타입. 플레이어가 피격 반응 애니메이션·넉백 강도 분기에 사용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBADamageReactionType DamageReactionType = EBADamageReactionType::HitReact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NextComboTid = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ComboTransitionTime = 0.2f;
};

UCLASS()
class BAPROJECT_API ABoss : public AEnemyBase, public IQuestActivatable
{
	GENERATED_BODY()

public:
	ABoss();

	virtual void InitializeFromTable(int32 InTid) override;

	// Utility AI — 현재 상황에서 가장 점수 높은 패턴 Tid 반환 (후보 없으면 0)
	int32 ChooseBestPattern();
	// 공격을 실제로 시작했으면 true. 몽타주 로드 실패 등으로 못 했으면 false
	// (false면 BT 태스크가 InProgress로 갇히지 않도록 호출부가 즉시 종료해야 한다).
	virtual bool ExecuteBossPattern(int32 PatternTid);

	// 패턴 Tid로 해당 패턴의 IdealRange를 조회 (없으면 0)
	float GetPatternIdealRange(int32 PatternTid) const;

	// 타겟 방향으로 회전 몽타주를 재생. 재생한 몽타주 반환 (회전 불필요/실패 시 nullptr)
	UAnimMontage* PlayTurnToTarget(AActor* Target);

	// 히트 트레이스용 무기 메시 반환. 보스는 검이 별도 컴포넌트라 GetMesh() 대신 이걸 쓴다.
	virtual UStaticMeshComponent* GetWeaponMesh() const override;

	UFUNCTION()
	virtual void HandleHPChanged(float CurrentHP, float MaxHP);

	// 패턴 몽타주 종료 콜백 (Anim Notify 누락 시 안전망)
	UFUNCTION()
	void OnPatternMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void ExecutePendingCombo();

	//Quest 인터페이스
	virtual void OnQuestActivated_Implementation(int32 tid) override;
	virtual void OnQuestDeactivated_Implementation(int32 tid) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool IsPersistentAggro() const override { return true; }

private:
	void LoadBossPatterns(int32 StageType);

	// Utility 점수 계산. 조건 게이트를 통과 못 하면 0 반환.
	float CalculatePatternScore(const FBossAttackData& PatternData, AActor* Target);

	// 플레이어가 보스 정면 기준 어느 구역(정면/측면/후방)에 있는지
	EBossPatternZone GetPlayerZone(AActor* Target) const;

	void StartPatternCooldown(int32 PatternTid, float CoolTime);
	bool IsPatternAvailable(int32 PatternTid) const;
	void TrackPlayerDuringComboTransition(float DeltaTime);

protected:
	bool bIsEnding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Quest")
	bool bStartPausedForQuest = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	int32 CurrentPhase = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Debug")
	bool bShowAIDebug = true;

	// 회전 몽타주 — 캡슐을 실제로 돌리려면 애니메이션에 Enable Root Motion(회전) 필수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	TObjectPtr<UAnimMontage> TurnLeft90Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	TObjectPtr<UAnimMontage> TurnRight90Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	TObjectPtr<UAnimMontage> TurnLeft180Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Turn")
	TObjectPtr<UAnimMontage> TurnRight180Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	TMap<int32, TObjectPtr<UAnimMontage>> LoadedMontageMap;

	// 검 타격 판정의 두께(박스 트레이스 반경). 소켓 위치는 CombatComponent의
	// StartSocketName/EndSocketName을 사용하므로, 그 두 소켓 사이를 이 두께로 스윕한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float WeaponHitRadius = 15.0f;

	// BP의 무기 메시 컴포넌트에 달아둘 태그. BeginPlay에서 이 태그로 무기 컴포넌트를 찾는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	FName WeaponComponentTag = TEXT("Weapon");

	// 태그로 찾은 무기 메시 컴포넌트 캐시. GetWeaponMesh()가 반환한다.
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> CachedWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<FBossAttackData> BossPatterns;

	UPROPERTY()
	TMap<int32, float> PatternCooldownMap;

	// 패턴별 보스전 누적 사용 횟수 (MaxUseCount 제한 체크용)
	UPROPERTY()
	TMap<int32, int32> PatternUseCount;

	// 직전에 발동한 패턴 Tid. 연속 발동 페널티에 사용 (다양성 확보)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	int32 LastUsedPatternTid = 0;

	// UI Boss 이름 저장
	UPROPERTY()
	FText CachedBossName;

	TArray<int32> PendingCooldownRemove;

	bool bIsComboTransitioning = false;
	int32 PendingComboTid = 0;
	float ComboMaxTrackingAngle = 90.0f;

	FTimerHandle ComboTransitionHandle;
};
