#include "Game/BAGameMode.h"

#include "Player/BAPlayerCharacter.h"
#include "Player/BAPlayerController.h"

ABAGameMode::ABAGameMode()
{
	DefaultPawnClass = ABAPlayerCharacter::StaticClass();
	PlayerControllerClass = ABAPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerRef(
		TEXT("/Game/Character/Blueprints/BP_PlayerController.BP_PlayerController_C"));
	if (PlayerControllerRef.Succeeded())
	{
		PlayerControllerClass = PlayerControllerRef.Class;
	}
}
