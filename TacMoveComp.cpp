/**
 * Written by Daniel Mann.
 * created in 2017
 * DanielMannGames@outlook.com
 */


#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


const int UTacMoveComp::FLOOR_DETECTION_PERCISION = 4;
const float UTacMoveComp::PENETRATE_ADITIONAL_SPACING = 0.125;
const float UTacMoveComp::RESOLVE_STRICTNESS = 0.1;
const float UTacMoveComp::TOUCH_TOLERANCE = 0.001f;

const float UTacMoveComp::MAX_FLOOR_DIST = 2.4;
const float UTacMoveComp::MIN_FLOOR_DIST = 1;

const float UTacMoveComp::GROUND_DETECT_RADIUS_TOLERANCE = 0.15;


UTacMoveComp::UTacMoveComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 150;
	maxRotationSpeed = 100;
	maxWalkableSlope = PI / 4.0;
	maxStepUpHeight = 25;
	gravity = -1200;

	moveState = MOVE_STATE::FALLING;

	bIgnoreInitPenetration = false;
	SetGroundPlane(FVector(0, 0, 1));
}


void UTacMoveComp::Initalize(UCapsuleComponent * CapCom)
{
	capsuleComponent = CapCom;
}


void UTacMoveComp::BeginPlay()
{
	Super::BeginPlay();	
}


void UTacMoveComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	performMovement(DeltaTime);
}


void UTacMoveComp::SetVelocity(const FVector& inVelocity)
{
	if (moveState == MOVE_STATE::WALKING)
	{
		inputVelocity = inVelocity;
	}
}


FVector UTacMoveComp::GetVelocity() const
{
	return inputVelocity;
}


void UTacMoveComp::SetRotationVelocity(const FRotator& inVelocity)
{
	rotationVelocity = inVelocity;
}


FRotator UTacMoveComp::GetRotationVelocity() const
{
	return rotationVelocity;
}


void UTacMoveComp::SetGroundPlane(const FVector& inNormal)
{
	groundPlane = inNormal.GetSafeNormal();

}


FVector UTacMoveComp::GetGroundPlane() const
{
	return groundPlane;
}


bool UTacMoveComp::IsSlopeAngleValid(const FVector& groundNormal)
{
	return FMath::Acos(FVector::DotProduct(groundNormal.GetSafeNormal(), FVector(0,0,1))) <= maxWalkableSlope;
}


