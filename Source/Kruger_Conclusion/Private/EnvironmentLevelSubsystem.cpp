#include "EnvironmentLevelSubsystem.h"

#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "EngineUtils.h"

void UEnvironmentLevelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ResetRoundState();
	ScanPlacedActors();
	BroadcastStateChanged();
}

void UEnvironmentLevelSubsystem::ResetRoundState()
{
	RegisteredPoachers.Reset();
	RegisteredAnimals.Reset();
	ArrestedPoachers.Reset();
	PermanentlyEscapedPoachers.Reset();
	PoachedAnimals.Reset();
	bRoundEnded = false;
	FinalRoundResult = FPPRoundResult();
}

void UEnvironmentLevelSubsystem::ScanPlacedActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<APPPoacherCharacter> It(World); It; ++It)
	{
		RegisterPoacher(*It);
	}

	for (TActorIterator<APPAnimalCharacter> It(World); It; ++It)
	{
		RegisterAnimal(*It);
	}
}

void UEnvironmentLevelSubsystem::RegisterPoacher(APPPoacherCharacter* Poacher)
{
	if (!IsValid(Poacher) || bRoundEnded || RegisteredPoachers.Contains(Poacher))
	{
		return;
	}
	RegisteredPoachers.Add(Poacher);
	BroadcastStateChanged();
}

void UEnvironmentLevelSubsystem::RegisterAnimal(APPAnimalCharacter* Animal)
{
	if (!IsValid(Animal) || bRoundEnded || RegisteredAnimals.Contains(Animal))
	{
		return;
	}
	RegisteredAnimals.Add(Animal);
	BroadcastStateChanged();
}

void UEnvironmentLevelSubsystem::ReportPoacherArrested(APPPoacherCharacter* Poacher)
{
	if (!IsValid(Poacher) || bRoundEnded || ArrestedPoachers.Contains(Poacher) || PermanentlyEscapedPoachers.Contains(Poacher))
	{
		return;
	}
	RegisterPoacher(Poacher);
	ArrestedPoachers.Add(Poacher);
	BroadcastStateChanged();
	CheckWinCondition();
}

void UEnvironmentLevelSubsystem::ReportPoacherPermanentlyEscaped(APPPoacherCharacter* Poacher)
{
	if (!IsValid(Poacher) || bRoundEnded || PermanentlyEscapedPoachers.Contains(Poacher) || ArrestedPoachers.Contains(Poacher))
	{
		return;
	}
	RegisterPoacher(Poacher);
	PermanentlyEscapedPoachers.Add(Poacher);
	BroadcastStateChanged();
	CheckWinCondition();
}

void UEnvironmentLevelSubsystem::ReportAnimalPoached(APPAnimalCharacter* Animal)
{
	if (!IsValid(Animal) || bRoundEnded || PoachedAnimals.Contains(Animal))
	{
		return;
	}
	RegisterAnimal(Animal);
	PoachedAnimals.Add(Animal);
	BroadcastStateChanged();
}

void UEnvironmentLevelSubsystem::OnPoacherCaptured()
{
	for (const TWeakObjectPtr<APPPoacherCharacter>& PoacherPtr : RegisteredPoachers)
	{
		APPPoacherCharacter* Poacher = PoacherPtr.Get();
		if (IsValid(Poacher) && Poacher->GetPoacherState() == EPPPoacherState::Arrested && !ArrestedPoachers.Contains(Poacher))
		{
			ReportPoacherArrested(Poacher);
			return;
		}
	}
}

void UEnvironmentLevelSubsystem::OnPoacherEscaped()
{
	for (const TWeakObjectPtr<APPPoacherCharacter>& PoacherPtr : RegisteredPoachers)
	{
		APPPoacherCharacter* Poacher = PoacherPtr.Get();
		if (IsValid(Poacher) && Poacher->WasProcessedAsPermanentlyEscaped() && !PermanentlyEscapedPoachers.Contains(Poacher))
		{
			ReportPoacherPermanentlyEscaped(Poacher);
			return;
		}
	}
}

void UEnvironmentLevelSubsystem::OnAnimalPoached()
{
	for (const TWeakObjectPtr<APPAnimalCharacter>& AnimalPtr : RegisteredAnimals)
	{
		APPAnimalCharacter* Animal = AnimalPtr.Get();
		const UPPHealthComponent* Health = IsValid(Animal) ? Animal->FindComponentByClass<UPPHealthComponent>() : nullptr;
		if (Health && Health->IsDead() && !PoachedAnimals.Contains(Animal))
		{
			ReportAnimalPoached(Animal);
			return;
		}
	}
}

