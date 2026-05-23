#include "Player/BAPlayerController.h"

#include "BAPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "InputMappingContext.h"
#include "Component/ActionComponent.h"
#include "Component/InteractorComponent.h"
#include "UI/SkillTree/SkillTreeWidget.h"
#include "UI/System/SubSystemUI.h"

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
			
			// TODO: 체크포인트 제작 후 이동
			if (CheckpointInputMappingContext)
			{
				InputSystem->AddMappingContext(CheckpointInputMappingContext, 1);
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

	ABAPlayerCharacter* ControlledCharacter = Cast<ABAPlayerCharacter>(GetPawn());
	if (!ControlledCharacter)
	{
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
	bWalkToggleEnabled = !bWalkToggleEnabled;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintStarted()
{
	if (ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn()))
	{
		PC->CancelGuardForSprintInput();
	}

	bSprintInputHeld = true;
	bSprintModifierHeld = false;
	SprintDodgePressedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ApplyMovementStateByModifier();
}

void ABAPlayerController::OnSprintCompleted()
{
	const bool bShouldDodge = IsSprintDodgeTap();

	bSprintInputHeld = false;
	bSprintModifierHeld = false;
	ApplyMovementStateByModifier();

	if (bShouldDodge)
	{
		TryStartDodgeAction();
	}
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

void ABAPlayerController::TryStartDodgeAction() const
{
	ABAPlayerCharacter* PC = Cast<ABAPlayerCharacter>(GetPawn());
	if (!PC || !PC->CanAcceptActionInput())
	{
		return;
	}

	UActionComponent* ActionComponent = PC->GetActionComponent();
	if (!ActionComponent)
	{
		return;
	}

	const EActionDirection DodgeDirection = PC->GetActionDirectionFromMoveInput(PC->GetMoveInputVector());
	ActionComponent->TryStartAction(EActionCommand::Dodge, DodgeDirection);
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
