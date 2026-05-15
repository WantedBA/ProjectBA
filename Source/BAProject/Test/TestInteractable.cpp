
// TestInteractable.cpp
#include "TestInteractable.h"
#include "Components/StaticMeshComponent.h"

ATestInteractable::ATestInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{

		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ATestInteractable::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning,TEXT("[TestInteractable] %s interacted by%s"), *GetName(),*GetNameSafe(Interactor));
	if (GEngine)
	{

		GEngine->AddOnScreenDebugMessage(-1, 3.f,
		 FColor::Green,

		FString::Printf(TEXT("Interact! %s"),*GetName()));}
}

FText ATestInteractable::GetInteractionPrompt_Implementation() const
{
	return Prompt;
}