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
		AActor* Monster = LockOnComp->GetTargetActor();

		if (Monster)
		{
			// 해당 몬스터의 스텟 컴포넌트 연결
			if (UStatComponent* Stat = Monster->FindComponentByClass<UStatComponent>())
			{
				Stat->OnHPChanged.RemoveDynamic(this, &UTargetHPBar::HandleHPChanged);
				Stat->OnHPChanged.AddDynamic(this, &UTargetHPBar::HandleHPChanged);

				// 초기 세팅
				UpdateHP(Stat->GetCurrentHP(), Stat->GetMaxHP());
				UE_LOG(LogTemp, Warning, TEXT(">>> Success: Bound to %s via Player LockOn!"),
					*Monster->GetName());
			}
		}
	}
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
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 새로운 타겟 정보 갱신
	APlayerController* PC = GetOwningPlayer();
	ABAPlayerCharacter* Player = PC ? Cast<ABAPlayerCharacter>(PC->GetPawn()) : nullptr;

	if (Player)
	{
		if (ULockOnTargetComponent* LockOnComp = Player->FindComponentByClass<ULockOnTargetComponent>())
		{
			AActor* NewMonster = LockOnComp->GetTargetActor();
			if (NewMonster)
			{
				if (UStatComponent* Stat = NewMonster->FindComponentByClass<UStatComponent>())
				{
					// 이전에 락온 된 몬스터 정보가 있을경우 끊음
					if (CurrentBoundStat)
					{
						CurrentBoundStat->OnHPChanged.RemoveDynamic(this, &UTargetHPBar::HandleHPChanged);
					}
					// 새 몬스터 연결
					Stat->OnHPChanged.AddDynamic(this, &UTargetHPBar::HandleHPChanged);

					CurrentBoundStat = Stat;

					UpdateHP(Stat->GetCurrentHP(), Stat->GetMaxHP());
				}
			}
		}
	}

	if (UWidgetComponent* ParentComp = Cast<UWidgetComponent>(GetOuter()))
	{
		ParentComp->SetVisibility(true);
	}
}

void UTargetHPBar::OnTargetReleased()
{
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

void UTargetHPBar::HideWidget()
{
	// 1초 경과 후 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	if (UWidgetComponent* ParentComp = Cast<UWidgetComponent>(GetOuter()))
	{
		ParentComp->SetVisibility(false);
	}
}
