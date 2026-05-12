// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StackElem.h"
#include "LayerBase.generated.h"

/**
 * 
 */

class ULayerBase;

// 애니메이션이 끝난 시점에 매니저 호출
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimationFinished, ULayerBase*, Widget);

UCLASS()
class BAPROJECT_API ULayerBase : public UUserWidget, public IStackElem
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

public:
	// SubSystemUI 에서 위젯을 닫을때 호출
	virtual void StartCloseProcess();

	// 애니메이션이 종료 되었을때
	UPROPERTY(BlueprintAssignable, Category = "BA|UI|Event")
	FOnAnimationFinished OnCloseAnimationFinished;

protected:
	// 에디터에서 정렬 순서 변경을 위한 변수
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	int32 SortOrder = 0;

	// 애니메이션 종료 시 호출될 함수
	UFUNCTION()
	void OnOutAnimationFinished();

	// 애니메이션 연결용 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BA|UI|Animation")
	class UWidgetAnimation* OutAnimation;
};
