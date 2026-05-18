#include "Enemy/Boss.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"

ABoss::ABoss()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABoss::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterTid != 0)
	{
		InitializeFromTable(MonsterTid);
	}
}

void ABoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (auto It = PatternCooldownMap.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaTime;
		if (It.Value() <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}
}

void ABoss::InitializeFromTable(int32 InTid)
{
	Super::InitializeFromTable(InTid);

	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	const FMonsterRows* Row = TableManager->FindMonster(InTid);
	if (Row)
	{
		LoadBossPatterns(Row->StageType);
	}
}

void ABoss::LoadBossPatterns(int32 StageType)
{
	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	BossPatterns.Empty();

	auto MapToBossData = [&](const auto& Rows) 
	{
		for (const auto& Pair : Rows)
		{
			if (Pair.Value == nullptr)
			{
				continue;
			}

			FBossAttackData NewData;
			NewData.Tid = Pair.Value->Tid;
			NewData.PatternMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(Pair.Value->MontagePath));
			NewData.CoolTime = static_cast<float>(Pair.Value->CoolTime);
			NewData.ConditionType = Pair.Value->ConditionType;
			NewData.Var1 = Pair.Value->Var1;
			NewData.Var2 = Pair.Value->Var2;
			NewData.Var3 = Pair.Value->Var3;
			NewData.Weight = Pair.Value->Weight;
			NewData.IdealRange = Pair.Value->IdealRange;
			NewData.AttackAngle = Pair.Value->AttackAngle;
			NewData.ScoreMultiplier = Pair.Value->ScoreMultiplier;

			BossPatterns.Add(NewData);
		}
	};

	switch (StageType)
	{
	case 1:
		MapToBossData(TableManager->GetBossAttackMap1());
		break;
	case 2:
		MapToBossData(TableManager->GetBossAttackMap2());
		break;
	case 3:
		MapToBossData(TableManager->GetBossAttackMap3());
		break;
	default:
		break;
	}
}

bool ABoss::IsPatternAvailable(int32 PatternTid) const
{
	return PatternCooldownMap.Contains(PatternTid) == false;
}

void ABoss::StartPatternCooldown(int32 PatternTid, float CoolTime)
{
	if (CoolTime > 0.0f)
	{
		PatternCooldownMap.Add(PatternTid, CoolTime);
	}
}

void ABoss::ExecuteBossPattern(int32 PatternTid)
{
	if (IsDead())
	{
		return;
	}

	if (LoadedMontageMap.Contains(PatternTid))
	{
		UAnimMontage* CachedMontage = LoadedMontageMap[PatternTid];
		if (CachedMontage)
		{
			SetState(EEnemyState::Attack);
			PlayAnimMontage(CachedMontage);
			K2_OnExecuteBossPattern(PatternTid);
			return;
		}
	}

	for (const FBossAttackData& Data : BossPatterns)
	{
		if (Data.Tid == PatternTid)
		{
			UAnimMontage* LoadedMontage = Data.PatternMontage.LoadSynchronous();
			if (LoadedMontage)
			{
				LoadedMontageMap.Add(PatternTid, LoadedMontage);

				SetState(EEnemyState::Attack);
				PlayAnimMontage(LoadedMontage);
			}
			break;
		}
	}

	K2_OnExecuteBossPattern(PatternTid);
}
