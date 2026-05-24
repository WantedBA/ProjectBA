// Copyright TeamBA. All Rights Reserved.

#include "BANiagaraAnalyzer.h"
#include "NiagaraEmitter.h"
#include "NiagaraCommon.h"
#include "NiagaraSystem.h"
#include "NiagaraScript.h"
#include "NiagaraEffectType.h"
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

	// 시스템 스케일러비티(Cull Distance) 분석 데이터
	bool bSystemCullEnabled = false;
	float SystemMaxCullDistance = 0.0f;

	const UNiagaraEffectType* EffectType = InSystem->GetEffectType();
	if (EffectType)
	{
		const FNiagaraSystemScalabilitySettingsArray& ScalabilitySettings = EffectType->SystemScalabilitySettings;
		if (ScalabilitySettings.Settings.Num() > 0)
		{
			const FNiagaraSystemScalabilitySettings& BaseSettings = ScalabilitySettings.Settings[0];

			bSystemCullEnabled = BaseSettings.bCullByDistance;
			SystemMaxCullDistance = BaseSettings.MaxDistance;
		}
	}

	OutData.bUsesDistanceCull = bSystemCullEnabled;
	OutData.CullDistance = SystemMaxCullDistance;

	// 시스템 파라미터(User Exposed) 추출
	const FNiagaraUserRedirectionParameterStore& UserParams = InSystem->GetExposedParameters();
	TArray<FNiagaraVariable> OutVariables;
	UserParams.GetParameters(OutVariables);

	for (const FNiagaraVariable& Var : OutVariables)
	{
		OutData.SystemParameters.Add(Var.GetName().ToString(), Var.GetType().GetName());
	}

	OutData.SystemSemanticTags.Add(TEXT("FX_System"));

	// Emitters
	for (const FNiagaraEmitterHandle& Handle : InSystem->GetEmitterHandles())
	{
		if (Handle.GetIsEnabled() == true)
		{
			FNiagaraEmitterAnalysisData EmitterData;

			EmitterData.bHasCullDistance = bSystemCullEnabled;
			EmitterData.MaxCullDistance = SystemMaxCullDistance;

			AnalyzeEmitter(Handle, EmitterData);
			OutData.Emitters.Add(EmitterData);
		}
	}

	// 시스템 총 비용 합산
	float TotalCost = 0.0f;
	for (const auto& E : OutData.Emitters)
	{
		TotalCost += E.EstimatedGPUCost;
	}

	OutData.TotalEstimatedCost = TotalCost;

	return true;
}

