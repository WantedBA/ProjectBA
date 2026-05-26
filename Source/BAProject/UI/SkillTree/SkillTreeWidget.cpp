// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"
#include "SkillNodeWidget.h"
#include "SkillConnectionLine.h"
#include "Instance/SkillTreeSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"

#include "Tables/BATableManager.h"
#include "Tables/SkillRows.h"

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 화면에 생길 때 델리게이트 등록
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.AddUniqueDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
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

