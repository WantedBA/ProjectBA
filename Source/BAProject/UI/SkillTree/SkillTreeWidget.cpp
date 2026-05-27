// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"
#include "SkillNodeWidget.h"
#include "SkillConnectionLine.h"
#include "Instance/SkillTreeSubsystem.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"

#include "Tables/BATableManager.h"
#include "Tables/SkillRows.h"

namespace
{
	constexpr float RightStickNavigationThreshold = 0.99f;
	constexpr float RightStickNavigationResetThreshold = 0.5f;
	constexpr float SkillNavigationMinDirectionDot = 0.35f;
	constexpr float SkillNavigationSmallDistance = 1.0f;
	constexpr float GamepadMouseMoveTolerance = 3.0f;

	FKey SkillTreeGenericUSBControllerButton(const int32 ButtonNumber)
	{
		return FKey(FName(*FString::Printf(TEXT("GenericUSBController_Button%d"), ButtonNumber)));
	}

	bool IsSkillTreeAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Enter
			|| Key == EKeys::Virtual_Accept
			|| Key == EKeys::Gamepad_FaceButton_Bottom
			|| Key == SkillTreeGenericUSBControllerButton(1)
			|| Key == SkillTreeGenericUSBControllerButton(2);
	}

	FVector2D NormalizeNavigationVector(const FVector2D& DirectionVector)
	{
		return DirectionVector.GetSafeNormal();
	}
}

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 화면에 생길 때 델리게이트 등록
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.AddUniqueDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
}

void USkillTreeWidget::NativeDestruct()
{
	ClearGamepadSelectedSkillNode();
	Super::NativeDestruct();
	
	// 화면에서 해제될 때 델리게이트 해제
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.RemoveDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
}

FReply USkillTreeWidget::NativeOnAnalogValueChanged(
	const FGeometry& InGeometry,
	const FAnalogInputEvent& InAnalogEvent)
{
	if (!IsRightStickNavigationKey(InAnalogEvent.GetKey()))
	{
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		RightStickNavigationInput.X = InAnalogEvent.GetAnalogValue();
	}
	else if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightY)
	{
		RightStickNavigationInput.Y = InAnalogEvent.GetAnalogValue();
	}

	const float StrongestInput = FMath::Max(FMath::Abs(RightStickNavigationInput.X), FMath::Abs(RightStickNavigationInput.Y));
	if (StrongestInput < RightStickNavigationResetThreshold)
	{
		bRightStickNavigationReady = true;
		return FReply::Handled();
	}

	if (!bRightStickNavigationReady || StrongestInput < RightStickNavigationThreshold)
	{
		return FReply::Handled();
	}

	bRightStickNavigationReady = false;
	return TryNavigateSkillNode(GetRightStickNavigationVector())
		? FReply::Handled()
		: FReply::Handled();
}

FReply USkillTreeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsSkillTreeAcceptKey(InKeyEvent.GetKey()) && ActivateGamepadSelectedSkillNode())
	{
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USkillTreeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bGamepadNavigationActive
		&& FVector2D::Distance(InMouseEvent.GetScreenSpacePosition(), LastGamepadCursorAbsolute) > GamepadMouseMoveTolerance)
	{
		bGamepadNavigationActive = false;
		ClearGamepadSelectedSkillNode();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
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
}

bool USkillTreeWidget::TryNavigateSkillNode(const FVector2D& DirectionVector)
{
	const int32 CurrentSkillId = SkillNodeMap.Contains(GamepadSelectedSkillId)
		? GamepadSelectedSkillId
		: ResolveInitialGamepadSkillNodeId();
	if (!SkillNodeMap.Contains(CurrentSkillId))
	{
		return false;
	}

	const int32 TargetSkillId = FindBestSkillNodeInDirection(CurrentSkillId, DirectionVector);
	if (!SkillNodeMap.Contains(TargetSkillId))
	{
		return SelectSkillNodeByGamepad(CurrentSkillId);
	}

	return SelectSkillNodeByGamepad(TargetSkillId);
}

