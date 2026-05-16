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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Elevator")
	void StartElevateToOther();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ElevatorActorRoot;
	// 시작 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> StartPoint;
	// 정지 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EndPoint;
	//이동하는 모든 컴포넌트를 묶는 그룹
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ElevatorBody;
	// 캐릭터 올라타는 플랫폼
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;
	//BP 인스턴스에서 디자이너가 설정할 수 있게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Visual")
	TObjectPtr<UStaticMesh> PlatformMeshAsset;
	// 이동 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Motion",
		meta = (ClampMin = "0.1"))
	float ElevateDuration = 3.0f;

	// 시작 위치 (true=StartPoint, false=EndPoint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|State")
	bool bStartAtStart = true;

	// 자동으로 시작할지 여부 (디버그용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Debug")
	bool bAutoElevateOnBeginPlay = true;

private:
	bool bAtStart = true;
	bool bIsElevating = false;
	float ElevateAlpha = 0.f;
	FVector ElevateStartLoc = FVector::ZeroVector;
	FVector ElevateEndLoc = FVector::ZeroVector;
};
