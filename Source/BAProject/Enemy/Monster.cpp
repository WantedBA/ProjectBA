#include "Enemy/Monster.h"

AMonster::AMonster()
{
}
void AMonster::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterTid != 0)
	{
		InitializeFromTable(MonsterTid);
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