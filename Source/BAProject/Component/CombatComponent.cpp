#include "Component/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bIsHitChecking = false;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsHitChecking)
	{
		CheckHit();
	}
}

void UCombatComponent::ExecuteAttack(UAnimMontage* AttackMontage, float PlayRate)
{
	if (AttackMontage == nullptr)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerCharacter->PlayAnimMontage(AttackMontage, PlayRate);
	}
}

void UCombatComponent::CheckAnimNotifyHitStart(float InRadius, float InDamage, FName InSocketName)
{
	bIsHitChecking = true;
	CurrentRadius = InRadius;
	CurrentDamage = InDamage;
	CurrentSocketName = InSocketName;
	
	HitActors.Empty();
	SetComponentTickEnabled(true);
}

void UCombatComponent::CheckAnimNotifyHitEnd()
{
	bIsHitChecking = false;
	SetComponentTickEnabled(false);
}

void UCombatComponent::SetWeaponCollisionActive(bool bActive)
{
	bIsHitChecking = bActive;
	if (bActive)
	{
		HitActors.Empty();
		SetComponentTickEnabled(true);
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

void UCombatComponent::CheckHit()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	FVector TraceLocation = OwnerCharacter->GetMesh()->GetSocketLocation(CurrentSocketName);
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(OwnerCharacter);

	TArray<AActor*> OutActors;
	
	bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		TraceLocation,
		CurrentRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(), // 필터
		nullptr,
		IgnoreActors,
		OutActors
	);

	if (bHit)
	{
		for (AActor* Victim : OutActors)
		{
			if (Victim && !HitActors.Contains(Victim))
			{
				HitActors.Add(Victim);
				// 데미지 처리..
			}
		}
	}
}