void UBANiagaraAnalyzer::AnalyzeEmitter(const FNiagaraEmitterHandle& InHandle, FNiagaraEmitterAnalysisData& OutEmitterData)
{
	OutEmitterData.EmitterName = InHandle.GetName().ToString();
	OutEmitterData.bIsEnabled = InHandle.GetIsEnabled();

	FVersionedNiagaraEmitter VersionedEmitter = InHandle.GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (Emitter == nullptr)
	{
		return;
	}

	OutEmitterData.MotionPattern = DetectMotionPattern(OutEmitterData.EmitterName);

	FVersionedNiagaraEmitterData* EmitterData = VersionedEmitter.GetEmitterData();
	if (EmitterData == nullptr)
	{
		return;
	}

	int32 ResolvedMaxParticles = EmitterData->GetMaxInstanceCount();
	OutEmitterData.ExplicitMaxParticleCount = ResolvedMaxParticles;

	// 기본 파라미터 초기화값 설정 (폴백)
	OutEmitterData.LifetimeMin = 0.0f;
	OutEmitterData.LifetimeMax = 0.0f;
	OutEmitterData.VelocityMode = TEXT("Static");

	OutEmitterData.SpawnData.SpawnRate = 0.0f;
	OutEmitterData.SpawnData.BurstCount = 0;
	OutEmitterData.SpawnData.bHasMultiBurst = false;
	OutEmitterData.SpawnData.EstimatedMaxParticles = (float)OutEmitterData.ExplicitMaxParticleCount;

	// 정밀 스폰 세팅 추적용 플래그
	bool bHasSpawnRateModule = false;
	bool bHasBurstModule = false;

	// 0. 모듈 및 파라미터 정보 분석
	TArray<UNiagaraScript*> Scripts;
	Scripts.Add(EmitterData->SpawnScriptProps.Script);
	Scripts.Add(EmitterData->UpdateScriptProps.Script);
	Scripts.Add(EmitterData->EmitterSpawnScriptProps.Script);
	Scripts.Add(EmitterData->EmitterUpdateScriptProps.Script);

	for (UNiagaraScript* Script : Scripts)
	{
		if (Script == nullptr)
		{
			continue;
		}

		FString ScriptName = Script->GetName();
		OutEmitterData.ModuleNames.AddUnique(Script->GetName());

		// 모듈 이름 기반 스폰 메커니즘 1차 수집
		if (ScriptName.Contains(TEXT("SpawnRate")))
		{
			bHasSpawnRateModule = true;
		}
		if (ScriptName.Contains(TEXT("Burst")))
		{
			bHasBurstModule = true;
		}

		// Rapid Iteration 파라미터 스토어 분석
		const FNiagaraParameterStore& Params = Script->RapidIterationParameters;
		TArray<FNiagaraVariable> InputVariables;
		Params.GetParameters(InputVariables);

		for (const FNiagaraVariable& Var : InputVariables)
		{
			if (Var.GetType().IsValid() == false)
			{
				continue;
			}

			FString VarName = Var.GetName().ToString();
			FString TypeName = Var.GetType().GetName();

			// 데이터 흐름 파악용 링크 바인딩 추적
			if (VarName.Contains(TEXT("Bound")) || VarName.Contains(TEXT("Link")) || VarName.Contains(TEXT("Module.")))
			{
				OutEmitterData.BoundParameters.AddUnique(VarName + TEXT(" (") + TypeName + TEXT(")"));
			}

			// 스크립트 단계를 접두어로 붙여 중복 Key로 인한 데이터 유실 및 덮어쓰기 완전 방지
			if (VarName.Contains(TEXT("Spawn")) || VarName.Contains(TEXT("Rate")) ||
				VarName.Contains(TEXT("Lifetime")) || VarName.Contains(TEXT("Size")) || VarName.Contains(TEXT("Burst")))
			{
				OutEmitterData.ParameterValues.Add(ScriptName + TEXT("_") + VarName, TypeName);
			}

			// 실제 세팅 수치 정밀 파싱
			if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
			{
				float FloatVal = Params.GetParameterValue<float>(Var);

				if (VarName.Contains(TEXT("Lifetime")))
				{
					if (VarName.Contains(TEXT("Min"))) 
					{ 
						OutEmitterData.LifetimeMin = FloatVal; 
					}
					else if (VarName.Contains(TEXT("Max"))) 
					{ 
						OutEmitterData.LifetimeMax = FloatVal; 
					}
					else if (OutEmitterData.LifetimeMax == 0.0f)
					{
						OutEmitterData.LifetimeMin = FloatVal;
						OutEmitterData.LifetimeMax = FloatVal;
					}
				}
				else if (VarName.Contains(TEXT("SpawnRate")))
				{
					OutEmitterData.SpawnData.SpawnRate = FloatVal;
				}
				else if (VarName.Contains(TEXT("SpawnPerUnit")))
				{
					// 궤적(Trail) 등 이동 거리 기반 스폰 가중치 감지용 로그 기록 및 변수화 유도
					OutEmitterData.BoundParameters.AddUnique(FString::Printf(TEXT("SpawnPerUnit_Value: %.2f"), FloatVal));
				}
				else if (VarName.Contains(TEXT("BurstInterval")))
				{
					OutEmitterData.SpawnData.BurstInterval = FloatVal;
				}
				else if (VarName.Contains(TEXT("SpriteSize")) || VarName.Contains(TEXT("Size")))
				{
					if (VarName.Contains(TEXT("X"))) 
					{ 
						OutEmitterData.SpriteSizeMax.X = FloatVal;
					}
					if (VarName.Contains(TEXT("Y"))) 
					{ 
						OutEmitterData.SpriteSizeMax.Y = FloatVal; 
					}
				}
			}
			else if (Var.GetType() == FNiagaraTypeDefinition::GetIntDef())
			{
				int32 IntVal = Params.GetParameterValue<int32>(Var);
				if (VarName.Contains(TEXT("BurstCount")))
				{
					OutEmitterData.SpawnData.BurstCount = IntVal;
				}
			}
			else if (Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
			{
				bool bBoolVal = Params.GetParameterValue<bool>(Var);
				if (VarName.Contains(TEXT("Infinite")))
				{
					OutEmitterData.SpawnData.bInfiniteSpawn = bBoolVal;
				}
			}

			// 모듈 활성화 기반 비헤이비어 체크
			if (VarName.Contains(TEXT("CurlNoise")) || VarName.Contains(TEXT("Curl"))) 
			{ 
				OutEmitterData.bHasCurlNoise = true;
			}
			if (VarName.Contains(TEXT("Drag"))) 
			{ 
				OutEmitterData.bHasDrag = true;
			}
			if (VarName.Contains(TEXT("Vortex"))) 
			{ 
				OutEmitterData.bHasVortex = true;
			}
			if (VarName.Contains(TEXT("Collision"))) 
			{ 
				OutEmitterData.bHasCollision = true;
			}

			// 'Velocity', 'Inherit' 조건식을 추가하여 무기 궤적 및 투사체 꼬리 분별
			if (VarName.Contains(TEXT("Gravity")) || VarName.Contains(TEXT("Acceleration")) ||
				VarName.Contains(TEXT("Velocity")) || VarName.Contains(TEXT("Inherit")))
			{
				OutEmitterData.bHasGravity = true;
				OutEmitterData.VelocityMode = TEXT("Dynamic/Accelerated");
			}

			if (VarName.Contains(TEXT("RibbonWidth")) || VarName.Contains(TEXT("WidthScale")))
			{
				if (TypeName.Contains(TEXT("Curve"))) // 커브 컴포넌트나 인터페이스 타입인 경우
				{
					OutEmitterData.bHasRibbonWidthCurve = true;
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
				{
					OutEmitterData.RibbonWidth = Params.GetParameterValue<float>(Var);
				}
			}
		}

		// 데이터 인터페이스 분석
		const TArray<UNiagaraDataInterface*>& DataInterfaces = Params.GetDataInterfaces();
		for (UNiagaraDataInterface* DI : DataInterfaces)
		{
			if (DI != nullptr)
			{
				OutEmitterData.DataInterfaces.Add(DI->GetClass()->GetName());
			}
		}
	}

	// 최종 스폰 형태(Spawn Method Type) 결정 자동화
	if (bHasSpawnRateModule && bHasBurstModule)
	{
		OutEmitterData.SpawnData.SpawnMethodType = ENiagaraSpawnMethodType::Mixed;
	}
	else if (bHasBurstModule)
	{
		OutEmitterData.SpawnData.SpawnMethodType = ENiagaraSpawnMethodType::Burst;
	}
	else
	{
		OutEmitterData.SpawnData.SpawnMethodType = ENiagaraSpawnMethodType::Continuous;
	}

	// 구동 방식 (DriveMode) 판단 로직 삽입 위치
	OutEmitterData.DriveMode = TEXT("Standard SpawnRate");

	bool bHasSkeletalMeshDI = false;
	for (const FString& DI : OutEmitterData.DataInterfaces)
	{
		if (DI.Contains(TEXT("SkeletalMesh")))
		{
			bHasSkeletalMeshDI = true;
			break;
		}
	}

	bool bHasSpawnPerUnit = false;
	for (const FString& Module : OutEmitterData.ModuleNames)
	{
		if (Module.Contains(TEXT("SpawnPerUnit")))
		{
			bHasSpawnPerUnit = true;
			break;
		}
	}

	if (bHasSkeletalMeshDI && bHasSpawnPerUnit)
	{
		OutEmitterData.DriveMode = TEXT("AnimNotifyState_Trail Linked (Skeletal Mesh Socket)");
	}
	else if (bHasSkeletalMeshDI)
	{
		OutEmitterData.DriveMode = TEXT("Skeletal Mesh Attachment (Continuous)");
	}
	else if (bHasSpawnPerUnit)
	{
		OutEmitterData.DriveMode = TEXT("Movement-based Spawn (Spawn Per Unit)");
	}

	// 시뮬레이션 대상(CPU/GPU) 및 공간 정보 저장
	OutEmitterData.SimTarget = (EmitterData->SimTarget == ENiagaraSimTarget::CPUSim) ? TEXT("CPU") : TEXT("GPU");
	OutEmitterData.bFixedBounds = (EmitterData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed);
	OutEmitterData.bWorldSpace = (EmitterData->bLocalSpace == false);

	// Renderer 및 머티리얼 상세 분석
	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	for (UNiagaraRendererProperties* Renderer : Renderers)
	{
		if (Renderer == nullptr || !Renderer->GetIsEnabled())
		{
			continue;
		}

		FNiagaraRendererAnalysisData RendererData;
		RendererData.RendererType = Renderer->GetClass()->GetName().Replace(TEXT("Niagara"), TEXT("")).Replace(TEXT("RendererProperties"), TEXT(""));

		UMaterialInterface* Material = nullptr;

		if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			Material = SpriteRenderer->Material;
			OutEmitterData.bFaceCamera = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::FaceCamera);
			OutEmitterData.bCameraFacing = OutEmitterData.bFaceCamera;
			OutEmitterData.bVelocityAligned = (SpriteRenderer->Alignment == ENiagaraSpriteAlignment::VelocityAligned);
			OutEmitterData.bCustomFacing = (SpriteRenderer->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector);
			RendererData.SortMode = UEnum::GetValueAsString(SpriteRenderer->SortMode);

			// 동적 추적 실패 시에만 작동할 기본 안전 폴백 최소값 지정
			if (OutEmitterData.SpriteSizeMax.IsZero()) {
				OutEmitterData.SpriteSizeMin = FVector2D(10.0f, 10.0f);
				OutEmitterData.SpriteSizeMax = FVector2D(100.0f, 100.0f);
			}
		}
		else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
		{
			Material = RibbonRenderer->Material;
			OutEmitterData.bVelocityAligned = (RibbonRenderer->FacingMode == ENiagaraRibbonFacingMode::Screen);
			OutEmitterData.BoundParameters.AddUnique(FString::Printf(TEXT("Ribbon_Tessellation_Slices: %.2f"), RibbonRenderer->CurveTension));

			// 리본 기본 두께 추출 (렌더러 바인딩 값 확인)
			OutEmitterData.RibbonWidth = RibbonRenderer->RibbonWidthBinding.GetParamMapBindableVariable().GetName() == NAME_None ? 10.0f : 0.0f;
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
			RendererData.bIsTwoSided = Material->IsTwoSided();

			// 머티리얼 라이팅 연산 여부 추출 (Unlit 여부)
			RendererData.bIsUnlitMaterial = (Material->GetShadingModels() == MSM_Unlit);
			if (RendererData.MaterialName.Contains(TEXT("Distort"), ESearchCase::IgnoreCase) ||
				RendererData.MaterialName.Contains(TEXT("Refraction"), ESearchCase::IgnoreCase))
			{
				RendererData.bIsDistortion = true;
				RendererData.bUsesDistortion = true;
			}

			AnalyzeMaterialParameters(Material, RendererData);

			// 클리핑 버그 위험 판단 (DepthFade가 미사용된 반투명)
			if (!RendererData.bUsesDepthFade && !RendererData.bUsesSoftParticle)
			{
				if (RendererData.BlendMode.Contains(TEXT("Translucent")) || RendererData.BlendMode.Contains(TEXT("Additive")))
				{
					OutEmitterData.bDepthFadeDisabledRisk = true;
				}
			}
		}

		OutEmitterData.Renderers.Add(RendererData);
	}

	// 복잡도 점수 포화(포화 한계점 100점 점령) 현상 제어 공식 고도화, 로그 배수(Loge) 가중치를 활용하여 대량 파티클 스폰 시에도 변별력을 정밀하게 유지
	float BaseParticleCount = FMath::Max(1.0f, OutEmitterData.SpawnData.EstimatedMaxParticles);

	float StructuralScore = (float)OutEmitterData.ModuleNames.Num() * 1.5f;
	StructuralScore += (float)OutEmitterData.Renderers.Num() * 4.0f;
	StructuralScore += (OutEmitterData.SimTarget == TEXT("GPU")) ? 5.0f : 15.0f;
	StructuralScore += (float)OutEmitterData.DataInterfaces.Num() * 10.0f;

	// 로그 함수를 통해 수천 개의 파티클 분출 시 부하 점수가 무한대로 폭발(포화)하지 않도록 유연하게 압축 처리
	float DensityMultiplier = 1.0f + (FMath::Loge(BaseParticleCount) * 0.15f);
	float FinalScore = StructuralScore * DensityMultiplier;

	float EstimatedArea = OutEmitterData.SpriteSizeMax.X * OutEmitterData.SpriteSizeMax.Y;
	float Coverage = (EstimatedArea * OutEmitterData.SpawnData.EstimatedMaxParticles) / 1000000.0f;

	OutEmitterData.EstimatedScreenCoverage = FMath::Clamp(Coverage, 0.0f, 1.0f);
	OutEmitterData.bCenterScreenDominant = (OutEmitterData.EstimatedScreenCoverage > 0.4f);
	OutEmitterData.bPeripheralFX = (OutEmitterData.EstimatedScreenCoverage < 0.1f);


	OutEmitterData.ComplexityScore = FMath::Clamp(FinalScore, 0.0f, 100.0f);

	// 최종 GPU 비용과 오버드로우 예측값 보정
	OutEmitterData.EstimatedGPUCost = OutEmitterData.ComplexityScore * 0.5f;
	OutEmitterData.EstimatedOverdraw = OutEmitterData.bFaceCamera ? 2.5f : 1.0f;

	// 시맨틱 및 경고 처리
	ClassifyEmitter(OutEmitterData);
	CheckReadabilityWarnings(OutEmitterData);

	OutEmitterData.TemporalBehavior = DetectTemporalBehavior(OutEmitterData);
	OutEmitterData.GameplayRole = DetectGameplayRole(OutEmitterData);
	if (OutEmitterData.Renderers.Num() > 0)
	{
		OutEmitterData.ColorSemantic = DetectColorSemantic(OutEmitterData.Renderers[0]);
	}
	OutEmitterData.bFrontLoadedEffect = (OutEmitterData.SpawnData.SpawnMethodType ==ENiagaraSpawnMethodType::Burst);
	OutEmitterData.bTrailingPersistence = (OutEmitterData.LifetimeMax > 1.0f);

	OutEmitterData.SemanticTags.Add(TEXT("Analyzed_Emitter"));
}

