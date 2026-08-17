// Copyright Epic Games, Inc. All Rights Reserved.


#include "Kruger_ConclusionPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kruger_ConclusionCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Kruger_Conclusion.h"
#include "EnvironmentLevelSubsystem.h"
#include "Data/PPGameFlowSubsystem.h"
#include "PPPatrolHUDWidget.h"
#include "UI/PPRoundReportWidget.h"
#include "UI/PPRestraintMinigameWidget.h"
#include "Characters/PPPoacherCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SVirtualJoystick.h"

AKruger_ConclusionPlayerController::AKruger_ConclusionPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AKruger_ConclusionCameraManager::StaticClass();

	static ConstructorHelpers::FClassFinder<UPPPatrolHUDWidget> PatrolHUDClass(TEXT("/Game/Poaching_Patrol/UI/WBP_PPPatrolHUD"));
	if (PatrolHUDClass.Succeeded())
	{
		PoachingPatrolHUDWidgetClass = PatrolHUDClass.Class;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MinimapZoomActionFinder(TEXT("/Game/Input/Actions/IA_MinimapZoom.IA_MinimapZoom"));
	if (MinimapZoomActionFinder.Succeeded())
	{
		MinimapZoomAction = MinimapZoomActionFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UPPRoundReportWidget> ReportClass(TEXT("/Game/Poaching_Patrol/UI/WBP_PPRoundReport"));
	if (ReportClass.Succeeded())
	{
		RoundReportWidgetClass = ReportClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UPPRestraintMinigameWidget> RestraintClass(TEXT("/Game/Poaching_Patrol/UI/WBP_PPRestraintMinigame"));
	if (RestraintClass.Succeeded())
	{
		RestraintMinigameWidgetClass = RestraintClass.Class;
	}
}

void AKruger_ConclusionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogKruger_Conclusion, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (bShowPoachingPatrolHUD && IsLocalPlayerController())
	{
		TSubclassOf<UPPPatrolHUDWidget> WidgetClass = PoachingPatrolHUDWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UPPPatrolHUDWidget::StaticClass();
		}

		PoachingPatrolHUDWidget = CreateWidget<UPPPatrolHUDWidget>(this, WidgetClass);
		if (PoachingPatrolHUDWidget)
		{
			PoachingPatrolHUDWidget->AddToPlayerScreen(1);
		}
	}

	if (IsLocalPlayerController())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
		{
			LevelSubsystem->OnRoundEnded.AddUniqueDynamic(this, &AKruger_ConclusionPlayerController::HandlePoachingPatrolRoundEnded);
			if (LevelSubsystem->HasRoundEnded())
			{
				HandlePoachingPatrolRoundEnded(LevelSubsystem->GetFinalRoundResult());
			}
		}

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPPGameFlowSubsystem* Flow = GameInstance->GetSubsystem<UPPGameFlowSubsystem>(); Flow && Flow->ConsumeReplayBypass())
			{
				ApplyReplayMenuBypass();
				GetWorldTimerManager().SetTimer(ReplayMenuBypassTimer, this, &AKruger_ConclusionPlayerController::ApplyReplayMenuBypass, 0.1f, false);
			}
		}
	}
}

void AKruger_ConclusionPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbortActiveRestraint();
	Super::EndPlay(EndPlayReason);
}

bool AKruger_ConclusionPlayerController::StartPoacherRestraint(APPPoacherCharacter* Poacher)
{
	if (!IsLocalPlayerController()
		|| !IsValid(Poacher)
		|| RestraintMinigameWidget
		|| RoundReportWidget
		|| IsPaused()
		|| !Poacher->BeginCaptureAttempt())
	{
		return false;
	}

	TSubclassOf<UPPRestraintMinigameWidget> WidgetClass = RestraintMinigameWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UPPRestraintMinigameWidget::StaticClass();
	}

	RestraintMinigameWidget = CreateWidget<UPPRestraintMinigameWidget>(this, WidgetClass);
	if (!RestraintMinigameWidget)
	{
		Poacher->AbortCaptureAttempt();
		return false;
	}

	ActiveRestraintPoacher = Poacher;
	ActiveRestraintCaptor = GetPawn();
	RestraintMinigameWidget->OnRestraintFinished.AddUniqueDynamic(this, &AKruger_ConclusionPlayerController::HandleRestraintFinished);
	RestraintMinigameWidget->StartSession(Poacher, Poacher->IsPepperSprayed());
	RestraintMinigameWidget->AddToPlayerScreen(50);

	if (PoachingPatrolHUDWidget)
	{
		PreviousPatrolHUDVisibility = PoachingPatrolHUDWidget->GetVisibility();
		PoachingPatrolHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		RestraintMinigameWidget->RemoveFromParent();
		RestraintMinigameWidget = nullptr;
		ActiveRestraintPoacher = nullptr;
		ActiveRestraintCaptor = nullptr;
		Poacher->AbortCaptureAttempt();
		if (PoachingPatrolHUDWidget)
		{
			PoachingPatrolHUDWidget->SetVisibility(PreviousPatrolHUDVisibility);
		}
		return false;
	}

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(RestraintMinigameWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	RestraintMinigameWidget->SetKeyboardFocus();
	return true;
}

