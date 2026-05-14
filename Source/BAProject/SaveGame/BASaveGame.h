// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BASaveGame.generated.h"

USTRUCT(BlueprintType)
struct FSkillTreeSaveData
{
	GENERATED_BODY()
	
	UPROPERTY(SaveGame)
	int32 SkillPoint = -1;
	
	UPROPERTY(SaveGame)
	TSet<int32> ActivatedSkillIds;
};

/**
 * 
 */
UCLASS()
class BAPROJECT_API UBASaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(SaveGame)
	FSkillTreeSaveData SkillTreeData;
};