int32 UEnvironmentLevelSubsystem::CalculateRequiredArrests(int32 TotalPoachers, float Threshold)
{
	return TotalPoachers <= 0 ? 0 : FMath::CeilToInt(static_cast<float>(TotalPoachers) * FMath::Clamp(Threshold, 0.0f, 1.0f));
}

FPPRoundSnapshot UEnvironmentLevelSubsystem::GetRoundSnapshot() const
{
	FPPRoundSnapshot Snapshot;
	Snapshot.TotalPoachers = RegisteredPoachers.Num();
	Snapshot.PoachersArrested = ArrestedPoachers.Num();
	Snapshot.PoachersPermanentlyEscaped = PermanentlyEscapedPoachers.Num();
	Snapshot.ActivePoachers = FMath::Max(0, Snapshot.TotalPoachers - Snapshot.PoachersArrested - Snapshot.PoachersPermanentlyEscaped);
	Snapshot.TotalAnimals = RegisteredAnimals.Num();
	Snapshot.AnimalsPoached = FMath::Min(Snapshot.TotalAnimals, PoachedAnimals.Num());
	Snapshot.AnimalsAlive = FMath::Max(0, Snapshot.TotalAnimals - Snapshot.AnimalsPoached);
	Snapshot.RequiredArrests = CalculateRequiredArrests(Snapshot.TotalPoachers, WinThresholdPercentage);
	Snapshot.CaptureRate = Snapshot.TotalPoachers > 0
		? static_cast<float>(Snapshot.PoachersArrested) / static_cast<float>(Snapshot.TotalPoachers)
		: 0.0f;
	Snapshot.bQuotaMet = Snapshot.TotalPoachers > 0 && Snapshot.PoachersArrested >= Snapshot.RequiredArrests;
	return Snapshot;
}

TArray<FPPObjectiveState> UEnvironmentLevelSubsystem::GetObjectivesForPlayer(AActor* Captor) const
{
	const FPPRoundSnapshot Snapshot = GetRoundSnapshot();
	TArray<FPPObjectiveState> Objectives;

	FPPObjectiveState Primary;
	Primary.Identifier = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0 ? TEXT("ResolveRemaining") : TEXT("ArrestQuota");
	Primary.Kind = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0 ? EPPObjectiveKind::ResolveRemaining : EPPObjectiveKind::ArrestQuota;
	Primary.Title = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0
		? FText::FromString(TEXT("Resolve the remaining poachers"))
		: FText::Format(NSLOCTEXT("PoachingPatrol", "ArrestQuota", "Arrest at least {0} of {1} poachers"), Snapshot.RequiredArrests, Snapshot.TotalPoachers);
	Primary.Detail = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0
		? FText::Format(NSLOCTEXT("PoachingPatrol", "RemainingProgress", "Remaining: {0}"), Snapshot.ActivePoachers)
		: FText::Format(NSLOCTEXT("PoachingPatrol", "ArrestProgress", "Arrested: {0} / {1}"), Snapshot.PoachersArrested, Snapshot.RequiredArrests);
	Primary.CurrentValue = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0 ? 0 : Snapshot.PoachersArrested;
	Primary.TargetValue = Snapshot.bQuotaMet && Snapshot.ActivePoachers > 0 ? Snapshot.ActivePoachers : Snapshot.RequiredArrests;
	Primary.ProgressState = Snapshot.bQuotaMet && Snapshot.ActivePoachers == 0 ? EPPObjectiveProgressState::Completed : EPPObjectiveProgressState::Active;
	Objectives.Add(Primary);

	FPPObjectiveState Conservation;
	Conservation.Identifier = TEXT("ConservationStatus");
	Conservation.Kind = EPPObjectiveKind::ConservationStatus;
	Conservation.Title = FText::Format(NSLOCTEXT("PoachingPatrol", "AnimalsSafe", "Animals safe: {0} of {1}"), Snapshot.AnimalsAlive, Snapshot.TotalAnimals);
	Conservation.Detail = FText::Format(NSLOCTEXT("PoachingPatrol", "AnimalsPoached", "Animals poached: {0}"), Snapshot.AnimalsPoached);
	Conservation.CurrentValue = Snapshot.AnimalsAlive;
	Conservation.TargetValue = Snapshot.TotalAnimals;
	Conservation.bRequired = false;
	Objectives.Add(Conservation);

	FPPEscortStatus EscortStatus;
	if (GetMostUrgentEscortStatus(Captor, EscortStatus))
	{
		FPPObjectiveState Escort;
		Escort.Identifier = TEXT("EscortToArrestZone");
		Escort.Kind = EPPObjectiveKind::EscortToArrestZone;
		Escort.Title = FText::FromString(TEXT("Escort captured poacher to the arrest zone"));
		Escort.Detail = FText::Format(NSLOCTEXT("PoachingPatrol", "EscortSafeRange", "Stay within {0} m"), FText::AsNumber(FMath::RoundToInt(EscortStatus.SafeRange / 100.0f)));
		Escort.CurrentValue = FMath::RoundToInt(EscortStatus.EscapeProgress * 10.0f);
		Escort.TargetValue = FMath::RoundToInt(EscortStatus.EscapeGraceTime * 10.0f);
		Objectives.Add(Escort);
	}

	return Objectives;
}

