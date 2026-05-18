// Copyright TeamBA. All Rights Reserved.

#include "Quest/QuestTriggerActor.h"
#include "Components/BoxComponent.h"
#include "Instance/QuestManageSubsystem.h"
#include "Player/BAPlayerCharacter.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#endif

AQuestTriggerActor::AQuestTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetBoxExtent(FVector(300.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->ShapeColor = FColor(255, 196, 64);
	RootComponent = TriggerBox;

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	if (EditorSprite)
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> TriggerTexture(TEXT("/Engine/EditorResources/S_Trigger"));
		if (TriggerTexture.Succeeded())
		{
			EditorSprite->SetSprite(TriggerTexture.Object);
		}
		EditorSprite->SetupAttachment(TriggerBox);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif
}

void AQuestTriggerActor::ReArm()
{
	bArmed = true;
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetGenerateOverlapEvents(false);
}

void AQuestTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	GatherSpawnTransforms();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AQuestTriggerActor::OnTriggerBoxBeginOverlap);
	
	if (QuestTid > 0)
	{
		if (UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this))
		{
			for (AActor* Actor : LinkedActivatables)
			{
				if (Actor)
				{
					QM->RegisterActivatable(QuestTid, Actor);
				}
			}
			QM->RegisterTrigger(QuestTid, this);
		}
	}
}

void AQuestTriggerActor::GatherSpawnTransforms()
{
	CachedSpawnTransforms.Reset();

	if (!GetRootComponent())
	{
		return;
	}

	TArray<USceneComponent*> SpawnPoints;
	GetRootComponent()->GetChildrenComponents(false, SpawnPoints);

	for (USceneComponent* Child : SpawnPoints)
	{
		if (Child && Child != TriggerBox)
		{
			CachedSpawnTransforms.Add(Child->GetComponentTransform());
		}
	}
}

void AQuestTriggerActor::OnTriggerBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bArmed)
	{
		return;
	}
	if (!Cast<ABAPlayerCharacter>(OtherActor))
	{
		return;
	}

	if (QuestTid <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestTriggerActor] %s: QuestTid not set."), *GetName());
		return;
	}

	if (CachedSpawnTransforms.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestTriggerActor] %s: no spawn points (add SceneComponent children)."), *GetName());
		return;
	}

	UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this);
	if (!QM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestTriggerActor] QuestManageSubsystem not available."));
		return;
	}

	QM->StartQuest(QuestTid, CachedSpawnTransforms, MonsterClass);
	
	bArmed = false;
	TriggerBox->SetGenerateOverlapEvents(false);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
