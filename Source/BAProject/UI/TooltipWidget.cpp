// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/MultiLineEditableText.h"
#include "Components/Image.h"
#include "TimerManager.h"
#include "Engine/AssetManager.h"
#include "Tables/BATableManager.h" // 데이터 테이블 조회용
#include "Tables/SkillRows.h" // 스킬 데이터
#include "Tables/Text.h" // 텍스트 데이터 
#include "MediaPlayer.h" // 영상 재생용
#include "Materials/MaterialInterface.h"
#include "FileMediaSource.h"

void UTooltipWidget::RequestShowTooltip(int32 InTid)
{
	// 새로운 요청 실행 시 기존에 돌고 있던 로딩 및 타이머를 즉시 취소
	HideTooltip();

	// 유예 시간 적용(빠르게 스쳐 지나가는 정도에서는 로딩 방지)
	GetWorld()->GetTimerManager().SetTimer(HoverDelayTimerHandle, [this, InTid]() {ProcessLoadData(InTid); }, 0.15f, false);
}

void UTooltipWidget::HideTooltip()
{
	// 모든 대기열과 로딩 상태를 초기화하고 위젯 숨기기
	GetWorld()->GetTimerManager().ClearTimer(HoverDelayTimerHandle);

	// 진행 중인 비동기 로딩이 있다면 즉시 취소하여 메모리 및 성능 낭비 방지
	if (AsyncLoadHandle.IsValid() && AsyncLoadHandle->IsActive())
	{
		// 현재 진행 중인 비동기 파일 로딩 작업을 즉시 중단
		AsyncLoadHandle->CancelHandle();
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UTooltipWidget::ProcessLoadData(int32 InTid)
{
	// 데이터 테이블에서 정보를 읽어옴
	UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return;
	}

	// 1. 스킬 테이블에서 정보 찾기
	const FSkillRow* SkillData = TableManager->FindSkill(InTid);
	if (!SkillData)
	{
		return;
	}

	// 2. 스킬 이름 설정 
	const FTextRows* NameData = TableManager->FindText(SkillData->SkillNameTid);
	if (NameData)
	{
		TitleText->SetText(FText::FromString(NameData->KoreanText));
	}

	// 3. 스킬 설명 설정
	const FTextRows* DescData = TableManager->FindText(SkillData->TextTid);
	if (DescData)
	{
		DescriptionText->SetText(FText::FromString(DescData->KoreanText));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Tooltip: FAILED to find Text for ID: %d"), SkillData->TextTid);
	}


	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 에셋(이미지/영상) 비동기 로딩 시작
	if (SkillData->IconTexture.IsPending())
	{
		// 비동기 로딩 시스템에 전달하기 위해 로딩할 파일 목록 배열 생성
		TArray<FSoftObjectPath> AssetsToLoad;

		// 배열 위에서 구한 에셋의 실제 데이터 경로
		AssetsToLoad.Add(SkillData->IconTexture.ToSoftObjectPath());

		// 비동기 로드 요청 및 완료 시 OnAssetLoadCompleted 호출 예약
		AsyncLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			AssetsToLoad,
			FStreamableDelegate::CreateUObject(this, &UTooltipWidget::OnAssetLoadCompleted, InTid));
	}
	else
	{
		// 이미 로드되어 있다면 즉시 실행
		OnAssetLoadCompleted(InTid);
	}

}

void UTooltipWidget::OnAssetLoadCompleted(int32 InTid)
{

	UE_LOG(LogTemp, Error, TEXT("!!! Tooltip: OnAssetLoadCompleted CALLED for ID: %d"), InTid);

	// 로딩 중에 마우스를 뗐다면 처리 중단(Race Condition 방지)
	if (GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}

	UBATableManager* TableManager = UBATableManager::Get(this);
	const FSkillRow* SkillData = TableManager->FindSkill(InTid);

	if (SkillData && PreviewVideoImage)
	{
		// 영상 소스가 있는지 확인
		if (TooltipMediaPlayer && SkillData->VideoSource.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Tooltip: Found Video Source, Opening..."));
			// 이전에 아이콘이 출력되었다면 다시 영상 머티리얼 브러시를 복구
			if (VideoMaterial)
			{
				PreviewVideoImage->SetBrushFromMaterial(VideoMaterial);
			}

			TooltipMediaPlayer->Close();

			// 영상 재생
			if (TooltipMediaPlayer->OpenSource(SkillData->VideoSource.Get()))
			{
				// 열기 성공시 재생
				TooltipMediaPlayer->Rewind();
				TooltipMediaPlayer->Play();
				
				UE_LOG(LogTemp, Warning, TEXT("Tooltip: Now Opening Video Source..."));
			}
		}
		else
		{
			// 영상이 없다면 텍스처(아이콘)로 브러시를 교체
			UE_LOG(LogTemp, Error, TEXT("Tooltip: Video Source is NOT Valid! (Icon fallback)"));
			UTexture2D* SkillIcon = SkillData->IconTexture.Get();
			if (SkillIcon)
			{
				PreviewVideoImage->SetBrushFromTexture(SkillIcon);
			}
		}

		const FTextRows* NameData = TableManager->FindText(SkillData->SkillNameTid);
		const FTextRows* DescData = TableManager->FindText(SkillData->TextTid);
		if (NameData)
		{
			TitleText->SetText(FText::FromString(NameData->KoreanText));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Tooltip: Still CANNOT find Name for Tid: %d"), SkillData->SkillNameTid);
		}
		if (DescData)
		{
			DescriptionText->SetText(FText::FromString(DescData->KoreanText));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Tooltip: Still CANNOT find Description for Tid: %d"),SkillData->TextTid);
		}
	}
}
