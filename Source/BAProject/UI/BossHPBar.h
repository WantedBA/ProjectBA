// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseWidget.h"
#include "BossHPBar.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UBossHPBar : public UBaseWidget
{
	GENERATED_BODY()

protected:
	// 보스의 이름을 표시할 텍스트 (WBP에서 이름 맞추기)
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* BossNameText;

	// 체력 게이지 담당
	UPROPERTY(meta = (BindWidget))
	class UStatusBar* HPBar;

public:
	// 보스의 이름을 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void SetBossName(FText NewName);

	// 보스의 현재 체력을 업데이트할 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void UpdateHP(float CurrentHP, float MaxHP);

	// 보스 등장/퇴장 시 위젯 가시성 제어
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void SetAppearance(bool bVisible);

	// 보스전이 시작될 때 이름과 초기 체력을 설정 및 연출
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void InitializeBossBar(FText Name, float MaxHP);

	// 보스체력바 끄고 데이터 초기화
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void ResetBossBar();
	
};
