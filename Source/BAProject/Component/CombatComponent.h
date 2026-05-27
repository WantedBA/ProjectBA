#pragma once

#include "CoreMinimal.h"
#include "Combat/BADamageTypes.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitDetected, AActor*, Victim, const FHitResult&, HitResult);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDamageResolved, AActor* /*Victim*/, const FHitResult& /*HitResult*/, float /*AppliedDamage*/);

USTRUCT()
struct FCombatTargetHitRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Target;

	UPROPERTY()
	int32 HitCount = 0;
};

/**
 * 근접 전투 판정과 타격 피드백을 담당하는 컴포넌트.
 *
 * 공격 몽타주 재생, 무기 소켓 기반 sphere sweep, 피해 적용,
 * 충격파/왜곡 VFX를 처리한다. 한 번의 공격 안에서는 같은 액터의
 * 타격 횟수를 제한하도록 타겟별 히트 기록을 유지한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	// 소유 캐릭터의 애니메이션 몽타주를 지정한 재생 속도로 실행한다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ExecuteAttack(UAnimMontage* AttackMontage, float PlayRate = 1.0f);

	// 지정한 반경/피해량/소켓 기준으로 매 프레임 히트 스윕을 시작한다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHitStart(float InRadius, float InDamage, FName InStartSocket = NAME_None, FName InEndSocket = NAME_None);

	// 현재 진행 중인 히트 스윕을 종료하고 컴포넌트 Tick을 끈다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHitEnd();

	// 노티파이 등에서 기본값으로 사용할 공격 판정 데이터를 저장한다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAttackData(
		float InRadius,
		float InDamage,
		FName InStartSocket = NAME_None,
		FName InEndSocket = NAME_None,
		EBADamageReactionType InDamageReactionType = EBADamageReactionType::HitReact,
		float InLaunchHorizontalSpeed = 0.f,
		float InLaunchVerticalSpeed = 0.f);

	// 타격 위치에 Shockwave/Distortion Niagara 효과를 생성한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnShockwave(FVector Location, float Scale = 1.0f);

	// SetAttackData로 저장된 기본 판정값으로 히트 체크를 시작한다.
	void CheckHitStartDefault();

	void SetShowDebugTrace(bool bInShowDebugTrace)
	{
		this->bShowDebugTrace = bInShowDebugTrace;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	EBADamageReactionType GetDamageReactionType() const { return CurrentDamageReactionType; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCurrentDamage() const { return CurrentDamage; }

	// 0이면 제한 없음. 기본값 1은 한 공격에서 같은 타겟을 한 번만 타격한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Limit")
	void SetMaxHitsPerTargetPerAttack(int32 InMaxHits);

	UFUNCTION(BlueprintPure, Category = "Combat|Hit Limit")
	int32 GetMaxHitsPerTargetPerAttack() const { return MaxHitsPerTargetPerAttack; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ProcessHitCheck(); // 이전 프레임과 현재 프레임의 무기 선분 사이를 박스 스윕

	void ApplyDamage(AActor* Victim, const FHitResult& HitResult);
	void ResetTargetHitRecords();
	bool CanRegisterHit(AActor* Victim) const;
	void RegisterTargetHit(AActor* Victim);
	FCombatTargetHitRecord* FindHitRecord(AActor* Victim);
	const FCombatTargetHitRecord* FindHitRecord(AActor* Victim) const;

public:
	// 새 피해 대상이 감지되고 피해 적용까지 끝난 뒤 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable)
	FOnHitDetected OnHitDetected;

	// TakeDamage 반환값까지 필요한 공격자 쪽 피드백용 이벤트.
	FOnDamageResolved OnDamageResolved;

protected:
	// 타격 시 생성할 Shockwave 나이아가라 시스템
	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> ShockwaveSystem;

	// 타격 시 생성할 Distortion 나이아가라 시스템
	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> DistortionSystem;

	// 무기 히트 트레이스 PIE 화면 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	bool bShowDebugTrace = false;

	UPROPERTY()
	bool bIsHitChecking = false;

	UPROPERTY()
	float CurrentRadius;

	UPROPERTY()
	float CurrentDamage;

	// SetAttackData 호출 전 기본 피격 리액션 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EBADamageReactionType CurrentDamageReactionType = EBADamageReactionType::HitReact;

	// 한 공격에서 같은 타겟을 몇 번까지 맞출 수 있는지 제한한다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit Limit", meta = (ClampMin = "0"))
	int32 MaxHitsPerTargetPerAttack = 1;

	// SetAttackData 호출 전 기본 수평 런치 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Launch", meta = (ClampMin = "0.0"))
	float CurrentLaunchHorizontalSpeed = 0.f;

	// SetAttackData 호출 전 기본 수직 런치 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Launch", meta = (ClampMin = "0.0"))
	float CurrentLaunchVerticalSpeed = 0.f;

	// KnockDown 근접 전방 fallback 거리 여유
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Launch", meta = (ClampMin = "0.0"))
	float KnockDownDirectionContactTolerance = 220.f;

	// 히트 트레이스 시작 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Socket")
	FName StartSocketName = TEXT("Sword_Start");

	// 히트 트레이스 끝 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Socket")
	FName EndSocketName = TEXT("Sword_End");

	UPROPERTY()
	FVector PrevStartLocation;

	UPROPERTY()
	FVector PrevEndLocation;

	UPROPERTY()
	TArray<FCombatTargetHitRecord> TargetHitRecords;
};
