// Copyright TeamBA. All Rights Reserved.

#include "Component/ActionAnimationComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/CharacterBase.h"
#include "Component/ActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Tables/ActionRows.h"
#include "Tables/BATableManager.h"

namespace
{
	constexpr int32 ActionAnimationInvalidActionTid = 0;
	constexpr int32 ActionAnimationInvalidActionAnimationTid = 0;
	constexpr int32 ActionAnimationInvalidPlaybackInstanceId = 0;

	const TCHAR* LexToString(const EActionAnimationPlaybackResult Result)
	{
		switch (Result)
		{
		case EActionAnimationPlaybackResult::Success:
			return TEXT("Success");
		case EActionAnimationPlaybackResult::ActionComponentUnavailable:
			return TEXT("ActionComponentUnavailable");
		case EActionAnimationPlaybackResult::TableManagerUnavailable:
			return TEXT("TableManagerUnavailable");
		case EActionAnimationPlaybackResult::AnimationDataNotFound:
			return TEXT("AnimationDataNotFound");
		case EActionAnimationPlaybackResult::MontageUnavailable:
			return TEXT("MontageUnavailable");
		case EActionAnimationPlaybackResult::AnimInstanceUnavailable:
			return TEXT("AnimInstanceUnavailable");
		case EActionAnimationPlaybackResult::MontagePlayFailed:
			return TEXT("MontagePlayFailed");
		default:
			return TEXT("Unknown");
		}
	}

	void LogPlaybackFailure(
		const int32 ActionTid,
		const EActionAnimationPlaybackResult Result,
		const FString& Detail = FString())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ActionAnimation] Failed to play action %d: %s%s%s"),
			ActionTid,
			LexToString(Result),
			Detail.IsEmpty() ? TEXT("") : TEXT(" - "),
			*Detail);
	}
}

UActionAnimationComponent::UActionAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActionAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedActionComponent = Owner->FindComponentByClass<UActionComponent>();
		CachedMeshComponent = ResolveMeshComponent();
	}

	if (bAutoBindToOwnerActionComponent && CachedActionComponent)
	{
		CachedActionComponent->OnActionStarted.AddUniqueDynamic(this, &UActionAnimationComponent::HandleActionStarted);
		CachedActionComponent->OnActionCompleted.AddUniqueDynamic(this, &UActionAnimationComponent::HandleActionCompleted);
	}
}

void UActionAnimationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedActionComponent)
	{
		CachedActionComponent->OnActionStarted.RemoveDynamic(this, &UActionAnimationComponent::HandleActionStarted);
		CachedActionComponent->OnActionCompleted.RemoveDynamic(this, &UActionAnimationComponent::HandleActionCompleted);
	}

	StopActiveMontage(false);

	Super::EndPlay(EndPlayReason);
}

void UActionAnimationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance || !ActiveMontage)
	{
		CloseAllActionWindows();
		RefreshActionWindowTick();
		return;
	}

	TickActionWindows(AnimInstance->Montage_GetPosition(ActiveMontage));
	RefreshActionWindowTick();
}

