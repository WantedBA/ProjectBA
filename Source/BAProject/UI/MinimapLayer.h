// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/LayerBase.h"
#include "MinimapLayer.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UMinimapLayer : public ULayerBase
{
	GENERATED_BODY()
	
protected:
	// 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	class UImage* MapImage;

	UPROPERTY(meta = (BindWidget))
	class UImage* PlayerMarker;

	// 미니맵 설정 값
	// 실제 월드의 가로/세로 크기
	UPROPERTY(EditAnywhere, Category = "Minimap|Settings")
	float WorldSize = 10000.0f;

	// 미니맵 UI의 실제 픽셀 크기
	UPROPERTY(EditAnywhere, Category = "Minimap|Settings")
	float MinimapSize = 250.0f;

	// 위젯이 생성될 때 호출되는 초기화 함수
	// MapInfoActor를 찾아서 데이터 주입
	virtual void NativeConstruct() override;

	// 현재 맵의 정보를 담고 있는 액터의 참조
	UPROPERTY()
	class AMapInfoActor* StoredMapInfo;

	// 회전용 컨테이너 패널
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* MapContainer;

public:
	// 매 프레임 캐릭터 위치를 체크해서 마커 이동 함수
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
