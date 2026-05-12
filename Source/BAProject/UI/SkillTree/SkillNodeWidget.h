// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instance/SkillTreeTypes.h"
#include "SkillNodeWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillNodeClicked, int32, SkillId);

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Setter
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void SetSkillNodeState(const ESkillNodeState InSkillNodeState)
	{
		this->SkillNodeState = InSkillNodeState;
	}
	
	// Delegate
	UPROPERTY(BlueprintAssignable, Category = SkillTree)
	FOnSkillNodeClicked OnSkillNodeClicked;
	
protected:
	// 재정의 함수
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> SkillNodeButton;
	
// 데이터
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"), Category = SkillTree)
	int32 SkillId = -1;
	
	// 게임 중 스킬 상태
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SkillTree)
	ESkillNodeState SkillNodeState = ESkillNodeState::Locked;
	
private:
	// 브로드캐스팅 -> SkillTree에서 수신
	UFUNCTION()
	void HandleSkillNodeButtonClicked();
};
