#pragma once

#include "CoreMinimal.h"
#include "ABaseCharacter.h"
#include "InputActionValue.h"
#include "ARangerCharacter.generated.h"

class ABaseGun;

UCLASS()
class KRUGER_CONCLUSION_API ARangerCharacter : public ABaseCharacter
{
    GENERATED_BODY()

public:
    ARangerCharacter();

	UFUNCTION(BlueprintCallable, Category = "Character")
    void Resupply();


protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // How fast the headbob bounces up and down. Higher = quicker steps. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0"))
    float HeadBobFrequency = 12.0f;

    // Up/down camera offset. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0"))
    float HeadBobVerticalAmount = 5.0f;

    // Side-to-side camera offset. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0"))
    float HeadBobHorizontalAmount = 1.0f;

    // Camera roll that tilts with each stride. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0"))
    float HeadBobRollAmount = 0.5f;

    // How quickly the bob blends in when sprint starts and out when it stops. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|HeadBob", meta = (ClampMin = "0.0"))
    float HeadBobBlendSpeed = 8.0f;

    // Input Actions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* InteractAction;

    // Fire Action
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* FireAction;

    // Gun Properties
    UPROPERTY(EditAnywhere, Category = "Weapon")
    TSubclassOf<ABaseGun> GunClass;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    ABaseGun* CurrentGun;

public:
    UFUNCTION(BlueprintPure, Category = "Weapon")
    ABaseGun* GetCurrentGun() const { return CurrentGun; }

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetCurrentGun(ABaseGun* NewGun) { CurrentGun = NewGun; }

private:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    void StartCrouch(const FInputActionValue& Value);

    void Fire();

    // Applies a sin-wave camera offset while sprinting on the ground. 
    void UpdateHeadBob(float DeltaTime);

    // Camera transform, for restoring its position when sprinting stops.
    FVector CameraRestRelativeLocation = FVector::ZeroVector;
    FRotator CameraRestRelativeRotation = FRotator::ZeroRotator;

    // Running phase of the sine wave. Reset when the bob fully blends out.
    float HeadBobTime = 0.0f;

    // 0 = rest pose, 1 = full sprint bob. Interpolated each tick. 
    float HeadBobPoint = 0.0f;
};
