#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "MapResetPoint.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkillTreeWidget;

UCLASS()
class BAPROJECT_API AMapResetPoint : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	AMapResetPoint();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual FVector GetInteractionLocation_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	void ToggleSkillTreeInResetPoint();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InteractionPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

private:
	bool bIsPlayerResting = false; // 플레이어가 휴식 중인지 여부
	bool bConditionUnlocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FText PromptEnter = NSLOCTEXT("Interaction", "RestEnter", "E - 휴식하기");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FText PromptExit = NSLOCTEXT("Interaction", "RestExit", "E - 일어나기");

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI|SkillTree")
	TSubclassOf<USkillTreeWidget> SkillTreeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USkillTreeWidget> SkillTreeWidget;
};
