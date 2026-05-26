#include "World/MapResetPoint.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Component/StatComponent.h"
#include "Instance/QuestManageSubsystem.h"
#include "SaveGame/SaveGameManager.h"
#include "Player/BAPlayerCharacter.h"
#include "UI/SkillTree/SkillTreeWidget.h"
#include "UI/System/SubSystemUI.h"

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
	InteractionPivot->SetRelativeLocation(FVector(30.f, 30.f, 10.f));
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

void AMapResetPoint::ToggleSkillTreeInResetPoint()
{
	if (SkillTreeWidget && SkillTreeWidget->IsInViewport())
	{
		SkillTreeWidget->ClosePopup();
		SkillTreeWidget = nullptr;
		return;
	}

	if (!ensureMsgf(SkillTreeWidgetClass, TEXT("SkillTreeWidgetClass is not configured on %s"), *GetName()))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	USubSystemUI* UISubsystem = GameInstance->GetSubsystem<USubSystemUI>();
	if (!ensureMsgf(UISubsystem, TEXT("USubSystemUI is not available.")))
	{
		return;
	}

	SkillTreeWidget = Cast<USkillTreeWidget>(UISubsystem->PushUIByClass(SkillTreeWidgetClass));
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
	
	return true;
}

void AMapResetPoint::Interact_Implementation(AActor* Interactor)
{
	ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(Interactor);
	if (Player == nullptr)
	{
		return;
	}

	if (bIsPlayerResting == true)
	{
		bIsPlayerResting = false;
		Player->UnlockMovementForCutscene();
	}
	else
	{
		bIsPlayerResting = true;

		// RecoverPlayer
		if (UStatComponent* StatComp = Player->FindComponentByClass<UStatComponent>())
		{
			StatComp->RestoreAll();
		}

		// RespawnEnemies
		if (UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this))
		{
			QM->RespawnQuestZoneEnemies();
		}

		// OpenSkillTree
		ToggleSkillTreeInResetPoint();

		// SaveProgress
		if (UGameInstance* GI = GetGameInstance())
		{
			if (USaveGameManager* SaveManager = GI->GetSubsystem<USaveGameManager>())
			{
				SaveManager->SetRespawnPoint(RespawnPoint->GetComponentLocation(), RespawnPoint->GetComponentRotation());
				SaveManager->SaveGame();
			}
		}

		Player->LockMovementForCutscene();
	}
}

FText AMapResetPoint::GetInteractionPrompt_Implementation() const
{
	return bIsPlayerResting ? PromptExit : PromptEnter;
}

FVector AMapResetPoint::GetInteractionLocation_Implementation() const
{
	return GetActorLocation();
}
