// Copyright TeamBA. All Rights Reserved.

#include "BANiagaraExporter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

bool BANiagaraExporter::ExportToText(const FNiagaraGameplayAnalysisData& InData, FString& OutContent)
{
	OutContent = TEXT("========================================\n");
	OutContent += FString::Printf(TEXT("System: %s\n"), *InData.SystemName);
	OutContent += TEXT("Gameplay Readability Audit\n");
	OutContent += TEXT("========================================\n\n");

	for (const auto& Emitter : InData.Emitters)
	{
		OutContent += TEXT("[Emitter]\n");
		OutContent += Emitter.EmitterName + TEXT("\n\n");

		OutContent += TEXT("Enabled:\nTRUE\n\n");

		OutContent += TEXT("Category:\n");
		OutContent += UEnum::GetValueAsString(Emitter.Category)
			.Replace(TEXT("ENiagaraEmitterCategory::"), TEXT("")) + TEXT("\n\n");

		OutContent += TEXT("Sim Target:\n");
		OutContent += Emitter.SimTarget + TEXT("\n\n");

		if (Emitter.SpawnData.BurstCount > 0)
		{
			OutContent += FString::Printf(TEXT("Spawn:\nBurst = %d\n\n"), Emitter.SpawnData.BurstCount);
		}
		else if (Emitter.SpawnData.SpawnRate > 0)
		{
			OutContent += FString::Printf(TEXT("Spawn:\nRate = %.2f\n\n"), Emitter.SpawnData.SpawnRate);
		}

		if (Emitter.LifetimeMax > 0)
		{
			OutContent += FString::Printf(TEXT("Lifetime:\n%.2f ~ %.2f\n\n"),
				Emitter.LifetimeMin,
				Emitter.LifetimeMax);
		}

		if (Emitter.Renderers.Num() > 0)
		{
			OutContent += TEXT("Renderer:\n");

			for (const auto& Renderer : Emitter.Renderers)
			{
				OutContent += Renderer.RendererType
					+ TEXT(" (")
					+ Renderer.BlendMode
					+ TEXT(")\n");
			}

			OutContent += TEXT("\n");
		}

		OutContent += TEXT("Facing:\n");

		if (Emitter.bFaceCamera == true)
		{
			OutContent += TEXT("FaceCamera\n");
		}

		if (Emitter.bVelocityAligned == true)
		{
			OutContent += TEXT("VelocityAligned\n");
		}

		if (Emitter.bCustomFacing == true)
		{
			OutContent += TEXT("CustomFacing\n");
		}

		if (Emitter.bFaceCamera == false &&
			Emitter.bVelocityAligned == false &&
			Emitter.bCustomFacing == false)
		{
			OutContent += TEXT("Default\n");
		}

		OutContent += TEXT("\n");

		OutContent += TEXT("Warnings:\n");

		if (Emitter.Warnings.Num() > 0)
		{
			for (const auto& Warning : Emitter.Warnings)
			{
				OutContent += TEXT("- ") + Warning + TEXT("\n");
			}
		}
		else
		{
			OutContent += TEXT("NONE\n");
		}

		OutContent += TEXT("\n----------------------------------------\n\n");
	}

	return true;
}

bool BANiagaraExporter::ExportToJson(const FNiagaraGameplayAnalysisData& InData, FString& OutContent)
{
	return FJsonObjectConverter::UStructToJsonObjectString(InData, OutContent);
}

void BANiagaraExporter::SaveToFile(const FString& InFileName, const FString& InContent)
{
	FString SavePath =
		FPaths::ProjectSavedDir() / TEXT("FXAnalyzer") / InFileName;

	FFileHelper::SaveStringToFile(InContent, *SavePath);

	UE_LOG(LogTemp, Log, TEXT("FX Analysis saved to: %s"), *SavePath);
}