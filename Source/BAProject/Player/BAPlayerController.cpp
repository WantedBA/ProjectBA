#include "Player/BAPlayerController.h"

#include "BAPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Component/ActionComponent.h"
#include "Component/InteractorComponent.h"
#include "UI/SkillTree/SkillTreeWidget.h"
#include "UI/System/SubSystemUI.h"

namespace
{
	FKey PlayerControllerGenericUSBControllerButton(const int32 ButtonNumber)
	{
		return FKey(FName(*FString::Printf(TEXT("GenericUSBController_Button%d"), ButtonNumber)));
	}
}

ABAPlayerController::ABAPlayerController()
{
	// IMC, Input Action 설정은 BP_PlayerController에서 설정함
	UseConsumableAction = CreateDefaultSubobject<UInputAction>(TEXT("UseConsumableAction"));
	UseConsumableAction->ValueType = EInputActionValueType::Boolean;
	
	// static ConstructorHelpers::FClassFinder<UHUDWidget> HUDWidgetRef(TEXT("/Game/BAProject/UI/WBP_HUD.WBP_HUD_C"));
	// if (HUDWidgetRef.Succeeded())
	// {
	// 	HUDWidgetClass = HUDWidgetRef.Class;
	// }
}

void ABAPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Game 시작 시 화면에 입력 고정
	FInputModeGameOnly GameOnlyInputMode;
	SetInputMode(GameOnlyInputMode);
	
	// IMC 설정
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (InputMappingContext)
			{
				ConfigureGamepadInputMappings();
				InputSystem->AddMappingContext(InputMappingContext, 0);
			}
			
			// TODO: 체크포인트 제작 후 이동
			if (CheckpointInputMappingContext)
			{
				InputSystem->AddMappingContext(CheckpointInputMappingContext, 1);
			}
		}
	}
	
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
		PlayerCameraManager->ViewRollMin = 0.0f;
		PlayerCameraManager->ViewRollMax = 0.0f;
	}
	
	// HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	// if (HUDWidget)
	// {
	// 	HUDWidget->AddToViewport();
	// }
}

void ABAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(EnhancedInputComponent, TEXT("EnhancedInputComponent is required.")))
	{
		return;
	}

	if (ensureMsgf(RunAction, TEXT("RunAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Triggered, this, &ABAPlayerController::Move);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &ABAPlayerController::OnMoveCompleted);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Canceled, this, &ABAPlayerController::OnMoveCompleted);
	}

	if (ensureMsgf(LookAction, TEXT("LookAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABAPlayerController::Look);
	}

	if (ensureMsgf(LightAttackAction, TEXT("LightAttackAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Triggered, this, &ABAPlayerController::LightAttack);
	}
	
	if (ensureMsgf(HeavyAttackAction, TEXT("HeavyAttackAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &ABAPlayerController::HeavyAttack);
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &ABAPlayerController::HeavyAttackCompleted);
	}

	if (ensureMsgf(WalkAction, TEXT("WalkAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &ABAPlayerController::ToggleWalk);
	}

	if (ensureMsgf(SprintAction, TEXT("SprintAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ABAPlayerController::OnSprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABAPlayerController::OnSprintCompleted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ABAPlayerController::OnSprintCompleted);
	}

	if (ensureMsgf(GuardAction, TEXT("GuardAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, this, &ABAPlayerController::OnGuardStarted);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Completed, this, &ABAPlayerController::OnGuardCompleted);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Canceled, this, &ABAPlayerController::OnGuardCompleted);
	}

	if (ensureMsgf(UseConsumableAction, TEXT("UseConsumableAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			UseConsumableAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::OnUseConsumable);
	}
	
	if (ensureMsgf(InteractAction, TEXT("InteractAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			InteractAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::OnInteract);
	}
	
	if (ensureMsgf(LockOnAction, TEXT("LockOnAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			LockOnAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::OnLockOnStarted
		);
	}
	
	// 체크포인트(스킬트리 열기)
	if (ensureMsgf(SkillTreeToggleAction, TEXT("SkillTreeToggleAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			SkillTreeToggleAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::ToggleSkillTree
		);
	}
}

void ABAPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateSprintHoldState();
}

void ABAPlayerController::Move(const FInputActionValue& Value)
{
	FVector2D Movement = Value.Get<FVector2D>();
	if (Movement.SizeSquared() > 1.f)
	{
		Movement.Normalize();
	}
	LastMoveInputVector = Movement;

	ABAPlayerCharacter* ControlledCharacter = Cast<ABAPlayerCharacter>(GetPawn());
	if (!ControlledCharacter)
	{
		return;
	}

	// 사다리 위에서는 CanAcceptActionInput()이 false라 일반 입력 차단을 거치지 않고 W/S를 TickLadderClimb로 전달
	if (ControlledCharacter->IsOnLadder())
	{
		bHasMoveInput = !Movement.IsNearlyZero();
		ControlledCharacter->SetMoveInputVector(Movement);
		ApplyMovementStateByModifier();
		return;
	}

	if (ControlledCharacter->IsRecoveryEscapeRequiredForCurrentState())
	{
		if (ControlledCharacter->TryStartRecoveryEscapeMove(Movement))
		{
			bHasMoveInput = !Movement.IsNearlyZero();
			if (UActionComponent* ActionComponent = ControlledCharacter->GetActionComponent())
			{
				ActionComponent->UpdateBufferedActionDirection(ControlledCharacter->GetActionDirectionFromMoveInput(Movement));
			}
		}
		else
		{
			bHasMoveInput = false;
			ControlledCharacter->SetMoveInputVector(FVector2D::ZeroVector);
		}

		ApplyMovementStateByModifier();
		return;
	}

	if (!ControlledCharacter->CanAcceptActionInput() && !ControlledCharacter->IsDamageReacting())
	{
		bHasMoveInput = false;
		ControlledCharacter->SetMoveInputVector(FVector2D::ZeroVector);
		ApplyMovementStateByModifier();
		return;
	}

	bHasMoveInput = !Movement.IsNearlyZero();
	ControlledCharacter->SetMoveInputVector(Movement);
	if (UActionComponent* ActionComponent = ControlledCharacter->GetActionComponent())
	{
		ActionComponent->UpdateBufferedActionDirection(ControlledCharacter->GetActionDirectionFromMoveInput(Movement));
	}
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnMoveCompleted()
{
	bHasMoveInput = false;
	LastMoveInputVector = FVector2D::ZeroVector;

	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->SetMoveInputVector(FVector2D::ZeroVector);
	}

	ApplyMovementStateByModifier();
}

void ABAPlayerController::Look(const FInputActionValue& Value)
{
	ApplyLookInput(Value.Get<FVector2D>());
}

void ABAPlayerController::ApplyLookInput(const FVector2D& LookInput)
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()); PC && PC->IsLockOnTargetLocked())
	{
		PC->SwitchLockOnTargetInput(LookInput);
		return;
	}

	AddYawInput(LookInput.X);
	AddPitchInput(LookInput.Y);
}

void ABAPlayerController::LightAttack()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->TryAttack(EActionCommand::LightAttack);
	}
}

void ABAPlayerController::HeavyAttack()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->TryAttack(EActionCommand::HeavyAttack);
	}
}

void ABAPlayerController::HeavyAttackCompleted()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->ChargeAttackCompleted();
	}
}

void ABAPlayerController::ToggleWalk()
{
	if (const ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
		PC && !PC->CanAcceptActionInput())
	{
		return;
	}

	bWalkToggleEnabled = !bWalkToggleEnabled;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintStarted()
{
	// 사다리 위에서는 탭=닷지 체계 무시 — Space 누르면 즉시 빨리오르기
	if (ABAPlayerCharacter* LadderPC = Cast<ABAPlayerCharacter>(GetPawn());
		LadderPC && LadderPC->IsOnLadder())
	{
		bSprintInputHeld = true;
		bSprintModifierHeld = true;
		ApplyMovementStateByModifier();
		return;
	}

	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		if (PC->IsRecoveryEscapeRequiredForCurrentState())
		{
			const FVector2D DodgeInput = PC->GetMoveInputVector().IsNearlyZero()
				? LastMoveInputVector
				: PC->GetMoveInputVector();
			const EActionDirection DodgeDirection = PC->GetActionDirectionFromMoveInput(DodgeInput);
			PC->TryStartRecoveryEscapeAction(EActionCommand::Dodge, DodgeDirection);
			++RecoveryEscapeSprintInputReleaseBlockCount;
			bSprintInputHeld = false;
			bSprintModifierHeld = false;
			ApplyMovementStateByModifier();
			return;
		}

		if (PC->IsDamageReacting())
		{
			const EActionDirection DodgeDirection = PC->GetActionDirectionFromMoveInput(PC->GetMoveInputVector());
			PC->SetKnockDownGetUpDodgeInputHeld(true, DodgeDirection);
			PC->RequestKnockDownGetUpDodgeEscape(DodgeDirection);
			bSprintInputHeld = false;
			bSprintModifierHeld = false;
			ApplyMovementStateByModifier();
			return;
		}

		if (!PC->CanAcceptActionInput() && !PC->IsLandingRecoveryActive())
		{
			bSprintInputHeld = false;
			bSprintModifierHeld = false;
			ApplyMovementStateByModifier();
			return;
		}

		if (!PC->IsLandingRecoveryActive())
		{
			PC->CancelGuardForSprintInput();
		}
	}

	bSprintInputHeld = true;
	bSprintModifierHeld = false;
	SprintDodgePressedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintCompleted()
{
	// 사다리 위에서는 닷지 트리거 X, 단순히 sprint 해제
	if (ABAPlayerCharacter* LadderPC = Cast<ABAPlayerCharacter>(GetPawn());
		LadderPC && LadderPC->IsOnLadder())
	{
		bSprintInputHeld = false;
		bSprintModifierHeld = false;
		ApplyMovementStateByModifier();
		return;
	}

	if (RecoveryEscapeSprintInputReleaseBlockCount > 0)
	{
		--RecoveryEscapeSprintInputReleaseBlockCount;
		bSprintInputHeld = false;
		bSprintModifierHeld = false;
		ApplyMovementStateByModifier();
		return;
	}

	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
		PC && PC->IsRecoveryEscapeRequiredForCurrentState())
	{
		bSprintInputHeld = false;
		bSprintModifierHeld = false;
		ApplyMovementStateByModifier();
		return;
	}

	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
		PC && PC->IsDamageReacting())
	{
		PC->SetKnockDownGetUpDodgeInputHeld(false, EActionDirection::Any);
		bSprintInputHeld = false;
		bSprintModifierHeld = false;
		ApplyMovementStateByModifier();
		return;
	}

	const bool bShouldDodge = bSprintInputHeld && IsSprintDodgeTap();

	bSprintInputHeld = false;
	bSprintModifierHeld = false;
	if (bShouldDodge && TryStartDodgeAction())
	{
		return;
	}

	ApplyMovementStateByModifier();
}

bool ABAPlayerController::IsLadderSprintGamepadButtonActive() const
{
	const FKey GenericCircleButton = PlayerControllerGenericUSBControllerButton(3);
	return IsInputKeyDown(EKeys::Gamepad_FaceButton_Right)
		|| WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)
		|| IsInputKeyDown(GenericCircleButton)
		|| WasInputKeyJustPressed(GenericCircleButton);
}

void ABAPlayerController::UpdateSprintHoldState()
{
	if (!bSprintInputHeld || bSprintModifierHeld)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetTimeSeconds() - SprintDodgePressedTime < SprintHoldRequiredTime)
	{
		return;
	}

	bSprintModifierHeld = true;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::ApplyMovementStateByModifier() const
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return;
	}

	PC->SetHasMoveInput(bHasMoveInput);

	if (!bHasMoveInput)
	{
		PC->SetMovementState(EMovementState::Run);
		return;
	}

	if (bSprintModifierHeld)
	{
		PC->SetMovementState(EMovementState::Sprint);
	}
	else if (bWalkToggleEnabled)
	{
		PC->SetMovementState(EMovementState::Walk);
	}
	else
	{
		PC->SetMovementState(EMovementState::Run);
	}
}

bool ABAPlayerController::IsSprintDodgeTap() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return World->GetTimeSeconds() - SprintDodgePressedTime <= SprintDodgeTapMaxTime;
}

bool ABAPlayerController::TryStartDodgeAction() const
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return false;
	}

	FVector2D DodgeInput = PC->IsLandingRecoveryActive()
		? LastMoveInputVector
		: PC->GetMoveInputVector();
	if (DodgeInput.IsNearlyZero())
	{
		DodgeInput = LastMoveInputVector;
	}
	if (PC->IsRecoveryEscapeRequiredForCurrentState())
	{
		const FVector2D RecoveryDodgeInput = DodgeInput.IsNearlyZero()
			? LastMoveInputVector
			: DodgeInput;
		const EActionDirection RecoveryDodgeDirection = PC->GetActionDirectionFromMoveInput(RecoveryDodgeInput);
		return PC->TryStartRecoveryEscapeAction(EActionCommand::Dodge, RecoveryDodgeDirection);
	}

	const EActionDirection DodgeDirection = PC->GetActionDirectionFromMoveInput(DodgeInput);
	if (!PC->ResolveLandingRecoveryBeforeAction(EActionCommand::Dodge, DodgeDirection))
	{
		return false;
	}

	if (!PC->CanAcceptActionInput())
	{
		if (PC->IsDamageReacting())
		{
			PC->RequestKnockDownGetUpDodgeEscape(DodgeDirection);
		}
		return false;
	}

	UActionComponent* ActionComponent = PC->GetActionComponent();
	if (!ActionComponent)
	{
		return false;
	}

	return DodgeInput.IsNearlyZero()
		? ActionComponent->TryStartAction(EActionCommand::Dodge, DodgeDirection)
		: ActionComponent->TryStartActionOfType(EActionCommand::Dodge, DodgeDirection, EActionType::DodgeRoll);
}

void ABAPlayerController::OnGuardStarted()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->TryStartGuard();
	}
}

