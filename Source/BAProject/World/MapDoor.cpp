// Copyright TeamBA. All Rights Reserved.

#include "MapDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

AMapDoor::AMapDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
	SetRootComponent(DoorRoot);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(DoorRoot);

	HingePivotA = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivotA"));
	HingePivotA->SetupAttachment(DoorRoot);

	LeafMeshA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeafMeshA"));
	LeafMeshA->SetupAttachment(HingePivotA);

	HingePivotB = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivotB"));
	HingePivotB->SetupAttachment(DoorRoot);

	LeafMeshB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeafMeshB"));
	LeafMeshB->SetupAttachment(HingePivotB);

	InteractionPivot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPivot"));
	InteractionPivot->SetupAttachment(DoorRoot);
}

void AMapDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FrameMesh)
	{
		FrameMesh->SetStaticMesh(FrameMeshAsset);
	}
	if (HingePivotA)
	{
		HingePivotA->SetRelativeLocation(HingeOffsetA);
	}
	if (LeafMeshA)
	{
		LeafMeshA->SetStaticMesh(LeafMeshAssetA);
		LeafMeshA->SetRelativeLocation(LeafOffsetA);
	}
	if (HingePivotB)
	{
		HingePivotB->SetRelativeLocation(HingeOffsetB);
		HingePivotB->SetVisibility(bDoubleLeaf, true);
	}
	if (LeafMeshB)
	{
		LeafMeshB->SetStaticMesh(bDoubleLeaf ? LeafMeshAssetB : nullptr);
		LeafMeshB->SetRelativeLocation(LeafOffsetB);
		LeafMeshB->SetVisibility(bDoubleLeaf, true);
		LeafMeshB->SetCollisionEnabled(bDoubleLeaf
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision);
	}
	if (InteractionPivot)
	{
		InteractionPivot->SetRelativeLocation(InteractionOffset);
	}
}

void AMapDoor::BeginPlay()
{
	Super::BeginPlay();

	HingeAClosedRelative = HingePivotA
		? HingePivotA->GetRelativeTransform()
		: FTransform::Identity;
	HingeBClosedRelative = HingePivotB
		? HingePivotB->GetRelativeTransform()
		: FTransform::Identity;

	if (bStartOpen)
	{
		CurrentAlpha = 1.f;
		ApplyAlpha(1.f);
		SetDoorState(EDoorState::Open);
	}
	else
	{
		CurrentAlpha = 0.f;
		ApplyAlpha(0.f);
		SetDoorState(EDoorState::Closed);
	}
}

void AMapDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MotionDir == 0) return;

	const float Step = (OpenDuration > KINDA_SMALL_NUMBER)
		? (DeltaTime / OpenDuration)
		: 1.f;
	CurrentAlpha = FMath::Clamp(
		CurrentAlpha + Step * static_cast<float>(MotionDir),
		0.f, 1.f);

	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, CurrentAlpha, 2.f);
	ApplyAlpha(Eased);

	if (MotionDir > 0 && CurrentAlpha >= 1.f)
	{
		MotionDir = 0;
		SetActorTickEnabled(false);
		SetDoorState(EDoorState::Open);
		OnDoorOpened.Broadcast();
	}
	else if (MotionDir < 0 && CurrentAlpha <= 0.f)
	{
		MotionDir = 0;
		SetActorTickEnabled(false);
		SetDoorState(EDoorState::Closed);
		OnDoorClosed.Broadcast();
	}
}

bool AMapDoor::CanInteract_Implementation(AActor* /*Interactor*/) const
{
	if (bOnce && bUsed) return false;
	if (DoorState == EDoorState::Opening || DoorState == EDoorState::Closing)
	{
		return false;
	}
	return true;
}

void AMapDoor::Interact_Implementation(AActor* /*Interactor*/)
{
	Toggle();
}

FText AMapDoor::GetInteractionPrompt_Implementation() const
{
	switch (DoorState)
	{
	case EDoorState::Closed: return PromptOpen;
	case EDoorState::Open:   return PromptClose;
	default:                 return FText::GetEmpty();
	}
}

FVector AMapDoor::GetInteractionLocation_Implementation() const
{
	return InteractionPivot
		? InteractionPivot->GetComponentLocation()
		: GetActorLocation();
}

void AMapDoor::Open()
{
	if (bOnce && bUsed) return;
	if (DoorState == EDoorState::Open || DoorState == EDoorState::Opening) return;

	MotionDir = 1;
	SetActorTickEnabled(true);
	SetDoorState(EDoorState::Opening);
	bUsed = true;
}

void AMapDoor::Close()
{
	if (DoorState == EDoorState::Closed || DoorState == EDoorState::Closing) return;

	MotionDir = -1;
	SetActorTickEnabled(true);
	SetDoorState(EDoorState::Closing);
}

void AMapDoor::Toggle()
{
	if (DoorState == EDoorState::Closed) Open();
	else if (DoorState == EDoorState::Open) Close();
}

void AMapDoor::SetDoorState(EDoorState NewState)
{
	if (NewState == DoorState) return;
	const EDoorState Old = DoorState;
	DoorState = NewState;
	OnDoorStateChanged.Broadcast(Old, NewState);
}

void AMapDoor::ApplyAlpha(float Alpha)
{
	auto ApplyOne = [&](USceneComponent* Pivot,
	                    const FTransform& ClosedRel,
	                    bool bMirror)
	{
		if (!Pivot) return;
		FTransform Target = ClosedRel;
		if (Motion == EDoorMotion::Swing)
		{
			FRotator Rot = OpenRotation;
			if (bMirror) { Rot.Yaw = -Rot.Yaw; }
			Target.SetRotation(ClosedRel.GetRotation() * FQuat(Rot * Alpha));
		}
		else
		{
			FVector Off = OpenOffset;
			if (bMirror) { Off.Y = -Off.Y; }
			Target.SetLocation(ClosedRel.GetLocation() + Off * Alpha);
		}
		Pivot->SetRelativeTransform(Target);
	};

	ApplyOne(HingePivotA, HingeAClosedRelative, false);
	if (bDoubleLeaf)
	{
		ApplyOne(HingePivotB, HingeBClosedRelative, true);
	}
}