bool UEnvironmentLevelSubsystem::GetMostUrgentEscortStatus(AActor* Captor, FPPEscortStatus& OutStatus) const
{
	OutStatus = FPPEscortStatus();
	if (!IsValid(Captor))
	{
		return false;
	}

	float BestProgress = -1.0f;
	float BestSecondsRemaining = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<APPPoacherCharacter>& PoacherPtr : RegisteredPoachers)
	{
		APPPoacherCharacter* Poacher = PoacherPtr.Get();
		if (!IsValid(Poacher) || !Poacher->IsCaptured() || Poacher->GetCaptorActor() != Captor || Poacher->GetPoacherState() == EPPPoacherState::Arrested)
		{
			continue;
		}

		OutStatus.ActiveEscortCount++;
		const float GraceTime = FMath::Max(0.0f, Poacher->GetEscapeGraceTime());
		const float Progress = FMath::Clamp(Poacher->GetEscapeProgress(), 0.0f, GraceTime);
		const bool bUnderPressure = Poacher->IsEscortTooFarFromCaptor();
		const float NormalizedProgress = GraceTime > KINDA_SMALL_NUMBER ? Progress / GraceTime : (bUnderPressure ? 1.0f : 0.0f);
		const float SecondsRemaining = FMath::Max(0.0f, GraceTime - Progress);

		if (!OutStatus.SelectedPoacher || NormalizedProgress > BestProgress || (FMath::IsNearlyEqual(NormalizedProgress, BestProgress) && SecondsRemaining < BestSecondsRemaining))
		{
			OutStatus.SelectedPoacher = Poacher;
			OutStatus.DistanceToCaptor = FVector::Distance(Poacher->GetActorLocation(), Captor->GetActorLocation());
			OutStatus.SafeRange = Poacher->GetEscapeDistance();
			OutStatus.EscapeProgress = Progress;
			OutStatus.EscapeGraceTime = GraceTime;
			OutStatus.SecondsRemaining = SecondsRemaining;
			OutStatus.bUnderEscapePressure = bUnderPressure;
			BestProgress = NormalizedProgress;
			BestSecondsRemaining = SecondsRemaining;
		}
	}

	return OutStatus.SelectedPoacher != nullptr;
}

void UEnvironmentLevelSubsystem::BroadcastStateChanged()
{
	OnRoundStateChanged.Broadcast(GetRoundSnapshot());
}

void UEnvironmentLevelSubsystem::CheckWinCondition()
{
	if (bRoundEnded)
	{
		return;
	}

	const FPPRoundSnapshot Snapshot = GetRoundSnapshot();
	if (Snapshot.TotalPoachers <= 0 || Snapshot.ActivePoachers > 0)
	{
		return;
	}

	bRoundEnded = true;
	FinalRoundResult.Snapshot = Snapshot;
	FinalRoundResult.Outcome = Snapshot.bQuotaMet ? EPPRoundOutcome::Success : EPPRoundOutcome::Failure;
	OnRoundEnded.Broadcast(FinalRoundResult);

	if (FinalRoundResult.Outcome == EPPRoundOutcome::Success)
	{
		OnLevelWon.Broadcast();
	}
	else
	{
		OnLevelLost.Broadcast();
	}
}
