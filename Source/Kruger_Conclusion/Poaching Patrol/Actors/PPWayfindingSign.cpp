#include "Actors/PPWayfindingSign.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APPWayfindingSign::APPWayfindingSign()
{
	PrimaryActorTick.bCanEverTick = false;

	SignMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoneSign"));
	SetRootComponent(SignMesh);
	SignMesh->SetCollisionProfileName(TEXT("NoCollision"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
		TEXT("/Game/Poaching_Patrol/Environment/Wayfinding/SM_KrugerWayfinding.SM_KrugerWayfinding"));
	if (Mesh.Succeeded())
	{
		SignMesh->SetStaticMesh(Mesh.Object);
	}

	PanelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PanelText"));
	PanelText->SetupAttachment(SignMesh);
	// FBX import places the recessed board on local +Y in Unreal.
	PanelText->SetRelativeLocation(FVector(0.0f, 48.0f, 140.0f));
	PanelText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PanelText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	PanelText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	PanelText->SetTextRenderColor(FColor(255, 206, 73));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextMaterial(
		TEXT("/Game/Poaching_Patrol/Environment/Wayfinding/M_Kruger_SignText.M_Kruger_SignText"));
	if (TextMaterial.Succeeded())
	{
		PanelText->SetTextMaterial(TextMaterial.Object);
	}
	PanelText->SetCollisionProfileName(TEXT("NoCollision"));
	PanelText->bAlwaysRenderAsText = true;
}

void APPWayfindingSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdatePanelText();
}

void APPWayfindingSign::SetSignText(const FText& NewText)
{
	SignText = NewText;
	UpdatePanelText();
}

void APPWayfindingSign::UpdatePanelText()
{
	if (!PanelText) return;
	const int32 CharacterCount = FMath::Max(1, SignText.ToString().Len());
	const float TextSize = FMath::Clamp(230.0f / (CharacterCount * 0.56f), 19.0f, 42.0f);
	PanelText->SetWorldSize(TextSize);
	PanelText->SetText(SignText);
}
