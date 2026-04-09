#pragma once

#include "CoreMinimal.h"
#include "ABaseCharacter.h"
#include "InputActionValue.h"
#include "ARangerCharacter.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API ARangerCharacter : public ABaseCharacter
{
    GENERATED_BODY()

public:
    ARangerCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

private:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    void StartCrouch(const FInputActionValue& Value);
};