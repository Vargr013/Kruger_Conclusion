#include "ARangerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h" 
#include "BaseGun.h"                

ARangerCharacter::ARangerCharacter()
{
}

void ARangerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Setup Input Context
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }

    // SPAWN AND ATTACH GUN
    if (GunClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        CurrentGun = GetWorld()->SpawnActor<ABaseGun>(GunClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

        if (CurrentGun)
        {
            // Attach to the Camera inherited from ABaseCharacter
            CurrentGun->AttachToComponent(Camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

            // Adjust this offset so the gun sits nicely in view
            CurrentGun->SetActorRelativeLocation(FVector(100.0f, 40.0f, -30.0f));
        }
    }
}

void ARangerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput) return;

    // Movement & Looking
    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARangerCharacter::Move);
    EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARangerCharacter::Look);

    // Jump
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ARangerCharacter::StartJump);
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARangerCharacter::StopJump);

    // Sprint & Crouch
    EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ARangerCharacter::StartSprint);
    EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ARangerCharacter::StartCrouch);

    // Interact
    EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ARangerCharacter::Interact);

    // NEW: Bind Fire Action
    EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ARangerCharacter::Fire);
}

void ARangerCharacter::Fire()
{
    if (CurrentGun)
    {
        CurrentGun->Shoot();
    }
}

// ... Keep your existing Move, Look, StartSprint, StopSprint, and StartCrouch logic below ...

void ARangerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MoveVector = Value.Get<FVector2D>();
    if (Controller && (MoveVector.X != 0.0f || MoveVector.Y != 0.0f))
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, MoveVector.Y);
        AddMovementInput(RightDirection, MoveVector.X);
    }
}

void ARangerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookVector.X);
    AddControllerPitchInput(LookVector.Y);
}

void ARangerCharacter::StartSprint(const FInputActionValue& Value) { ToggleSprint(); }
void ARangerCharacter::StopSprint(const FInputActionValue& Value) { ToggleSprint(); }
void ARangerCharacter::StartCrouch(const FInputActionValue& Value) { ToggleCrouch(); }