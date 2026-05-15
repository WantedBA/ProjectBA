// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InvisibleWallBase.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BAPROJECT_API AInvisibleWallBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInvisibleWallBase();

	//BP 에서 사용할 차단 Collision의 OnOff 함수
	UFUNCTION(BlueprintCallable, Category = "InvisibleWall")
	void SetWallActive(bool isActive);

	UFUNCTION(BlueprintPure, Category = "InvisibleWall")
	bool IsWallActive() const { return bIsActive; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// WallMesh 크기를 BlockingBox의 BoxExtent에 맞춤 (BlockingBox는 디테일에서 직접 편집)
	virtual void OnConstruction(const FTransform& Transform) override;

	void SetWallOpacity(float Opacity);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	// 시작 시 차단 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall")
	bool bStartActive = true;

	// 비주얼 머티리얼.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall|Visual")
	TObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InvisibleWall|Visual")
	FName OpacityParameterName = TEXT("Opacity");

	//Material 동적 생성용
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WallMID;

private:
	bool bIsActive = true;
};
