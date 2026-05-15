#include "Enemy/Animation/EnemyAnimInstance.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    Owner = Cast<AEnemyBase>(TryGetPawnOwner());

	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	int32 MonsterTid = Owner->GetMonsterTid();
	const FMonsterRows* MonsterRow = TableManager->FindMonster(MonsterTid);
	if (MonsterRow == nullptr)
	{
		return;
	}

	MoveSpeed = static_cast<float>(MonsterRow->MoveSpeed);
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (Owner == nullptr)
    {
        Owner = Cast<AEnemyBase>(TryGetPawnOwner());
    }

    if (Owner)
    {
        MoveSpeed = Owner->GetVelocity().Size();
        CurrentState = Owner->GetCurrentState();
    }
}
