// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "SkillRows.generated.h"

/**
 * Item.xlsx 의 "Consume" 시트 한 행.
 * 필드 이름은 JSON 키와 정확히 일치해야 한다 (Item.json 참고).
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FSkillRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 SkillTid = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 TextTid = 0;

	// 엑셀에서 읽어올 값
	UPROPERTY(BlueprintReadOnly, Category = "Skill", meta = (HideInDetailPanel))
	FString Prerequisites;
	
	// 위 값(Prerequisites)을 파싱해서 int32 배열로 변환한 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> PrerequisiteIds;
	
	// 최초 초기화 이후 PrerequisiteIds으로 찾은 값 TODO: TableManager에서 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ChildIds;

	// 서로 동시에 찍을 수 없는 스킬
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FString ExclusiveSkills;
	
	// 위 값을 파싱해서 int32 배열로 변환
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ExclusiveSkillIds;

	// 기본 스킬인지 여부(기본 스킬이면 할당 해제 불가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	bool bIsDefaultSkill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 NeededSkillPoint = 0;
	
	// 아이콘 경로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FString IconPathString;
	
	// IconPathString 값을 TSoftObjectPath로 변환하여 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TSoftObjectPtr<UTexture2D> IconTexture;
	
	// Canvas Slot 내 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float PositionX = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float PositionY = 0.0f;
	
	virtual void PostRead() override
	{
		bTid = SkillTid;
		
		// 필요한 값 변환
		PrerequisiteIds = ParseIntArray(Prerequisites);
		IconTexture = ConvertToSoftObjectPtr(IconPathString);
		ExclusiveSkillIds = ParseIntArray(ExclusiveSkills);
	}

	// 문자열을 쉼표로 구분하여 int32 배열로 변환
	static TArray<int32> ParseIntArray(const FString& InString)
	{
		UE_LOG(LogTemp, Log, TEXT("Parsing prerequisites: %s"), *InString);
		TArray<int32> Result;
		TArray<FString> Tokens;
		InString.ParseIntoArray(Tokens, TEXT(","), true);
		for (const FString& Token : Tokens)
		{
			const FString Trimmed = Token.TrimStartAndEnd();
			if (!Trimmed.IsEmpty())
			{
				Result.Add(FCString::Atoi(*Trimmed));
			}
		}
		return Result;
	}
	
	static TSoftObjectPtr<UTexture2D> ConvertToSoftObjectPtr(const FString& IconPathString)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPathString));
	}
};
