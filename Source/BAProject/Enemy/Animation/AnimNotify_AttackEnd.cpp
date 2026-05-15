#include "Enemy/Animation/AnimNotify_AttackEnd.h"
#include "Enemy/EnemyBase.h"

void UAnimNotify_AttackEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    Super::Notify(MeshComp, Animation);

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
        if (Enemy->OnAnimationFinished.IsBound())
        {
            Enemy->OnAnimationFinished.Broadcast(TargetEndState);
        }
    }
}
