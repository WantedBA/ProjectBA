#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyBase.h"
#include "Tables/BossMonster.h"
#include "Quest/QuestActivatable.h"
#include "Boss.generated.h"

USTRUCT(BlueprintType)
struct BAPROJECT_API FBossAttackData
{
	GENERATED_BODY()
	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Tid = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CoolTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ConditionType = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Var1 = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Var2 = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Var3 = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Weight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float IdealRange = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AttackAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ScoreMultiplier = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Attack = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimMontage> PatternMontage;
};

UCLASS()
class BAPROJECT_API ABoss : public AEnemyBase, public IQuestActivatable
{
	GENERATED_BODY()

public:
	ABoss();

	virtual void InitializeFromTable(int32 InTid) override;

	int32 ChooseBestPattern();
	virtual void ExecuteBossPattern(int32 PatternTid);

	// 패턴 Tid로 해당 패턴의 IdealRange를 조회 (없으면 0)
	float GetPatternIdealRange(int32 PatternTid) const;

	UFUNCTION()
	virtual void HandleHPChanged(float CurrentHP, float MaxHP);

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
	float CalculatePatternScore(const FBossAttackData& PatternData, AActor* Target);	
	void StartPatternCooldown(int32 PatternTid, float CoolTime);
	bool IsPatternAvailable(int32 PatternTid) const;

protected:
	bool bIsEnding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Quest")
	bool bStartPausedForQuest = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	int32 CurrentPhase = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Debug")
	bool bShowAIDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	TMap<int32, TObjectPtr<UAnimMontage>> LoadedMontageMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<FBossAttackData> BossPatterns;

	// 회피 패턴 (ConditionType=4). ChooseBestPattern에서 제외, 별도 트리거 시스템에서 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<FBossAttackData> EvasionPatterns;

	UPROPERTY()
	TMap<int32, float> PatternCooldownMap;

	TArray<int32> PendingCooldownRemove;
};
