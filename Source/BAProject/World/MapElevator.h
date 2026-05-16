// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapElevator.generated.h"

UENUM(BlueprintType)
enum class EElevatorState : uint8
{
	Idle,
	Aligning,
	ClosingDoor,
	Moving,
	OpeningDoor
};

UCLASS()
class BAPROJECT_API AMapElevator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapElevator();
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ElevatorActorRoot;
	// 시작 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> StartPoint;
	// 정지 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EndPoint;
	// 캐릭터 올라타는 플랫폼
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;
	//BP 인스턴스에서 디자이너가 설정할 수 있게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Visual")
	TObjectPtr<UStaticMesh> PlatformMeshAsset;
};
