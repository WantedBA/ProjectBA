// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapInfoActor.generated.h"

UCLASS()
class BAPROJECT_API AMapInfoActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapInfoActor();

	// 맵의 가로/세로 월드 크기 (에디터에서 직접 입력)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapInfo")
	float MapWorldSize = 10000.0f;

	// 맵 전용 미니맵 이미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapInfo")
	class UTexture2D* MapTexture;

};
