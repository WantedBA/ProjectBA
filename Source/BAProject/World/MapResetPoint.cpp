#include "World/MapResetPoint.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Component/StatComponent.h"
#include "Instance/QuestManageSubsystem.h"
#include "SaveGame/SaveGameManager.h"
#include "Player/BAPlayerCharacter.h"
#include "UI/NotifyLayer.h"
#include "UI/SkillTree/SkillTreeWidget.h"
#include "UI/System/LayerBase.h"
#include "UI/System/SubSystemUI.h"
#include "TimerManager.h"

AMapResetPoint::AMapResetPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("ResetPointRoot"));
	RootComponent = Root;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	// 감지를 위해 콜리전 활성화 (InteractorComponent의 Sphere가 감지할 수 있도록)
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeshComponent->SetupAttachment(Root);

	InteractionPivot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPivot"));
	InteractionPivot->SetRelativeLocation(FVector(40.f, 40.f, 30.f));
	InteractionPivot->SetupAttachment(Root);

	RespawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(Root);
	RespawnPoint->ArrowSize = 3.0f;
	RespawnPoint->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
}

void AMapResetPoint::BeginPlay()
{
	Super::BeginPlay();
}

void AMapResetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OpenSkillTreeTimerHandle);
		World->GetTimerManager().ClearTimer(FinishRestExitTimerHandle);
	}

	if (SkillTreeWidget)
	{
		SkillTreeWidget->OnCloseAnimationFinished.RemoveDynamic(this, &AMapResetPoint::HandleSkillTreeClosed);
	}

	Super::EndPlay(EndPlayReason);
}

void AMapResetPoint::OpenSkillTreeInResetPoint()
{
	if (!ensureMsgf(SkillTreeWidgetClass, TEXT("SkillTreeWidgetClass is not configured on %s"), *GetName()))
	{
		BeginRestExit();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		BeginRestExit();
		return;
	}

	USubSystemUI* UISubsystem = GameInstance->GetSubsystem<USubSystemUI>();
	if (!ensureMsgf(UISubsystem, TEXT("USubSystemUI is not available.")))
	{
		BeginRestExit();
		return;
	}

	SkillTreeWidget = Cast<USkillTreeWidget>(UISubsystem->PushUIByClass(SkillTreeWidgetClass));
	if (!SkillTreeWidget)
	{
		BeginRestExit();
		return;
	}

	SkillTreeWidget->OnCloseAnimationFinished.RemoveDynamic(this, &AMapResetPoint::HandleSkillTreeClosed);
	SkillTreeWidget->OnCloseAnimationFinished.AddDynamic(this, &AMapResetPoint::HandleSkillTreeClosed);
	bRestTransitionInProgress = false;
}

void AMapResetPoint::CloseSkillTreeInResetPoint()
{
	if (SkillTreeWidget && SkillTreeWidget->IsInViewport())
	{
		SkillTreeWidget->ClosePopup();
		return;
	}

	BeginRestExit();
}

void AMapResetPoint::HandleSkillTreeClosed(ULayerBase* ClosedWidget)
{
	if (ClosedWidget != SkillTreeWidget)
	{
		return;
	}

	SkillTreeWidget->OnCloseAnimationFinished.RemoveDynamic(this, &AMapResetPoint::HandleSkillTreeClosed);
	SkillTreeWidget = nullptr;
	BeginRestExit();
}

bool AMapResetPoint::CanInteract_Implementation(AActor* Interactor) const
{
#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), GetActorLocation(), 100.f, 12, FColor::Yellow, false, 0.1f);
#endif

	if (bConditionUnlocked == false)
	{
		return false;
	}

	if (bRestTransitionInProgress)
	{
		return false;
	}
	
	return true;
}

void AMapResetPoint::Interact_Implementation(AActor* Interactor)
{
	if (bRestTransitionInProgress)
	{
		return;
	}

	if (bIsPlayerResting == true)
	{
		CloseSkillTreeInResetPoint();
		return;
	}

	ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(Interactor);
	if (Player == nullptr)
	{
		return;
	}

	BeginRest(*Player);
}

