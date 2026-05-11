#pragma once

#include "CoreMinimal.h"

template<typename T>
struct TStack
{
public:
	// 데이터를 스택 가장 위에 추가
	void Push(T InElem)
	{
		Items.Add(InElem);
	}

	// 스택 맨 위 요소를 제거 후 돌려줌
	T Pop()
	{
		if (Items.Num() > 0)
		{
			T TopElem = Items.Last();
			Items.RemoveAt(Items.Num() - 1);
			return TopElem;
		}

		// 데이터가 없을 경우 기본값 반환
		return T();
	}

	// 현재 스택 맨 위에 있는 요소 확인 (제거X)
	T Peek() const
	{
		return Items.Num() > 0 ? Items.Last() : T();
	}

	// 스택이 비어있는지 확인
	bool IsEmpty() const { return Items.Num() == 0; }

	// 전체 개수 확인
	int32 Num() const { return Items.Num(); }

private:
	TArray<T> Items;
};
