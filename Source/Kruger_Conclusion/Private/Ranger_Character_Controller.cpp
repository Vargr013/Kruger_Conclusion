// Fill out your copyright notice in the Description page of Project Settings.


#include "Ranger_Character_Controller.h"

// Sets default values
ARanger_Character_Controller::ARanger_Character_Controller()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARanger_Character_Controller::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARanger_Character_Controller::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARanger_Character_Controller::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