void AMapResetPoint::BeginRest(ABAPlayerCharacter& Player)
{
	bIsPlayerResting = true;
	bRestTransitionInProgress = true;
	RestingPlayer = &Player;

	MovePlayerToRestPosition(Player);
	Player.LockMovementForCutscene();
	PlayPlayerMontage(Player, SitDownMontage);
	ApplyRestEffects(Player);
	SaveRespawnProgress();
	PlayCheckpointFade(false);

	if (UWorld* World = GetWorld())
	{
		const float OpenDelay = FMath::Max(0.f, SkillTreeOpenDelay);
		World->GetTimerManager().ClearTimer(OpenSkillTreeTimerHandle);
		if (OpenDelay <= KINDA_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &AMapResetPoint::OpenSkillTreeInResetPoint);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				OpenSkillTreeTimerHandle,
				this,
				&AMapResetPoint::OpenSkillTreeInResetPoint,
				OpenDelay,
				false);
		}
	}
	else
	{
		OpenSkillTreeInResetPoint();
	}
}

void AMapResetPoint::BeginRestExit()
{
	if (!bIsPlayerResting && !RestingPlayer)
	{
		return;
	}

	bRestTransitionInProgress = true;
	PlayCheckpointFade(true);

	ABAPlayerCharacter* Player = RestingPlayer.Get();
	if (Player && SitDownMontage)
	{
		Player->StopAnimMontage(SitDownMontage);
	}
	const float StandUpDuration = Player ? PlayPlayerMontage(*Player, StandUpMontage) : 0.f;
	const float UnlockDelay = ExitUnlockDelay > 0.f ? ExitUnlockDelay : StandUpDuration;
	if (UWorld* World = GetWorld())
	{
		const float ResolvedUnlockDelay = FMath::Max(0.f, UnlockDelay);
		World->GetTimerManager().ClearTimer(FinishRestExitTimerHandle);
		if (ResolvedUnlockDelay <= KINDA_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &AMapResetPoint::FinishRestExit);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				FinishRestExitTimerHandle,
				this,
				&AMapResetPoint::FinishRestExit,
				ResolvedUnlockDelay,
				false);
		}
	}
	else
	{
		FinishRestExit();
	}
}

void AMapResetPoint::FinishRestExit()
{
	if (ABAPlayerCharacter* Player = RestingPlayer.Get())
	{
		Player->UnlockMovementForCutscene();
	}

	RestingPlayer = nullptr;
	bIsPlayerResting = false;
	bRestTransitionInProgress = false;
}

void AMapResetPoint::MovePlayerToRestPosition(ABAPlayerCharacter& Player) const
{
	if (!InteractionPivot)
	{
		return;
	}

	Player.SetActorLocationAndRotation(
		InteractionPivot->GetComponentLocation(),
		InteractionPivot->GetComponentRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (APlayerController* PlayerController = Cast<APlayerController>(Player.GetController()))
	{
		PlayerController->SetControlRotation(InteractionPivot->GetComponentRotation());
	}
}

void AMapResetPoint::ApplyRestEffects(ABAPlayerCharacter& Player) const
{
	if (UStatComponent* StatComp = Player.FindComponentByClass<UStatComponent>())
	{
		StatComp->RestoreAll();
	}

	if (UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this))
	{
		QM->ResetActiveQuests();
		QM->RespawnQuestZoneEnemies();
	}
}

void AMapResetPoint::SaveRespawnProgress() const
{
	if (!RespawnPoint)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveGameManager* SaveManager = GI->GetSubsystem<USaveGameManager>())
		{
			SaveManager->SetRespawnPoint(RespawnPoint->GetComponentLocation(), RespawnPoint->GetComponentRotation());
			SaveManager->SaveGame();
		}
	}
}

void AMapResetPoint::PlayCheckpointFade(const bool bFadeIn) const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USubSystemUI* UISubsystem = GI->GetSubsystem<USubSystemUI>())
		{
			if (UNotifyLayer* NotifyLayer = UISubsystem->GetNotifyLayer())
			{
				NotifyLayer->PlayFadeEffect(bFadeIn);
			}
		}
	}
}

float AMapResetPoint::PlayPlayerMontage(ABAPlayerCharacter& Player, UAnimMontage* Montage) const
{
	return Montage ? Player.PlayAnimMontage(Montage) : 0.f;
}

FText AMapResetPoint::GetInteractionPrompt_Implementation() const
{
	return bIsPlayerResting ? PromptExit : PromptEnter;
}

FVector AMapResetPoint::GetInteractionLocation_Implementation() const
{
	return GetActorLocation();
}
