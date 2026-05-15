// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatus.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UPlayerStatus : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	class UStatusBar* HPBar; // HP바 연결

	UPROPERTY(meta = (BindWidget))
	class UStatusBar* StaminaBar; // 스테미너바 연결

public:
	void UpdateHP(float Current, float Max);
	void UpdateStamina(float Current, float Max);
};
