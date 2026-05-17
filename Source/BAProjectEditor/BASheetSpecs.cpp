// Copyright TeamBA. All Rights Reserved.

#include "BASheetSpecs.h"

#include "Tables/BAPropTable.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectIterator.h"

namespace
{
	static const FName MetaSheet = TEXT("BASheet");

	FString GetRecipePath()
	{
		return FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("BADesign") / TEXT("Json") / TEXT("SheetRecipe.json"));
	}

	TMap<FString, FString> LoadRecipe()
	{
		TMap<FString, FString> Out;

		const FString Path = GetRecipePath();
		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *Path))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BASheetSpecs] Recipe not found: %s. Excel 변환을 먼저 수행하세요."),
				*Path);
			return Out;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BASheetSpecs] Invalid recipe JSON: %s"), *Path);
			return Out;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			FString Value;
			if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
			{
				Out.Add(Pair.Key, Value);
			}
		}
		return Out;
	}

	bool& BuiltFlag()
	{
		static bool bBuilt = false;
		return bBuilt;
	}

	TMap<FString, FBASheetSpec>& MutableSpecMap()
	{
		static TMap<FString, FBASheetSpec> Map;
		return Map;
	}

	const TMap<FString, FBASheetSpec>& GetSpecMap()
	{
		TMap<FString, FBASheetSpec>& Map = MutableSpecMap();
		if (BuiltFlag())
		{
			return Map;
		}

		Map.Reset();

		const TMap<FString, FString> Recipe = LoadRecipe();
		UScriptStruct* Base = FBARowBase::StaticStruct();

		TMap<FString, UScriptStruct*> SheetToStruct;
		for (TObjectIterator<UScriptStruct> It; It; ++It)
		{
			UScriptStruct* S = *It;
			if (!S || S == Base) continue;
			if (!S->IsChildOf(Base)) continue;
			if (!S->HasMetaData(MetaSheet)) continue;

			const FString Sheet = S->GetMetaData(MetaSheet);
			if (Sheet.IsEmpty()) continue;

			if (UScriptStruct** Existing = SheetToStruct.Find(Sheet))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BASheetSpecs] Duplicate BASheet '%s' on %s (이미 %s 사용 중). 무시함."),
					*Sheet, *S->GetName(), *(*Existing)->GetName());
				continue;
			}
			SheetToStruct.Add(Sheet, S);
		}

		for (const TPair<FString, UScriptStruct*>& Pair : SheetToStruct)
		{
			const FString& Sheet = Pair.Key;
			UScriptStruct* S = Pair.Value;

			const FString* Key = Recipe.Find(Sheet);
			if (!Key || Key->IsEmpty())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BASheetSpecs] '%s' (struct %s): Recipe 에 키 컬럼 항목이 없습니다."),
					*Sheet, *S->GetName());
				continue;
			}
			Map.Add(Sheet, FBASheetSpec{ S, *Key });
		}

		// Recipe 에 있지만 매핑되는 USTRUCT 가 없는 시트는 경고
		for (const TPair<FString, FString>& Pair : Recipe)
		{
			if (!SheetToStruct.Contains(Pair.Key))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BASheetSpecs] Recipe sheet '%s' 에 대응하는 USTRUCT 가 없습니다 (meta = (BASheet = \"%s\")) 누락)."),
					*Pair.Key, *Pair.Key);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[BASheetSpecs] %d sheet(s) registered."), Map.Num());
		BuiltFlag() = true;
		return Map;
	}
}

const FBASheetSpec* FBASheetSpecs::Find(const FString& SheetName)
{
	return GetSpecMap().Find(SheetName);
}

TArray<FString> FBASheetSpecs::GetAllSheetNames()
{
	TArray<FString> Out;
	GetSpecMap().GetKeys(Out);
	return Out;
}

void FBASheetSpecs::Invalidate()
{
	BuiltFlag() = false;
	MutableSpecMap().Reset();
}
