// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/SaveGameManager.h"

#include "BASaveGame.h"
#include "Instance/SkillTreeSubsystem.h"
#include "Instance/QuestManageSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USaveGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USkillTreeSubsystem>();
	
	Super::Initialize(Collection);
}

void USaveGameManager::SaveGame()
{
	if (UBASaveGame* SaveInstance = CreateSaveGame())
	{
		AsyncSaveStart(SaveInstance);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create save game object"));
	}
}

void USaveGameManager::LoadGame()
{
	if (!IsSaveGameExist())
	{
		UE_LOG(LogTemp, Warning, TEXT("Saved game does not exist"));
		return;
	}
	
	UBASaveGame* SaveGame = Cast<UBASaveGame>(UGameplayStatics::LoadGameFromSlot(DefaultSaveSlotName, 0));
	
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load save game"));
		return;
	}
	
	// 불러온 데이터 적용
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->ApplySaveData(SaveGame->SkillTreeData);
	
	RespawnLocation = SaveGame->RespawnLocation;
	RespawnRotation = SaveGame->RespawnRotation;

	UE_LOG(LogTemp, Log, TEXT("Save game loaded successfully"));
}

bool USaveGameManager::IsSaveGameExist() const
{
	return UGameplayStatics::DoesSaveGameExist(DefaultSaveSlotName, 0);
}

void USaveGameManager::SetRespawnPoint(const FVector& Location, const FRotator& Rotation)
{
	RespawnLocation = Location;
	RespawnRotation = Rotation;
}

void USaveGameManager::AsyncSaveStart(UBASaveGame* SaveGame)
{
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Save game object is null"));
		OnSaveGameComplete.Broadcast(false);
		return;
	}
	
	// 비동기 저장 - 마지막 저장 데이터 보장
	if (bIsSaving)
	{
		UE_LOG(LogTemp, Warning, TEXT("Save game already in progress... waiting"));
		WaitingSaveGame = SaveGame;
		bIsSaveWaiting = true;
		return;
	}
	bIsSaving = true;
	
	// 엔진 델리게이트
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	SavedDelegate.BindUObject(this, &USaveGameManager::SaveComplete);
		
	// 세이브 델리게이트
	// OnSaveGameComplete.AddUniqueDynamic(대상 객체, 함수(bool 성공 여부));
		
	// 단일 세이브, 비동기 저장
	// 추후 세이브 파일 구분 시 SlotName 지정
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, DefaultSaveSlotName, 0, SavedDelegate);
	
	WaitingSaveGame = nullptr;
	bIsSaveWaiting = false;
		
	UE_LOG(LogTemp, Log, TEXT("Save game started"));
}

UBASaveGame* USaveGameManager::CreateSaveGame()
{
	UBASaveGame* SaveGameInstance 
		= Cast<UBASaveGame>(UGameplayStatics::CreateSaveGameObject(UBASaveGame::StaticClass()));
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create save game object"));
		return nullptr;
	}

	// 저장할 데이터 추가
	SaveGameInstance->SkillTreeData = GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->MakeSaveData();
	SaveGameInstance->QuestData.QuestTids = GetGameInstance()->GetSubsystem<UQuestManageSubsystem>()->MakeQuestSaveData();
	
	SaveGameInstance->RespawnLocation = RespawnLocation;
	SaveGameInstance->RespawnRotation = RespawnRotation;

	return SaveGameInstance;
}

void USaveGameManager::SaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	bIsSaving = false;
	
	if (bIsSaveWaiting)
	{
		if (!WaitingSaveGame)
		{
			UE_LOG(LogTemp, Error, TEXT("Waiting save game is null"));
			OnSaveGameComplete.Broadcast(false);
			return;
		}
		AsyncSaveStart(WaitingSaveGame);
		return;
	}
	
	if (!bSuccess)
	{
		// 저장 실패
		UE_LOG(LogTemp, Error, TEXT("Save game failed"));
		OnSaveGameComplete.Broadcast(false);
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Save game completed successfully"));
	OnSaveGameComplete.Broadcast(bSuccess);
}


