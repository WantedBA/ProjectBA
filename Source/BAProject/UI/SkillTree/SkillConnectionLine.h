// SkillTree 블루프린트에서 구현하는 게 나을 지 고민 중


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillConnectionLine.generated.h"

/**

 #1#  */
UCLASS()
class BAPROJECT_API USkillConnectionLine : public UUserWidget
{
	GENERATED_BODY()

public:
	// 부모 노드와 자식 노드 설정 
	UFUNCTION(BlueprintCallable, Category = "BA|SkillTree")
	void SetSkillNodes(class USkillNodeWidget* InSourceNode, class USkillNodeWidget* InTargetNode);

protected:
	// 선 그리기 함수
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	//  두 노드의 중앙 좌표 계산 보조 함수
	TArray<FVector2D> MakePointsToDrawLine(const FGeometry& AllottedGeometry) const;

protected:
	// 연결할 대상 노드
	UPROPERTY()
	class USkillNodeWidget* SourceNode;

	UPROPERTY()
	class USkillNodeWidget* TargetNode;

	// 선 그릴 때 사용할 속성
	UPROPERTY(EditAnywhere, Category = "BA|SkillTree")
	float LineThickness = 2.0f;
};

