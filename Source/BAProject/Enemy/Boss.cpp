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
#include "Components/StaticMeshComponent.h"
#include "BrainComponent.h"
#include "AI/EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

// Utility 패턴 선택 튜닝 상수 (밸런싱 시 한곳에서 조정)
namespace BossPatternTuning
{
	static constexpr float RepeatPenalty    = 0.3f;    // 직전 패턴 재선택 시 점수 배율
	static constexpr float StrongAttackBias = 1.0f;    // 강공 점수 배율 (1.0 = 무영향, 밸런싱용)
	static constexpr float FrontHalfAngle   = 60.0f;   // 정면 판정 반각 (|angle| 이하 = Front)
	static constexpr float BackHalfAngle    = 135.0f;  // 후방 판정 (|angle| 이상 = Back)
}

ABoss::ABoss()
{
	PrimaryActorTick.bCanEverTick = true;
	EnemyGrade = EEnemyGrade::Boss;
}

void ABoss::BeginPlay()
{
	Super::BeginPlay();

	bIsEnding = false;

	// BP에 추가된 무기 메시 컴포넌트를 태그로 찾아 캐싱 (히트 트레이스 소켓 조회용)
	TArray<UActorComponent*> WeaponComps = GetComponentsByTag(UStaticMeshComponent::StaticClass(), WeaponComponentTag);
	if (WeaponComps.Num() > 0)
	{
		CachedWeaponMesh = Cast<UStaticMeshComponent>(WeaponComps[0]);
	}
	else
	{
		// 태그를 못 찾으면 첫 StaticMeshComponent로 폴백 (보스 본체는 SkeletalMesh라 보통 무기뿐)
		CachedWeaponMesh = FindComponentByClass<UStaticMeshComponent>();
		UE_LOG(LogTemp, Warning,
			TEXT("[ABoss] 무기 태그 '%s' 미발견 → 첫 StaticMeshComponent 폴백(%s). BP에서 Component Tag 지정 권장."),
			*WeaponComponentTag.ToString(),
			CachedWeaponMesh ? *CachedWeaponMesh->GetName() : TEXT("없음"));
	}

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

	int32 BestPatternTid = 0;
	float MaxScore = 0.0f;

	for (const FBossAttackData& Pattern : BossPatterns)
	{
		const float Score = CalculatePatternScore(Pattern, Target);
		if (Score > MaxScore)
		{
			MaxScore = Score;
			BestPatternTid = Pattern.Tid;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ChooseBestPattern] Selected Tid=%d (Score=%.2f)"), BestPatternTid, MaxScore);
	return BestPatternTid;
}

float ABoss::CalculatePatternScore(const FBossAttackData& PatternData, AActor* Target)
{
	if (Target == nullptr)
	{
		return 0.0f;
	}

	// --- 게이트: 하나라도 실패하면 후보에서 제외 (0점) ---

	// (Step 2 한정) 특수 패턴은 실행부 미구현 — Step 5·6에서 이 가드 해제
	if (PatternData.PatternType != EBossPatternType::Normal)
	{
		return 0.0f;
	}

	// 쿨타임
	if (IsPatternAvailable(PatternData.Tid) == false)
	{
		return 0.0f;
	}

	// 보스전 사용 횟수 제한 (MaxUseCount 0 = 무제한)
	if (PatternData.MaxUseCount > 0)
	{
		if (PatternUseCount.FindRef(PatternData.Tid) >= PatternData.MaxUseCount)
		{
			return 0.0f;
		}
	}

	// 보스 HP% 범위
	if (StatComponent)
	{
		const float MaxHP = StatComponent->GetMaxHP();
		if (MaxHP > 0.0f)
		{
			const int32 HPPercent = FMath::RoundToInt(StatComponent->GetCurrentHP() / MaxHP * 100.0f);
			if (HPPercent < PatternData.MinHPPercent || HPPercent > PatternData.MaxHPPercent)
			{
				return 0.0f;
			}
		}
	}

	// 플레이어 거리 범위 (0 = 제한 없음)
	const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (PatternData.MinDistance > 0.0f && Distance < PatternData.MinDistance)
	{
		return 0.0f;
	}
	if (PatternData.MaxDistance > 0.0f && Distance > PatternData.MaxDistance)
	{
		return 0.0f;
	}

	// 플레이어 위치 구역
	if (PatternData.RequiredZone != EBossPatternZone::Any)
	{
		if (GetPlayerZone(Target) != PatternData.RequiredZone)
		{
			return 0.0f;
		}
	}

	// --- 게이트 통과: Utility 점수 산출 ---
	float Score = PatternData.BaseWeight * PatternData.ScoreMultiplier;

	// 연속 발동 페널티 (다양성 확보)
	if (PatternData.Tid == LastUsedPatternTid)
	{
		Score *= BossPatternTuning::RepeatPenalty;
	}

	// 강공 보정 (StrongAttackBias 기본 1.0이면 무영향 — 밸런싱 시 조정)
	if (PatternData.bIsStrongAttack)
	{
		Score *= BossPatternTuning::StrongAttackBias;
	}

	return Score;
}

EBossPatternZone ABoss::GetPlayerZone(AActor* Target) const
{
	if (Target == nullptr)
	{
		return EBossPatternZone::Any;
	}

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Dot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToTarget);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

	if (AngleDeg <= BossPatternTuning::FrontHalfAngle)
	{
		return EBossPatternZone::Front;
	}
	if (AngleDeg >= BossPatternTuning::BackHalfAngle)
	{
		return EBossPatternZone::Back;
	}
	return EBossPatternZone::Side;
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
	PatternUseCount.Empty();
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

UAnimMontage* ABoss::PlayTurnToTarget(AActor* Target)
{
	if (Target == nullptr)
	{
		return nullptr;
	}

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return nullptr;
	}

	const float TargetYaw = ToTarget.Rotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetYaw);
	const float AbsDelta = FMath::Abs(DeltaYaw);

	// 정면 기준 45도 이내면 회전 불필요 (공격 추적이 잔여 오차를 흡수)
	if (AbsDelta <= 45.0f)
	{
		return nullptr;
	}

	// DeltaYaw > 0 = 타겟이 오른쪽, < 0 = 왼쪽 (UE Yaw 기준)
	UAnimMontage* TurnMontage = nullptr;
	if (DeltaYaw > 0.0f)
	{
		TurnMontage = (AbsDelta > 135.0f) ? TurnRight180Montage : TurnRight90Montage;
	}
	else
	{
		TurnMontage = (AbsDelta > 135.0f) ? TurnLeft180Montage : TurnLeft90Montage;
	}

	if (TurnMontage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABoss::PlayTurnToTarget] Turn montage not assigned (DeltaYaw=%.1f)"), DeltaYaw);
		return nullptr;
	}

	const float Duration = PlayAnimMontage(TurnMontage);
	return (Duration > 0.0f) ? TurnMontage : nullptr;
}

