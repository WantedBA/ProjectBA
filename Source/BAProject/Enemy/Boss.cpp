#include "Enemy/Boss.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "Component/CombatComponent.h"
#include "Component/StatComponent.h"
#include "Engine/SkeletalMesh.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"

ABoss::ABoss()
{
	PrimaryActorTick.bCanEverTick = true;
	EnemyGrade = EEnemyGrade::Boss;
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

	// AI 디버그 정보 시각화
	if (bShowAIDebug)
	{
		AAIController* AIC = Cast<AAIController>(GetController());
		if (AIC)
		{
			UBlackboardComponent* BB = AIC->GetBlackboardComponent();
			AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BBKey::TargetActor)) : nullptr;
			
			FString DebugInfo = FString::Printf(TEXT("Phase: %d\n"), CurrentPhase);
			if (Target)
			{
				float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
				DebugInfo += FString::Printf(TEXT("Target: %s (Dist: %.1f)\n"), *Target->GetName(), Dist);
				
				DebugInfo += TEXT("--- Pattern Scores ---\n");
				for (const FBossAttackData& Pattern : BossPatterns)
				{
					float Score = CalculatePatternScore(Pattern, Target);
					FString CooldownStr = IsPatternAvailable(Pattern.Tid) ? TEXT("Ready") : TEXT("CD");
					DebugInfo += FString::Printf(TEXT("Tid[%d]: %.2f (%s)\n"), Pattern.Tid, Score, *CooldownStr);
				}
			}

			DrawDebugString(GetWorld(), FVector(0, 0, 150), DebugInfo, this, FColor::Yellow, DeltaTime);
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
	if (Row == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss InitializeFromTable: No MonsterRow found for Tid %d"), InTid);
		return;
	}

	LoadBossPatterns(Row->StageType);
}

int32 ABoss::ChooseBestPattern()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC == nullptr)
	{
		return 0;
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (BB == nullptr)
	{
		return 0;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BBKey::TargetActor));
	if (Target == nullptr)
	{
		return 0;
	}

	int32 BestPatternTid = 0;
	float MaxScore = -1.0f;

	for (const FBossAttackData& Pattern : BossPatterns)
	{
		float Score = CalculatePatternScore(Pattern, Target);
		if (Score > MaxScore)
		{
			MaxScore = Score;
			BestPatternTid = Pattern.Tid;
		}
	}

	return BestPatternTid;
}

float ABoss::CalculatePatternScore(const FBossAttackData& PatternData, AActor* Target)
{
	if (Target == nullptr)
	{
		return 0.0f;
	}

	// 쿨타임 체크
	if (IsPatternAvailable(PatternData.Tid) == false)
	{
		return 0.0f;
	}

	// 컨디션 체크
	const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	const float RandVal = FMath::RandRange(1, 10000);

	float HPRatio = 1.0f;
	if (StatComponent)
	{
		HPRatio = StatComponent->GetCurrentHP() / StatComponent->GetMaxHP();
	}

	bool bConditionMet = true;
	switch (PatternData.ConditionType)
	{
	case 1: // 1. Pure probability, Var1 = 확률
		if (RandVal > PatternData.Var1)
		{
			bConditionMet = false;
		}
		break;

	case 2: // 2. HP + probability, Var1 = 확률, Var2 = HP %
		if (RandVal > PatternData.Var1)
		{
			bConditionMet = false;
			break;
		}

		if (HPRatio * 100.0f > PatternData.Var2)
		{
			bConditionMet = false;
		}
		break;

	case 3: // 3. Distance + probability, Var1 = 확률, Var2 = 거리
		if (RandVal > PatternData.Var1)
		{
			bConditionMet = false;
			break;
		}

		if (Distance > PatternData.Var2)
		{
			bConditionMet = false;
		}
		break;

	case 4: // 4. Phase range, Var2 ~ Var3
		if (CurrentPhase < PatternData.Var2 || CurrentPhase > PatternData.Var3)
		{
			bConditionMet = false;
		}
		break;

	default:
		bConditionMet = false;
		break;
	}

	if (!bConditionMet)
	{
		return 0.0f;
	}

	// 거리 점수 (IdealRange 이하일 때 가장 높음)
	float FinalScore = PatternData.Weight * PatternData.ScoreMultiplier;
	float DistanceScore = 1.0f;

	if (PatternData.IdealRange > 0.0f)
	{
		if (Distance > PatternData.IdealRange)
		{
			DistanceScore = FMath::Max(0.0f,1.0f - (Distance - PatternData.IdealRange) / 1000.0f);
		}
	}
	FinalScore *= DistanceScore;

	// 각도 점수
	FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	float Dot = FVector::DotProduct(GetActorForwardVector(), ToTarget);

	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	if (Angle > PatternData.AttackAngle)
	{
		FinalScore *= 0.2f;
	}

	return FinalScore;
}

void ABoss::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatComponent)
	{
		StatComponent->OnHPChanged.AddDynamic(this, &ABoss::HandleHPChanged);
	}
}

void ABoss::HandleHPChanged(float CurrentHP, float MaxHP)
{
	float HPRatio = CurrentHP / MaxHP;

	int32 NewPhase = 1;
	if (HPRatio <= 0.3f)
	{
		NewPhase = 3;
	}
	else if (HPRatio <= 0.6f)
	{
		NewPhase = 2;
	}

	if (NewPhase != CurrentPhase)
	{
		CurrentPhase = NewPhase;
		
		// 페이즈 전환 시 로직 (예: 광폭화, 패턴 추가 등)
		UE_LOG(LogTemp, Warning, TEXT("Boss Phase Changed: %d"), CurrentPhase);
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
			NewData.IdealRange = static_cast<float>(Pair.Value->IdealRange);
			NewData.AttackAngle = static_cast<float>(Pair.Value->AttackAngle);
			NewData.ScoreMultiplier = Pair.Value->ScoreMultiplier;
			NewData.Attack = static_cast<float>(Pair.Value->Attack);

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
	const float* Cooldown = PatternCooldownMap.Find(PatternTid);
	if (Cooldown == nullptr)
	{
		return true;
	}

	return *Cooldown <= 0.0f;
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

	UAnimMontage* MontageToPlay = nullptr;

	if (LoadedMontageMap.Contains(PatternTid))
	{
		MontageToPlay = LoadedMontageMap[PatternTid];
	}
	else
	{
		for (const FBossAttackData& Data : BossPatterns)
		{
			if (Data.Tid == PatternTid)
			{
				MontageToPlay = Data.PatternMontage.LoadSynchronous();
				if (MontageToPlay)
				{
					LoadedMontageMap.Add(PatternTid, MontageToPlay);
				}
				
				// 쿨타임 시작
				StartPatternCooldown(PatternTid, Data.CoolTime);
				break;
			}
		}
	}

	if (MontageToPlay)
	{
		SetState(EEnemyState::Attack);
		if (CombatComponent)
		{
			// 패턴 데이터를 기반으로 CombatComponent 데이터 설정
			for (const FBossAttackData& Data : BossPatterns)
			{
				if (Data.Tid == PatternTid)
				{
					CombatComponent->SetAttackData(Data.IdealRange, Data.Attack, TEXT("Muzzle"));
					break;
				}
			}
			CombatComponent->ExecuteAttack(MontageToPlay);
		}
	}
}