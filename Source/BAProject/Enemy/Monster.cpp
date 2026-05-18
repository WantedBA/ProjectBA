#include "Enemy/Monster.h"
#include "AIController.h"

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	EnemyGrade = EEnemyGrade::None;
}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterTid != 0)
	{
		InitializeFromTable(MonsterTid);
	}
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// AI 디버그 정보 시각화
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		const UEnum* EnumPtr = StaticEnum<EEnemyState>();
		FString StateString = EnumPtr? EnumPtr->GetNameStringByValue((int64)GetCurrentState()): TEXT("Invalid");
		FString DebugInfo = FString::Printf(TEXT("CurrentState: %s"),*StateString);
		DrawDebugString(GetWorld(), FVector(0, 0, 150), DebugInfo, this, FColor::Yellow, DeltaTime);
	}
}

void AMonster::InitializeFromTable(int32 InTid)
{
	Super::InitializeFromTable(InTid);
}

void AMonster::OnDeath()
{
	Super::OnDeath();
}