void UBANiagaraAnalyzer::AnalyzeMaterialParameters(UMaterialInterface* InMaterial, FNiagaraRendererAnalysisData& OutRendererData)
{
	if (InMaterial == nullptr)
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
			FString ParamName = Info.Name.ToString().ToLower();
			OutRendererData.ScalarParameters.Add(Info.Name.ToString(), Value);
			if (ParamName.Contains(TEXT("depthfade")) || ParamName.Contains(TEXT("fadedist")) || ParamName.Contains(TEXT("soft")))
			{
				OutRendererData.bUsesDepthFade = true;
				OutRendererData.bUsesSoftParticle = true;
			}
			if (ParamName.Contains(TEXT("fresnel")) || ParamName.Contains(TEXT("rim")) || ParamName.Contains(TEXT("fresnelpower")))
			{
				OutRendererData.bUsesFresnel = true;
			}
			if (ParamName.Contains(TEXT("distort")) || ParamName.Contains(TEXT("refract")) || ParamName.Contains(TEXT("bump")))
			{
				OutRendererData.bUsesDistortion = true;
			}
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
	else if (Name.Contains(TEXT("spark")) || Name.Contains(TEXT("spake")) || Name.Contains(TEXT("ember")))
	{
		OutEmitterData.Category = ENiagaraEmitterCategory::Spark;
	}
	else if (Name.Contains(TEXT("smoke")) || Name.Contains(TEXT("mist")) || Name.Contains(TEXT("fog")) || Name.Contains(TEXT("ash")) || Name.Contains(TEXT("dissolve")))
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
	else if (bHasRibbon == true || Name.Contains(TEXT("trail")) || Name.Contains(TEXT("flow")) || Name.Contains(TEXT("soul")) || Name.Contains(TEXT("wisp")))
	{
		OutEmitterData.Category = bHasRibbon == true ? ENiagaraEmitterCategory::Ribbon: ENiagaraEmitterCategory::Flow;
	}
	else if (Name.Contains(TEXT("ring")) || Name.Contains(TEXT("circle")) || Name.Contains(TEXT("shockwave")))
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
	if (OutEmitterData.SimTarget == TEXT("GPU"))
	{
		for (const auto& Renderer : OutEmitterData.Renderers)
		{
			if (Renderer.RendererType.Contains(TEXT("Ribbon")))
			{
				OutEmitterData.Warnings.Add(TEXT("CRITICAL: GPU Ribbon Renderer Crash/Bug Risk. (Switch to CPU Sim Target)"));
			}
		}
	}
	if (OutEmitterData.LifetimeMax > 0.25f)
	{
		OutEmitterData.Warnings.Add(TEXT("Long Lifetime Risk"));
	}

	if (OutEmitterData.SpriteSizeMax.X > 300.0f || OutEmitterData.SpriteSizeMax.Y > 300.0f)
	{
		OutEmitterData.Warnings.Add(TEXT("Screen Pollution Risk"));
	}

	if (OutEmitterData.VelocityMode.Contains(TEXT("Random")) || OutEmitterData.VelocityMode.Contains(TEXT("Sphere")))
	{
		OutEmitterData.Warnings.Add(TEXT("Omni Directional Spread"));
	}

	if (OutEmitterData.Category == ENiagaraEmitterCategory::Distortion && OutEmitterData.LifetimeMax > 0.05f)
	{
		OutEmitterData.Warnings.Add(TEXT("Lingering Distortion"));
	}

	int32 AdditiveCount = 0;
	bool bLitMaterialFound = false;
	for (const auto& Renderer : OutEmitterData.Renderers)
	{
		if (Renderer.BlendMode.Contains(TEXT("Additive")))
		{
			AdditiveCount++;
		}
		if (Renderer.bIsUnlitMaterial == false)// 라이팅 연산을 켜둔 반투명 이미터 경고 추가
		{
			bLitMaterialFound = true;
		}
	}

	if (AdditiveCount >= 3)
	{
		OutEmitterData.Warnings.Add(TEXT("Additive Layer Overuse"));
	}

	if (bLitMaterialFound)
	{
		OutEmitterData.Warnings.Add(TEXT("HIGH COST RISK: Translucent Lit Shading Model detected. (Switch to Unlit to optimize GPU Draw)"));
	}

	// 하드 엣지 클리핑 버그 위험 경고 바인딩
	if (OutEmitterData.bDepthFadeDisabledRisk && (OutEmitterData.SpriteSizeMax.X > 150.0f || OutEmitterData.SpriteSizeMax.Y > 150.0f))
	{
		OutEmitterData.Warnings.Add(TEXT("VISUAL BUG RISK: Hard Edge Clipping. Large transulcent particle requires Depth Fade in Material."));
	}

	// 컬링 미세팅 경고 바인딩
	if (!OutEmitterData.bHasCullDistance)
	{
		OutEmitterData.Warnings.Add(TEXT("OPTIMIZATION RISK: Distance Cull Settings Not Configured. Background simulation will drain CPU/GPU resources."));
	}
}

