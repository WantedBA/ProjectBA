// Copyright TeamBA. All Rights Reserved.

#include "BANiagaraAnalyzer.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "NiagaraCommon.h"
#include "NiagaraPlatformSet.h"

UBANiagaraAnalyzer::UBANiagaraAnalyzer()
{
}

bool UBANiagaraAnalyzer::AnalyzeNiagaraSystem(UNiagaraSystem* InSystem, FNiagaraGameplayAnalysisData& OutData)
{
	if (InSystem == nullptr)
	{
		return false;
	}

	OutData.SystemName = InSystem->GetName();

	for (const FNiagaraEmitterHandle& Handle : InSystem->GetEmitterHandles())
	{
		// Requirement: ONLY ENABLED EMITTERS
		if (Handle.GetIsEnabled() == true)
		{
			FNiagaraEmitterAnalysisData EmitterData;
			AnalyzeEmitter(Handle, EmitterData);
			OutData.Emitters.Add(EmitterData);
		}
	}

	return true;
}

void UBANiagaraAnalyzer::AnalyzeEmitter(const FNiagaraEmitterHandle& InHandle, FNiagaraEmitterAnalysisData& OutEmitterData)
{
	OutEmitterData.EmitterName = InHandle.GetName().ToString();

	FVersionedNiagaraEmitter VersionedEmitter = InHandle.GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;

	if (Emitter == nullptr)
	{
		return;
	}

	FVersionedNiagaraEmitterData* EmitterData = VersionedEmitter.GetEmitterData();

	if (EmitterData == nullptr)
	{
		return;
	}

	OutEmitterData.SimTarget =
		(EmitterData->SimTarget == ENiagaraSimTarget::CPUSim)
		? TEXT("CPU")
		: TEXT("GPU");

	OutEmitterData.bFixedBounds =
		(EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

	// renderer analysis
	for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
	{
		if (Renderer == nullptr || Renderer->GetIsEnabled() == false)
		{
			continue;
		}

		FNiagaraRendererAnalysisData RendererData;

		RendererData.RendererType =
			Renderer->GetClass()->GetName()
			.Replace(TEXT("Niagara"), TEXT(""))
			.Replace(TEXT("RendererProperties"), TEXT(""));

		UMaterialInterface* Material = nullptr;

		if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			Material = SpriteRenderer->Material;

			OutEmitterData.bFaceCamera =
				(SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::FaceCamera);

			OutEmitterData.bVelocityAligned =
				(SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned);

			OutEmitterData.bCustomFacing =
				(SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector);

			RendererData.SortMode =
				UEnum::GetValueAsString(SpriteRenderer->SortMode);
		}
		else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
		{
			Material = RibbonRenderer->Material;
		}
		else if (UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
		{
			if (MeshRenderer->Meshes.Num() > 0)
			{
				Material =
					MeshRenderer->Meshes[0].Mesh != nullptr
					? MeshRenderer->Meshes[0].Mesh->GetMaterial(0)
					: nullptr;
			}
		}

		if (Material != nullptr)
		{
			RendererData.MaterialName = Material->GetName();
			RendererData.BlendMode = UEnum::GetValueAsString(Material->GetBlendMode());

			if (RendererData.MaterialName.Contains(TEXT("Distort"), ESearchCase::IgnoreCase) ||
				RendererData.MaterialName.Contains(TEXT("Refraction"), ESearchCase::IgnoreCase))
			{
				RendererData.bIsDistortion = true;
			}
		}

		OutEmitterData.Renderers.Add(RendererData);
	}

	ClassifyEmitter(OutEmitterData);

	// 🔥 FIX: 의미를 "CullDistance" → "Scalability Proxy Flag"로 명확화
	OutEmitterData.bHasCullDistance =
		(EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

	CheckReadabilityWarnings(OutEmitterData);
}

void UBANiagaraAnalyzer::ClassifyEmitter(FNiagaraEmitterAnalysisData& OutEmitterData)
{
	FString Name = OutEmitterData.EmitterName.ToLower();

	bool bHasSprite = false;
	bool bHasRibbon = false;
	bool bHasMesh = false;
	bool bHasDistortion = false;
	bool bHasAdditive = false;

	for (const auto& Renderer : OutEmitterData.Renderers)
	{
		if (Renderer.RendererType.Contains(TEXT("Sprite")))
		{
			bHasSprite = true;
		}

		if (Renderer.RendererType.Contains(TEXT("Ribbon")))
		{
			bHasRibbon = true;
		}

		if (Renderer.RendererType.Contains(TEXT("Mesh")))
		{
			bHasMesh = true;
		}

		if (Renderer.bIsDistortion == true)
		{
			bHasDistortion = true;
		}

		if (Renderer.BlendMode.Contains(TEXT("Additive")))
		{
			bHasAdditive = true;
		}
	}

	if (bHasDistortion == true || Name.Contains(TEXT("distort")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Distortion;
	}
	else if (Name.Contains(TEXT("spark")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Spark;
	}
	else if (Name.Contains(TEXT("smoke")) || Name.Contains(TEXT("mist")) || Name.Contains(TEXT("fog")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Smoke;
	}
	else if (Name.Contains(TEXT("flash")) || (bHasSprite == true && bHasAdditive == true && Name.Contains(TEXT("glow"))))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Flash;
	}
	else if (Name.Contains(TEXT("debris")) || Name.Contains(TEXT("rock")) || Name.Contains(TEXT("dust")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Debris;
	}
	else if (bHasRibbon == true || Name.Contains(TEXT("trail")) || Name.Contains(TEXT("flow")))
	{
		OutEmitterData.Category =
			bHasRibbon == true
			? ENiagaraEmitterCategory::Ribbon
			: ENiagaraEmitterCategory::Flow;
	}
	else if (Name.Contains(TEXT("ring")) || Name.Contains(TEXT("circle")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Ring;
	}
	else
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Unknown;
	}
}

void UBANiagaraAnalyzer::CheckReadabilityWarnings(FNiagaraEmitterAnalysisData& OutEmitterData)
{
	if (OutEmitterData.LifetimeMax > 0.25f)
	{
		OutEmitterData.Warnings.Add(TEXT("Long Lifetime Risk"));
	}

	if (OutEmitterData.SpriteSizeMax.X > 300.0f ||
		OutEmitterData.SpriteSizeMax.Y > 300.0f)
	{
		OutEmitterData.Warnings.Add(TEXT("Screen Pollution Risk"));
	}

	if (OutEmitterData.VelocityMode.Contains(TEXT("Random")) ||
		OutEmitterData.VelocityMode.Contains(TEXT("Sphere")))
	{
		OutEmitterData.Warnings.Add(TEXT("Omni Directional Spread"));
	}

	if (OutEmitterData.Category == ENiagaraEmitterCategory::Distortion &&
		OutEmitterData.LifetimeMax > 0.05f)
	{
		OutEmitterData.Warnings.Add(TEXT("Lingering Distortion"));
	}

	int32 AdditiveCount = 0;

	for (const auto& Renderer : OutEmitterData.Renderers)
	{
		if (Renderer.BlendMode.Contains(TEXT("Additive")))
		{
			AdditiveCount++;
		}
	}

	if (AdditiveCount >= 3)
	{
		OutEmitterData.Warnings.Add(TEXT("Additive Layer Overuse"));
	}

	FString Name = OutEmitterData.EmitterName.ToLower();

	if (Name.Contains(TEXT("smoke")) ||
		Name.Contains(TEXT("mist")) ||
		Name.Contains(TEXT("fog")) ||
		Name.Contains(TEXT("haze")))
	{
		OutEmitterData.Warnings.Add(TEXT("Gameplay Readability Obstruction"));
	}
	else
	{
		for (const auto& Renderer : OutEmitterData.Renderers)
		{
			FString MatName = Renderer.MaterialName.ToLower();

			if (MatName.Contains(TEXT("smoke")) ||
				MatName.Contains(TEXT("mist")) ||
				MatName.Contains(TEXT("fog")) ||
				MatName.Contains(TEXT("haze")))
			{
				OutEmitterData.Warnings.Add(TEXT("Gameplay Readability Obstruction"));
				break;
			}
		}
	}
}