#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyBase.h"
#include "Tables/BossMonster.h"
#include "Boss.generated.h"

UCLASS()
class BAPROJECT_API ABoss : public AEnemyBase
{
	GENERATED_BODY()

public:
	ABoss();

	virtual void InitializeFromTable(int32 InTid) override;

	UFUNCTION(BlueprintPure, Category = "Boss|Data")
	const TArray<F1StageBossAttackRows>& GetPattern1() const { return Pattern1; }

	UFUNCTION(BlueprintPure, Category = "Boss|Data")
	const TArray<F2StageBossAttackRows>& GetPattern2() const { return Pattern2; }

	UFUNCTION(BlueprintPure, Category = "Boss|Data")
	const TArray<F3StageBossAttackRows>& GetPattern3() const { return Pattern3; }

protected:
	virtual void BeginPlay() override;
	void LoadBossPatterns(int32 StageType);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<F1StageBossAttackRows> Pattern1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<F2StageBossAttackRows> Pattern2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Data")
	TArray<F3StageBossAttackRows> Pattern3;
};
