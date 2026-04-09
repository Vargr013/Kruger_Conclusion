#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/EngineTypes.h"           
#include "ABaseCharacter.generated.h"

// Delegate for noise system
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoiseRadiusChanged, float, NoiseRadius);

UCLASS(Abstract)
class KRUGER_CONCLUSION_API ABaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABaseCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* Camera;

    // stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Currency = 0;

	// check if character is currently sprinting
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;

    // noise system
    /**
     * Called whenever noise radius changes (walking / sprinting / crouching)
     */
    UPROPERTY(BlueprintAssignable, Category = "Noise")
    FOnNoiseRadiusChanged OnNoiseRadiusChanged;

protected:
    virtual void MoveForward(float Value);
    virtual void MoveRight(float Value);
    virtual void StartJump();
    virtual void StopJump();
    virtual void ToggleSprint();
    virtual void ToggleCrouch();

    virtual void Interact();

    /** Called whenever sprinting or crouching state changes */
    virtual void OnMovementStateChanged();

private:
    void UpdateNoiseRadius();
};