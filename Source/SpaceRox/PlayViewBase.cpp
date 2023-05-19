// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "DaylonUtils.h"
#include "UDaylonParticlesWidget.h"
#include "Constants.h"

#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "UMG/Public/Components/CanvasPanel.h"
#include "UMG/Public/Components/CanvasPanelSlot.h"
#include "UMG/Public/Components/HorizontalBoxSlot.h"
#include "UMG/Public/Components/VerticalBoxSlot.h"
#include "UMG/Public/Components/GridPanel.h"
#include "UMG/Public/Components/GridSlot.h"
#include "UMG/Public/Blueprint/WidgetTree.h"
#include "UMG/Public/Blueprint/WidgetLayoutLibrary.h"
#include "UMG/Public/Blueprint/WidgetBlueprintLibrary.h"
#include "UMG/Public/Animation/WidgetAnimation.h"
#include "Runtime/MovieScene/Public/MovieScene.h"
#include "Runtime/GeometryCore/Public/BoxTypes.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

DEFINE_LOG_CATEGORY(LogGame)

// Set to 1 to enable debugging
#define DEBUG_MODULE                0

// Make wave start with three rocks (big/medium/small) at rest.
#define TEST_ASTEROIDS              0

#define FEATURE_SPINNING_ASTEROIDS  1

#define FEATURE_MULTIPLE_ENEMIES    1

#define FEATURE_SCAVENGERS          1


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



static FVector2D MakeInertia(const FVector2D& InertiaOld, float MinDeviation, float MaxDeviation)
{
	FVector2D NewInertia = UDaylonUtils::Rotate(InertiaOld, FMath::RandRange(MinDeviation, MaxDeviation));

	return NewInertia;
}


FVector2D UPlayViewBase::WrapPositionToViewport(const FVector2D& P)
{
	return FVector2D(UKismetMathLibrary::FWrap(P.X, 0.0, ViewportSize.X), UKismetMathLibrary::FWrap(P.Y, 0.0, ViewportSize.Y));
}


void UPlayViewBase::PreloadSound(USoundBase* Sound)
{
	// hack: preload sound by playing it at near-zero volume.

	PlaySound(Sound, 0.01f);
}


void UPlayViewBase::PreloadSounds()
{
	PreloadSound(ErrorSound               );
	PreloadSound(PlayerShipDestroyedSound );
	PreloadSound(PlayerShipBonusSound     );
	PreloadSound(ThrustSound              );
	PreloadSound(TorpedoSound             );
	PreloadSound(DoubleTorpedoSound       );
	PreloadSound(GainDoubleGunPowerupSound);
	PreloadSound(GainShieldPowerupSound   );
	PreloadSound(ShieldBonkSound          );
	PreloadSound(EnemyShipSmallSound      );
	PreloadSound(EnemyShipBigSound        );
	PreloadSound(MenuItemSound            );
	PreloadSound(ForwardSound             );

	for(auto Sound : ExplosionSounds)
	{
		PreloadSound(Sound);
	}
}


void UPlayViewBase::InitializeScore()
{
	PlayerScore = (StartingScore > 0 ? StartingScore : 0);

	UpdatePlayerScoreReadout();
}


void UPlayViewBase::InitializePlayerShipCount()
{
	NumPlayerShips = 0;

	// Clear out any content from the PlayerShipsReadout, to be safe.
	PlayerShipsReadout->ClearChildren();

	AddPlayerShips(InitialPlayerShipCount);
}


void UPlayViewBase::CreatePlayerShip()
{
	PlayerShip = FPlayerShip::Create(PlayerShipAtlas, FVector2D(32), 0.4f);

	InitializePlayerShip();
}


void UPlayViewBase::InitializePlayerShip()
{
	PlayerShip->IsUnderThrust      = false;
	PlayerShip->IsSpawning         = false;
	PlayerShip->DoubleShotsLeft    = bGodMode ? 10000 : 0;
	PlayerShip->ShieldsLeft        = 0.0f;
	PlayerShip->InvincibilityLeft  = 0.0f;

	PlayerShip->Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	PlayerShip->Hide   ();
}


void UPlayViewBase::InitializePlayerDefenses()
{
	PlayerShield = SNew(Daylon::SpritePlayObject2D);

	Daylon::FinishCreating<SDaylonSprite>(PlayerShield, 0.5f);

	PlayerShield->SetAtlas(DefensesAtlas->Atlas);
	PlayerShield->SetCurrentCel(ShieldDefenseAtlasCel);
	PlayerShield->SetSize(DefensesAtlas->Atlas.GetCelPixelSize());
	PlayerShield->UpdateWidgetSize();

	PlayerShield->Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	PlayerShield->Hide   ();


	PlayerInvincibilityShield = SNew(Daylon::SpritePlayObject2D);

	Daylon::FinishCreating<SDaylonSprite>(PlayerInvincibilityShield, 0.5f);

	PlayerInvincibilityShield->SetAtlas(DefensesAtlas->Atlas);
	PlayerInvincibilityShield->SetCurrentCel(InvincibilityDefenseAtlasCel);
	PlayerInvincibilityShield->SetSize(DefensesAtlas->Atlas.GetCelPixelSize());
	PlayerInvincibilityShield->UpdateWidgetSize();
	PlayerInvincibilityShield->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);
	PlayerInvincibilityShield->Hide();
}


void UPlayViewBase::CreateTorpedos()
{
	for(int32 Index = 0; Index < TorpedoCount; Index++)
	{
		//auto TorpedoPtr = FTorpedo::Create(TorpedoBrush, 0.5f);
		auto TorpedoPtr = FTorpedo::Create(TorpedoAtlas, 0.5f);

		TorpedoPtr->Inertia = FVector2D(0);
		TorpedoPtr->LifeRemaining = 0.0f;
		TorpedoPtr->Hide();

		Torpedos.Add(TorpedoPtr);
	}
}


void UPlayViewBase::InitializeVariables()
{
	bEnemyShootsAtPlayer          = false;
	bHighScoreWasEntered          = false;

	PlayerScore                   = 0;
	WaveNumber                    = 0;

	NumSmallEnemyShips            = 0;
	NumBigEnemyShips              = 0;

	ThrustSoundTimeRemaining      = 0.0f;
	StartMsgAnimationAge          = 0.0f;
	TimeUntilNextWave             = 0.0f;
	TimeUntilNextPlayerShip       = 0.0f;
	TimeUntilNextEnemyShip        = 0.0f;
	TimeUntilGameOverStateEnds    = 0.0f;
	MruHighScoreAnimationAge      = 0.0f;
	TimeUntilNextScavenger        = 5.0f;

	GameState                     = EGameState::Startup;
	SelectedMenuItem              = EMenuItem::StartPlaying;
}


void UPlayViewBase::InitializeAtlases()
{
	PlayerShipAtlas           -> Atlas.InitCache();
	LargeRockAtlas            -> Atlas.InitCache();
	MediumRockAtlas           -> Atlas.InitCache();
	SmallRockAtlas            -> Atlas.InitCache();
	BigEnemyAtlas             -> Atlas.InitCache();
	SmallEnemyAtlas           -> Atlas.InitCache();
	DefensesAtlas             -> Atlas.InitCache();
	DoubleGunsPowerupAtlas    -> Atlas.InitCache();
	ShieldPowerupAtlas        -> Atlas.InitCache();
	InvincibilityPowerupAtlas -> Atlas.InitCache();
	ScavengerAtlas            -> Atlas.InitCache();
	TorpedoAtlas              -> Atlas.InitCache();
}


void UPlayViewBase::InitializeSoundLoops()
{
	PlayerShipThrustSoundLoop.Set (this, ThrustSound);
	BigEnemyShipSoundLoop.Set     (this, EnemyShipBigSound);
	SmallEnemyShipSoundLoop.Set   (this, EnemyShipSmallSound);
}


void UPlayViewBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	IsInitialized = false;

	if(RootCanvas == nullptr)
	{
		StopRunning(TEXT("Cannot get canvas"), ReasonIsFatal);
		return;
	}

	UDaylonUtils::SetWidgetTree(WidgetTree);
	UDaylonUtils::SetRootCanvas(RootCanvas);

	// Note: UMG Canvas will not have its SConstraintCanvas member populated until later.

	PreloadSounds          ();

	InitializeVariables    ();
	InitializeAtlases      ();
	InitializeSoundLoops   ();


	Asteroids.Reserve(MaxInitialAsteroids * 4);

	TransitionToState(EGameState::Intro);

	LoadHighScores();

	IsInitialized = true;
}


void UPlayViewBase::UpdateMenuReadout()
{
	// Show selected menu item highlighted, all others plain.

	if(MenuContent == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("MenuContent is nullptr"));
		return;
	}

	for(int32 Index = 0; Index < MenuContent->GetChildrenCount(); Index++)
	{
		auto MenuItemWidget = Cast<UTextBlock>(MenuContent->GetChildAt(Index));

		if(MenuItemWidget != nullptr)
		{
			MenuItemWidget->SetOpacity(0.5f);
		}
	}

	auto MenuItemWidget = Cast<UTextBlock>(MenuContent->GetChildAt((int32)SelectedMenuItem));

	if(MenuItemWidget == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("MenuItemWidget is nullptr"));
		return;
	}

	MenuItemWidget->SetOpacity(1.0f);
}


void UPlayViewBase::NavigateMenu(Daylon::EListNavigationDirection Direction)
{
	if(GameState != EGameState::MainMenu)
	{
		UE_LOG(LogGame, Error, TEXT("GameState is not MainMenu"));
		return;
	}

	PlaySound(MenuItemSound);

	auto N = (int32)Direction + (int32)SelectedMenuItem;

	N = (N + 1000) % (int32)EMenuItem::Count;

	SelectedMenuItem = (EMenuItem)N;

	UpdateMenuReadout();
}



