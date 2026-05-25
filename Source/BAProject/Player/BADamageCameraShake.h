#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "BADamageCameraShake.generated.h"

UCLASS()
class BAPROJECT_API UBADamageCameraShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UBADamageCameraShakePattern(const FObjectInitializer& ObjectInitializer);

private:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Camera Shake", meta = (ClampMin = "0.0"))
	float Duration = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Camera Shake")
	float PitchAmplitude = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Camera Shake")
	float YawAmplitude = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Camera Shake")
	float RollAmplitude = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Camera Shake", meta = (ClampMin = "0.0"))
	float Frequency = 34.f;

	float ElapsedTime = 0.f;
};

UCLASS()
class BAPROJECT_API UBADamageCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBADamageCameraShake(const FObjectInitializer& ObjectInitializer);
};
