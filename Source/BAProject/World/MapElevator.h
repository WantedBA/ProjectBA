// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapElevator.generated.h"

class USCeneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class ABAPlayerCharacter;

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
	UFUNCTION()
	void OnEnterTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	void StartAlign(ABAPlayerCharacter* Player);

	void TickAlign(float DeltaTime);
	void TickMoving(float DeltaTime);

	void SetState(EElevatorState NewState);

	void TickDoor(float DeltaTime, bool bClosing);

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

	// 진입 감지 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> EnterTrigger;

	// 캐릭터 자동정렬 목적지점
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CenterPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> StartDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StartDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EndDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EndDoorMesh;

	//BP 인스턴스에서 디자이너가 설정할 수 있게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Visual")
	TObjectPtr<UStaticMesh> PlatformMeshAsset;

	// 이동 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Motion",
		meta = (ClampMin = "0.1"))
	float ElevateDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Visual")
	TObjectPtr<UStaticMesh> DoorMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Door")
	FRotator DoorOpenRotation = FRotator(0.f, 90.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Door",
		meta = (ClampMin = "0.05"))
	float DoorDuration = 0.8f;

	// 캐릭터 정렬 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Motion",
		meta = (ClampMin = "0.05"))
	float AlignDuration = 1.2f;

	// 시작 위치 (true=StartPoint, false=EndPoint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|State")
	bool bStartAtStart = true;

	// 자동으로 시작할지 여부 (디버그용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator|Debug")
	bool bAutoElevateOnBeginPlay = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Elevator",
		meta = (AllowPrivateAccess = "true"))
	EElevatorState State = EElevatorState::Idle;

private:
	bool bAtStart = true;
	//현재 타고 있는 플레이어
	TWeakObjectPtr<ABAPlayerCharacter> RidingPlayer;

	// 보간 Alpha
	float Alpha = 0.f;

	// 정렬 보간 시작/끝
	FVector AlignStartLoc = FVector::ZeroVector;
	FVector AlignEndLoc = FVector::ZeroVector;

	FVector ElevateStartLoc = FVector::ZeroVector;
	FVector ElevateEndLoc = FVector::ZeroVector;
};
