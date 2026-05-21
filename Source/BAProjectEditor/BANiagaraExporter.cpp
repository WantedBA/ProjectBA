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
		OutContent += FString::Printf(TEXT("[Emitter]\n%s\n\n"), *Emitter.EmitterName);

		// 기본 상태 정보
		OutContent += TEXT("Basic Info:\n");
		OutContent += TEXT("  Enabled:    TRUE\n");
		OutContent += FString::Printf(TEXT("  Category:   %s\n"), *UEnum::GetValueAsString(Emitter.Category).Replace(TEXT("ENiagaraEmitterCategory::"), TEXT("")));
		OutContent += FString::Printf(TEXT("  Sim Target: %s\n"), *Emitter.SimTarget);
		OutContent += FString::Printf(TEXT("  Scalability: %s\n"), Emitter.bFixedBounds ? TEXT("Fixed Bounds (Safe)") : TEXT("Dynamic Bounds (Risk)"));
		OutContent += TEXT("\n");

		// 🔥 추가: Spawn 분석 데이터 명시
		OutContent += TEXT("Spawn Analysis:\n");
		if (Emitter.SpawnData.BurstCount > 0)
		{
			OutContent += FString::Printf(TEXT("  Spawn Type:  Burst\n"));
			OutContent += FString::Printf(TEXT("  Burst Count: %d\n"), Emitter.SpawnData.BurstCount);
			OutContent += FString::Printf(TEXT("  Multi Burst: %s\n"), Emitter.SpawnData.bHasMultiBurst ? TEXT("YES (Screen Pollution Risk)") : TEXT("NO"));
		}
		else if (Emitter.SpawnData.SpawnRate > 0)
		{
			OutContent += FString::Printf(TEXT("  Spawn Type:  Continuous Rate\n"));
			OutContent += FString::Printf(TEXT("  Spawn Rate:  %.2f particles/sec\n"), Emitter.SpawnData.SpawnRate);
		}
		else
		{
			OutContent += TEXT("  Spawn Type:  Inactive or Parameter Driven\n");
		}
		OutContent += TEXT("\n");

		// 🔥 추가: Particle 세부 수치 데이터 명시
		OutContent += TEXT("Particle Properties:\n");
		OutContent += FString::Printf(TEXT("  Lifetime:    %.3f ~ %.3f sec\n"), Emitter.LifetimeMin, Emitter.LifetimeMax);
		OutContent += FString::Printf(TEXT("  Max Size:    [X: %.1f, Y: %.1f]\n"), Emitter.SpriteSizeMax.X, Emitter.SpriteSizeMax.Y);
		OutContent += FString::Printf(TEXT("  Velocity:    %s\n"), *Emitter.VelocityMode);
		OutContent += TEXT("\n");

		// 렌더러 및 머티리얼 디테일
		if (Emitter.Renderers.Num() > 0)
		{
			OutContent += TEXT("Renderer & Material Analysis:\n");
			for (const auto& Renderer : Emitter.Renderers)
			{
				OutContent += FString::Printf(TEXT("  - Type:      %s\n"), *Renderer.RendererType);
				OutContent += FString::Printf(TEXT("    Material:  %s\n"), *Renderer.MaterialName);
				OutContent += FString::Printf(TEXT("    BlendMode: %s\n"), *Renderer.BlendMode);
				OutContent += FString::Printf(TEXT("    SortMode:  %s\n"), *Renderer.SortMode);
			}
			OutContent += TEXT("\n");
		}

		// 페이싱 규칙
		OutContent += TEXT("Facing Mode:\n");
		if (Emitter.bFaceCamera) { OutContent += TEXT("  - FaceCamera\n"); }
		if (Emitter.bVelocityAligned) { OutContent += TEXT("  - VelocityAligned\n"); }
		if (Emitter.bCustomFacing) { OutContent += TEXT("  - CustomFacing\n"); }
		if (!Emitter.bFaceCamera && !Emitter.bVelocityAligned && !Emitter.bCustomFacing)
		{
			OutContent += TEXT("  - Default\n");
		}
		OutContent += TEXT("\n");

		// 오딧 경고 및 추천 액션 가이드
		OutContent += TEXT("Gameplay Readability Warnings:\n");
		if (Emitter.Warnings.Num() > 0)
		{
			for (const auto& Warning : Emitter.Warnings)
			{
				OutContent += FString::Printf(TEXT("  [WARNING] %s\n"), *Warning);
			}

			// 경고 기반 AI/작업자용 실무 추천 액션 출력
			OutContent += TEXT("\n  Recommendation Action:\n");
			if (Emitter.Warnings.Contains(TEXT("Screen Pollution Risk")) || Emitter.Warnings.Contains(TEXT("Lingering Distortion")))
			{
				OutContent += TEXT("    -> Reduce Initial Sprite/Mesh Size under 300 units.\n");
			}
			if (Emitter.Warnings.Contains(TEXT("Long Lifetime Risk")))
			{
				OutContent += TEXT("    -> Force Clamp Lifetime under 0.25s for hit-impact readability.\n");
			}
		}
		else
		{
			OutContent += TEXT("  NONE (Pass)\n");
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