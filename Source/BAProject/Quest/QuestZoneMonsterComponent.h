// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuestZoneMonsterComponent.generated.h"

UCLASS(ClassGroup=(Quest), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UQuestZoneMonsterComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UQuestZoneMonsterComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Quest")
	int32 QuestTid = 0;
};
