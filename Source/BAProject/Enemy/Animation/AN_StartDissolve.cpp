#include "Enemy/Animation/AN_StartDissolve.h"
#include "Enemy/EnemyBase.h"

void UAN_StartDissolve::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    UE_LOG(LogTemp, Warning, TEXT("StartDissolve Notify"));

    if (MeshComp == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartDissolve Notify Meshcomponent nullptr"));
        return;
    }

    AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
    if (IsValid(Enemy) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartDissolve Notify EnemyBase Invalid"));
        return;
    }

    Enemy->OnStartDissolve();
}
