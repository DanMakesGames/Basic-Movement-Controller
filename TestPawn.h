// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TestPawn.generated.h"


class UTacMoveComp;

UCLASS()
class MOVECOMPTEST_API ATestPawn : public APawn
{
	GENERATED_BODY()

private:
	UPROPERTY()
		class UTacMoveComp * moveComponent;
	
	class UCameraComponent * cameraComponent;
	
	void MoveForward(float val);
	void MoveStrafe(float val);
	void CameraYaw(float val);
	void CameraPitch(float val);

	FRotator cameraRotation;

	bool UpdateCamera(float DeltaTime);

public:
	// Sets default values for this pawn's properties
	ATestPawn();
	class UCapsuleComponent * movementShape;
	

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
};