void UPlayViewBase::OnAbortButtonPressed()
{
	if(PlayerShip)
	{
		Daylon::Destroy(PlayerShip);
		PlayerShip.Reset();
	}

	if(PlayerShield)
	{
		Daylon::Destroy(PlayerShield);
		PlayerShield.Reset();
	}

	if(PlayerInvincibilityShield)
	{
		Daylon::Destroy(PlayerInvincibilityShield);
		PlayerInvincibilityShield.Reset();
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


void UPlayViewBase::OnAimPlayerShip(const FVector2D& Direction)
{
	// Aiming with a gamepad joystick doesn't really work that well; 
	// one keeps expecting it to also include thrust. We could do 
	// that, so we may revisit this as a todo item.

	if(GameState == EGameState::Active)
	{
		PlayerShip->SetAngle(UDaylonUtils::Vector2DToAngle(Direction));
	}
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


void UPlayViewBase::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	if(Animation == MenuOutro)
	{
		UDaylonUtils::Hide(MenuContent);
		ExecuteMenuItem(SelectedMenuItem);
		GetOwningPlayer()->EnableInput(nullptr);
	}
}


void UPlayViewBase::OnEnterHighScore(const FString& Name)
{
	FString Str = Name.TrimStartAndEnd();

	if(Str.IsEmpty() || Str.Len() > HighScores.MaxNameLength)
	{
		// Stay inside high score entry screen until name has valid length.
		PlaySound(ErrorSound);

		// Keep the entry field focused.
		HighScoreNameEntry->SetFocus();
		return;
	}
	
	// Keep the focus on the game window
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(GetOwningPlayer(), true);

	bHighScoreWasEntered = true;

	MostRecentHighScore.Set(PlayerScore, Str);

	HighScores.Add(PlayerScore, Str);

	SaveHighScores();

	TransitionToState(EGameState::HighScores);
}


void UPlayViewBase::ExecuteMenuItem(EMenuItem Item)
{
	switch(Item)
	{
		case EMenuItem::StartPlaying:    TransitionToState(EGameState::Active);      break;
		case EMenuItem::ShowHighScores:  TransitionToState(EGameState::HighScores);  break;
		case EMenuItem::ShowHelp:        TransitionToState(EGameState::Help);        break;
		case EMenuItem::ShowCredits:     TransitionToState(EGameState::Credits);     break;

		case EMenuItem::Exit:            StopRunning(TEXT("Player exited program")); break;

		default:
			UE_LOG(LogGame, Error, TEXT("Unknown main menu item %d"), (int32)Item);
			break;
	}
}


void UPlayViewBase::TransitionToState(EGameState State)
{
	if(GameState == State)
	{
		UE_LOG(LogGame, Warning, TEXT("Trying to transition to state %d when it's already current"), (int32)State);
		return;
	}

	const auto PreviousState = GameState;

	GameState = State;

	UDaylonUtils::Show(GameTitle, GameState != EGameState::Intro);

	// todo: maybe use a polymorphic game state class with an Enter() method instead of a switch statement.

	switch(GameState)
	{
		case EGameState::Intro:

			if(PreviousState != EGameState::Startup)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering intro state"), (int32)PreviousState);
			}

			TimeUntilIntroStateEnds = MaxIntroStateLifetime;
			RemoveAsteroids();

			UDaylonUtils::Hide (MenuContent);
			UDaylonUtils::Hide (PlayerScoreReadout);
			UDaylonUtils::Hide (PlayerShipsReadout);
			UDaylonUtils::Hide (PowerupReadouts);
			UDaylonUtils::Hide (GameOverMessage);
			UDaylonUtils::Show (IntroContent, InitialDelay == 0.0f);

			PlayAnimation(PressToStartFlash, 0.0f, 0);

			break;


		case EGameState::MainMenu:

			if(PreviousState == EGameState::Intro)
			{
				StopAnimation(PressToStartFlash);
			}

			UDaylonUtils::Hide (IntroContent);
			UDaylonUtils::Hide (HelpContent);
			UDaylonUtils::Hide (CreditsContent);
			UDaylonUtils::Hide (HighScoresContent);
			UDaylonUtils::Hide (PlayerScoreReadout);
			UDaylonUtils::Hide (PlayerShipsReadout);
			UDaylonUtils::Hide (PowerupReadouts);
			UDaylonUtils::Hide (GameOverMessage);
			UDaylonUtils::Hide (HighScoreEntryContent);
			
			RemoveExplosions  ();
			RemoveTorpedos    ();
			RemoveEnemyShips  ();
			RemovePowerups    ();


			StartMsgAnimationAge = 0.0f;
			UDaylonUtils::Show(MenuContent);
			MenuContent->SetRenderOpacity(1.0f);
			UpdateMenuReadout();

			if(Asteroids.IsEmpty())
			{
				SpawnAsteroids(6);
			}

			break;


		case EGameState::Active:

			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering active state"), (int32)PreviousState);
			}

			TimeUntilNextEnemyShip = 20.0f;
			TimeUntilNextWave      =  2.0f;

			if(!PlayerShip)
			{
				CreatePlayerShip         ();
				InitializePlayerDefenses ();
			}

			if(Torpedos.IsEmpty())
			{
				CreateTorpedos         ();
			}

			InitializeScore           ();
			InitializePlayerShipCount ();
			InitializePlayerShip      ();

			RemoveAsteroids           ();
			RemoveEnemyShips          ();
			RemovePowerups            ();

			WaveNumber = 0;

			UDaylonUtils::Show  (PlayerScoreReadout);
			UDaylonUtils::Show  (PlayerShipsReadout);
			UDaylonUtils::Show  (PowerupReadouts);

			UpdatePowerupReadout(EPowerup::DoubleGuns);
			UpdatePowerupReadout(EPowerup::Shields);
			UpdatePowerupReadout(EPowerup::Invincibility);


			PlayerShip->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);

			break;


		case EGameState::Over:

			if(PreviousState != EGameState::Active)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering game over state"), (int32)PreviousState);
			}

			Daylon::Destroy(PlayerShip);
			PlayerShip.Reset();

			Daylon::Destroy(PlayerShield);
			PlayerShield.Reset();

			Daylon::Destroy(PlayerInvincibilityShield);
			PlayerInvincibilityShield.Reset();

			UDaylonUtils::Show(GameOverMessage);

			TimeUntilGameOverStateEnds = MaxTimeUntilGameOverStateEnds;

			break;


		case EGameState::HighScores:
			
			if(PreviousState != EGameState::MainMenu && PreviousState != EGameState::HighScoreEntry)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering high scores state"), (int32)PreviousState);
			}

			LoadHighScores     ();
			PopulateHighScores (); 

			UDaylonUtils::Hide (GameOverMessage);
			UDaylonUtils::Hide (HighScoreEntryContent);
			UDaylonUtils::Show (HighScoresContent);

			UDaylonUtils::Show (PlayerScoreReadout, (PreviousState == EGameState::HighScoreEntry));

			RemoveEnemyShips();

			break;


		case EGameState::HighScoreEntry:

			if(PreviousState != EGameState::Over)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering high score entry state"), (int32)PreviousState);
			}

			RemoveAsteroids   ();
			RemoveEnemyShips  ();
			RemovePowerups    ();

			UDaylonUtils::Show(PlayerScoreReadout);
			UDaylonUtils::Hide(GameOverMessage);
			
			HighScoreEntryContent->SetVisibility(ESlateVisibility::Visible);
			HighScoreNameEntry->SetFocus();

			break;


		case EGameState::Credits:
			
			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering credits state"), (int32)PreviousState);
			}

			RemoveEnemyShips ();
			RemovePowerups   ();

			UDaylonUtils::Show(CreditsContent);

			break;


		case EGameState::Help:
			
			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering help state"), (int32)PreviousState);
			}

			RemoveAsteroids   ();
			RemoveEnemyShips  ();
			RemovePowerups    ();

			UDaylonUtils::Show(HelpContent);

			break;


		default:
	
			UE_LOG(LogGame, Error, TEXT("Trying to transition to unknown state %d"), (int32)State);
			break;
	}
}



void UPlayViewBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if(!IsInitialized)
	{
		return;
	}

	UpdateTasks(InDeltaTime);

	static float ExploCountAge = 1.0f;

	switch(GameState)
	{
		case EGameState::Intro:

			InitialDelay -= InDeltaTime;

			if(InitialDelay > 0.0f)
			{
				return;
			}

			UDaylonUtils::Show(IntroContent);


			// Fade in the title graphic and version number while explosions rage

			{
				static FBox2d Box(FVector2D(480, 300), FVector2D(1500, 550));

				if(TimeUntilIntroStateEnds > 0.0f)
				{
					TitleGraphic->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 1.5f) / MaxIntroStateLifetime));

					VersionReadout->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 3.0f) / MaxIntroStateLifetime));

					ExploCountAge -= InDeltaTime;
				
					if(ExploCountAge <= 0.0f)
					{
						ExploCountAge = 0.1f;

						SpawnExplosion(
							UDaylonUtils::RandomPtWithinBox(Box),
							4.5f,   // MinParticleSize
							9.0f,   // MaxParticleSize
							45.0f,  // MinParticleVelocity
							240.0f, // MaxParticleVelocity
							0.5f,   // MinParticleLifetime
							4.0f,   // MaxParticleLifetime
							0.25f,  // FinalOpacity
							80      // NumParticles
							);

						if(FMath::RandRange(0, 5) == 0)
						{
							PlaySound(PlayerShipDestroyedSound);
						}
						else
						{
							PlaySound(ExplosionSounds[FMath::RandRange(0, ExplosionSounds.Num() - 1)]);
						}
					}
				}
			}

			UpdateExplosions(InDeltaTime);

			TimeUntilIntroStateEnds -= InDeltaTime;

			break;


		case EGameState::MainMenu:

			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);

			break;


		case EGameState::Active:

			if(IsPlayerPresent())
			{
				UpdatePlayerRotation    (InDeltaTime);
				UpdatePlayerShip        (InDeltaTime);
			}

			UpdateEnemyShips          (InDeltaTime);
			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			UpdateExplosions          (InDeltaTime);

			CheckCollisions();

			ProcessWaveTransition     (InDeltaTime);

			if(PlayerShip && PlayerShip->IsSpawning)
			{
				ProcessPlayerShipSpawn    (InDeltaTime);
			}

			break;


		case EGameState::Over:

			UpdateEnemyShips          (InDeltaTime);
			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			UpdateExplosions          (InDeltaTime);
			CheckCollisions(); // In case any late torpedos or enemies hit something

			// Make the "game over" message blink
			GameOverMessage->SetOpacity(0.5f + sin(TimeUntilGameOverStateEnds * PI) * 0.5f);

			TimeUntilGameOverStateEnds -= InDeltaTime;

			if(TimeUntilGameOverStateEnds <= 0.0f)
			{
				LoadHighScores();

				if(HighScores.CanAdd(PlayerScore))
				{
					TransitionToState(EGameState::HighScoreEntry);
				}
				else
				{
					TransitionToState(EGameState::MainMenu);
				}
			}

			break;


		case EGameState::HighScores:

			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);

			if(MostRecentHighScoreTextBlock[0] != nullptr)
			{
				check(MostRecentHighScoreTextBlock[1] != nullptr);

				const auto T = MruHighScoreAnimationAge * PI * 2.0f;

				for(auto TextBlock : MostRecentHighScoreTextBlock)
				{
					TextBlock->SetOpacity(FMath::Lerp(0.5f, 1.0f, 0.5f + sin(T) * 0.5f));
				}

				MruHighScoreAnimationAge = FMath::Wrap(MruHighScoreAnimationAge + InDeltaTime, 0.0f, 10.0f);
			}

			break;

		case EGameState::Credits:
		case EGameState::Help:

			//UpdateEnemyShips        (InDeltaTime);
			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			break;


		case EGameState::HighScoreEntry:
			break;
	}
}


