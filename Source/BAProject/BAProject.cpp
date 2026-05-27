// Copyright Epic Games, Inc. All Rights Reserved.

#include "BAProject.h"
#include "MoviePlayer.h"
#include "Modules/ModuleManager.h"
#include "Styling/CoreStyle.h"
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
	}

	virtual void ShutdownModule() override
	{
		FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);

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
		LoadingScreen.MinimumLoadingScreenDisplayTime = PostLoadScreenHoldSeconds;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.bWaitForManualStop = false;
		LoadingScreen.bMoviesAreSkippable = false;
		LoadingScreen.bAllowEngineTick = true;

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FBAProjectModule, BAProject, "BAProject");
