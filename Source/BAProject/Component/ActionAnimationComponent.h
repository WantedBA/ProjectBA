// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimEnums.h"
#include "Components/ActorComponent.h"
#include "Tables/ActionEnums.h"
#include "ActionAnimationComponent.generated.h"

class UActionComponent;
class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
struct FActionAnimationDataRow;
struct FActionWindowDataRow;

// 액션 애니메이션 재생 요청이 실패했을 때의 세부 원인.
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

// 액션 몽타주 재생이 시작될 때 선택된 액션 정보와 몽타주를 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnActionMontageStarted,
	int32,
	ActionTid,
	EActionType,
	ActionType,
	UAnimMontage*,
	Montage);

// 액션 몽타주가 종료될 때 중단 여부와 함께 알린다.
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

// ActionWindowData의 윈도우가 열리거나 닫힐 때 호출되는 공용 이벤트.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnActionWindowEvent,
	int32,
	ActionTid,
	int32,
	ActionAnimationTid,
	int32,
	ActionWindowTid,
	EActionWindowType,
	WindowType,
	const FString&,
	Payload);

// 액션 도메인이 런타임 실행 방향을 애니메이션 행 검색 방향으로 변환할 때 사용한다.
DECLARE_DELEGATE_RetVal_TwoParams(EActionDirection, FResolveActionAnimationDirection, int32, EActionDirection);

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
 * ActionWindowData 기반 시간축 판정을 몽타주 재생 위치에 맞춰 열고 닫는다.
 * 현재는 무적 iframe 과 인터럽트 잠금 윈도우를 적용하고, 다른 윈도우 타입은 이벤트로 노출한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UActionAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActionAnimationComponent();

	// 몽타주 재생이 성공적으로 시작된 직후 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionMontageStarted OnActionMontageStarted;

	// 몽타주 종료 콜백에서 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionMontageEnded OnActionMontageEnded;

	// 액션 윈도우가 활성화되는 순간 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionWindowEvent OnActionWindowOpened;

	// 액션 윈도우가 비활성화되는 순간 브로드캐스트된다.
	UPROPERTY(BlueprintAssignable, Category = "Action|Animation|Event")
	FOnActionWindowEvent OnActionWindowClosed;

	FResolveActionAnimationDirection ResolveActionAnimationDirection;

	// ActionTid에 가장 적합한 ActionAnimationData를 찾아 몽타주를 재생한다.
	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	bool PlayActionAnimation(int32 ActionTid, EActionType ActionType);

	// 현재 액션 몽타주를 정지하고 필요하면 중단 종료로 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	void StopActiveMontage(bool bInterrupted = true);

	// 액션 몽타주가 활성 상태인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	bool IsPlayingActionMontage() const { return ActiveMontage != nullptr; }

	// 현재 재생 중인 ActionAnimationData Tid를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	int32 GetActiveActionAnimationTid() const { return ActiveActionAnimationTid; }

	// 현재 재생 중인 몽타주를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	UAnimMontage* GetActiveMontage() const { return ActiveMontage; }

	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	bool SetActiveMontageNextSection(FName SectionName, FName NextSectionName);

	UFUNCTION(BlueprintCallable, Category = "Action|Animation")
	bool JumpActiveMontageToSection(FName SectionName);

	// 가장 최근 재생 요청 결과를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Animation")
	EActionAnimationPlaybackResult GetLastPlaybackResult() const { return LastPlaybackResult; }

	// 현재 방향/전투 태세/무기 문맥에 맞는 애니메이션 행을 찾는다.
	const FActionAnimationDataRow* FindBestAnimationData(int32 ActionTid) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleActionStarted(int32 ActionTid, EActionType ActionType);

	UFUNCTION()
	void HandleActionCompleted(int32 ActionTid, EActionType ActionType);

	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 PlaybackInstanceId);
	void CompleteActionIfStillActive(int32 ActionTid);
	USkeletalMeshComponent* ResolveMeshComponent() const;
	UAnimInstance* ResolveAnimInstance() const;
	EActionDirection ResolveAnimationDirection(int32 ActionTid, EActionDirection ActionDirection) const;
	void OrientOwnerToActionDirection(EActionDirection Direction) const;
	void ApplyRootMotionModeForAnimation(UAnimInstance& AnimInstance, const FActionAnimationDataRow& AnimationData);
	void RestoreRootMotionMode();
	void RestoreRootMotionRotation();
	void InitializeActionWindows();
	void TickActionWindows(float MontagePosition);
	void OpenActionWindow(const FActionWindowDataRow& WindowData);
	void CloseActionWindow(const FActionWindowDataRow& WindowData);
	void CloseAllActionWindows();
	void ApplyInvincibleWindowDelta(int32 Delta);
	void ApplyInterruptLockWindowDelta(int32 Delta);
	void ApplyInputBufferWindowDelta(int32 Delta);
	void RefreshActionWindowTick();
	void ClearActivePlayback();
	void MaintainRootMotionRotationLock();

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bAutoBindToOwnerActionComponent = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bCompleteActionWhenMontageEnds = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bStopMontageWhenActionCompletes = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation")
	bool bSynchronousLoadMontage = true;

	UPROPERTY(EditAnywhere, Category = "Action|Animation|RootMotion")
	bool bIgnoreRootMotionRotation = true;

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

	int32 ActivePlaybackInstanceId = 0;
	int32 NextPlaybackInstanceId = 1;

	UPROPERTY(VisibleAnywhere, Category = "Action|Animation|Runtime")
	EActionAnimationPlaybackResult LastPlaybackResult = EActionAnimationPlaybackResult::Success;

	TArray<const FActionWindowDataRow*> PendingActionWindows;
	TArray<const FActionWindowDataRow*> OpenActionWindows;
	int32 OpenInvincibleWindowCount = 0;
	int32 OpenInterruptLockWindowCount = 0;
	int32 OpenInputBufferWindowCount = 0;
	TWeakObjectPtr<UAnimInstance> RootMotionModeAnimInstance;
	TEnumAsByte<ERootMotionMode::Type> PreviousRootMotionMode = ERootMotionMode::NoRootMotionExtraction;
	bool bRootMotionModeOverridden = false;
	FRotator LockedRootMotionRotation = FRotator::ZeroRotator;
	bool bRootMotionRotationLocked = false;
	bool bHandlingMontageEnd = false;
};