bool UTacMoveComp::performMovement(float DeltaTime)
{
	// The initial movement delta applied to the player.
	FVector moveDelta;
	// Rotation that we want to move to.
	FQuat newRotation = capsuleComponent->GetComponentQuat() * (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion();
	
	// If Walking apply input velocity to velocity.
	if (moveState == WALKING)
	{
		velocity += FVector(newRotation.RotateVector(inputVelocity).GetSafeNormal().X, newRotation.RotateVector(inputVelocity).GetSafeNormal().Y, -(GetGroundPlane() | newRotation.RotateVector(inputVelocity).GetSafeNormal()) / GetGroundPlane().Z).GetSafeNormal() * maxMoveSpeed;
	}
	// If falling apply gravity.
	else if (moveState == FALLING)
	{
		velocity += FVector(0, 0, gravity) * DeltaTime;
	}

	moveDelta = velocity * DeltaTime;

	//Move the player
	FHitResult hit;
	bool bInitalMoveComplete = ResolveAndMove(moveDelta, newRotation, hit);
	
	//If Hit Did Occur durring the move
	if(!bInitalMoveComplete)
	{
		//Fist see if we can preform a step up. 
		bool bDidStepUp = false;
		if (hit.ImpactNormal.Z < KINDA_SMALL_NUMBER && hit.ImpactNormal.Z > -KINDA_SMALL_NUMBER && moveState == MOVE_STATE::WALKING)
		{
			bDidStepUp = PerformStepUp(moveDelta, hit);
		}

		if (!bDidStepUp)
		{
			double DistanceFromCenter = (hit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
			bool bIsInRange = DistanceFromCenter < FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE);
			if (bIsInRange)
			{
				if (moveState == MOVE_STATE::WALKING)
				{
					// Walk Up slope if it is not too sloped.
					if (IsSlopeAngleValid(hit.ImpactNormal))
					{
						// Set the ground plane	
						SetGroundPlane(hit.ImpactNormal);
						
						// Calculate the movement up the slope
						FVector flat = FVector(moveDelta.X, moveDelta.Y, 0);
						float dot = hit.ImpactNormal | flat;
						FVector rampMove = FVector(flat.X, flat.Y, -(dot / hit.ImpactNormal.Z));
						FVector walkupDelta = rampMove.GetSafeNormal() * (moveDelta * (1.0f - hit.Time)).Size();

						//Perform the walkup
						FHitResult tempHit;
						if (!ResolveAndMove(walkupDelta, capsuleComponent->GetComponentQuat(), tempHit))
						{
							//If the walkup hit a wall, slide against it.
							const FVector travelDirection = FVector::CrossProduct(tempHit.ImpactNormal, hit.ImpactNormal).GetSafeNormal();
							FVector slideVector = (walkupDelta * (1 - tempHit.Time)).Size() * (FVector::DotProduct(walkupDelta.GetSafeNormal(), travelDirection)) * travelDirection;
							ResolveAndMove(slideVector, newRotation, tempHit);
						}
					}
					else
					{	
						// If the ground did not have a valid angle, and could not be walked up, just slide against it.
						SlideAgainstWall(moveDelta, hit);
					}
				}

				// If falling and something hits the bottom of the capsule
				if (moveState == MOVE_STATE::FALLING)
				{
					// Is the thing hit valid ground?
					if (IsSlopeAngleValid(hit.ImpactNormal))
					{
						// Yes it is ground. Set it as the new ground normal and set stated to walking.
						SetGroundPlane(hit.ImpactNormal);
						moveState = MOVE_STATE::WALKING;
					}
					else 
					{
						// No the floor is not valid ground due to slope. Slide down it.
						FHitResult slideDownHit;
						velocity -= FVector::DotProduct(hit.ImpactNormal, velocity.GetSafeNormal()) * velocity.Size() * hit.ImpactNormal;
						ResolveAndMove(velocity * (DeltaTime) * (1 - hit.Time), capsuleComponent->GetComponentQuat(), slideDownHit);
					}
				}
			}

			// Hit object is not touching bottom of the capsule
			else
			{
				//if we hit something while falling remove that normal from the velocity
				if (moveState == FALLING)
				{
					velocity -= FVector::DotProduct(hit.ImpactNormal, velocity.GetSafeNormal()) * velocity.Size() * hit.ImpactNormal;
				}

				//Sliding along wall behavior
				if (moveState == MOVE_STATE::WALKING)
				{
					SlideAgainstWall(moveDelta, hit);
				}
			}
		}
	}

	//Check if we should start falling
	if (moveState == MOVE_STATE::WALKING)
	{
		//Here we are checking for ground
		TArray<FHitResult> outHits;
		FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());
		GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() - FVector(0, 0, MAX_FLOOR_DIST), capsuleComponent->GetComponentQuat(), outParams);
	
		if(outHits.Num() > 0)
		{
			//Block did occur, now we must determine if it is ground
			bool bGroundFound = false;
			for (int index = 0; index < outHits.Num(); index++)
			{
				
				//Check if it is ground
				if (CutOff(outHits[index].ImpactPoint.Z, FLOOR_DETECTION_PERCISION) < CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, FLOOR_DETECTION_PERCISION))
				{
					// Set Ground normal if valid
					if (IsSlopeAngleValid(outHits[index].ImpactNormal))
						SetGroundPlane(outHits[index].ImpactNormal);
					
					//Floor Magnetism: Keep at a constant distance from the ground when moving
					if (outHits[index].Distance > MIN_FLOOR_DIST)
					{
						FHitResult myHit;			
						ResolveAndMove(FVector(0.f, 0.f, MIN_FLOOR_DIST - outHits[index].Distance), capsuleComponent->GetComponentQuat(), myHit);
					}

					bGroundFound = true;
					break;
				}
				
			}

			// Ground was not found, so begin falling
			if (!bGroundFound)
			{
				moveState = MOVE_STATE::FALLING;
			}
			// We are still grounded after move
			else
			{
				velocity = FVector::ZeroVector;
			}
		}
		// nothing touching player so they fall
		else
		{
			moveState = MOVE_STATE::FALLING;
		}
	}
	
	// Reset the input information in preperation for new imformation next tick.
	inputVelocity = FVector::ZeroVector;
	rotationVelocity = FRotator(0, 0, 0);

	return true;
}


