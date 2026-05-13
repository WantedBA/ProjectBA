// SkillTree 블루프린트에서 구현하는 게 나을 지 고민 중

/*
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillConnectionLine.generated.h"

/**
 * 
 #1#
UCLASS()
class BAPROJECT_API USkillConnectionLine : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 서로 연결할 스킬 노드 설정
	UFUNCTION(BlueprintCallable, Category = SkillTree)
	void SetSkillNodes(class USkillNodeWidget* InSourceNode, class USkillNodeWidget* InTargetNode);
	
protected:
	// 재정의 함수
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	
	// 선 그릴 점들
	TArray<FVector2D> MakePointsToDrawLine() const;
	
protected:
	// 연결할 대상 노드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TObjectPtr<class USkillNodeWidget> SourceNode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TObjectPtr<class USkillNodeWidget> TargetNode;

// 선 그릴 때 사용할 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	FLinearColor LineColor = FLinearColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	float LineThickness = 3.0f;
};
*/
