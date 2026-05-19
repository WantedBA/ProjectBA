// Copyright TeamBA. All Rights Reserved.

#include "BATableGenerator.h"

#include "BASheetSpecs.h"
#include "Tables/BAPropTable.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "BATableGenerator"

namespace
{
	const TCHAR* TablePackageRoot = TEXT("/Game/Table");

	FString GetBADesignDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("BADesign"));
	}

	FString GetConverterExePath()
	{
		return GetBADesignDir() / TEXT("ExcelToJsonConverter") / TEXT("ExcelToJsonConverter.exe");
	}

	FString GetJsonDir()
	{
		return GetBADesignDir() / TEXT("Json");
	}

	void Notify(const FText& Message, bool bSuccess)
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;
		const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
		if (Item.IsValid())
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}
}

void FBATableGenerator::Generate()
{
	GenerateInternal(!IsRunningCommandlet());
}

bool FBATableGenerator::GenerateHeadless()
{
	return GenerateInternal(false);
}

bool FBATableGenerator::GenerateInternal(const bool bShowUi)
{
	TUniquePtr<FScopedSlowTask> SlowTask;
	if (bShowUi)
	{
		SlowTask = MakeUnique<FScopedSlowTask>(3.0f, LOCTEXT("Generating", "Generating DataTables..."));
		SlowTask->MakeDialog();
	}

	if (SlowTask)
	{
		SlowTask->EnterProgressFrame(1.0f, LOCTEXT("RunningConverter", "Excel -> JSON"));
	}
	FString ConverterLog;
	if (!RunConverterExe(ConverterLog))
	{
		UE_LOG(LogTemp, Error, TEXT("[BATableGenerator] Converter failed:\n%s"), *ConverterLog);
		if (bShowUi)
		{
			Notify(LOCTEXT("ConverterFailed", "Excel -> JSON conversion failed. See Output Log."), false);
		}
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[BATableGenerator] Converter output:\n%s"), *ConverterLog);

	if (SlowTask)
	{
		SlowTask->EnterProgressFrame(1.0f, LOCTEXT("ImportingJson", "JSON -> DataTable"));
	}
	FBASheetSpecs::Invalidate();
	ImportAllJson();

	if (SlowTask)
	{
		SlowTask->EnterProgressFrame(1.0f, LOCTEXT("Done", "Done"));
	}
	if (bShowUi)
	{
		Notify(LOCTEXT("DoneMsg", "DataTable generation complete."), true);
	}
	return true;
}

bool FBATableGenerator::RunConverterExe(FString& OutLog)
{
	const FString ExePath = GetConverterExePath();
	if (!FPaths::FileExists(ExePath))
	{
		OutLog = FString::Printf(TEXT("Converter exe not found: %s"), *ExePath);
		return false;
	}

	int32 ReturnCode = -1;
	FString StdOut;
	FString StdErr;
	const bool bExecuted = FPlatformProcess::ExecProcess(
		*ExePath,
		TEXT(""),
		&ReturnCode,
		&StdOut,
		&StdErr,
		*FPaths::GetPath(ExePath));

	OutLog = FString::Printf(
		TEXT("ExitCode=%d\n--- STDOUT ---\n%s\n--- STDERR ---\n%s"),
		ReturnCode, *StdOut, *StdErr);

	return bExecuted && ReturnCode == 0;
}

void FBATableGenerator::ImportAllJson()
{
	const FString JsonDir = GetJsonDir();
	if (!IFileManager::Get().DirectoryExists(*JsonDir))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableGenerator] Json dir not found: %s"), *JsonDir);
		return;
	}

	TArray<FString> JsonFiles;
	IFileManager::Get().FindFiles(JsonFiles, *(JsonDir / TEXT("*.json")), true, false);

	for (const FString& Filename : JsonFiles)
	{
		// SheetRecipe.json 은 데이터 파일이 아니라 spec 사이드카임 — 임포트 대상에서 제외
		if (Filename.Equals(TEXT("SheetRecipe.json"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		ImportJsonFile(JsonDir / Filename);
	}
}

void FBATableGenerator::ImportJsonFile(const FString& JsonPath)
{
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *JsonPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableGenerator] Cannot read %s"), *JsonPath);
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableGenerator] Invalid JSON: %s"), *JsonPath);
		return;
	}

	const FString FileStem = FPaths::GetBaseFilename(JsonPath);

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Root->Values)
	{
		const FString& SheetName = Field.Key;
		const TSharedPtr<FJsonValue>& Value = Field.Value;

		const FBASheetSpec* Spec = FBASheetSpecs::Find(SheetName);
		if (!Spec)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BATableGenerator] Sheet '%s' is not registered in FBASheetSpecs. Skipped."),
				*SheetName);
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* JsonRows = nullptr;
		if (!Value->TryGetArray(JsonRows) || !JsonRows)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BATableGenerator] Sheet '%s' is not a JSON array."), *SheetName);
			continue;
		}

		const FString AssetName = FString::Printf(TEXT("DT_%s_%s"), *FileStem, *SheetName);
		const FString PackagePath = FString::Printf(TEXT("%s/%s"), TablePackageRoot, *AssetName);

		FString Error;
		if (!BuildAndSaveDataTable(AssetName, PackagePath, *Spec, *JsonRows, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[BATableGenerator] %s failed: %s"), *AssetName, *Error);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[BATableGenerator] Wrote %s (%d rows)"),
				*AssetName, JsonRows->Num());
		}
	}
}