bool UTacMoveComp::Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & outHit, AActor * ignoreActor = NULL)
{
	//Reset hit reference
	outHit.Reset();

	//Peform sweep to see if this move will hit anything
	TArray<FHitResult> outHits;
	FComponentQueryParams outParams(FName(TEXT("Move Trace")), GetOwner());
	
	if (bIgnoreInitPenetration)
	{
		outParams.bFindInitialOverlaps = false;
	}

	if (ignoreActor != NULL)
	{
		outParams.AddIgnoredActor(ignoreActor);
	}

	capsuleComponent->SetWorldRotation(NewRotation);
	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, NewRotation, outParams);
	
	//Do not move if the player started already penetrating
	if (outHits.Num() > 0)
	{
		if (outHits[0].bStartPenetrating)
		{
			outHit = outHits[0];
			return false;
		}
	}

	/* Loop through, looking for a valid blocking hit. These hits must have normals facing
	 * the same direction as the velocity, then set the value of the outHit*/
	bool bCompleteMove = true;
	for (int i = 0; i < outHits.Num(); i++)
	{
		//Check if we are moving into the hit object. This indicates a true blocking collision.
		if (FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()) < 0)
		{
			outHit = outHits[i];
			bCompleteMove = false;
			break;
		}
	}

	if (bCompleteMove)
	{
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + Delta);
	}
	else
	{
		if (bIgnoreInitPenetration)
			capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * outHit.Time));
		else
			capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * outHit.Time) + (outHit.Normal * TOUCH_TOLERANCE));
	}
	
	return bCompleteMove;
}


bool UTacMoveComp::ResolveAndMove(const FVector& positionDelta, const FQuat& newRotation, FHitResult& outHit)
{
	FHitResult hit;
	//Initial Move attempt
	bool bMoveCompleted = Move(positionDelta,newRotation,hit);

	//If starting move penetrated, resolve the penetration and try move again
	if (!bMoveCompleted && hit.bStartPenetrating)
	{
		FVector adjustment = GetPenetrationAdjustment(hit);
		ResolvePenetration(adjustment, hit, capsuleComponent->GetComponentQuat());
		
		bMoveCompleted = Move(positionDelta, newRotation, hit);
	}

	outHit = hit;
	
	return bMoveCompleted;
}

bool UTacMoveComp::PerformWalkUp(const FVector& delta, const FHitResult& slopeHit, FHitResult* outHit = NULL)
{
	// Calculate walk up delta
	FVector flatMovement = FVector(delta.X, delta.Y, 0);
	float groundFlatDeltaDot = slopeHit.ImpactNormal | flatMovement;
	FVector moveDirection = FVector(flatMovement.X, flatMovement.Y, -(groundFlatDeltaDot / slopeHit.ImpactNormal.Z));
	FVector walkUpDelta = moveDirection.GetSafeNormal() * (delta * (1.0f - slopeHit.Time)).Size();
	
	if (outHit != NULL)
		return ResolveAndMove(walkUpDelta,capsuleComponent->GetComponentQuat(),*outHit);

	FHitResult tempHit;
		return ResolveAndMove(walkUpDelta, capsuleComponent->GetComponentQuat(), tempHit);
}


bool UTacMoveComp::SlideAgainstWall(const FVector& delta, const FHitResult& wallHit)
{
	//Check if we have a vertical normal
	FVector wallHeading = FVector(wallHit.ImpactNormal.X, wallHit.ImpactNormal.Y, 0).GetSafeNormal();

	if (wallHeading == FVector::ZeroVector)
		return false;

	// Calculate the slide movement Delta
	const FVector travelDirection = FVector::CrossProduct(wallHeading, GetGroundPlane()).GetSafeNormal();
	const FVector slideVector = (delta * (1 - wallHit.Time)).Size() * (FVector::DotProduct(delta.GetSafeNormal(), travelDirection)) * travelDirection;
	
	FHitResult slideHit;
	if (!ResolveAndMove(slideVector, capsuleComponent->GetComponentQuat(), slideHit))
	{
		// If a slope is hit, walk up the slope
		double DistanceFromCenter = (slideHit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
		bool bIsInRange = DistanceFromCenter < FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE);
		if (bIsInRange)
		{
			if (IsSlopeAngleValid(slideHit.ImpactNormal))
				PerformWalkUp(slideVector, slideHit, &slideHit);
		}
	}

	return true;
}


