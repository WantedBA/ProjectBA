#include "Game/BAGameMode.h"

#include "Player/BAPlayerCharacter.h"

ABAGameMode::ABAGameMode()
{
	DefaultPawnClass = ABAPlayerCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
}
