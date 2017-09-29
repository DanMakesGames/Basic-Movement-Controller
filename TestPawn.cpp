// Fill out your copyright notice in the Description page of Project Settings.

#include "TestPawn.h"
#include "TacMoveComp.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ATestPawn::ATestPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	moveComponent = CreateDefaultSubobject<UTacMoveComp>(TEXT("Movement Component"));

	movementShape = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Cylander shape for movement"));
	SetRootComponent(movementShape);
	movementShape->InitCapsuleSize(10,50);
	moveComponent->Initalize(movementShape);
	movementShape->bMultiBodyOverlap = true;
	movementShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);


	
	cameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	cameraComponent->SetupAttachment(RootComponent);
	cameraRotation = FRotator(0,0,0);
}

// Called when the game starts or when spawned
void ATestPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATestPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCamera(DeltaTime);

}

// Called to bind functionality to input
void ATestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	//InputComponent->BindAxis("Forward", this, &AAvatar::MoveForward);
	PlayerInputComponent->BindAxis("Forward", this, &ATestPawn::MoveForward);
	PlayerInputComponent->BindAxis("Strafe", this, &ATestPawn::MoveStrafe);
	PlayerInputComponent->BindAxis("Yaw", this, &ATestPawn::CameraYaw);
	PlayerInputComponent->BindAxis("Pitch", this, &ATestPawn::CameraPitch);

	

}

//void MoveForward(float val);
void ATestPawn::MoveForward(float val)
{
	//UE_LOG(LogTemp, Warning, TEXT("MoveForward: %f"), val);
	moveComponent->SetVelocity(FVector(val, 0, 0) + moveComponent->GetVelocity());
}

//void MoveStrafe(float val);
void ATestPawn::MoveStrafe(float val)
{
	moveComponent->SetVelocity(FVector(0, val, 0) + moveComponent->GetVelocity());
}

void ATestPawn::CameraYaw(float val)
{
	//cameraRotation.Add(0, val, 0);
	moveComponent->SetRotationVelocity(FRotator(0, val, 0));
}

void ATestPawn::CameraPitch(float val)
{
	
	cameraRotation += FRotator(val * moveComponent->maxRotationSpeed, 0, 0);

}

bool ATestPawn::UpdateCamera(float DeltaTime)
{

	FQuat finalRotation = movementShape->GetComponentQuat() * (cameraRotation * DeltaTime).Quaternion();
	cameraComponent->SetWorldRotation(finalRotation);
	return true;
}