UStaticMeshComponent* ABoss::GetWeaponMesh() const
{
	return CachedWeaponMesh;
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

			// A. 식별 / 타입
			NewData.Tid             = Pair.Value->Tid;
			NewData.PatternType     = static_cast<EBossPatternType>(Pair.Value->PatternType);
			NewData.bIsStrongAttack = (Pair.Value->IsStrongAttack != 0);

			// B. 선택 조건 게이트
			NewData.MinHPPercent    = Pair.Value->MinHPPercent;
			NewData.MaxHPPercent    = Pair.Value->MaxHPPercent;
			NewData.MinDistance     = Pair.Value->MinDistance;
			NewData.MaxDistance     = Pair.Value->MaxDistance;
			NewData.RequiredZone    = static_cast<EBossPatternZone>(Pair.Value->RequiredZone);
			NewData.MaxUseCount     = Pair.Value->MaxUseCount;

			// C. 선택 가중치
			NewData.BaseWeight      = Pair.Value->BaseWeight;
			NewData.ScoreMultiplier = Pair.Value->ScoreMultiplier;
			NewData.CoolTime        = Pair.Value->CoolTime;

			// D. 실행 파라미터
			NewData.PatternMontage      = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(Pair.Value->MontagePath));
			NewData.IdealRange          = Pair.Value->IdealRange;
			NewData.Attack              = Pair.Value->Attack;
			NewData.NextComboTid        = Pair.Value->NextComboTid;
			NewData.ComboTransitionTime = Pair.Value->ComboTransitionTime;

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
	UE_LOG(LogTemp, Warning, TEXT("[ABoss::ExecuteBossPattern] Tid=%d"), PatternTid);

	if (bIsEnding || IsDead())
	{
		UE_LOG(LogTemp, Warning, TEXT("  → bIsEnding/IsDead, early return"));
		return;
	}

	// 패턴 데이터 1회 조회 — 몽타주/쿨타임/사용횟수/전투 데이터에 모두 사용
	const FBossAttackData* PatternData = BossPatterns.FindByPredicate(
		[PatternTid](const FBossAttackData& Data) { return Data.Tid == PatternTid; });

	if (PatternData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("  → PatternTid=%d not found in BossPatterns"), PatternTid);
		return;
	}

	// 몽타주: 캐시 우선, 없으면 로드
	UAnimMontage* MontageToPlay = LoadedMontageMap.FindRef(PatternTid);
	if (MontageToPlay == nullptr && bIsEnding == false)
	{
		MontageToPlay = PatternData->PatternMontage.LoadSynchronous();
		if (MontageToPlay)
		{
			LoadedMontageMap.Add(PatternTid, MontageToPlay);
		}
	}

	if (MontageToPlay == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("  → MontageToPlay NULL (load failed)"));
		return;
	}

	// 쿨타임·사용횟수는 실행할 때마다 갱신 (캐시 히트/미스와 무관)
	StartPatternCooldown(PatternTid, PatternData->CoolTime);
	PatternUseCount.FindOrAdd(PatternTid)++;

	UE_LOG(LogTemp, Warning, TEXT("  → MontageToPlay=%s, calling ExecuteAttack"), *MontageToPlay->GetName());

	// 연속 발동 페널티용: 마지막 패턴 기록
	LastUsedPatternTid = PatternTid;

	SetState(EEnemyState::Attack);

	if (CombatComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("  → CombatComponent NULL"));
		return;
	}

	// 타격 소켓은 CombatComponent에 설정된 StartSocketName/EndSocketName을 그대로 사용한다.
	// 소켓 인자를 생략(NAME_None)하면 SetAttackData가 기존 소켓 이름을 덮어쓰지 않는다.
	// IdealRange는 AI 위치 선정용 거리라 타격 반경으로 쓰면 안 된다 → 검 두께(WeaponHitRadius) 사용.
	CombatComponent->SetAttackData(WeaponHitRadius, PatternData->Attack);

	CombatComponent->ExecuteAttack(MontageToPlay);

	// 안전망: 몽타주 끝나면 자동으로 OnPatternMontageEnded 호출
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

	const EEnemyState State = GetCurrentState();

	// 공격 상태가 아니다 = AN_EnemyAttackEnd 노티 또는 피격이 이미 상태를 바꿔놓았다.
	if (State != EEnemyState::Attack)
	{
		// 피격/경직으로 공격이 강제 중단된 경우, BT 래턴트 태스크(ExecuteBossPattern)는
		// 아직 InProgress로 살아있다. 종료 신호를 안 보내면 트리가 영구히 멈춰 보스가 정지한다.
		if (bInterrupted && (State == EEnemyState::Hit || State == EEnemyState::Stagger))
		{
			bIsComboTransitioning = false;
			PendingComboTid = 0;
			GetWorldTimerManager().ClearTimer(ComboTransitionHandle);
			OnAttackAnimationFinished.Broadcast(State);
		}
		// 그 외(노티가 이미 정상 처리)는 중복 방지를 위해 아무것도 하지 않는다.
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
