// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillConnectionLine.h"
#include "UI/SkillTree/SkillNodeWidget.h"
#include "Rendering/DrawElements.h"


void USkillConnectionLine::SetSkillNodes(USkillNodeWidget* InSourceNode, USkillNodeWidget* InTargetNode)
{
	SourceNode = InSourceNode;
	TargetNode = InTargetNode;
}

int32 USkillConnectionLine::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (IsValid(SourceNode) && IsValid(TargetNode))
	{
		// 선 색상 결정
		FLinearColor Color = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // 다크 그레이

		if (TargetNode->GetSkillNodeState() == ESkillNodeState::Activated)
		{
			Color = FLinearColor::White; // 스킬 획득 시 화이트
		}
		else if (TargetNode->GetSkillNodeState() == ESkillNodeState::Available)
		{
			Color = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f); // 선행 스킬 배운 경우 약간 밝게 조절
		}

		// 좌표 계산 및 그리기
		TArray<FVector2D> Points = MakePointsToDrawLine(AllottedGeometry);

		if (Points.Num() >= 2 && !Points[0].Equals(Points[1], 0.1f))
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				Color,
				true,
				LineThickness);
		}
	}
	return LayerId;
}

TArray<FVector2D> USkillConnectionLine::MakePointsToDrawLine(const FGeometry& AllottedGeometry) const
{
	TArray<FVector2D> Points;

	// 각 노드의 로컬 좌표 중앙값을 구해서 선 잇기
	auto GetNodeCenter = [&](USkillNodeWidget* Node) -> FVector2D
		{
			FVector2D Pos = Node->GetPaintSpaceGeometry().GetAbsolutePosition();
			return AllottedGeometry.GetAccumulatedRenderTransform().Inverse().TransformPoint(Pos) + (Node->GetDesiredSize() * 0.5f);
		};

	Points.Add(GetNodeCenter(SourceNode));
	Points.Add(GetNodeCenter(TargetNode));

	return Points;
}
