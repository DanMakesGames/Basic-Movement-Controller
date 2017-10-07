// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacMoveComp.generated.h"

enum MOVE_STATE {
	WALKING,
	FALLING
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MOVECOMPTEST_API UTacMoveComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTacMoveComp();

	void SetVelocity(const FVector& inVelocity);
	FVector GetVelocity();
	void SetRotationVelocity(const FRotator& inVelocity);
	FRotator GetRotationVelocity();

	float maxMoveSpeed;
	float maxRotationSpeed;
	float gravity;

	bool bIgnoreInitPenetration;


	MOVE_STATE moveState;


	void Initalize(class UCapsuleComponent* CapCom);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	static double CutOff(double value, int place);

private:
	UCapsuleComponent* capsuleComponent;
	
	FVector velocity;
	FRotator rotationVelocity;
	FVector inputVelocity;

	int FLOOR_DETECTION_PERCISION;

	//MovementComponent::MoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, ETeleportType Teleport)
	
	/* Move until blocking hit is made. No ground or character logic really exists here. 
	 * Returns true if move takes place fully,false if not*/
	bool Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & OutHit);

	/* This is the head of the movement update chain.*/
	bool performMovement(float DeltaTime);

	bool ResolvePenetration(const FVector& proposedAdjustment, const FHitResult & hit, const FQuat & newRotation);
	FVector GetPenetrationAdjustment(const FHitResult & hit);

	float PENETRATE_ADITIONAL_SPACING;
	float RESOLVE_STRICTNESS;
	
		
	
};
