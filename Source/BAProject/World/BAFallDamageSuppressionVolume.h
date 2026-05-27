// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/TriggerEventVolume.h"
#include "BAFallDamageSuppressionVolume.generated.h"

UCLASS()
class BAPROJECT_API ABAFallDamageSuppressionVolume : public ATriggerEventVolume
{
	GENERATED_BODY()

public:
	ABAFallDamageSuppressionVolume();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleSuppressionBegin(AActor* OverlappingActor);

	UFUNCTION()
	void HandleSuppressionEnd(AActor* OverlappingActor);

private:
	FName GetSuppressionSource() const;

	// 비우면 볼륨 이름
	UPROPERTY(EditInstanceOnly, Category = "Movement|Falling")
	FName SuppressionSource;
};
