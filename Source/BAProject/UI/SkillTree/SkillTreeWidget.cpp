// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"
#include "SkillNodeWidget.h"
#include "SkillConnectionLine.h"
#include "Instance/SkillTreeSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"

#include "Tables/BATableManager.h"
#include "Tables/SkillRows.h"

namespace
{
	constexpr float SkillNavigationMinDirectionDot = 0.35f;
	constexpr float SkillNavigationSmallDistance = 1.0f;

	constexpr EUINavigation SkillNavigationDirections[] =
	{
		EUINavigation::Up,
		EUINavigation::Down,
		EUINavigation::Left,
		EUINavigation::Right,
	};

	FVector2D GetNavigationDirectionVector(const EUINavigation Direction)
	{
		switch (Direction)
		{
		case EUINavigation::Up:
			return FVector2D(0.0f, -1.0f);
		case EUINavigation::Down:
			return FVector2D(0.0f, 1.0f);
		case EUINavigation::Left:
			return FVector2D(-1.0f, 0.0f);
		case EUINavigation::Right:
			return FVector2D(1.0f, 0.0f);
		default:
			return FVector2D::ZeroVector;
		}
	}
}

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 화면에 생길 때 델리게이트 등록
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.AddUniqueDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);

	ConfigureSkillNodeNavigation();
}

void USkillTreeWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	// 화면에서 해제될 때 델리게이트 해제
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.RemoveDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
}

void USkillTreeWidget::OnPopped()
{
	Super::OnPopped();

	// 스킬트리가 닫힐 때 툴팁이 뜬 상태라면 강제로 숨김
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USubSystemUI* UISubsystem = GI->GetSubsystem<USubSystemUI>())
		{
			if (UMainHUD* MainHUD = UISubsystem->GetMainHUD())
			{
				MainHUD->HideTooltip();
			}
		}
	}
	
	// 팝업이 닫힐 때, 스킬트리 서브시스템에서 스킬트리 변경사항 브로드캐스팅
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillTreeChangeCompleted.Broadcast();
}

void USkillTreeWidget::CreateSkillLines()
{
	// 로그1
	UE_LOG(LogTemp, Error, TEXT("!!! CreateSkillLines: NodeMap Count is %d !!!"), SkillNodeMap.Num());

	// 기존 선이 있다면 전부 지우기(초기화)
	for (auto Line : SkillLines)
	{
		if (Line)
		{
			Line->RemoveFromParent();
			SkillLines.Empty();
		}
	}

	if (!SkillLineClass)
	{
		//로그2
		UE_LOG(LogTemp, Error, TEXT("!!! CreateSkillLines: SkillLineClass is NULL !!!"));
		return;
	}

	// 모든 노드를 돌면서 부모 노드 찾기
	for (auto& NodePair : SkillNodeMap)
	{
		int32 ChildId = NodePair.Key;
		USkillNodeWidget* ChildNode = NodePair.Value;

		// 엑셀에서 자식 스킬의 부모 ID 리스트 가져오기
		const FSkillRow* SkillRow = UBATableManager::Get(this)->FindSkill(ChildId);
		if (SkillRow)
		{
			for (int32 ParentId : SkillRow->PrerequisiteIds)
			{
				// 부모 노드가 맵에 존재하는지 확인
				if (SkillNodeMap.Contains(ParentId))
				{
					USkillNodeWidget* ParentNode = SkillNodeMap[ParentId];

					USkillConnectionLine* NewLine = CreateWidget<USkillConnectionLine>(GetWorld(), SkillLineClass);
					if (NewLine)
					{
						// 부모 자식 연결
						NewLine->SetSkillNodes(ParentNode, ChildNode);

						if(LineCanvas)
						{
							UCanvasPanelSlot* LineSlot = LineCanvas->AddChildToCanvas(NewLine);
							if (LineSlot)
							{
								LineSlot->SetZOrder(0);
								LineSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
								LineSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
							}
						

						//// 화면에 루트 캔버스 추가 (Order 조절)
						//if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget()))
						//{
						//	UCanvasPanelSlot* LineSlot = RootCanvas->AddChildToCanvas(NewLine);
						//	if (LineSlot)
						//	{
						//		LineSlot->SetZOrder(1);
						//		LineSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
						//		LineSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
						//	}
						}
						// 관리 리스트에 추가
						SkillLines.Add(NewLine);
					}
				}
			}
		}
	}

	ConfigureSkillNodeNavigation();
}

