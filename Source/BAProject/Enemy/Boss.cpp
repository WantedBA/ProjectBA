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
#include "BrainComponent.h"
#include "AI/EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

ABoss::ABoss()
{
	PrimaryActorTick.bCanEverTick = true;
	EnemyGrade = EEnemyGrade::Boss;
}

void ABoss::BeginPlay()
{
	Super::BeginPlay();

	bIsEnding = false;


	if (MonsterTid != 0)
	{
		InitializeFromTable(MonsterTid);
	}
}

void ABoss::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (bStartPausedForQuest)
	{
		if (AAIController* AIC = Cast<AAIController>(NewController))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->PauseLogic(TEXT("WaitingForQuest"));
			}
		}
	}
}

void ABoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsEnding)
	{
		return;
	}

	PendingCooldownRemove.Reset();
	for (auto& Pair : PatternCooldownMap)
	{
		Pair.Value -= DeltaTime;

		if (Pair.Value <= 0.f)
		{
			PendingCooldownRemove.Add(Pair.Key);
		}
	}

	for (int32 Key : PendingCooldownRemove)
	{
		PatternCooldownMap.Remove(Key);
	}

	// 콤보 전환 중 플레이어 방향 추적
	if (bIsComboTransitioning)
	{
		TrackPlayerDuringComboTransition(DeltaTime);
	}

	// AI 디버그 정보 시각화
	if (bShowAIDebug)
	{
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController == nullptr)
		{
			return;
		}

		UBlackboardComponent* BBComponent = AIController->GetBlackboardComponent();
		AActor* Target = BBComponent ? Cast<AActor>(BBComponent->GetValueAsObject(BBKey::TargetActor)) : nullptr;
			
		FString DebugInfo = FString::Printf(TEXT("Phase: %d\n"), CurrentPhase);
		if (Target == nullptr)
		{
			return;
		}

		float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		DebugInfo += FString::Printf(TEXT("Target: %s (Dist: %.1f)\n"), *Target->GetName(), Dist);
		DebugInfo += TEXT("--- Pattern Scores ---\n");
		for (const FBossAttackData& Pattern : BossPatterns)
		{
			float Score = CalculatePatternScore(Pattern, Target);
			FString CooldownStr = IsPatternAvailable(Pattern.Tid) ? TEXT("Ready") : TEXT("CD");
			DebugInfo += FString::Printf(TEXT("Tid[%d]: %.2f (%s)\n"), Pattern.Tid, Score, *CooldownStr);
		}
		DrawDebugString(GetWorld(), FVector(0, 0, 150), DebugInfo, this, FColor::Yellow, DeltaTime);
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
		UE_LOG(LogTemp, Warning, TEXT("[ChooseBestPattern] Target NULL"));
		return 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ChooseBestPattern] BossPatterns.Num=%d, EvasionPatterns.Num=%d"), BossPatterns.Num(), EvasionPatterns.Num());

	int32 BestPatternTid = 0;
	float MaxScore = 0.0f;

	for (const FBossAttackData& Pattern : BossPatterns)
	{
		float Score = CalculatePatternScore(Pattern, Target);
		UE_LOG(LogTemp, Warning, TEXT("  Pattern Tid=%d, Score=%f"), Pattern.Tid, Score);
		if (Score > MaxScore)
		{
			MaxScore = Score;
			BestPatternTid = Pattern.Tid;
		}
	}

	// Fallback: 모든 패턴이 0점이면 (조건 미달 등) 쿨다운 안 끝난 건 빼고 IdealRange가 가장 적합한 패턴 강제 선택
	// 보스가 "할 게 없어서 가만히 있는" 상태 방지
	if (BestPatternTid == 0 && BossPatterns.Num() > 0)
	{
		const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		float MinDistFromIdeal = MAX_flt;

		for (const FBossAttackData& Pattern : BossPatterns)
		{
			if (IsPatternAvailable(Pattern.Tid) == false)
			{
				continue; // 쿨다운 중인 건 fallback에서도 제외
			}

			const float DistFromIdeal = FMath::Abs(Distance - Pattern.IdealRange);
			if (DistFromIdeal < MinDistFromIdeal)
			{
				MinDistFromIdeal = DistFromIdeal;
				BestPatternTid = Pattern.Tid;
			}
		}

		if (BestPatternTid != 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("  → Fallback selection (all 0 score), Tid=%d, DistFromIdeal=%f"), BestPatternTid, MinDistFromIdeal);
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

		if (Distance < PatternData.Var2)
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

	// 거리 점수 (벨 커브): IdealRange와 일치할 때 만점, 가깝거나 멀어질수록 감소
	// 최소 0.1 보장해서 사거리 밖이어도 완전히 0이 되지 않게 함 (조건 통과 시 fallback 가능)
	float FinalScore = PatternData.Weight * PatternData.ScoreMultiplier;
	float DistanceScore = 1.0f;

	if (PatternData.IdealRange > 0.0f)
	{
		const float DistFromIdeal = FMath::Abs(Distance - PatternData.IdealRange);
		const float Window = PatternData.IdealRange * 0.75f;  // 사거리의 75% 안에서 의미있는 점수
		DistanceScore = FMath::Max(0.1f, 1.0f - DistFromIdeal / Window);
	}
	FinalScore *= DistanceScore;

	// 연속 발동 페널티: 직전 패턴이면 30% 점수만 (다양성 확보)
	if (PatternData.Tid == LastUsedPatternTid)
	{
		FinalScore *= 0.3f;
	}

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

void ABoss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bIsEnding = true;

	GetWorldTimerManager().ClearTimer(ComboTransitionHandle);
	bIsComboTransitioning = false;
	PendingComboTid = 0;

	PatternCooldownMap.Empty();
	PendingCooldownRemove.Empty();
	BossPatterns.Empty();
	LoadedMontageMap.Empty();

	Super::EndPlay(EndPlayReason);
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

void ABoss::OnQuestActivated_Implementation(int32 tid)
{
	if (IsDead())
		return;

	AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController());
	if (AIC == nullptr)
		return;
	
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		AIC->EngageTarget(PlayerPawn);
	}

	if (UBrainComponent* Brain = AIC->GetBrainComponent())
	{
		Brain->ResumeLogic(TEXT("WaitingForQuest"));
	}
}

