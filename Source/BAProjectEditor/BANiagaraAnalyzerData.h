// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BANiagaraAnalyzerData.generated.h"

UENUM(BlueprintType)
enum class ENiagaraTemporalBehavior : uint8
{
	Instant,
	Sustain,
	Looping,
	Pulsing,
	Crescendo,
	FadeOut,
	MultiPhase,
	Unknown
};

UENUM(BlueprintType)
enum class ENiagaraGameplayRole : uint8
{
	HitImpact,
	ProjectileTrail,
	BuffAura,
	AreaWarning,
	Explosion,
	Footstep,
	EnvironmentAmbient,
	WeaponSwing,
	MagicCast,
	UIFX,
	Unknown
};

UENUM(BlueprintType)
enum class ENiagaraColorSemantic : uint8
{
	Fire,
	Ice,
	Arcane,
	Poison,
	Holy,
	Electric,
	Blood,
	Smoke,
	Neutral,
	Unknown
};
UENUM(BlueprintType)
enum class ENiagaraEmitterCategory : uint8
{
	Flash,
	Spark,
	Smoke,
	Distortion,
	Debris,
	Flow,
	Ring,
	Ribbon,
	Unknown
};

UENUM(BlueprintType)
enum class ENiagaraMotionPattern : uint8
{
	Static,
	Radial,
	Cone,
	Forward,
	Homing,
	Orbit,
	Vortex,
	Turbulence,
	Noise,
	Trail,
	Unknown
};

UENUM(BlueprintType)
enum class ENiagaraSpawnMethodType : uint8
{
	Burst,
	Continuous,
	Mixed
};

USTRUCT(BlueprintType)
struct FNiagaraCurveSample
{
	GENERATED_BODY()

	UPROPERTY()
	FString ParameterName;

	UPROPERTY()
	TArray<float> TimeKeys;

	UPROPERTY()
	TArray<float> Values;
};

USTRUCT(BlueprintType)
struct FNiagaraSpawnAnalysisData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 BurstCount = 0;

	UPROPERTY()
	float SpawnRate = 0.0f;

	UPROPERTY()
	bool bHasMultiBurst = false;

	UPROPERTY()
	bool bHasBurstTimingOffset = false;

	UPROPERTY()
	float BurstInterval = 0.0f;

	UPROPERTY()
	bool bInfiniteSpawn = false;

	UPROPERTY()
	float EstimatedMaxParticles = 0.0f;

	UPROPERTY()
	ENiagaraSpawnMethodType SpawnMethodType = ENiagaraSpawnMethodType::Continuous;
};

USTRUCT(BlueprintType)
struct FNiagaraRendererAnalysisData
{
	GENERATED_BODY()

	UPROPERTY()
	FString RendererType;

	UPROPERTY()
	FString MaterialName;

	UPROPERTY()
	FString BlendMode;

	UPROPERTY()
	FString SortMode;

	UPROPERTY()
	bool bIsTwoSided = false;

	UPROPERTY()
	bool bIsDistortion = false;

	/** 머티리얼 상세 파라미터 (AI 분석용) */
	UPROPERTY()
	TMap<FString, float> ScalarParameters;

	UPROPERTY()
	TMap<FString, FLinearColor> VectorParameters;

	UPROPERTY()
	TMap<FString, FString> TextureParameters;

	UPROPERTY()
	bool bUsesDepthFade = false;

	UPROPERTY()
	bool bUsesFresnel = false;

	UPROPERTY()
	bool bUsesSoftParticle = false;

	UPROPERTY()
	bool bUsesDistortion = false;

	UPROPERTY()
	bool bIsUnlitMaterial = true;
};

USTRUCT(BlueprintType)
struct FNiagaraEmitterAnalysisData
{
	GENERATED_BODY()

	UPROPERTY()
	FString EmitterName;

	UPROPERTY()
	bool bIsEnabled = true;

	UPROPERTY()
	FString SimTarget; // CPU/GPU

	UPROPERTY()
	ENiagaraEmitterCategory Category = ENiagaraEmitterCategory::Unknown;

	UPROPERTY()
	ENiagaraMotionPattern MotionPattern = ENiagaraMotionPattern::Unknown;

	UPROPERTY()
	FNiagaraSpawnAnalysisData SpawnData;

	UPROPERTY()
	float LifetimeMin = 0.0f;

