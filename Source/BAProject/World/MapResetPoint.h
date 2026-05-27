#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "MapResetPoint.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UArrowComponent;
class UAnimMontage;
class USkillTreeWidget;
class ULayerBase;
class ABAPlayerCharacter;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BeginRest(ABAPlayerCharacter& Player);
	void BeginRestExit();
	void FinishRestExit();
	void MovePlayerToRestPosition(ABAPlayerCharacter& Player) const;
	void ApplyRestEffects(ABAPlayerCharacter& Player) const;
	void SaveRespawnProgress() const;
	void OpenSkillTreeInResetPoint();
	void CloseSkillTreeInResetPoint();
	void PlayCheckpointFade(bool bFadeIn) const;
	float PlayPlayerMontage(ABAPlayerCharacter& Player, UAnimMontage* Montage) const;

	UFUNCTION()
	void HandleSkillTreeClosed(ULayerBase* ClosedWidget);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InteractionPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> RespawnPoint;

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

	UPROPERTY(EditAnywhere, Category = "Interaction|Animation")
	TObjectPtr<UAnimMontage> RestMontage;

	UPROPERTY(EditAnywhere, Category = "Interaction|Animation")
	TObjectPtr<UAnimMontage> StandUpMontage;

	UPROPERTY(EditAnywhere, Category = "Interaction|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float SkillTreeOpenDelay = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float ExitUnlockDelay = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI|SkillTree")
	TSubclassOf<USkillTreeWidget> SkillTreeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USkillTreeWidget> SkillTreeWidget;

	UPROPERTY(Transient)
	TObjectPtr<ABAPlayerCharacter> RestingPlayer;

	FTimerHandle OpenSkillTreeTimerHandle;
	FTimerHandle FinishRestExitTimerHandle;

	bool bRestTransitionInProgress = false;
};