bool USkillTreeWidget::SelectSkillNodeByGamepad(const int32 SkillId)
{
	TObjectPtr<USkillNodeWidget>* NodePtr = SkillNodeMap.Find(SkillId);
	USkillNodeWidget* SkillNode = NodePtr ? NodePtr->Get() : nullptr;
	if (!SkillNode)
	{
		return false;
	}

	if (GamepadSelectedSkillId != SkillId)
	{
		ClearGamepadSelectedSkillNode();
	}

	GamepadSelectedSkillId = SkillId;
	bGamepadNavigationActive = true;
	MoveMouseToSkillNode(SkillId);
	SkillNode->SetGamepadHoverActive(true);

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SetUserFocus(OwningPlayer);
	}
	SetKeyboardFocus();

	return true;
}

bool USkillTreeWidget::ActivateGamepadSelectedSkillNode()
{
	if (!SkillNodeMap.Contains(GamepadSelectedSkillId))
	{
		const int32 InitialSkillId = ResolveInitialGamepadSkillNodeId();
		if (!SelectSkillNodeByGamepad(InitialSkillId))
		{
			return false;
		}
	}

	TObjectPtr<USkillNodeWidget>* NodePtr = SkillNodeMap.Find(GamepadSelectedSkillId);
	USkillNodeWidget* SkillNode = NodePtr ? NodePtr->Get() : nullptr;
	if (!SkillNode)
	{
		return false;
	}

	MoveMouseToSkillNode(GamepadSelectedSkillId);
	SkillNode->ActivateSkillNodeButtonByGamepad();
	return true;
}

int32 USkillTreeWidget::ResolveInitialGamepadSkillNodeId() const
{
	if (SkillNodeMap.Contains(GamepadSelectedSkillId))
	{
		return GamepadSelectedSkillId;
	}

	const int32 MouseSkillId = FindSkillNodeUnderMouse();
	if (SkillNodeMap.Contains(MouseSkillId))
	{
		return MouseSkillId;
	}

	for (const TPair<int32, TObjectPtr<USkillNodeWidget>>& NodePair : SkillNodeMap)
	{
		if (NodePair.Value)
		{
			return NodePair.Key;
		}
	}

	return INDEX_NONE;
}

int32 USkillTreeWidget::FindSkillNodeUnderMouse() const
{
	const APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return INDEX_NONE;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!OwningPlayer->GetMousePosition(MouseX, MouseY))
	{
		return INDEX_NONE;
	}

	FVector2D MousePosition(MouseX, MouseY);
	for (const TPair<int32, TObjectPtr<USkillNodeWidget>>& NodePair : SkillNodeMap)
	{
		const USkillNodeWidget* SkillNode = NodePair.Value.Get();
		const UButton* SkillButton = SkillNode ? SkillNode->GetSkillNodeButton() : nullptr;
		if (!SkillButton)
		{
			continue;
		}

		FVector2D PixelPosition;
		FVector2D ViewportPosition;
		FVector2D BottomRightPixelPosition;
		FVector2D BottomRightViewportPosition;
		USlateBlueprintLibrary::LocalToViewport(
			const_cast<USkillTreeWidget*>(this),
			SkillButton->GetCachedGeometry(),
			FVector2D::ZeroVector,
			PixelPosition,
			ViewportPosition);
		USlateBlueprintLibrary::LocalToViewport(
			const_cast<USkillTreeWidget*>(this),
			SkillButton->GetCachedGeometry(),
			SkillButton->GetCachedGeometry().GetLocalSize(),
			BottomRightPixelPosition,
			BottomRightViewportPosition);
		if (MousePosition.X >= PixelPosition.X
			&& MousePosition.X <= BottomRightPixelPosition.X
			&& MousePosition.Y >= PixelPosition.Y
			&& MousePosition.Y <= BottomRightPixelPosition.Y)
		{
			return NodePair.Key;
		}
	}

	return INDEX_NONE;
}

