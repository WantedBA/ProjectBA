#include "Enemy/Animation/ANS_GhostTrail.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "TimerManager.h"

UANS_GhostTrail::UANS_GhostTrail()
{
	SpawnInterval = 0.05f;
	GhostLifeTime = 0.5f;
}

void UANS_GhostTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	LastSpawnTime = 0.0f;
}

void UANS_GhostTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	LastSpawnTime += FrameDeltaTime;

	if (LastSpawnTime >= SpawnInterval)
	{
		LastSpawnTime = 0.0f;

		AActor* Owner = MeshComp->GetOwner();

		if (Owner == nullptr)
		{
			return;
		}

		SpawnGhost(MeshComp);
	}
}

void UANS_GhostTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UANS_GhostTrail::SpawnGhost(USkeletalMeshComponent* SourceMesh)
{
	if (SourceMesh == nullptr)
	{
		return;
	}

	AActor* Owner = SourceMesh->GetOwner();

	if (Owner == nullptr)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();

	if (World == nullptr)
	{
		return;
	}

	// 1. Poseable Mesh 생성
	UPoseableMeshComponent* GhostMesh = NewObject<UPoseableMeshComponent>(Owner);
	if (GhostMesh == nullptr)
	{
		return;
	}

	GhostMesh->RegisterComponent();
	// 현재 스켈레탈 메쉬 복사
	GhostMesh->SetSkeletalMesh(SourceMesh->SkeletalMesh);

	// 2. Transform 동기화
	GhostMesh->SetWorldTransform(SourceMesh->GetComponentTransform());

	// 3. 포즈 복사
	GhostMesh->CopyPoseFromSkeletalComponent(SourceMesh);

	// 4. 머티리얼 잔상 처리
	int32 MaterialCount = GhostMesh->GetNumMaterials();

	for (int32 i = 0; i < MaterialCount; i++)
	{
		UMaterialInterface* BaseMat = GhostMesh->GetMaterial(i);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, GhostMesh);
			if (MID)
			{
				// 투명 + 색감 조정
				MID->SetScalarParameterValue(TEXT("Opacity"), 0.3f);
				MID->SetVectorParameterValue(TEXT("ColorTint"), FLinearColor(0.2f, 0.6f, 1.0f));
				GhostMesh->SetMaterial(i, MID);
			}
		}
	}

	// 일정 시간 후 삭제
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		[GhostMesh]()
	{
		if (GhostMesh)
		{
			GhostMesh->DestroyComponent();
		}
	},
		GhostLifeTime,false
	);
}