void UPlayViewBase::AddPlayerShips(int32 Amount)
{
	NumPlayerShips += Amount;

	NumPlayerShips = FMath::Max(0, NumPlayerShips);

	// Update readout.

	if(Amount > 0 && PlayerShipsReadout->GetChildrenCount() < MaxPlayerShipsDisplayable)
	{
		while(PlayerShipsReadout->GetChildrenCount() < NumPlayerShips)
		{
			UImage* Image = UDaylonUtils::MakeWidget<UImage>();
			Image->SetBrush(PlayerShipAtlas->Atlas.AtlasBrush);
			Image->Brush.SetImageSize(FVector2D(24));
			Image->Brush.SetUVRegion(PlayerShipAtlas->Atlas.GetUVsForCel(PlayerShipNormalAtlasCel));
			
			PlayerShipsReadout->AddChildToHorizontalBox(Image);
		}
	}
	else if(Amount < 0)
	{
		while(PlayerShipsReadout->GetChildrenCount() > NumPlayerShips)
		{
			PlayerShipsReadout->RemoveChildAt(PlayerShipsReadout->GetChildrenCount() - 1);
		}
	}
}


void UPlayViewBase::ProcessPlayerShipSpawn(float DeltaTime)
{
	// The player ship got destroyed and we're waiting to spawn a new one.

	if(!PlayerShip->IsSpawning)
	{
		return;
	}

	if(IsWaitingToSpawnPlayer())
	{
		TimeUntilNextPlayerShip -= DeltaTime;
		return;
	}

	if(NumPlayerShips == 0)
	{
		// Mandatory period ended but we have no ships, so transition to game over state.
		TransitionToState(EGameState::Over);
		return;
	}

	// The mandatory period has ended but don't spawn until there's enough clear space around the screen center.
	if(!IsSafeToSpawnPlayer())
	{
		return;
	}

	PlayerShip->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);

	PlayerShip->IsSpawning = false;
}


template<typename T>
bool ObjectsIntersectBox(const TArray<T>& PlayObjects, const UE::Geometry::FAxisAlignedBox2d& SafeZone)
{
	for(const auto& PlayObject : PlayObjects)
	{
		const auto ObjectHalfSize = PlayObject.GetSize() / 2;

		const UE::Geometry::FAxisAlignedBox2d ObjectBox
		(
			PlayObject.UnwrappedNewPosition - ObjectHalfSize,
			PlayObject.UnwrappedNewPosition + ObjectHalfSize
		);

		if(ObjectBox.Intersects(SafeZone))
		{
			return true;
		}
	}
	return false;
}


template <typename T>
bool ImageObjectsIntersectBox(const TArray<TSharedPtr<T>>& PlayObjects, const UE::Geometry::FAxisAlignedBox2d& SafeZone)
{
	for(const auto& PlayObject : PlayObjects)
	{
		const auto ObjectHalfSize = PlayObject->GetSize() / 2;

		const UE::Geometry::FAxisAlignedBox2d ObjectBox
		(
			PlayObject->UnwrappedNewPosition - ObjectHalfSize,
			PlayObject->UnwrappedNewPosition + ObjectHalfSize
		);

		if(ObjectBox.Intersects(SafeZone))
		{
			return true;
		}
	}
	return false;
}


bool UPlayViewBase::IsSafeToSpawnPlayer() const
{
	if(!PlayerShip->IsSpawning)
	{
		UE_LOG(LogGame, Error, TEXT("Checking if safe to spawn player when player is not waiting to spawn"));
		return false;
	}

	// If any asteroid or enemy ship intersects a box in the center of the screen, return false.

	// Make the safezone smaller if the player has enough shields. 
	// This makes it easier to respawn when there are lots of things around
	// without having to potentially wait a really long time.

	const auto SafeZoneDivisor = (PlayerShip->ShieldsLeft > 3.0f) ? 8 : 4;

	const auto ScreenCenter = ViewportSize / 2;
	const auto SafeZoneSize = ViewportSize / SafeZoneDivisor;

	UE::Geometry::FAxisAlignedBox2d SafeZone(
		ScreenCenter - SafeZoneSize / 2,
		ScreenCenter + SafeZoneSize / 2);

	return (!ImageObjectsIntersectBox(Asteroids, SafeZone) && !ImageObjectsIntersectBox(EnemyShips, SafeZone));
}


bool UPlayViewBase::IsWaitingToSpawnPlayer() const
{
	return (TimeUntilNextPlayerShip > 0.0f);
}


void UPlayViewBase::ProcessWaveTransition(float DeltaTime)
{
	// If there are no more targets, then spawn the next wave after a few seconds.

	if(TimeUntilNextWave < TimeBetweenWaves)
	{
		// We are currently counting down to the start of the next wave.

		TimeUntilNextWave -= DeltaTime;

		if(TimeUntilNextWave > 0.0f)
		{
			// Time still remaining, don't start next wave yet.
			return;
		}

		// Next wave can start.
		StartWave();

		return;
	}

	if(!Asteroids.IsEmpty())
	{
		// Current wave still has targets.
		return;
	}

	// Wave has ended, start counting down.
	TimeUntilNextWave = TimeBetweenWaves - DeltaTime;
}




void UPlayViewBase::RemoveAsteroid(int32 Index)
{
	if(!Asteroids.IsValidIndex(Index))
	{
		UE_LOG(LogGame, Error, TEXT("RemoveAsteroid: bad index %d"), Index);
		return;
	}

	auto& Asteroid = *Asteroids[Index].Get();

	if(Asteroid.HasPowerup())
	{
		Daylon::Destroy(Asteroid.Powerup);
	}

	Daylon::Destroy(Asteroids[Index]);

	Asteroids.RemoveAt(Index);
}


void UPlayViewBase::RemoveAsteroids()
{
	for(int32 Index = Asteroids.Num() - 1; Index >= 0; Index--)
	{
		RemoveAsteroid(Index);		
	}
}


void UPlayViewBase::SpawnPowerup(TSharedPtr<FPowerup>& PowerupPtr, const FVector2D& P)
{
	auto PowerupKind = (EPowerup)FMath::RandRange(1, 3);

	UDaylonSpriteWidgetAtlas* Atlas = nullptr;

	switch(PowerupKind)
	{
		case EPowerup::DoubleGuns:    Atlas = DoubleGunsPowerupAtlas;    break;
		case EPowerup::Shields:       Atlas = ShieldPowerupAtlas;        break;
		// todo: this powerup is special and should be scarcer, even available only after a certain score or wave reached.
		case EPowerup::Invincibility: Atlas = InvincibilityPowerupAtlas; break;
	}

	check(Atlas);

	Atlas->Atlas.AtlasBrush.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, PowerupOpacity);

	check(!PowerupPtr);

	PowerupPtr = FPowerup::Create(Atlas, FVector2D(32));

	auto& Powerup = *PowerupPtr.Get();

	Powerup.Kind = PowerupKind;

	//Powerup.Show();
	Powerup.SetPosition(P);
	Powerup.Inertia.Set(0, 0);
}


void UPlayViewBase::SpawnAsteroids(int32 NumAsteroids)
{
	// The Slate way.

	for(int32 Index = 0; Index < NumAsteroids; Index++)
	{
		// 0=big, 1=med, 2=small
#if(TEST_ASTEROIDS==1)
		const int32 AsteroidSize = Index;
#else
		const int32 AsteroidSize = 0; 
#endif


#if(TEST_ASTEROIDS==1)
		FVector2D P(500, Index * 300 + 200);
#else
		// Place randomly along edges of screen.
		FVector2D P(0);

		if(FMath::RandBool())
		{
			P.X = FMath::RandRange(0.0, ViewportSize.X);
		}
		else
		{
			P.Y = FMath::RandRange(0.0, ViewportSize.Y);
		}
#endif

		const auto V = FMath::VRand();

#if(TEST_ASTEROIDS==1)
		const auto Inertia = FVector2D(0);
#else
		const auto Inertia = UDaylonUtils::RandVector2D() * FMath::Lerp(MinAsteroidSpeed, MaxAsteroidSpeed, FMath::FRand());
#endif

		//FSlateBrush AsteroidBrush;
		UDaylonSpriteWidgetAtlas* AsteroidAtlas = nullptr;
		int32 AsteroidValue = 0;

		switch(AsteroidSize)
		{
			case 0:
				AsteroidAtlas = LargeRockAtlas;
				AsteroidValue = ValueBigAsteroid;

				break;

			case 1: 
				AsteroidAtlas = MediumRockAtlas;
				AsteroidValue = ValueMediumAsteroid;
				break;

			case 2: 
				AsteroidAtlas = SmallRockAtlas;
				AsteroidValue = ValueSmallAsteroid;
				break;
		}

		auto Asteroid = FAsteroid::Create(AsteroidAtlas);
		Asteroid->Value = AsteroidValue;
		Asteroid->LifeRemaining = 1.0f;
		Asteroid->SpinSpeed = FMath::RandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);

		if(AsteroidSize == 0 && Index % 4 == 0)
		{
			SpawnPowerup(Asteroid->Powerup, P);
		}

		Asteroid->Spawn(P, Inertia, 1.0f);

		// Do this last since the play object is copied.
		Asteroids.Add(Asteroid);
	}
}