void ABAPlayerController::OnGuardCompleted()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->StopGuard();
	}
}

void ABAPlayerController::OnUseConsumable()
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return;
	}

	if (!PC->ResolveLandingRecoveryBeforeAction(EActionCommand::UseConsumable))
	{
		return;
	}

	if (!PC->CanAcceptActionInput())
	{
		return;
	}

	PC->TryHeal();
}

void ABAPlayerController::OnInteract()
{
	ABAPlayerCharacter* PC = 
		Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return;
	}

	// 사다리 매달린 상태: 카메라 방향과 무관하게 즉시 이탈
	if (PC->IsOnLadder())
	{
		if (IsLadderSprintGamepadButtonActive())
		{
			return;
		}

		PC->ExitLadder(PC->GetActorLocation());
		return;
	}

	if (!PC->CanAcceptActionInput())
	{
		return;
	}
	
	UInteractorComponent* Interactor = PC->GetInteractorComponent();
	if (Interactor)
	{
		Interactor->TryInteract();
	}
}

void ABAPlayerController::OnLockOnStarted()
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return;
	}

	if (!PC->CanAcceptActionInput())
	{
		return;
	}

	PC->ToggleLockOnTargeting();
}

void ABAPlayerController::ToggleSkillTree()
{
	if (SkillTreeWidget && SkillTreeWidget->IsInViewport())
	{
		SkillTreeWidget->ClosePopup();
		SkillTreeWidget = nullptr;
		return;
	}

	if (!ensureMsgf(SkillTreeWidgetClass, TEXT("SkillTreeWidgetClass is not configured on %s"), *GetName()))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	USubSystemUI* UISubsystem = GameInstance->GetSubsystem<USubSystemUI>();
	if (!ensureMsgf(UISubsystem, TEXT("USubSystemUI is not available.")))
	{
		return;
	}

	SkillTreeWidget = Cast<USkillTreeWidget>(UISubsystem->PushUIByClass(SkillTreeWidgetClass));
}

