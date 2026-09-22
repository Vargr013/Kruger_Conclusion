#include "Actors/PPTutorialDirector.h"
#include "Actors/PPTutorialTarget.h"
#include "Actors/PPRestPoint.h"
#include "Actors/PPArrestZone.h"
#include "Characters/PPPoacherCharacter.h"
#include "Characters/ARangerCharacter.h"
#include "Kruger_ConclusionPlayerController.h"
#include "EnvironmentLevelSubsystem.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

APPTutorialDirector::APPTutorialDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
	auto Add = [this](EPPTutorialStage Key, const TCHAR* Objective, const TCHAR* Hint, const TCHAR* Line)
	{
		FPPTutorialLesson Lesson;
		Lesson.Objective = FText::FromString(Objective);
		Lesson.Hint = FText::FromString(Hint);
		if (*Line)
		{
			FPPTutorialLine Speech;
			Speech.Speaker = FText::FromString(TEXT("Senior Ranger"));
			Speech.Text = FText::FromString(Line);
			Speech.Seconds = 7.0f;
			Lesson.Lines.Add(Speech);
		}
		Lessons.Add(Key, Lesson);
	};
	Add(EPPTutorialStage::Arrival, TEXT("Meet the senior ranger at the post"), TEXT("Follow the camp signs to the ranger beside the tent."), TEXT("Over here, rookie. Your first patrol starts at the post."));
	Add(EPPTutorialStage::Briefing, TEXT("Receive your assignment"), TEXT("Listen to the briefing. Subtitles contain the full assignment."), TEXT("You found the post. Good start. Today: supplies, equipment, then one short patrol. We bring people in alive."));
	Add(EPPTutorialStage::Supply, TEXT("Restock at the Land Cruiser"), TEXT("Stand beside the vehicle and hold E until restocking completes, even if you are full."), TEXT("Check your supplies at the vehicle. A full kit now saves a long walk later."));
	Add(EPPTutorialStage::Practice, TEXT("Hit the marked practice target"), TEXT("Aim and fire with the left mouse button. Switch equipment with the mouse wheel. Restock whenever needed."), TEXT("Try your equipment on the marked target. One clean hit will do."));
	Add(EPPTutorialStage::Patrol, TEXT("Reach the reported sighting"), TEXT("Follow the trail signs. Both paths around the rocks reach the clearing."), TEXT("A sighting came in from the clearing. Take either approach. I'll stay at the post and guide you by radio."));
	Add(EPPTutorialStage::Subdual, TEXT("Subdue the poacher"), TEXT("Use your non-lethal equipment. Pepper spray works at close range; keep the target in view."), TEXT("Keep control of the situation. Subdue them before attempting restraint."));
	Add(EPPTutorialStage::Restraint, TEXT("Restrain the subdued poacher"), TEXT("Move close and press E, then follow the restraint prompts. If they recover, subdue them again."), TEXT("Now move in and restrain them. Stay calm and follow the prompts."));
	Add(EPPTutorialStage::Escort, TEXT("Escort the captive to the camp flare"), TEXT("Stay close so they keep following. An escape means recovering the same poacher."), TEXT("Restraint isn't the end. Bring them to the arrest flare and keep them close."));
	Add(EPPTutorialStage::Debrief, TEXT("First arrest completed"), TEXT("Stay at the post for the debrief."), TEXT("Arrest confirmed. You brought them in alive and completed the handover. That's a patrol finished properly."));
	Add(EPPTutorialStage::Radio, TEXT("Listen to the urgent radio call"), TEXT("Incoming transmission..."), TEXT(""));
	FPPTutorialLine Pause;
	Pause.Seconds = 2.0f;
	Lessons[EPPTutorialStage::Radio].Lines.Add(Pause);
	FPPTutorialLine Radio;
	Radio.Speaker = FText::FromString(TEXT("Control"));
	Radio.Text = FText::FromString(TEXT("All units: another incident deeper in the reserve. The patrol sent to investigate is no longer responding."));
	Radio.Seconds = 8.0f;
	Lessons[EPPTutorialStage::Radio].Lines.Add(Radio);
	Radio.Speaker = FText::FromString(TEXT("Senior Ranger"));
	Radio.Text = FText::FromString(TEXT("Control, send their last known position. We're heading out."));
	Radio.Seconds = 6.0f;
	Lessons[EPPTutorialStage::Radio].Lines.Add(Radio);
}

