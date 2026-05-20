// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MinimapLayer.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h" // 액터 검색용
#include "Map/MapInfoActor.h"		// 맵 정보 액터

void UMinimapLayer::NativeConstruct()
{
	Super::NativeConstruct();

	// 월드에 배치된 MapInfoActor 찾아서 저장
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMapInfoActor::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		StoredMapInfo = Cast<AMapInfoActor>(FoundActors[0]);
		if (StoredMapInfo)
		{
			// [데이터 주입] 맵에 작성된 정보 사용
			WorldSize = StoredMapInfo->MapWorldSize;

			// [이미지 자동 변경] 맵 전용 지도로 교체
			if (MapImage && StoredMapInfo->MapTexture)
			{
				MapImage->SetBrushFromTexture(StoredMapInfo->MapTexture);
			}
		}
	}
}

void UMinimapLayer::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (MinimapFrame && FrameTexture)
	{
		MinimapFrame->SetBrushFromTexture(FrameTexture);
	}
}

void UMinimapLayer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 플레이어 캐릭터와 위치 정보 가져오기
	APawn* PlayerPawn = GetOwningPlayerPawn();
	
	// 예외 처리
	if (!PlayerPawn || !PlayerMarker || !MapImage || !StoredMapInfo)
	{
		return;
	}

	// 데이터 가져오기 (플레이어 위치, 맵 중심점)
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector MapCenterLocation = StoredMapInfo->GetActorLocation();

	// 맵 중심점으로부터 상대 거리 계산
	FVector RelativeLocation = PlayerLocation - MapCenterLocation;

	// 위젯 이동 (지도 이미지 이동)
	if (UCanvasPanelSlot* MapSlot = Cast<UCanvasPanelSlot>(MapImage->Slot))
	{
		// 에디터에서 설정한 실제 이미지 큭ㅣ를 기준으로 비율 계산
		float ActualImageSize = MapSlot->GetSize().X;

		// 월드 이동 비율을 이미지 크기에 반영
		float MapX = -(RelativeLocation.Y / WorldSize) * ActualImageSize;
		float MapY = (RelativeLocation.X / WorldSize) * ActualImageSize;

		MapSlot->SetPosition(FVector2D(MapX, MapY));

		// [회전] MapContainer 자체를 회전값만큼 돌림
		// 컨테이너가 현위치를 기준으로 회전
		float PlayerYaw = PlayerPawn->GetActorRotation().Yaw;
		MapContainer->SetRenderTransformAngle(-PlayerYaw);
	}

	// [마커 설정] 마커는 항상 위를 향하도록 고정
	PlayerMarker->SetRenderTransformAngle(-90.f);

	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot))
	{
		MarkerSlot->SetPosition(FVector2D::ZeroVector);
	}
}
