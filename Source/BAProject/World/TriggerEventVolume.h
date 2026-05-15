// Copyright TeamBA. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriggerEventVolume.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeBeginOverlap, AActor*, OverlappingActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeEndOverlap, AActor*, OverlappingActor);

UCLASS()
class BAPROJECT_API ATriggerEventVolume : public AActor
{
    GENERATED_BODY()

public:
    ATriggerEventVolume();

    UPROPERTY(BlueprintAssignable, Category = "Trigger")
    FOnVolumeBeginOverlap OnVolumeBeginOverlap;

    UPROPERTY(BlueprintAssignable, Category = "Trigger")
    FOnVolumeEndOverlap OnVolumeEndOverlap;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    bool IsTargetActor(const AActor* OtherActor) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerVolume;

    // 이 클래스(또는 하위 클래스)인 액터만 이벤트를 Broadcast. 비우면 전부 허용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
    TSubclassOf<AActor> TargetFilterClass;

    // 이 볼륨이 작동시킬 대상 액터들
    // BP 그래프에서 OnVolumeBeginOverlap 처리 시 읽어서 사용
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Trigger")
    TArray<TObjectPtr<AActor>> TargetActors;
};
