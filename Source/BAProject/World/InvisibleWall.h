// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/InvisibleWallBase.h"
#include "InvisibleWall.generated.h"

UCLASS()
class BAPROJECT_API AInvisibleWall : public AInvisibleWallBase
{
	GENERATED_BODY()

public:
	AInvisibleWall();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProximityBegin(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProximityEnd(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// 플레이어가 이 영역에 들어오면 유리 마테리얼이 표시된다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBoxComponent> ProximityBox;

	// BlockingBox 크기에서 각 축으로 추가 확장할 값. 벽 앞쪽(접근 방향) Y축을 늘려 감지 거리 조절.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall|Visual")
	FVector ProximityExpansion = FVector(0.f, 100.f, 0.f);
};
