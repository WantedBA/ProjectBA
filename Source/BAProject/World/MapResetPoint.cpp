#include "World/MapResetPoint.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/BAPlayerCharacter.h"
#include "Component/StatComponent.h"
#include "Instance/QuestManageSubsystem.h"
#include "SaveGame/SaveGameManager.h"
#include "Kismet/GameplayStatics.h"

AMapResetPoint::AMapResetPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionPivot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPivot"));
	RootComponent = InteractionPivot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
}

void AMapResetPoint::BeginPlay()
{
	Super::BeginPlay();
}

bool AMapResetPoint::CanInteract_Implementation(AActor* Interactor) const
{
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
		OnRequestOpenSkillTree.Broadcast();

		// SaveProgress
		if (UGameInstance* GI = GetGameInstance())
		{
			if (USaveGameManager* SaveManager = GI->GetSubsystem<USaveGameManager>())
			{
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
