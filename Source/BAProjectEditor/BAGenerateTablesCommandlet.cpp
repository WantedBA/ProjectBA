// Copyright TeamBA. All Rights Reserved.

#include "BAGenerateTablesCommandlet.h"

#include "BATableGenerator.h"

UBAGenerateTablesCommandlet::UBAGenerateTablesCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UBAGenerateTablesCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Log, TEXT("[BAGenerateTablesCommandlet] Generating DataTables."));

	if (!FBATableGenerator::GenerateHeadless())
	{
		UE_LOG(LogTemp, Error, TEXT("[BAGenerateTablesCommandlet] Failed to generate DataTables."));
		return 1;
	}

	UE_LOG(LogTemp, Log, TEXT("[BAGenerateTablesCommandlet] DataTable generation complete."));
	return 0;
}
