#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"
#include "Instance/UserDataSubsystem.h"
#include "Instance/QuestManageSubsystem.h"
#include "SaveGame/SaveGameManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Constants/BAProjectConstant.h"

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

				if (APlayerController* PC = Cast<APlayerController>(GetController()))
				{
					PC->SetControlRotation(RespawnRot);
				}
			}

			// 2. 상태 초기화
			ResetLandingRecovery();
			ClearFallDamageSuppression();
			ResetWeaponAttachment(); // 무기 부착
			bFallTrackingActive = false;
			LastFallDistance = 0.f;
			CharacterState = ECharacterState::Alive;
			SetBAPlayerState(EBAPlayerState::Respawning);

			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				MoveComp->SetMovementMode(MOVE_Walking);
				MoveComp->StopMovementImmediately();
			}

			// 3. 애니메이션 재생
			if (RespawnMontage)
			{
				const float Duration = PlayAnimMontage(RespawnMontage);
				if (Duration > 0.f)
				{
					if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
					{
						FOnMontageEnded EndedDelegate;
						EndedDelegate.BindUObject(this, &ABAPlayerCharacter::HandleRespawnMontageEnded);
						AnimInstance->Montage_SetEndDelegate(EndedDelegate, RespawnMontage);
					}
				}
				else
				{
					SetBAPlayerState(EBAPlayerState::None);
				}
			}
			else
			{
				SetBAPlayerState(EBAPlayerState::None);
			}

			// 4. 스탯 복구
			if (StatComponent)
			{
				StatComponent->RestoreAll();
			}

			// 4. 적들 리스폰
			if (UQuestManageSubsystem* QM = GI->GetSubsystem<UQuestManageSubsystem>())
			{
				QM->ResetActiveQuests();
				QM->RespawnQuestZoneEnemies();
			}

			UE_LOG(LogTemp, Log, TEXT("Player Respawned at %s"), *RespawnLoc.ToString());
		}
	}
}

void ABAPlayerCharacter::ResetWeaponAttachment()
{
	if (!WeaponMeshComponent || !GetMesh())
	{
		return;
	}

	// 1. 물리 시뮬레이션 즉시 중단
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->PutRigidBodyToSleep();

	// 2. 충돌 설정 초기화 (NoCollision 명시)
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	// 3. 소켓에 다시 부착
	const FName TargetSocket = FName(SocketName::RightHandTargetSocketName);
	if (GetMesh()->DoesSocketExist(TargetSocket))
	{
		// 부착 시 상대 변환값을 유지하도록 설정한 뒤, 저장해둔 기본값으로 초기화
		WeaponMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TargetSocket);
		WeaponMeshComponent->SetRelativeLocation(DefaultWeaponRelativeLocation);
		WeaponMeshComponent->SetRelativeRotation(DefaultWeaponRelativeRotation);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BAPlayerCharacter] ResetWeaponAttachment: Socket %s not found on mesh."), *TargetSocket.ToString());
	}

	bWeaponDroppedForDeath = false;
}

void ABAPlayerCharacter::HandleRespawnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (BAPlayerState == EBAPlayerState::Respawning)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
}
