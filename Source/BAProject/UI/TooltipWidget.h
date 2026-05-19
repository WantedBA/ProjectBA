// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseWidget.h"
#include "Engine/StreamableManager.h" // 비동기 로딩용
#include "TooltipWidget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UTooltipWidget : public UBaseWidget
{
	GENERATED_BODY()
	
protected:
	// 스킬, 아이템 이름을 표시할 텍스트
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TitleText;

	// 설명 표시 텍스트
	UPROPERTY(meta = (BindWidget))
	class UMultiLineEditableText* DescriptionText;

	// 미리보기 영상을 출력할 이미지 위젯
	UPROPERTY(meta = (BindWidget))
	class UImage* PreviewVideoImage;

	// 영상을 제어할 미디어 플레이어 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BA|UI|Tooltip")
	class UMediaPlayer* TooltipMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BA|UI|Tooltip")
	class UMediaSource* PreviewVideoSource;

	// 비동기 로딩을 관리 핸들
	TSharedPtr<FStreamableHandle> AsyncLoadHandle;

	// 마우스 오버 시 즉시 로딩을 방지하기 위한 타이머 핸들
	FTimerHandle HoverDelayTimerHandle;

public:
	// 데이터 테이블의 ID를 받아 툴팁 정보 로드
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Tooltip")
	void RequestShowTooltip(int32 InTid);

	// 툴팁을 숨기고 진행 중인 모든 비동기 로직 중단
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Tooltip")
	void HideTooltip();

private:
	// 실제 데이터를 로드하고 화면에 표시
	void ProcessLoadData(int32 InTid);

	// 비동기 데이터(이미지/영상) 로딩이 완료되었을 때 호출될 콜백
	void OnAssetLoadCompleted(int32 InTid);
};

