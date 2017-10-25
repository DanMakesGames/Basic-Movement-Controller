/**
* Written by Daniel Mann.
* created in 2017
* DanielMannGames@outlook.com
*/

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
	// The maximum ground speed of the player
	float maxMoveSpeed;

	// The maximum rate the player can turn.
	float maxRotationSpeed;

	/* This is the maximum angle in terms of radians from the ground normal to (0, 0, 1)
	* can be before the ground is considered too sloped to walk on */
	float maxWalkableSlope;

	// The maximum height of actor that the player can step onto.
	float maxStepUpHeight;

	// The gravity the player is effected by when falling
	float gravity;

	// Constructor
	UTacMoveComp();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Cuts off the decimal part of value, by place places
	static double CutOff(double value, int place);

	
	// Sets the inputVelocity only if the player is walking. This function is the primary means the pawn has to effect the player movement.
	void SetVelocity(const FVector& inVelocity);
	
	// Gets the velocity
	FVector GetVelocity() const;

	// Sets the rotationalVelocity. This is the means by which the pawn effects the rotation of the player.
	void SetRotationVelocity(const FRotator& inVelocity);

	// Gets the rotationalVelocity.
	FRotator GetRotationVelocity() const;

	/**
	 * Preforms a move, and resolves penetration if the move started penetrating.
	 * Returns true if move final move occurs with no blocks
	 */
	bool ResolveAndMove(const FVector& positionDelta, const FQuat& newRotation, FHitResult& outHit);

	// This function exists so capsuleComponent can get set.
	void Initalize(class UCapsuleComponent* CapCom);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:

	/** 
	 * This is the component of the pawn that will be manipulated by this movement component. In order to move the pawn, this component needs to be the
	 * root of the pawn. This component's bounds will be used for collision with the world.
	 */
	UCapsuleComponent* capsuleComponent;
	
	// The current velocity of the player. This is what is applied to move.
	FVector velocity;

	// Used to store input from the pawn. This determines what the next rotation of the player will be.
	FRotator rotationVelocity;

	// Used to store input from the pawn. Used to collect desired movement.
	FVector inputVelocity;

	// The current movement state of the player. 
	MOVE_STATE moveState;

	//The normal that defines the plane of the ground we are currently on.
	FVector groundPlane;

	// If true, then allow for sweeps to occur where the object ignores first contact. Used for penetration resolution.
	bool bIgnoreInitPenetration;

	/* Move until blocking hit is made. No ground or character logic really exists here. 
	 * Returns true if move takes place fully,false if not*/
	bool Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & OutHit, AActor * ignoreActor);

	/* This is the head of the movement update chain.*/
	bool performMovement(float DeltaTime);

	// Resolves any penetration the play currently is in.
	bool ResolvePenetration(const FVector& proposedAdjustment, const FHitResult & hit, const FQuat & newRotation);
	
	// Moves the player up a hit slope
	bool PerformWalkUp(const FVector& delta, const FHitResult& slopeHit, FHitResult* outHit);
	
	// Moves the player against a hit wall.
	bool SlideAgainstWall(const FVector& delta, const FHitResult& wallHit);
	
	// Steps the player onto an hit actor if possible.
	bool PerformStepUp(const FVector& delta, const FHitResult& blockingHit);
	
	// Sets groundPlane
	void SetGroundPlane(const FVector& inNormal);

	// Compairs the angle between the ground and the (0,0,1) vector  against maxWalkableSlope.
	bool IsSlopeAngleValid(const FVector& groundNormal);

	// Returns groundPlane
	FVector GetGroundPlane() const;

	// Returns a vector used to move out of a penetration.
	FVector GetPenetrationAdjustment(const FHitResult & hit);

	// Tolorance for what is ground.
	static const int FLOOR_DETECTION_PERCISION;

	// How much additional distance to move the player out of a penetration. Used by GetPenetrationAdjustment
	static const float PENETRATE_ADITIONAL_SPACING;

	// How strict ResolvePenetration is when making the decision to teleport the player out of a penetration.
	static const float RESOLVE_STRICTNESS;

	// Distance we set from an object after a blocking collision.
	static const float TOUCH_TOLERANCE;
	
	// Maximum distance from the ground the player can before they are considered falling.
	static const float MAX_FLOOR_DIST;

	// Minimum distance we want to be from the floor. If closer than this we back off.
	static const float MIN_FLOOR_DIST;

	// Tolorance for the radius method of detecting ground. The larger the less tolorant the detection (aka the harder it is to be ground)
	static const float GROUND_DETECT_RADIUS_TOLERANCE;
};
