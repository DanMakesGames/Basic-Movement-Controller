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
	
	UTacMoveComp();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	static double CutOff(double value, int place);

	void SetVelocity(const FVector& inVelocity);
	FVector GetVelocity() const;

	void SetRotationVelocity(const FRotator& inVelocity);
	FRotator GetRotationVelocity() const;

	void SetGroundPlane(const FVector& inNormal);
	FVector GetGroundPlane() const;

	/**
	 * Preforms a move, and resolves penetration if the move started penetrating.
	 * Returns true if move final move occurs with no blocks
	 */
	bool ResolveAndMove(const FVector& positionDelta, const FQuat& newRotation, FHitResult& outHit);

	void Initalize(class UCapsuleComponent* CapCom);
	
	bool IsSlopeAngleValid(const FVector& groundNormal);

	float maxMoveSpeed;
	float maxRotationSpeed;

	/* This is the maximum angle in terms of radians from the ground normal to (0, 0, 1)
	 * can be before the ground is considered too sloped to walk on */
	float maxWalkableSlope;
	float maxStepUpHeight;
	float gravity;

	MOVE_STATE moveState;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	//bool bIgnoreInitPenetration;//if true, then allow for sweeps to occur where the object ignores first contact.
	UCapsuleComponent* capsuleComponent;
	
	// The current velocity of the player. This is what is applied to move.
	FVector velocity;
	FRotator rotationVelocity;
	FVector inputVelocity;

	//The normal that defines the plane of the ground we are currently on.
	FVector groundPlane;

	bool bIgnoreInitPenetration;

	int FLOOR_DETECTION_PERCISION;
	
	/* Move until blocking hit is made. No ground or character logic really exists here. 
	 * Returns true if move takes place fully,false if not*/
	bool Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & OutHit, AActor * ignoreActor);

	/* This is the head of the movement update chain.*/
	bool performMovement(float DeltaTime);

	bool ResolvePenetration(const FVector& proposedAdjustment, const FHitResult & hit, const FQuat & newRotation);
	bool PerformWalkUp(const FVector& delta, const FHitResult& slopeHit, FHitResult* outHit);
	bool SlideAgainstWall(const FVector& delta, const FHitResult& wallHit);
	bool PerformStepUp(const FVector& delta, const FHitResult& blockingHit);

	FVector GetPenetrationAdjustment(const FHitResult & hit);

	float PENETRATE_ADITIONAL_SPACING;
	float RESOLVE_STRICTNESS;

	// Distance we set from an object after a blocking collision.
	float TOUCH_TOLERANCE;
	
	//Maximum distance from the ground the player can before they are considered falling.
	float MAX_FLOOR_DIST;

	//Minimum distance we want to be from the floor. If closer than this we back off.
	float MIN_FLOOR_DIST;

	//Tolorance for the radius method of detecting ground. The larger the less tolorant the detection (aka the harder it is to be ground)
	float GROUND_DETECT_RADIUS_TOLERANCE;

	
	
		
	
};
