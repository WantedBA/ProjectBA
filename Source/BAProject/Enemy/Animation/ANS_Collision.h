#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Collision.generated.h"

UCLASS()
class BAPROJECT_API UANS_Collision : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	FName StartSocket = TEXT("Start_SocketName");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	FName EndSocket = TEXT("End_SocketName");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float HitRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bUseDefaultData = true;
};
