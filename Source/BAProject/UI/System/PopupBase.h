// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/LayerBase.h"
#include "PopupBase.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UPopupBase : public ULayerBase
{
	GENERATED_BODY()

public:
	UPopupBase(const FObjectInitializer& ObjectInitializer);

	// 인터페이스 함수 재정의
	// Popup 타입인지 반환
	virtual EStackElemType GetStackType() const override { return EStackElemType::Popup; }

	// 팝업창 닫기버튼 클릭 시 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	virtual void ClosePopup();

protected:
	// 위젯이 키보드 입력을 받았을 때 실행되는 함수
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
};
