#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Enemy/EnemyBase.h"
#include "AN_EnemyAttackEnd.generated.h"

UCLASS()
class BAPROJECT_API UAN_EnemyAttackEnd : public UAnimNotify
{
	GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
    EEnemyState TargetEndState = EEnemyState::Idle;
};
