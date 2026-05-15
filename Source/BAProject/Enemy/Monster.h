#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyBase.h"
#include "Monster.generated.h"

UCLASS()
class BAPROJECT_API AMonster : public AEnemyBase
{
	GENERATED_BODY()

public:
	AMonster();

protected:
	virtual void BeginPlay() override;
	virtual void OnDeath() override;
};
