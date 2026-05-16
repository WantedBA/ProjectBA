// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MapElevator.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

// Sets default values
AMapElevator::AMapElevator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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
}

void AMapElevator::BeginPlay()
{
	Super::BeginPlay();

	bAtStart = bStartAtStart;

	if (bAutoElevateOnBeginPlay)
	{
		StartElevateToOther();
	}
}

void AMapElevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsElevating == false)
		return;

	ElevateAlpha = ElevateDuration <= 0.f ?
		1.f : FMath::Clamp(ElevateAlpha + DeltaTime/ ElevateDuration, 0.f, 1.f);
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, ElevateAlpha, 2.f);

	if(ElevatorBody)
		ElevatorBody->SetRelativeLocation(FMath::Lerp(ElevateStartLoc, ElevateEndLoc, Eased));

	if (ElevateAlpha >= 1.f)
	{
		bIsElevating = false;
		bAtStart = !bAtStart;
		SetActorTickEnabled(false);
	}
}

void AMapElevator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PlatformMesh)
	{
		PlatformMesh->SetStaticMesh(PlatformMeshAsset);
	}

	// 시작 위치
	if (ElevatorBody && StartPoint && EndPoint)
	{
		const FVector InitLoc = bStartAtStart
			? StartPoint->GetRelativeLocation()
			: EndPoint->GetRelativeLocation();
		ElevatorBody->SetRelativeLocation(InitLoc);
	}
}

void AMapElevator::StartElevateToOther()
{
	if (bIsElevating)
		return;
	if (ElevatorBody == nullptr || StartPoint == nullptr || EndPoint == nullptr)
		return;

	ElevateStartLoc = ElevatorBody->GetRelativeLocation();
	ElevateEndLoc = bAtStart ? 
		EndPoint->GetRelativeLocation() : StartPoint->GetRelativeLocation();
	ElevateAlpha = 0.f;
	bIsElevating = true;
	SetActorTickEnabled(true);
}
