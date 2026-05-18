// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tables/ActionEnums.h"
#include "ActionAnimationComponent.generated.h"

class UActionComponent;
class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
struct FActionAnimationDataRow;

UENUM(BlueprintType)
enum class EActionAnimationPlaybackResult : uint8
{
	Success,
	ActionComponentUnavailable,
	TableManagerUnavailable,
	AnimationDataNotFound,
	MontageUnavailable,
	AnimInstanceUnavailable,
	MontagePlayFailed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnActionMontageStarted,
	int32,
	ActionTid,
	EActionType,
	ActionType,
	UAnimMontage*,
	Montage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnActionMontageEnded,
	int32,
	ActionTid,
	EActionType,
	ActionType,
	UAnimMontage*,
	Montage,
	bool,
	bInterrupted);

/**
 * 공용 Action 애니메이션 재생 컴포넌트.
 *
 * ActionComponent 가 시작한 ActionTid 를 ActionAnimationData 로 해석한 뒤,
 * 소유자의 SkeletalMesh 애니메이션 인스턴스에서 몽타주를 재생한다.
 *
 * 책임:
 *   - ActionComponent 의 시작/종료 이벤트를 구독한다.
 *   - ActionAnimationData 테이블에서 현재 방향, 전투 태세, 무기에 맞는 행을 고른다.
 *   - 몽타주 재생, 섹션 이동, 종료 콜백을 처리한다.
 *   - 몽타주가 끝나면 ActionComponent 에 액션 종료를 통지한다.
 *
 * 무적 iframe, 히트 프레임, 캔슬 구간 같은 시간축 판정은
 * ActionWindowData 를 읽는 후속 단계에서 이 컴포넌트에 연결한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UActionAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActionAnimationComponent();

	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionMontageStarted OnActionMontageStarted;

	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionMontageEnded OnActionMontageEnded;

	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	bool PlayActionAnimation(int32 ActionTid, EActionType ActionType);

	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	void StopActiveMontage(bool bInterrupted = true);

	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	bool IsPlayingActionMontage() const { return ActiveMontage != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	int32 GetActiveActionAnimationTid() const { return ActiveActionAnimationTid; }

	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	UAnimMontage* GetActiveMontage() const { return ActiveMontage; }

	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	EActionAnimationPlaybackResult GetLastPlaybackResult() const { return LastPlaybackResult; }

	const FActionAnimationDataRow* FindBestAnimationData(int32 ActionTid) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleActionStarted(int32 ActionTid, EActionType ActionType);

	UFUNCTION()
	void HandleActionCompleted(int32 ActionTid, EActionType ActionType);

	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void CompleteActionIfStillActive(int32 ActionTid);
	USkeletalMeshComponent* ResolveMeshComponent() const;
	UAnimInstance* ResolveAnimInstance() const;
	void ClearActivePlayback();

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bAutoBindToOwnerActionComponent = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bCompleteActionWhenMontageEnds = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bStopMontageWhenActionCompletes = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bSynchronousLoadMontage = true;

	UPROPERTY(Transient)
	TObjectPtr<UActionComponent> CachedActionComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	int32 ActiveActionTid = 0;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	int32 ActiveActionAnimationTid = 0;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	EActionType ActiveActionType = EActionType::None;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	TObjectPtr<UAnimMontage> ActiveMontage;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	EActionAnimationPlaybackResult LastPlaybackResult = EActionAnimationPlaybackResult::Success;

	bool bHandlingMontageEnd = false;
};
