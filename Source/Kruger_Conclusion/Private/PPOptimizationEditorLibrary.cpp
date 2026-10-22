#include "PPOptimizationEditorLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "FoliageEditUtility.h"
#include "FoliageType.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

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

bool UPPOptimizationEditorLibrary::SamplePatrolGround(FVector Position, FHitResult& Ground)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(PatrolCoverGround), false);
	Query.bReturnPhysicalMaterial = true;
	for (TActorIterator<APawn> It(World); It; ++It) Query.AddIgnoredActor(*It);
	const bool bHit = World->LineTraceSingleByChannel(Ground, FVector(Position.X, Position.Y, 10000),
		FVector(Position.X, Position.Y, -10000), ECC_Visibility, Query);
	return bHit && Ground.GetActor() && Ground.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape"));
}
#endif
