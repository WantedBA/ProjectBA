// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/SaveGameManager.h"

#include "BASaveGame.h"
#include "Instance/SkillTreeSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USaveGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USaveGameManager::SaveGame()
{
	if (UBASaveGame* SaveGame 
		= Cast<UBASaveGame>(UGameplayStatics::CreateSaveGameObject(UBASaveGame::StaticClass())))
	{
		// 엔진 델리게이트
		FAsyncSaveGameToSlotDelegate SavedDelegate;
		SavedDelegate.BindUObject(this, &USaveGameManager::SaveComplete);
		
		// 세이브 델리게이트
		// OnSaveGameSuccess.AddDynamic(대상 객체, 함수(bool 성공 여부));

		// 저장할 데이터 추가
		SaveGame->SkillTreeData = GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->MakeSaveData();
		
		// 단일 세이브, 비동기 저장
		// 추후 세이브 파일 구분 시 SlotName 지정
		UGameplayStatics::AsyncSaveGameToSlot(SaveGame, DefaultSaveSlotName, 0, SavedDelegate);
		
		UE_LOG(LogTemp, Log, TEXT("Save game started"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create save game object"));
	}
}

void USaveGameManager::LoadGame()
{
	UBASaveGame* SaveGame = Cast<UBASaveGame>(UGameplayStatics::LoadGameFromSlot(DefaultSaveSlotName, 0));
	
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load save game"));
		return;
	}
	
	// 불러온 데이터 적용
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->ApplySaveData(SaveGame->SkillTreeData);
	
	UE_LOG(LogTemp, Log, TEXT("Save game loaded successfully"));
}

bool USaveGameManager::IsSaveGameExist() const
{
	return UGameplayStatics::DoesSaveGameExist(DefaultSaveSlotName, 0);
}

void USaveGameManager::SaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	if (!bSuccess)
	{
		// 저장 실패
		UE_LOG(LogTemp, Error, TEXT("Save game failed"));
		 __debugbreak();
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Save game completed successfully"));
	OnSaveGameSuccess.Broadcast(bSuccess);
}


