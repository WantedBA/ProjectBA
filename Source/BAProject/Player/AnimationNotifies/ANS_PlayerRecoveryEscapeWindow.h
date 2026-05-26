#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "ANS_PlayerRecoveryEscapeWindow.generated.h"

/**
 * 공격/일반 피격 후딜에서 입력 탈출이 가능한 구간을 연다.
 *
 * 이 NotifyState가 열린 동안에만 플레이어가 현재 공격/피격 후딜을 끊고
 * 허용된 공격, 회피, 가드, 이동 입력으로 복귀할 수 있다.
 */
UCLASS()
class BAPROJECT_API UANS_PlayerRecoveryEscapeWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
	FBAPlayerRecoveryEscapeWindowSettings Settings;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
