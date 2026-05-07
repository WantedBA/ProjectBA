// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusBar.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UStatusBar : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* Bar;

public:
	// Bar 퍼센트 조절
	void SetProgress(float Current, float Max);
};
