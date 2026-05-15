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
	virtual void Tick(float DeltaTime) override;

	UFUNCTION() // 차단 영역 근접시 이벤트 처리 함수
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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USphereComponent> ProximitySphere;

	// 이 거리부터 Material이 점점 보이게 됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall|Visual")
	float FadeStartDistance = 100.f;

	// 이 거리 이하에서 Material이 완전히 보임
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall|Visual")
	float FadeEndDistance = 20.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> TrackedActor;
};
