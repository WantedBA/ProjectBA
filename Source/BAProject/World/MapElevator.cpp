// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MapElevator.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/BAPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AMapElevator::AMapElevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ElevatorActorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ElevatorActorRoot"));
	SetRootComponent(ElevatorActorRoot);

	StartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(ElevatorActorRoot);

	EndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EndPoint"));
	EndPoint->SetupAttachment(ElevatorActorRoot);

	ElevatorBody = CreateDefaultSubobject<USceneComponent>(TEXT("ElevatorBody"));
	ElevatorBody->SetupAttachment(ElevatorActorRoot);
	ElevatorBody->SetMobility(EComponentMobility::Movable);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(ElevatorBody);
	PlatformMesh->SetMobility(EComponentMobility::Movable);

	EnterTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EnterTrigger"));
	EnterTrigger->SetupAttachment(ElevatorBody);  // 본체와 같이 이동
	EnterTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EnterTrigger->SetGenerateOverlapEvents(true);

	CenterPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CenterPoint"));
	CenterPoint->SetupAttachment(ElevatorBody);

	StartDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("StartDoorPivot"));
	StartDoorPivot->SetupAttachment(ElevatorBody);

	StartDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartDoorMesh"));
	StartDoorMesh->SetupAttachment(StartDoorPivot);
	StartDoorMesh->SetMobility(EComponentMobility::Movable);

	EndDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("EndDoorPivot"));
	EndDoorPivot->SetupAttachment(ElevatorBody);

	EndDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndDoorMesh"));
	EndDoorMesh->SetupAttachment(EndDoorPivot);
	EndDoorMesh->SetMobility(EComponentMobility::Movable);
}

void AMapElevator::BeginPlay()
{
	Super::BeginPlay();

	bAtStart = bStartAtStart;
	State = EElevatorState::Idle;

	if (EnterTrigger)
	{
		EnterTrigger->OnComponentBeginOverlap.AddDynamic(
			this, &AMapElevator::OnEnterTriggerBeginOverlap);
	}

	if (bAutoElevateOnBeginPlay)
	{
		StartElevateToOther();
	}
}

void AMapElevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (State)
	{
	case EElevatorState::Aligning: TickAlign(DeltaTime); break;
	case EElevatorState::ClosingDoor: TickDoor(DeltaTime, true);  break;
	case EElevatorState::Moving:   TickMoving(DeltaTime); break;
	case EElevatorState::OpeningDoor: TickDoor(DeltaTime, false); break;
	default: break;
	}
}

void AMapElevator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PlatformMesh)
	{
		PlatformMesh->SetStaticMesh(PlatformMeshAsset);
	}

	if (StartDoorMesh) 
		StartDoorMesh->SetStaticMesh(DoorMeshAsset);
	if (EndDoorMesh)  
		EndDoorMesh->SetStaticMesh(DoorMeshAsset);

	if (ElevatorBody && StartPoint && EndPoint)
	{
		const FVector InitLoc = bStartAtStart
			? StartPoint->GetRelativeLocation()
			: EndPoint->GetRelativeLocation();
		ElevatorBody->SetRelativeLocation(InitLoc);
	}

	if (StartDoorPivot && EndDoorPivot)
	{
		// 시작 위치 쪽 문은 열림(=OpenRotation), 반대쪽은 닫힘(=0)
		const FRotator OpenRot = DoorOpenRotation;
		const FRotator ClosedRot = FRotator::ZeroRotator;
		StartDoorPivot->SetRelativeRotation(bStartAtStart ? OpenRot : ClosedRot);
		EndDoorPivot->SetRelativeRotation(bStartAtStart ? ClosedRot : OpenRot);
	}
}

void AMapElevator::StartElevateToOther()
{
	if (!ElevatorBody || !StartPoint || !EndPoint) return;

	ElevateStartLoc = ElevatorBody->GetRelativeLocation();
	ElevateEndLoc = bAtStart
		? EndPoint->GetRelativeLocation()
		: StartPoint->GetRelativeLocation();
	Alpha = 0.f;
	SetState(EElevatorState::Moving);
}

