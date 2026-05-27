// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillNodeWidget.h"
#include "UI/System/PopupBase.h"
#include "SkillTreeWidget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillTreeWidget : public UPopupBase
{
	GENERATED_BODY()
	
protected:
// 재정의 함수
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
public:
	virtual void OnPopped() override;

	UFUNCTION(BlueprintCallable, Category = "BA|SkillTree")
	void CreateSkillLines();
	
protected:
// 델리게이트로 실행되는 함수
	// SkillNodeWidget
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	void HandleSkillNodeClicked(int32 SkillId);
	
	// SkillTreeSubsystem
	UFUNCTION()
	void HandleSkillNodeStateChanged(int32 SkillId, ESkillNodeState NewState);
	
// 블루프린트에서 사용할 함수
	// 스킬 노드 상태값 갱신
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	void RefreshAllSkillNodeState();

	bool TryNavigateSkillNode(const FVector2D& DirectionVector);
	bool SelectSkillNodeByGamepad(int32 SkillId);
	bool ActivateGamepadSelectedSkillNode();
	int32 ResolveInitialGamepadSkillNodeId() const;
	int32 FindSkillNodeUnderMouse() const;
	int32 FindBestSkillNodeInDirection(int32 SkillId, const FVector2D& DirectionVector) const;
	int32 FindAdjacentSkillNodeInDirection(int32 SkillId, const FVector2D& DirectionVector) const;
	TArray<int32> GetConnectedSkillNodeIds(int32 SkillId) const;
	FVector2D GetSkillNodeCenterAbsolute(int32 SkillId) const;
	void MoveMouseToSkillNode(int32 SkillId);
	void ClearGamepadSelectedSkillNode();
	bool IsRightStickNavigationKey(const FKey& Key) const;
	FVector2D GetRightStickNavigationVector() const;
	
	
// 데이터
	// Table Manager에서 스킬 id 목록을 가져오는 함수
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	TArray<int32> GetSkillTids() const;
	
	// 스킬 Tid를 키 값으로 하는 스킬 노드 맵
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TMap<int32, TObjectPtr<USkillNodeWidget>> SkillNodeMap;

protected:
	// 생성된 모든 선 위젯 담아둘 배열
	UPROPERTY()
	TArray<TObjectPtr<class USkillConnectionLine>> SkillLines;

	// 선을 만들 때 쓸 블루프린트 클래스
	UPROPERTY(EditAnywhere, Category = "BA|SkillTree")
	TSubclassOf<class USkillConnectionLine> SkillLineClass;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* LineCanvas;

	int32 GamepadSelectedSkillId = INDEX_NONE;
	FVector2D RightStickNavigationInput = FVector2D::ZeroVector;
	FVector2D LastGamepadCursorAbsolute = FVector2D::ZeroVector;
	bool bRightStickNavigationReady = true;
	bool bGamepadNavigationActive = false;
	
};
