#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"
#include "Instance/UserDataSubsystem.h"
#include "Instance/QuestManageSubsystem.h"
#include "SaveGame/SaveGameManager.h"
#include "GameFramework/CharacterMovementComponent.h"

// HP 변경을 UserDataSubsystem에 전달해 UI/HUD가 최신 체력 값을 보게 한다.
void ABAPlayerCharacter::OnHealthChanged(float CurrentHP, float MaxHP)
{
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	if (UserData && StatComponent)
	{
		// 변경 된 HP및 현 시점의 스테미너 수치를 전달
		UserData->NotifyPlayerStatChanged(CurrentHP, MaxHP, StatComponent->GetCurrentStamina(), StatComponent->GetMaxStamina());
	}
}

// Stamina 변경을 UserDataSubsystem에 전달해 UI/HUD가 최신 스태미나 값을 보게 한다.
void ABAPlayerCharacter::OnStaminaChanged(float CurrentStamina, float MaxStamina)
{
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	if (UserData && StatComponent)
	{
		// 변경된 Stamina및 현 시점의 체력 수치를 전달
		UserData->NotifyPlayerStatChanged(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP(), CurrentStamina, MaxStamina);
	}
}

void ABAPlayerCharacter::Respawn()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		USaveGameManager* SaveManager = GI->GetSubsystem<USaveGameManager>();
		if (SaveManager)
		{
			// 1. 마지막 체크포인트 위치로 이동
			FVector RespawnLoc = SaveManager->GetRespawnLocation();
			FRotator RespawnRot = SaveManager->GetRespawnRotation();

			if (!RespawnLoc.IsZero())
			{
				SetActorLocationAndRotation(RespawnLoc, RespawnRot, false, nullptr, ETeleportType::TeleportPhysics);
			}

			// 2. 상태 초기화
			CharacterState = ECharacterState::Alive;
			SetBAPlayerState(EBAPlayerState::None);
			
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}

			// 3. 스탯 복구
			if (StatComponent)
			{
				StatComponent->RestoreAll();
			}

			// 4. 적들 리스폰
			if (UQuestManageSubsystem* QM = GI->GetSubsystem<UQuestManageSubsystem>())
			{
				QM->RespawnQuestZoneEnemies();
			}

			UE_LOG(LogTemp, Log, TEXT("Player Respawned at %s"), *RespawnLoc.ToString());
		}
	}
}
