#include "Player/BALandingCameraShake.h"

UBALandingRecoveryCameraShakePattern::UBALandingRecoveryCameraShakePattern(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBALandingRecoveryCameraShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
	OutInfo.BlendIn = 0.f;
	OutInfo.BlendOut = 0.06f;
}

void UBALandingRecoveryCameraShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& /*Params*/)
{
	ElapsedTime = 0.f;
}

void UBALandingRecoveryCameraShakePattern::UpdateShakePatternImpl(
	const FCameraShakePatternUpdateParams& Params,
	FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime = FMath::Min(Duration, ElapsedTime + Params.DeltaTime);
	const float NormalizedTime = Duration <= 0.f ? 1.f : FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f);
	const float Falloff = FMath::Square(1.f - NormalizedTime);
	const float Dip = FMath::Sin(NormalizedTime * UE_PI);

	OutResult.Location.Z = -Dip * VerticalAmplitude * Falloff;
	OutResult.Rotation.Pitch = Dip * PitchAmplitude * Falloff;
}

bool UBALandingRecoveryCameraShakePattern::IsFinishedImpl() const
{
	return ElapsedTime >= Duration;
}

UBALandingRecoveryCameraShake::UBALandingRecoveryCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	UBALandingRecoveryCameraShakePattern* LandingShakePattern =
		ObjectInitializer.CreateDefaultSubobject<UBALandingRecoveryCameraShakePattern>(
			this,
			TEXT("LandingRecoveryShakePattern"));
	SetRootShakePattern(LandingShakePattern);
}
