#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyBase.h"
#include "Tables/BossMonster.h"
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
	int32 IdealRange = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AttackAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ScoreMultiplier = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimMontage> PatternMontage;
};

UCLASS()
class BAPROJECT_API ABoss : public AEnemyBase
{
	GENERATED_BODY()

public:
	ABoss();

	virtual void InitializeFromTable(int32 InTid) override;

	const TArray<FBossAttackData>& GetBossPatterns() const
	{
		return BossPatterns;
	}

	bool IsPatternAvailable(int32 PatternTid) const;
	void StartPatternCooldown(int32 PatternTid, float CoolTime);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	virtual void ExecuteBossPattern(int32 PatternTid);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat", meta = (DisplayName = "OnExecuteBossPattern"))
	void K2_OnExecuteBossPattern(int32 PatternTid);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	void LoadBossPatterns(int32 StageType);
	virtual void OnDeath() override;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	TMap<int32, TObjectPtr<UAnimMontage>> LoadedMontageMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<FBossAttackData> BossPatterns;

	UPROPERTY()
	TMap<int32, float> PatternCooldownMap;
};
