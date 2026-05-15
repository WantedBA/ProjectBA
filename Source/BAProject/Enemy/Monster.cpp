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

void AMonster::OnDeath()
{
	Super::OnDeath();
	//// [Opinion] 일반 몬스터는 단순한 소멸 이펙트만 재생
	//if (DefaultDeathMontage)
	//{
	//	PlayAnimMontage(DefaultDeathMontage);
	//}

	//// 몬스터는 사망 시 단순히 경험치나 아이템 드랍 로직 처리
	//DropLoot();
}