ENiagaraTemporalBehavior UBANiagaraAnalyzer::DetectTemporalBehavior(const FNiagaraEmitterAnalysisData& Data)
{
	if (Data.SpawnData.bInfiniteSpawn)
	{
		return ENiagaraTemporalBehavior::Looping;
	}

	if (Data.SpawnData.SpawnMethodType == ENiagaraSpawnMethodType::Burst)
	{
		if (Data.LifetimeMax <= 0.2f)
		{
			return ENiagaraTemporalBehavior::Instant;
		}

		return ENiagaraTemporalBehavior::FadeOut;
	}

	if (Data.SpawnData.SpawnRate > 0.0f && Data.LifetimeMax > 1.0f)
	{
		return ENiagaraTemporalBehavior::Sustain;
	}

	return ENiagaraTemporalBehavior::Unknown;
}

ENiagaraGameplayRole UBANiagaraAnalyzer::DetectGameplayRole(const FNiagaraEmitterAnalysisData& Data)
{
	const FString Name = Data.EmitterName.ToLower();

	if (Name.Contains(TEXT("impact")))
	{
		return ENiagaraGameplayRole::HitImpact;
	}

	if (Name.Contains(TEXT("trail")) || Data.bVelocityAligned)
	{
		return ENiagaraGameplayRole::ProjectileTrail;
	}

	if (Name.Contains(TEXT("aura")))
	{
		return ENiagaraGameplayRole::BuffAura;
	}

	if (Name.Contains(TEXT("explosion")))
	{
		return ENiagaraGameplayRole::Explosion;
	}

	if (Name.Contains(TEXT("slash")))
	{
		return ENiagaraGameplayRole::WeaponSwing;
	}

	return ENiagaraGameplayRole::Unknown;
}

