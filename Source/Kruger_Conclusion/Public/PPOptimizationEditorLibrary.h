#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PPOptimizationEditorLibrary.generated.h"

/** Narrow editor-only bridge used by the repeatable optimisation scripts. */
UCLASS()
class KRUGER_CONCLUSION_API UPPOptimizationEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Optimisation")
	static bool ReplaceFoliageTypeInEditor(const FString& OldAssetPath, const FString& NewAssetPath);
#endif
};