	UPROPERTY()
	float LifetimeMax = 0.0f;

	UPROPERTY()
	FVector2D SpriteSizeMin = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D SpriteSizeMax = FVector2D::ZeroVector;

	UPROPERTY()
	FString VelocityMode;

	UPROPERTY()
	TArray<FNiagaraRendererAnalysisData> Renderers;

	/** 이미터에 사용된 모든 모듈 이름 */
	UPROPERTY()
	TArray<FString> ModuleNames;

	/** 주요 파라미터 설정값 (AI 분석용) */
	UPROPERTY()
	TMap<FString, FString> ParameterValues;

	/** 외부 데이터 연동 정보 (SkeletalMesh, Spline 등) */
	UPROPERTY()
	TArray<FString> DataInterfaces;

	/** 다른 변수에 바인딩된 파라미터 리스트 */
	UPROPERTY()
	TArray<FString> BoundParameters;

	UPROPERTY() 
	TArray<FNiagaraCurveSample> Curves;

	UPROPERTY()
	bool bFaceCamera = false;

	UPROPERTY()
	bool bCustomFacing = false;

	UPROPERTY()
	bool bHasCurlNoise = false;

	UPROPERTY()
	bool bHasDrag = false;

	UPROPERTY()
	bool bHasVortex = false;

	UPROPERTY()
	bool bHasGravity = false;

	UPROPERTY()
	bool bFixedBounds = false;

	UPROPERTY()
	bool bHasCullDistance = false;
	// Spatial behavior
	UPROPERTY()
	bool bWorldSpace = false;

	UPROPERTY()
	bool bCameraFacing = false;

	UPROPERTY()
	bool bVelocityAligned = false;

	UPROPERTY()
	bool bHasCollision = false;

	// Cost Model
	/** 이미터 복잡도 점수 (0~100) */
	UPROPERTY()
	float ComplexityScore = 0.0f;

	UPROPERTY()
	float EstimatedGPUCost = 0.0f;

	UPROPERTY()
	float EstimatedOverdraw = 0.0f;

	// AI semantic
	UPROPERTY() 
	TArray<FString> SemanticTags;

	UPROPERTY() 
	TArray<FString> Warnings;

	/* 리본 기본 두께 및 커브 바인딩 여부 */
	UPROPERTY()
	float RibbonWidth = 0.0f;

	UPROPERTY()
	bool bHasRibbonWidthCurve = false;

	/* 애니메이션 노티파이 스테이트(AnimNotifyState_Trail) 연동 방식 */
	UPROPERTY()
	FString DriveMode; // "NotifyState", "SpawnRate", "SkeletalMeshBinding" 등

	/* 에디터에 명시적으로 세팅된 실제 Max Particle Count */
	UPROPERTY()
	int32 ExplicitMaxParticleCount = 0;

	UPROPERTY()
	bool bDepthFadeDisabledRisk = false;

	UPROPERTY()
	float MaxCullDistance = 0.0f;

	UPROPERTY()
	ENiagaraTemporalBehavior TemporalBehavior = ENiagaraTemporalBehavior::Unknown;

	UPROPERTY()
	ENiagaraGameplayRole GameplayRole = ENiagaraGameplayRole::Unknown;

	UPROPERTY()
	ENiagaraColorSemantic ColorSemantic = ENiagaraColorSemantic::Unknown;

	UPROPERTY()
	float EstimatedScreenCoverage = 0.0f;

	UPROPERTY()
	bool bCenterScreenDominant = false;

	UPROPERTY()
	bool bPeripheralFX = false;

	UPROPERTY()
	bool bFrontLoadedEffect = false;

	UPROPERTY()
	bool bTrailingPersistence = false;
};

USTRUCT(BlueprintType)
struct FNiagaraGameplayAnalysisData
{
	GENERATED_BODY()

	UPROPERTY()
	FString SystemName;

	/** 시스템/유저 레벨 파라미터 및 변수 (AI 분석용) */
	UPROPERTY()
	TMap<FString, FString> SystemParameters;

	UPROPERTY()
	TArray<FNiagaraEmitterAnalysisData> Emitters;

	UPROPERTY()
	TArray<FString> SystemSemanticTags;

	UPROPERTY()
	float TotalEstimatedCost = 0.0f;

	UPROPERTY()
	bool bUsesDistanceCull = false;

	UPROPERTY()
	float CullDistance = 0.0f;
};
