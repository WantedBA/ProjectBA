// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/PopupBase.h"
#include "TitleMenu.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UTitleMenu : public UPopupBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
};