void ABAPlayerController::ConfigureGamepadInputMappings()
{
	MapActionKeyIfMissing(RunAction, EKeys::Gamepad_Left2D);
	MapActionKeyIfMissing(LookAction, EKeys::Gamepad_Right2D);

	MapActionKeyIfMissing(LightAttackAction, EKeys::Gamepad_RightShoulder);
	MapActionKeyIfMissing(LightAttackAction, PlayerControllerGenericUSBControllerButton(6)); // DualSense R1

	MapActionKeyIfMissing(HeavyAttackAction, EKeys::Gamepad_RightTrigger);
	MapActionKeyIfMissing(HeavyAttackAction, PlayerControllerGenericUSBControllerButton(8)); // DualSense R2

	MapActionKeyIfMissing(GuardAction, EKeys::Gamepad_LeftShoulder);
	MapActionKeyIfMissing(GuardAction, PlayerControllerGenericUSBControllerButton(5)); // DualSense L1

	// B/Circle은 사다리 위에서 빠른 등반에 쓰이는 Sprint 입력이다.
	// 기존 IMC에 Interact로 남아 있으면 OnInteract가 같이 호출되어 사다리에서 즉시 이탈한다.
	UnmapActionKeyIfPresent(InteractAction, EKeys::Gamepad_FaceButton_Right);
	UnmapActionKeyIfPresent(InteractAction, PlayerControllerGenericUSBControllerButton(3));
	MapActionKeyIfMissing(SprintAction, EKeys::Gamepad_FaceButton_Right);
	MapActionKeyIfMissing(SprintAction, PlayerControllerGenericUSBControllerButton(3)); // DualSense Circle

	MapActionKeyIfMissing(InteractAction, EKeys::Gamepad_FaceButton_Bottom);
	MapActionKeyIfMissing(InteractAction, PlayerControllerGenericUSBControllerButton(2)); // DualSense Cross

	MapActionKeyIfMissing(UseConsumableAction, EKeys::Gamepad_FaceButton_Left);
	MapActionKeyIfMissing(UseConsumableAction, PlayerControllerGenericUSBControllerButton(1)); // DualSense Square

	MapActionKeyIfMissing(LockOnAction, EKeys::Gamepad_RightThumbstick);
	MapActionKeyIfMissing(LockOnAction, PlayerControllerGenericUSBControllerButton(12)); // DualSense R3
}

void ABAPlayerController::MapActionKeyIfMissing(const UInputAction* Action, const FKey& Key)
{
	if (!InputMappingContext || !Action || !Key.IsValid())
	{
		return;
	}

	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action && Mapping.Key == Key)
		{
			return;
		}
	}

	InputMappingContext->MapKey(Action, Key);
}

void ABAPlayerController::UnmapActionKeyIfPresent(const UInputAction* Action, const FKey& Key)
{
	if (!InputMappingContext || !Action || !Key.IsValid())
	{
		return;
	}

	bool bHasMapping = false;
	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action && Mapping.Key == Key)
		{
			bHasMapping = true;
			break;
		}
	}

	if (bHasMapping)
	{
		InputMappingContext->UnmapKey(Action, Key);
	}
}