void UPlayViewBase::StartWave()
{
	TimeUntilNextWave = TimeBetweenWaves;

	WaveNumber++;

	// Fill with asteroids.

#if(TEST_ASTEROIDS == 1)
	const int32 NumAsteroids = 3;
#else
	const int32 NumAsteroids = NumAsteroidsOverride > 0 ? NumAsteroidsOverride : FMath::Min(MaxInitialAsteroids, 2 + (WaveNumber * 2));
#endif

	RemoveAsteroids(); // to be safe
	SpawnAsteroids(NumAsteroids);

	TimeUntilNextEnemyShip = MaxTimeUntilNextEnemyShip; 

	
	// Spawn extra powerups if requested.

	int32 NumExtraPowerups = NumPowerupsOverride - Powerups.Num();

	// Spawn extra powerups in a centered 1/2 box 
	FBox2d Box(FVector2D(ViewportSize.X * 0.25, ViewportSize.Y * 0.25), 
	           FVector2D(ViewportSize.X * 0.75, ViewportSize.Y * 0.75));

	while(NumExtraPowerups-- > 0)
	{
		TSharedPtr<FPowerup> PowerupPtr;

		SpawnPowerup(PowerupPtr, UDaylonUtils::RandomPtWithinBox(Box));

		Powerups.Add(PowerupPtr);
	}
}


void UPlayViewBase::UpdatePlayerRotation(float DeltaTime)
{
	// Uses a 1D axis value instead of action buttons

	if(!PlayerShip->IsValid())
	{
		UE_LOG(LogGame, Error, TEXT("Invalid player ship"));
		return;
	}

	const float Amt = PlayerRotationSpeed * DeltaTime;

	//UE_LOG(LogGame, Log, TEXT("Rotation force: %.5f"), RotationForce);

	PlayerShip->SetAngle(UDaylonUtils::WrapAngle(PlayerShip->GetAngle() + Amt * RotationForce));
}


void UPlayViewBase::UpdatePlayerShip(float DeltaTime)
{
	if(!PlayerShip->IsValid())
	{
		UE_LOG(LogGame, Error, TEXT("Invalid player ship"));
		return;
	}

	// Change widget brush only if the thrust state actually changed.
	const bool bThrustStateChanged = (PlayerShip->IsUnderThrust != bThrustActive);

	PlayerShip->IsUnderThrust = bThrustActive;

	if (bThrustActive)
	{
		if(bThrustStateChanged)
		{
			PlayerShip->SetCurrentCel(PlayerShipThrustingAtlasCel);
			PlayerShipThrustSoundLoop.Start();
		}
		else
		{
			// Still thrusting.
			PlayerShipThrustSoundLoop.Tick(DeltaTime);
		}

		const float Thrust = PlayerThrustForce * DeltaTime;

		const FVector2D Force = PlayerShip->GetDirectionVector() * Thrust;

		PlayerShip->Inertia += Force;

		// Limit speed to avoid breaking collision detector.

		if(PlayerShip->Inertia.Length() > MaxPlayerShipSpeed)
		{
			PlayerShip->Inertia.Normalize();
			PlayerShip->Inertia *= MaxPlayerShipSpeed;
		}
	}
	else
	{
		if(bThrustStateChanged)
		{
			PlayerShip->SetCurrentCel(PlayerShipNormalAtlasCel);
		}
	}

	PlayerShip->Move(DeltaTime, WrapPositionToViewport);


	PlayerShield->Show(bShieldActive && PlayerShip->ShieldsLeft > 0.0f);

	if(PlayerShield->IsVisible())
	{
		// We have to budge the shield texture by two px to look nicely centered around the player ship.
		PlayerShield->SetPosition(PlayerShip->GetPosition() + UDaylonUtils::Rotate(FVector2D(0, 2), PlayerShip->GetAngle()));
		AdjustShieldsLeft(-DeltaTime);
	}


	if(PlayerShip->InvincibilityLeft <= 0.0f)
	{
		PlayerInvincibilityShield->Hide();
	}
	else if(PlayerShip->InvincibilityLeft <= 5.0f) // todo: use constant
	{
		// Flash the invincibility shield to indicate that it has only a few seconds of power left.
		TimeUntilNextInvincibilityWarnFlash -= DeltaTime;

		if(TimeUntilNextInvincibilityWarnFlash <= 0.0f)
		{
			TimeUntilNextInvincibilityWarnFlash = MaxInvincibilityWarnTime;	
		}
		PlayerInvincibilityShield->Show(TimeUntilNextInvincibilityWarnFlash < MaxInvincibilityWarnTime * 0.5f);
	}
	else
	{
		PlayerInvincibilityShield->Show();
	}


	if(PlayerInvincibilityShield->IsVisible())
	{
		PlayerInvincibilityShield->SetAngle(PlayerShip->GetAngle());
		PlayerInvincibilityShield->SetPosition(PlayerShip->GetPosition() + UDaylonUtils::Rotate(FVector2D(0, -2), PlayerShip->GetAngle()));
	}

	if(PlayerShip->InvincibilityLeft > 0.0f)
	{
		AdjustInvincibilityLeft(-DeltaTime);
	}
}


void UPlayViewBase::UpdateAsteroids(float DeltaTime)
{
	for (auto& Elem : Asteroids)
	{
		auto& Asteroid = *Elem.Get();

		if (Asteroid.LifeRemaining > 0.0f)
		{
			Asteroid.Move(DeltaTime, WrapPositionToViewport);
			if(Asteroid.HasPowerup())
			{
				Asteroid.Powerup->SetPosition(Asteroid.GetPosition());
				Asteroid.Powerup->Update(DeltaTime);
			}
#if(FEATURE_SPINNING_ASTEROIDS == 1)
			Asteroid.SetAngle(UDaylonUtils::WrapAngle(Asteroid.GetAngle() + Asteroid.SpinSpeed * DeltaTime));
#endif
		}
	}
}


void UPlayViewBase::UpdatePowerups(float DeltaTime)
{
	// This will move any free-floating powerups.

	for(auto& Powerup : Powerups)
	{
		Powerup->Move(DeltaTime, WrapPositionToViewport);
		Powerup->Update(DeltaTime);
	}
}


void UPlayViewBase::SpawnExplosion
(
	const FVector2D& P,
	float MinParticleSize,
	float MaxParticleSize,
	float MinParticleVelocity,
	float MaxParticleVelocity,
	float MinParticleLifetime,
	float MaxParticleLifetime,
	float FinalOpacity,
	int32 NumParticles
)
{
	auto Explosion = SNew(SDaylonParticles)
		.MinParticleSize      (MinParticleSize)
		.MaxParticleSize      (MaxParticleSize)
		.MinParticleVelocity  (MinParticleVelocity)
		.MaxParticleVelocity  (MaxParticleVelocity)
		.MinParticleLifetime  (MinParticleLifetime)
		.MaxParticleLifetime  (MaxParticleLifetime)
		.FinalOpacity         (FinalOpacity)
		.NumParticles         (NumParticles);

	Explosion->SetVisibility(EVisibility::HitTestInvisible);
	Explosion->SetRenderTransformPivot(FVector2D(0.5f));

	Explosion->SetParticleBrush(TorpedoBrush);

	auto SlotArgs = RootCanvas->GetCanvasWidget()->AddSlot();

	SlotArgs[Explosion];
	SlotArgs.AutoSize(true);
	SlotArgs.Alignment(FVector2D(0.5));

	auto Margin = SlotArgs.GetSlot()->GetOffset();
	Margin.Left = P.X;
	Margin.Top  = P.Y;

	SlotArgs.GetSlot()->SetOffset(Margin);

	Explosions.Add(Explosion);
}


void UPlayViewBase::SpawnExplosion(const FVector2D& P)
{
	SpawnExplosion(P, 3.0f, 3.0f, 30.0f, 80.0f, 0.25f, 1.0f, 1.0f, 40);
}


void UPlayViewBase::SpawnPlayerShipExplosion(const FVector2D& P)
{
	SpawnExplosion(P, 
		3.0f,
		6.0f,
		30.0f,
		160.0f,
		0.5f,
		3.0f,
		0.25f,
		80);


	// Set up second explosion event for 3/4 second later

	Daylon::FScheduledTask Task;

	Task.When = 0.66f;

	Task.What = [P, This=TWeakObjectPtr<UPlayViewBase>(this)]()
	{
		if(!This.IsValid())
		{
			return;
		}

		if(This->GameState != EGameState::Active)
		{
			return;
		}

		This->SpawnExplosion(P, 
			4.5f,
			9.0f,
			45.0f,
			240.0f,
			0.5f,
			4.0f,
			0.25f,
			80);
	};

	ScheduledTasks.Add(Task);
}


void UPlayViewBase::UpdateExplosions(float DeltaTime)
{
	if(Explosions.IsEmpty())
	{
		return;
	}

	for(int32 Index = Explosions.Num() - 1; Index >= 0; Index--)
	{
		if(!Explosions[Index]->Update(DeltaTime))
		{
			RootCanvas->GetCanvasWidget()->RemoveSlot(Explosions[Index].ToSharedRef());
			Explosions.RemoveAt(Index);
		}
	}
}


void UPlayViewBase::PlaySound(USoundBase* Sound, float VolumeScale)
{
	UGameplayStatics::PlaySound2D(this, Sound, VolumeScale);
}


