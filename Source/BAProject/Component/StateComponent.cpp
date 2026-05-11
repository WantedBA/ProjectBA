#include "Component/StateComponent.h"

UStateComponent::UStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentState = EEnemyState::Idle;
}

void UStateComponent::SetState(EEnemyState NewState)
{
	if (CurrentState == NewState || !CanTransitionTo(NewState))
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	CurrentState = NewState;
	OnStateChanged.Broadcast(OldState, NewState);
}

bool UStateComponent::CanTransitionTo(EEnemyState NewState)
{
	if (CurrentState == EEnemyState::Dead)
	{
		return false;
	}
	return true;
}
