// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


#define TOUCH_TOLERANCE 0.001f // Distance we set from an object after a blocking collision 0.001f.
//#define TOUCH_TOLERANCE 1// Distance we set from an object after a blocking collision
#define MAX_FLOOR_DIST 2.4 // greatest distance we can be from the floor before we consider it falling
const float MIN_FLOOR_DIST = 1;

UTacMoveComp::UTacMoveComp()
{

	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 150;
	maxRotationSpeed = 100;
	moveState = MOVE_STATE::FALLING;

	FLOOR_DETECTION_PERCISION = 4;
	//moveState = MOVE_STATE::WALKING;

	PENETRATE_ADITIONAL_SPACING = 0.125;
	//PENETRATE_ADITIONAL_SPACING = 0.2f;
	RESOLVE_STRICTNESS = 0.1;
	
	bIgnoreInitPenetration = false;
	
}


void UTacMoveComp::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
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


FVector UTacMoveComp::GetVelocity()
{
	return inputVelocity;
}


void UTacMoveComp::SetRotationVelocity(const FRotator& inVelocity)
{
	rotationVelocity = inVelocity;
}


FRotator UTacMoveComp::GetRotationVelocity()
{
	return rotationVelocity;
}


bool UTacMoveComp::performMovement(float DeltaTime)
{
	FHitResult hit;
	FVector newVector;
	//UE_LOG(LogTemp, Warning, TEXT("-=-=-=-=-=-=-=-=-=-"));
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

	//Attempt Movement.
	bool bMoveCompleted = Move(newVector * DeltaTime, newRotation, hit);

	//Resolve Penetration if it has occured.
	if (!bMoveCompleted && hit.bStartPenetrating)
	{
		
		FVector adjustment = GetPenetrationAdjustment(hit);
		
		ResolvePenetration(adjustment, hit, newRotation);
		bMoveCompleted = Move(newVector * DeltaTime, newRotation, hit);

	}


	

	//--Move---------------------
	//Hit did occur
	if (!bMoveCompleted )
	{
		
		//if we hit something while falling remove that normal from the velocity
		
		
		
		

		//UE_LOG(LogTemp, Warning, TEXT("Is Floor time: My Method %f, %f"), (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z, (capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z);
		//if (hit.ImpactPoint.Z < (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
		//if (hit.ImpactPoint.Z < (capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z)
		
		//Evaluate if the ground is good enough to walk on
		double DistanceFromCenter = (hit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
		bool bIsInRange = DistanceFromCenter < capsuleComponent->GetScaledCapsuleRadius();

		//if (CutOff(hit.ImpactPoint.Z, FLOOR_DETECTION_PERCISION) < CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, FLOOR_DETECTION_PERCISION))
		if(bIsInRange)
		{

			UE_LOG(LogTemp, Warning, TEXT("Is Floor hitpoint: %f, %f"), CutOff(hit.ImpactPoint.Z, 4), CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4));
			//DrawDebugPoint(GetWorld(),hit.ImpactPoint,50,FColor::Cyan,false,10);
			if (moveState == MOVE_STATE::FALLING)
			{
				UE_LOG(LogTemp, Warning, TEXT("Set Walking"));
				moveState = MOVE_STATE::WALKING;
			}
		}
		// Hit Object is not ground. This will need to be modified for slopes
		else
		{
			
			if (moveState == FALLING)
			{
				UE_LOG(LogTemp, Warning, TEXT("Falling: Not Good Enough Ground."));
				velocity -= FVector::DotProduct(hit.Normal, velocity.GetSafeNormal()) * velocity.Size() * hit.Normal;

			}
			

			// SLIDE ALONG WALL
			if (moveState == MOVE_STATE::WALKING)
			{
				TArray<FHitResult> outHits;
				FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());
				float time = hit.Time;
				GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation()  + newVector * DeltaTime, capsuleComponent->GetComponentQuat(), outParams);
				FVector travelDirection = FVector::CrossProduct(hit.ImpactNormal, FVector(0, 0, 1)).GetSafeNormal();
				
				
				//DrawDebugDirectionalArrow(GetWorld(),capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + travelDirection * 100,50,FColor::Cyan,false, 10);
				//UE_LOG(LogTemp, Warning, TEXT("-------------------"));
				//if(hit.bStartPenetrating)
					//UE_LOG(LogTemp, Warning, TEXT("Sliding time: %b, %b"), hit.bBlockingHit, outHits[0].bBlockingHit);
					
				//UE_LOG(LogTemp, Warning, TEXT("hit blocking: %s"), *travelDirection.ToString());
				
					//if (outHits[0].bStartPenetrating)
					//UE_LOG(LogTemp, Warning, TEXT("outHits blocking"));
					//DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + hit.Normal * 100, 50, FColor::Magenta, false,5);
					//DrawDebugDirectionalArrow(GetWorld(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + travelDirection * 100, 50, FColor::Cyan, false, 5);
					//UE_LOG(LogTemp, Warning, TEXT("hi"));
				
				newVector = (newVector * DeltaTime * (1 - hit.Time)).Size() * (FVector::DotProduct(newVector.GetSafeNormal(),travelDirection)) * travelDirection;
				
				
				bMoveCompleted = Move(newVector, newRotation, hit);


				if (!bMoveCompleted && hit.bStartPenetrating)
				{

					FVector adjustment = GetPenetrationAdjustment(hit);

					ResolvePenetration(adjustment, hit, newRotation);
					DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + hit.Normal * 100, 50, FColor::Magenta, false, 5);
					bMoveCompleted = Move(newVector * DeltaTime, newRotation, hit);

				}

			}

		}
	}
	//Hit did not occur
	else
	{
		//Not Really sure what to put here.
	}

	//End Move.



	//Check if Fallingp------------------------------------------
	if (moveState == MOVE_STATE::WALKING)
	{
		//Here we are checking for ground.
		TArray<FHitResult> outHits;
		FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());
		
		GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() - FVector(0, 0, MAX_FLOOR_DIST), capsuleComponent->GetComponentQuat(), outParams);
	
		if(outHits.Num() > 0)
		{
			//Block did occur, now we must determine if it is ground.
			bool bGroundFound = false;
			for (int index = 0; index < outHits.Num(); index++)
			{
				
				//Check if it is ground.
				//if (outHits[index].ImpactPoint.Z < (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
				
				//double DistanceFromCenter = (outHits[index].ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
				//bool bIsInRange = DistanceFromCenter < (capsuleComponent->GetScaledCapsuleRadius());
				//if(bIsInRange)
				if (CutOff(outHits[index].ImpactPoint.Z, FLOOR_DETECTION_PERCISION) < CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, FLOOR_DETECTION_PERCISION))
				{
					// It is ground
					
					if (outHits[index].Distance > MIN_FLOOR_DIST)
					{
						UE_LOG(LogTemp, Warning, TEXT("closing Gap"));
						FHitResult myHit;
						float aveDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5;
						
						bool bMoveCompleted = Move( FVector(0.f, 0.f, MIN_FLOOR_DIST - outHits[index].Distance), capsuleComponent->GetComponentQuat(), myHit);
						
						if (!bMoveCompleted && myHit.bStartPenetrating)
						{

							FVector adjustment = GetPenetrationAdjustment(myHit);
							ResolvePenetration(adjustment, myHit, capsuleComponent->GetComponentQuat());
							bMoveCompleted = Move(FVector(0.f, 0.f, MIN_FLOOR_DIST - outHits[index].Distance), capsuleComponent->GetComponentQuat(), myHit);

						}
					}

					bGroundFound = true;
					break;
				}
				
			}

			if (!bGroundFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("Setting falling 1"));
				moveState = MOVE_STATE::FALLING;
			}
			else
			{
				velocity = FVector::ZeroVector;
			}
		}
		//nothing touhing us so we falls
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Setting falling 2"));
			moveState = MOVE_STATE::FALLING;
		}
	}
	
	inputVelocity = FVector::ZeroVector;
	rotationVelocity = FRotator(0, 0, 0);

	return true;
}