void UPlayViewBase::KillPlayerShip()
{
	SpawnPlayerShipExplosion(PlayerShip->UnwrappedNewPosition);

	PlaySound(PlayerShipDestroyedSound);

	PlayerShip->Hide();

	AddPlayerShips(-1);

	PlayerShip->IsSpawning = true;
	TimeUntilNextPlayerShip = MaxTimeUntilNextPlayerShip;

	// ProcessPlayerShipSpawn() will handle the wait til next spawn and transition to game over, if needed. 
}


void UPlayViewBase::KillAsteroid(int32 AsteroidIndex, bool KilledByPlayer)
{
	// Kill the rock. Split it if isn't a small rock.
	// Slate style.

	if(!Asteroids.IsValidIndex(AsteroidIndex))
	{
		return;
	}

	auto& Asteroid = *Asteroids[AsteroidIndex].Get();

	if(KilledByPlayer)
	{
		IncreasePlayerScoreBy(Asteroid.Value);
	}

	SpawnExplosion(Asteroid.UnwrappedNewPosition);

	int32 SoundIndex = 0;
	
	if(Asteroid.Value == ValueMediumAsteroid)
	{
		SoundIndex = 1;
	}
	else if(Asteroid.Value == ValueSmallAsteroid)
	{
		SoundIndex = 2;
	}

	if(ExplosionSounds.IsValidIndex(SoundIndex))
	{
		PlaySound(ExplosionSounds[SoundIndex]);
	}


	// If asteroid was small, just delete it.
	if(Asteroid.Value == ValueSmallAsteroid)
	{
		// Release any powerup the asteroid was holding.

		if(Asteroid.HasPowerup())
		{
			auto PowerupIndex = Powerups.Add(Asteroid.Powerup);
			Asteroid.Powerup.Reset();

			if(PowerupsCanMove)
			{
				Powerups[PowerupIndex]->Inertia = Asteroid.Inertia;
			}
		}

		RemoveAsteroid(AsteroidIndex);

		return;
	}

	// Asteroid was not small, so split it up.
	// Apparently we always split into two children.
	// For efficiency, we reformulate the parent rock into one of the kids.
	// For the new inertias, we want the kids to generally be faster but 
	// once in a while, one of the kids can be slower.

	UDaylonSpriteWidgetAtlas* NewAsteroidAtlas = nullptr;

	switch(Asteroid.Value)
	{
		case ValueBigAsteroid:
			NewAsteroidAtlas = MediumRockAtlas;
			Asteroid.Value = ValueMediumAsteroid;
			Asteroid.SetAtlas(MediumRockAtlas->Atlas);
			break;

		case ValueMediumAsteroid:
			NewAsteroidAtlas = SmallRockAtlas;
			Asteroid.Value = ValueSmallAsteroid;
			Asteroid.SetAtlas(SmallRockAtlas->Atlas);
			break;
	}

	Asteroid.SetSize(NewAsteroidAtlas->Atlas.GetCelPixelSize());
	Asteroid.UpdateWidgetSize();

	Asteroid.SetCurrentCel(FMath::RandRange(0, NewAsteroidAtlas->Atlas.NumCels - 1));


	auto NewAsteroidPtr = FAsteroid::Create(NewAsteroidAtlas);
	auto& NewAsteroid   = *NewAsteroidPtr.Get();

	NewAsteroid.Value = Asteroid.Value;


	const bool BothKidsFast = FMath::RandRange(0, 10) < 9;

	NewAsteroid.Inertia = MakeInertia(Asteroid.Inertia, MinAsteroidSplitAngle, MaxAsteroidSplitAngle);
	NewAsteroid.Inertia *= FMath::RandRange(1.2f, 3.0f);

	NewAsteroid.LifeRemaining = 1.0f;
	NewAsteroid.SpinSpeed     = Asteroid.SpinSpeed * AsteroidSpinScale;// FMath::RandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);

	Asteroid.Inertia = MakeInertia(Asteroid.Inertia, -MinAsteroidSplitAngle, -MaxAsteroidSplitAngle);

	if(BothKidsFast)
	{
		Asteroid.Inertia *= FMath::RandRange(1.2f, 3.0f);
	}
	else
	{
		Asteroid.Inertia *= FMath::RandRange(0.25f, 1.0f);
	}


	// Update size of existing rock.
	//Asteroid.UpdateWidgetSize();

	Asteroid.SpinSpeed *= AsteroidSpinScale;

	//NewAsteroid.Create(NewAsteroidBrush, 0.5f);
	NewAsteroid.Show();

	NewAsteroid.OldPosition = 
	NewAsteroid.UnwrappedNewPosition = Asteroid.UnwrappedNewPosition;
	NewAsteroid.SetPosition(NewAsteroid.UnwrappedNewPosition);

	// Do this last since the play object is copied.
	Asteroids.Add(NewAsteroidPtr);
}


