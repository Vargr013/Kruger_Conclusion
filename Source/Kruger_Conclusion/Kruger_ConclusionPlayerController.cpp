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
#include "UI/PPTutorialWidget.h"
#include "UI/PPUpgradeMenuWidget.h"
#include "UI/PPRestraintMinigameWidget.h"
#include "UI/PPPauseMenuWidget.h"
#include "UI/PPMainMenuWidget.h"
#include "Characters/PPPoacherCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "InputCoreTypes.h"

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
			TutorialHUDWidget = CreateWidget<UPPTutorialWidget>(this, UPPTutorialWidget::StaticClass());
			if (TutorialHUDWidget) TutorialHUDWidget->AddToPlayerScreen(5);
			LevelSubsystem->OnRoundEnded.AddUniqueDynamic(this, &AKruger_ConclusionPlayerController::HandlePoachingPatrolRoundEnded);
			if (LevelSubsystem->HasRoundEnded())
			{
				HandlePoachingPatrolRoundEnded(LevelSubsystem->GetFinalRoundResult());
			}
		}

		SuppressLegacyMainMenuOverlay();

		bool bBypassMainMenu = false;
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPPGameFlowSubsystem* Flow = GameInstance->GetSubsystem<UPPGameFlowSubsystem>())
			{
				bBypassMainMenu = Flow->ConsumeReplayBypass();
			}
		}

		if (bBypassMainMenu)
		{
			ApplyReplayMenuBypass();
			GetWorldTimerManager().SetTimer(ReplayMenuBypassTimer, this, &AKruger_ConclusionPlayerController::ApplyReplayMenuBypass, 0.1f, false);
		}
		else if (!RoundReportWidget && !TutorialResultWidget && !UpgradeMenuWidget)
		{
			OpenPoachingPatrolMainMenu();
		}
	}
}

void AKruger_ConclusionPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalPlayerController() || IsPaused() || bShowMouseCursor || !GetPawn())
	{
		return;
	}
	UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	// I waited until the menu closed so its opening frame did not start the clock.
	if (Rules && !Rules->HasPatrolStarted() && !Rules->HasRoundEnded() && !IsMainMenuVisible())
	{
		Rules->StartPatrol();
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
		|| TutorialResultWidget || RoundReportWidget || UpgradeMenuWidget
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
	if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
	{
		LevelSubsystem->CancelAllPoacherAttackWindups();
	}
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
	if (PoachingPatrolHUDWidget && !RoundReportWidget && !UpgradeMenuWidget)
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
	if (!IsLocalPlayerController() || TutorialResultWidget || RoundReportWidget || UpgradeMenuWidget)
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

void AKruger_ConclusionPlayerController::ShowTutorialResult(bool bSuccess, const FText& Message)
{
	if (!IsLocalPlayerController() || TutorialResultWidget || UpgradeMenuWidget) return;
	AbortActiveRestraint();
	TutorialResultWidget = CreateWidget<UPPTutorialWidget>(this, UPPTutorialWidget::StaticClass());
	if (!TutorialResultWidget) return;
	TutorialResultWidget->ShowResult(bSuccess, Message);
	TutorialResultWidget->AddToPlayerScreen(100);
	if (PoachingPatrolHUDWidget) PoachingPatrolHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(TutorialResultWidget->TakeWidget());
	SetInputMode(Mode);
}

void AKruger_ConclusionPlayerController::ShowUpgradeMenu(EPPUpgradeContinueDestination Destination)
{
	if (!IsLocalPlayerController() || UpgradeMenuWidget)
	{
		return;
	}

	PendingUpgradeContinueDestination = Destination;

	if (RoundReportWidget)
	{
		RoundReportWidget->RemoveFromParent();
		RoundReportWidget = nullptr;
	}
	if (TutorialResultWidget)
	{
		TutorialResultWidget->RemoveFromParent();
		TutorialResultWidget = nullptr;
	}

	UpgradeMenuWidget = CreateWidget<UPPUpgradeMenuWidget>(this, UPPUpgradeMenuWidget::StaticClass());
	if (!UpgradeMenuWidget)
	{
		return;
	}

	UpgradeMenuWidget->Configure(Destination);
	UpgradeMenuWidget->AddToPlayerScreen(110);
	if (PoachingPatrolHUDWidget)
	{
		PoachingPatrolHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(UpgradeMenuWidget->TakeWidget());
	SetInputMode(InputMode);
}

void AKruger_ConclusionPlayerController::ContinueFromUpgradeMenu()
{
	if (UpgradeMenuWidget)
	{
		UpgradeMenuWidget->RemoveFromParent();
		UpgradeMenuWidget = nullptr;
	}

	if (PendingUpgradeContinueDestination == EPPUpgradeContinueDestination::StartNormalPatrol)
	{
		RestartPoachingPatrolInMode(false);
		return;
	}

	ReplayPoachingPatrolDay();
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
	if (UpgradeMenuWidget)
	{
		UpgradeMenuWidget->RemoveFromParent();
		UpgradeMenuWidget = nullptr;
	}
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
	if (RoundReportWidget)
	{
		RoundReportWidget->RemoveFromParent();
		RoundReportWidget = nullptr;
	}
	if (TutorialResultWidget)
	{
		TutorialResultWidget->RemoveFromParent();
		TutorialResultWidget = nullptr;
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPPGameFlowSubsystem* Flow = GameInstance->GetSubsystem<UPPGameFlowSubsystem>())
		{
			Flow->ClearReplayBypass();
			Flow->ClearRequestedPatrolMode();
		}
	}
	ReloadCurrentPatrolLevel();
}

void AKruger_ConclusionPlayerController::OpenPoachingPatrolMainMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	SuppressLegacyMainMenuOverlay();

	if (!MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UPPMainMenuWidget>(this, UPPMainMenuWidget::StaticClass());
		if (!MainMenuWidget)
		{
			return;
		}
		MainMenuWidget->AddToPlayerScreen(120);
	}

	MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	MainMenuWidget->ShowRootMenu();
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	MainMenuWidget->SetKeyboardFocus();
}