bool APPTutorialDirector::ValidateSetup(FString& Error) const
{
	TArray<FString> Missing;
	if (!IsValid(TutorialStart)) Missing.Add(TEXT("TutorialStart"));
	if (!IsValid(BriefingPoint)) Missing.Add(TEXT("BriefingPoint"));
	if (!IsValid(PatrolDestination)) Missing.Add(TEXT("PatrolDestination"));
	if (!IsValid(ResupplyPoint)) Missing.Add(TEXT("ResupplyPoint"));
	if (!IsValid(PracticeTarget)) Missing.Add(TEXT("PracticeTarget"));
	if (!IsValid(TutorialPoacher)) Missing.Add(TEXT("TutorialPoacher"));
	if (!IsValid(ArrestZone)) Missing.Add(TEXT("ArrestZone"));
	if (TutorialPoacher && (!TutorialPoacher->bTutorialEncounter || !TutorialPoacher->bStartEncounterInactive)) Missing.Add(TEXT("poacher must be Tutorial Encounter and Start Encounter Inactive"));
	for (uint8 Value = uint8(EPPTutorialStage::Arrival); Value <= uint8(EPPTutorialStage::Radio); ++Value)
		if (!Lessons.Contains(EPPTutorialStage(Value)) || Lessons[EPPTutorialStage(Value)].Objective.IsEmpty()) Missing.Add(FString::Printf(TEXT("objective for stage %d"), Value));
	int32 Directors = 0;
	for (TActorIterator<APPTutorialDirector> It(GetWorld()); It; ++It) ++Directors;
	if (Directors != 1) Missing.Add(TEXT("level must contain exactly one tutorial director"));
	Error = FString::Join(Missing, TEXT(", "));
	return Missing.IsEmpty();
}

#if WITH_EDITOR
void APPTutorialDirector::CheckForErrors()
{
	Super::CheckForErrors();
	FString Error;
	if (!ValidateSetup(Error)) FMessageLog("MapCheck").Error()->AddToken(FUObjectToken::Create(this))->AddToken(FTextToken::Create(FText::FromString(TEXT("Tutorial setup: ") + Error)));
}
#endif

void APPTutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	if (ResupplyPoint) ResupplyPoint->OnResupplyCompleted.AddUniqueDynamic(this, &APPTutorialDirector::OnResupplied);
	if (TutorialPoacher) TutorialPoacher->OnPoacherStateChanged.AddUniqueDynamic(this, &APPTutorialDirector::OnPoacherState);
	FString Error;
	if (!ValidateSetup(Error)) UE_LOG(LogTemp, Error, TEXT("Tutorial setup invalid on %s: %s"), *GetName(), *Error);
}

void APPTutorialDirector::StartTutorial()
{
	if (PatrolMode != EPPPatrolMode::Tutorial || Stage != EPPTutorialStage::WaitingForMenu) return;
	FString Error;
	if (!ValidateSetup(Error)) { FailTutorial(FText::FromString(TEXT("Tutorial setup is incomplete: ") + Error)); return; }
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn) { FailTutorial(FText::FromString(TEXT("The ranger could not be found. Please retry."))); return; }
	Pawn->SetActorLocationAndRotation(TutorialStart->GetActorLocation(), TutorialStart->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (auto* PC = Pawn->GetController()) PC->SetControlRotation(TutorialStart->GetActorRotation());
	TutorialPoacher->SetEncounterActive(false);
	EnterStage(EPPTutorialStage::Arrival);
}

void APPTutorialDirector::EnterStage(EPPTutorialStage Next)
{
	if (Stage == Next || Stage == EPPTutorialStage::Complete || Stage == EPPTutorialStage::Failed) return;
	ClearDialogue();
	FeedbackRemaining = Next > Stage && Stage > EPPTutorialStage::Briefing ? 2.5f : 0;
	Stage = Next;
	StageElapsed = 0;
	if (const auto* Lesson = Lessons.Find(Stage)) Dialogue = Lesson->Lines;
	const FText Community = Stage == EPPTutorialStage::Briefing ? ReviewedCommunityOpening : Stage == EPPTutorialStage::Debrief ? ReviewedCommunityDebrief : FText::GetEmpty();
	if (!Community.IsEmpty())
	{
		FPPTutorialLine Line;
		Line.Speaker = FText::FromString(TEXT("Senior Ranger")); Line.Text = Community; Line.Seconds = 8;
		Dialogue.Add(Line);
	}
	AdvanceDialogue();
	if (Stage == EPPTutorialStage::Subdual && TutorialPoacher && !TutorialPoacher->IsEncounterActive()) TutorialPoacher->SetEncounterActive(true);
	if (Stage == EPPTutorialStage::Complete)
		if (auto* PC = Cast<AKruger_ConclusionPlayerController>(UGameplayStatics::GetPlayerController(this, 0))) PC->ShowTutorialResult(true, FText::FromString(TEXT("First Day on Patrol complete. Another patrol is missing. Your next assignment is waiting.")));
}