void UPlayViewBase::KillEnemyShip(int32 EnemyIndex)
{
	if(!EnemyShips.IsValidIndex(EnemyIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid enemy ship index %d"), EnemyIndex);
		return;
	}

	auto& EnemyShip = *EnemyShips[EnemyIndex].Get();

	SpawnExplosion(EnemyShip.GetPosition());
	PlaySound(ExplosionSounds[EnemyShip.Value == ValueBigEnemy ? 0 : 1]);
	RemoveEnemyShip(EnemyIndex);
}


void UPlayViewBase::KillScavenger(int32 ScavengerIndex)
{
	if(!Scavengers.IsValidIndex(ScavengerIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid scavenger index %d"), ScavengerIndex);
		return;
	}

	auto& Scavenger = *Scavengers[ScavengerIndex].Get();

	// Have scavenger drop any powerups it was carrying.

	// For now, place them in a line trailing away from the scavenger.
	FVector2D Direction = Scavenger.Inertia;
	Direction.Normalize();
	Direction *= -1; // Start from the scavenger's position and work backwards.

	for(int32 Index = 0; Index < Scavenger.AcquiredPowerups.Num(); Index++)
	{
		auto DroppedPowerupPtr = Scavenger.AcquiredPowerups[Index];
		DroppedPowerupPtr->Show();
		DroppedPowerupPtr->SetPosition(WrapPositionToViewport(Scavenger.GetPosition() + (Direction * DroppedPowerupPtr->GetRadius() * 2.5f * Index)));
		Powerups.Add(DroppedPowerupPtr);
	}
	Scavenger.AcquiredPowerups.Empty();

	SpawnExplosion(Scavenger.GetPosition());

	// todo: have scavenger explosion sound
	PlaySound(ExplosionSounds[0]);
	RemoveScavenger(ScavengerIndex);
}


void UPlayViewBase::KillPowerup(int32 PowerupIndex)
{
	if(!Powerups.IsValidIndex(PowerupIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid powerup index %d"), PowerupIndex);
		return;
	}

	auto& Powerup = Powerups[PowerupIndex];

	//SpawnExplosion(EnemyShip.GetPosition());
	// todo: PlaySound(ExplosionSounds[EnemyShip.Value == ValueBigEnemy ? 0 : 1]);
	RemovePowerup(PowerupIndex);
}


void UPlayViewBase::IncreasePlayerScoreBy(int32 Amount)
{
	if(PlayerScore >= MaxPlayerScore)
	{
		return;
	}

	const int32 PrevLevel = PlayerScore / PlayerShipBonusAt;

	PlayerScore += Amount;

	if(PlayerScore > MaxPlayerScore)
	{
		PlayerScore = MaxPlayerScore;
	}

	if(PrevLevel != PlayerScore / PlayerShipBonusAt)
	{
		AddPlayerShips(1);

		PlaySound(PlayerShipBonusSound);
	}

	UpdatePlayerScoreReadout();

}


void UPlayViewBase::UpdatePlayerScoreReadout()
{
	PlayerScoreReadout->SetText(FText::FromString(FString::Format(TEXT("{0}"), { PlayerScore })));
}


void UPlayViewBase::UpdatePowerupReadout(EPowerup PowerupKind)
{
	FString Str;

	switch(PowerupKind)
	{
		case EPowerup::DoubleGuns:
			Str = FString::Printf(TEXT("%d"), PlayerShip->DoubleShotsLeft);
			DoubleGunReadout->SetText(FText::FromString(Str));
			break;

		case EPowerup::Shields:
			Str = FString::Printf(TEXT("%d"), FMath::RoundToInt(PlayerShip->ShieldsLeft));
			PlayerShieldReadout->SetText(FText::FromString(Str));
			break;

		case EPowerup::Invincibility:
			Str = FString::Printf(TEXT("%d"), FMath::RoundToInt(PlayerShip->InvincibilityLeft));
			InvincibilityReadout->SetText(FText::FromString(Str));
			break;
	}
}


void UPlayViewBase::AdjustDoubleShotsLeft(int32 Amount)
{
	PlayerShip->DoubleShotsLeft += Amount;
	UpdatePowerupReadout(EPowerup::DoubleGuns);
}


void UPlayViewBase::AdjustShieldsLeft(float Amount)
{
	if(PlayerShip->ShieldsLeft < 0.0f)
	{
		PlayerShip->ShieldsLeft = 0.0f;
	}

	PlayerShip->ShieldsLeft = FMath::Max(0.0f, PlayerShip->ShieldsLeft + Amount);
	UpdatePowerupReadout(EPowerup::Shields);
}


void UPlayViewBase::AdjustInvincibilityLeft(float Amount)
{
	if(PlayerShip->InvincibilityLeft < 0.0f)
	{
		PlayerShip->InvincibilityLeft = 0.0f;
	}

	PlayerShip->InvincibilityLeft = FMath::Max(0.0f, PlayerShip->InvincibilityLeft + Amount);

	UpdatePowerupReadout(EPowerup::Invincibility);
}


void UPlayViewBase::ProcessPlayerCollision()
{
	check(PlayerShip);

	if(bGodMode || PlayerShip->InvincibilityLeft > 0.0f)
	{
		PlaySound(ShieldBonkSound);
		return;
	}

	if(PlayerShield->IsVisible())
	{
		AdjustShieldsLeft(-ShieldBonkDamage);
		PlaySound(ShieldBonkSound);
		return;
	}

	KillPlayerShip();
}


void UPlayViewBase::CheckCollisions()
{
	// Build a triangle representing the player ship.

	FVector2D PlayerShipTriangle[3]; // tip, LR corner, LL corner.
	FVector2D PlayerShipLineStart;
	FVector2D PlayerShipLineEnd;

	if(IsPlayerPresent())
	{
		PlayerShipLineStart = PlayerShip->OldPosition;
		PlayerShipLineEnd   = PlayerShip->UnwrappedNewPosition;

		float PlayerShipH = PlayerShip->GetSize().Y;
		float PlayerShipW = PlayerShipH * (73.0f / 95.0f);

		PlayerShipTriangle[0].Set(0.0f,            -PlayerShipH / 2);
		PlayerShipTriangle[1].Set( PlayerShipW / 2, PlayerShipH / 2);
		PlayerShipTriangle[2].Set(-PlayerShipW / 2, PlayerShipH / 2);

		// Triangle is pointing up, vertices relative to its center. We need to rotate it for its current angle.
			 
		auto ShipAngle = PlayerShip->GetAngle();

		// Rotate and translate the triangle to match its current display space.

		for(auto& Triangle : PlayerShipTriangle)
		{
			Triangle = UDaylonUtils::Rotate(Triangle, ShipAngle);

			// Use the ship's old position because the current position can cause unwanted self-intersections.
			Triangle += PlayerShip->OldPosition;
		}

		// Get the line segment for the player ship.
		// Line segments are used to better detect collisions involving fast-moving objects.

		PlayerShipLineStart = PlayerShip->OldPosition;
		PlayerShipLineEnd   = PlayerShip->UnwrappedNewPosition;
	}

	// See what the active torpedos have collided with.

	for (auto& TorpedoPtr : Torpedos)
	{
		auto& Torpedo = *TorpedoPtr.Get();

		if(!Torpedo.IsAlive())
		{
			continue;
		}

		// todo: we might not detect collisions near edge of viewport for
		// wrapped objects. E.g. a big rock could be partly visible on the west edge,
		// but it's centroid (position) is wrapped around to the east edge.

		const FVector2D OldP     = Torpedo.OldPosition;
		const FVector2D CurrentP = Torpedo.UnwrappedNewPosition;

		// See if we hit any rocks. todo: we only need current pos, not next.
		// todo: we should first move all objects, then check for collisions.

		int32 AsteroidIndex = -1;

		for(auto& Elem : Asteroids)
		{
			AsteroidIndex++;

			auto& Asteroid = *Elem.Get();

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, Asteroid.GetPosition(), Asteroid.GetRadius())
				|| UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, OldP, Asteroid.GetRadius()))
			{
				// A torpedo hit a rock.

				Torpedo.Kill();
				KillAsteroid(AsteroidIndex, Torpedo.FiredByPlayer);

				// We don't need to check any other asteroids.
				break;
			}
		}
		
		if(Torpedo.IsAlive() && !Torpedo.FiredByPlayer && IsPlayerPresent())
		{
			// Torpedo didn't hit a rock and the player didn't fire it, so see if it hit the player ship.

			if(UDaylonUtils::DoesLineSegmentIntersectTriangle(OldP, CurrentP, PlayerShipTriangle))
			{
				Torpedo.Kill();
				SpawnExplosion(OldP);
				ProcessPlayerCollision();
			}
		}

		if(Torpedo.IsAlive() && Torpedo.FiredByPlayer)
		{
			// See if the torpedo hit an enemy ship.

			for(int32 EnemyIndex = EnemyShips.Num() - 1; EnemyIndex >= 0; EnemyIndex--)
			{
				auto& EnemyShip = *EnemyShips[EnemyIndex].Get();

				if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, EnemyShip.GetPosition(), EnemyShip.GetRadius()))
				{
					Torpedo.Kill();
					IncreasePlayerScoreBy(EnemyShip.Value);
					SpawnExplosion(EnemyShip.GetPosition());
					PlaySound(ExplosionSounds[EnemyShip.Value == ValueBigEnemy ? 0 : 1]);
					RemoveEnemyShip(EnemyIndex);
				}
			}

			// See if the torpedo hit a scavenger.

			for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
			{
				auto& Scavenger = *Scavengers[ScavengerIndex].Get();

				if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, Scavenger.GetPosition(), Scavenger.GetRadius()))
				{
					Torpedo.Kill();
					IncreasePlayerScoreBy(Scavenger.Value);
					KillScavenger(ScavengerIndex);
				}
			}
		}
	} // next torpedo


	for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		// Did a scavenger collide with a powerup?

		auto& Scavenger = *Scavengers[ScavengerIndex].Get();

		int32 PowerupIndex = 0;

		for(auto PowerupPtr : Powerups)
		{
			const auto Distance = FVector2D::Distance(Scavenger.GetPosition(), PowerupPtr.Get()->GetPosition());
					
			if(Distance < Scavenger.GetRadius() + PowerupPtr.Get()->GetRadius())
			{
				// Collision occurred; acquire the powerup.
				
				PlaySound(GainDoubleGunPowerupSound);  // todo: play a scavenger-specific sound.

				PowerupPtr.Get()->Hide();
				Scavenger.AcquiredPowerups.Add(PowerupPtr);
				Powerups.RemoveAt(PowerupIndex);

				Scavenger.CurrentTarget.Reset();

				break;
			}
			PowerupIndex++;
		}
	}


	// Check if player ship collided with a rock

	if(IsPlayerPresent())
	{
		int32 AsteroidIndex = -1;

		for(auto& Elem : Asteroids)
		{
			AsteroidIndex++;

			auto& Asteroid = *Elem.Get();

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Asteroid.GetPosition(), Asteroid.GetRadius() + PlayerShip->GetRadius())
				|| UDaylonUtils::DoesLineSegmentIntersectTriangle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, PlayerShipTriangle))
			{
				// Player collided with a rock.

				ProcessPlayerCollision();
				// todo: should we credit player if he used his shields?
				KillAsteroid(AsteroidIndex, CreditPlayerForKill);

				break;
			}
		}
	}


	// Check if enemy ship collided with the player

	if(IsPlayerPresent())
	{
		for(int32 EnemyIndex = EnemyShips.Num() - 1; EnemyIndex >= 0; EnemyIndex--)
		{
			auto& EnemyShip = *EnemyShips[EnemyIndex].Get();

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, EnemyShip.OldPosition, EnemyShip.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, EnemyShip.UnwrappedNewPosition) < EnemyShip.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(EnemyShip.Value);
				KillEnemyShip(EnemyIndex);

				ProcessPlayerCollision();

				break;
			}
		}
	}

	if(IsPlayerPresent())
	{
		for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
		{
			auto& Scavenger = *Scavengers[ScavengerIndex].Get();

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Scavenger.OldPosition, Scavenger.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, Scavenger.UnwrappedNewPosition) < Scavenger.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(Scavenger.Value);
				KillScavenger(ScavengerIndex);

				ProcessPlayerCollision();

				break;
			}
		}
	}


	// Check if player ship collided with a powerup

	if(IsPlayerPresent())
	{
		for(int32 PowerupIndex = Powerups.Num() - 1; PowerupIndex >= 0; PowerupIndex--)
		{
			auto& Powerup = *Powerups[PowerupIndex].Get();

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Powerup.OldPosition, Powerup.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, Powerup.UnwrappedNewPosition) < Powerup.GetRadius() + PlayerShip->GetRadius())
			{
				// Powerup collided with player ship.

				switch(Powerup.Kind)
				{
					case EPowerup::DoubleGuns:

						AdjustDoubleShotsLeft(DoubleGunsPowerupIncrease);
						PlaySound(GainDoubleGunPowerupSound);
						PlayAnimation(DoubleGunReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;


					case EPowerup::Shields:

						AdjustShieldsLeft(ShieldPowerupIncrease);
						PlaySound(GainShieldPowerupSound);
						PlayAnimation(ShieldReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;

					case EPowerup::Invincibility:
						PlaySound(GainShieldPowerupSound); // todo: specific sound
						AdjustInvincibilityLeft(MaxInvincibilityTime);
						TimeUntilNextInvincibilityWarnFlash = MaxInvincibilityWarnTime;
						PlayAnimation(InvincibilityReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;
				}
				
				KillPowerup(PowerupIndex);

				break;
			}
		}
	}


	// Check if enemy ship collided with an asteroid

	for(int32 EnemyIndex = EnemyShips.Num() - 1; EnemyIndex >= 0; EnemyIndex--)
	{
		auto& EnemyShip = *EnemyShips[EnemyIndex].Get();

		int32 AsteroidIndex = -1; 

		for(auto& AsteroidPtr : Asteroids)
		{
			AsteroidIndex++;

			auto& Asteroid = *AsteroidPtr.Get();

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, EnemyShip.GetPosition(), EnemyShip.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(EnemyShip.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + EnemyShip.GetRadius())
			{
				// Enemy ship collided with a rock.

				KillEnemyShip(EnemyIndex);
				KillAsteroid(AsteroidIndex, DontCreditPlayerForKill);

				break;
			}
		}
	}

	for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		auto& Scavenger = *Scavengers[ScavengerIndex].Get();

		int32 AsteroidIndex = -1; 

		for(auto& AsteroidPtr : Asteroids)
		{
			AsteroidIndex++;

			auto& Asteroid = *AsteroidPtr.Get();

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, Scavenger.GetPosition(), Scavenger.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(Scavenger.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + Scavenger.GetRadius())
			{
				// Scavenger collided with a rock.

				KillScavenger(ScavengerIndex);
				KillAsteroid(AsteroidIndex, DontCreditPlayerForKill);

				break;
			}
		}
	}
}


void UPlayViewBase::UpdateTasks(float DeltaTime)
{
	// Iterate backwards so we can safely remove tasks from the array.

	for(int32 Index = ScheduledTasks.Num() - 1; Index >= 0; Index--)
	{
		if(ScheduledTasks[Index].Tick(DeltaTime))
		{
			ScheduledTasks.RemoveAt(Index);
		}
	}

	for(int32 Index = DurationTasks.Num() - 1; Index >= 0; Index--)
	{
		if(!DurationTasks[Index].Tick(DeltaTime))
		{
			DurationTasks.RemoveAt(Index);
		}
	}
}