void AKruger_ConclusionPlayerController::ClosePoachingPatrolMainMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	SuppressLegacyMainMenuOverlay();
}

void AKruger_ConclusionPlayerController::StartPoachingPatrolFromMenu()
{
	ClosePoachingPatrolMainMenu();
	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AKruger_ConclusionPlayerController::QuitPoachingPatrolGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

bool AKruger_ConclusionPlayerController::RestartPoachingPatrolInMode(bool bTutorialMode)
{
	UEnvironmentLevelSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr;
	UGameInstance* GameInstance = GetGameInstance();
	if (!Rules || !Rules->GetTutorialDirector() || !GameInstance)
	{
		UE_LOG(LogKruger_Conclusion, Error, TEXT("Could not restart in %s because patrol mode switching is unavailable in this level."),
			bTutorialMode ? TEXT("Tutorial") : TEXT("Normal Patrol"));
		return false;
	}

	if (UPPGameFlowSubsystem* Flow = GameInstance->GetSubsystem<UPPGameFlowSubsystem>())
	{
		Flow->RequestPatrolMode(bTutorialMode);
		Flow->RequestReplayBypass();
		ReloadCurrentPatrolLevel();
		return true;
	}

	return false;
}

void AKruger_ConclusionPlayerController::OpenGraphicsSettings()
{
	OpenPoachingPatrolMainMenu();
	if (MainMenuWidget)
	{
		MainMenuWidget->ShowOptionsMenu();
	}
}

void AKruger_ConclusionPlayerController::CloseGraphicsSettings()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->ShowRootMenu();
	}
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
	ClosePoachingPatrolMainMenu();
	SuppressLegacyMainMenuOverlay();
	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AKruger_ConclusionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	FInputKeyBinding& PauseBinding = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AKruger_ConclusionPlayerController::ToggleGameplayPause);
	PauseBinding.bExecuteWhenPaused = true;
	FInputKeyBinding& TabPauseBinding = InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AKruger_ConclusionPlayerController::ToggleGameplayPause);
	TabPauseBinding.bExecuteWhenPaused = true;

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

void AKruger_ConclusionPlayerController::ToggleGameplayPause()
{
	if (!IsLocalPlayerController() || RestraintMinigameWidget || TutorialResultWidget || RoundReportWidget || UpgradeMenuWidget || IsMainMenuVisible())
	{
		return;
	}

	if (PauseMenuWidget)
	{
		ClosePauseOverlay();
	}
	else
	{
		OpenPauseOverlay();
	}
}

void AKruger_ConclusionPlayerController::OpenPauseOverlay()
{
	if (!IsLocalPlayerController() || PauseMenuWidget || RestraintMinigameWidget || TutorialResultWidget || RoundReportWidget || UpgradeMenuWidget || IsMainMenuVisible())
	{
		return;
	}

	PauseMenuWidget = CreateWidget<UPPPauseMenuWidget>(this, UPPPauseMenuWidget::StaticClass());
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->AddToPlayerScreen(75);
	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
		return;
	}
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	PauseMenuWidget->SetKeyboardFocus();
}

void AKruger_ConclusionPlayerController::ClosePauseOverlay()
{
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;
	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

bool AKruger_ConclusionPlayerController::IsMainMenuVisible() const
{
	return MainMenuWidget && MainMenuWidget->IsInViewport()
		&& MainMenuWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& MainMenuWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void AKruger_ConclusionPlayerController::SuppressLegacyMainMenuOverlay()
{
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (IsValid(Widget)
			&& Widget->GetWorld() == GetWorld()
			&& Widget->GetClass()->GetName().Contains(TEXT("WPB_MainMenuOverlay")))
		{
			Widget->RemoveFromParent();
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
