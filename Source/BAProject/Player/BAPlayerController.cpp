#include "Player/BAPlayerController.h"

ABAPlayerController::ABAPlayerController()
{
	// static ConstructorHelpers::FClassFinder<UHUDWidget> HUDWidgetRef(TEXT("/Game/BAProject/UI/WBP_HUD.WBP_HUD_C"));
	// if (HUDWidgetRef.Succeeded())
	// {
	// 	HUDWidgetClass = HUDWidgetRef.Class;
	// }
}

void ABAPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	FInputModeGameOnly GameOnlyInputMode;
	SetInputMode(GameOnlyInputMode);
	
	// HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	// if (HUDWidget)
	// {
	// 	HUDWidget->AddToViewport();
	// }
}
