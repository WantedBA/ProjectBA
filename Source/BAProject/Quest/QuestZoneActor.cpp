// Copyright TeamBA. All Rights Reserved.

#include "Quest/QuestZoneActor.h"
#include "Components/BoxComponent.h"
#include "Instance/QuestManageSubsystem.h"
#include "Player/BAPlayerCharacter.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#endif


// Sets default values
AQuestZoneActor::AQuestZoneActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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
        static ConstructorHelpers::FObjectFinder<UTexture2D>
            TriggerTexture(TEXT("/Engine/EditorResources/S_Trigger"));
        if (TriggerTexture.Succeeded())
            EditorSprite->SetSprite(TriggerTexture.Object);
        EditorSprite->SetupAttachment(TriggerBox);
        EditorSprite->bIsScreenSizeScaled = true;
    }
#endif
}

void AQuestZoneActor::ReArm()
{
    bArmed = true;
    if (bUseTriggerVolume)
    {
        TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        TriggerBox->SetGenerateOverlapEvents(true);
    }
}

void AQuestZoneActor::RespawnEnemiesInZone()
{
    if (QuestTid <= 0)
    {
        return;
    }

    UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this);
    if (QM == nullptr)
    {
        return;
    }

    // 기존 시체, 몬스터 제거
    QM->ClearQuestMonsters(QuestTid);

    // 현재 진행 중인 해당 퀘스트가 있다면 몬스터만 리필 (진행도 유지)
    if (QM->IsQuestActive(QuestTid))
    {
        QM->RefreshQuestMonsters(QuestTid, CachedSpawnTransforms, MonsterClass);
    }
    else
    {
        // 완료되었거나 시작 전인 경우 트리거 재활성화
        ReArm();
    }
}

// Called when the game starts or when spawned
void AQuestZoneActor::BeginPlay()
{
    Super::BeginPlay();

    GatherSpawnTransforms();

    if (QuestTid <= 0) return;

    UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this);
    if (!QM) return;

    for (AActor* Actor : LinkedActivatables)
    {
        if (Actor)
            QM->RegisterActivatable(QuestTid, Actor);
    }

    if (bUseTriggerVolume)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AQuestZoneActor::OnTriggerBoxBeginOverlap);
        QM->RegisterTrigger(QuestTid, this);
    }
    else
    {
        TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TriggerBox->SetGenerateOverlapEvents(false);
    }
}

void AQuestZoneActor::TriggerStart()
{
    if (!bArmed || QuestTid <= 0) return;

    UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this);
    if (!QM) return;

    bArmed = false;
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TriggerBox->SetGenerateOverlapEvents(false);

    QM->StartQuest(QuestTid, CachedSpawnTransforms, MonsterClass);
}

void AQuestZoneActor::OnTriggerBoxBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!bArmed || !Cast<ABAPlayerCharacter>(OtherActor)) return;
    if (QuestTid <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[QuestZoneActor] %s: QuestTid not set."), *GetName());
        return;
    }

    TriggerStart();
}

void AQuestZoneActor::GatherSpawnTransforms()
{
    CachedSpawnTransforms.Reset();
    if (!GetRootComponent()) return;

    TArray<USceneComponent*> ChildComponents;
    GetRootComponent()->GetChildrenComponents(false, ChildComponents);

    for (USceneComponent* Child : ChildComponents)
    {
#if WITH_EDITORONLY_DATA
        if (Child == EditorSprite) continue;
#endif
        if (Child)
            CachedSpawnTransforms.Add(Child->GetComponentTransform());
    }
}

