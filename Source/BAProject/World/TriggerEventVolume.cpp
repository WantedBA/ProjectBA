// Copyright TeamBA. All Rights Reserved.

#include "World/TriggerEventVolume.h"
#include "Components/BoxComponent.h"
#include "Player/BAPlayerCharacter.h"

// Sets default values
ATriggerEventVolume::ATriggerEventVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);

	TargetFilterClass = ABAPlayerCharacter::StaticClass();
}

// Called when the game starts or when spawned
void ATriggerEventVolume::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATriggerEventVolume::HandleBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ATriggerEventVolume::HandleEndOverlap);
}

void ATriggerEventVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsTargetActor(OtherActor))
	{
		return;
	}

	OnVolumeBeginOverlap.Broadcast(OtherActor);
}

void ATriggerEventVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsTargetActor(OtherActor))
	{
		return;
	}

	OnVolumeEndOverlap.Broadcast(OtherActor);
}

bool ATriggerEventVolume::IsTargetActor(const AActor* otherActor) const
{
	if (IsValid(otherActor) == false)
	{
		return false;
	}

	if(TargetFilterClass == nullptr)
	{
		return true;
	}

	return otherActor->IsA(TargetFilterClass);
}
