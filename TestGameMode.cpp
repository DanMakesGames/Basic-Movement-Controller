/**
* Written by Daniel Mann.
* created in 2017
* DanielMannGames@outlook.com
*/

#include "TestGameMode.h"
#include "TestPawn.h"
#include "TestController.h"


ATestGameMode::ATestGameMode()
{
	DefaultPawnClass = ATestPawn::StaticClass();
	PlayerControllerClass = ATestController::StaticClass();
}
