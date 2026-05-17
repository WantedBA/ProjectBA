#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_GhostTrail.generated.h"

/**
 * 캐릭터의 잔상을 일정 간격으로 생성하는 노티파이
 */
UCLASS()
class BAPROJECT_API UANS_GhostTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_GhostTrail();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	void SpawnGhost(USkeletalMeshComponent* SourceMesh);

protected:
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float SpawnInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Ghost")
	float GhostLifeTime = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Ghost")
	TObjectPtr<UMaterialInterface> GhostMaterial;


private:
	float LastSpawnTime = 0.0f;
};
