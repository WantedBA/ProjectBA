#pragma once

//Header 추가 금지
//constexpr만 있는 헤더는 컴파일 타임에 상수로 치환되고 끝이라 상관없음.

namespace TablePath
{
	constexpr const TCHAR* LoadTablePath = TEXT("/Game/Table/");
}

namespace BBKey
{
	const FName TargetActor = TEXT("TargetActor");
	const FName HomePos = TEXT("HomePos");
	const FName PatrolPos = TEXT("PatrolPos");
	const FName SplineIndex = TEXT("SplineIndex");
	const FName DetectRange = TEXT("DetectRange");
	const FName AttackRange = TEXT("AttackRange");
	const FName State = TEXT("State");
}