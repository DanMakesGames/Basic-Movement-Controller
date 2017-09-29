// Fill out your copyright notice in the Description page of Project Settings.

#include "TestGameMode.h"
#include "TestPawn.h"
#include "TestController.h"


ATestGameMode::ATestGameMode()
{
	DefaultPawnClass = ATestPawn::StaticClass();
	PlayerControllerClass = ATestController::StaticClass();
}
