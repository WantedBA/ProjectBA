/*
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillConnectionLine.h"

#include "SkillNodeWidget.h"

void USkillConnectionLine::SetSkillNodes(class USkillNodeWidget* InSourceNode, class USkillNodeWidget* InTargetNode)
{
	SourceNode = InSourceNode;
	TargetNode = InTargetNode;
	
	
}

int32 USkillConnectionLine::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	
	// SourceNode와 TargetNode가 유효한지 확인
	if (IsValid(SourceNode) && IsValid(TargetNode))
	{
		TArray<FVector2D> Points = MakePointsToDrawLine();
		
		FSlateDrawElement::MakeLines(
			OutDrawElements, ++CurrentLayer, AllottedGeometry.ToPaintGeometry(), 
			Points,
			ESlateDrawEffect::None,
			LineColor, LineThickness);		
	}
	
	return CurrentLayer;
}

TArray<FVector2D> USkillConnectionLine::MakePointsToDrawLine() const
{
	TArray<FVector2D> Points;
	
	// 시작점
	Points.Add(FVector2D::Zero());
	
	// TargetNode의 위치를 Points에 추가
	Points.Add();
	
	return Points;
}
*/
