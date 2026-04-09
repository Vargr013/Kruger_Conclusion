#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable) // Add Blueprintable here!
class UInteractInterface : public UInterface
{
    GENERATED_BODY()
};

class KRUGER_CONCLUSION_API IInteractInterface
{
    GENERATED_BODY()

public:
    /** This is the 'Adept' way to define an interaction function */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void Interact(AActor* Interactor);
};