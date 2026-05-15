#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable, meta = (DisplayName = "Interactable"))
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class BAPROJECT_API IInteractable
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction" )
	bool CanInteract(AActor* interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const { return true; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor) {}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;
	virtual FText GetInteractionPrompt_Implementation() const
	{
		return NSLOCTEXT("Interaction", "DefaultPrompt", "Interact");
	}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetInteractionLocation() const;
	virtual FVector GetInteractionLocation_Implementation() const { return FVector::ZeroVector; }
};
