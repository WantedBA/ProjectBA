// Copyright TeamBA. All Rights Reserved.

#include "Component/ActionAnimationComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tables/ActionRows.h"
#include "Tables/BATableManager.h"

namespace
{
	constexpr int32 ActionAnimationInvalidActionTid = 0;
	constexpr int32 ActionAnimationInvalidActionAnimationTid = 0;
}

UActionAnimationComponent::UActionAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		CachedActionComponent->OnActionStarted.AddDynamic(this, &UActionAnimationComponent::HandleActionStarted);
		CachedActionComponent->OnActionCompleted.AddDynamic(this, &UActionAnimationComponent::HandleActionCompleted);
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

bool UActionAnimationComponent::PlayActionAnimation(const int32 ActionTid, const EActionType ActionType)
{
	if (!CachedActionComponent)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::ActionComponentUnavailable;
		return false;
	}

	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::TableManagerUnavailable;
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	const FActionAnimationDataRow* AnimationData = FindBestAnimationData(ActionTid);
	if (!AnimationData)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::AnimationDataNotFound;
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
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	UAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance)
	{
		LastPlaybackResult = EActionAnimationPlaybackResult::AnimInstanceUnavailable;
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	StopActiveMontage(true);

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
		CompleteActionIfStillActive(ActionTid);
		return false;
	}

	if (!AnimationData->StartSection.IsNone())
	{
		AnimInstance->Montage_JumpToSection(AnimationData->StartSection, Montage);
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UActionAnimationComponent::HandleMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

	ActiveActionTid = ActionTid;
	ActiveActionAnimationTid = AnimationData->Tid;
	ActiveActionType = ActionType;
	ActiveMontage = Montage;
	LastPlaybackResult = EActionAnimationPlaybackResult::Success;

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

	ClearActivePlayback();
	OnActionMontageEnded.Broadcast(StoppedActionTid, StoppedActionType, MontageToStop, bInterrupted);
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
		if (Row->Direction != EActionDirection::Any && Row->Direction != Direction)
		{
			continue;
		}

		int32 Score = 0;
		Score += Row->WeaponType == WeaponType ? 8 : 0;
		Score += Row->Direction == Direction ? 4 : 0;
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

	ClearActivePlayback();
}

void UActionAnimationComponent::HandleMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	if (!ActiveMontage || Montage != ActiveMontage)
	{
		return;
	}

	const int32 CompletedActionTid = ActiveActionTid;
	const EActionType CompletedActionType = ActiveActionType;
	UAnimMontage* CompletedMontage = ActiveMontage;

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

void UActionAnimationComponent::ClearActivePlayback()
{
	ActiveActionTid = ActionAnimationInvalidActionTid;
	ActiveActionAnimationTid = ActionAnimationInvalidActionAnimationTid;
	ActiveActionType = EActionType::None;
	ActiveMontage = nullptr;
}