void APPTutorialDirector::ClearDialogue()
{
	if (VoiceComponent) { VoiceComponent->Stop(); VoiceComponent->DestroyComponent(); }
	VoiceComponent = nullptr; Dialogue.Reset(); LineIndex = -1; LineRemaining = 0;
	Subtitle = FText::GetEmpty(); Speaker = FText::GetEmpty();
}

void APPTutorialDirector::AdvanceDialogue()
{
	if (VoiceComponent) { VoiceComponent->Stop(); VoiceComponent->DestroyComponent(); }
	VoiceComponent = nullptr;
	++LineIndex;
	if (!Dialogue.IsValidIndex(LineIndex)) { Subtitle = FText::GetEmpty(); Speaker = FText::GetEmpty(); return; }
	const auto& Line = Dialogue[LineIndex];
	Subtitle = Line.Text; Speaker = Line.Speaker;
	LineRemaining = FMath::Max(Line.Seconds, Line.Text.ToString().Len() / 15.0f);
	if (Line.Voice)
	{
		LineRemaining = FMath::Max(LineRemaining, Line.Voice->GetDuration());
		VoiceComponent = UGameplayStatics::SpawnSound2D(this, Line.Voice, 1, 1, 0, nullptr, false, false);
		if (VoiceComponent) VoiceComponent->bIsUISound = false;
	}
}

void APPTutorialDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bPaused = UGameplayStatics::IsGamePaused(this);
	if (VoiceComponent) VoiceComponent->SetPaused(bPaused);
	if (bPaused) return;
	if (!IsActive() || PatrolMode != EPPPatrolMode::Tutorial) return;
	StageElapsed += DeltaSeconds;
	FeedbackRemaining = FMath::Max(0.0f, FeedbackRemaining - DeltaSeconds);
	if (Stage <= EPPTutorialStage::Escort && (!IsValid(BriefingPoint) || !IsValid(ResupplyPoint) || !IsValid(PracticeTarget) || !IsValid(PatrolDestination) || !IsValid(ArrestZone) || !IsValid(TutorialPoacher) || TutorialPoacher->WasProcessedAsPermanentlyEscaped()))
	{ FailTutorial(FText::FromString(TEXT("The training encounter is no longer available. Please retry."))); return; }
	if (Dialogue.IsValidIndex(LineIndex))
	{
		LineRemaining -= DeltaSeconds;
		if (LineRemaining <= 0) AdvanceDialogue();
	}
	const bool bDialogueDone = !Dialogue.IsValidIndex(LineIndex);
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Stage == EPPTutorialStage::Arrival && Pawn && BriefingPoint && FVector::Dist(Pawn->GetActorLocation(), BriefingPoint->GetActorLocation()) <= ArrivalRadius) EnterStage(EPPTutorialStage::Briefing);
	else if (Stage == EPPTutorialStage::Briefing && bDialogueDone) EnterStage(EPPTutorialStage::Supply);
	else if (Stage == EPPTutorialStage::Patrol && Pawn && PatrolDestination && FVector::Dist(Pawn->GetActorLocation(), PatrolDestination->GetActorLocation()) <= EncounterRadius) EnterStage(EPPTutorialStage::Subdual);
	else if (Stage == EPPTutorialStage::Debrief && bDialogueDone) EnterStage(EPPTutorialStage::Radio);
	else if (Stage == EPPTutorialStage::Radio && bDialogueDone) EnterStage(EPPTutorialStage::Complete);
	ReconcilePoacher();
}

void APPTutorialDirector::OnResupplied(APPRestPoint* Point, ARangerCharacter* Ranger)
{
	if (Stage == EPPTutorialStage::Supply && Point == ResupplyPoint && Ranger == UGameplayStatics::GetPlayerPawn(this, 0)) EnterStage(EPPTutorialStage::Practice);
}

