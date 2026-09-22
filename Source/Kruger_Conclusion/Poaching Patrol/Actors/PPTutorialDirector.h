#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PPGameTypes.h"
#include "PPTutorialDirector.generated.h"

class APPRestPoint;
class APPPoacherCharacter;
class APPArrestZone;
class APPTutorialTarget;
class ARangerCharacter;
class USoundBase;
class UAudioComponent;

UENUM(BlueprintType)
enum class EPPPatrolMode : uint8 { NormalPatrol, Tutorial };

UENUM(BlueprintType)
enum class EPPTutorialStage : uint8
{
	WaitingForMenu, Arrival, Briefing, Supply, Practice, Patrol, Subdual, Restraint, Escort, Debrief, Radio, Complete, Failed
};

USTRUCT(BlueprintType)
struct FPPTutorialLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> Voice;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float Seconds = 5.0f;
};

USTRUCT(BlueprintType)
struct FPPTutorialLesson
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Objective;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Hint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPPTutorialLine> Lines;
};

/** One placed director configures the level before the opening menu closes. */
UCLASS()
class KRUGER_CONCLUSION_API APPTutorialDirector : public AActor
{
	GENERATED_BODY()
public:
	APPTutorialDirector();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Level Configuration") EPPPatrolMode PatrolMode = EPPPatrolMode::Tutorial;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<AActor> TutorialStart;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<AActor> BriefingPoint;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<AActor> PatrolDestination;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<APPRestPoint> ResupplyPoint;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<APPTutorialTarget> PracticeTarget;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<APPPoacherCharacter> TutorialPoacher;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Tutorial|Assignments") TObjectPtr<APPArrestZone> ArrestZone;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial|Script") TMap<EPPTutorialStage, FPPTutorialLesson> Lessons;
	UPROPERTY(EditAnywhere, Category="Tutorial|Script", meta=(MultiLine=true)) FText ReviewedCommunityOpening;
	UPROPERTY(EditAnywhere, Category="Tutorial|Script", meta=(MultiLine=true)) FText ReviewedCommunityDebrief;
	UPROPERTY(EditAnywhere, Category="Tutorial|Distances", meta=(ClampMin="100")) float ArrivalRadius = 450.0f;
	UPROPERTY(EditAnywhere, Category="Tutorial|Distances", meta=(ClampMin="100")) float EncounterRadius = 1400.0f;
	UPROPERTY(EditAnywhere, Category="Tutorial|Distances", meta=(ClampMin="10")) float ReminderSeconds = 40.0f;

	UFUNCTION(BlueprintPure, Category="Tutorial") EPPTutorialStage GetStage() const { return Stage; }
	UFUNCTION(BlueprintPure, Category="Tutorial") FText GetObjective() const;
	UFUNCTION(BlueprintPure, Category="Tutorial") FText GetHint() const;
	UFUNCTION(BlueprintPure, Category="Tutorial") FText GetSubtitle() const { return Subtitle; }
	UFUNCTION(BlueprintPure, Category="Tutorial") FText GetSpeaker() const { return Speaker; }
	UFUNCTION(BlueprintPure, Category="Tutorial") AActor* GetDestination() const;
	bool IsActive() const { return Stage > EPPTutorialStage::WaitingForMenu && Stage < EPPTutorialStage::Complete; }
	bool HasRecentCompletion() const { return FeedbackRemaining > 0.0f; }
	void StartTutorial();
	void FailTutorial(const FText& Reason);
	void NotifyPracticeHit(APPTutorialTarget* Target, AController* EventInstigator);
	bool AllowsArrest(const APPPoacherCharacter* Poacher, const APPArrestZone* Zone) const;
	void NotifyArrest(APPPoacherCharacter* Poacher, APPArrestZone* Zone);
	bool ValidateSetup(FString& Error) const;
#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION() void OnResupplied(APPRestPoint* Point, ARangerCharacter* Ranger);
	UFUNCTION() void OnPoacherState(APPPoacherCharacter* Poacher, EPPPoacherState State);
	void EnterStage(EPPTutorialStage Next);
	void AdvanceDialogue();
	void ClearDialogue();
	void ReconcilePoacher();
	UPROPERTY(Transient) EPPTutorialStage Stage = EPPTutorialStage::WaitingForMenu;
	UPROPERTY(Transient) TArray<FPPTutorialLine> Dialogue;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> VoiceComponent;
	FText Subtitle;
	FText Speaker;
	float LineRemaining = 0.0f;
	float StageElapsed = 0.0f;
	float FeedbackRemaining = 0.0f;
	int32 LineIndex = -1;
};
