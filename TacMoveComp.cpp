// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   

// Sets default values for this component's properties
UTacMoveComp::UTacMoveComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 100;
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
		velocity = inVelocity;
	}
}

//FVector GetVelocity();
FVector UTacMoveComp::GetVelocity()
{
	return velocity;
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
	
	FQuat newRotation =capsuleComponent->GetComponentQuat() * (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion();
	//Standard motion
	if (moveState == WALKING)
	{
		//UE_LOG(LogTemp, Warning, TEXT("WALKING"));
		newVector = newRotation.RotateVector(velocity.GetSafeNormal());
		newVector.Z = 0;
	}
	//if falling apply downward motion.
	else if (moveState == FALLING)
	{
		//UE_LOG(LogTemp, Warning, TEXT("FALLING"));
		newVector = velocity + FVector(0, 0, -100);
	}
	//FVector newVector = velocity;
	

	//--Move---------------------
	//Hit did occur
	if ( !Move(newVector * DeltaTime * maxMoveSpeed, (rotationVelocity * DeltaTime * maxRotationSpeed).Quaternion(), hit ) )
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
	velocity = FVector(0, 0, 0);
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
			UE_LOG(LogTemp, Warning, TEXT("Correction"));
			capsuleComponent->SetWorldLocation(outHits[0].ImpactPoint + outHits[0].ImpactNormal * outHits[0].PenetrationDepth);
		}
	}
	
	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, capsuleComponent->GetComponentQuat() * NewRotation, outParams);


	/* Loop through, looking for a valid blocking hit. These hits must have normals facing
	 * the same direction as the velocity, then set the value of the outHit*/
	for (int i = 0; i < outHits.Num(); i++)
	{
		//DrawDebugDirectionalArrow(GetWorld(), outHits[i].ImpactPoint, outHits[i].ImpactPoint + outHits[i].ImpactNormal * 100, 10, FColor::Red,false, 100,100,10);
		//DrawDebugLine(GetWorld(), outHits[i].ImpactPoint, outHits[i].ImpactPoint + outHits[i].ImpactNormal * 100, FColor::Cyan,false, 100 , 100, 10);
		//No-Blocking hit
		//UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %f , %f"), outHits[i].PenetrationDepth,acos(FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal())) * (180 / PI), atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
		//UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %f , %f"), outHits[i].PenetrationDepth,FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()), atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
		//UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %s , %s"), outHits[i].PenetrationDepth, *outHits[i].Normal.ToString(), *Delta.GetSafeNormal().ToString());
		UE_LOG(LogTemp, Warning, TEXT("Hit angle dep:%f, cos %f , %f"), outHits[i].PenetrationDepth,FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()), atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
		//if ( acos(FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) <= (PI / 2.0f) )
		//if (atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) <= (PI / 2.0f))
		//if (acos(FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal())) <= (PI / 2.0f))
		if (FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetSafeNormal()) >= 0 )
		{
			//do nothing?
			UE_LOG(LogTemp, Warning, TEXT("LESS THAN"));
		}
		//Blocking hit
		else
		{
			/*
			ENGINE_API void DrawDebugPoint(
				const UWorld* InWorld,
				FVector const& Position,
				float Size,
				FColor const& PointColor,
				bool bPersistentLines = false,
				float LifeTime = -1.f,
				uint8 DepthPriority = 0
			);
			*/
			//DrawDebugPoint(GetWorld(), outHits[i].ImpactPoint ,50,FColor::Cyan,false, -1);
			//DrawDebugCone();
			
			//UE_LOG(LogTemp, Warning, TEXT("Hit Detected: %s , Time %f"), *(outHits[i].Actor->GetName()),outHits[i].Time );
			//UE_LOG(LogTemp, Warning, TEXT("Hit angle cos: %f , %f"), acos(FVector::DotProduct(outHits[i].Normal, Delta.GetSafeNormal())) * (180 / PI) , atan2(FVector::CrossProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal()).Size(), FVector::DotProduct(outHits[i].ImpactNormal, Delta.GetUnsafeNormal())) * (180 / PI));
			OutHit = outHits[i];
			time = outHits[i].Time;
			bComplete = false;
			break;
		}
		
	}

	//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) - (Delta.GetSafeNormal() * 0.0001f));
	capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) + OutHit.ImpactNormal * 0.001f);
	//capsuleComponent->SetWorldLocation(capsuleComponent->GetComponentLocation() + (Delta * time) );

	capsuleComponent->SetWorldRotation(capsuleComponent->GetComponentQuat() * NewRotation);

	return bComplete;
}


void UTacMoveComp::Initalize(UCapsuleComponent * CapCom)
{
	capsuleComponent = CapCom;
}