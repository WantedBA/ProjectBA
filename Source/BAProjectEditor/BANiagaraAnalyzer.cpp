// Copyright TeamBA. All Rights Reserved.

#include "BANiagaraAnalyzer.h"
#include "NiagaraEmitter.h"
#include "NiagaraCommon.h"
#include "NiagaraSystem.h"
#include "NiagaraScript.h"
#include "NiagaraPlatformSet.h"
#include "NiagaraDataInterface.h"
#include "NiagaraParameterStore.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "Materials/MaterialInterface.h"

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

	// 시스템 파라미터(User Exposed) 추출
	const FNiagaraUserRedirectionParameterStore& UserParams = InSystem->GetExposedParameters();
	TArray<FNiagaraVariable> OutVariables;
	UserParams.GetParameters(OutVariables);

	for (const FNiagaraVariable& Var : OutVariables)
	{
		OutData.SystemParameters.Add(Var.GetName().ToString(), Var.GetType().GetName());
	}

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

	// 0. 모듈 및 파라미터 정보 분석
	TArray<UNiagaraScript*> Scripts;
	Scripts.Add(EmitterData->SpawnScriptProps.Script);
	Scripts.Add(EmitterData->UpdateScriptProps.Script);

	for (UNiagaraScript* Script : Scripts)
	{
		if (Script)
		{
			OutEmitterData.ModuleNames.Add(Script->GetName());
			
			// 스크립트 변수 및 바인딩 추적
			const FNiagaraParameterStore& Params = Script->RapidIterationParameters;
			for (const FNiagaraVariable& Var : Params.ReadParameterVariables())
			{
				FString VarName = Var.GetName().ToString();
				
				// 바인딩된 변수 식별 (Data 흐름 파악)
				if (VarName.Contains(TEXT("Bound")) || VarName.Contains(TEXT("Link")))
				{
					OutEmitterData.BoundParameters.Add(VarName);
				}

				if (VarName.Contains(TEXT("Spawn")) || VarName.Contains(TEXT("Rate")) || 
					VarName.Contains(TEXT("Lifetime")) || VarName.Contains(TEXT("Size")))
				{
					OutEmitterData.ParameterValues.Add(VarName, Var.GetType().GetName());
				}
			}

			// 데이터 인터페이스 분석
			const TArray<UNiagaraDataInterface*>& DataInterfaces = Params.GetDataInterfaces();
			for (UNiagaraDataInterface* DI : DataInterfaces)
			{
				if (DI == nullptr)
				{
					continue;
				}

				OutEmitterData.DataInterfaces.Add(DI->GetClass()->GetName());
			}
		}
	}

	// 복잡도 점수 산출 (Heuristic)
	float Score = (float)OutEmitterData.ModuleNames.Num() * 2.0f;
	Score += (float)OutEmitterData.Renderers.Num() * 5.0f;
	Score += OutEmitterData.SimTarget == TEXT("GPU") ? 10.0f : 20.0f; // CPU 시뮬레이션이 일반적으로 더 무거움
	Score += (float)OutEmitterData.DataInterfaces.Num() * 15.0f;
	OutEmitterData.ComplexityScore = FMath::Clamp(Score, 0.0f, 100.0f);

	// 시뮬레이션 대상(CPU/GPU) 저장
	OutEmitterData.SimTarget = (EmitterData->SimTarget == ENiagaraSimTarget::CPUSim) ? TEXT("CPU") : TEXT("GPU");

	// 고정 Bounds 사용 여부 저장
	OutEmitterData.bFixedBounds = (EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

	// 1. Spawn 데이터 초기화
	OutEmitterData.SpawnData.BurstCount = 0;
	OutEmitterData.SpawnData.SpawnRate = 0.0f;
	OutEmitterData.SpawnData.bHasMultiBurst = false;

	// 2. 기본 파티클 속성 초기화
	OutEmitterData.LifetimeMin = 0.05f;
	OutEmitterData.LifetimeMax = 0.35f;

	OutEmitterData.SpriteSizeMin = FVector2D::ZeroVector;
	OutEmitterData.SpriteSizeMax = FVector2D(400.0f, 400.0f);

	OutEmitterData.VelocityMode = TEXT("Static");

	OutEmitterData.bFaceCamera = false;
	OutEmitterData.bVelocityAligned = false;
	OutEmitterData.bCustomFacing = false;

	// 3. Renderer 분석
	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();

	for (UNiagaraRendererProperties* Renderer : Renderers)
	{
		if (Renderer == nullptr || !Renderer->GetIsEnabled())
		{
			continue;
		}

		FNiagaraRendererAnalysisData RendererData;
		RendererData.RendererType = Renderer->GetClass()->GetName()
			.Replace(TEXT("Niagara"), TEXT(""))
			.Replace(TEXT("RendererProperties"), TEXT(""));

		UMaterialInterface* Material = nullptr;

		if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			Material = SpriteRenderer->Material;
			OutEmitterData.bFaceCamera = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::FaceCamera);
			OutEmitterData.bVelocityAligned = (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned);
			OutEmitterData.bCustomFacing = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector);
			RendererData.SortMode = UEnum::GetValueAsString(SpriteRenderer->SortMode);
		}
		else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
		{
			Material = RibbonRenderer->Material;
		}
		else if (UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
		{
			if (MeshRenderer->Meshes.IsValidIndex(0) && MeshRenderer->Meshes[0].Mesh != nullptr)
			{
				Material = MeshRenderer->Meshes[0].Mesh->GetMaterial(0);
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

			AnalyzeMaterialParameters(Material, RendererData);
		}

		OutEmitterData.Renderers.Add(RendererData);
	}

	// 4. 이미터 분류 수행
	ClassifyEmitter(OutEmitterData);

	// 5. Cull/Bounds 정보 저장
	OutEmitterData.bHasCullDistance = (EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

	// 6. 경고 및 품질 검사
	CheckReadabilityWarnings(OutEmitterData);
}

void UBANiagaraAnalyzer::AnalyzeMaterialParameters(UMaterialInterface* InMaterial, FNiagaraRendererAnalysisData& OutRendererData)
{
	if (!InMaterial)
	{
		return;
	}

	// 1. 스칼라 파라미터 추출
	TArray<FMaterialParameterInfo> ScalarInfo;
	TArray<FGuid> ScalarIds;
	InMaterial->GetAllScalarParameterInfo(ScalarInfo, ScalarIds);

	for (const FMaterialParameterInfo& Info : ScalarInfo)
	{
		float Value;
		if (InMaterial->GetScalarParameterValue(Info, Value))
		{
			OutRendererData.ScalarParameters.Add(Info.Name.ToString(), Value);
		}
	}

	// 2. 벡터 파라미터 추출
	TArray<FMaterialParameterInfo> VectorInfo;
	TArray<FGuid> VectorIds;
	InMaterial->GetAllVectorParameterInfo(VectorInfo, VectorIds);

	for (const FMaterialParameterInfo& Info : VectorInfo)
	{
		FLinearColor Value;
		if (InMaterial->GetVectorParameterValue(Info, Value))
		{
			OutRendererData.VectorParameters.Add(Info.Name.ToString(), Value);
		}
	}

	// 3. 텍스처 파라미터 추출
	TArray<FMaterialParameterInfo> TextureInfo;
	TArray<FGuid> TextureIds;
	InMaterial->GetAllTextureParameterInfo(TextureInfo, TextureIds);

	for (const FMaterialParameterInfo& Info : TextureInfo)
	{
		UTexture* Value;
		if (InMaterial->GetTextureParameterValue(Info, Value))
		{
			if (Value)
			{
				OutRendererData.TextureParameters.Add(Info.Name.ToString(), Value->GetName());
			}
		}
	}
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
	else if (Name.Contains(TEXT("spark")) || Name.Contains(TEXT("spake")))
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
