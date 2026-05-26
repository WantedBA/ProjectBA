#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraOcclusionFadeComponent.generated.h"

class UCameraComponent;
class UMeshComponent;
class USceneComponent;

struct FCameraOcclusionFadeState
{
	float CurrentOpacity = 1.f;
	float TargetOpacity = 1.f;
};

/**
 * 카메라와 플레이어 사이를 가리는 렌더 메쉬를 투명하게 만든다.
 * 충돌 trace 대신 opt-in 태그와 bounds를 사용하고, 태그가 붙은 메쉬는 Camera 채널을 막지 않게 보정한다.
 */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UCameraOcclusionFadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraOcclusionFadeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	bool bEnableOcclusionFade = true;

	// Actor 또는 PrimitiveComponent에 이 태그가 있어야 페이드 후보로 잡힌다.
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	FName FadeTargetTag = TEXT("CameraFade");

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OccludedOpacity = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestoredOpacity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade", meta = (ClampMin = "0.0", Units = "cm"))
	float ProbeRadius = 45.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade", meta = (ClampMin = "0.0", Units = "s"))
	float RefreshInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade", meta = (ClampMin = "0.0"))
	float FadeInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	FName OpacityParameterName = TEXT("CameraFadeOpacity");

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	FName FallbackOpacityParameterName = TEXT("Opacity");

	// CameraFade 대상은 스프링암을 당기지 않아야 하므로 Camera 채널 Block을 런타임에 Ignore로 바꾼다.
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	bool bDisableCameraCollisionOnFadeTargets = true;

	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion Fade")
	TEnumAsByte<ECollisionChannel> CameraBlockingChannel = ECC_Camera;

private:
	void ResolveDefaultComponents();
	void RefreshObstructingComponents();
	void UpdateFadeStates(float DeltaTime);
	void RestoreAllFadeTargets();
	bool ShouldConsiderComponent(const UMeshComponent* Component, const AActor* OwnerActor) const;
	void PrepareFadeTargetComponent(UMeshComponent& Component) const;
	bool IsComponentObstructingCamera(
		const UMeshComponent& Component,
		const FVector& FocusLocation,
		const FVector& CameraLocation) const;
	void SetTargetOpacity(UMeshComponent* Component, float TargetOpacity);
	void ApplyOpacity(UMeshComponent& Component, float Opacity) const;

	TWeakObjectPtr<UCameraComponent> CachedCamera;
	TWeakObjectPtr<USceneComponent> CachedFocusComponent;
	TMap<TWeakObjectPtr<UMeshComponent>, FCameraOcclusionFadeState> FadeStates;
	float TimeSinceLastRefresh = 0.f;
};
