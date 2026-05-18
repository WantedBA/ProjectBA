// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestTriggerActor.generated.h"

class AMonster;
class UBillboardComponent;
class UBoxComponent;

UCLASS()
class BAPROJECT_API AQuestTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	AQuestTriggerActor();
	
	void ReArm();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTriggerBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void GatherSpawnTransforms();

	UPROPERTY(VisibleAnywhere, Category = "Quest")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnywhere, Category = "Quest")
	int32 QuestTid = 0;

	UPROPERTY(EditAnywhere, Category = "Quest")
	TSubclassOf<AMonster> MonsterClass;

	UPROPERTY(Transient)
	TArray<FTransform> CachedSpawnTransforms;

	bool bArmed = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif
};
