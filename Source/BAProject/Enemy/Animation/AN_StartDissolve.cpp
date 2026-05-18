#include "Enemy/Animation/AN_StartDissolve.h"
#include "Enemy/EnemyBase.h"

void UAN_StartDissolve::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (MeshComp == nullptr)
    {
        return;
    }

    AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
    if (IsValid(Enemy))
    {
        //Enemy->OnStartDissolve();
    }
}
