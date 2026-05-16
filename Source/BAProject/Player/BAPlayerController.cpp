#include "Player/BAPlayerController.h"

#include "BAPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Component/InteractorComponent.h"

ABAPlayerController::ABAPlayerController()
{
	// IMC, Input Action 설정은 BP_PlayerController에서 설정함
	
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
				InputSystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
	
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -75.0f; // 80
		PlayerCameraManager->ViewPitchMax = 60.0f; // 45
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

	if (ensureMsgf(WalkAction, TEXT("WalkAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &ABAPlayerController::OnWalkStarted);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Completed, this, &ABAPlayerController::OnWalkCompleted);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Canceled, this, &ABAPlayerController::OnWalkCompleted);
	}

	if (ensureMsgf(SprintAction, TEXT("SprintAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ABAPlayerController::OnSprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABAPlayerController::OnSprintCompleted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ABAPlayerController::OnSprintCompleted);
	}
	
	if (ensureMsgf(InteractAction, TEXT("InteractAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			InteractAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::OnInteract);
	}
	
	// 임시 기능
	if (ensureMsgf(ToggleStrafeAction, TEXT("ToggleStrafeAction is not configured on %s"), *GetName()))
	{
		EnhancedInputComponent->BindAction(
			ToggleStrafeAction,
			ETriggerEvent::Started,
			this,
			&ABAPlayerController::ToggleStrafe
		);
	}
}

void ABAPlayerController::Move(const FInputActionValue& Value)
{
	FVector2D Movement = Value.Get<FVector2D>();
	if (Movement.SizeSquared() > 1.f)
	{
		Movement.Normalize();
	}

	ABAPlayerCharacter* ControlledCharacter = Cast<ABAPlayerCharacter>(GetPawn());
	if (!ControlledCharacter)
	{
		return;
	}

	bHasMoveInput = !Movement.IsNearlyZero();
	ControlledCharacter->SetMoveInputVector(Movement);
	ApplyMovementStateByModifier();

	// 사다리 모드: 일반 이동 입력 무시 (캐릭터 Tick이 등반 처리)
	if (ControlledCharacter->IsOnLadder())
	{
		return;
	}
	
	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	ControlledCharacter->AddMovementInput(Forward, Movement.Y);
	ControlledCharacter->AddMovementInput(Right, Movement.X);
}

void ABAPlayerController::OnMoveCompleted()
{
	bHasMoveInput = false;

	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->SetMoveInputVector(FVector2D::ZeroVector);
	}

	ApplyMovementStateByModifier();
}

void ABAPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D Rotation = Value.Get<FVector2D>();
	AddYawInput(Rotation.X);
	AddPitchInput(Rotation.Y);
}

void ABAPlayerController::LightAttack()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->Attack();
	}
}

void ABAPlayerController::OnWalkStarted()
{
	bWalkModifierHeld = true;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnWalkCompleted()
{
	bWalkModifierHeld = false;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintStarted()
{
	bSprintModifierHeld = true;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintCompleted()
{
	bSprintModifierHeld = false;
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
	else if (bWalkModifierHeld)
	{
		PC->SetMovementState(EMovementState::Walk);
	}
	else
	{
		PC->SetMovementState(EMovementState::Run);
	}
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
		PC->ExitLadder(PC->GetActorLocation());
		return;
	}
	
	UInteractorComponent* Interactor = PC->GetInteractorComponent();
	if (Interactor)
	{
		Interactor->TryInteract();
	}
}

void ABAPlayerController::ToggleStrafe()
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC)
	{
		return;
	}

	const EPlayerLocomotionMode NextMode =
		PC->GetLocomotionMode() == EPlayerLocomotionMode::Strafe
			? EPlayerLocomotionMode::Free
			: EPlayerLocomotionMode::Strafe;

	PC->SetLocomotionMode(NextMode);
}
