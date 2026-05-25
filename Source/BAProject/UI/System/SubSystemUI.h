// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Stack.h"
#include "StackElem.h"
#include "SubSystemUI.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class BAPROJECT_API USubSystemUI : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
public:
	// 위젯 출력 및 스택 추가 함수
	// LayerBase를 상속받은 경우만 가능하도록 설계
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void PushUI(class ULayerBase* InWidget);

	// 클래스 타입을 넘겨주면 내부에서 생성 해주는 자동화 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI", meta = (DisplayName = "Push UI By Class"))
	class ULayerBase* PushUIByClass(TSubclassOf<ULayerBase> InWidgetClass, class UWorld* InWorld = nullptr);

	// 알림 레이어 전용 함수 
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	class UNotifyLayer* GetNotifyLayer() const { return CachedNotifyLayer; }

	template<typename T>
	T* PushUI()
	{
		return Cast<T>(PushUIByClass(T::StaticClass()));
	}

	// 가장 위에 있는 위젯을 끄고 스택에서 제거
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void PopUI();

	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	bool HandleBackAction();

	// 화면에 출력된 MainHUD를 밖에서 가져갈 수 있게 함
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	class UMainHUD* GetMainHUD() const { return CachedMainHUD; }

	// 맵 이동 시 모든 UI 비우기
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void ClearAllUI();


protected:
	// 인터페이스(IStackElem) 타입으로 쌓을 스택 바구니
	TStack<IStackElem*> UIStack;

	// 위젯의 애니메이션이 끝났을 때 실행할 삭제 함수
	UFUNCTION()
	void OnWidgetCloseAnimationFinished(ULayerBase* Widget);

	// 플레이어 사망 시 실행 함수
	UFUNCTION()
	void HandlePlayerDeath();

	// 맵이 시작될 때마다 실행될 함수
	void HandleWorldInit(UWorld* World, const UWorld::InitializationValues IValues);

	// 월드 변경 시 기존 UI 정보 비우기
	void CleanupUI();

	// 모든 팝업 뒤에 적용할 블러 위젯
	UPROPERTY()
	class UUserWidget* GlobalBlurWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BA|UI")
	TSubclassOf<UUserWidget> BlurWidgetClass;

	UPROPERTY()
	class UMainHUD* CachedMainHUD;

	UPROPERTY()
	class UNotifyLayer* CachedNotifyLayer;

	UPROPERTY(EditAnywhere, Category = "BA|UI")
	TSubclassOf<UNotifyLayer> NotifyLayerClass;

private:
	// 스택 상황에 따라 마우스 커서와 입력 모드 결정
	void RefreshInputMode();
};
