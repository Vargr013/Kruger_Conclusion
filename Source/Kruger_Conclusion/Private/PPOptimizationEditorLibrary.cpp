#include "PPOptimizationEditorLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "FoliageEditUtility.h"
#include "FoliageType.h"

bool UPPOptimizationEditorLibrary::ReplaceFoliageTypeInEditor(const FString& OldAssetPath, const FString& NewAssetPath)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	UFoliageType* OldType = LoadObject<UFoliageType>(nullptr, *OldAssetPath);
	UFoliageType* NewType = LoadObject<UFoliageType>(nullptr, *NewAssetPath);
	if (!World || !OldType || !NewType || OldType == NewType)
	{
		return false;
	}
	FFoliageEditUtility::ReplaceFoliageTypeObject(World, OldType, NewType);
	return true;
}
#endif
