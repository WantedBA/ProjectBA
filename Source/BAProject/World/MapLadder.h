// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "MapLadder.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UCLASS()
class BAPROJECT_API AMapLadder : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
    AMapLadder();
    virtual void OnConstruction(const FTransform& Transform) override;

    virtual bool CanInteract_Implementation(AActor* Interactor) const override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual FVector GetInteractionLocation_Implementation() const override;

    UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
    FVector GetBottomEntryLocation() const;

    UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
    FVector GetTopEntryLocation() const;

    UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
    FRotator GetClimbFaceRotation() const;

    UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
    float GetTotalHeight() const { return SegmentCount * SegmentHeight; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> LadderRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInstancedStaticMeshComponent> SegmentISMC;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> BottomEntry;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> TopEntry;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> InteractionPivot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Ladder|Visual")
    TObjectPtr<UStaticMesh> SegmentMeshAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Ladder|Visual",
        meta = (ClampMin = "1"))
    int32 SegmentCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Ladder|Visual",
        meta = (ClampMin = "1.0"))
    float SegmentHeight = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Ladder|UI")
    FText PromptEnter = NSLOCTEXT("Ladder", "Enter", "E - 사다리 타기");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Ladder|UI")
    FText PromptExit = NSLOCTEXT("Ladder", "Exit", "E - 내리기");
};
