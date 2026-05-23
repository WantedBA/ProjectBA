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

	
};