void APPTutorialDirector::NotifyPracticeHit(APPTutorialTarget* Target, AController* EventInstigator)
{
	if (Stage == EPPTutorialStage::Practice && Target == PracticeTarget && EventInstigator == UGameplayStatics::GetPlayerController(this, 0)) EnterStage(EPPTutorialStage::Patrol);
}

void APPTutorialDirector::OnPoacherState(APPPoacherCharacter* Poacher, EPPPoacherState State)
{
	if (Poacher == TutorialPoacher) ReconcilePoacher();
}

void APPTutorialDirector::ReconcilePoacher()
{
	if (TutorialPoacher && TutorialPoacher->GetPoacherState() == EPPPoacherState::Arrested) return;
	if (!TutorialPoacher || Stage < EPPTutorialStage::Subdual || Stage > EPPTutorialStage::Escort) return;
	if (TutorialPoacher->IsCaptured()) EnterStage(EPPTutorialStage::Escort);
	else if (TutorialPoacher->IsSubdued()) EnterStage(EPPTutorialStage::Restraint);
	else if (Stage != EPPTutorialStage::Subdual)
	{
		EnterStage(EPPTutorialStage::Subdual);
		ClearDialogue();
		FPPTutorialLine Line;
		Line.Speaker = FText::FromString(TEXT("Senior Ranger"));
		Line.Text = FText::FromString(TEXT("They're loose. Stay calm. Get control and try again."));
		Dialogue.Add(Line); AdvanceDialogue();
	}
}

bool APPTutorialDirector::AllowsArrest(const APPPoacherCharacter* Poacher, const APPArrestZone* Zone) const
{
	return Stage == EPPTutorialStage::Escort && Poacher == TutorialPoacher && Zone == ArrestZone;
}

void APPTutorialDirector::NotifyArrest(APPPoacherCharacter* Poacher, APPArrestZone* Zone)
{
	if (AllowsArrest(Poacher, Zone) && Poacher->GetPoacherState() == EPPPoacherState::Arrested) EnterStage(EPPTutorialStage::Debrief);
}

void APPTutorialDirector::FailTutorial(const FText& Reason)
{
	if (Stage == EPPTutorialStage::Failed || Stage == EPPTutorialStage::Complete) return;
	UE_LOG(LogTemp, Display, TEXT("Tutorial retry: %s"), *Reason.ToString());
	ClearDialogue(); Stage = EPPTutorialStage::Failed;
	if (TutorialPoacher) TutorialPoacher->SetEncounterActive(false);
	if (auto* PC = Cast<AKruger_ConclusionPlayerController>(UGameplayStatics::GetPlayerController(this, 0))) PC->ShowTutorialResult(false, Reason);
}

FText APPTutorialDirector::GetObjective() const
{
	const auto* Lesson = Lessons.Find(Stage);
	return Lesson ? Lesson->Objective : FText::GetEmpty();
}

FText APPTutorialDirector::GetHint() const
{
	const auto* Lesson = Lessons.Find(Stage);
	if (!Lesson) return FText::GetEmpty();
	return StageElapsed >= ReminderSeconds ? FText::Format(NSLOCTEXT("Tutorial", "Reminder", "Take your time. {0}"), Lesson->Hint) : Lesson->Hint;
}

AActor* APPTutorialDirector::GetDestination() const
{
	switch (Stage)
	{
	case EPPTutorialStage::Arrival: case EPPTutorialStage::Briefing: return BriefingPoint;
	case EPPTutorialStage::Supply: return ResupplyPoint;
	case EPPTutorialStage::Practice: return PracticeTarget;
	case EPPTutorialStage::Patrol: return PatrolDestination;
	case EPPTutorialStage::Subdual: case EPPTutorialStage::Restraint: return TutorialPoacher;
	case EPPTutorialStage::Escort: return ArrestZone;
	default: return nullptr;
	}
}

void APPTutorialDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearDialogue();
	if (ResupplyPoint) ResupplyPoint->OnResupplyCompleted.RemoveDynamic(this, &APPTutorialDirector::OnResupplied);
	if (TutorialPoacher) TutorialPoacher->OnPoacherStateChanged.RemoveDynamic(this, &APPTutorialDirector::OnPoacherState);
	Super::EndPlay(Reason);
}
