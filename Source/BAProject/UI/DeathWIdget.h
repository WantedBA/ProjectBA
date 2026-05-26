// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/PopupBase.h"
#include "DeathWIdget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UDeathWidget : public UPopupBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

public:
	// 마지막 체크포인트에서 다시 시작
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void RetryGame();

	// 타이틀 화면으로 돌아가기
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void BackToTitle();
};
