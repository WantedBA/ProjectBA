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

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent), Blueprintable)
class BAPROJECT_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UInteractorComponent();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
			FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();
	
	UFUNCTION(BlueprintPure, Category="Interaction")
	AActor* GetCurrentInteractable() const {return CurrentInteractable;}

	// C++로 동적 생성된 컴포넌트에 OutlineMaterial을 주입할 때 사용
	UFUNCTION(BlueprintCallable, Category="Interaction|Outline")
	void SetOutlineMaterial(UMaterialInterface* InMaterial) { OutlineMaterial = InMaterial; }

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0"))
	float DetectionRadius = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenWeight = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0"))
	float ScanInterval = 0.1f;

	// Pawn forward 기준 dot 임계값. 0.5 ≒ ±60° cone. -1 = 전방향 허용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|View",
		meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinForwardDot = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|View")
	bool bRequireLineOfSight = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Outline",
				meta = (ClampMin = "0", ClampMax = "255"))
	int32 OutlineStencilValue = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|UI")
	TSubclassOf<UInteractionWidget> WidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bShowDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> DetectionSphere;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentInteractable;
	
	UPROPERTY(Transient)
	TObjectPtr<UInteractionWidget> CurrentWidget;
	
	float TimeSinceLastScan = 0.f;
};
