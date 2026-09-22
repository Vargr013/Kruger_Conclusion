#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPTutorialWidget.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API UPPTutorialWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ShowResult(bool bSuccess, const FText& Message);
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
private:
	UFUNCTION() void Replay();
	UFUNCTION() void MainMenu();
	UPROPERTY() TObjectPtr<class UBorder> ObjectivePanel;
	UPROPERTY() TObjectPtr<class UBorder> SubtitlePanel;
	UPROPERTY() TObjectPtr<class UBorder> ResultPanel;
	UPROPERTY() TObjectPtr<class UTextBlock> Objective;
	UPROPERTY() TObjectPtr<class UTextBlock> Hint;
	UPROPERTY() TObjectPtr<class UTextBlock> Destination;
	UPROPERTY() TObjectPtr<class UTextBlock> Feedback;
	UPROPERTY() TObjectPtr<class UTextBlock> Subtitle;
	UPROPERTY() TObjectPtr<class UTextBlock> Speaker;
	UPROPERTY() TObjectPtr<class UTextBlock> ResultTitle;
	UPROPERTY() TObjectPtr<class UTextBlock> ResultText;
	UPROPERTY() TObjectPtr<class UTextBlock> ReplayText;
	bool bShowingResult = false;
};
