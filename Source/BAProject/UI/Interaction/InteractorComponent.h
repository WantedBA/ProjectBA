#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, 
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
	
private:
	void UpdateBestInteractable();
	UObject* SelectBest() const;
	void SetCurrentInteractable(UObject* NewInteractable);
	
	void ShowWidgetTarget(UObject* Target);
	void HideWidget();
	void RefreshWidgetPrompt();
	
	FVector GetInteractionLocation(UObject* Object) const;
	APlayerController* GetOwnerController() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float DetectionRadius = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 300.f;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UObject> CurrentInteractable;
	
	UPROPERTY(Transient)
	TObjectPtr<UInteractionWidget> CurrentWidget;
	
	float TimeSinceLastScan = 0.f;

};
