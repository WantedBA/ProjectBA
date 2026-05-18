#include "Enemy/Animation/AN_CameraShake.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UAN_CameraShake::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (CameraShakeClass == nullptr)
	{
		return;
	}

	// 0번 로컬 플레이어 컨트롤러를 통해 카메라 쉐이크 실행
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->ClientStartCameraShake(CameraShakeClass, Scale);
	}
}
