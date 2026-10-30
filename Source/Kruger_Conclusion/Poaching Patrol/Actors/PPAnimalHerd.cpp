#include "Actors/PPAnimalHerd.h"
#include "Actors/PPSpawnDirector.h"
#include "Characters/PPAnimalCharacter.h"
#include "Data/PPHealthComponent.h"
#include "EnvironmentLevelSubsystem.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

APPAnimalHerd::APPAnimalHerd()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void APPAnimalHerd::Initialise(APPSpawnDirector* Director, const FVector& InitialCenter,
	const FPPAnimalSpawnProfile& Settings, const TArray<APPAnimalCharacter*>& Animals, int32 Seed)
{
	SpawnDirector = Director; Profile = Settings; Center = TerritoryCenter = InitialCenter;
	SetActorLocation(Center);
	Random.Initialize(Seed);
	for (APPAnimalCharacter* Animal : Animals)
	{
		if (!IsValid(Animal) || Animal->IsPredator()) continue;
		Members.AddUnique(Animal);
		FVector Offset = Animal->GetActorLocation() - Center; Offset.Z = 0;
		Offsets.Add(Animal, Offset);
		Goals.Add(Animal, Animal->GetActorLocation());
		Animal->SetHerd(this);
	}
	GetWorldTimerManager().SetTimer(UpdateTimer, this, &APPAnimalHerd::UpdateHerd,
		FMath::Max(0.5f, Profile.UpdateSeconds), true, Random.FRandRange(0.5f, 1.0f));
}

TArray<APPAnimalCharacter*> APPAnimalHerd::GetLivingMembers() const
{
	TArray<APPAnimalCharacter*> Living;
	for (const TWeakObjectPtr<APPAnimalCharacter>& Member : Members)
	{
		APPAnimalCharacter* Animal = Member.Get();
		if (Animal && Animal->GetHealthComponent() && !Animal->GetHealthComponent()->IsDead()) Living.Add(Animal);
	}
	return Living;
}

void APPAnimalHerd::RemoveMember(APPAnimalCharacter* Animal)
{
	Members.Remove(Animal); Offsets.Remove(Animal); Goals.Remove(Animal); AlertTimes.Remove(Animal);
	if (Animal && Animal->GetHerd() == this) Animal->SetHerd(nullptr);
	if (Members.IsEmpty() && !GetWorld()->bIsTearingDown) Destroy();
}

void APPAnimalHerd::ReportThreat(APPAnimalCharacter* Observer, const FVector& LastKnownLocation)
{
	if (!Observer || !Members.Contains(Observer)) return;
	const float Now = GetWorld()->GetTimeSeconds();
	LastThreatTime = Now; bNeedsSafeRegroup = true;
	if (const float* Previous = AlertTimes.Find(Observer); Previous && Now - *Previous < 2) return;
	AlertTimes.Add(Observer, Now);
	for (APPAnimalCharacter* Animal : GetLivingMembers())
	{
		if (Animal != Observer && FVector::DistSquared2D(Observer->GetActorLocation(), Animal->GetActorLocation()) <= FMath::Square(Profile.AlertDistance))
			Animal->ReceiveHerdAlert(LastKnownLocation, Now + 3.0f);
	}
}

bool APPAnimalHerd::GetRoamGoal(APPAnimalCharacter* Animal, FVector& OutGoal) const
{
	if (GetWorld()->GetTimeSeconds() - LastThreatTime < Profile.CalmSeconds) return false;
	if (const FVector* Goal = Goals.Find(Animal)) { OutGoal = *Goal; return true; }
	return false;
}

bool APPAnimalHerd::ShouldRegroup(const APPAnimalCharacter* Animal) const
{
	return FVector::DistSquared2D(Animal->GetActorLocation(), Center) > FMath::Square(Profile.RegroupDistance);
}

bool APPAnimalHerd::FindSafeGoal(APPAnimalCharacter* Animal, const FVector& Desired, FVector& OutGoal) const
{
	FName Reason;
	return SpawnDirector.IsValid() && SpawnDirector->ProjectCreatureLocation(Animal, Desired, OutGoal, Reason)
		&& SpawnDirector->HasCreatureRoute(Animal, Animal->GetActorLocation(), OutGoal, Profile.RoamRadius * 4);
}

