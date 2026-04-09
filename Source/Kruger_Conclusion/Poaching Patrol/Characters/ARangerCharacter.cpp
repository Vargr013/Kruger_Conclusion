#include "ARangerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

ARangerCharacter::ARangerCharacter()
{
}

void ARangerCharacter::BeginPlay()
{
    Super::BeginPlay();

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
}

void ARangerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput) return;

    // Movement
    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARangerCharacter::Move);

    // Looking
    EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARangerCharacter::Look);

    // Jump
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ARangerCharacter::StartJump);
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARangerCharacter::StopJump);

    // Sprint (Hold to Sprint)
    EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ARangerCharacter::StartSprint);

    // Crouch (Press to Toggle)
    EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ARangerCharacter::StartCrouch);

    // Interact
    EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ARangerCharacter::Interact);
}

// Input handlers

void ARangerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MoveVector = Value.Get<FVector2D>();

    if (Controller && (MoveVector.X != 0.0f || MoveVector.Y != 0.0f))
    {
        // Find out which way is forward based on where we are looking
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

void ARangerCharacter::StartSprint(const FInputActionValue& Value)
{
    ToggleSprint();
}

void ARangerCharacter::StopSprint(const FInputActionValue& Value)
{
    ToggleSprint();        // Toggle off when released
}

void ARangerCharacter::StartCrouch(const FInputActionValue& Value)
{
    ToggleCrouch();
}