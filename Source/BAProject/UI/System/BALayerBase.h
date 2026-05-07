// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BAStackElem.h"
#include "BALayerBase.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UBALayerBase : public UUserWidget, public IBAStackElem
{
	GENERATED_BODY()
	
public:
	// 기본 값은 Layer 타입으로 지정
	virtual EStackElemType GetStackType() const override { return EStackElemType::Layer; }

	// 기본 정렬 순서 설정
	virtual int32 GetSortOrder() const override { return SortOrder; }

	// Push/Pop 될 때 실행될 로직
	virtual void OnPushed() override;
	virtual void OnPopped() override;

protected:
	// 에디터에서 정렬 순서 변경을 위한 변수
	UPROPERTY(EditAnywhere, Category = "UI")
	int32 SortOrder = 0;
};
