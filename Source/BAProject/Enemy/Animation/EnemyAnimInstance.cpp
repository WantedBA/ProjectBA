#include "Enemy/Animation/EnemyAnimInstance.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    Owner = Cast<AEnemyBase>(TryGetPawnOwner());
    if (Owner == nullptr)
    {
        return;
    }

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

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter == nullptr)
    {
        return;
    }

    FVector ToPlayer =PlayerCharacter->GetActorLocation()- Owner->GetActorLocation();
    FRotator TargetRot = ToPlayer.Rotation();
    FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(TargetRot,Owner->GetActorRotation());

    TargetToAngle = DeltaRot.Yaw;
}
