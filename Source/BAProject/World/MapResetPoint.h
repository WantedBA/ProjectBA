#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "MapResetPoint.generated.h"

class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestOpenSkillTree);

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

	UPROPERTY(BlueprintAssignable, Category = "Events") //블루프린트 UI 단에서 바인딩하여 스킬트리를 열 때 사용
	FOnRequestOpenSkillTree OnRequestOpenSkillTree;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsPlayerResting = false; // 플레이어가 휴식 중인지 여부
	bool bConditionUnlocked = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> InteractionPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FText PromptEnter = NSLOCTEXT("Interaction", "RestEnter", "E - 휴식하기");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FText PromptExit = NSLOCTEXT("Interaction", "RestExit", "E - 일어나기");
};