bool UActionAnimationComponent::PlayActionAnimation(const int32 ActionTid, const EActionType ActionType)
{
	if (!CachedActionComponent)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::ActionComponentUnavailable;
		LogPlaybackFailure(ActionTid, LastPlaybackResult);
		return false;
	}

	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::TableManagerUnavailable;
		LogPlaybackFailure(ActionTid, LastPlaybackResult);
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	const FActionAnimationDataRow* AnimationData = FindBestAnimationData(ActionTid);
	if (!AnimationData)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::AnimationDataNotFound;
		LogPlaybackFailure(ActionTid, LastPlaybackResult);
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	UAnimMontage* Montage = AnimationData->Montage.Get();
	if (!Montage && bSynchronousLoadMontage)
	{
		Montage = AnimationData->Montage.LoadSynchronous();
	}
	if (!Montage)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::MontageUnavailable;
		LogPlaybackFailure(ActionTid, LastPlaybackResult, AnimationData->Montage.ToSoftObjectPath().ToString());
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	UAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::AnimInstanceUnavailable;
		LogPlaybackFailure(ActionTid, LastPlaybackResult, GetNameSafe(CachedMeshComponent.Get()));
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	StopActiveMontage(true);
	OrientOwnerToActionDirection(ResolveOrientationDirection(ActionTid, CachedActionComponent->GetActiveActionDirection()));

	const float PlayRate = AnimationData->PlayRate > 0.f ? AnimationData->PlayRate : 1.f;
	const FMontageBlendSettings BlendInSettings(FMath::Max(0.f, AnimationData->BlendIn));
	const float PlayDuration = AnimInstance->Montage_PlayWithBlendSettings(
		Montage,
		BlendInSettings,
		PlayRate,
		EMontagePlayReturnType::Duration);

	if (PlayDuration <= 0.f)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::MontagePlayFailed;
		LogPlaybackFailure(ActionTid, LastPlaybackResult, GetNameSafe(Montage));
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	ApplyRootMotionModeForAnimation(*AnimInstance, *AnimationData);

	if (!AnimationData->StartSection.IsNone())
	{
		AnimInstance->Montage_JumpToSection(AnimationData->StartSection, Montage);
	}

	const int32 PlaybackInstanceId = NextPlaybackInstanceId++;
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UActionAnimationComponent::HandleMontageEnded, PlaybackInstanceId);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

	ActiveActionTid = ActionTid;
	ActiveActionAnimationTid = AnimationData->Tid;
	ActiveActionType = ActionType;
	ActiveMontage = Montage;
	ActivePlaybackInstanceId = PlaybackInstanceId;
	LastPlaybackResult = EActionAnimationPlaybackResult::Success;

	InitializeActionWindows();
	if (UAnimInstance* CurrentAnimInstance = ResolveAnimInstance())
	{
		TickActionWindows(CurrentAnimInstance->Montage_GetPosition(ActiveMontage));
		RefreshActionWindowTick();
	}

	OnActionMontageStarted.Broadcast(ActiveActionTid, ActiveActionType, ActiveMontage);
	return true;
}

void UActionAnimationComponent::StopActiveMontage(const bool bInterrupted)
{
	if (!ActiveMontage)
	{
		return;
	}

	UAnimMontage* MontageToStop = ActiveMontage;
	const int32 StoppedActionTid = ActiveActionTid;
	const EActionType StoppedActionType = ActiveActionType;
	ActivePlaybackInstanceId = ActionAnimationInvalidPlaybackInstanceId;

	if (UAnimInstance* AnimInstance = ResolveAnimInstance())
	{
		float BlendOut = 0.1f;
		if (const UBATableManager* TableManager = UBATableManager::Get(this))
		{
			if (const FActionAnimationDataRow* AnimationData =
				TableManager->FindActionAnimationData(ActiveActionAnimationTid))
			{
				BlendOut = FMath::Max(0.f, AnimationData->BlendOut);
			}
		}

		AnimInstance->Montage_Stop(BlendOut, MontageToStop);
	}

	CloseAllActionWindows();
	ClearActivePlayback();
	OnActionMontageEnded.Broadcast(StoppedActionTid, StoppedActionType, MontageToStop, bInterrupted);
}

bool UActionAnimationComponent::SetActiveMontageNextSection(const FName SectionName, const FName NextSectionName)
{
	UAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance || !ActiveMontage || SectionName.IsNone() || NextSectionName.IsNone())
	{
		return false;
	}

	if (!ActiveMontage->IsValidSectionName(SectionName) || !ActiveMontage->IsValidSectionName(NextSectionName))
	{
		return false;
	}

	AnimInstance->Montage_SetNextSection(SectionName, NextSectionName, ActiveMontage);
	return true;
}

bool UActionAnimationComponent::JumpActiveMontageToSection(const FName SectionName)
{
	UAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance || !ActiveMontage || SectionName.IsNone() || !ActiveMontage->IsValidSectionName(SectionName))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(SectionName, ActiveMontage);
	return true;
}

const FActionAnimationDataRow* UActionAnimationComponent::FindBestAnimationData(const int32 ActionTid) const
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return nullptr;
	}

	const EActionDirection Direction = CachedActionComponent
		? CachedActionComponent->GetActiveActionDirection()
		: EActionDirection::Any;
	const EActionDirection AnimationDirection = ResolveAnimationDirection(ActionTid, Direction);
	const ECombatStance CombatStance = CachedActionComponent
		? CachedActionComponent->GetCombatStance()
		: ECombatStance::Relaxed;
	const EActionWeaponType WeaponType = CachedActionComponent
		? CachedActionComponent->GetWeaponType()
		: EActionWeaponType::Any;

	const FActionAnimationDataRow* BestRow = nullptr;
	int32 BestScore = MIN_int32;

	for (const TPair<int32, FActionAnimationDataRow*>& Pair : TableManager->GetActionAnimationDataTable())
	{
		const FActionAnimationDataRow* Row = Pair.Value;
		if (!Row || Row->ActionTid != ActionTid)
		{
			continue;
		}
		if (Row->CombatStance != CombatStance)
		{
			continue;
		}
		if (Row->WeaponType != EActionWeaponType::Any && Row->WeaponType != WeaponType)
		{
			continue;
		}
		if (Row->Direction != EActionDirection::Any && Row->Direction != AnimationDirection)
		{
			continue;
		}

		int32 Score = 0;
		Score += Row->WeaponType == WeaponType ? 8 : 0;
		Score += Row->Direction == AnimationDirection ? 4 : 0;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestRow = Row;
		}
	}

	return BestRow;
}