void ABoss::OnQuestDeactivated_Implementation(int32 tid)
{
	if (IsDead())
		return;
	
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->PauseLogic("WaitingForQuest");
		}
	}
}

float ABoss::GetPatternIdealRange(int32 PatternTid) const
{
	for (const FBossAttackData& Data : BossPatterns)
	{
		if (Data.Tid == PatternTid)
		{
			return Data.IdealRange;
		}
	}
	return 0.0f;
}

void ABoss::LoadBossPatterns(int32 StageType)
{
	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	BossPatterns.Empty();
	EvasionPatterns.Empty();

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
			NewData.NextComboTid = Pair.Value->NextComboTid;
			NewData.ComboTransitionTime = Pair.Value->ComboTransitionTime;
			NewData.MaxTrackingAngle = Pair.Value->MaxTrackingAngle;

			// ConditionType 4 = 회피. ChooseBestPattern에 포함시키지 않고 별도 트리거에서 사용
			if (NewData.ConditionType == 4)
			{
				EvasionPatterns.Add(NewData);
			}
			else
			{
				BossPatterns.Add(NewData);
			}
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
	UE_LOG(LogTemp, Warning, TEXT("[ABoss::ExecuteBossPattern] Tid=%d"), PatternTid);

	if (bIsEnding || IsDead())
	{
		UE_LOG(LogTemp, Warning, TEXT("  → bIsEnding/IsDead, early return"));
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
				if (bIsEnding == false)
				{
					MontageToPlay = Data.PatternMontage.LoadSynchronous();
				}

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

	if (MontageToPlay == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("  → MontageToPlay NULL (load failed)"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("  → MontageToPlay=%s, calling ExecuteAttack"), *MontageToPlay->GetName());

	// 연속 발동 페널티용: 마지막 패턴 기록
	LastUsedPatternTid = PatternTid;

	SetState(EEnemyState::Attack);

	if (CombatComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("  → CombatComponent NULL"));
		return;
	}

	// 패턴 데이터를 기반으로 CombatComponent 데이터 설정
	for (const FBossAttackData& Data : BossPatterns)
	{
		if (Data.Tid == PatternTid)
		{
			// IdealRange가 0인 경우(무한 인지용) 실제 타격 반경으로 150.0f 사용
			float HitRadius = (Data.IdealRange <= 0.0f) ? 150.0f : Data.IdealRange;
			CombatComponent->SetAttackData(HitRadius, Data.Attack, FName("Weapon"), FName("Weapon"));
			break;
		}
	}

	CombatComponent->ExecuteAttack(MontageToPlay);

	// 안전망: 몽타주 끝나면 자동으로 OnEnemyAttackAniFinished 호출
	// AN_EnemyAttackEnd Notify가 박혀있으면 두 번 호출되니까, 거기서 중복 방지 처리 필요
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ABoss::OnPatternMontageEnded);
			AnimInst->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
		}
	}
}

