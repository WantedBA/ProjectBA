#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "BALandingCameraShake.generated.h"

UCLASS()
class BAPROJECT_API UBALandingRecoveryCameraShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UBALandingRecoveryCameraShakePattern(const FObjectInitializer& ObjectInitializer);

private:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Landing Camera Shake", meta = (ClampMin = "0.0"))
	float Duration = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "Landing Camera Shake")
	float VerticalAmplitude = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Landing Camera Shake")
	float PitchAmplitude = 0.45f;

	float ElapsedTime = 0.f;
};

UCLASS()
class BAPROJECT_API UBALandingRecoveryCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBALandingRecoveryCameraShake(const FObjectInitializer& ObjectInitializer);
};