void USkillTreeWidget::ConfigureSkillNodeNavigation()
{
	for (const TPair<int32, TObjectPtr<USkillNodeWidget>>& NodePair : SkillNodeMap)
	{
		USkillNodeWidget* SourceNode = NodePair.Value;
		if (!SourceNode)
		{
			continue;
		}

		UButton* SourceButton = SourceNode->GetSkillNodeButton();
		if (!SourceButton)
		{
			continue;
		}

		for (const EUINavigation Direction : SkillNavigationDirections)
		{
			const int32 TargetSkillId = FindBestSkillNodeInDirection(NodePair.Key, Direction);
			if (TargetSkillId == INDEX_NONE)
			{
				SourceButton->SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
				continue;
			}

			TObjectPtr<USkillNodeWidget>* TargetNodePtr = SkillNodeMap.Find(TargetSkillId);
			UButton* TargetButton = TargetNodePtr && TargetNodePtr->Get() ? TargetNodePtr->Get()->GetSkillNodeButton() : nullptr;
			if (!TargetButton)
			{
				SourceButton->SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
				continue;
			}

			SourceButton->SetNavigationRuleExplicit(Direction, TargetButton);
		}
	}
}

TArray<int32> USkillTreeWidget::GetConnectedSkillNodeIds(const int32 SkillId) const
{
	TArray<int32> ConnectedIds;
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return ConnectedIds;
	}

	if (const FSkillRow* SkillRow = TableManager->FindSkill(SkillId))
	{
		for (const int32 ParentId : SkillRow->PrerequisiteIds)
		{
			if (SkillNodeMap.Contains(ParentId))
			{
				ConnectedIds.AddUnique(ParentId);
			}
		}

		for (const int32 ChildId : SkillRow->ChildIds)
		{
			if (SkillNodeMap.Contains(ChildId))
			{
				ConnectedIds.AddUnique(ChildId);
			}
		}
	}

	// ChildIds가 아직 빌드되지 않은 경우에도 연결된 자식 노드를 찾는다.
	for (const TPair<int32, FSkillRow*>& SkillPair : TableManager->GetSkillMap())
	{
		const FSkillRow* CandidateRow = SkillPair.Value;
		if (!CandidateRow || SkillPair.Key == SkillId || !SkillNodeMap.Contains(SkillPair.Key))
		{
			continue;
		}

		if (CandidateRow->PrerequisiteIds.Contains(SkillId))
		{
			ConnectedIds.AddUnique(SkillPair.Key);
		}
	}

	return ConnectedIds;
}

int32 USkillTreeWidget::FindBestSkillNodeInDirection(const int32 SkillId, const EUINavigation Direction) const
{
	const FVector2D DirectionVector = GetNavigationDirectionVector(Direction);
	if (DirectionVector.IsNearlyZero())
	{
		return INDEX_NONE;
	}

	const FVector2D SourcePosition = GetSkillNodePosition(SkillId);
	const TArray<int32> ConnectedIds = GetConnectedSkillNodeIds(SkillId);

	int32 BestSkillId = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();

	for (const int32 CandidateId : ConnectedIds)
	{
		const FVector2D CandidatePosition = GetSkillNodePosition(CandidateId);
		const FVector2D Delta = CandidatePosition - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= SkillNavigationSmallDistance)
		{
			continue;
		}

		const FVector2D CandidateDirection = Delta.GetSafeNormal();
		const float DirectionDot = FVector2D::DotProduct(CandidateDirection, DirectionVector);
		if (DirectionDot < SkillNavigationMinDirectionDot)
		{
			continue;
		}

		const float DirectionPenalty = 1.0f - DirectionDot;
		const float Score = DistanceSquared * (1.0f + DirectionPenalty);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestSkillId = CandidateId;
		}
	}

	return BestSkillId != INDEX_NONE ? BestSkillId : FindAdjacentSkillNodeInDirection(SkillId, Direction);
}

