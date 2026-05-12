// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StackElem.generated.h"

UENUM(BlueprintType)
enum class EStackElemType : uint8
{
	Layer,
	Popup
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UStackElem : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BAPROJECT_API IStackElem
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 위젯이 레이어인지 팝업인지 알려주는 함수
	virtual EStackElemType GetStackType() const = 0;

	// 화면에 그려지는 순서(Order)
	virtual int32 GetSortOrder() const = 0;

	// 스택에 Push 되었을 때 실행할 로직
	virtual void OnPushed() = 0;

	// 스택에서 Pop 되었을 때 실행할 로직
	virtual void OnPopped() = 0;
};
