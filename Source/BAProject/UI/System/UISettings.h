// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UISettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta=(DisplayName="BA UI System Settings"))
class BAPROJECT_API UUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(COnfig, EditAnywhere, BlueprintReadOnly, Category = "Default UI")
	TSubclassOf<class UNotifyLayer> DefaultNotifyLayerClass;
};