bool FBATableGenerator::BuildAndSaveDataTable(
	const FString& AssetName,
	const FString& PackagePath,
	const FBASheetSpec& Spec,
	const TArray<TSharedPtr<FJsonValue>>& JsonRows,
	FString& OutError)
{
	if (!Spec.RowStruct)
	{
		OutError = TEXT("RowStruct is null");
		return false;
	}

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		OutError = TEXT("CreatePackage failed");
		return false;
	}
	Package->FullyLoad();

	UDataTable* DataTable = FindObject<UDataTable>(Package, *AssetName);
	const bool bNewlyCreated = (DataTable == nullptr);
	if (DataTable)
	{
		DataTable->EmptyTable();
	}
	else
	{
		DataTable = NewObject<UDataTable>(Package, FName(*AssetName), RF_Public | RF_Standalone);
	}
	DataTable->RowStruct = Spec.RowStruct;

	const int32 RowSize = Spec.RowStruct->GetStructureSize();

	for (const TSharedPtr<FJsonValue>& Element : JsonRows)
	{
		const TSharedPtr<FJsonObject>* RowObjPtr = nullptr;
		if (!Element->TryGetObject(RowObjPtr) || !RowObjPtr)
		{
			continue;
		}
		const TSharedPtr<FJsonObject>& RowObj = *RowObjPtr;

		const TSharedPtr<FJsonValue> KeyValue = RowObj->TryGetField(Spec.KeyColumnName);
		if (!KeyValue.IsValid())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BATableGenerator] %s: row missing key column '%s'"),
				*AssetName, *Spec.KeyColumnName);
			continue;
		}

		FString KeyString;
		if (KeyValue->Type == EJson::Number)
		{
			KeyString = FString::Printf(TEXT("%lld"), static_cast<int64>(KeyValue->AsNumber()));
		}
		else
		{
			KeyString = KeyValue->AsString();
		}
		const FName RowName(*KeyString);

		void* RowMemory = FMemory::Malloc(RowSize);
		Spec.RowStruct->InitializeStruct(RowMemory);

		const bool bConverted = FJsonObjectConverter::JsonObjectToUStruct(
			RowObj.ToSharedRef(), Spec.RowStruct, RowMemory, /*CheckFlags*/ 0, /*SkipFlags*/ 0);

		if (bConverted)
		{
			DataTable->AddRow(RowName, *static_cast<FBARowBase*>(RowMemory));
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BATableGenerator] %s: JsonObjectToUStruct failed for row '%s'"),
				*AssetName, *KeyString);
		}

		Spec.RowStruct->DestroyStruct(RowMemory);
		FMemory::Free(RowMemory);
	}

	DataTable->MarkPackageDirty();
	if (bNewlyCreated)
	{
		FAssetRegistryModule::AssetCreated(DataTable);
	}

	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.bWarnOfLongFilename = false;

	const bool bSaved = UPackage::SavePackage(Package, DataTable, *FilePath, SaveArgs);
	if (!bSaved)
	{
		OutError = FString::Printf(TEXT("SavePackage failed: %s"), *FilePath);
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