int32 USkillTreeWidget::FindBestSkillNodeInDirection(const int32 SkillId, const FVector2D& DirectionVector) const
{
	const FVector2D NormalizedDirection = NormalizeNavigationVector(DirectionVector);
	if (NormalizedDirection.IsNearlyZero())
	{
		return INDEX_NONE;
	}

	const FVector2D SourcePosition = GetSkillNodeCenterAbsolute(SkillId);
	const TArray<int32> ConnectedIds = GetConnectedSkillNodeIds(SkillId);

	int32 BestSkillId = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();

	for (const int32 CandidateId : ConnectedIds)
	{
		const FVector2D CandidatePosition = GetSkillNodeCenterAbsolute(CandidateId);
		const FVector2D Delta = CandidatePosition - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= SkillNavigationSmallDistance)
		{
			continue;
		}

		const float DirectionDot = FVector2D::DotProduct(Delta.GetSafeNormal(), NormalizedDirection);
		if (DirectionDot < SkillNavigationMinDirectionDot)
		{
			continue;
		}

		const float Score = DistanceSquared * (2.0f - DirectionDot);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestSkillId = CandidateId;
		}
	}

	return BestSkillId != INDEX_NONE ? BestSkillId : FindAdjacentSkillNodeInDirection(SkillId, DirectionVector);
}

int32 USkillTreeWidget::FindAdjacentSkillNodeInDirection(const int32 SkillId, const FVector2D& DirectionVector) const
{
	const FVector2D NormalizedDirection = NormalizeNavigationVector(DirectionVector);
	if (NormalizedDirection.IsNearlyZero())
	{
		return INDEX_NONE;
	}

	const FVector2D SourcePosition = GetSkillNodeCenterAbsolute(SkillId);

	int32 BestSkillId = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();

	for (const TPair<int32, TObjectPtr<USkillNodeWidget>>& NodePair : SkillNodeMap)
	{
		if (NodePair.Key == SkillId || !NodePair.Value)
		{
			continue;
		}

		const FVector2D CandidatePosition = GetSkillNodeCenterAbsolute(NodePair.Key);
		const FVector2D Delta = CandidatePosition - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= SkillNavigationSmallDistance)
		{
			continue;
		}

		const float DirectionDot = FVector2D::DotProduct(Delta.GetSafeNormal(), NormalizedDirection);
		if (DirectionDot < SkillNavigationMinDirectionDot)
		{
			continue;
		}

		const float Score = DistanceSquared * (2.0f - DirectionDot);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestSkillId = NodePair.Key;
		}
	}

	return BestSkillId;
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

FVector2D USkillTreeWidget::GetSkillNodeCenterAbsolute(const int32 SkillId) const
{
	const TObjectPtr<USkillNodeWidget>* NodePtr = SkillNodeMap.Find(SkillId);
	const USkillNodeWidget* SkillNode = NodePtr ? NodePtr->Get() : nullptr;
	const UButton* SkillButton = SkillNode ? SkillNode->GetSkillNodeButton() : nullptr;
	if (!SkillButton)
	{
		return FVector2D::ZeroVector;
	}

	const FGeometry& Geometry = SkillButton->GetCachedGeometry();
	return Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
}

void USkillTreeWidget::MoveMouseToSkillNode(const int32 SkillId)
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	LastGamepadCursorAbsolute = GetSkillNodeCenterAbsolute(SkillId);
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(this, LastGamepadCursorAbsolute, PixelPosition, ViewportPosition);
	OwningPlayer->SetMouseLocation(FMath::RoundToInt(PixelPosition.X), FMath::RoundToInt(PixelPosition.Y));
}

void USkillTreeWidget::ClearGamepadSelectedSkillNode()
{
	TObjectPtr<USkillNodeWidget>* NodePtr = SkillNodeMap.Find(GamepadSelectedSkillId);
	if (USkillNodeWidget* SkillNode = NodePtr ? NodePtr->Get() : nullptr)
	{
		SkillNode->SetGamepadHoverActive(false);
	}
	GamepadSelectedSkillId = INDEX_NONE;
}

bool USkillTreeWidget::IsRightStickNavigationKey(const FKey& Key) const
{
	return Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY;
}

FVector2D USkillTreeWidget::GetRightStickNavigationVector() const
{
	return FVector2D(RightStickNavigationInput.X, -RightStickNavigationInput.Y).GetSafeNormal();
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

