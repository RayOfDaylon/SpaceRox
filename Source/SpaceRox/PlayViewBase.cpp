// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Constants.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

/*
#include "UDaylonParticlesWidget.h"
#include "UMG/Public/Blueprint/WidgetLayoutLibrary.h"
#include "UMG/Public/Blueprint/WidgetBlueprintLibrary.h"
#include "UMG/Public/Animation/WidgetAnimation.h"
#include "Runtime/Core/Public/Logging/LogMacros.h"
*/

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);

DEFINE_LOG_CATEGORY(LogGame)

// Set to 1 to enable debugging
#define DEBUG_MODULE                0

// Make wave start with three rocks (big/medium/small) at rest.
#define TEST_ASTEROIDS              0

#define FEATURE_MINIBOSS            1



#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



FVector2D UPlayViewBase::WrapPositionToViewport(const FVector2D& P)
{
	return FVector2D(UKismetMathLibrary::FWrap(P.X, 0.0, ViewportSize.X), UKismetMathLibrary::FWrap(P.Y, 0.0, ViewportSize.Y));
}


void UPlayViewBase::InitializeScore()
{
	PlayerScore = (StartingScore > 0 ? StartingScore : 0);

	UpdatePlayerScoreReadout();
}


void UPlayViewBase::CreateTorpedos()
{
	for(int32 Index = 0; Index < TorpedoCount; Index++)
	{
		auto TorpedoPtr = FTorpedo::Create(TorpedoAtlas->Atlas, 0.5f);

		TorpedoPtr->Inertia = FVector2D(0);
		TorpedoPtr->LifeRemaining = 0.0f;
		TorpedoPtr->Hide();

		Torpedos.Add(TorpedoPtr);
	}
}


