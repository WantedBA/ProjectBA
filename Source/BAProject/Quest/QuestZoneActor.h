

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestZoneActor.generated.h"

class AMonster;
class UBillboardComponent;
class UBoxComponent;

UCLASS()
class BAPROJECT_API AQuestZoneActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AQuestZoneActor();

	void ReArm();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void TriggerStart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTriggerBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, 
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, 
		bool bFromSweep,
		const FHitResult& SweepResult);

	void GatherSpawnTransforms();

private:
	UPROPERTY(VisibleAnywhere, Category = "Quest")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnywhere, Category = "Quest")
	int32 QuestTid = 0;

	// true: BoxVolume 진입하면 자동 발동 / false: TriggerStart() 외부 호출시 발동
	UPROPERTY(EditAnywhere, Category = "Quest")
	bool bUseTriggerVolume = true;

	UPROPERTY(EditAnywhere, Category = "Quest",
		meta = (EditCondition = "bUseTriggerVolume", EditConditionHides))
	TSubclassOf<AMonster> MonsterClass;

	UPROPERTY(EditInstanceOnly, Category = "Quest",
		meta = (MustImplement = "/Script/BAProject.QuestActivatable"))
	TArray<TObjectPtr<AActor>> LinkedActivatables;

	UPROPERTY(Transient)
	TArray<FTransform> CachedSpawnTransforms;

	bool bArmed = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif
};
