// Copyright TeamBA. All Rights Reserved.

#include "World/BAFallDamageSuppressionVolume.h"

#include "Player/BAPlayerCharacter.h"

ABAFallDamageSuppressionVolume::ABAFallDamageSuppressionVolume()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABAFallDamageSuppressionVolume::BeginPlay()
{
	Super::BeginPlay();

	OnVolumeBeginOverlap.AddDynamic(this, &ABAFallDamageSuppressionVolume::HandleSuppressionBegin);
	OnVolumeEndOverlap.AddDynamic(this, &ABAFallDamageSuppressionVolume::HandleSuppressionEnd);
}

void ABAFallDamageSuppressionVolume::HandleSuppressionBegin(AActor* OverlappingActor)
{
	ABAPlayerCharacter* PlayerCharacter = Cast<ABAPlayerCharacter>(OverlappingActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->SetFallDamageSuppressed(true, GetSuppressionSource());
}

void ABAFallDamageSuppressionVolume::HandleSuppressionEnd(AActor* OverlappingActor)
{
	ABAPlayerCharacter* PlayerCharacter = Cast<ABAPlayerCharacter>(OverlappingActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->SetFallDamageSuppressed(false, GetSuppressionSource());
}

FName ABAFallDamageSuppressionVolume::GetSuppressionSource() const
{
	return SuppressionSource.IsNone() ? GetFName() : SuppressionSource;
}