void AKruger_ConclusionPlayerController::HandleRestraintFinished(EPPRestraintResult Result)
{
	APPPoacherCharacter* Poacher = ActiveRestraintPoacher.Get();
	APawn* Captor = ActiveRestraintCaptor.Get();

	if (RestraintMinigameWidget)
	{
		RestraintMinigameWidget->OnRestraintFinished.RemoveDynamic(this, &AKruger_ConclusionPlayerController::HandleRestraintFinished);
		RestraintMinigameWidget->RemoveFromParent();
		RestraintMinigameWidget = nullptr;
	}
	ActiveRestraintPoacher = nullptr;
	ActiveRestraintCaptor = nullptr;
	RestoreGameplayAfterRestraint();

	if (IsValid(Poacher))
	{
		Poacher->ResolveCaptureAttempt(Result, Captor);
	}
}

void AKruger_ConclusionPlayerController::RestoreGameplayAfterRestraint()
{
	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (PoachingPatrolHUDWidget && !RoundReportWidget)
	{
		PoachingPatrolHUDWidget->SetVisibility(PreviousPatrolHUDVisibility);
	}
}

void AKruger_ConclusionPlayerController::AbortActiveRestraint()
{
	if (APPPoacherCharacter* Poacher = ActiveRestraintPoacher.Get())
	{
		Poacher->AbortCaptureAttempt();
	}
	if (RestraintMinigameWidget)
	{
		RestraintMinigameWidget->OnRestraintFinished.RemoveDynamic(this, &AKruger_ConclusionPlayerController::HandleRestraintFinished);
		RestraintMinigameWidget->RemoveFromParent();
		RestraintMinigameWidget = nullptr;
	}
	ActiveRestraintPoacher = nullptr;
	ActiveRestraintCaptor = nullptr;
}

void AKruger_ConclusionPlayerController::HandlePoachingPatrolRoundEnded(FPPRoundResult Result)
{
	if (!IsLocalPlayerController() || RoundReportWidget)
	{
		return;
	}

	if (RestraintMinigameWidget)
	{
		HandleRestraintFinished(EPPRestraintResult::Cancelled);
	}

	TSubclassOf<UPPRoundReportWidget> WidgetClass = RoundReportWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UPPRoundReportWidget::StaticClass();
	}

	RoundReportWidget = CreateWidget<UPPRoundReportWidget>(this, WidgetClass);
	if (!RoundReportWidget)
	{
		return;
	}

	RoundReportWidget->SetRoundResult(Result);
	RoundReportWidget->AddToPlayerScreen(100);
	if (PoachingPatrolHUDWidget)
	{
		PoachingPatrolHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(RoundReportWidget->TakeWidget());
	SetInputMode(InputMode);
}

void AKruger_ConclusionPlayerController::ReplayPoachingPatrolDay()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetSubsystem<UPPGameFlowSubsystem>()->RequestReplayBypass();
	}
	ReloadCurrentPatrolLevel();
}

void AKruger_ConclusionPlayerController::ReturnToPoachingPatrolMenu()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetSubsystem<UPPGameFlowSubsystem>()->ClearReplayBypass();
	}
	ReloadCurrentPatrolLevel();
}

void AKruger_ConclusionPlayerController::ReloadCurrentPatrolLevel()
{
	UGameplayStatics::SetGamePaused(this, false);
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelName));
	}
}

void AKruger_ConclusionPlayerController::ApplyReplayMenuBypass()
{
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (IsValid(Widget) && Widget->GetWorld() == GetWorld() && Widget->GetClass()->GetName().Contains(TEXT("WPB_MainMenuOverlay")))
		{
			Widget->RemoveFromParent();
		}
	}

	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AKruger_ConclusionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent); EnhancedInput && MinimapZoomAction)
	{
		EnhancedInput->BindAction(MinimapZoomAction, ETriggerEvent::Started, this, &AKruger_ConclusionPlayerController::HandleMinimapZoom);
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

void AKruger_ConclusionPlayerController::HandleMinimapZoom()
{
	if (PoachingPatrolHUDWidget)
	{
		PoachingPatrolHUDWidget->CycleMinimapZoom();
	}
}

bool AKruger_ConclusionPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
