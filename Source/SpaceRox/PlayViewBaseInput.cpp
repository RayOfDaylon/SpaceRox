// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Constants.h"

/*
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "UDaylonParticlesWidget.h"
#include "UMG/Public/Blueprint/WidgetLayoutLibrary.h"
#include "UMG/Public/Blueprint/WidgetBlueprintLibrary.h"
#include "UMG/Public/Animation/WidgetAnimation.h"
#include "Runtime/Core/Public/Logging/LogMacros.h"
*/

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);


// Set to 1 to enable debugging
#define DEBUG_MODULE                0



#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



void UPlayViewBase::OnAbortButtonPressed()
{
	if(PlayerShip)
	{
		PlayerShip->ReleaseResources();
		Daylon::Uninstall(PlayerShip);
		PlayerShip.Reset();
	}

	TransitionToState(EGameState::MainMenu);
}


void UPlayViewBase::OnBackButtonPressed()
{
	NavigateMenu(Daylon::EListNavigationDirection::Backwards);
}


void UPlayViewBase::OnForwardButtonPressed()
{
	NavigateMenu(Daylon::EListNavigationDirection::Forwards);
}


void UPlayViewBase::OnStartButtonPressed()
{
	if(GameState != EGameState::Active)
	{
		PlaySound(ForwardSound);
	}

	switch(GameState)
	{
		case EGameState::Intro:

			TransitionToState(EGameState::MainMenu);
			break;

	
		case EGameState::MainMenu:

			GetOwningPlayer()->DisableInput(nullptr);
			PlayAnimation(MenuOutro, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
			break;


		case EGameState::HighScores:
	
			if(bHighScoreWasEntered)
			{
				// hack: when we set the input mode to game only after high score entry,
				// the Enter keypress used to enter the score gets interpreted by the 
				// high scores screen to exit to the main menu. So consume that input
				// so the screen doesn't exit immediately.
				bHighScoreWasEntered = false;
				return;
			}
			UDaylonUtils::Hide(HighScoresContent);
			TransitionToState(EGameState::MainMenu);
			break;


		case EGameState::HighScoreEntry:

			// Don't need to call OnEnterHighScore; widget input event will do so.
			break;


		case EGameState::Credits:

			UDaylonUtils::Hide(CreditsContent);
			TransitionToState(EGameState::MainMenu);
			break;


		case EGameState::Help:

			UDaylonUtils::Hide(HelpContent);
			TransitionToState(EGameState::MainMenu);
			break;

		default:
			UE_LOG(LogGame, Error, TEXT("Unknown or unhandled GameState %d"), (int32)GameState);
			break;
	}
}


void UPlayViewBase::OnFireTorpedo()
{
	// Called when the 'fire torpedo' button is pressed.

	if(!IsPlayerShipPresent())
	{
		return;
	}

	PlayerShip->FireTorpedo();
}



#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
