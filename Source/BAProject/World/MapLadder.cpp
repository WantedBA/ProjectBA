// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MapLadder.h"

// Sets default values
AMapLadder::AMapLadder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMapLadder::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMapLadder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

