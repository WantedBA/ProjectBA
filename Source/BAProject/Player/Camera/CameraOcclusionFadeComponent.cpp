#include "Player/Camera/CameraOcclusionFadeComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UCameraOcclusionFadeComponent::UCameraOcclusionFadeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCameraOcclusionFadeComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveDefaultComponents();
}

void UCameraOcclusionFadeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreAllFadeTargets();
	Super::EndPlay(EndPlayReason);
}

void UCameraOcclusionFadeComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableOcclusionFade)
	{
		RestoreAllFadeTargets();
		return;
	}

	if (!CachedCamera.IsValid() || !CachedFocusComponent.IsValid())
	{
		ResolveDefaultComponents();
		if (!CachedCamera.IsValid() || !CachedFocusComponent.IsValid())
		{
			return;
		}
	}

	TimeSinceLastRefresh += DeltaTime;
	if (TimeSinceLastRefresh >= RefreshInterval)
	{
		TimeSinceLastRefresh = 0.f;
		RefreshObstructingComponents();
	}

	UpdateFadeStates(DeltaTime);
}

void UCameraOcclusionFadeComponent::ResolveDefaultComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	CachedCamera = Owner->FindComponentByClass<UCameraComponent>();
	CachedFocusComponent = Owner->GetRootComponent();
}

void UCameraOcclusionFadeComponent::RefreshObstructingComponents()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	UCameraComponent* Camera = CachedCamera.Get();
	USceneComponent* FocusComponent = CachedFocusComponent.Get();
	if (!World || !Owner || !Camera || !FocusComponent)
	{
		return;
	}

	const FVector CameraLocation = Camera->GetComponentLocation();
	const FVector FocusLocation = FocusComponent->GetComponentLocation();
	TSet<TWeakObjectPtr<UMeshComponent>> CurrentObstructions;

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor || Actor == Owner)
		{
			continue;
		}

		TInlineComponentArray<UMeshComponent*> MeshComponents;
		Actor->GetComponents(MeshComponents);
		for (UMeshComponent* Component : MeshComponents)
		{
			if (!ShouldConsiderComponent(Component, Actor))
			{
				continue;
			}

			PrepareFadeTargetComponent(*Component);

			if (IsComponentObstructingCamera(*Component, FocusLocation, CameraLocation))
			{
				CurrentObstructions.Add(Component);
				SetTargetOpacity(Component, OccludedOpacity);
			}
		}
	}

	for (TPair<TWeakObjectPtr<UMeshComponent>, FCameraOcclusionFadeState>& Pair : FadeStates)
	{
		if (!CurrentObstructions.Contains(Pair.Key))
		{
			Pair.Value.TargetOpacity = RestoredOpacity;
		}
	}
}

void UCameraOcclusionFadeComponent::UpdateFadeStates(const float DeltaTime)
{
	for (auto It = FadeStates.CreateIterator(); It; ++It)
	{
		UMeshComponent* Component = It.Key().Get();
		if (!Component || !Component->IsRegistered())
		{
			It.RemoveCurrent();
			continue;
		}

		FCameraOcclusionFadeState& State = It.Value();
		State.CurrentOpacity = FMath::FInterpTo(
			State.CurrentOpacity,
			State.TargetOpacity,
			DeltaTime,
			FadeInterpSpeed);

		if (FMath::IsNearlyEqual(State.CurrentOpacity, State.TargetOpacity, 0.01f))
		{
			State.CurrentOpacity = State.TargetOpacity;
		}

		ApplyOpacity(*Component, State.CurrentOpacity);

		if (FMath::IsNearlyEqual(State.CurrentOpacity, RestoredOpacity, 0.01f)
			&& FMath::IsNearlyEqual(State.TargetOpacity, RestoredOpacity, 0.01f))
		{
			ApplyOpacity(*Component, RestoredOpacity);
			It.RemoveCurrent();
		}
	}
}

void UCameraOcclusionFadeComponent::RestoreAllFadeTargets()
{
	for (const TPair<TWeakObjectPtr<UMeshComponent>, FCameraOcclusionFadeState>& Pair : FadeStates)
	{
		if (UMeshComponent* Component = Pair.Key.Get())
		{
			ApplyOpacity(*Component, RestoredOpacity);
		}
	}

	FadeStates.Empty();
	TimeSinceLastRefresh = 0.f;
}

bool UCameraOcclusionFadeComponent::ShouldConsiderComponent(
	const UMeshComponent* Component,
	const AActor* OwnerActor) const
{
	if (!Component || !OwnerActor || !Component->IsRegistered() || !Component->IsVisible())
	{
		return false;
	}

	if (!OwnerActor->ActorHasTag(FadeTargetTag) && !Component->ComponentHasTag(FadeTargetTag))
	{
		return false;
	}

	if (Component->GetNumMaterials() <= 0)
	{
		return false;
	}

	return true;
}

void UCameraOcclusionFadeComponent::PrepareFadeTargetComponent(UMeshComponent& Component) const
{
	if (!bDisableCameraCollisionOnFadeTargets)
	{
		return;
	}

	if (Component.GetCollisionEnabled() != ECollisionEnabled::NoCollision
		&& Component.GetCollisionResponseToChannel(CameraBlockingChannel) == ECR_Block)
	{
		Component.SetCollisionResponseToChannel(CameraBlockingChannel, ECR_Ignore);
	}
}

bool UCameraOcclusionFadeComponent::IsComponentObstructingCamera(
	const UMeshComponent& Component,
	const FVector& FocusLocation,
	const FVector& CameraLocation) const
{
	if (FocusLocation.Equals(CameraLocation))
	{
		return false;
	}

	const FBoxSphereBounds& Bounds = Component.Bounds;
	const FVector ClosestPoint = FMath::ClosestPointOnSegment(Bounds.Origin, FocusLocation, CameraLocation);
	const float CombinedRadius = Bounds.SphereRadius + ProbeRadius;
	return FVector::DistSquared(Bounds.Origin, ClosestPoint) <= FMath::Square(CombinedRadius);
}

void UCameraOcclusionFadeComponent::SetTargetOpacity(UMeshComponent* Component, const float TargetOpacity)
{
	if (!Component)
	{
		return;
	}

	FCameraOcclusionFadeState& State = FadeStates.FindOrAdd(Component);
	State.TargetOpacity = FMath::Clamp(TargetOpacity, 0.f, 1.f);
}

void UCameraOcclusionFadeComponent::ApplyOpacity(UMeshComponent& Component, const float Opacity) const
{
	const float SafeOpacity = FMath::Clamp(Opacity, 0.f, 1.f);
	if (!OpacityParameterName.IsNone())
	{
		Component.SetScalarParameterValueOnMaterials(OpacityParameterName, SafeOpacity);
	}

	if (!FallbackOpacityParameterName.IsNone() && FallbackOpacityParameterName != OpacityParameterName)
	{
		Component.SetScalarParameterValueOnMaterials(FallbackOpacityParameterName, SafeOpacity);
	}
}
