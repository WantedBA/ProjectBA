#include "Player/Camera/BADamageCameraShake.h"

UBADamageCameraShakePattern::UBADamageCameraShakePattern(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBADamageCameraShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
	OutInfo.BlendIn = 0.f;
	OutInfo.BlendOut = 0.04f;
}

void UBADamageCameraShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& /*Params*/)
{
	ElapsedTime = 0.f;
}

void UBADamageCameraShakePattern::UpdateShakePatternImpl(
	const FCameraShakePatternUpdateParams& Params,
	FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime = FMath::Min(Duration, ElapsedTime + Params.DeltaTime);
	const float NormalizedTime = Duration <= 0.f ? 1.f : FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f);
	const float Strength = FMath::Square(1.f - NormalizedTime);
	const float Wave = ElapsedTime * Frequency * UE_TWO_PI;

	OutResult.Rotation.Pitch = FMath::Sin(Wave) * PitchAmplitude * Strength;
	OutResult.Rotation.Yaw = FMath::Sin(Wave * 0.73f) * YawAmplitude * Strength;
	OutResult.Rotation.Roll = FMath::Sin(Wave * 1.27f) * RollAmplitude * Strength;
}

bool UBADamageCameraShakePattern::IsFinishedImpl() const
{
	return ElapsedTime >= Duration;
}

UBADamageCameraShake::UBADamageCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	UBADamageCameraShakePattern* DamageShakePattern =
		ObjectInitializer.CreateDefaultSubobject<UBADamageCameraShakePattern>(
			this,
			TEXT("DamageShakePattern"));
	SetRootShakePattern(DamageShakePattern);
}