int32 USkillTreeWidget::FindAdjacentSkillNodeInDirection(const int32 SkillId, const EUINavigation Direction) const
{
	const FVector2D DirectionVector = GetNavigationDirectionVector(Direction);
	if (DirectionVector.IsNearlyZero())
	{
		return INDEX_NONE;
	}

	const FVector2D SourcePosition = GetSkillNodePosition(SkillId);

	int32 BestSkillId = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();

	for (const TPair<int32, TObjectPtr<USkillNodeWidget>>& NodePair : SkillNodeMap)
	{
		if (NodePair.Key == SkillId || !NodePair.Value)
		{
			continue;
		}

		const FVector2D CandidatePosition = GetSkillNodePosition(NodePair.Key);
		const FVector2D Delta = CandidatePosition - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= SkillNavigationSmallDistance)
		{
			continue;
		}

		const FVector2D CandidateDirection = Delta.GetSafeNormal();
		const float DirectionDot = FVector2D::DotProduct(CandidateDirection, DirectionVector);
		if (DirectionDot < SkillNavigationMinDirectionDot)
		{
			continue;
		}

		const float DirectionPenalty = 1.0f - DirectionDot;
		const float Score = DistanceSquared * (1.0f + DirectionPenalty);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestSkillId = NodePair.Key;
		}
	}

	return BestSkillId;
}

FVector2D USkillTreeWidget::GetSkillNodePosition(const int32 SkillId) const
{
	if (const UBATableManager* TableManager = UBATableManager::Get(this))
	{
		if (const FSkillRow* SkillRow = TableManager->FindSkill(SkillId))
		{
			return FVector2D(SkillRow->PositionX, SkillRow->PositionY);
		}
	}

	if (const TObjectPtr<USkillNodeWidget>* NodePtr = SkillNodeMap.Find(SkillId))
	{
		if (const USkillNodeWidget* Node = NodePtr->Get())
		{
			const FGeometry& Geometry = Node->GetCachedGeometry();
			return Geometry.GetAbsolutePosition() + Geometry.GetLocalSize() * 0.5f;
		}
	}

	return FVector2D::ZeroVector;
}

void USkillTreeWidget::HandleSkillNodeStateChanged(int32 SkillId, ESkillNodeState NewState)
{
	SkillNodeMap[SkillId]->SetSkillNodeState(NewState);
}

void USkillTreeWidget::RefreshAllSkillNodeState()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("USkillTreeWidget::RefreshAllSkillNodeState - GameInstance is not found"));
		return;
	}
	
	const USkillTreeSubsystem* SkillTreeSubsystem = GameInstance->GetSubsystem<USkillTreeSubsystem>();

	if (!SkillTreeSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("USkillTreeWidget::RefreshAllSkillNodeState - SkillTreeSubsystem is not found"));
		return;
	}
	
	for (auto& SkillNodePair : SkillNodeMap)
	{
		SkillNodePair.Value->SetSkillNodeState(SkillTreeSubsystem->CalculateSkillNodeState(SkillNodePair.Key));
	}
}

TArray<int32> USkillTreeWidget::GetSkillTids() const
{
	// 스킬 키 배열 생성
	TArray<int32> SkillTids;
	UBATableManager::Get(this)->GetSkillMap().GenerateKeyArray(SkillTids);
	return SkillTids;
}


void USkillTreeWidget::HandleSkillNodeClicked(int32 SkillId)
{
	// SkillTreeSubsystem으로 스킬Id 전달
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->TryToggleSkill(SkillId);
}

