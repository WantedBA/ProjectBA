#include "Player/BAPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ABAPlayerCharacter::ABAPlayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 800.0f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionRef(
		TEXT("")
	);
	if (MoveActionRef.Succeeded())
	{
		MoveAction = MoveActionRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionRef(
		TEXT("")
	);
	if (LookActionRef.Succeeded())
	{
		LookAction = LookActionRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> AttackActionRef(
		TEXT("")
	);
	if (AttackActionRef.Succeeded())
	{
		AttackAction = AttackActionRef.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextRef(
		TEXT("")
	);
	if (InputMappingContextRef.Succeeded())
	{
		InputMappingContext = InputMappingContextRef.Object;
	}
}

void ABAPlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent
		= Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (ensureMsgf(
		EnhancedInputComponent != nullptr,
		TEXT("APlayerCharacter requires an EnhancedInputComponent to bind input actions.")))
	{
		if (ensureMsgf(
			MoveAction != nullptr,
			TEXT("MoveAction is not configured on %s."),
			*GetName()))
		{
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Triggered,
				this,
				&ABAPlayerCharacter::Move
			);
		}

		if (ensureMsgf(
			LookAction != nullptr,
			TEXT("LookAction is not configured on %s."),
			*GetName()))
		{
			EnhancedInputComponent->BindAction(
				LookAction,
				ETriggerEvent::Triggered,
				this,
				&ABAPlayerCharacter::Look
			);
		}

		if (ensureMsgf(
			AttackAction != nullptr,
			TEXT("AttackAction is not configured on %s."),
			*GetName()))
		{
			EnhancedInputComponent->BindAction(
				AttackAction,
				ETriggerEvent::Triggered,
				this,
				&ABAPlayerCharacter::Attack
			);
		}
	}
	
	if (InputMappingContext == nullptr)
	{
		return;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				InputSystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}


void ABAPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D Movement = Value.Get<FVector2D>();

	FRotator Rotation = GetControlRotation();
	FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightVector	= FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardVector, Movement.Y);
	AddMovementInput(RightVector, Movement.X);
}

void ABAPlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D RotationValue = Value.Get<FVector2D>();

	AddControllerYawInput(RotationValue.X);
	AddControllerPitchInput(RotationValue.Y);
}

void ABAPlayerCharacter::Attack()
{
	// ProcessComboCommand();
}