void UPlayViewBase::InitializeVariables()
{
	bHighScoreWasEntered          = false;

	PlayerScore.Bind([this](int32){ this->UpdatePlayerScoreReadout(); });

	PlayerScore                   = 0;
	WaveNumber                    = 0;

	ThrustSoundTimeRemaining      = 0.0f;
	StartMsgAnimationAge          = 0.0f;
	TimeUntilNextWave             = 0.0f;
	TimeUntilNextPlayerShip       = 0.0f;
	TimeUntilNextEnemyShip        = 0.0f;
	TimeUntilNextBoss             = 0.0f;
	TimeUntilGameOverStateEnds    = 0.0f;
	MruHighScoreAnimationAge      = 0.0f;
	TimeUntilNextScavenger        = 5.0f;

	Asteroids.Arena               =
	Explosions.Arena              = 
	ShieldExplosions.Arena        = 
	EnemyShips.Arena              = this;

	Explosions.InertialFactor       = ExplosionInertialFactor;
	ShieldExplosions.InertialFactor = ExplosionInertialFactor;

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


void UPlayViewBase::InitializeTitleGraphics()
{
	// Location and size, in px.
	FBox2d SrcUVs[] =
	{
		{{   0.,  0.},   {   87.,   85. }  },
		{{  91.,  0.},   {   96. ,  85. }  },
		{{ 194.,  0.},   {   80. ,  85. }  },
		{{ 281.,  0.},   {   73. ,  85. }  },
		{{ 281.,  0.},   {   73. ,  85. }  },
		{{ 359.,  0.},   {  105. ,  85. }  },
		{{ 470.,  0.},   {   91. ,  85. }  },
		{{   0., 88.},   {  127. , 107. }  },
		{{ 131., 88.},   {  130. , 107. }  },
		{{ 263., 87.},   {  123. , 108. }  },
		{{ 388., 88.},   {  111. , 107. }  },
		{{ 501., 88.},   {   98. , 107. }  },
		{{   0., 88.},   {  127. , 107. }  },
	};

	// Location and size, in px.
	FInt32Rect DstPxs[] =
	{
		{{  456 , 315}, {  93 ,  88  }},
		{{  553 , 315}, { 102 ,  88  }},
		{{  662 , 315}, {  85 ,  88  }},
		{{  764 , 315}, {  78 ,  88  }},
		{{  851 , 315}, {  74 ,  88  }},
		{{  925 , 315}, { 111 ,  88  }},
		{{ 1045 , 315}, {  96 ,  88  }},
		{{  731 , 441}, { 117 ,  96  }},
		{{  858 , 426}, { 138 , 110  }},
		{{  969 , 428}, { 112 ,  97  }},
		{{ 1091 , 425}, { 118 , 111  }},
		{{ 1227 , 445}, {  87 ,  92  }},
		{{ 1328 , 432}, { 134 , 110  }},
	};

	int32 Index = 0;
	for(auto& SrcUV : SrcUVs)
	{
		// Convert from px to UV position.
		SrcUV.Min /= TitleSheet.GetImageSize();
		// Convert cel size from px to UV size and location.
		SrcUV.Max = SrcUV.Max / TitleSheet.GetImageSize() + SrcUV.Min;

		// For now, make all the source px rects the same, coming from the center.
		FInt32Rect SrcPx(ViewportSize.X / 2, ViewportSize.Y / 2, ViewportSize.X / 2, ViewportSize.Y / 2);

		FInt32Rect DstPx = DstPxs[Index];
		DstPx.Max += DstPx.Min;

		TSharedPtr<Daylon::FAnimSpriteCel> Cel = SNew(Daylon::FAnimSpriteCel);
		Daylon::Install<SImage>(Cel, 0.5f);

		Cel->Init(TitleSheet, SrcUV, SrcPx, DstPx, Index * 0.2f, MaxIntroStateLifetime / 8,
			[WeakThis = TWeakObjectPtr<UPlayViewBase>(this)](Daylon::FAnimSpriteCel&)
			{ if(auto This = WeakThis.Get()) { This->PlaySound(This->GetTorpedoSound()); } }
			);

		TitleCels.Add(Cel);

		Index++;
	}
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

	TransitionToState(EGameState::Intro);

	LoadHighScores();

	IsInitialized = true;
}


void UPlayViewBase::ScheduleExplosion
(
	float                         When,
	const FVector2D&              P, 
	const FVector2D&              Inertia, 
	const FDaylonParticlesParams& Params
)
{
	Daylon::FScheduledTask Task;

	Task.When = When;

	Task.What = [P, Inertia, Params, ArenaPtr = TWeakObjectPtr<UPlayViewBase>(this)]()
	{
		if(!ArenaPtr.IsValid() || !ArenaPtr->CanExplosionOccur())
		{
			return;
		}

		ArenaPtr->GetExplosions().SpawnOne(P, Params, Inertia);
	};

	AddScheduledTask(Task);
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


void UPlayViewBase::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	if(Animation == MenuOutro)
	{
		UDaylonUtils::Hide(MenuContent);
		ExecuteMenuItem(SelectedMenuItem);
		GetOwningPlayer()->EnableInput(nullptr);
	}
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
			Asteroids.RemoveAll();

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
				for(auto& CelPtr : TitleCels)
				{
					Daylon::Uninstall(CelPtr);
				}
				TitleCels.Empty();
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
			
			Explosions.RemoveAll       ();
			ShieldExplosions.RemoveAll ();
			RemoveTorpedos             ();
			EnemyShips.RemoveAll       ();
			RemovePowerups             ();

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
			TimeUntilNextBoss      = 22.0f;
			TimeUntilNextWave      =  2.0f;

			if(!PlayerShip)
			{
				CreatePlayerShip               ();
				PlayerShip->InitializeDefenses ();
			}

			if(Torpedos.IsEmpty())
			{
				CreateTorpedos         ();
			}

			PlayerShip->Initialize    (this);
			InitializeScore           ();
			InitializePlayerShipCount ();

			Asteroids.RemoveAll();
			EnemyShips.RemoveAll();
			RemovePowerups            ();

			WaveNumber = 0;

			UDaylonUtils::Show  (PlayerScoreReadout);
			UDaylonUtils::Show  (PlayerShipsReadout);
			UDaylonUtils::Show  (PowerupReadouts);

			PlayerShip->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);


#if(FEATURE_MINIBOSS == 1)
			EnemyShips.SpawnBoss();
#endif

			break;


		case EGameState::Over:

			if(PreviousState != EGameState::Active)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering game over state"), (int32)PreviousState);
			}

			PlayerShip->ReleaseResources();
			Daylon::Uninstall(PlayerShip);
			PlayerShip.Reset();


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

			EnemyShips.RemoveAll();

			break;


		case EGameState::HighScoreEntry:

			if(PreviousState != EGameState::Over)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering high score entry state"), (int32)PreviousState);
			}

			Asteroids.RemoveAll();
			EnemyShips.RemoveAll();
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

			EnemyShips.RemoveAll();
			RemovePowerups   ();

			UDaylonUtils::Show(CreditsContent);

			break;


		case EGameState::Help:
			
			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering help state"), (int32)PreviousState);
			}

			Asteroids.RemoveAll();
			EnemyShips.RemoveAll();
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
			//TitleGraphic->SetOpacity(0.0f);


			// Fade in the title graphic and version number while explosions rage

			if(TitleCels.IsEmpty())
			{
				InitializeTitleGraphics();
			}

			{
				static FBox2d Box(FVector2D(480, 300), FVector2D(1500, 550));

				if(TimeUntilIntroStateEnds > 0.0f)
				{
					for(auto& CelPtr : TitleCels)
					{
						CelPtr->Update(InDeltaTime);
					}

					// TitleGraphic->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 1.5f) / MaxIntroStateLifetime));

					VersionReadout->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 3.0f) / MaxIntroStateLifetime));

					ExploCountAge -= InDeltaTime;
				
					if(ExploCountAge <= 0.0f)
					{
						ExploCountAge = Daylon::FRandRange(0.05f, 0.2f);

						Explosions.SpawnOne(UDaylonUtils::RandomPtWithinBox(Box), IntroExplosionParams);

						if(Daylon::RandRange(0, 5) == 0)
						{
							PlaySound(PlayerShipDestroyedSound);
						}
						else
						{
							PlaySound(ExplosionSounds[Daylon::RandRange(0, ExplosionSounds.Num() - 1)]);
						}
					}
				}
			}

			Explosions.Update(WrapPositionToViewport, InDeltaTime);

			TimeUntilIntroStateEnds -= InDeltaTime;

			break;


		case EGameState::MainMenu:

			Asteroids.Update          (InDeltaTime);
			UpdatePowerups            (InDeltaTime);

			break;


		case EGameState::Active:

			if(IsPlayerShipPresent())
			{
				PlayerShip->Perform     (InDeltaTime);
			}

			EnemyShips.Update         (InDeltaTime);
			Asteroids.Update          (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			Explosions.Update         (WrapPositionToViewport, InDeltaTime);
			ShieldExplosions.Update   (WrapPositionToViewport, InDeltaTime);

			CheckCollisions();

			ProcessWaveTransition     (InDeltaTime);

			if(PlayerShip && PlayerShip->IsSpawning)
			{
				ProcessPlayerShipSpawn    (InDeltaTime);
			}

			break;


		case EGameState::Over:

			EnemyShips.Update         (InDeltaTime);
			Asteroids.Update          (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			Explosions.Update         (WrapPositionToViewport, InDeltaTime);
			ShieldExplosions.Update   (WrapPositionToViewport, InDeltaTime);

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

			Asteroids.Update          (InDeltaTime);
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

			//EnemyShips.Update       (InDeltaTime);
			Asteroids.Update          (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			break;


		case EGameState::HighScoreEntry:
			break;
	}
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


void UPlayViewBase::SpawnPowerup(TSharedPtr<FPowerup>& PowerupPtr, const FVector2D& P)
{
	auto PowerupKind = (EPowerup)Daylon::RandRange(1, 3);

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

	PowerupPtr = FPowerup::Create(Atlas->Atlas, FVector2D(32));

	auto& Powerup = *PowerupPtr.Get();

	Powerup.Kind = PowerupKind;

	//Powerup.Show();
	Powerup.SetPosition(P);
	Powerup.Inertia.Set(0, 0);
}


void UPlayViewBase::SpawnAsteroids(int32 NumAsteroids)
{
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

		if(Daylon::RandBool())
		{
			P.X = Daylon::FRandRange(0.0, ViewportSize.X);
		}
		else
		{
			P.Y = Daylon::FRandRange(0.0, ViewportSize.Y);
		}
#endif


#if(TEST_ASTEROIDS==1)
		const auto Inertia = FVector2D(0);
#else
		const auto Inertia = UDaylonUtils::RandVector2D() * Daylon::FRandRange(MinAsteroidSpeed, MaxAsteroidSpeed);
#endif

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

		auto Asteroid = FAsteroid::Create(this, AsteroidAtlas->Atlas);
		Asteroid->Value = AsteroidValue;
		Asteroid->LifeRemaining = 1.0f;
		Asteroid->SpinSpeed = Daylon::FRandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);

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

	Asteroids.RemoveAll(); // to be safe
	SpawnAsteroids(NumAsteroids);

	TimeUntilNextEnemyShip = MaxTimeUntilNextEnemyShip; 
	TimeUntilNextBoss      = MaxTimeUntilNextBoss;

	
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


void UPlayViewBase::UpdatePowerups(float DeltaTime)
{
	// This will move any free-floating powerups.

	for(auto& Powerup : Powerups)
	{
		Powerup->Move(DeltaTime, WrapPositionToViewport);
		Powerup->Update(DeltaTime);
	}
}


void UPlayViewBase::SpawnExplosion(const FVector2D& P, const FVector2D& Inertia)
{
	Explosions.SpawnOne(P, Inertia);
}


void UPlayViewBase::KillPowerup(int32 PowerupIndex)
{
	if(!Powerups.IsValidIndex(PowerupIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid powerup index %d"), PowerupIndex);
		return;
	}

	auto& Powerup = Powerups[PowerupIndex];

	RemovePowerup(PowerupIndex);
}


void UPlayViewBase::UpdateTasks(float DeltaTime)
{
	// Iterate backwards so we can safely remove tasks from the array.

	for(int32 Index = ScheduledTasks.Num() - 1; Index >= 0; Index--)
	{
		if(ScheduledTasks[Index].Tick(DeltaTime))
		{
			ScheduledTasks.RemoveAtSwap(Index);
		}
	}

	for(int32 Index = DurationTasks.Num() - 1; Index >= 0; Index--)
	{
		if(!DurationTasks[Index].Tick(DeltaTime))
		{
			DurationTasks.RemoveAtSwap(Index);
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


TSharedPtr<FTorpedo> UPlayViewBase::GetAvailableTorpedo()
{
	auto Index = GetIndexOfAvailableTorpedo();

	return (Index == INDEX_NONE ? nullptr : Torpedos[Index]);
}



int32 UPlayViewBase::GetIndexOfAvailableTorpedo() const
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

	Daylon::Uninstall(Powerups[PowerupIndex]);

	Powerups.RemoveAt(PowerupIndex);
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