void ABoss::OnPatternMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABoss::OnPatternMontageEnded] Montage=%s, bInterrupted=%d, CurrentState=%d"),
		Montage ? *Montage->GetName() : TEXT("NULL"), (int32)bInterrupted, (int32)GetCurrentState());

	if (GetCurrentState() != EEnemyState::Attack)
	{
		return;
	}

	// 인터럽트되지 않은 경우에만 콤보 연결 시도
	if (!bInterrupted)
	{
		int32 NextTid = 0;
		float TransitionTime = 0.2f;

		for (const FBossAttackData& Data : BossPatterns)
		{
			if (Data.Tid == LastUsedPatternTid)
			{
				NextTid = Data.NextComboTid;
				TransitionTime = Data.ComboTransitionTime;
				ComboMaxTrackingAngle = Data.MaxTrackingAngle;
				break;
			}
		}

		if (NextTid > 0)
		{
			PendingComboTid = NextTid;
			bIsComboTransitioning = true;

			GetWorldTimerManager().SetTimer(
				ComboTransitionHandle,
				this, &ABoss::ExecutePendingCombo,
				TransitionTime, false
			);
			return;
		}
	}

	bIsComboTransitioning = false;
	GetWorldTimerManager().ClearTimer(ComboTransitionHandle);
	OnEnemyAttackAniFinished(EEnemyState::Idle);
}

void ABoss::ExecutePendingCombo()
{
	bIsComboTransitioning = false;

	if (PendingComboTid > 0 && !bIsEnding && !IsDead())
	{
		int32 Tid = PendingComboTid;
		PendingComboTid = 0;
		ExecuteBossPattern(Tid);
	}
}

void ABoss::TrackPlayerDuringComboTransition(float DeltaTime)
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BBKey::TargetActor));
	if (!Target) return;

	FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	FRotator TargetRot = ToTarget.Rotation();
	FRotator CurrentRot = GetActorRotation();

	// 현재 방향에서 타겟 방향까지의 각도 차이
	float AngleDiff = FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetRot.Yaw);

	// 최대 회전 각도만큼만 허용 (너무 많이 돌지 않게)
	float Clamped = FMath::Clamp(AngleDiff, -ComboMaxTrackingAngle, ComboMaxTrackingAngle);

	FRotator Desired = CurrentRot;
	Desired.Yaw += Clamped;

	SetActorRotation(FMath::RInterpTo(CurrentRot, Desired, DeltaTime, 8.0f));
}