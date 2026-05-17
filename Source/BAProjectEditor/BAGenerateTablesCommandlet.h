// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BAGenerateTablesCommandlet.generated.h"

UCLASS()
class BAPROJECTEDITOR_API UBAGenerateTablesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBAGenerateTablesCommandlet();

	virtual int32 Main(const FString& Params) override;
};
