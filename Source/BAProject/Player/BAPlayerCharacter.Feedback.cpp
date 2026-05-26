#include "Player/BAPlayerCharacter.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "TimerManager.h"

#if PLATFORM_WINDOWS
namespace
{
	struct FXInputVibration
	{
		uint16 LeftMotorSpeed = 0;
		uint16 RightMotorSpeed = 0;
	};

	using FXInputSetStateFunction = uint32 (*)(uint32, FXInputVibration*);

	void* LoadXInputDll()
	{
		static void* XInputDllHandle = nullptr;
		static bool bTriedLoad = false;
		if (bTriedLoad)
		{
			return XInputDllHandle;
		}

		bTriedLoad = true;
		for (const TCHAR* DllName : { TEXT("xinput1_4.dll"), TEXT("xinput1_3.dll"), TEXT("xinput9_1_0.dll") })
		{
			XInputDllHandle = FPlatformProcess::GetDllHandle(DllName);
			if (XInputDllHandle)
			{
				break;
			}
		}

		return XInputDllHandle;
	}

	FXInputSetStateFunction LoadXInputSetState()
	{
		static FXInputSetStateFunction XInputSetState = nullptr;
		static bool bTriedLoad = false;
		if (bTriedLoad)
		{
			return XInputSetState;
		}

		bTriedLoad = true;
		if (void* XInputDllHandle = LoadXInputDll())
		{
			XInputSetState = reinterpret_cast<FXInputSetStateFunction>(
				FPlatformProcess::GetDllExport(XInputDllHandle, TEXT("XInputSetState")));
		}

		return XInputSetState;
	}

	bool SetXInputRumble(const int32 UserIndex, const float LeftMotor, const float RightMotor)
	{
		FXInputSetStateFunction XInputSetState = LoadXInputSetState();
		if (!XInputSetState)
		{
			return false;
		}

		FXInputVibration Vibration;
		Vibration.LeftMotorSpeed = static_cast<uint16>(FMath::RoundToInt(FMath::Clamp(LeftMotor, 0.f, 1.f) * 65535.f));
		Vibration.RightMotorSpeed = static_cast<uint16>(FMath::RoundToInt(FMath::Clamp(RightMotor, 0.f, 1.f) * 65535.f));
		return XInputSetState(static_cast<uint32>(FMath::Clamp(UserIndex, 0, 3)), &Vibration) == 0;
	}
}
#endif

void ABAPlayerCharacter::PlayDamageReactionForceFeedback(
	const EBADamageReactionType DamageReactionType,
	const bool bGuarding,
	const bool bGuardBreak) const
{
	if (bGuardBreak)
	{
		PlayConfiguredForceFeedback(GuardBreakForceFeedbackIntensity, GuardBreakForceFeedbackDuration);
		return;
	}

	if (bGuarding)
	{
		PlayConfiguredForceFeedback(GuardHitForceFeedbackIntensity, GuardHitForceFeedbackDuration);
		return;
	}

	switch (DamageReactionType)
	{
	case EBADamageReactionType::KnockDown:
		PlayConfiguredForceFeedback(KnockDownForceFeedbackIntensity, KnockDownForceFeedbackDuration);
		break;
	case EBADamageReactionType::LargeHitReact:
		PlayConfiguredForceFeedback(LargeHitReactForceFeedbackIntensity, LargeHitReactForceFeedbackDuration);
		break;
	case EBADamageReactionType::HitReact:
	default:
		PlayConfiguredForceFeedback(HitReactForceFeedbackIntensity, HitReactForceFeedbackDuration);
		break;
	}
}

void ABAPlayerCharacter::PlayPerfectGuardForceFeedback() const
{
	PlayConfiguredForceFeedback(PerfectGuardForceFeedbackIntensity, PerfectGuardForceFeedbackDuration);
}

void ABAPlayerCharacter::PlayGuardStartForceFeedback() const
{
	PlayConfiguredForceFeedback(GuardStartForceFeedbackIntensity, GuardStartForceFeedbackDuration);
}

void ABAPlayerCharacter::PlayConfiguredForceFeedback(const float Intensity, const float Duration) const
{
	if (!bEnableGamepadForceFeedback || Intensity <= 0.f || Duration <= 0.f)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	PlayerController->PlayDynamicForceFeedback(
		FMath::Clamp(Intensity, 0.f, 1.f),
		Duration,
		true,
		true,
		true,
		true,
		EDynamicForceFeedbackAction::Start);

	PlayXInputForceFeedback(Intensity, Duration);
}

void ABAPlayerCharacter::PlayXInputForceFeedback(const float Intensity, const float Duration) const
{
	if (!bEnableXInputForceFeedbackFallback || Intensity <= 0.f || Duration <= 0.f)
	{
		return;
	}

#if PLATFORM_WINDOWS
	const float ClampedIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
	if (!SetXInputRumble(XInputForceFeedbackUserIndex, ClampedIntensity, ClampedIntensity))
	{
		return;
	}

	++XInputForceFeedbackPlaybackId;
	const int32 PlaybackId = XInputForceFeedbackPlaybackId;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(XInputForceFeedbackStopTimerHandle);
		World->GetTimerManager().SetTimer(
			XInputForceFeedbackStopTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, PlaybackId]()
			{
				StopXInputForceFeedback(PlaybackId);
			}),
			Duration,
			false);
	}
#endif
}

void ABAPlayerCharacter::StopXInputForceFeedback() const
{
#if PLATFORM_WINDOWS
	SetXInputRumble(XInputForceFeedbackUserIndex, 0.f, 0.f);
#endif
}

void ABAPlayerCharacter::StopXInputForceFeedback(const int32 PlaybackId) const
{
	if (PlaybackId != XInputForceFeedbackPlaybackId)
	{
		return;
	}

	StopXInputForceFeedback();
}