void UPlayViewBase::UpdateTorpedos(float DeltaTime)
{
	// todo?: fade torpedo from white to black as it gets older, and flicker it.

	for (auto& TorpedoPtr : Torpedos)
	{
		auto& Torpedo = *TorpedoPtr.Get();

		if(Torpedo.IsDead())
		{
			continue;
		}

		Torpedo.LifeRemaining -= DeltaTime;

		if (Torpedo.IsDead())
		{
			Torpedo.Kill();
			continue;
		}

		Torpedo.Move(DeltaTime, WrapPositionToViewport);
	}
}


int32 UPlayViewBase::GetAvailableTorpedo() const
{
	int32 Result = -1;

	for(const auto& TorpedoPtr : Torpedos)
	{
		Result++;

		if(TorpedoPtr->IsDead())
		{
			return Result;
		}
	}

	return INDEX_NONE;
}


void UPlayViewBase::RemovePowerup(int32 PowerupIndex)
{
	if(!Powerups.IsValidIndex(PowerupIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid powerup index %d"), PowerupIndex);
		return;
	}

	Daylon::Destroy(Powerups[PowerupIndex]);

	Powerups.RemoveAt(PowerupIndex);
}


void UPlayViewBase::RemoveEnemyShip(int32 EnemyIndex)
{
	if(!EnemyShips.IsValidIndex(EnemyIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid enemy ship index %d"), EnemyIndex);
		return;
	}

	auto& EnemyShip = *EnemyShips[EnemyIndex].Get();

	if(EnemyShip.Value == ValueBigEnemy)
	{
		NumBigEnemyShips--;
	}
	else
	{
		NumSmallEnemyShips--;
	}

	check(NumBigEnemyShips >= 0 && NumSmallEnemyShips >= 0);

	Daylon::Destroy(EnemyShips[EnemyIndex]);

	EnemyShips.RemoveAt(EnemyIndex);
}


void UPlayViewBase::RemoveEnemyShips()
{
	while(!EnemyShips.IsEmpty())
	{
		RemoveEnemyShip(0);
	}

	while(!Scavengers.IsEmpty())
	{
		RemoveScavenger(0);
	}
}


void UPlayViewBase::RemoveScavenger(int32 ScavengerIndex)
{
	if(!Scavengers.IsValidIndex(ScavengerIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid ScavangerIndex %d"), ScavengerIndex);
		return;
	}

	auto& Scavenger = *Scavengers[ScavengerIndex].Get();

	for(auto PowerupPtr : Scavenger.AcquiredPowerups)
	{
		Daylon::Destroy(PowerupPtr);
	}

	Daylon::Destroy(Scavengers[ScavengerIndex]);

	Scavengers.RemoveAt(ScavengerIndex);
}


void UPlayViewBase::RemoveExplosions()
{
	while(!Explosions.IsEmpty())
	{
		RootCanvas->GetCanvasWidget()->RemoveSlot(Explosions.Last(0).ToSharedRef());
		Explosions.Pop();
	}
}


void UPlayViewBase::RemovePowerups()
{
	while(!Powerups.IsEmpty())
	{
		RemovePowerup(Powerups.Num() - 1);
	}
}


void UPlayViewBase::RemoveTorpedos()
{
	for(auto TorpedoPtr : Torpedos)
	{
		TorpedoPtr->LifeRemaining = 0.0f;
		TorpedoPtr->Hide();
	}
}


void UPlayViewBase::SpawnEnemyShip()
{
	// Generate a big enemy ship vs. small one based on player score.
	// The higher the score, the likelier a small enemy will appear.
	// Regardless of score, there's always a 10% chance of a big enemy ship.

	const int32 ScoreTmp = FMath::Max(0, PlayerScore - 5000);

	float BigEnemyProbability = pow(FMath::Lerp(1.0f, 0.1f,  FMath::Min(1.0f, ScoreTmp / 65'000.0f)), 2.0f);
	BigEnemyProbability = FMath::Max(0.1f, BigEnemyProbability);

	const bool IsBigEnemy = (FMath::FRand() <= BigEnemyProbability);


	auto EnemyShipPtr = FEnemyShip::Create(
		IsBigEnemy ? BigEnemyAtlas : SmallEnemyAtlas, 
		IsBigEnemy ? ValueBigEnemy : ValueSmallEnemy,
		0.375f);


	// Choose a random Y-pos to appear at. Leave room to avoid ship appearing clipped.
	FVector2D P(0.0, FMath::RandRange(EnemyShipPtr->GetSize().Y + 2, ViewportSize.Y - (EnemyShipPtr->GetSize().Y + 2)));

	auto Inertia = FVector2D(1, 0) * 300; // todo: use global constant for speed, maybe min/max it

	if(FMath::RandBool())
	{
		Inertia.X *= -1; // make enemy ship travel from right to left.
		P.X = ViewportSize.X - 1.0f; // avoid immediate removal
	}

	EnemyShipPtr->Spawn(WrapPositionToViewport(P), Inertia, 0.0f);

	EnemyShips.Add(EnemyShipPtr);

	if(IsBigEnemy)
	{
		NumBigEnemyShips++;
	}
	else
	{
		NumSmallEnemyShips++;
	}


	if(IsBigEnemy)
	{
		if(NumBigEnemyShips == 1)
		{
			BigEnemyShipSoundLoop.Start();
		}
	}
	else
	{
		if(NumSmallEnemyShips == 1)
		{
			SmallEnemyShipSoundLoop.Start();
		}
	}
}


void UPlayViewBase::UpdateEnemyShips(float DeltaTime)
{
	check(NumBigEnemyShips + NumSmallEnemyShips == EnemyShips.Num());

	for(int32 ShipIndex = EnemyShips.Num() - 1; ShipIndex >= 0; ShipIndex--)
	{
		auto& EnemyShip = *EnemyShips[ShipIndex].Get();

		// If we've reached the opposite side of the viewport, remove us.
		const auto P2 = WrapPositionToViewport(EnemyShip.UnwrappedNewPosition);

		if(P2.X != EnemyShip.UnwrappedNewPosition.X)
		{
			RemoveEnemyShip(ShipIndex);
			continue;
		}

		EnemyShip.Perform(*this, DeltaTime);
	}

#if(FEATURE_MULTIPLE_ENEMIES == 0)
	if(EnemyShips.IsEmpty())
#endif
	{
		// See if we need to spawn an enemy ship.
		// Time between enemy ship spawns depends on asteroid density (no ships if too many rocks)
		// and while low density, spawn once every ten seconds down to two seconds depending on player score.

		TimeUntilNextEnemyShip -= DeltaTime;

		if(TimeUntilNextEnemyShip <= 0.0f)
		{
			SpawnEnemyShip();

			// Reset the timer.
			TimeUntilNextEnemyShip = FMath::Lerp(MaxTimeUntilEnemyRespawn, MinTimeUntilEnemyRespawn, (float)FMath::Min(PlayerScore, ExpertPlayerScore) / ExpertPlayerScore);
			// score = 10'000 --> 9 seconds
			//         20'000 --> 8 seconds
			//         50'000 --> 5 seconds
			//        100'000+ --> 1 second
			// might want to apply a gamma curve to speed up spawning at lower scores
		}
	}

	if(NumBigEnemyShips > 0)
	{
		BigEnemyShipSoundLoop.Tick(DeltaTime);
	}

	if(NumSmallEnemyShips > 0)
	{
		SmallEnemyShipSoundLoop.Tick(DeltaTime);
	}


#if(FEATURE_SCAVENGERS == 1)
	
	for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		auto& Scavenger = *Scavengers[ScavengerIndex].Get();

		Scavenger.Update(DeltaTime);


		if(Scavenger.CurrentTarget.IsValid())
		{
			// Keep moving toward target.
			Scavenger.Move(DeltaTime, WrapPositionToViewport);
		}
		else
		{
			// No current target.

			if(!Powerups.IsEmpty())
			{
				// Powerups exist, assign ourselves one as a target.
				// Pick the one closest to us.
				// Note that we don't care if multiple scavengers target the same powerup.

				double ShortestDistance = 1.0e7;
				int32 NearestPowerupIndex = 0;
				int32 Index = 0;

				for(auto PowerupPtr : Powerups)
				{
					const auto Distance = FVector2D::Distance(Scavenger.GetPosition(), PowerupPtr.Get()->GetPosition());
					
					if(Distance < ShortestDistance)
					{
						ShortestDistance = Distance;
						NearestPowerupIndex = Index;
					}
					Index++;
				}

				Scavenger.CurrentTarget = Powerups[NearestPowerupIndex];

				// Point the scavenger at the powerup.
				Scavenger.Inertia = (Powerups[NearestPowerupIndex].Get()->GetPosition() - Scavenger.GetPosition());
				Scavenger.Inertia.Normalize();
				Scavenger.Inertia *= MaxScavengerSpeed;
				Scavenger.SetAngle(UDaylonUtils::Vector2DToAngle(Scavenger.Inertia));
			}
			else
			{
				// No target exists and none are available. Just move flat towards edge of sector.
				if(Scavenger.Inertia.Y != 0)
				{
					Scavenger.Inertia = FVector2D(1, 0) * MaxScavengerSpeed;
					Scavenger.SetAngle(UDaylonUtils::Vector2DToAngle(Scavenger.Inertia));
				}

				Scavenger.Move(DeltaTime, WrapPositionToViewport);

				// If we've reached the opposite side of the viewport, remove us.
				const auto P2 = WrapPositionToViewport(Scavenger.UnwrappedNewPosition);

				if(P2.X != Scavenger.UnwrappedNewPosition.X)
				{
					RemoveScavenger(ScavengerIndex);
				}
			}
		}
	} // for all scavengers


	// If there are no scavengers, spawn one if enough time has passed.

	if(Scavengers.IsEmpty())
	{
		TimeUntilNextScavenger -= DeltaTime;

		if(TimeUntilNextScavenger <= 0.0f)
		{
			TimeUntilNextScavenger = MaxTimeUntilNextScavenger;

			auto Scavenger = FScavenger::Create(ScavengerAtlas, FVector2D(32));

			Scavenger->SetPosition(FVector2D(0, FMath::FRandRange(ViewportSize.Y * 0.1, ViewportSize.Y * 0.9)));
			Scavenger->Inertia.Set(MaxScavengerSpeed, 0);
			Scavenger->SetAngle(UDaylonUtils::Vector2DToAngle(Scavenger->Inertia));

			Scavengers.Add(Scavenger);
		}
	}

#endif
}


bool UPlayViewBase::IsPlayerPresent() const
{
	return (PlayerShip && PlayerShip->IsVisible());
}


#if 0
void UPlayViewBase::LaunchTorpedoFromEnemy(const FEnemyShip& Shooter, bool IsBigEnemy)
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!IsPlayerPresent())
	{
		return;
	}

	const int32 TorpedoIdx = GetAvailableTorpedo();

	if(TorpedoIdx == INDEX_NONE)
	{
		return;
	}

	auto& Torpedo = *Torpedos[TorpedoIdx].Get();

	Torpedo.FiredByPlayer = false;

	FVector2D Direction;
	float     Speed;

	// Position torpedo a little outside the ship.
	auto LaunchP = Shooter.OldPosition;

	if(IsBigEnemy)
	{
		// Shot vector is random.
		Direction = UDaylonUtils::RandVector2D();
		LaunchP += Direction * (Shooter.GetSize().X / 2 + 2);
		LaunchP = WrapPositionToViewport(LaunchP);

		Speed = BigEnemyTorpedoSpeed;
	}
	else
	{
		// Small enemy.
		// Shots switch between shooting at an available asteroid or the player.

		Speed = SmallEnemyTorpedoSpeed;

		FVector2D TargetP(-1);
		//const float Time = FMath::RandRange(0.75f, 1.25f);


		bEnemyShootsAtPlayer = !bEnemyShootsAtPlayer;

		if(bEnemyShootsAtPlayer || Asteroids.IsEmpty())
		{
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, PlayerShip->GetPosition(), PlayerShip->Inertia);
		}
		else
		{
			// Shoot at an asteroid.
			const auto& Asteroid = *Asteroids[FMath::RandRange(0, Asteroids.Num() - 1)].Get();
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Asteroid.GetPosition(), Asteroid.Inertia);
		}
	}

	Torpedo.Spawn(LaunchP, Direction * Speed, MaxTorpedoLifeTime);

	PlaySound(TorpedoSound);
}
#endif



void UPlayViewBase::OnFireTorpedo()
{
	// Called when the 'fire torpedo' button is pressed.

	if(!IsPlayerPresent())
	{
		return;
	}

	const FVector2D PlayerFwd = PlayerShip->GetDirectionVector();

	const auto Inertia = (PlayerFwd * MaxTorpedoSpeed) + PlayerShip->Inertia;

	// Position torpedo at nose of player ship.


	int32 TorpedoIndex = GetAvailableTorpedo();

	if(TorpedoIndex == INDEX_NONE)
	{
		return;
	}

	PlaySound(TorpedoSound);

    if(PlayerShip->DoubleShotsLeft == 0)
	{
		// Find an available torpedo, spawn it at the nose of the player ship,
		// and give it an inertia which is player ship intertia + player ship fwd * MaxTorpedoSpeed

		auto& Torpedo = *Torpedos[TorpedoIndex].Get();

		Torpedo.FiredByPlayer = true;

		auto P = PlayerShip->GetPosition();
		P += PlayerFwd * (PlayerShip->GetSize().Y / 2 + /*PlayerShip->Inertia.Length()*/ 2.0); // The last offset is so that the bullet doesn't start off accidentally overlapping the player ship
		P = WrapPositionToViewport(P);

		Torpedo.Spawn(P, Inertia, MaxTorpedoLifeTime);
	}
	else
	{
		AdjustDoubleShotsLeft(-1);

		auto& Torpedo1 = *Torpedos[TorpedoIndex].Get();
		Torpedo1.FiredByPlayer = true;

		auto P = PlayerFwd * (PlayerShip->GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, 90.0f);
		P += PlayerShip->GetPosition();
		//P += PlayerFwd * FMath::RandRange(0.0f, 10.0f);
		P = WrapPositionToViewport(P);

		Torpedo1.Spawn(P, Inertia, MaxTorpedoLifeTime);

		TorpedoIndex = GetAvailableTorpedo();

		if(TorpedoIndex == INDEX_NONE)
		{
			return;
		}

		auto& Torpedo2 = *Torpedos[TorpedoIndex].Get();
		Torpedo2.FiredByPlayer = true;

		P = PlayerFwd * (PlayerShip->GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, -90.0f);
		P += PlayerShip->GetPosition();
		//P += PlayerFwd * FMath::RandRange(0.0f, 10.0f);
		P = WrapPositionToViewport(P);

		Torpedo2.Spawn(P, Inertia, MaxTorpedoLifeTime);
	}
}


void UPlayViewBase::SaveHighScores()
{
	FString Text;

	for(const auto& Entry : HighScores.Entries)
	{
		Text += FString::Format(TEXT("{0} {1}\n"), { Entry.Score, Entry.Name });
	}

	const FString Filespec = FPaths::ProjectSavedDir() / TEXT("highscores.txt");

	if(!FFileHelper::SaveStringToFile(Text, *Filespec))
	{
		UE_LOG(LogGame, Error, TEXT("Could not save high scores to %s"), *Filespec);
	}
	else
	{
		UE_LOG(LogGame, Log, TEXT("High scores saved to %s"), *Filespec);
	}
}


void UPlayViewBase::LoadHighScores()
{
	HighScores.Clear();

#if 0
	HighScores.Add(      0, TEXT("Big loser") );
	HighScores.Add(     50, TEXT("Hey I hit something") );
	HighScores.Add(    200, TEXT("Triple digits, yeah") );
	HighScores.Add(   5000, TEXT("Killstreaker") );
	HighScores.Add(  12345, TEXT("Bonus getter") );
	HighScores.Add( 492000, TEXT("Ace") );
#else
	
	FString Text;
	const FString Filespec = FPaths::ProjectSavedDir() / TEXT("highscores.txt");

	if(!FFileHelper::LoadFileToString(Text, *Filespec))
	{
		return;
	}

	//DebugSavePath->SetVisibility(ESlateVisibility::Visible);
	//DebugSavePath->SetText(FText::FromString(Filespec));


	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);

	for(auto& Line : Lines)
	{
		Line.TrimStartAndEndInline();

		TArray<FString> Parts;
		Line.ParseIntoArrayWS(Parts);

		if(Parts.Num() < 2 || !Parts[0].IsNumeric())
		{
			continue;
		}

		const int32 Score = FCString::Atoi(*Parts[0]);

		// Assemble parts[1..n] into name
		FString Name;

		for(int32 Index = 1; Index < Parts.Num(); Index++)
		{
			Name += Parts[Index];
			Name += TEXT(" ");
		}
		Name.TrimEndInline();

		// Because the uppercase glyphs in the Hyperspace font kern too much,
		// force all highscore names to be lowercase.
		Name.ToLowerInline();

		HighScores.Add(Score, Name);
	}
#endif
}


