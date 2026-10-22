#include "Data/PPAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

void UPPAudioSubsystem::PlayPredator(AActor* Predator, bool bAttack)
{
	if (!IsValid(Predator) || !Predator->GetWorld() || Predator->GetWorld()->IsPaused()) return;
	UPPAudioSubsystem* Audio = Predator->GetWorld()->GetSubsystem<UPPAudioSubsystem>();
	if (!Audio) return;
	Audio->PredatorVoices.RemoveAll([](const TObjectPtr<UAudioComponent>& Voice) { return !IsValid(Voice) || !Voice->IsPlaying(); });
	if (!bAttack && Audio->PredatorVoices.ContainsByPredicate([Predator](const TObjectPtr<UAudioComponent>& Voice)
		{ return Voice->GetOwner() == Predator && Voice->ComponentHasTag(TEXT("PredatorAttack")); })) return;
	StopPredator(Predator);
	if (Audio->PredatorVoices.Num() >= 3)
	{
		if (!bAttack) return;
		const int32 QuietIndex = Audio->PredatorVoices.IndexOfByPredicate([](const TObjectPtr<UAudioComponent>& Voice)
			{ return !Voice->ComponentHasTag(TEXT("PredatorAttack")); });
		if (QuietIndex == INDEX_NONE) return;
		Audio->PredatorVoices[QuietIndex]->Stop();
		Audio->PredatorVoices[QuietIndex]->DestroyComponent();
		Audio->PredatorVoices.RemoveAt(QuietIndex);
	}
	USoundBase* Sound = Audio->Sounds.FindRef(bAttack ? TEXT("PredatorAttack") : TEXT("PredatorAlert"));
	if (!Sound) return;
	UAudioComponent* Voice = UGameplayStatics::SpawnSoundAttached(Sound, Predator->GetRootComponent(), NAME_None,
		FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true, bAttack ? 0.32f : 0.22f, 1.0f, 0.0f, Audio->PredatorAttenuation);
	if (Voice)
	{
		if (bAttack) Voice->ComponentTags.Add(TEXT("PredatorAttack"));
		Audio->PredatorVoices.Add(Voice);
	}
}

void UPPAudioSubsystem::StopPredator(AActor* Predator)
{
	if (!Predator || !Predator->GetWorld()) return;
	if (UPPAudioSubsystem* Audio = Predator->GetWorld()->GetSubsystem<UPPAudioSubsystem>())
	{
		Audio->PredatorVoices.RemoveAll([Predator](const TObjectPtr<UAudioComponent>& Voice)
		{
			if (!IsValid(Voice)) return true;
			if (Voice->GetOwner() != Predator) return false;
			Voice->Stop();
			Voice->DestroyComponent();
			return true;
		});
	}
}