ENiagaraColorSemantic UBANiagaraAnalyzer::DetectColorSemantic(const FNiagaraRendererAnalysisData& Renderer)
{
	const FString Mat = Renderer.MaterialName.ToLower();

	if (Mat.Contains(TEXT("fire")) || Mat.Contains(TEXT("flame")))
	{
		return ENiagaraColorSemantic::Fire;
	}

	if (Mat.Contains(TEXT("ice")) || Mat.Contains(TEXT("frost")))
	{
		return ENiagaraColorSemantic::Ice;
	}

	if (Mat.Contains(TEXT("electric")) || Mat.Contains(TEXT("lightning")))
	{
		return ENiagaraColorSemantic::Electric;
	}

	if (Mat.Contains(TEXT("poison")))
	{
		return ENiagaraColorSemantic::Poison;
	}

	if (Mat.Contains(TEXT("smoke")))
	{
		return ENiagaraColorSemantic::Smoke;
	}

	return ENiagaraColorSemantic::Neutral;
}

ENiagaraMotionPattern UBANiagaraAnalyzer::DetectMotionPattern(const FString& Name)
{
	FString N = Name.ToLower();

	if (N.Contains("orbit"))
	{
		return ENiagaraMotionPattern::Orbit;
	}

	if (N.Contains("vortex"))
	{
		return ENiagaraMotionPattern::Vortex;
	}

	if (N.Contains("noise") || N.Contains("turb"))
	{
		return ENiagaraMotionPattern::Turbulence;
	}

	if (N.Contains("trail"))
	{
		return ENiagaraMotionPattern::Trail;
	}

	if (N.Contains("radial"))
	{
		return ENiagaraMotionPattern::Radial;
	}

	if (N.Contains("cone"))
	{
		return ENiagaraMotionPattern::Cone;
	}

	if (N.Contains("home") || N.Contains("seek"))
	{
		return ENiagaraMotionPattern::Homing;
	}

	return ENiagaraMotionPattern::Unknown;
}
