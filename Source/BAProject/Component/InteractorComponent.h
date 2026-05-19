// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

class UInteractionWidget;
class USphereComponent;
class UMaterialInterface;

#include "InteractorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableChanged, 
	AActor*, NewInteractable, AActor*, OldInteractable);

/**
 * 플레이어 주변의 IInteractable 후보를 선택하고 상호작용 UI/외곽선을 관리하는 컴포넌트.
 *
 * DetectionSphere에 들어온 후보 중 거리, 화면 중앙과의 거리, 캐릭터 전방 cone,
 * 시야 차단 여부를 점수화해 가장 적합한 대상을 CurrentInteractable로 유지한다.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent), Blueprintable)
class BAPROJECT_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UInteractorComponent();
	
	// 감지 Sphere를 동적으로 만들고 외곽선 PostProcess 머티리얼을 카메라에 등록한다.
	virtual void BeginPlay() override;

	// 현재 대상의 외곽선/UI를 정리한다.
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// ScanInterval마다 최적 상호작용 후보와 프롬프트를 갱신한다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
			FActorComponentTickFunction* ThisTickFunction) override;
	
	// 현재 선택된 상호작용 대상의 Interact 인터페이스를 실행한다.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();
	
	// 현재 선택된 상호작용 대상. 없으면 nullptr이다.
	UFUNCTION(BlueprintPure, Category="Interaction")
	AActor* GetCurrentInteractable() const {return CurrentInteractable;}

	// C++로 동적 생성된 컴포넌트에 OutlineMaterial을 주입할 때 사용
	UFUNCTION(BlueprintCallable, Category="Interaction|Outline")
	void SetOutlineMaterial(UMaterialInterface* InMaterial) { OutlineMaterial = InMaterial; }

	// 선택 대상이 바뀔 때 새 대상과 이전 대상을 함께 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;
	
private:
	void UpdateBestInteractable();
	AActor* SelectBest() const;
	void SetCurrentInteractable(AActor* NewInteractable);
	void SetOutline(AActor* Object, bool bEnabled) const;
	void ShowWidgetTarget(AActor* Target);
	void HideWidget();
	void RefreshWidgetPrompt();
	
	FVector GetInteractionLocation(AActor* Object) const;
	APlayerController* GetOwnerController() const;
	
protected:
	// 후보를 탐색할 overlap 반경.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0"))
	float DetectionRadius = 400.f;
	
	// 실제 상호작용을 허용하는 최대 수평 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 200.f;

	// 화면 중앙 거리 점수의 가중치. 1에 가까울수록 조준 중앙을 더 우선한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenWeight = 0.6f;

	// 후보 재평가 주기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0"))
	float ScanInterval = 0.1f;

	// Pawn forward 기준 dot 임계값. 0.5 ≒ ±60° cone. -1 = 전방향 허용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|View",
		meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinForwardDot = 0.5f;

	// true면 Visibility trace로 벽 뒤 대상이 선택되지 않도록 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|View")
	bool bRequireLineOfSight = true;
	
	// 외곽선 머티리얼에서 사용할 CustomDepth stencil 값.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Outline",
				meta = (ClampMin = "0", ClampMax = "255"))
	int32 OutlineStencilValue = 1;
	
	// 현재 대상 프롬프트를 표시할 위젯 클래스.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|UI")
	TSubclassOf<UInteractionWidget> WidgetClass;
	
	// 개발 빌드에서 화면 디버그 프롬프트를 표시할지 여부.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bShowDebug = true;

	// 선택 대상 외곽선을 렌더링하는 PostProcess 머티리얼.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;
	
	// 런타임에 생성되어 주변 후보 overlap을 수집하는 SphereComponent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> DetectionSphere;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentInteractable;
	
	UPROPERTY(Transient)
	TObjectPtr<UInteractionWidget> CurrentWidget;
	
	float TimeSinceLastScan = 0.f;
};