void UActionAnimationComponent::HandleActionStarted(const int32 ActionTid, const EActionType ActionType)
{
	PlayActionAnimation(ActionTid, ActionType);
}

void UActionAnimationComponent::HandleActionCompleted(const int32 ActionTid, const EActionType ActionType)
{
	if (bHandlingMontageEnd || ActionTid != ActiveActionTid)
	{
		return;
	}

	if (bStopMontageWhenActionCompletes)
	{
		StopActiveMontage(true);
		return;
	}

	CloseAllActionWindows();
	ClearActivePlayback();
}

void UActionAnimationComponent::HandleMontageEnded(
	UAnimMontage* Montage,
	const bool bInterrupted,
	const int32 PlaybackInstanceId)
{
	if (!ActiveMontage
		|| Montage != ActiveMontage
		|| PlaybackInstanceId != ActivePlaybackInstanceId)
	{
		return;
	}

	const int32 CompletedActionTid = ActiveActionTid;
	const EActionType CompletedActionType = ActiveActionType;
	UAnimMontage* CompletedMontage = ActiveMontage;

	CloseAllActionWindows();
	ClearActivePlayback();
	OnActionMontageEnded.Broadcast(CompletedActionTid, CompletedActionType, CompletedMontage, bInterrupted);

	if (bCompleteActionWhenMontageEnds)
	{
		bHandlingMontageEnd = true;
		CompleteActionIfStillActive(CompletedActionTid);
		bHandlingMontageEnd = false;
	}
}

void UActionAnimationComponent::CompleteActionIfStillActive(const int32 ActionTid)
{
	if (CachedActionComponent && CachedActionComponent->GetActiveActionTid() == ActionTid)
	{
		CachedActionComponent->CompleteCurrentAction();
	}
}

USkeletalMeshComponent* UActionAnimationComponent::ResolveMeshComponent() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

UAnimInstance* UActionAnimationComponent::ResolveAnimInstance() const
{
	USkeletalMeshComponent* MeshComponent = CachedMeshComponent ? CachedMeshComponent.Get() : ResolveMeshComponent();
	return MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
}

// 액션별 방향 정책이 필요한 경우 ResolveActionAnimationDirection Delegate로 전달할 것
EActionDirection UActionAnimationComponent::ResolveAnimationDirection(
	const int32 ActionTid,
	const EActionDirection ActionDirection) const
{
	return ResolveActionAnimationDirection.IsBound()
		? ResolveActionAnimationDirection.Execute(ActionTid, ActionDirection)
		: ActionDirection;
}

EActionDirection UActionAnimationComponent::ResolveOrientationDirection(
	const int32 ActionTid,
	const EActionDirection ActionDirection) const
{
	return ResolveActionOrientationDirection.IsBound()
		? ResolveActionOrientationDirection.Execute(ActionTid, ActionDirection)
		: ActionDirection;
}

void UActionAnimationComponent::OrientOwnerToActionDirection(const EActionDirection Direction) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const APawn* PawnOwner = Cast<APawn>(Owner);
	if (Direction == EActionDirection::Any)
	{
		return;
	}

	FVector2D LocalDirection = FVector2D::ZeroVector;
	switch (Direction)
	{
	case EActionDirection::Forward:
		LocalDirection = FVector2D(0.f, 1.f);
		break;
	case EActionDirection::Backward:
		LocalDirection = FVector2D(0.f, -1.f);
		break;
	case EActionDirection::Left:
		LocalDirection = FVector2D(-1.f, 0.f);
		break;
	case EActionDirection::Right:
		LocalDirection = FVector2D(1.f, 0.f);
		break;
	case EActionDirection::ForwardLeft:
		LocalDirection = FVector2D(-1.f, 1.f).GetSafeNormal();
		break;
	case EActionDirection::ForwardRight:
		LocalDirection = FVector2D(1.f, 1.f).GetSafeNormal();
		break;
	case EActionDirection::BackwardLeft:
		LocalDirection = FVector2D(-1.f, -1.f).GetSafeNormal();
		break;
	case EActionDirection::BackwardRight:
		LocalDirection = FVector2D(1.f, -1.f).GetSafeNormal();
		break;
	case EActionDirection::Any:
	default:
		return;
	}

	const FRotator BaseRotation = PawnOwner ? PawnOwner->GetControlRotation() : Owner->GetActorRotation();
	const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector WorldDirection = (Forward * LocalDirection.Y + Right * LocalDirection.X).GetSafeNormal();
	if (WorldDirection.IsNearlyZero())
	{
		return;
	}

	GetOwner()->SetActorRotation(FRotator(0.f, WorldDirection.Rotation().Yaw, 0.f));
}

