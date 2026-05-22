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
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "AreaAttack")
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, Category = "AreaAttack")
	EBADamageReactionType DamageReactionType = EBADamageReactionType::LargeHitReact;

	// 스폰할 VFX 목록. 복수 지정 가능하며 모두 동시에 재생된다.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX")
	TArray<TObjectPtr<UNiagaraSystem>> AreaVFXList;

	// VFX 에셋이 설계된 기준 반경. Radius / VFXBaseRadius 비율로 자동 스케일된다.
	UPROPERTY(EditAnywhere, Category = "AreaAttack|VFX")
	float VFXBaseRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "AreaAttack|Debug")
	bool bShowDebugRadius = false;

private:
	// NotifyBegin에서 스폰된 컴포넌트를 NotifyEnd까지 추적한다.
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ActiveVFXList;
};
