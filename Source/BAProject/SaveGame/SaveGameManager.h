// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BASaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSaveGameComplete, bool, bSuccess);

/**
 * @class USaveGameManager
 * @brief 비동기 저장, 동기 로드
 * 저장할 정보 추가: BASaveGame.h에 구조체 작성, 저장할 정보를 가지고 있는 곳에서 MakeSaveGame()과 ApplySaveGame(구조체) 작성,
 * CreateSaveGame()과 LoadGame()에서 호출
 * 
 * 완료 델리게이트 필요 시: AsyncSaveStart()에서 OnSaveGameComplete.AddDynamic
 */
UCLASS()
class BAPROJECT_API USaveGameManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// 저장, 로드
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void SaveGame();
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void LoadGame();
	
	// 기존 세이브가 있는지 확인
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	bool IsSaveGameExist() const;
	
private:
	const FString DefaultSaveSlotName = TEXT("SaveGame");
	
	// 비동기 저장 마지막 저장 보장
	bool bIsSaving = false;
	bool bIsSaveWaiting = false;
	UPROPERTY()
	UBASaveGame* WaitingSaveGame = nullptr;
	
	void AsyncSaveStart(UBASaveGame* SaveGame);
	UBASaveGame* CreateSaveGame();
	
	// 엔진의 세이브 완료 델리게이트와 연결하는 중간 함수
	UFUNCTION()
	void SaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);
	
public:
	// 세이브 성공 델리게이트
	UPROPERTY(BlueprintAssignable, Category=SaveGame)
	FSaveGameComplete OnSaveGameComplete;
};
