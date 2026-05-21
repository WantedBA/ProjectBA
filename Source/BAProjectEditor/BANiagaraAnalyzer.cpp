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

    // 시뮬레이션 대상(CPU/GPU) 저장
    OutEmitterData.SimTarget = (EmitterData->SimTarget == ENiagaraSimTarget::CPUSim) ? TEXT("CPU") : TEXT("GPU");

    // 고정 Bounds 사용 여부 저장
    OutEmitterData.bFixedBounds = (EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

    // 1. Spawn 데이터 초기화
    OutEmitterData.SpawnData.BurstCount = 0;
    OutEmitterData.SpawnData.SpawnRate = 0.0f;
    OutEmitterData.SpawnData.bHasMultiBurst = false;

    // 2. 기본 파티클 속성 초기화
    // 수명 기본값
    OutEmitterData.LifetimeMin = 0.05f;
    OutEmitterData.LifetimeMax = 0.35f;

    // 스프라이트 크기 기본값
    OutEmitterData.SpriteSizeMin = FVector2D::ZeroVector;
    OutEmitterData.SpriteSizeMax = FVector2D(400.0f, 400.0f);

    // 기본 속도 모드
    OutEmitterData.VelocityMode = TEXT("Static");

    // Facing/Alignment 상태 초기화
    OutEmitterData.bFaceCamera = false;
    OutEmitterData.bVelocityAligned = false;
    OutEmitterData.bCustomFacing = false;

    // 3. Renderer 분석
    // 현재 이미터에 연결된 모든 Renderer 가져오기
    const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();

    for (UNiagaraRendererProperties* Renderer : Renderers)
    {
        if (Renderer == nullptr)
        {
            continue;
        }

        // 비활성 Renderer 제외
        if (!Renderer->GetIsEnabled())
        {
            continue;
        }

        // Velocity / Facing 모드 분석

        if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
        {
            // Velocity 방향 정렬 여부
            if (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
            {
                OutEmitterData.VelocityMode = TEXT("VelocityAligned");
            }
            // 카메라 Facing 여부
            else if (
                SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::FaceCamera)
            {
                OutEmitterData.VelocityMode = TEXT("Random/Spherical");
            }
        }

        // Renderer 상세 데이터 분석
        FNiagaraRendererAnalysisData RendererData;

        // 클래스 이름 기반 Renderer 타입 추출
        RendererData.RendererType =
            Renderer->GetClass()->GetName()
            .Replace(TEXT("Niagara"), TEXT(""))
            .Replace(TEXT("RendererProperties"), TEXT(""));

        // 머티리얼 포인터
        UMaterialInterface* Material = nullptr;

        // Sprite Renderer 분석
        if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
        {
            Material = SpriteRenderer->Material;

            // 카메라 Facing 여부
            OutEmitterData.bFaceCamera = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::FaceCamera);

            // Velocity 정렬 여부
            OutEmitterData.bVelocityAligned = (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned);

            // 사용자 지정 Facing 여부
            OutEmitterData.bCustomFacing = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector);

            // Sort Mode 문자열 저장
            RendererData.SortMode = UEnum::GetValueAsString(SpriteRenderer->SortMode);
        }
        // Ribbon Renderer 분석
        else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
        {
            Material = RibbonRenderer->Material;
        }
        // Mesh Renderer 분석
        else if (UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
        {
            // 첫 번째 메시 존재 여부 검사
            if (MeshRenderer->Meshes.IsValidIndex(0))
            {
                if (MeshRenderer->Meshes[0].Mesh != nullptr)
                {
                    // 메시의 첫 번째 머티리얼 추출
                    Material =
                        MeshRenderer->Meshes[0]
                        .Mesh
                        ->GetMaterial(0);
                }
            }
        }

        // 머티리얼 분석
        if (Material != nullptr)
        {
            // 머티리얼 이름 저장
            RendererData.MaterialName = Material->GetName();

            // BlendMode 문자열 저장
            RendererData.BlendMode = UEnum::GetValueAsString(Material->GetBlendMode());

            // 왜곡 계열 머티리얼 여부 검사
            if (RendererData.MaterialName.Contains(TEXT("Distort"), ESearchCase::IgnoreCase) ||
                RendererData.MaterialName.Contains(TEXT("Refraction"),ESearchCase::IgnoreCase))
            {
                RendererData.bIsDistortion = true;
            }
        }

        // 분석 결과 추가
        OutEmitterData.Renderers.Add(RendererData);
    }

    // 4. 이미터 분류 수행
    ClassifyEmitter(OutEmitterData);

    // 5. Cull/Bounds 정보 저장
    OutEmitterData.bHasCullDistance = (EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);

    // 6. 경고 및 품질 검사
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