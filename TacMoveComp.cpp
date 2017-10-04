// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


#define TOUCH_TOLERANCE 0.001f // Distance we set from an object after a blocking collision
//#define TOUCH_TOLERANCE 1// Distance we set from an object after a blocking collision
#define MAX_FLOOR_DIST 1 // greatest distance we can be from the floor before we consider it falling
// Sets default values for this component's properties

UTacMoveComp::UTacMoveComp()
{

	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 70;
	maxRotationSpeed = 100;
	moveState = MOVE_STATE::FALLING;

	FLOOR_DETECTION_PERCISION = 4;
	//moveState = MOVE_STATE::WALKING;

	PENETRATE_ADITIONAL_SPACING = 0.125f; 
	
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

	

	//--Move---------------------
	//Hit did occur
	if ( !Move(newVector * DeltaTime, (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion(), hit ) )
	{
		
		//Evaluate if the ground is good enough to walk on:
		//yes this is ground

		//UE_LOG(LogTemp, Warning, TEXT("Is Floor time: My Method %f, %f"), (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z, (capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z);
		//if (hit.ImpactPoint.Z < (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
		//if (hit.ImpactPoint.Z < (capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z)
		
		
		if (CutOff(hit.ImpactPoint.Z, 4) < CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4))
		{

			UE_LOG(LogTemp, Warning, TEXT("Is Floor hitpoint: %f, %f"), CutOff(hit.ImpactPoint.Z, 4), CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4));
			DrawDebugPoint(GetWorld(),hit.ImpactPoint,50,FColor::Cyan,false,10);
			if (moveState == MOVE_STATE::FALLING)
			{
				
				moveState = MOVE_STATE::WALKING;
			}
		}
		// Hit Object is not ground. This will need to be modified for slopes
		else
		{
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
					DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + hit.ImpactNormal * 100, 50, FColor::Magenta, false,5);
					DrawDebugDirectionalArrow(GetWorld(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + travelDirection * 100, 50, FColor::Cyan, false, 5);
					//UE_LOG(LogTemp, Warning, TEXT("hi"));
				
				newVector = (newVector * DeltaTime * (1 - hit.Time)).Size() * (FVector::DotProduct(newVector.GetSafeNormal(),travelDirection)) * travelDirection;
				
				
				Move(newVector, (rotationVelocity * DeltaTime * maxRotationSpeed * (1 - hit.Time)).Quaternion(), hit);
			}

		}
	}
	//Hit did not occur
	else
	{
		//Not Really sure what to put here.
	}

	//Check if Falling.
	if (moveState == MOVE_STATE::WALKING)
	{
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
				if (CutOff(outHits[index].ImpactPoint.Z, 4) < CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4))
				{
					// It is ground
					bGroundFound = true;
					break;
				}
				
			}

			if (!bGroundFound)
			{
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
	/* First do a cylinder cast out then get some sort of hit. Move up to that hit*/

	/*insert collision scan here*/
	bool bComplete = true;
	float time = 1.0f;
	TArray<FHitResult> outHits;
	FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());

	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, capsuleComponent->GetComponentQuat() * NewRotation, outParams);
	if (outHits.Num() > 0)
	{
		if (outHits[0].bStartPenetrating)
		{
		    
			//capsuleComponent->SetWorldLocation(outHits[0].ImpactPoint + (outHits[0].Normal * outHits[0].PenetrationDepth) + (outHits[0].Normal * 0.5));
			capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + GetPenetrationAdjustment(outHits[0]));
			

		}
	}
	
	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, capsuleComponent->GetComponentQuat() * NewRotation, outParams);
	
	/* Loop through, looking for a valid blocking hit. These hits must have normals facing
	 * the same direction as the velocity, then set the value of the outHit*/
	for (int i = 0; i < outHits.Num(); i++)
	{
		
		//UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %f , %f"), outHits[i].PenetrationDepth,FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()), atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
		if(outHits[i].bStartPenetrating)

			UE_LOG(LogTemp, Warning, TEXT("Still Penetrating Time %f"), outHits[i].Time);


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

	if(bComplete)
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time));
	else
		capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) + (OutHit.Normal * TOUCH_TOLERANCE));
		//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time));
		//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) - (Delta.GetSafeNormal() * TOUCH_TOLERANCE));

	capsuleComponent->SetWorldRotation(capsuleComponent->GetComponentQuat() * NewRotation);

	return bComplete;
}

bool UTacMoveComp::ResolvePenetration(const FVector& proposedAdjustment, const FHitResult & hit, const FQuat & newRotation)
{
	return true;
}



FVector UTacMoveComp::GetPenetrationAdjustment(const FHitResult & hit)
{
	if (!hit.bStartPenetrating)
		return FVector::FVector(0,0,0);

	const float penetrationDepth = hit.PenetrationDepth > 0.f ? hit.PenetrationDepth : PENETRATE_ADITIONAL_SPACING;

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