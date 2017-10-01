// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


#define TOUCH_TOLERANCE 0.001f // Distance we set from an object after a blocking collision
#define MAX_FLOOR_DIST 1 // greatest distance we can be from the floor before we consider it falling
// Sets default values for this component's properties
UTacMoveComp::UTacMoveComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 70;
	maxRotationSpeed = 100;
	moveState = MOVE_STATE::FALLING;
	//moveState = MOVE_STATE::WALKING;
	

	// ...
}


// Called when the game starts
void UTacMoveComp::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTacMoveComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	performMovement(DeltaTime);


}

//void SetVelocity(const FVector& inVelocity);
void UTacMoveComp::SetVelocity(const FVector& inVelocity)
{
	//UE_LOG(LogTemp, Warning, TEXT("MoveForward: %f"), val);
	//UE_LOG(LogTemp, Warning, TEXT("Mii %s"), *inVelocity.ToString());
	//UE_LOG(LogTemp, Warning, TEXT(inVelocity.ToString()));
	if (moveState == MOVE_STATE::WALKING)
	{
		inputVelocity = inVelocity;
	}
}

//FVector GetVelocity();
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
	//SHould I apply rotation first or second
	//UE_LOG(LogTemp, Warning, TEXT("Velocit: %s"), *velocity.ToString());
	FQuat newRotation =capsuleComponent->GetComponentQuat() * (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion();
	//Standard motion
	if (moveState == WALKING)
	{

		newVector = velocity + newRotation.RotateVector(inputVelocity.GetSafeNormal()) * maxMoveSpeed;
		velocity = newVector;

	}
	//if falling apply downward motion.
	else if (moveState == FALLING)
	{
		
		newVector = velocity + FVector(0, 0, -0.5) * maxMoveSpeed;

	}

	

	//--Move---------------------
	//Hit did occur
	if ( !Move(newVector * DeltaTime, (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion(), hit ) )
	{
		//Evaluate if the ground is good enough to walk on:
		//yes this is ground
		if (hit.ImpactPoint.Z <= (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
		{
			if (moveState == MOVE_STATE::FALLING)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Setting Walking"));
				moveState = MOVE_STATE::WALKING;
			}
		}
	}
	//Hit did not occur
	else
	{
		//Not Really sure what to put here.
	}


	/* Test if grounded here. Just test if the object is touching anything*/
	
	/*
	if (moveState == MOVE_STATE::WALKING)
	{
		if (GetWorld()->SweepSingleByChannel(hit, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, capsuleComponent->GetCollisionShape(TOUCH_TOLERANCE)))
		{
			//Block did occur, now we must determine if it is ground.
			if (hit.ImpactPoint.Z > (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
			{
				UE_LOG(LogTemp, Warning, TEXT("Set falling : %s"), *hit.Actor->GetName());
				DrawDebugPoint(GetWorld(), hit.ImpactPoint - FVector(0,0,10),50,FColor::Cyan,false, 3);
				//moveState = MOVE_STATE::FALLING;
			}
		}
	}
	*/
	/*
	if (moveState == MOVE_STATE::WALKING)
	{
		TArray<FOverlapResult> outOverlaps;
		if (GetWorld()->OverlapMultiByObjectType(outOverlaps, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, capsuleComponent->GetCollisionShape(TOUCH_TOLERANCE)))
		{
			//Block did occur, now we must determine if it is ground.
			if (hit.ImpactPoint.Z > (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
			{
				moveState = MOVE_STATE::FALLING;
			}
		}
	}
	*/

	if (moveState == MOVE_STATE::WALKING)
	{
		TArray<FHitResult> outHits;
		FComponentQueryParams outParams(FName(TEXT("DUDE")), GetOwner());
		//if (GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() - FVector(0,0,TOUCH_TOLERANCE), capsuleComponent->GetComponentQuat(), outParams))
		GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() - FVector(0, 0, MAX_FLOOR_DIST), capsuleComponent->GetComponentQuat(), outParams);
	
		if(outHits.Num() > 0)
		{
			
			//Block did occur, now we must determine if it is ground.
			if (outHits[0].ImpactPoint.Z > (capsuleComponent->GetComponentLocation() - (capsuleComponent->GetScaledCapsuleHalfHeight() - capsuleComponent->GetScaledCapsuleRadius())).Z)
			{
				UE_LOG(LogTemp, Warning, TEXT("Not Ground: Falling"));
				DrawDebugPoint(GetWorld(), hit.ImpactPoint - FVector(0, 0, 10), 50, FColor::Cyan, false, 3);
				moveState = MOVE_STATE::FALLING;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Touching Ground"));
				velocity = FVector(0, 0, 0);

			}
		}
		//nothing touhing us so we falls
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Touching nothing: Falling"));
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
		
			capsuleComponent->SetWorldLocation(outHits[0].ImpactPoint + outHits[0].ImpactNormal * outHits[0].PenetrationDepth);
		}
	}
	
	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, capsuleComponent->GetComponentQuat() * NewRotation, outParams);


	/* Loop through, looking for a valid blocking hit. These hits must have normals facing
	 * the same direction as the velocity, then set the value of the outHit*/
	for (int i = 0; i < outHits.Num(); i++)
	{
		
		//UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %f , %f"), outHits[i].PenetrationDepth,FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()), atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
	
		if (FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()) >= 0 )
		{
			//do nothing?
			//UE_LOG(LogTemp, Warning, TEXT("LESS THAN"));
		}
		//Blocking hit
		else
		{
	
			//DrawDebugPoint(GetWorld(), outHits[i].ImpactPoint ,50,FColor::Cyan,false, -1);
			
			//UE_LOG(LogTemp, Warning, TEXT("Hit Detected: %s , Time %f"), *(outHits[i].Actor->GetName()),outHits[i].Time );
			//UE_LOG(LogTemp, Warning, TEXT("Hit angle cos: %f , %f"), acos(FVector::DotProduct(outHits[i].Normal, Delta.GetSafeNormal())) * (180 / PI) , atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
			OutHit = outHits[i];
			time = outHits[i].Time;
			bComplete = false;
			break;
		}
		
	}

	//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) - (Delta.GetSafeNormal() * 0.0001f));
	capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) + OutHit.ImpactNormal * TOUCH_TOLERANCE);
	//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) );

	capsuleComponent->SetWorldRotation(capsuleComponent->GetComponentQuat() * NewRotation);


	return bComplete;
}


void UTacMoveComp::Initalize(UCapsuleComponent * CapCom)
{
	capsuleComponent = CapCom;
}