void AMapElevator::OnEnterTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (State != EElevatorState::Idle) return;

	ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(OtherActor);
	if (!Player) return;

	StartAlign(Player);
}

void AMapElevator::StartAlign(ABAPlayerCharacter* Player)
{
	if (!Player || !CenterPoint) return;

	RidingPlayer = Player;
	AlignEndLoc = CenterPoint->GetComponentLocation();
	AlignEndLoc.Z = Player->GetActorLocation().Z;  // 높이는 유지

	// 입력만 차단 (캐릭터 무브먼트는 Walking 유지)
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->DisableInput(PC);
	}
	// 걷기 속도로 자동 이동
	Player->SetMovementState(EMovementState::Walk);

	SetState(EElevatorState::Aligning);
}

void AMapElevator::TickAlign(float DeltaTime)
{
	ABAPlayerCharacter* Player = RidingPlayer.Get();
	if (!Player) return;

	FVector ToTarget = AlignEndLoc - Player->GetActorLocation();
	ToTarget.Z = 0.f;
	const float Dist = ToTarget.Size();

	constexpr float ArrivalThreshold = 20.f;  // cm

	if (Dist <= ArrivalThreshold)
	{
		// 도착 — 정지 후 문 닫힘 단계로
		if (UCharacterMovementComponent* Move = Player->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		Alpha = 0.f;
		SetState(EElevatorState::ClosingDoor);
		return;
	}

	// 캐릭터를 목적지 방향으로 걷게
	const FVector Dir = ToTarget / Dist;
	Player->AddMovementInput(Dir, 1.0f);
}

void AMapElevator::TickMoving(float DeltaTime)
{
	Alpha = (ElevateDuration <= 0.f) ? 1.f
		: FMath::Clamp(Alpha + DeltaTime / ElevateDuration, 0.f, 1.f);
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	if (ElevatorBody)
	{
		ElevatorBody->SetRelativeLocation(FMath::Lerp(ElevateStartLoc, ElevateEndLoc, Eased));
	}

	if (Alpha >= 1.f)
	{
		Alpha = 0.f;
		bAtStart = !bAtStart;  // 도착
		SetState(EElevatorState::OpeningDoor);
	}
}

void AMapElevator::SetState(EElevatorState NewState)
{
	State = NewState;
	const bool bNeedTick =
		(NewState == EElevatorState::Aligning) ||
		(NewState == EElevatorState::ClosingDoor) ||
		(NewState == EElevatorState::Moving) ||
		(NewState == EElevatorState::OpeningDoor);
	SetActorTickEnabled(bNeedTick);
}

void AMapElevator::TickDoor(float DeltaTime, bool bClosing)
{
	Alpha = (DoorDuration <= 0.f) ? 1.f
		: FMath::Clamp(Alpha + DeltaTime / DoorDuration, 0.f, 1.f);
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	USceneComponent* DoorPivot = bAtStart ? StartDoorPivot : EndDoorPivot;

	if (DoorPivot)
	{
		const float TargetAlpha = bClosing ? (1.f - Eased) : Eased;
		DoorPivot->SetRelativeRotation(DoorOpenRotation * TargetAlpha);
	}

	if (Alpha >= 1.f)
	{
		Alpha = 0.f;
		if (bClosing)
		{
			// 문 닫힘 끝 -> 이동 시작
			if (ABAPlayerCharacter* Player = RidingPlayer.Get())
			{
				Player->UnlockMovementForCutscene();
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
				{
					PC->EnableInput(PC);
				}
			}
			StartElevateToOther();  // State = Moving
		}
		else
		{
			// 문 열림 끝 → Idle
			RidingPlayer.Reset();
			SetState(EElevatorState::Idle);
		}
	}
}