void UPlayViewBase::PopulateHighScores()
{
	if(HighScoresReadout == nullptr)
	{
		return;
	}

	HighScoresReadout->ClearChildren();

	// We want one line per score.

	int32 RowIndex = 0;
	bool  MruScoreHighlighted = false;

	MostRecentHighScoreTextBlock[0] = nullptr;
	MostRecentHighScoreTextBlock[1] = nullptr;

	for(const auto& Entry : HighScores.Entries)
	{
		auto EntryColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));

		const bool IsMostRecentHighScore = (!MruScoreHighlighted && Entry == MostRecentHighScore);

		if(IsMostRecentHighScore)
		{
			EntryColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}

		auto TextBlockScore = UDaylonUtils::MakeWidget<UTextBlock>();
		TextBlockScore->SetFont(HighScoreReadoutFont);
		TextBlockScore->SetColorAndOpacity(EntryColor);
		TextBlockScore->SetText(FText::FromString(FString::Format(TEXT("{0}"), { Entry.Score })));

		auto ScoreSlot = HighScoresReadout->AddChildToGrid(TextBlockScore);
		ScoreSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		ScoreSlot->SetRow(RowIndex);
		ScoreSlot->SetColumn(0);

		auto TextBlockName = UDaylonUtils::MakeWidget<UTextBlock>();
		TextBlockName->SetFont(HighScoreReadoutFont);
		TextBlockName->SetColorAndOpacity(EntryColor);
		TextBlockName->SetText(FText::FromString(Entry.Name));

		auto NameSlot = HighScoresReadout->AddChildToGrid(TextBlockName);
		NameSlot->SetPadding(FMargin(30.0f, 0.0f, 0.0f, 0.0f));
		NameSlot->SetRow(RowIndex);
		NameSlot->SetColumn(1);

		if(IsMostRecentHighScore)
		{
			MostRecentHighScoreTextBlock[0] = TextBlockScore;
			MostRecentHighScoreTextBlock[1] = TextBlockName;
		}

		RowIndex++;
	}
}


void UPlayViewBase::StopRunning(const FString& Reason, bool bFatal)
{
	if(bFatal)
	{
		UE_LOG(LogSlate, Error, TEXT("%s"), *Reason);
	}
	else
	{
		UE_LOG(LogSlate, Log, TEXT("%s"), *Reason);
	}

	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}


/*
void UPlayViewBase::AnimateStartMessage(float DeltaTime)
{
	const auto T = StartMsgAnimationAge * PI;

	PressStartKeyMessage->SetOpacity(0.5f + sin(T) * 0.5f);

	StartMsgAnimationAge = FMath::Wrap(StartMsgAnimationAge + DeltaTime, 0.0f, 10.0f);
}
*/

#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
