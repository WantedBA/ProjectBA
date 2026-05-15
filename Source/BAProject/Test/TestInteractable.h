#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "TestInteractable.generated.h"

UCLASS()
class BAPROJECT_API ATestInteractable : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ATestInteractable();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override { return true;}
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText  GetInteractionPrompt_Implementation() const override;
	virtual FVector GetInteractionLocation_Implementation() const override
	{
		return GetActorLocation();
	}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category="Test")
	FText Prompt = FText::FromString(TEXT("E - 테스트"));
};
