#include "Instance/BATimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UBATimeSubsystem::ApplySlowMotion(float TimeDilation, float Duration)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 기존 타이머가 있다면 취소
	World->GetTimerManager().ClearTimer(TimeResetTimerHandle);

	UGameplayStatics::SetGlobalTimeDilation(World, TimeDilation);

	if (Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			TimeResetTimerHandle,
			this,
			&UBATimeSubsystem::ResetTimeDilation,
			Duration,
			false
		);
	}
}

void UBATimeSubsystem::ApplyHitStop(float Duration, float TimeDilation)
{
	// 역경직은 아주 강한 슬로우를 짧게 적용
	ApplySlowMotion(TimeDilation, Duration);
}

void UBATimeSubsystem::ResetTimeDilation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	World->GetTimerManager().ClearTimer(TimeResetTimerHandle);
}
