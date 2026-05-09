// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BAStack.h"
#include "BAStackElem.h"
#include "BASubSystemUI.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class BAPROJECT_API UBASubSystemUI : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// 위젯 출력 및 스택 추가 함수
	// BALayerBase를 상속받은 경우만 가능하도록 설계
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PushUI(class UBALayerBase* InWidget);

	// 가장 위에 있는 위젯을 끄고 스택에서 제거
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopUI();

protected:
	// 인터페이스(IBAStackElem) 타입으로 쌓을 스택 바구니
	TBAStack<IBAStackElem*> UIStack;

private:
	// 스택 상황에 따라 마우스 커서와 입력 모드 결정
	void RefreshInputMode();
};
