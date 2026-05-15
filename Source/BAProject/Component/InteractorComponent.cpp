// Copyright TeamBA. All Rights Reserved.

#include "InteractorComponent.h"
#include "UI/Interaction/InteractionWidget.h"
#include "Components/SphereComponent.h"
#include "Interactable/Interactable.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (AActor* Owner = GetOwner())
	{
		DetectionSphere = NewObject<USphereComponent>(Owner, 
			TEXT("InteractorDetectionSphere"));
		DetectionSphere->RegisterComponent();
		if (USceneComponent* Root = Owner->GetRootComponent())
		{
			DetectionSphere->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		}
		DetectionSphere->SetSphereRadius(DetectionRadius);
		DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
		DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		DetectionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
		DetectionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		DetectionSphere->SetGenerateOverlapEvents(true);
	}
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	SetCurrentInteractable(nullptr);
	Super::EndPlay(Reason);
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	TimeSinceLastScan += DeltaTime;
	if (TimeSinceLastScan >= ScanInterval)
	{
		TimeSinceLastScan = 0.f;
		UpdateBestInteractable();
	}
	
#if !UE_BUILD_SHIPPING
	if (bShowDebug && CurrentInteractable && GEngine)
	{
		const FText Prompt = IInteractable::Execute_GetInteractionPrompt(CurrentInteractable);
		const FString Msg = FString::Printf(TEXT("[E] %s  (%s)"),
				*Prompt.ToString(), *CurrentInteractable->GetName());
		GEngine->AddOnScreenDebugMessage((uint64)this, 0.f, FColor::Cyan, Msg);
	}
#endif
}

void UInteractorComponent::TryInteract()
{
	if (!CurrentInteractable) return;
	AActor* Interactor = GetOwner();
	if (!Interactor) return;
	if (!IInteractable::Execute_CanInteract(CurrentInteractable, Interactor)) return;
	IInteractable::Execute_Interact(CurrentInteractable, Interactor);
}

void UInteractorComponent::UpdateBestInteractable()
{
	AActor* Best = SelectBest();
	if (Best != CurrentInteractable)
	{
		SetCurrentInteractable(Best);
	}
	else if (Best)
	{
		// 후보 변경이 없어도 프롬프트가 바뀔 수 있음 (예: 잠금 해제로 텍스트 변경)
		RefreshWidgetPrompt();
	}
}

AActor* UInteractorComponent::SelectBest() const
{
	if (!DetectionSphere) return nullptr;
        AActor* Owner = GetOwner();
        if (!Owner) return nullptr;

        APlayerController* PC = GetOwnerController();
        if (!PC) return nullptr;

        int32 ViewSizeX = 0, ViewSizeY = 0;
        PC->GetViewportSize(ViewSizeX, ViewSizeY);
        if (ViewSizeX <= 0 || ViewSizeY <= 0) return nullptr;

        const FVector2D ScreenCenter(ViewSizeX * 0.5f, ViewSizeY * 0.5f);
        // 정규화 기준: 화면 대각선의 절반 (중앙에서 코너까지의 최대 거리)
        const float MaxScreenDist = FMath::Sqrt(
                static_cast<float>(ViewSizeX * ViewSizeX + ViewSizeY * ViewSizeY)) * 0.5f;

        const FVector OwnerLocation = Owner->GetActorLocation();

        TArray<AActor*> Overlapping;
        DetectionSphere->GetOverlappingActors(Overlapping);

        AActor* Best = nullptr;
        float BestScore = TNumericLimits<float>::Max();

        for (AActor* Candidate : Overlapping)
        {
                if (!Candidate || Candidate == Owner) continue;
                if (!Candidate->Implements<UInteractable>()) continue;
                if (!IInteractable::Execute_CanInteract(Candidate, Owner)) continue;

                const FVector TargetLoc = GetInteractionLocation(Candidate);

                // 1) 월드 거리
                const float WorldDist = FVector::Distance(OwnerLocation, TargetLoc);
                if (WorldDist > MaxInteractionDistance) continue;
                const float WorldNorm = WorldDist / FMath::Max(MaxInteractionDistance, 1.f);

                // 2) 스크린 거리
                FVector2D ScreenPos;
                const bool bOnScreen = PC->ProjectWorldLocationToScreen(TargetLoc, ScreenPos);
                if (!bOnScreen) continue;
                const float ScreenDist = FVector2D::Distance(ScreenPos, ScreenCenter);
                const float ScreenNorm = FMath::Clamp(ScreenDist / FMath::Max(MaxScreenDist, 1.f), 0.f, 1.f);

                // 3) 가중합 (낮을수록 좋음)
                const float Score = ScreenWeight * ScreenNorm + (1.f - ScreenWeight) * WorldNorm;
                if (Score < BestScore)
                {
                        BestScore = Score;
                        Best = Candidate;
                }
        }

        return Best;
}

