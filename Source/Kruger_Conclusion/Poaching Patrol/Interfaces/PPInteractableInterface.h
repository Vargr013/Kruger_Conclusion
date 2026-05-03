#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PPInteractableInterface.generated.h"

UINTERFACE(Blueprintable)
class KRUGER_CONCLUSION_API UPPInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class KRUGER_CONCLUSION_API IPPInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt() const;
};
