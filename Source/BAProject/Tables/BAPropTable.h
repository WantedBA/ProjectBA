// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include <type_traits>
#include "BAPropTable.generated.h"

/**
 * 매니저의 PostRead 단계에 참여하는 객체용 태그 인터페이스.
 */
class IBAPostRead
{
public:
	virtual ~IBAPostRead() = default;
	virtual void PostRead() = 0;
};

/**
 * TeamBA 모든 DataTable 행 USTRUCT 의 베이스. C# IProp<uint> 와 동치.
 *
 * 자식은 PostRead() 를 오버라이드하고 자신의 키 컬럼을 Tid 에 대입해야 함:
 *     virtual void PostRead() override { Tid = ItemTid; }
 *
 * Tid 는 Transient — 런타임에 키 컬럼으로부터 계산되며 .uasset 에는
 * 직렬화되지 않는다.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FBARowBase : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "BA")
	int32 Tid = 0;

	int32 GetKey() const { return Tid; }

	virtual void PostRead() {}
};

/**
 *
 * 라이프사이클:
 *   1) Build(DataTable)            -> 테이블에서 행 포인터들을 수집.
 *   2) 매니저가 PostRead() 호출    -> 각 행의 PostRead 실행 (Tid 설정),
 *                                     그 후 조회용 맵을 빌드.
 *
 * Find() / Has() 는 PostRead 단계가 끝난 뒤에만 유효한 결과를 돌려준다.
 */
template<typename RowType, typename KeyType>
class TBAPropTable : public IBAPostRead
{
public:
	static_assert(std::is_base_of_v<FBARowBase, RowType>,
		"RowType 은 FBARowBase 를 상속해야 함");

	void Build(const UDataTable* InTable)
	{
		SourceTable = InTable;
		Rows.Reset();
		Map.Reset();

		if (!InTable)
		{
			return;
		}

		const TMap<FName, uint8*>& RowMap = InTable->GetRowMap();
		Rows.Reserve(RowMap.Num());
		for (const TPair<FName, uint8*>& Pair : RowMap)
		{
			Rows.Add(reinterpret_cast<RowType*>(Pair.Value));
		}
	}

	virtual void PostRead() override
	{
		for (RowType* Row : Rows)
		{
			Row->PostRead();
		}

		Map.Reset();
		Map.Reserve(Rows.Num());
		for (RowType* Row : Rows)
		{
			Map.Add(Row->GetKey(), Row);
		}
	}

	int32 Num() const { return Map.Num(); }

	bool Has(const KeyType& Key) const { return Map.Contains(Key); }

	const RowType* Find(const KeyType& Key) const
	{
		const RowType* const* Found = Map.Find(Key);
		return Found ? *Found : nullptr;
	}

	const TMap<KeyType, RowType*>& GetMap() const { return Map; }

	const UDataTable* GetSource() const { return SourceTable.Get(); }

private:
	TArray<RowType*> Rows;
	TMap<KeyType, RowType*> Map;
	TWeakObjectPtr<const UDataTable> SourceTable;
};
