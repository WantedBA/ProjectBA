// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

class UInteractionWidget;
class USphereComponent;

#include "InteractorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableChanged, 
	UObject*, NewInteractable, UObject*, OldInteractable);

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
	UObject* GetCurrentInteractable() const {return CurrentInteractable;}
	
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;
	
private:
	void UpdateBestInteractable();
	UObject* SelectBest() const;
	void SetCurrentInteractable(UObject* NewInteractable);
	void SetOutline(UObject* Object, bool bEnabled) const;
	void ShowWidgetTarget(UObject* Target);
	void HideWidget();
	void RefreshWidgetPrompt();
	
	FVector GetInteractionLocation(UObject* Object) const;
	APlayerController* GetOwnerController() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0"))
	float DetectionRadius = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenWeight = 0.6f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", 
		meta = (ClampMin = "0.0"))
	float ScanInterval = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Outline",
				meta = (ClampMin = "0", ClampMax = "255"))
	int32 OutlineStencilValue = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|UI")
	TSubclassOf<UInteractionWidget> WidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bShowDebug = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> DetectionSphere;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UObject> CurrentInteractable;
	
	UPROPERTY(Transient)
	TObjectPtr<UInteractionWidget> CurrentWidget;
	
	float TimeSinceLastScan = 0.f;
};
