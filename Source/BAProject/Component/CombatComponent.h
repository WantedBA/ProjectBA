#pragma once

#include "CoreMinimal.h"
#include "Combat/BADamageTypes.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitDetected, AActor*, Victim, const FHitResult&, HitResult);

/**
 * 근접 전투 판정과 타격 피드백을 담당하는 컴포넌트.
 *
 * 공격 몽타주 재생, 무기 소켓 기반 sphere sweep, 피해 적용, 히트스톱,
 * 충격파/왜곡 VFX를 한곳에서 처리한다. 한 번의 판정 구간 안에서는 같은 액터를
 * 중복 타격하지 않도록 HitActors를 유지한다.
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
		EBADamageReactionType InDamageReactionType = EBADamageReactionType::HitReact);

	// 전역 시간 배율을 짧게 낮춰 타격감을 만든다.
	void TriggerHitStop(float Duration);

	// 타격 위치에 Shockwave/Distortion Niagara 효과를 생성한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnShockwave(FVector Location, float Scale = 1.0f);

	// SetAttackData로 저장된 기본 판정값으로 히트 체크를 시작한다.
	void CheckHitStartDefault();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsPerfectWindowActive() const { return bIsPerfectWindowActive; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetPerfectWindowActive(bool bActive, float TimeDilation = -1.f, float Duration = -1.f);

	void SetShowDebugTrace(bool bInShowDebugTrace)
	{
		this->bShowDebugTrace = bInShowDebugTrace;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	EBADamageReactionType GetDamageReactionType() const { return CurrentDamageReactionType; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ProcessHitCheck(); // 이전 프레임과 현재 프레임의 무기 선분 사이를 박스 스윕

	void ApplyDamage(AActor* Victim, const FHitResult& HitResult);

public:
	// 새 피해 대상이 감지되고 피해 적용까지 끝난 뒤 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable)
	FOnHitDetected OnHitDetected;

protected:
	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> ShockwaveSystem;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> DistortionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	bool bShowDebugTrace = false;

	UPROPERTY()
	bool bIsHitChecking = false;

	UPROPERTY()
	bool bIsPerfectWindowActive = false;

	UPROPERTY()
	float ActivePerfectTimeDilation = -1.f;

	UPROPERTY()
	float ActivePerfectDuration = -1.f;

	UPROPERTY()
	float CurrentRadius;

	UPROPERTY()
	float CurrentDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EBADamageReactionType CurrentDamageReactionType = EBADamageReactionType::HitReact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Socket")
	FName StartSocketName = TEXT("Sword_Start");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Socket")
	FName EndSocketName = TEXT("Sword_End");

	UPROPERTY()
	FVector PrevStartLocation;

	UPROPERTY()
	FVector PrevEndLocation;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	FTimerHandle HitStopTimerHandle;
	bool bIsHitStopActive = false;
};
