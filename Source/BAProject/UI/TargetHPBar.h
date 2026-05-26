// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseWidget.h"
#include "TargetHPBar.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UTargetHPBar : public UBaseWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "BA|UI")
	class UStatusBar* TargetHPBar;


	// 연결된 몬스터 값 저장
	UPROPERTY()
	class UStatComponent* CurrentBoundStat;

	// 타이머 핸들
	FTimerHandle HideTimerHandle;

protected:
	// 몬스터의 component 신호 받을 함수
	UFUNCTION()
	void HandleHPChanged(float CurrentHP, float MaxHP);

	// 위젯 생성 시 몬스터와 연결
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	// 락온 대상 HP 갱신
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void UpdateHP(float CurrentHP, float MaxHP);

	// 체력바 숨기기 처리
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void StartHideTimer();

	// 락온 시 호출 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void OnTargetCaptured();

	// 락온 해제 시 호출 함
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void OnTargetReleased();

	// 재 락온 시 타이머 취소
	void CancelHideTimer();

private:
	void BindToTarget(AActor* TargetActor);
	void UnbindCurrentTarget();

	// 위젯 숨기기 함수
	void HideWidget();
	
};
