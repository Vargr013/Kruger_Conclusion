// Copyright Epic Games, Inc. All Rights Reserved.

#include "Kruger_ConclusionCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PPHealthComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kruger_Conclusion.h"
#include "Interfaces/PPInteractableInterface.h"

AKruger_ConclusionCharacter::AKruger_ConclusionCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;
	HealthComponent = CreateDefaultSubobject<UPPHealthComponent>(TEXT("HealthComponent"));

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AKruger_ConclusionCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AKruger_ConclusionCharacter::OnPlayerHealthDepleted);
	}
}

float AKruger_ConclusionCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageAmount <= 0.0f || !HealthComponent || HealthComponent->IsDead())
	{
		return AppliedDamage;
	}

	HealthComponent->ApplyDamage(DamageAmount);
	return DamageAmount;
}

void AKruger_ConclusionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AKruger_ConclusionCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AKruger_ConclusionCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKruger_ConclusionCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKruger_ConclusionCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AKruger_ConclusionCharacter::LookInput);

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AKruger_ConclusionCharacter::DoInteract);
		}
	}
	else
	{
		UE_LOG(LogKruger_Conclusion, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AKruger_ConclusionCharacter::DoInteract);
}


void AKruger_ConclusionCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AKruger_ConclusionCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AKruger_ConclusionCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AKruger_ConclusionCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AKruger_ConclusionCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AKruger_ConclusionCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AKruger_ConclusionCharacter::DoInteract()
{
	if (!FirstPersonCameraComponent || !GetWorld())
	{
		return;
	}

	const FVector StartLocation = FirstPersonCameraComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation + (FirstPersonCameraComponent->GetForwardVector() * 500.0f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bVisibilityHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
	if (!bVisibilityHit || !HitResult.GetActor() || !HitResult.GetActor()->GetClass()->ImplementsInterface(UPPInteractableInterface::StaticClass()))
	{
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		GetWorld()->LineTraceSingleByObjectType(HitResult, StartLocation, EndLocation, ObjectParams, Params);
	}

	AActor* HitActor = HitResult.GetActor();
	if (HitActor && HitActor->GetClass()->ImplementsInterface(UPPInteractableInterface::StaticClass()))
	{
		IPPInteractableInterface::Execute_Interact(HitActor, this);
		UE_LOG(LogKruger_Conclusion, Log, TEXT("Interacted with %s"), *GetNameSafe(HitActor));
	}
}

void AKruger_ConclusionCharacter::OnPlayerHealthDepleted()
{
	if (bIsDowned)
	{
		return;
	}

	bIsDowned = true;
	GetCharacterMovement()->DisableMovement();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}

	BP_OnPlayerDowned();
}

void AKruger_ConclusionCharacter::ResetPlayerHealth()
{
	bIsDowned = false;

	if (HealthComponent)
	{
		HealthComponent->ResetHealth();
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		EnableInput(PlayerController);
	}
}
