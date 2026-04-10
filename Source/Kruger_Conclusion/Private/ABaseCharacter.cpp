#include "ABaseCharacter.h"
#include "InteractInterface.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true; // Impacts performance, runs code every frame for this actor

    // Camera Setup
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 300.0f;
    SpringArm->bUsePawnControlRotation = true;        // Camera follows mouse look

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->bUsePawnControlRotation = false;          // Only SpringArm rotates with mouse

    // movement setup
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 600.0f;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

void ABaseCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ABaseCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// movement functions - called by player character

void ABaseCharacter::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void ABaseCharacter::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
}

void ABaseCharacter::StartJump()
{
    Jump();
}

void ABaseCharacter::StopJump()
{
    StopJumping();
}

void ABaseCharacter::ToggleSprint()
{

    if (bIsCrouched) return; // prevent sprinting while crouched

    bIsSprinting = !bIsSprinting;

    if (bIsSprinting)
    {
        GetCharacterMovement()->MaxWalkSpeed = 900.0f;   // Sprint speed
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = 600.0f;   // Normal walk speed
    }

    OnMovementStateChanged();
}

void ABaseCharacter::ToggleCrouch()
{
	if (bIsCrouched) // using ACharacter's built in crouch state check
    {
        UnCrouch();
    }
    else
    {
        bIsSprinting = false;
        GetCharacterMovement()->MaxWalkSpeed = 600.0f;
        Crouch();
    }
    GetWorldTimerManager().SetTimerForNextTick(this, &ABaseCharacter::OnMovementStateChanged);
}

void ABaseCharacter::UpdateNoiseRadius()
{
    float CurrentNoiseRadius;

    if (bIsCrouched)
    {
        CurrentNoiseRadius = 200.0f;
    }
    else if (bIsSprinting)
    {
        CurrentNoiseRadius = 1200.0f;
    }
    else
    {
        CurrentNoiseRadius = 600.0f;   // Default for walking noise
    }

    OnNoiseRadiusChanged.Broadcast(CurrentNoiseRadius);
}

// Called when movement state changes in toggle sprint
void ABaseCharacter::OnMovementStateChanged()
{
    UpdateNoiseRadius();
}

void ABaseCharacter::Interact()
{
    if (!Camera) return;

    FVector StartLocation = Camera->GetComponentLocation();
    FVector ForwardDirection = Camera->GetForwardVector();
    FVector EndLocation = StartLocation + (ForwardDirection * 400.0f);   // 4 meter reach

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        Params
    );

    if (bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();

        // Check if the actor implements the interface
        if (HitActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
        {
            // Use the 'Execute_' prefix. This is the magic Unreal way to call interfaces safely.
            IInteractInterface::Execute_Interact(HitActor, this);
        }
    }
    else
    {
        DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f);
    }
}