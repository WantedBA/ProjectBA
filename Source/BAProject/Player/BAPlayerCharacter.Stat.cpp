#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"
#include "Instance/UserDataSubsystem.h"

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
