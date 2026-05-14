#include "Enemy/AI/PatrolPath.h"

APatrolPath::APatrolPath()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetClosedLoop(true);
	SetRootComponent(SplineComponent);

	SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor::Red;
	SplineComponent->EditorSelectedSplineSegmentColor = FLinearColor::White;
}

void APatrolPath::PrintPathCoordinates()
{
	if (SplineComponent == nullptr)
	{
		return;
	}

	int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
	FString Output = FString::Printf(TEXT("Patrol Points for Tag '%s': "), *GetActorNameOrLabel());

	for (int32 i = 0; i < NumPoints; ++i)
	{
		FVector Loc = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		Output += FString::Printf(TEXT("(%.1f, %.1f, %.1f)%s"), Loc.X, Loc.Y, Loc.Z, (i == NumPoints - 1) ? TEXT("") : TEXT(", "));
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Output);
	
	// 화면에도 출력
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Output);
	}
}
