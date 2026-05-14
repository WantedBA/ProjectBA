#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "PatrolPath.generated.h"

/**
 * 디자이너가 레벨에서 편집할 수 있는 순찰 경로 액터
 */
UCLASS()
class BAPROJECT_API APatrolPath : public AActor
{
	GENERATED_BODY()
	
public:	
	APatrolPath();

	FORCEINLINE USplineComponent* GetSplineComponent() const { return SplineComponent; }

	UFUNCTION(CallInEditor, Category = "Path")
	void PrintPathCoordinates();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<USplineComponent> SplineComponent;
	UPROPERTY(EditAnywhere, Category = "Path|Visualization")
	FColor PathColor = FColor::Green;

	UPROPERTY(EditAnywhere, Category = "Path|Visualization")
	float PathThickness = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Path|Visualization")
	bool bShowDebugPath = true;
};
