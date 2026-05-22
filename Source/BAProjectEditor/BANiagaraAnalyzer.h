// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BANiagaraAnalyzerData.h"
#include "NiagaraSystem.h"
#include "BANiagaraAnalyzer.generated.h"

UCLASS()
class UBANiagaraAnalyzer : public UObject
{
	GENERATED_BODY()

public:
	UBANiagaraAnalyzer();

	bool AnalyzeNiagaraSystem(UNiagaraSystem* InSystem, FNiagaraGameplayAnalysisData& OutData);

private:
	void AnalyzeEmitter(const FNiagaraEmitterHandle& InHandle, FNiagaraEmitterAnalysisData& OutEmitterData);
	void ClassifyEmitter(FNiagaraEmitterAnalysisData& OutEmitterData);
	void CheckReadabilityWarnings(FNiagaraEmitterAnalysisData& OutEmitterData);
};