void UActionAnimationComponent::ApplyRootMotionModeForAnimation(
	UAnimInstance& AnimInstance,
	const FActionAnimationDataRow& AnimationData)
{
	if (!AnimationData.bUseRootMotion)
	{
		return;
	}

	if (!bRootMotionModeOverridden)
	{
		PreviousRootMotionMode = AnimInstance.RootMotionMode;
		RootMotionModeAnimInstance = &AnimInstance;
		bRootMotionModeOverridden = true;
	}

	AnimInstance.SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
}

void UActionAnimationComponent::RestoreRootMotionMode()
{
	if (!bRootMotionModeOverridden)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = RootMotionModeAnimInstance.Get())
	{
		AnimInstance->SetRootMotionMode(PreviousRootMotionMode);
	}

	RootMotionModeAnimInstance = nullptr;
	bRootMotionModeOverridden = false;
}

void UActionAnimationComponent::InitializeActionWindows()
{
	PendingActionWindows.Reset();
	OpenActionWindows.Reset();
	OpenInvincibleWindowCount = 0;
	OpenInterruptLockWindowCount = 0;
	OpenInputBufferWindowCount = 0;

	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager || ActiveActionAnimationTid == ActionAnimationInvalidActionAnimationTid)
	{
		RefreshActionWindowTick();
		return;
	}

	for (const TPair<int32, FActionWindowDataRow*>& Pair : TableManager->GetActionWindowDataTable())
	{
		const FActionWindowDataRow* Row = Pair.Value;
		if (!Row || Row->ActionAnimationTid != ActiveActionAnimationTid || Row->EndTime <= Row->StartTime)
		{
			continue;
		}

		PendingActionWindows.Add(Row);
	}

	PendingActionWindows.Sort([](const FActionWindowDataRow& Left, const FActionWindowDataRow& Right)
	{
		if (!FMath::IsNearlyEqual(Left.StartTime, Right.StartTime))
		{
			return Left.StartTime < Right.StartTime;
		}
		return Left.Tid < Right.Tid;
	});

	RefreshActionWindowTick();
}

void UActionAnimationComponent::TickActionWindows(const float MontagePosition)
{
	while (!PendingActionWindows.IsEmpty())
	{
		const FActionWindowDataRow* WindowData = PendingActionWindows[0];
		if (!WindowData)
		{
			PendingActionWindows.RemoveAt(0, 1, EAllowShrinking::No);
			continue;
		}
		if (WindowData->StartTime > MontagePosition)
		{
			break;
		}

		PendingActionWindows.RemoveAt(0, 1, EAllowShrinking::No);
		OpenActionWindow(*WindowData);
	}

	for (int32 Index = OpenActionWindows.Num() - 1; Index >= 0; --Index)
	{
		const FActionWindowDataRow* WindowData = OpenActionWindows[Index];
		if (!WindowData || WindowData->EndTime <= MontagePosition)
		{
			if (WindowData)
			{
				CloseActionWindow(*WindowData);
			}
			else
			{
				OpenActionWindows.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			}
		}
	}
}

void UActionAnimationComponent::OpenActionWindow(const FActionWindowDataRow& WindowData)
{
	OpenActionWindows.Add(&WindowData);

	if (WindowData.WindowType == EActionWindowType::Invincible)
	{
		ApplyInvincibleWindowDelta(1);
	}
	else if (WindowData.WindowType == EActionWindowType::InterruptLock)
	{
		ApplyInterruptLockWindowDelta(1);
	}
	else if (WindowData.WindowType == EActionWindowType::InputBuffer)
	{
		ApplyInputBufferWindowDelta(1);
	}

	OnActionWindowOpened.Broadcast(
		ActiveActionTid,
		ActiveActionAnimationTid,
		WindowData.Tid,
		WindowData.WindowType,
		WindowData.Payload);
}