/* TODO: upgrade*/
bool UTacMoveComp::Move(const FVector& Delta, const FQuat& NewRotation, FHitResult & OutHit)
{
	OutHit.Reset();
	/*insert collision scan here*/
	bool bComplete = true;
	float time = 1.0f;
	TArray<FHitResult> outHits;
	FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());
	/*
	if (bIgnoreInitPenetration)
	{
		outParams.bFindInitialOverlaps = false;
	}
	*/

	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, NewRotation, outParams);
	if (outHits.Num() > 0)
	{
		if (outHits[0].bStartPenetrating)
		{
			UE_LOG(LogTemp, Warning, TEXT("Start pentraiting %f"), outHits[0].PenetrationDepth);
			OutHit = outHits[0];
			return false;

		}
	}
	
	
	
	/* Loop through, looking for a valid blocking hit. These hits must have normals facing
	 * the same direction as the velocity, then set the value of the outHit*/
	for (int i = 0; i < outHits.Num(); i++)
	{
	
		if (FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()) >= 0 )
		{
			//do nothing?
		}
		//Blocking hit
		else
		{
			
			//UE_LOG(LogTemp, Warning, TEXT("Hit Detected: %s , Time %f"), *(outHits[i].Actor->GetName()),outHits[i].Time );
			//UE_LOG(LogTemp, Warning, TEXT("Hit angle cos: %f , %f"), acos(FVector::DotProduct(outHits[i].Normal, Delta.GetSafeNormal())) * (180 / PI) , atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
			OutHit = outHits[i];
			time = outHits[i].Time;
			bComplete = false;
			break;
		}
		
	}

	if (bComplete)
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta));
	else
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) + (OutHit.Normal * TOUCH_TOLERANCE));
	
		//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time));
		//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) - (Delta.GetSafeNormal() * TOUCH_TOLERANCE));

	capsuleComponent->SetWorldRotation(NewRotation);

	return bComplete;
}


bool UTacMoveComp::ResolvePenetration(const FVector& proposedAdjustment, const FHitResult & hit, const FQuat & newRotation)
{
	//First we test the proposed location with overlap.
	FCollisionQueryParams QueryParams(FName(TEXT("dog")), false, capsuleComponent->GetOwner());
	FCollisionResponseParams ResponseParam;

	bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(hit.TraceStart + proposedAdjustment, newRotation, capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(RESOLVE_STRICTNESS), QueryParams, ResponseParam);
	if (!bOverlapping)
	{
		//no overlaps means we can resolve.
		capsuleComponent->SetWorldLocation(hit.TraceStart + proposedAdjustment);

	}
	else
	{
		
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


FVector UTacMoveComp::GetPenetrationAdjustment(const FHitResult & hit)
{
	if (!hit.bStartPenetrating)
	{
		return FVector::FVector(0, 0, 0);
	}
	const float penetrationDepth = (hit.PenetrationDepth > 0.f ? hit.PenetrationDepth : PENETRATE_ADITIONAL_SPACING);

	return hit.Normal * (penetrationDepth + PENETRATE_ADITIONAL_SPACING);
}


void UTacMoveComp::Initalize(UCapsuleComponent * CapCom)
{
	capsuleComponent = CapCom;
}


double UTacMoveComp::CutOff(double value, int place)
{
	int test = value * pow(10, place) * 10;
	double test2 = test / (10);
	test2 = test2 / pow(10, place);
	
	return test2;
}