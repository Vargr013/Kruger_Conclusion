// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Ranger_Character_Controller.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API ARanger_Character_Controller : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARanger_Character_Controller();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
