// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


#define TOUCH_TOLERANCE 0.001f // Distance we set from an object after a blocking collision 0.001f.
#define MAX_FLOOR_DIST 2.4 // greatest distance we can be from the floor before we consider it falling
const float MIN_FLOOR_DIST = 1;


UTacMoveComp::UTacMoveComp()
{

	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 150;
	maxRotationSpeed = 100;
	moveState = MOVE_STATE::FALLING;

	FLOOR_DETECTION_PERCISION = 4;
	PENETRATE_ADITIONAL_SPACING = 0.125;
	RESOLVE_STRICTNESS = 0.1;
	
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


bool UTacMoveComp::performMovement(float DeltaTime)
{
	
	FVector newVector;

	//Rotation that we want to move to.
	FQuat newRotation = capsuleComponent->GetComponentQuat() * (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion();
	
	//Standard motion
	if (moveState == WALKING)
	{
		newVector = velocity + newRotation.RotateVector(inputVelocity.GetSafeNormal()) * maxMoveSpeed;
		velocity = newVector;
	}
	//if falling apply downward motion.
	else if (moveState == FALLING)
	{
		
		newVector = velocity + FVector(0, 0, -1) * maxMoveSpeed;

	}

	//Move the player
	FHitResult hit;
	bool bInitalMoveComplete = ResolveAndMove(newVector * DeltaTime, newRotation, hit);

	//If Hit Did Occur durring the move
	if(!bInitalMoveComplete)
	{
		//Evaluate if the ground is good enough to walk on.
		double DistanceFromCenter = (hit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
		bool bIsInRange = DistanceFromCenter < capsuleComponent->GetScaledCapsuleRadius();
		
		//The Hit Object is ground.
		if(bIsInRange)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Is Floor hitpoint: %f, %f"), CutOff(hit.ImpactPoint.Z, 4), CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4));
			if (moveState == MOVE_STATE::FALLING)
			{
				moveState = MOVE_STATE::WALKING;
			}
		}

		//Hit Object is not ground. This will need to be modified for slopes
		else
		{
			//if we hit something while falling remove that normal from the velocity
			if (moveState == FALLING)
			{
				velocity -= FVector::DotProduct(hit.Normal, velocity.GetSafeNormal()) * velocity.Size() * hit.Normal;
			}
			
			//Sliding along wall behavior
			if (moveState == MOVE_STATE::WALKING)
			{
				const FVector travelDirection = FVector::CrossProduct(hit.ImpactNormal, FVector(0, 0, 1)).GetSafeNormal();
				const FVector slideVector = (newVector * DeltaTime * (1 - hit.Time)).Size() * (FVector::DotProduct(newVector.GetSafeNormal(),travelDirection)) * travelDirection;
				
				ResolveAndMove(slideVector, newRotation, hit);
			}
		}
	}

	//Check if falling
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
					// It is ground
					
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

			//Ground was not found, so begin falling
			if (!bGroundFound)
			{
				moveState = MOVE_STATE::FALLING;
			}
			//We are still grounded after move
			else
			{
				//apply ground friction, which slows the player down (in future more complex ground friction could be applied)
				velocity = FVector::ZeroVector;
			}
		}
		//nothing touching player so they fall
		else
		{
			moveState = MOVE_STATE::FALLING;
		}
	}
	
	//Reset the input imformation in preperation for new imformation next tick.
	inputVelocity = FVector::ZeroVector;
	rotationVelocity = FRotator(0, 0, 0);

	return true;
}


bool UTacMoveComp::Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & outHit)
{
	//Reset hit reference
	outHit.Reset();

	//Peform sweep to see if this move will hit anything
	TArray<FHitResult> outHits;
	FComponentQueryParams outParams(FName(TEXT("Move Trace")), GetOwner());
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
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * outHit.Time) + (outHit.Normal * TOUCH_TOLERANCE));
	}
	
	capsuleComponent->SetWorldRotation(NewRotation);

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


//TODO: This function needs to be enhanced to full function, in particualr handling resolving penetraing two objects.
bool UTacMoveComp::ResolvePenetration(const FVector& proposedAdjustment, const FHitResult& hit, const FQuat& newRotation)
{
	//First we test the proposed location with overlap.
	FCollisionQueryParams QueryParams(FName(TEXT("resolve penetration")), false, capsuleComponent->GetOwner());
	FCollisionResponseParams ResponseParam;
	bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(hit.TraceStart + proposedAdjustment, newRotation, capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(RESOLVE_STRICTNESS), QueryParams, ResponseParam);
	
	if (!bOverlapping)
	{
		//no overlaps means we can resolve.
		capsuleComponent->SetWorldLocation(hit.TraceStart + proposedAdjustment);
	}
	else
	{	
		//TODO: This needs to be enchanced and all around improved
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + proposedAdjustment);
		/*
		FHitResult SweepOutHit(1.0f);
		bIgnoreInitPenetration = true;
		bool bDidMove = Move(proposedAdjustment, capsuleComponent->GetComponentQuat(), SweepOutHit);
		//If we are still penetrating.
		
		if (SweepOutHit.bStartPenetrating)
		{
			UE_LOG(LogTemp, Warning, TEXT("DUAL PENE REQUE"));
			FVector otherAdjustment = GetPenetrationAdjustment(SweepOutHit);
			FVector comboAdjustment = otherAdjustment + proposedAdjustment;
			if (otherAdjustment != proposedAdjustment && !comboAdjustment.IsZero())
			{
				bDidMove = Move(comboAdjustment, capsuleComponent->GetComponentQuat(), SweepOutHit);
			}
		}
		
		
		bIgnoreInitPenetration = false;
		*/
	}

	if (GetWorld()->OverlapBlockingTestByChannel(capsuleComponent->GetComponentLocation(), newRotation, capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(), QueryParams, ResponseParam))
	{
		UE_LOG(LogTemp, Warning, TEXT("Still"));
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