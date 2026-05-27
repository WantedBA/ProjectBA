// Copyright TeamBA. All Rights Reserved.

#include "BANiagaraExporter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

bool BANiagaraExporter::ExportToText(const FNiagaraGameplayAnalysisData& InData, FString& OutContent)
{
	OutContent = FString::Printf(TEXT("System: %s\n"), *InData.SystemName);
	OutContent += TEXT("Gameplay Readability Audit & Deep Analysis\n");

	// 시스템 파라미터 출력
	if (InData.SystemParameters.Num() > 0)
	{
		OutContent += TEXT("[System Parameters]\n");
		for (const auto& KVP : InData.SystemParameters)
		{
			OutContent += FString::Printf(TEXT("  %s: %s\n"), *KVP.Key, *KVP.Value);
		}
		OutContent += TEXT("\n");
	}

	// 이미터 순회 상세 분석 출력
	for (const auto& Emitter : InData.Emitters)
	{
		OutContent += FString::Printf(TEXT("[Emitter] %s\n"), *Emitter.EmitterName);

		// 기본 상태 정보
		OutContent += TEXT("Basic Info:\n");
		OutContent += FString::Printf(TEXT("  - Enabled:              %s\n"), Emitter.bIsEnabled ? TEXT("TRUE") : TEXT("FALSE"));
		OutContent += FString::Printf(TEXT("  - Category:             %s\n"), *UEnum::GetValueAsString(Emitter.Category).Replace(TEXT("ENiagaraEmitterCategory::"), TEXT("")));
		OutContent += FString::Printf(TEXT("  - Motion Pattern:       %s\n"), *UEnum::GetValueAsString(Emitter.MotionPattern).Replace(TEXT("ENiagaraMotionPattern::"), TEXT("")));
		OutContent += FString::Printf(TEXT("  - Sim Target:           %s\n"), *Emitter.SimTarget);
		OutContent += FString::Printf(TEXT("  - Bounds Management:    %s\n"), Emitter.bFixedBounds ? TEXT("Fixed Bounds (Safe)") : TEXT("Dynamic Bounds (Risk)"));
		OutContent += FString::Printf(TEXT("  - Cull Distance Settings:%s\n"), Emitter.bHasCullDistance ? TEXT("Configured") : TEXT("Not Configured"));
		OutContent += FString::Printf(TEXT("  - Drive Control Mode:   %s\n"), *Emitter.DriveMode); // 구동 방식 출력
		OutContent += FString::Printf(TEXT("  - Max Particle Count:   %d\n"), Emitter.ExplicitMaxParticleCount); // 실제 맥스수 출력
		OutContent += FString::Printf(TEXT("  - Complexity Score:     %.1f / 100.0\n"), Emitter.ComplexityScore);
		OutContent += TEXT("\n");

		// 공간 및 물리 동작 특성
		OutContent += TEXT("Spatial & Physics Behavior:\n");
		OutContent += FString::Printf(TEXT("  - Simulation Space:     %s\n"), Emitter.bWorldSpace ? TEXT("World Space") : TEXT("Local Space"));
		OutContent += FString::Printf(TEXT("  - Collision Enabled:    %s\n"), Emitter.bHasCollision ? TEXT("YES") : TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Physics Modules:      Gravity=%s, Drag=%s, CurlNoise=%s, Vortex=%s\n"),
			Emitter.bHasGravity ? TEXT("YES") : TEXT("NO"),
			Emitter.bHasDrag ? TEXT("YES") : TEXT("NO"),
			Emitter.bHasCurlNoise ? TEXT("YES") : TEXT("NO"),
			Emitter.bHasVortex ? TEXT("YES") : TEXT("NO"));
		OutContent += TEXT("\n");

		// 데이터 인터페이스 및 바인딩 정보
		if (Emitter.DataInterfaces.Num() > 0)
		{
			OutContent += TEXT("  - Interfaces: ");
			for (const FString& DI : Emitter.DataInterfaces) { OutContent += DI + TEXT(" "); }
			OutContent += TEXT("\n");
		}

		if (Emitter.BoundParameters.Num() > 0)
		{
			OutContent += TEXT("  - Bindings:   ");
			for (const FString& BP : Emitter.BoundParameters) { OutContent += BP + TEXT(" "); }
			OutContent += TEXT("\n");
		}

		// 사용된 모듈 출력
		if (Emitter.ModuleNames.Num() > 0)
		{
			OutContent += TEXT("  Modules:    ");
			OutContent += TEXT("  - ");
			for (int32 i = 0; i < Emitter.ModuleNames.Num(); ++i)
			{
				OutContent += Emitter.ModuleNames[i] + (i == Emitter.ModuleNames.Num() - 1 ? TEXT("") : TEXT(", "));
			}
			OutContent += TEXT("\n\n");
		}
		OutContent += TEXT("\n");

		// Spawn 분석 데이터
		OutContent += TEXT("Spawn Analysis:\n");
		OutContent += FString::Printf(TEXT("  - Base Spawn Rate:      %.2f particles/sec\n"), Emitter.SpawnData.SpawnRate);
		OutContent += FString::Printf(TEXT("  - Burst Count:          %d\n"), Emitter.SpawnData.BurstCount);
		OutContent += FString::Printf(TEXT("  - Has Multi-Burst:      %s\n"), Emitter.SpawnData.bHasMultiBurst ? TEXT("YES") : TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Burst Interval:       %.2f sec\n"), Emitter.SpawnData.BurstInterval);
		OutContent += FString::Printf(TEXT("  - Infinite Loop Spawn:  %s\n"), Emitter.SpawnData.bInfiniteSpawn ? TEXT("YES") : TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Est. Max Particles:   %.1f\n"), Emitter.SpawnData.EstimatedMaxParticles);
		OutContent += TEXT("\n");

		// Particle 세부 수치
		OutContent += TEXT("Particle Properties:\n");
		OutContent += FString::Printf(TEXT("  - Lifetime Range:       %.3f ~ %.3f sec\n"), Emitter.LifetimeMin, Emitter.LifetimeMax);
		if (Emitter.Category == ENiagaraEmitterCategory::Ribbon)
		{
			// [추가] 리본 이미터일 경우 두께 제어 방식 상세 출력
			OutContent += FString::Printf(TEXT("  - Ribbon Base Width:    %.1f units\n"), Emitter.RibbonWidth);
			OutContent += FString::Printf(TEXT("  - Ribbon Width Curve:   %s\n"), Emitter.bHasRibbonWidthCurve ? TEXT("YES (Dynamic)") : TEXT("NO (Static)"));
		}
		else
		{
			OutContent += FString::Printf(TEXT("  - Sprite Size Min:      [X: %.1f, Y: %.1f]\n"), Emitter.SpriteSizeMin.X, Emitter.SpriteSizeMin.Y);
			OutContent += FString::Printf(TEXT("  - Sprite Size Max:      [X: %.1f, Y: %.1f]\n"), Emitter.SpriteSizeMax.X, Emitter.SpriteSizeMax.Y);
		}
		OutContent += FString::Printf(TEXT("  - Velocity Mode:        %s\n"), *Emitter.VelocityMode);
		OutContent += TEXT("\n");

		// 커브 샘플링 데이터 정보 출력
		if (Emitter.Curves.Num() > 0)
		{
			OutContent += TEXT("Sampled Curves:\n");
			for (const auto& Curve : Emitter.Curves)
			{
				OutContent += FString::Printf(TEXT("  - Parameter: %s (Keys: %d)\n"), *Curve.ParameterName, Curve.TimeKeys.Num());
			}
			OutContent += TEXT("\n");
		}

		// 렌더러 및 머티리얼 상세 분석 (Scalar, Vector 파라미터 포함)
		if (Emitter.Renderers.Num() > 0)
		{
			OutContent += TEXT("Renderer & Material Analysis:\n");
			for (const auto& Renderer : Emitter.Renderers)
			{
				OutContent += FString::Printf(TEXT("  - Type:        %s\n"), *Renderer.RendererType);
				OutContent += FString::Printf(TEXT("    Material:    %s\n"), *Renderer.MaterialName);
				OutContent += FString::Printf(TEXT("    BlendMode:   %s\n"), *Renderer.BlendMode);
				OutContent += FString::Printf(TEXT("    SortMode:    %s\n"), *Renderer.SortMode);
				OutContent += FString::Printf(TEXT("    Features:    SoftParticle=%s, DepthFade=%s, Fresnel=%s, Distortion=%s\n"),
					Renderer.bUsesSoftParticle ? TEXT("YES") : TEXT("NO"),
					Renderer.bUsesDepthFade ? TEXT("YES") : TEXT("NO"),
					Renderer.bUsesFresnel ? TEXT("YES") : TEXT("NO"),
					Renderer.bUsesDistortion ? TEXT("YES") : TEXT("NO"));

				// Scalar Parameters
				if (Renderer.ScalarParameters.Num() > 0)
				{
					OutContent += TEXT("    Scalars:   ");
					for (const auto& KVP : Renderer.ScalarParameters)
					{
						OutContent += FString::Printf(TEXT("%s:%.2f "), *KVP.Key, KVP.Value);
					}
					OutContent += TEXT("\n");
				}

				// Vector Parameters
				if (Renderer.VectorParameters.Num() > 0)
				{
					OutContent += TEXT("    Vectors:   ");
					for (const auto& KVP : Renderer.VectorParameters)
					{
						OutContent += FString::Printf(TEXT("%s:%s "), *KVP.Key, *KVP.Value.ToString());
					}
					OutContent += TEXT("\n");
				}

				// Texture Parameters
				if (Renderer.TextureParameters.Num() > 0)
				{
					OutContent += TEXT("    Textures:  ");
					for (const auto& KVP : Renderer.TextureParameters)
					{
						OutContent += FString::Printf(TEXT("%s:%s "), *KVP.Key, *KVP.Value);
					}
					OutContent += TEXT("\n");
				}
			}
			OutContent += TEXT("\n");
		}

		// 페이싱 규칙 정리
		OutContent += TEXT("Facing & Realignment:\n");
		OutContent += FString::Printf(TEXT("  - Screen Face:          %s\n"), Emitter.bFaceCamera ? TEXT("FaceCamera") : TEXT("Standard"));
		OutContent += FString::Printf(TEXT("  - Camera Alignment:     %s\n"), Emitter.bCameraFacing ? TEXT("YES") : TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Velocity Aligned:     %s\n"), Emitter.bVelocityAligned ? TEXT("YES") : TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Custom Facing Vector: %s\n"), Emitter.bCustomFacing ? TEXT("YES") : TEXT("NO"));

		OutContent += TEXT("AI Semantic Intent:\n");
		OutContent += FString::Printf(TEXT("  - Gameplay Role:       %s\n"),*UEnum::GetValueAsString(Emitter.GameplayRole));
		OutContent += FString::Printf(TEXT("  - Temporal Behavior:   %s\n"),*UEnum::GetValueAsString(Emitter.TemporalBehavior));
		OutContent += FString::Printf(TEXT("  - Color Semantic:      %s\n"),*UEnum::GetValueAsString(Emitter.ColorSemantic));
		OutContent += FString::Printf(TEXT("  - Screen Coverage:     %.2f\n"),Emitter.EstimatedScreenCoverage);
		OutContent += FString::Printf(TEXT("  - Center Dominant:     %s\n"),Emitter.bCenterScreenDominant? TEXT("YES"): TEXT("NO"));
		OutContent += FString::Printf(TEXT("  - Peripheral FX:       %s\n"),Emitter.bPeripheralFX? TEXT("YES"): TEXT("NO"));

		OutContent += TEXT("\n");

		// 오딧 가이드 및 경고 (루프 내에서 안전하게 개별 Emitter 정보 출력)
		OutContent += TEXT("Gameplay Readability Warnings:\n");
		if (Emitter.Warnings.Num() > 0)
		{
			for (const auto& Warning : Emitter.Warnings)
			{
				OutContent += FString::Printf(TEXT("  [WARNING] %s\n"), *Warning);
			}

			OutContent += TEXT("\n  Recommended Action:\n");
			if (Emitter.Warnings.Contains(TEXT("Screen Pollution Risk")))
			{
				OutContent += TEXT("    -> Reduce Sprite/Mesh Size under 300 units immediately.\n");
			}
			if (Emitter.Warnings.Contains(TEXT("Long Lifetime Risk")))
			{
				OutContent += TEXT("    -> Clamp particle lifetime under 0.25s for immediate hit-impact clarity.\n");
			}
		}
		else
		{
			OutContent += TEXT("  NONE (Pass)\n");
		}

		// 이미터 개별 AI 데이터 요약 (오류 수정 지점)
		OutContent += TEXT("\n  Emitter AI Semantic Summary:\n");
		OutContent += FString::Printf(TEXT("    - Est. GPU Cost:     %.1f\n"), Emitter.EstimatedGPUCost);
		OutContent += FString::Printf(TEXT("    - Est. Overdraw Score:%.1f\n"), Emitter.EstimatedOverdraw);
		if (Emitter.SemanticTags.Num() > 0)
		{
			OutContent += TEXT("    - Semantic Tags:     ");
			for (const FString& Tag : Emitter.SemanticTags) { OutContent += Tag + TEXT(" "); }
			OutContent += TEXT("\n");
		}

		OutContent += TEXT("\n--------------------------------------------------\n\n");
	}

	// 3. 전역 시스템 요약 출력
	OutContent += TEXT("=== SYSTEM TOTAL AI SEMANTIC SUMMARY ===\n");
	for (const FString& Tag : InData.SystemSemanticTags)
	{
		OutContent += TEXT("  - Summary Tag: ") + Tag + TEXT("\n");
	}
	OutContent += FString::Printf(TEXT("Total Aggregated Engine Cost: %.1f\n"), InData.TotalEstimatedCost);
	OutContent += TEXT("==================================================\n");

	return true;
}

bool BANiagaraExporter::ExportToJson(const FNiagaraGameplayAnalysisData& InData, FString& OutContent)
{
	return FJsonObjectConverter::UStructToJsonObjectString(InData, OutContent);
}

void BANiagaraExporter::SaveToFile(const FString& InFileName, const FString& InContent)
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("FXAnalyzer") / InFileName;
	FFileHelper::SaveStringToFile(InContent, *SavePath);
	UE_LOG(LogTemp, Log, TEXT("FX Analysis saved to: %s"), *SavePath);
}