// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BAPropTable.h"
#include "Tables/ItemRows.h"
#include "BATableManager.generated.h"

/**
 * Initialize() 시점에 /Game/Table 아래 등록된 모든 DataTable 을 로드한 뒤,
 * 등록 순서대로 PostRead 단계를 일괄 실행한다.
 */
UCLASS()
class BAPROJECT_API UBATableManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UBATableManager* Get(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "BA|Table")
	bool HasConsume(int32 InTid) const { return ConsumeTable.Has(InTid); }

	const FConsumeItemRow* FindConsume(int32 InTid) const { return ConsumeTable.Find(InTid); }

	UFUNCTION(BlueprintCallable, Category = "BA|Table", meta = (DisplayName = "Find Consume"))
	bool BP_FindConsume(int32 InTid, FConsumeItemRow& OutRow) const;

	const TMap<int32, FConsumeItemRow*>& GetConsumeMap() const { return ConsumeTable.GetMap(); }

private:
	template<typename RowType, typename KeyType>
	void LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const TCHAR* AssetPath);

	TBAPropTable<FConsumeItemRow, int32> ConsumeTable;

	TArray<IBAPostRead*> PostReadList;

	UPROPERTY()
	TArray<TObjectPtr<UDataTable>> LoadedTables;
};
