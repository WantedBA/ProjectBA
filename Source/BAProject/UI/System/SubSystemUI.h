// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Stack.h"
#include "StackElem.h"
#include "SubSystemUI.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class BAPROJECT_API USubSystemUI : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// 위젯 출력 및 스택 추가 함수
	// LayerBase를 상속받은 경우만 가능하도록 설계
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void PushUI(class ULayerBase* InWidget);

	// 클래스 타입을 넘겨주면 내부에서 생성 해주는 자동화 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI", meta = (DisplayName = "Push UI By Class"))
	class ULayerBase* PushUIByClass(TSubclassOf<ULayerBase> InWidgetClass);

	template<typename T>
	T* PushUI()
	{
		return Cast<T>(PushUIByClass(T::StaticClass()));
	}

	// 가장 위에 있는 위젯을 끄고 스택에서 제거
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void PopUI();

	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	bool HandleBackAction();

protected:
	// 인터페이스(IStackElem) 타입으로 쌓을 스택 바구니
	TStack<IStackElem*> UIStack;

private:
	// 스택 상황에 따라 마우스 커서와 입력 모드 결정
	void RefreshInputMode();
};
