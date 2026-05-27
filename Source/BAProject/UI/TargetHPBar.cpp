// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TargetHPBar.h"
#include "UI/StatusBar.h"

#include "Player/BAPlayerCharacter.h"
#include "LockOnTargetComponent.h"

#include "Component/StatComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/Image.h"


void UTargetHPBar::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	// 캐릭터 정보 가져오기
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return;
	}

	// 락온 컴포넌트 찾기
	ULockOnTargetComponent* LockOnComp = Pawn->FindComponentByClass<ULockOnTargetComponent>();

	if (LockOnComp)
	{
		// 락온 대상 찾기
		BindToTarget(LockOnComp->GetTargetActor());
	}
}

void UTargetHPBar::NativeDestruct()
{
	UnbindCurrentTarget();
	Super::NativeDestruct();
}

void UTargetHPBar::HandleHPChanged(float CurrentHP, float MaxHP)
{
	UE_LOG(LogTemp, Warning, TEXT(">>> HP Changed! Current: %f"), CurrentHP);
	UpdateHP(CurrentHP, MaxHP);
}

void UTargetHPBar::UpdateHP(float CurrentHP, float MaxHP)
{
	if (TargetHPBar)
	{
		// 우선 표시
		CancelHideTimer();
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// 게이지 갱신
		TargetHPBar->SetProgress(CurrentHP, MaxHP);
	}
}

void UTargetHPBar::StartHideTimer()
{
	// 1.0초 뒤에 HideWidget 함수 실행
	GetWorld()->GetTimerManager().SetTimer(HideTimerHandle, this, &UTargetHPBar::HideWidget, 1.0f, false);
}

void UTargetHPBar::OnTargetCaptured()
{
	// 타이머 리셋
	CancelHideTimer();

	// 새로운 타겟 정보 갱신
	APlayerController* PC = GetOwningPlayer();
	ABAPlayerCharacter* Player = PC ? Cast<ABAPlayerCharacter>(PC->GetPawn()) : nullptr;

	if (Player)
	{
		if (ULockOnTargetComponent* LockOnComp = Player->FindComponentByClass<ULockOnTargetComponent>())
		{
			BindToTarget(LockOnComp->GetTargetActor());
		}
	}

	if (UWidgetComponent* ParentComp = Cast<UWidgetComponent>(GetOuter()))
	{
		ParentComp->SetVisibility(true);
	}
}

void UTargetHPBar::OnTargetReleased()
{
	UnbindCurrentTarget();

	if (UWidgetComponent* ParentComp = Cast<UWidgetComponent>(GetOuter()))
	{
		ParentComp->SetVisibility(true);

		GetWorld()->GetTimerManager().SetTimer(HideTimerHandle, FTimerDelegate::CreateLambda([ParentComp, this]()
			{
				if (ParentComp)
				{
					ParentComp->SetVisibility(false);
				}

				this->SetVisibility(ESlateVisibility::Collapsed);
			}), 1.0f, false);
	}
	else
	{
		StartHideTimer();
	}

}

void UTargetHPBar::CancelHideTimer()
{
	// 타이머가 실행 중이라면 취소 및 게이지 표시 유지
	if (HideTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideTimerHandle);
	}
}

void UTargetHPBar::BindToTarget(AActor* TargetActor)
{
	UStatComponent* NewStat = TargetActor ? TargetActor->FindComponentByClass<UStatComponent>() : nullptr;
	if (!NewStat)
	{
		UnbindCurrentTarget();
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (CurrentBoundStat && CurrentBoundStat != NewStat)
	{
		CurrentBoundStat->OnHPChanged.RemoveDynamic(this, &UTargetHPBar::HandleHPChanged);
	}

	// BP 이벤트가 중복 호출돼도 같은 델리게이트가 중복 바인딩되지 않게 보장한다.
	NewStat->OnHPChanged.RemoveDynamic(this, &UTargetHPBar::HandleHPChanged);
	NewStat->OnHPChanged.AddUniqueDynamic(this, &UTargetHPBar::HandleHPChanged);
	CurrentBoundStat = NewStat;

	UpdateHP(NewStat->GetCurrentHP(), NewStat->GetMaxHP());
}

void UTargetHPBar::UnbindCurrentTarget()
{
	if (CurrentBoundStat)
	{
		CurrentBoundStat->OnHPChanged.RemoveDynamic(this, &UTargetHPBar::HandleHPChanged);
		CurrentBoundStat = nullptr;
	}
}

void UTargetHPBar::HideWidget()
{
	// 1초 경과 후 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	if (UWidgetComponent* ParentComp = Cast<UWidgetComponent>(GetOuter()))
	{
		ParentComp->SetVisibility(false);
	}
}
