// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapLadder.generated.h"

UCLASS()
class BAPROJECT_API AMapLadder : public AActor
{
	GENERATED_BODY()

public:
	AMapLadder();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
