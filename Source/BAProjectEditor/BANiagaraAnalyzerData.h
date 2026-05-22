// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BANiagaraAnalyzerData.generated.h"

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
	bool bIsDistortion = false;

	/** 머티리얼 상세 파라미터 (AI 분석용) */
	UPROPERTY()
	TMap<FString, float> ScalarParameters;

	UPROPERTY()
	TMap<FString, FLinearColor> VectorParameters;

	UPROPERTY()
	TMap<FString, FString> TextureParameters;
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

	UPROPERTY()
	bool bFaceCamera = false;

	UPROPERTY()
	bool bVelocityAligned = false;

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

	UPROPERTY()
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct FNiagaraGameplayAnalysisData
{
	GENERATED_BODY()

	UPROPERTY()
	FString SystemName;

	UPROPERTY()
	TArray<FNiagaraEmitterAnalysisData> Emitters;
};
