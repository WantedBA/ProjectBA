#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BAPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class USkillTreeWidget;

/**
 * 플레이어 입력을 캐릭터의 이동 상태, 액션 명령, 상호작용으로 변환하는 컨트롤러.
 *
 * Enhanced Input 액션은 블루프린트에서 할당하고, 이 클래스는 입력 지속 시간과
 * 이동 입력 방향을 해석해 걷기/질주/회피/공격/상호작용 명령을 캐릭터 컴포넌트에 전달한다.
 */
UCLASS()
class BAPROJECT_API ABAPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABAPlayerController();
	
protected:
	// 입력 모드, IMC, 카메라 제한을 초기화한다.
	virtual void BeginPlay() override;

	// Enhanced Input 액션을 각 입력 핸들러에 바인딩한다.
	virtual void SetupInputComponent() override;

	// 질주 버튼 홀드 시간이 지났는지 매 프레임 확인한다.
	virtual void PlayerTick(float DeltaTime) override;
	
private:
	// TODO: KM/Gamepad IMC 나누기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> GuardAction;

// 체크포인트 인풋
	// 체크포인트에서 활성화할 IMC
	UPROPERTY(EditDefaultsOnly, Category = "Input|Checkpoint")
	TObjectPtr<UInputMappingContext> CheckpointInputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input|Checkpoint")
	TObjectPtr<UInputAction> SkillTreeToggleAction;

	UPROPERTY(EditDefaultsOnly, Category = "UI|SkillTree")
	TSubclassOf<USkillTreeWidget> SkillTreeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USkillTreeWidget> SkillTreeWidget;
	
	// 이 시간 이내에 질주 입력을 떼면 회피 입력으로 해석한다.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float SprintDodgeTapMaxTime = 0.25f;

	// 이 시간 이상 질주 입력을 유지해야 실제 질주 modifier가 켜진다.
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.0"))
	float SprintHoldRequiredTime = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> InteractAction;
	
	// 락온 개발 전 임시 버튼
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleStrafeAction;
	
	// Input handlers
	// 이동 입력을 정규화해 캐릭터에 전달하고 액션 버퍼 방향을 갱신한다.
	void Move(const FInputActionValue& Value);

	// 이동 입력이 끝났을 때 입력 벡터를 초기화한다.
	void OnMoveCompleted();

	// 카메라 yaw/pitch 입력을 컨트롤러 회전에 적용한다.
	void Look(const FInputActionValue& Value);

	// 기본 공격 입력을 캐릭터 공격 진입점으로 전달한다.
	void LightAttack();
	
	// 강공격(우클릭) 입력을 캐릭터 공격 진입점으로 전달한다.
	void HeavyAttack();

	// 걷기 토글 상태를 전환하고 이동 상태를 다시 계산한다.
	void ToggleWalk();

	// 질주/회피 공용 입력 시작 시점을 기록한다.
	void OnSprintStarted();

	// 질주/회피 공용 입력 종료 시 회피 탭 여부를 판단한다.
	void OnSprintCompleted();

	// 홀드 시간이 충족되면 질주 modifier를 활성화한다.
	void UpdateSprintHoldState();

	// 현재 입력 modifier 조합을 캐릭터의 DesiredGait로 반영한다.
	void ApplyMovementStateByModifier() const;

	// 질주 입력이 짧은 탭으로 끝났는지 반환한다.
	bool IsSprintDodgeTap() const;

	// 현재 이동 입력 방향을 사용해 Dodge 액션을 시작한다.
	void TryStartDodgeAction() const;

	// 가드
	void OnGuardStarted();
	void OnGuardCompleted();

	// 사다리 상태면 이탈하고, 아니면 현재 상호작용 대상을 실행한다.
	void OnInteract();

	// 임시 기능
	void ToggleStrafe();
	
	// 스킬트리 열기
	void ToggleSkillTree();

	bool bWalkToggleEnabled = false;
	bool bSprintInputHeld = false;
	bool bSprintModifierHeld = false;
	bool bHasMoveInput = false;
	double SprintDodgePressedTime = 0.0;
	
};
