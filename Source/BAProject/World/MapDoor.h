// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interactable.h"
#include "MapDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EDoorMotion : uint8
{
	Swing UMETA(DisplayName = "Swing"),
	Slide UMETA(DisplayName = "Slide")
};

UENUM(BlueprintType)
enum class EDoorState : uint8
{
	Closed  UMETA(DisplayName = "Closed"),
	Opening UMETA(DisplayName = "Opening"),
	Open    UMETA(DisplayName = "Open"),
	Closing UMETA(DisplayName = "Closing")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoorStateChanged,
	EDoorState, OldState, EDoorState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorClosed);

UCLASS()
class BAPROJECT_API AMapDoor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AMapDoor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual FVector GetInteractionLocation_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Toggle();

	UFUNCTION(BlueprintPure, Category = "Door")
	EDoorState GetDoorState() const { return DoorState; }

public:
	UPROPERTY(BlueprintAssignable, Category = "Door|Events")
	FOnDoorStateChanged OnDoorStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Door|Events")
	FOnDoorOpened OnDoorOpened;

	UPROPERTY(BlueprintAssignable, Category = "Door|Events")
	FOnDoorClosed OnDoorClosed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DoorRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> HingePivotA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeafMeshA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> HingePivotB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeafMeshB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InteractionPivot;

	// Type
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Type")
	bool bDoubleLeaf = false;

	// Visual — meshes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual")
	TObjectPtr<UStaticMesh> FrameMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual")
	TObjectPtr<UStaticMesh> LeafMeshAssetA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual",
		meta = (EditCondition = "bDoubleLeaf", EditConditionHides))
	TObjectPtr<UStaticMesh> LeafMeshAssetB;

	// Visual — pivot offsets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual")
	FVector HingeOffsetA = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual")
	FVector LeafOffsetA = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual",
		meta = (EditCondition = "bDoubleLeaf", EditConditionHides))
	FVector HingeOffsetB = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual",
		meta = (EditCondition = "bDoubleLeaf", EditConditionHides))
	FVector LeafOffsetB = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Visual")
	FVector InteractionOffset = FVector(80.f, 0.f, 0.f);

	// Motion
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Motion")
	EDoorMotion Motion = EDoorMotion::Swing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Motion",
		meta = (EditCondition = "Motion == EDoorMotion::Swing", EditConditionHides))
	FRotator OpenRotation = FRotator(0.f, 90.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Motion",
		meta = (EditCondition = "Motion == EDoorMotion::Slide", EditConditionHides))
	FVector OpenOffset = FVector(0.f, 200.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Motion",
		meta = (ClampMin = "0.05"))
	float OpenDuration = 1.0f;

	// State
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|State")
	bool bStartOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|State")
	bool bOnce = false;

	// UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|UI")
	FText PromptOpen = NSLOCTEXT("Door", "Open", "E - 열기");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|UI")
	FText PromptClose = NSLOCTEXT("Door", "Close", "E - 닫기");

private:
	void SetDoorState(EDoorState NewState);
	void ApplyAlpha(float Alpha);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Door",
		meta = (AllowPrivateAccess = "true"))
	EDoorState DoorState = EDoorState::Closed;

	FTransform HingeAClosedRelative;
	FTransform HingeBClosedRelative;

	float CurrentAlpha = 0.f;
	int8 MotionDir = 0;
	bool bUsed = false;
};
