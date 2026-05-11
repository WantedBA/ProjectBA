#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BAPlayerController.generated.h"

UCLASS()
class BAPROJECT_API ABAPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABAPlayerController();
	
protected:
	virtual void BeginPlay() override;
	
// protected: TODO: 은성님 HUD 작업
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TSubClassOf<class UHUDWidget> HUDWidgetClass;
// 	
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TObjectPtr<class UHUDWidget> HUDWidget;
	
};
