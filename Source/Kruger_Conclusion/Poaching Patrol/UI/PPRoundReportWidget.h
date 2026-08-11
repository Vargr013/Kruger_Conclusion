#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameTypes.h"
#include "PPRoundReportWidget.generated.h"

class UTextBlock;

UCLASS()
class KRUGER_CONCLUSION_API UPPRoundReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Round Report")
	void SetRoundResult(const FPPRoundResult& InResult);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleReplayClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

private:
	void BuildDefaultWidgetTree();
	void RefreshText();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MastheadText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeadlineText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OutcomeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PoacherText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnimalText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FooterText;

	UPROPERTY(Transient)
	FPPRoundResult RoundResult;
};
