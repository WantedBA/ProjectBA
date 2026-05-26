// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UISettings.generated.h"

/**
 * 
 */
class UDeathWidget;
class UNotifyLayer;

UCLASS(Config = Game, DefaultConfig, meta=(DisplayName="BA UI System Settings"))
class BAPROJECT_API UUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Default UI")
	TSubclassOf<class UNotifyLayer> DefaultNotifyLayerClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Default UI")
	TSubclassOf<class UDeathWidget> DeathWidgetClass;
};