bool APPAnimalHerd::ChooseDestination(const FVector& DesiredCenter)
{
	APPSpawnDirector* Director = SpawnDirector.Get();
	const TArray<APPAnimalCharacter*> Living = GetLivingMembers();
	if (!Director || Living.IsEmpty()) return false;
	TMap<TWeakObjectPtr<APPAnimalCharacter>, FVector> Planned;
	FVector NewCenter; FName Reason;
	if (!Director->ProjectCreatureLocation(Living[0], DesiredCenter, NewCenter, Reason)) return false;
	for (APPAnimalCharacter* Animal : Living)
	{
		const FVector* Offset = Offsets.Find(Animal);
		FVector Goal;
		if (!Offset || !Director->ProjectCreatureLocation(Animal, NewCenter + *Offset, Goal, Reason)
			|| !Director->HasCreatureRoute(Animal, Animal->GetActorLocation(), Goal, Profile.RoamRadius * 4)
			|| !Director->HasCreatureRoute(Animal, NewCenter, Goal, Profile.SpreadRadius * 4)) return false;
		for (const auto& Existing : Planned)
		{
			const float Spacing = FMath::Max(Profile.MemberSpacing,
				APPSpawnDirector::GetBodyRadius(Animal) + APPSpawnDirector::GetBodyRadius(Existing.Key.Get()) + 100);
			if (FVector::DistSquared2D(Goal, Existing.Value) < FMath::Square(Spacing)) return false;
		}
		Planned.Add(Animal, Goal);
	}
	Center = NewCenter; Goals = MoveTemp(Planned);
	SetActorLocation(Center);
	return true;
}

bool APPAnimalHerd::GetSeparationGoal(APPAnimalCharacter* Animal, FVector& OutGoal, bool& bCrowded) const
{
	bCrowded = false;
	FVector Away = FVector::ZeroVector;
	float Push = 0;
	const auto Living = GetLivingMembers();
	for (APPAnimalCharacter* Other : Living)
	{
		if (Other == Animal) continue;
		FVector Difference = Animal->GetActorLocation() - Other->GetActorLocation(); Difference.Z = 0;
		const float Distance = Difference.Size();
		const float Spacing = FMath::Max(Profile.MemberSpacing, APPSpawnDirector::GetBodyRadius(Animal) + APPSpawnDirector::GetBodyRadius(Other) + 100);
		if (Distance >= Spacing - 50) continue;
		bCrowded = true;
		if (!Difference.Normalize()) Difference = Animal->GetName() < Other->GetName() ? FVector::ForwardVector : -FVector::ForwardVector;
		Away += Difference * (Spacing - Distance + 100);
		Push = FMath::Max(Push, Spacing - Distance + 150);
	}
	if (!bCrowded) return false;
	if (!Away.Normalize()) Away = Animal->GetActorForwardVector();
	for (float Angle : {0.0f, 45.0f, -45.0f, 90.0f, -90.0f})
	{
		if (!FindSafeGoal(Animal, Animal->GetActorLocation() + Away.RotateAngleAxis(Angle, FVector::UpVector) * FMath::Clamp(Push, 200.0f, 1500.0f), OutGoal)) continue;
		bool bFits = true;
		for (APPAnimalCharacter* Other : Living)
		{
			if (Other == Animal) continue;
			const float Spacing = FMath::Max(Profile.MemberSpacing, APPSpawnDirector::GetBodyRadius(Animal) + APPSpawnDirector::GetBodyRadius(Other) + 100);
			if (FVector::DistSquared2D(OutGoal, Other->GetActorLocation()) < FMath::Square(Spacing)) bFits = false;
		}
		if (bFits) return true;
	}
	return false;
}

void APPAnimalHerd::UpdateHerd()
{
	if (const auto* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>(); Rules
		&& (Rules->IsTutorialMode() || Rules->IsPatrolSetupPending() || Rules->HasRoundEnded())) return;
	const auto Living = GetLivingMembers();
	for (int32 I = Members.Num() - 1; I >= 0; --I)
		if (!Living.Contains(Members[I].Get())) { Offsets.Remove(Members[I]); Goals.Remove(Members[I]); AlertTimes.Remove(Members[I]); Members.RemoveAt(I); }
	if (Living.IsEmpty()) { Destroy(); return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastThreatTime < Profile.CalmSeconds) return;
	if (bNeedsSafeRegroup)
	{
		FVector Average = FVector::ZeroVector;
		for (APPAnimalCharacter* Animal : Living) Average += Animal->GetActorLocation();
		Average /= Living.Num();
		if (!ChooseDestination(Average)) return;
		TerritoryCenter = Center; bNeedsSafeRegroup = false;
		NextDestinationTime = Now + 15;
	}
	if (Now < NextDestinationTime) return;
	bool bArrived = true;
	for (APPAnimalCharacter* Animal : Living)
		if (const FVector* Goal = Goals.Find(Animal); Goal && FVector::DistSquared2D(Animal->GetActorLocation(), *Goal) > FMath::Square(300)) bArrived = false;
	if (!bArrived && Now - NextDestinationTime < 25) return;
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const float Angle = Random.FRandRange(0, 2 * PI);
		const float Radius = FMath::Sqrt(Random.FRand()) * Profile.RoamRadius;
		if (ChooseDestination(TerritoryCenter + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0)))
		{ NextDestinationTime = Now + Random.FRandRange(10, 18); return; }
	}
	NextDestinationTime = Now + 3;
}

void APPAnimalHerd::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);
	for (const auto& Member : Members)
		if (APPAnimalCharacter* Animal = Member.Get(); Animal && Animal->GetHerd() == this) Animal->SetHerd(nullptr);
	Super::EndPlay(Reason);
}