void UActionAnimationComponent::CloseActionWindow(const FActionWindowDataRow& WindowData)
{
	OpenActionWindows.RemoveSingleSwap(&WindowData, EAllowShrinking::No);

	if (WindowData.WindowType == EActionWindowType::Invincible)
	{
		ApplyInvincibleWindowDelta(-1);
	}
	else if (WindowData.WindowType == EActionWindowType::InterruptLock)
	{
		ApplyInterruptLockWindowDelta(-1);
	}
	else if (WindowData.WindowType == EActionWindowType::InputBuffer)
	{
		ApplyInputBufferWindowDelta(-1);
	}

	OnActionWindowClosed.Broadcast(
		ActiveActionTid,
		ActiveActionAnimationTid,
		WindowData.Tid,
		WindowData.WindowType,
		WindowData.Payload);
}

void UActionAnimationComponent::CloseAllActionWindows()
{
	while (!OpenActionWindows.IsEmpty())
	{
		const FActionWindowDataRow* WindowData = OpenActionWindows.Last();
		if (!WindowData)
		{
			OpenActionWindows.Pop(EAllowShrinking::No);
			continue;
		}

		CloseActionWindow(*WindowData);
	}

	PendingActionWindows.Reset();
	OpenInvincibleWindowCount = 0;
	OpenInterruptLockWindowCount = 0;
	OpenInputBufferWindowCount = 0;
	if (CachedActionComponent && CachedActionComponent->GetActiveActionTid() == ActiveActionTid)
	{
		CachedActionComponent->SetActiveActionInterruptLocked(false);
		CachedActionComponent->SetActiveActionInputBufferOpen(false);
	}
}

void UActionAnimationComponent::ApplyInvincibleWindowDelta(const int32 Delta)
{
	const int32 PreviousCount = OpenInvincibleWindowCount;
	OpenInvincibleWindowCount = FMath::Max(0, OpenInvincibleWindowCount + Delta);
	if ((PreviousCount > 0) == (OpenInvincibleWindowCount > 0))
	{
		return;
	}

	if (ACharacterBase* CharacterOwner = Cast<ACharacterBase>(GetOwner()))
	{
		CharacterOwner->SetInvincible(OpenInvincibleWindowCount > 0);
	}
}

void UActionAnimationComponent::ApplyInterruptLockWindowDelta(const int32 Delta)
{
	const int32 PreviousCount = OpenInterruptLockWindowCount;
	OpenInterruptLockWindowCount = FMath::Max(0, OpenInterruptLockWindowCount + Delta);
	if ((PreviousCount > 0) == (OpenInterruptLockWindowCount > 0))
	{
		return;
	}

	if (CachedActionComponent && CachedActionComponent->GetActiveActionTid() == ActiveActionTid)
	{
		CachedActionComponent->SetActiveActionInterruptLocked(OpenInterruptLockWindowCount > 0);
	}
}

void UActionAnimationComponent::ApplyInputBufferWindowDelta(const int32 Delta)
{
	const int32 PreviousCount = OpenInputBufferWindowCount;
	OpenInputBufferWindowCount = FMath::Max(0, OpenInputBufferWindowCount + Delta);
	if ((PreviousCount > 0) == (OpenInputBufferWindowCount > 0))
	{
		return;
	}

	if (CachedActionComponent && CachedActionComponent->GetActiveActionTid() == ActiveActionTid)
	{
		CachedActionComponent->SetActiveActionInputBufferOpen(OpenInputBufferWindowCount > 0);
	}
}

void UActionAnimationComponent::RefreshActionWindowTick()
{
	SetComponentTickEnabled(ActiveMontage && (!PendingActionWindows.IsEmpty() || !OpenActionWindows.IsEmpty()));
}

void UActionAnimationComponent::ClearActivePlayback()
{
	RestoreRootMotionMode();
	PendingActionWindows.Reset();
	OpenActionWindows.Reset();
	OpenInvincibleWindowCount = 0;
	OpenInterruptLockWindowCount = 0;
	OpenInputBufferWindowCount = 0;
	ActiveActionTid = ActionAnimationInvalidActionTid;
	ActiveActionAnimationTid = ActionAnimationInvalidActionAnimationTid;
	ActiveActionType = EActionType::None;
	ActiveMontage = nullptr;
	ActivePlaybackInstanceId = ActionAnimationInvalidPlaybackInstanceId;
	RefreshActionWindowTick();
}
