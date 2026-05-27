// Copyright Epic Games, Inc. All Rights Reserved.

#include "BAProject.h"
#include "MoviePlayer.h"
#include "Modules/ModuleManager.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	constexpr float PostLoadScreenHoldSeconds = 0.8f;
}

class FBAProjectModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		FCoreUObjectDelegates::PreLoadMap.AddRaw(this, &FBAProjectModule::HandlePreLoadMap);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FBAProjectModule::HandlePostLoadMap);
	}

	virtual void ShutdownModule() override
	{
		FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
		FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

		FDefaultGameModuleImpl::ShutdownModule();
	}

private:
	void HandlePreLoadMap(const FString& /*MapName*/)
	{
		if (!IsMoviePlayerEnabled())
		{
			return;
		}

		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.WidgetLoadingScreen = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
			.Padding(0.0f)
			[
				SNew(SBox)
			];
		LoadingScreen.MinimumLoadingScreenDisplayTime = 0.0f;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
		LoadingScreen.bWaitForManualStop = true;
		LoadingScreen.bMoviesAreSkippable = false;
		LoadingScreen.bAllowEngineTick = true;

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}

	void HandlePostLoadMap(UWorld* LoadedWorld)
	{
		if (!LoadedWorld || !IsMoviePlayerEnabled())
		{
			StopLoadingScreen();
			return;
		}

		LoadedWorld->GetTimerManager().SetTimer(
			PostLoadScreenTimerHandle,
			FTimerDelegate::CreateRaw(this, &FBAProjectModule::StopLoadingScreen),
			PostLoadScreenHoldSeconds,
			false);
	}

	void StopLoadingScreen()
	{
		if (IsMoviePlayerEnabled())
		{
			GetMoviePlayer()->StopMovie();
		}
	}

	FTimerHandle PostLoadScreenTimerHandle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FBAProjectModule, BAProject, "BAProject");
