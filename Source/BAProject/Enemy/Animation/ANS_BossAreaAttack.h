#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/BADamageTypes.h"
#include "ANS_BossAreaAttack.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BAPROJECT_API UANS_BossAreaAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "AreaAttack")
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, Category = "AreaAttack")
	EBADamageReactionType DamageReactionType = EBADamageReactionType::LargeHitReact;

	// BP에 배치된 SceneComponent의 태그명. 설정하면 해당 컴포넌트 위치를 Origin으로 사용, 비워두면 Actor 위치 사용
	UPROPERTY(EditAnywhere, Category = "AreaAttack")
	FName OriginComponentTag = NAME_None;

	// 스폰할 VFX 목록. 복수 지정 가능하며 모두 동시에 재생된다.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX")
	TArray<TObjectPtr<UNiagaraSystem>> AreaVFXList;

	// true: Radius / VFXBaseRadius 비율로 XY 스케일 자동 계산. false: VFXCustomScale 직접 지정.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX")
	bool bAutoScaleVFX = true;

	// bAutoScaleVFX = true 일 때 사용. VFX 에셋이 설계된 기준 반경.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX", meta = (EditCondition = "bAutoScaleVFX", EditConditionHides))
	float VFXBaseRadius = 100.0f;

	// bAutoScaleVFX = false 일 때 사용. VFX XY 스케일을 직접 지정.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX", meta = (EditCondition = "!bAutoScaleVFX", EditConditionHides))
	float VFXCustomScale = 1.0f;

	// Actor 로컬 스페이스 오프셋 (X=정면, Z=위). 데미지 판정 Origin과 별개로 VFX 위치만 이동.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX")
	FVector VFXSpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "AreaAttack|Debug")
	bool bShowDebugRadius = true;

private:
	FVector GetOrigin(AActor* Owner) const;

	// NotifyBegin에서 스폰된 컴포넌트를 NotifyEnd까지 추적한다.
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ActiveVFXList;
};