void UInteractorComponent::SetCurrentInteractable(AActor* NewInteractable)
{
	if (NewInteractable == CurrentInteractable) return;
	AActor* Old = CurrentInteractable;

	// 외곽선 갱신
	if (Old) SetOutline(Old, false);
	if (NewInteractable) SetOutline(NewInteractable, true);

	CurrentInteractable = NewInteractable;

	// 위젯 갱신
	if (NewInteractable)
	{
		ShowWidgetTarget(NewInteractable);
	}
	else
	{
		HideWidget();
	}

	OnInteractableChanged.Broadcast(NewInteractable, Old);
}

void UInteractorComponent::SetOutline(AActor* Object, bool bEnabled) const
{
	if (!Object) return;
	AActor* Actor = Cast<AActor>(Object);
	if (!Actor)
	{
		if (UActorComponent* C = Cast<UActorComponent>(Object))
		{
			Actor = C->GetOwner();
		}
	}
	if (!Actor) return;

	TArray<UPrimitiveComponent*> Prims;
	Actor->GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* P : Prims)
	{
		if (!P) continue;
		P->SetRenderCustomDepth(bEnabled);
		if (bEnabled)
		{
			P->SetCustomDepthStencilValue(OutlineStencilValue);
		}
	}
}

void UInteractorComponent::ShowWidgetTarget(AActor* Target)
{
	APlayerController* PC = GetOwnerController();
	if (!PC || !WidgetClass) return;

	if (!CurrentWidget)
	{
		CurrentWidget = CreateWidget<UInteractionWidget>(PC, WidgetClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport(100);
		}
	}
	if (CurrentWidget && Target)
	{
		const FText Prompt = IInteractable::Execute_GetInteractionPrompt(Target);
		CurrentWidget->SetPrompt(Prompt);
	}
}

void UInteractorComponent::HideWidget()
{
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}
}

void UInteractorComponent::RefreshWidgetPrompt()
{
	if (CurrentWidget && CurrentInteractable)
	{
		const FText Prompt = IInteractable::Execute_GetInteractionPrompt(CurrentInteractable);
		CurrentWidget->SetPrompt(Prompt);
	}
}

FVector UInteractorComponent::GetInteractionLocation(AActor* Object) const
{
	if (!Object) return FVector::ZeroVector;
	// 인터페이스가 정의한 위치 우선
	const FVector Loc = IInteractable::Execute_GetInteractionLocation(Object);
	if (!Loc.IsNearlyZero()) return Loc;
	// fallback: AActor 또는 컴포넌트 owner의 위치
	if (AActor* A = Cast<AActor>(Object)) return A->GetActorLocation();
	if (UActorComponent* C = Cast<UActorComponent>(Object))
	{
		if (AActor* O = C->GetOwner()) return O->GetActorLocation();
	}
	return FVector::ZeroVector;
}

APlayerController* UInteractorComponent::GetOwnerController() const
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}
	
	return nullptr;
}
