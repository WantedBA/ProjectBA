#include "Enemy/Animation/AN_EnemyAttackEnd.h"
#include "Enemy/EnemyBase.h"

void UAN_EnemyAttackEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (MeshComp == nullptr)
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (Owner == nullptr)
    {
        return;
    }

    AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
    if (IsValid(Enemy))
    {
        if (Enemy->OnAttackAnimationFinished.IsBound())
        {
            Enemy->OnAttackAnimationFinished.Broadcast(TargetEndState);
        }
    }
}
