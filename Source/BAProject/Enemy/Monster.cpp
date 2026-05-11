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
