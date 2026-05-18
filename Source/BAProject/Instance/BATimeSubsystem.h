#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BATimeSubsystem.generated.h"

/**
 * 전역 시간 팽창(Global Time Dilation)을 중앙 집중식으로 관리하는 서브시스템.
 */
UCLASS()
class BAPROJECT_API UBATimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 특정 비율로 일정 시간 동안 슬로우 모션 적용
	UFUNCTION(BlueprintCallable, Category = "Time")
	void ApplySlowMotion(float TimeDilation, float Duration);

	// 짧은 정지 효과 (역경직용)
	UFUNCTION(BlueprintCallable, Category = "Time")
	void ApplyHitStop(float Duration = 0.05f);

	// 시간 팽창 즉시 복구
	UFUNCTION(BlueprintCallable, Category = "Time")
	void ResetTimeDilation();

private:
	FTimerHandle TimeResetTimerHandle;
};