bool UTacMoveComp::PerformStepUp(const FVector& delta, const FHitResult& blockingHit)
{
	
	FHitResult sweepHit;
	//I only want to sweep against the hit actor

	// Perform a sweep downwards inorder to find the top of the hit object.
	const FVector traceStart(blockingHit.ImpactPoint.X, blockingHit.ImpactPoint.Y,blockingHit.ImpactPoint.Z + capsuleComponent->GetScaledCapsuleHalfHeight() * 2 + maxStepUpHeight);
	const FVector traceEnd(blockingHit.ImpactPoint.X, blockingHit.ImpactPoint.Y, blockingHit.ImpactPoint.Z - (capsuleComponent->GetScaledCapsuleHalfHeight() * 2 + maxStepUpHeight));
	bool bDidHit = blockingHit.Component->SweepComponent(sweepHit, traceStart, traceEnd,capsuleComponent->GetComponentQuat(),capsuleComponent->GetCollisionShape());

	// Calculate the distance from the top of the object to the bottom of the character
	float distance = sweepHit.ImpactPoint.Z - (capsuleComponent->GetComponentLocation().Z - capsuleComponent->GetScaledCapsuleHalfHeight());
	
	// If the distance is less than the maximum step up height, the character may step up.
	if (bDidHit && (distance <= maxStepUpHeight) && (distance >= 0))
	{
		
		// Now we need to make sure space exists large enough for the player to teleport to.
		// First perform a sweep to see how high up the ledge is.
		FCollisionQueryParams QueryParams(FName(TEXT("Step up")), true, GetOwner());
		FCollisionResponseParams ResponseParam;

		// This is the final destination of the step up
		FVector destination = FVector(sweepHit.ImpactPoint.X, sweepHit.ImpactPoint.Y,sweepHit.ImpactPoint.Z + capsuleComponent->GetScaledCapsuleHalfHeight() + MIN_FLOOR_DIST);
		bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(destination, capsuleComponent->GetComponentQuat(), capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(), QueryParams, ResponseParam);
		
		// If something occupies the space we want to step up to, then do not step up.
		if (!bOverlapping)
		{
			capsuleComponent->SetWorldLocation(destination);
			FHitResult moveHit;
			//ResolveAndMove(delta * (1 - blockingHit.Time), capsuleComponent->GetComponentQuat(), moveHit);
			return true;
		}
	}

	return false;
}


bool UTacMoveComp::ResolvePenetration(const FVector& proposedAdjustment, const FHitResult& hit, const FQuat& newRotation)
{
	//First we test the proposed location with overlap.
	FCollisionQueryParams QueryParams(FName(TEXT("resolve penetration")),true, GetOwner());
	FCollisionResponseParams ResponseParam;
	capsuleComponent->InitSweepCollisionParams(QueryParams, ResponseParam);
	bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(hit.TraceStart + proposedAdjustment, newRotation, capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(RESOLVE_STRICTNESS), QueryParams, ResponseParam);
	
	if (!bOverlapping)
	{
		//no overlaps means we can resolve with teleport.
		capsuleComponent->SetWorldLocation(hit.TraceStart + proposedAdjustment);
	}
	else
	{	
		FHitResult SweepOutHit(1.0f);
		bool bDidMove = Move(proposedAdjustment, capsuleComponent->GetComponentQuat(), SweepOutHit, hit.GetActor());
	
		//Dual Penetration solver
		if (!bDidMove && SweepOutHit.bStartPenetrating)
		{
			FVector otherAdjustment = GetPenetrationAdjustment(SweepOutHit);
			FVector comboAdjustment = otherAdjustment + proposedAdjustment;

			if (otherAdjustment != proposedAdjustment && !comboAdjustment.IsZero())
			{
				bIgnoreInitPenetration = true;
				bDidMove = Move(comboAdjustment, capsuleComponent->GetComponentQuat(), SweepOutHit);
				bIgnoreInitPenetration = false;
			}
		}

		//Failed Move Out sovler
		if (!bDidMove)
		{
			FVector otherAdjustment = SweepOutHit.ImpactNormal * PENETRATE_ADITIONAL_SPACING;
			FVector comboAdjustment = otherAdjustment + proposedAdjustment;

			if (otherAdjustment != proposedAdjustment && !comboAdjustment.IsZero())
			{
				bIgnoreInitPenetration = true;
				bDidMove = Move(comboAdjustment, capsuleComponent->GetComponentQuat(), SweepOutHit);
				bIgnoreInitPenetration = false;
			}
		}
	}

	//Check if we are still overlapping
	if (GetWorld()->OverlapBlockingTestByChannel(capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentQuat(), capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(), QueryParams, ResponseParam))
	{
		UE_LOG(LogTemp, Warning, TEXT("Still"));
		return false;
	}
	
	return true;
}


FVector UTacMoveComp::GetPenetrationAdjustment(const FHitResult& hit)
{
	if (!hit.bStartPenetrating)
	{
		return FVector::FVector(0, 0, 0);
	}

	const float penetrationDepth = (hit.PenetrationDepth > 0.f ? hit.PenetrationDepth : PENETRATE_ADITIONAL_SPACING);
	return hit.Normal * (penetrationDepth + PENETRATE_ADITIONAL_SPACING);
}


double UTacMoveComp::CutOff(double value, int place)
{
	int test = value * pow(10, place) * 10;
	double test2 = test / (10);
	test2 = test2 / pow(10, place);
	
	return test2;
}