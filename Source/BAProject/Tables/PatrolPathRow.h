// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "PatrolPathRow.generated.h"

/**
 * PatrolPath.xlsx 의 "PatrolPath" 시트 한 행.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FPatrolPathRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FName PathTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString Point;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Patrol")
	TArray<FVector> Points;

	virtual void PostRead() override
	{
		bTid = Tid;

		Points.Empty();
		if (!Point.IsEmpty())
		{
			// "(x,y,z), (x,y,z)" 형태를 파싱
			TArray<FString> VectorStrings;
			Point.ParseIntoArray(VectorStrings, TEXT("),"), true);

			for (FString& VStr : VectorStrings)
			{
				VStr = VStr.Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT("")).TrimStartAndEnd();
				FVector Vec;
				if (Vec.InitFromString(VStr))
				{
					Points.Add(Vec);
				}
			}
		}
	}
};
