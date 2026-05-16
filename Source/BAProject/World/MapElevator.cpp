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

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(ElevatorActorRoot);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
}

void AMapElevator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PlatformMesh)
	{
		PlatformMesh->SetStaticMesh(PlatformMeshAsset);
	}
}