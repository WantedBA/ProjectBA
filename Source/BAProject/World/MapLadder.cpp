// Copyright TeamBA. All Rights Reserved.

#include "World/MapLadder.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Player/BAPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AMapLadder::AMapLadder()
{
	PrimaryActorTick.bCanEverTick = false;

	LadderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LadderRoot"));
	SetRootComponent(LadderRoot);

	SegmentISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SegmentISMC"));
	SegmentISMC->SetupAttachment(LadderRoot);

	BottomEntry = CreateDefaultSubobject<USceneComponent>(TEXT("BottomEntry"));
	BottomEntry->SetupAttachment(LadderRoot);

	TopEntry = CreateDefaultSubobject<USceneComponent>(TEXT("TopEntry"));
	TopEntry->SetupAttachment(LadderRoot);

	InteractionPivot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPivot"));
	InteractionPivot->SetupAttachment(LadderRoot);
	InteractionPivot->SetRelativeLocation(FVector(80.f, 0.f, 0.f));
}

void AMapLadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SegmentISMC)
	{
		SegmentISMC->SetStaticMesh(SegmentMeshAsset);
		SegmentISMC->ClearInstances();
		for (int32 i = 0; i < SegmentCount; ++i)
		{
			FTransform T;
			T.SetLocation(FVector(0.f, 0.f, i * SegmentHeight));
			SegmentISMC->AddInstance(T);
		}
	}

	// TopEntry 자동 보정 제거 — 디자이너가 BP에서 직접 배치 (사다리 실제 끝 + 캡슐 절반 고려)
}

bool AMapLadder::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor) return false;
	if (const ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(Interactor))
	{
		// 이미 이 사다리에 매달려있으면 후보 제외 — 외곽선/프롬프트 OFF
		if (Player->IsOnLadder() && Player->GetCurrentLadder() == this)
		{
			return false;
		}
		// 공중(낙하) 중이면 사다리 다시 못 잡게
		if (const UCharacterMovementComponent* Move = Player->GetCharacterMovement())
		{
			if (Move->IsFalling()) return false;
		}
	}
	return true;
}

void AMapLadder::Interact_Implementation(AActor* Interactor)
{
	ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(Interactor);
	if (!Player) return;

	if (Player->IsOnLadder())
	{
		Player->ExitLadder(Player->GetActorLocation());
		return;
	}

	const FVector CharLoc = Player->GetActorLocation();
	const float DistBot = FVector::DistSquared(CharLoc, GetBottomEntryLocation());
	const float DistTop = FVector::DistSquared(CharLoc, GetTopEntryLocation());
	const FVector EntryLoc = (DistBot <= DistTop)
		? GetBottomEntryLocation()
		: GetTopEntryLocation();

	Player->EnterLadder(this, EntryLoc, GetClimbFaceRotation());
}

FText AMapLadder::GetInteractionPrompt_Implementation() const
{
	return PromptEnter;
}

FVector AMapLadder::GetInteractionLocation_Implementation() const
{
	if (!LadderRoot) return GetActorLocation();
	// 사다리 본체 중간 높이를 후보 좌표로 사용 — 캐릭터가 가까이 가도 forward cone 유지
	const float Mid = SegmentCount * SegmentHeight * 0.5f;
	return LadderRoot->GetComponentLocation() + FVector(0.f, 0.f, Mid);
}

FVector AMapLadder::GetBottomEntryLocation() const
{
	return BottomEntry ? BottomEntry->GetComponentLocation() : GetActorLocation();
}

FVector AMapLadder::GetTopEntryLocation() const
{
	return TopEntry ? TopEntry->GetComponentLocation() : GetActorLocation();
}

FRotator AMapLadder::GetClimbFaceRotation() const
{
	// BottomEntry의 forward가 곧 캐릭터 시선 — 디자이너가 BP에서 회전시켜 결정
	if (BottomEntry) return BottomEntry->GetComponentRotation();
	return GetActorRotation();
}