
// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/InvisibleWallBase.h"
#include "Quest/QuestActivatable.h"
#include "InvisibleWallTrigger.generated.h"

class ATriggerEventVolume;
class AInvisibleWallTrigger;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallClosed, AInvisibleWallTrigger*, ClosedWall);

UCLASS()
class BAPROJECT_API AInvisibleWallTrigger : public AInvisibleWallBase, public IQuestActivatable
{
    GENERATED_BODY()

public:
    AInvisibleWallTrigger();

    // 외부 시스템이 호출 — 벽을 열고 비주얼을 끔. bConsumed는 유지 → 재진입해도 다시 안 닫힘
    UFUNCTION(BlueprintCallable, Category = "InvisibleWall")
    void OpenWall();

    // 상태 초기화 — 벽을 열고 bConsumed를 false로 리셋하여 다시 작동 가능하게 함
    UFUNCTION(BlueprintCallable, Category = "InvisibleWall")
    void ReArm();

    virtual void OnQuestActivated_Implementation(int32 QuestTid) override;
    virtual void OnQuestDeactivated_Implementation(int32 QuestTid) override;

    // 벽이 닫힐 때 브로드캐스트 — 외부(전투/룸 매니저 등)가 구독
    UPROPERTY(BlueprintAssignable, Category = "InvisibleWall")
    FOnWallClosed OnWallClosed;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleVolumeOverlap(AActor* OverlappingActor);

protected:
    // 이 벽을 닫는 트리거 볼륨 (레벨에서 인스턴스별로 연결)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "InvisibleWall")
    TObjectPtr<ATriggerEventVolume> LinkedVolume;

private:
    // 한 번 닫히면 true → 이후 트리거 재진입 무시 (one-shot)
    bool bConsumed = false;
};
