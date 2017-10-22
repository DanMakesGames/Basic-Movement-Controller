// Fill out your copyright notice in the Description page of Project Settings.

#include "TacMoveComp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include <math.h>   


UTacMoveComp::UTacMoveComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	maxMoveSpeed = 150;
	maxRotationSpeed = 100;
	maxWalkableSlope = PI / 4.0;
	maxStepUpHeight = 20;
	gravity = -1200;

	moveState = MOVE_STATE::FALLING;

	FLOOR_DETECTION_PERCISION = 4;
	PENETRATE_ADITIONAL_SPACING = 0.125;
	RESOLVE_STRICTNESS = 0.1;
	TOUCH_TOLERANCE = 0.001f;
	
	MAX_FLOOR_DIST = 2.4;
	MIN_FLOOR_DIST = 1;

	GROUND_DETECT_RADIUS_TOLERANCE = 0.15;

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
	
	// Standard motion
	if (moveState == WALKING)
	{
		velocity += FVector(newRotation.RotateVector(inputVelocity).GetSafeNormal().X, newRotation.RotateVector(inputVelocity).GetSafeNormal().Y, -(GetGroundPlane() | newRotation.RotateVector(inputVelocity).GetSafeNormal()) / GetGroundPlane().Z).GetSafeNormal() * maxMoveSpeed;
	}
	// If falling apply downward motion.
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
		DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + GetGroundPlane() * 100, 4, FColor::Blue, false, 1);
		DrawDebugDirectionalArrow(GetWorld(),hit.ImpactPoint,hit.ImpactPoint + hit.ImpactNormal * 100,4, FColor::Green,false,1);

		//Fist see if we can preform a step up. 
		bool bDidStepUp = false;
		if (hit.ImpactNormal.Z == 0 && moveState == MOVE_STATE::WALKING)
		{
			UE_LOG(LogTemp, Warning, TEXT("Step Up Attempt"));
			bDidStepUp = PerformStepUp(moveDelta, hit);
		}
		if (!bDidStepUp)
		{
			double DistanceFromCenter = (hit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
			bool bIsInRange = DistanceFromCenter < FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE);
			if (bIsInRange)
			{
				//DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + hit.ImpactNormal * 100, 10, FColor::Green,false,5);
				////UE_LOG(LogTemp, Warning, TEXT("Is floor. Hitpoint: %f, %f"), DistanceFromCenter, FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE));
				//UE_LOG(LogTemp, Warning, TEXT("Is Floor hitpoint: %f, %f"), CutOff(hit.ImpactPoint.Z, 4), CutOff((capsuleComponent->GetComponentLocation() - capsuleComponent->GetUnscaledCapsuleHalfHeight_WithoutHemisphere()).Z, 4));
				//UE_LOG(LogTemp, Warning, TEXT("-----------------"));
				//Preform walkup onto slope

				if (moveState == MOVE_STATE::WALKING)
				{
			

					//Check if ground is not too sloped
					if (IsSlopeAngleValid(hit.ImpactNormal))
					{

						//FVector walkupDelta =  FVector::VectorPlaneProject( newVector * DeltaTime * (1 - hit.Time), hit.ImpactNormal );

						//FVector walkupDelta = FVector::VectorPlaneProject(FVector(newVector.X, newVector.Y, 0 ), hit.ImpactNormal).GetSafeNormal() * (newVector * DeltaTime * (1 - hit.Time)).Size();
						//FVector walkupDelta = FVector::VectorPlaneProject(FVector(newVector.X, newVector.Y, 0), hit.ImpactNormal).GetSafeNormal() * (newVector * DeltaTime * (1 - hit.Time)).Size();
						/*
						FVector flat = FVector(newVector.X, newVector.Y, 0);
						float dot = hit.ImpactNormal | flat;
						FVector rampMove = FVector(flat.X, flat.Y, -(dot  /  hit.ImpactNormal.Z));
						FVector walkupDelta = rampMove.GetSafeNormal() * (newVector * DeltaTime * (1.0f - hit.Time)).Size();
						*/
						if (IsSlopeAngleValid(hit.ImpactNormal))
							SetGroundPlane(hit.ImpactNormal);

						FVector flat = FVector(moveDelta.X, moveDelta.Y, 0);
						float dot = hit.ImpactNormal | flat;
						FVector rampMove = FVector(flat.X, flat.Y, -(dot / hit.ImpactNormal.Z));
						FVector walkupDelta = rampMove.GetSafeNormal() * (moveDelta * (1.0f - hit.Time)).Size();

						//FVector walkupDelta = FVector(newVector.X, newVector.Y, -(hit.ImpactNormal, newVector)) * (newVector * DeltaTime * (1 - hit.Time)).Size()
						//DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + walkupDelta * 100, 4, FColor::Orange, false, 10);
					//	DrawDebugDirectionalArrow(GetWorld(), hit.ImpactPoint, hit.ImpactPoint + newVector * 100, 10, FColor::Purple, false, 10);
						UE_LOG(LogTemp, Warning, TEXT("Walkup %f, %f"), walkupDelta.Size(), (walkupDelta.GetSafeNormal() | hit.ImpactNormal));
						FHitResult tempHit;
						if (!ResolveAndMove(walkupDelta, capsuleComponent->GetComponentQuat(), tempHit))
						{
							const FVector travelDirection = FVector::CrossProduct(tempHit.ImpactNormal, hit.ImpactNormal).GetSafeNormal();
							FVector slideVector = (walkupDelta * (1 - tempHit.Time)).Size() * (FVector::DotProduct(walkupDelta.GetSafeNormal(), travelDirection)) * travelDirection;
							//slideVector = FVector::VectorPlaneProject(slideVector, GetGroundPlane()).GetSafeNormal() * slideVector.Size();
							ResolveAndMove(slideVector, newRotation, tempHit);
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("not valid: %s"), *hit.ImpactNormal.ToString());
						if (moveState == MOVE_STATE::WALKING)
						{

							SlideAgainstWall(moveDelta, hit);
						}
					}
				}

				if (moveState == MOVE_STATE::FALLING)
				{
					if (IsSlopeAngleValid(hit.ImpactNormal))
					{
						SetGroundPlane(hit.ImpactNormal);
						moveState = MOVE_STATE::WALKING;
					}
					else {
						//move down the slope
						
						/*
						FVector heading = hit.ImpactNormal.GetSafeNormal2D();
						const float slopeAngle = FMath::Acos(FVector::DotProduct(hit.ImpactNormal, heading));
						const float deltaZ = -FMath::Sin(slopeAngle) *  (1.0f / FMath::Cos(slopeAngle));

						FVector travelNormal = FVector(heading.X, heading.Y,deltaZ).GetSafeNormal();
						
						*/
						//velocity += travelNormal * FMath::Cos(slopeAngle) * gravity * DeltaTime * (1 - hit.Time);
						//ResolveAndMove(travelNormal , capsuleComponent->GetComponentQuat(), slideDownHit);
						FHitResult slideDownHit;
						velocity -= FVector::DotProduct(hit.ImpactNormal, velocity.GetSafeNormal()) * velocity.Size() * hit.ImpactNormal;
						ResolveAndMove(velocity * (DeltaTime) * (1 - hit.Time), capsuleComponent->GetComponentQuat(), slideDownHit);
						
					}
				}
			}

			//Hit Object is not ground. This will need to be modified for slopes
			else
			{
				//if we hit something while falling remove that normal from the velocity
				if (moveState == FALLING)
				{
					UE_LOG(LogTemp, Warning, TEXT("Ding"));
					velocity -= FVector::DotProduct(hit.ImpactNormal, velocity.GetSafeNormal()) * velocity.Size() * hit.ImpactNormal;
				}

				//Sliding along wall behavior
				if (moveState == MOVE_STATE::WALKING)
				{

					SlideAgainstWall(moveDelta, hit);
					/*
					UE_LOG(LogTemp, Warning, TEXT("Not Ground. Sliding"));
					//FVector WallNormalHeading = FVector(hit.ImpactNormal.X, hit.ImpactNormal.Y, 0).GetSafeNormal;
					//const FVector travelDirection = FVector::CrossProduct(hit.ImpactNormal, FVector(0, 0, 1)).GetSafeNormal();
					const FVector travelDirection = FVector::CrossProduct(hit.ImpactNormal, GetGroundPlane()).GetSafeNormal();
					FVector slideVector = (newVector * DeltaTime * (1 - hit.Time)).Size() * (FVector::DotProduct(newVector.GetSafeNormal(),travelDirection)) * travelDirection;

					//slideVector = FVector::VectorPlaneProject(slideVector, GetGroundPlane()).GetSafeNormal() * slideVector.Size();
					DrawDebugDirectionalArrow(GetWorld(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + slideVector * 100, 4, FColor::Orange, false, 10);
					FHitResult slideHit;

					if (!ResolveAndMove(slideVector, newRotation, slideHit))
					{
						//UE_LOG(LogTemp, Warning, TEXT("                 continue slide"));

						double DistanceFromCenter = (slideHit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
						bool bIsInRange = DistanceFromCenter < FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE);
						if (bIsInRange)
						{
							if (IsSlopeAngleValid(slideHit.ImpactNormal))
								PerformWalkUp(slideVector, slideHit, &slideHit);
						}
						else
						{


							//THE SLIDE SLOPE MEGA FIX

							if(slideHit.Actor == hit.Actor && hit.ImpactNormal.Z != 0)
							{
								FVector correctedTravelDirection = FVector::CrossProduct(slideHit.ImpactNormal, GetGroundPlane()).GetSafeNormal();
								FVector correctedSlideVector = (newVector * DeltaTime * (1 - hit.Time)).Size() * (FVector::DotProduct(newVector.GetSafeNormal(), correctedTravelDirection)) * correctedTravelDirection;

								UE_LOG(LogTemp, Warning, TEXT("slide fix"));
								DrawDebugDirectionalArrow(GetWorld(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + correctedSlideVector * 100, 4, FColor::Purple, false, 10);

								if (!ResolveAndMove(correctedSlideVector, capsuleComponent->GetComponentQuat(), slideHit))
								{
									DistanceFromCenter = (slideHit.ImpactPoint - capsuleComponent->GetComponentLocation()).SizeSquared2D();
									bIsInRange = DistanceFromCenter < FMath::Square(capsuleComponent->GetScaledCapsuleRadius() - GROUND_DETECT_RADIUS_TOLERANCE);
									if (bIsInRange)
									{
										UE_LOG(LogTemp, Warning, TEXT("DOUBLE CORRECT: %s, %s"), *correctedSlideVector.ToString(), *correctedTravelDirection.ToString());
										if (IsSlopeAngleValid(slideHit.ImpactNormal))
											PerformWalkUp(correctedSlideVector, slideHit, &slideHit);
									}
								}
							}

						}

					}
					*/
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
					//DrawDebugDirectionalArrow(GetWorld(), outHits[index].ImpactPoint, outHits[index].ImpactPoint + GetGroundPlane() * 100, 10, FColor::Red, false, 10);
					// It is ground

					//If we are grounded, preform line trace to get the  normal of the ground we are on. If this line trace does not work, then use
					//the sweep impact normal as a last resort.
					/*
					FHitResult lineTraceHit;
					FCollisionObjectQueryParams lineObjectParams;
					lineObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
					lineObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);

					FCollisionQueryParams lineParams(TEXT("line trace"), false, GetOwner());
					lineParams.bFindInitialOverlaps = true;
					

					bool bDidHit = GetWorld()->LineTraceSingleByObjectType(lineTraceHit, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() - FVector(0, 0, 1000), lineObjectParams, lineParams);
					
					if (bDidHit)
					{
						if (lineTraceHit.Actor == outHits[index].Actor)
						{
							FVector lineTraceHeading(lineTraceHit.ImpactNormal.X, lineTraceHit.ImpactNormal.Y, 0);
							FVector outHitsHeading(outHits[index].ImpactNormal.X, outHits[index].ImpactNormal.Y, 0);

							if (FVector::DotProduct(lineTraceHeading.GetSafeNormal(), outHitsHeading.GetSafeNormal()) == -1)
							{
								if (IsSlopeAngleValid(lineTraceHit.ImpactNormal))
								{
									UE_LOG(LogTemp, Warning, TEXT("LINE NORMAL"));
									SetGroundPlane(lineTraceHit.ImpactNormal);
								}
							}
						}
						else
						{
							if (IsSlopeAngleValid(outHits[index].ImpactNormal))
							{
								UE_LOG(LogTemp, Warning, TEXT("ground normal: %s, %s"), *lineTraceHit.ImpactNormal.ToString(), *outHits[index].ImpactNormal.ToString());
								SetGroundPlane(outHits[index].ImpactNormal);
							}
						}
					}
					else if(!bDidHit) 
					{
						
						if (IsSlopeAngleValid(outHits[index].ImpactNormal))
						{
							UE_LOG(LogTemp, Warning, TEXT("WHAT:"));
							SetGroundPlane(outHits[index].ImpactNormal);
						}
					}
					
					if (!bDidHit)
					{
						UE_LOG(LogTemp, Warning, TEXT("WHAT:"));
					}

					*/
					/*
					if (lineTraceHit.ImpactNormal != outHits[index].ImpactNormal)
					{
						UE_LOG(LogTemp, Warning, TEXT("ground normal: %s, %s"), *lineTraceHit.ImpactNormal.ToString(), *outHits[index].ImpactNormal.ToString());
					}
					*/
					
					
					
					if (IsSlopeAngleValid(outHits[index].ImpactNormal))
						SetGroundPlane(outHits[index].ImpactNormal);
					

					//UE_LOG(LogTemp, Warning, TEXT("ground normal: %s"), *GetGroundPlane().ToString());
					
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
				UE_LOG(LogTemp, Warning, TEXT("Set Falling: Not Ground"));
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
			UE_LOG(LogTemp, Warning, TEXT("Set Falling: Nothing touching"));
			moveState = MOVE_STATE::FALLING;
		}
	}
	
	//Reset the input imformation in preperation for new imformation next tick.
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
	//outParams.bTraceComplex = true;
	capsuleComponent->SetWorldRotation(NewRotation);
	GetWorld()->ComponentSweepMulti(outHits, capsuleComponent, capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + Delta, NewRotation, outParams);
	
	//Do not move if the player started already penetrating
	if (outHits.Num() > 0)
	{
		if (outHits[0].bStartPenetrating)
		{
			UE_LOG(LogTemp, Warning, TEXT("Start Penetrating"));
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
	/*
	FVector flat = FVector(newVector.X, newVector.Y, 0);
	float dot = hit.ImpactNormal | flat;
	FVector rampMove = FVector(flat.X, flat.Y, -(dot / hit.ImpactNormal.Z));
	FVector walkupDelta = rampMove.GetSafeNormal() * (newVector * DeltaTime * (1.0f - hit.Time)).Size();
	FHitResult tempHit;
	if (!ResolveAndMove(walkupDelta, capsuleComponent->GetComponentQuat(), tempHit))
	*/
	FVector flatMovement = FVector(delta.X, delta.Y, 0);
	float groundFlatDeltaDot = slopeHit.ImpactNormal | flatMovement;
	FVector moveDirection = FVector(flatMovement.X, flatMovement.Y, -(groundFlatDeltaDot / slopeHit.ImpactNormal.Z));
	FVector walkUpDelta = moveDirection.GetSafeNormal() * (delta * (1.0f - slopeHit.Time)).Size();
	DrawDebugDirectionalArrow(GetWorld(), capsuleComponent->GetComponentLocation(), capsuleComponent->GetComponentLocation() + walkUpDelta * 1000, 4, FColor::Red, false, 10);
	if (outHit != NULL)
		return ResolveAndMove(walkUpDelta,capsuleComponent->GetComponentQuat(),*outHit);

	FHitResult tempHit;
		return ResolveAndMove(walkUpDelta, capsuleComponent->GetComponentQuat(), tempHit);
}


bool UTacMoveComp::SlideAgainstWall(const FVector& delta, const FHitResult& wallHit)
{
	//Check if we have a vertical normal
	FVector wallHeading = FVector(wallHit.ImpactNormal.X, wallHit.ImpactNormal.Y, 0).GetSafeNormal();
	//UE_LOG(LogTemp, Warning, TEXT("Vertical %s"), *wallHeading.ToString());
	/*
		//UE_LOG(LogTemp, Warning, TEXT("Vertical"));
		FVector traceStart = FVector(capsuleComponent->GetComponentLocation().X, capsuleComponent->GetComponentLocation().Y, wallHit.ImpactPoint.Z);
		FVector traceEnd = wallHit.ImpactPoint;
		DrawDebugDirectionalArrow(GetWorld(), traceStart, traceEnd, 4, FColor::Cyan, false, 10);
		FHitResult lineTraceHit;
		FCollisionObjectQueryParams lineObjectParams;
		lineObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
		lineObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);

		FCollisionQueryParams lineParams(TEXT("line trace"), false, GetOwner());
		lineParams.bFindInitialOverlaps = true;
		

		bool bDidHit = GetWorld()->LineTraceSingleByObjectType(lineTraceHit, traceStart, traceEnd + (traceEnd - traceStart).GetSafeNormal() * 1, lineObjectParams, lineParams);
		if (bDidHit && lineTraceHit.Actor == wallHit.Actor)
		{
			//DrawDebugDirectionalArrow(GetWorld(), lineTraceHit.ImpactPoint, , 4, FColor::Cyan, false, 10);
			//UE_LOG(LogTemp, Warning, TEXT("Vertical %s"), *FVector(lineTraceHit.ImpactNormal.X, lineTraceHit.ImpactNormal.Y, 0).GetSafeNormal().ToString(), *wallHeading.ToString());
			//wallHeading = lineTraceHit.ImpactNormal.GetSafeNormal2D();
			wallHeading = FVector(lineTraceHit.ImpactNormal.X, lineTraceHit.ImpactNormal.Y, 0).GetSafeNormal();
			if (wallHeading == FVector::ZeroVector)
			{
				return false;
			}
		}
		else
			return false;
		
	*/
	if (wallHeading == FVector::ZeroVector)
		return false;

	const FVector travelDirection = FVector::CrossProduct(wallHeading, GetGroundPlane()).GetSafeNormal();
	const FVector slideVector = (delta * (1 - wallHit.Time)).Size() * (FVector::DotProduct(delta.GetSafeNormal(), travelDirection)) * travelDirection;
	
	
	FHitResult slideHit;
	if (!ResolveAndMove(slideVector, capsuleComponent->GetComponentQuat(), slideHit))
	{
		//Wall Slide did not complete.
		//Reason 1: Could have hit a slope.
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

	const FVector traceStart(blockingHit.ImpactPoint.X, blockingHit.ImpactPoint.Y,blockingHit.ImpactPoint.Z + capsuleComponent->GetScaledCapsuleHalfHeight() * 2 + maxStepUpHeight);
	const FVector traceEnd(blockingHit.ImpactPoint.X, blockingHit.ImpactPoint.Y, blockingHit.ImpactPoint.Z - (capsuleComponent->GetScaledCapsuleHalfHeight() * 2 + maxStepUpHeight));
	//FCollisionQueryParams QueryParams(FName(TEXT("resolve penetration")), true, GetOwner());
	//DO LINE TRACE
	bool bDidHit = blockingHit.Component->SweepComponent(sweepHit, traceStart, traceEnd,capsuleComponent->GetComponentQuat(),capsuleComponent->GetCollisionShape());
	//bool bDidHit = blockingHit.Component->LineTraceComponent(sweepHit, traceStart, traceEnd, QueryParams);
	float distance = sweepHit.ImpactPoint.Z - (capsuleComponent->GetComponentLocation().Z - capsuleComponent->GetScaledCapsuleHalfHeight());

	if (bDidHit && (distance <= maxStepUpHeight) && (distance >= 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("CanStepUP"));
		//Can step up.
		//Now we need to make sure space exists large enough for the player to teleport to.
		// First perform a sweep to see how high up the ledge is.
		FCollisionQueryParams QueryParams(FName(TEXT("resolve penetration")), true, GetOwner());
		FCollisionResponseParams ResponseParam;
		FVector destination = FVector(sweepHit.ImpactPoint.X, sweepHit.ImpactPoint.Y,sweepHit.ImpactPoint.Z + capsuleComponent->GetScaledCapsuleHalfHeight() + MIN_FLOOR_DIST);
		bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(destination, capsuleComponent->GetComponentQuat(), capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(), QueryParams, ResponseParam);
		if (!bOverlapping)
		{
			
			capsuleComponent->SetWorldLocation(destination);
			FHitResult moveHit;
			ResolveAndMove(delta * (1 - blockingHit.Time), capsuleComponent->GetComponentQuat(), moveHit);
			return true;
		}
	}

	return false;
}

//TODO: This function needs to be enhanced to full function, in particualr handling resolving penetraing two objects.
bool UTacMoveComp::ResolvePenetration(const FVector& proposedAdjustment, const FHitResult& hit, const FQuat& newRotation)
{
	//First we test the proposed location with overlap.
	FCollisionQueryParams QueryParams(FName(TEXT("resolve penetration")),true, GetOwner());
	FCollisionResponseParams ResponseParam;
	capsuleComponent->InitSweepCollisionParams(QueryParams, ResponseParam);
	bool bOverlapping = GetWorld()->OverlapBlockingTestByChannel(hit.TraceStart + proposedAdjustment, newRotation, capsuleComponent->GetCollisionObjectType(), capsuleComponent->GetCollisionShape(RESOLVE_STRICTNESS), QueryParams, ResponseParam);
	
	if (!bOverlapping)
	{
		//no overlaps means we can resolve.
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