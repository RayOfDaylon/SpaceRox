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

#define FEATURE_MINIBOSS            1



#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif





// ------------------------------------------------------------------------------------------------------------------

/*FVector2D UPlayViewBase::MakeInertia(const FVector2D& InertiaOld, float MinDeviation, float MaxDeviation)
{
	return UDaylonUtils::Rotate(InertiaOld, FMath::RandRange(MinDeviation, MaxDeviation));
}*/


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

	PlayerShip->DoubleShotsLeft   .Bind([this](int32){ UpdatePlayerShipReadout(EPowerup::DoubleGuns);    });
	PlayerShip->ShieldsLeft       .Bind([this](int32){ UpdatePlayerShipReadout(EPowerup::Shields);       });
	PlayerShip->InvincibilityLeft .Bind([this](float){ UpdatePlayerShipReadout(EPowerup::Invincibility); });

	PlayerShip->Initialize(*this);
}


void UPlayViewBase::CreateTorpedos()
{
	for(int32 Index = 0; Index < TorpedoCount; Index++)
	{
		auto TorpedoPtr = FTorpedo::Create(TorpedoAtlas, 0.5f);

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


void UPlayViewBase::InitializeSoundLoops()
{
	PlayerShipThrustSoundLoop.Set (this, ThrustSound);
	BigEnemyShipSoundLoop.Set     (this, EnemyShipBigSound);
	SmallEnemyShipSoundLoop.Set   (this, EnemyShipSmallSound);
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

		TSharedPtr<FAnimSpriteCel> Cel = SNew(FAnimSpriteCel);
		Daylon::Install<SImage>(Cel, 0.5f);
		Cel->Init(TitleSheet, SrcUV, SrcPx, DstPx, Index * 0.2f, MaxIntroStateLifetime / 8);

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
	float When,
	const FVector2D& P, 
	const FVector2D& Inertia, 
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
	Daylon::FScheduledTask Task;

	Task.When = When;

	Task.What = [P, Inertia, MinParticleSize, MaxParticleSize, MinParticleVelocity, MaxParticleVelocity, MinParticleLifetime, MaxParticleLifetime, FinalOpacity, NumParticles,
                 ArenaPtr = TWeakObjectPtr<UPlayViewBase>(this)]()
	{
		if(!ArenaPtr.IsValid() || !ArenaPtr->CanExplosionOccur())
		{
			return;
		}

		ArenaPtr->GetExplosions().SpawnOne(*ArenaPtr.Get(), P, 
			MinParticleSize,
			MaxParticleSize,
			MinParticleVelocity,
			MaxParticleVelocity,
			MinParticleLifetime,
			MaxParticleLifetime,
			FinalOpacity,
			NumParticles,
			Inertia);
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



void UPlayViewBase::OnAbortButtonPressed()
{
	if(PlayerShip)
	{
		PlayerShip->ReleaseResources(*this);
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
			
			Explosions.RemoveAll       (*this);
			ShieldExplosions.RemoveAll (*this);
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
				PlayerShip->InitializeDefenses (*this);
			}

			if(Torpedos.IsEmpty())
			{
				CreateTorpedos         ();
			}

			InitializeScore           ();
			InitializePlayerShipCount ();
			PlayerShip->Initialize    (*this);

			Asteroids.RemoveAll();
			EnemyShips.RemoveAll();
			RemovePowerups            ();

			WaveNumber = 0;

			UDaylonUtils::Show  (PlayerScoreReadout);
			UDaylonUtils::Show  (PlayerShipsReadout);
			UDaylonUtils::Show  (PowerupReadouts);

			PlayerShip->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);


#if(FEATURE_MINIBOSS == 1)
			{
				EnemyShips.SpawnBoss(*this);
				//EnemyBoss = FEnemyBoss::Create(PlayerShipAtlas, 32, ValueMiniBoss, 3, BossShieldSpinSpeed);
				//EnemyBoss->SetPosition(FVector2D(400, 400));
				//EnemyBoss->Show();

				/*
				PolyShieldPtrs.Add(SNew(SDaylonPolyShield)
					.NumSides(9)
					.Thickness(3)
					.Size(100)
					.SpinSpeed(30.0f));

				PolyShieldPtrs.Add(SNew(SDaylonPolyShield)
					.NumSides(6)
					.Thickness(3)
					.Size(70)
					.SpinSpeed(-30.0f));

				int Index = 0;
				for(auto& PolyShieldPtr : PolyShieldPtrs)
				{
					auto SlotArgs = RootCanvas->GetCanvasWidget()->AddSlot();
					SlotArgs[PolyShieldPtr.ToSharedRef()];
					SlotArgs.Anchors(FAnchors());
					SlotArgs.AutoSize(true);
					SlotArgs.Alignment(FVector2D(0.5));
					FMargin Margin;
					Margin.Left = 250.0f;
					Margin.Top = 250.0f;
					Margin.Right = Margin.Bottom = (Index == 0 ? 100 : 70);
					SlotArgs.GetSlot()->SetOffset(Margin);

					PolyShieldPtr->SetRenderTransform(FSlateRenderTransform(FVector2D(0.5)));
					UDaylonUtils::Show(PolyShieldPtr.Get());
					Index++;
				}*/
			}
#endif

			break;


		case EGameState::Over:

			if(PreviousState != EGameState::Active)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering game over state"), (int32)PreviousState);
			}

			PlayerShip->ReleaseResources(*this);
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
			TitleGraphic->SetOpacity(0.0f);


			// Fade in the title graphic and version number while explosions rage

			if(TitleCels.IsEmpty())
			{
				InitializeTitleGraphics();
			}

			{
				static FBox2d Box(FVector2D(480, 300), FVector2D(1500, 550));

				if(TimeUntilIntroStateEnds > 0.0f)
				{
#if 1
					for(auto& CelPtr : TitleCels)
					{
						CelPtr->Update(InDeltaTime);
					}
#endif
					// TitleGraphic->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 1.5f) / MaxIntroStateLifetime));

					VersionReadout->SetOpacity(FMath::Max(0.0f, 1.0f - (TimeUntilIntroStateEnds * 3.0f) / MaxIntroStateLifetime));

					ExploCountAge -= InDeltaTime;
				
					if(ExploCountAge <= 0.0f)
					{
						ExploCountAge = FMath::FRandRange(0.05f, 0.2f);

						Explosions.SpawnOne(*this,
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

			Explosions.Update(*this, WrapPositionToViewport, InDeltaTime);

			TimeUntilIntroStateEnds -= InDeltaTime;

			break;


		case EGameState::MainMenu:

			Asteroids.Update          (*this, InDeltaTime);
			UpdatePowerups            (InDeltaTime);

			break;


		case EGameState::Active:

			if(IsPlayerPresent())
			{
				PlayerShip->Perform     (*this, InDeltaTime);
			}

			EnemyShips.Update         (*this, InDeltaTime);
			Asteroids.Update          (*this, InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			Explosions.Update         (*this, WrapPositionToViewport, InDeltaTime);
			ShieldExplosions.Update   (*this, WrapPositionToViewport, InDeltaTime);

			CheckCollisions();

			ProcessWaveTransition     (InDeltaTime);

			if(PlayerShip && PlayerShip->IsSpawning)
			{
				ProcessPlayerShipSpawn    (InDeltaTime);
			}

			break;


		case EGameState::Over:

			EnemyShips.Update         (*this, InDeltaTime);
			Asteroids.Update          (*this, InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			Explosions.Update         (*this, WrapPositionToViewport, InDeltaTime);
			ShieldExplosions.Update   (*this, WrapPositionToViewport, InDeltaTime);

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

			Asteroids.Update          (*this, InDeltaTime);
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
			Asteroids.Update          (*this, InDeltaTime);
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

	return (!ImageObjectsIntersectBox(Asteroids.Asteroids,   SafeZone) && 
	        !ImageObjectsIntersectBox(EnemyShips.Ships,      SafeZone) &&
			!ImageObjectsIntersectBox(EnemyShips.Scavengers, SafeZone));
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
	Explosions.SpawnOne(*this, P, Inertia);
}


void UPlayViewBase::PlaySound(USoundBase* Sound, float VolumeScale)
{
	UGameplayStatics::PlaySound2D(this, Sound, VolumeScale);
}


void UPlayViewBase::KillPlayerShip()
{
	PlayerShip->SpawnExplosion(*this);

	PlaySound(PlayerShipDestroyedSound);

	PlayerShip->Hide();

	AddPlayerShips(-1);

	PlayerShip->IsSpawning = true;
	TimeUntilNextPlayerShip = MaxTimeUntilNextPlayerShip;

	// ProcessPlayerShipSpawn() will handle the wait til next spawn and transition to game over, if needed. 
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

	PlayerScore = FMath::Min(MaxPlayerScore, PlayerScore + Amount);

	if(PrevLevel != PlayerScore / PlayerShipBonusAt)
	{
		AddPlayerShips(1);

		PlaySound(PlayerShipBonusSound);
	}
}


void UPlayViewBase::UpdatePlayerScoreReadout()
{
	PlayerScoreReadout->SetText(FText::FromString(FString::Format(TEXT("{0}"), { PlayerScore.GetValue() })));
}


void UPlayViewBase::UpdateRoundedReadout(UTextBlock* Readout, float Value, int32& OldValue)
{
	check(Readout);

	const int32 N = FMath::RoundToInt(Value);

	if(N != OldValue)
	{
		const auto Str = FString::Printf(TEXT("%d"), N);
		Readout->SetText(FText::FromString(Str));
		OldValue = N;
	}
}


void UPlayViewBase::UpdatePlayerShipReadout(EPowerup PowerupKind)
{
	FString Str;

	static int32 ShieldsLeft       = -10;
	static int32 InvincibilityLeft = -10;

	switch(PowerupKind)
	{
		case EPowerup::DoubleGuns:
			Str = FString::Printf(TEXT("%d"), PlayerShip->DoubleShotsLeft.GetValue());
			DoubleGunReadout->SetText(FText::FromString(Str));
			break;

		case EPowerup::Shields:
			UpdateRoundedReadout(PlayerShieldReadout, PlayerShip->ShieldsLeft, ShieldsLeft);
			break;

		case EPowerup::Invincibility:
			UpdateRoundedReadout(InvincibilityReadout, PlayerShip->InvincibilityLeft, InvincibilityLeft);
			break;
	}
}




void UPlayViewBase::ProcessPlayerCollision()
{
	check(PlayerShip);

	if(!PlayerShip->ProcessCollision(*this))
	{
		KillPlayerShip();
	}
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


		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, Asteroid.GetPosition(), Asteroid.GetRadius())
				|| UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, OldP, Asteroid.GetRadius()))
			{
				// A torpedo hit a rock.

				Torpedo.Kill();
				Asteroids.Kill(*this, AsteroidIndex, Torpedo.FiredByPlayer);

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
				SpawnExplosion(OldP, PlayerShip->Inertia);
				ProcessPlayerCollision();
			}
		}

		if(Torpedo.IsAlive() && Torpedo.FiredByPlayer)
		{
			// See if the torpedo hit an enemy ship.

			for(int32 EnemyIndex = EnemyShips.NumShips() - 1; EnemyIndex >= 0; EnemyIndex--)
			{
				auto& EnemyShip = EnemyShips.GetShip(EnemyIndex);

				if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, EnemyShip.GetPosition(), EnemyShip.GetRadius()))
				{
					Torpedo.Kill();
					IncreasePlayerScoreBy(EnemyShip.Value);
					EnemyShips.KillShip(*this, EnemyIndex);
					break;
				}
			}

			// See if the torpedo hit a scavenger.
			if(Torpedo.IsAlive())
			{
				for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
				{
					auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

					if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, Scavenger.GetPosition(), Scavenger.GetRadius()))
					{
						Torpedo.Kill();
						IncreasePlayerScoreBy(Scavenger.Value);
						EnemyShips.KillScavenger(*this, ScavengerIndex);
						break;
					}
				}
			}
		}

#if(FEATURE_MINIBOSS == 1)
		// Let bosses be hit by any torpedo, not just ours.
		if(Torpedo.IsAlive() /* && Torpedo.FiredByPlayer*/)
		{
			int32 ShieldSegmentIndex;
			int32 Part;
				
			for(int32 BossIndex = EnemyShips.NumBosses() - 1; BossIndex >= 0; BossIndex--)
			{
				auto& Boss = EnemyShips.GetBoss(BossIndex);

				Part = Boss.CheckCollision(OldP, CurrentP, ShieldSegmentIndex);

				if(Part != INDEX_NONE)
				{
					Torpedo.Kill();

					if(Part == 0) 
					{
						if(Torpedo.FiredByPlayer)
						{
							IncreasePlayerScoreBy(Boss.Value);
						}
						EnemyShips.KillBoss(*this, BossIndex);
					} 
					else
					{
						SpawnExplosion(CurrentP, FVector2D(0));
						PlaySound(ShieldBonkSound, 0.5f);
						float PartHealth = Boss.GetShieldSegmentHealth(Part, ShieldSegmentIndex);
						PartHealth = FMath::Max(0.0f, PartHealth - 0.25f);
						Boss.SetShieldSegmentHealth(Part, ShieldSegmentIndex, PartHealth);
					}

					break;
				}
			}
		}
#endif
	} // next torpedo


	for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		// Did a scavenger collide with a powerup?

		auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

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
				Powerups.RemoveAtSwap(PowerupIndex);

				Scavenger.CurrentTarget.Reset();

				break;
			}
			PowerupIndex++;
		}
	}


	// Check if player ship collided with a rock

	if(IsPlayerPresent())
	{
		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Asteroid.GetPosition(), Asteroid.GetRadius() + PlayerShip->GetRadius())
				|| UDaylonUtils::DoesLineSegmentIntersectTriangle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, PlayerShipTriangle))
			{
				// Player collided with a rock.

				ProcessPlayerCollision();
				// todo: should we credit player if he used his shields?
				Asteroids.Kill(*this, AsteroidIndex, CreditPlayerForKill);

				break;
			}
		}
	}


	// Check if enemy ship collided with the player

	if(IsPlayerPresent())
	{
		for(int32 EnemyIndex = EnemyShips.NumShips() - 1; EnemyIndex >= 0; EnemyIndex--)
		{
			auto& EnemyShip = EnemyShips.GetShip(EnemyIndex);

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, EnemyShip.OldPosition, EnemyShip.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, EnemyShip.UnwrappedNewPosition) < EnemyShip.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(EnemyShip.Value);
				EnemyShips.KillShip(*this, EnemyIndex);

				ProcessPlayerCollision();

				break;
			}
		}
	}


	// Check if scavenger collided with the player

	if(IsPlayerPresent())
	{
		for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
		{
			auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Scavenger.OldPosition, Scavenger.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, Scavenger.UnwrappedNewPosition) < Scavenger.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(Scavenger.Value);
				EnemyShips.KillScavenger(*this, ScavengerIndex);

				ProcessPlayerCollision();

				break;
			}
		}
	}


#if(FEATURE_MINIBOSS == 1)
	// Check if player ship collided with a boss

	if(IsPlayerPresent())
	{
		int32 Part;
		int32 ShieldSegmentIndex;
		FVector2D HitPt;

		for(int32 BossIndex = EnemyShips.NumBosses() - 1; BossIndex >= 0; BossIndex--)
		{
			auto& Boss = EnemyShips.GetBoss(BossIndex);

			Part = Boss.CheckCollision(PlayerShipLineStart, PlayerShipLineEnd, PlayerShip->GetRadius(), ShieldSegmentIndex, HitPt);

			if(Part == INDEX_NONE)
			{
				continue;
			}
			
			ProcessPlayerCollision();

			// Player will have died if it wasn't shielded.

			if(Part == 0) 
			{ 
				IncreasePlayerScoreBy(Boss.Value);
				EnemyShips.KillBoss(*this, BossIndex);
				break;
			} 

			// Player hit a boss' shield.

			SpawnExplosion(PlayerShipLineEnd, FVector2D(0));
			PlaySound(ShieldBonkSound, 0.5f);
			float PartHealth = Boss.GetShieldSegmentHealth(Part, ShieldSegmentIndex);
			PartHealth = FMath::Max(0.0f, PartHealth - 0.25f);
			Boss.SetShieldSegmentHealth(Part, ShieldSegmentIndex, PartHealth);

			if(IsPlayerPresent())
			{
				// Player was shielded or invincible (or in god mode)
				// so do an elastic collision.

				auto ShieldImpactNormal = (HitPt - Boss.UnwrappedNewPosition);
				ShieldImpactNormal.Normalize();

				auto BounceForce = ShieldImpactNormal * PlayerShip->GetSpeed();

				PlayerShip->Inertia += BounceForce;

				while(PlayerShip->GetSpeed() < 100.0f)
				{
					PlayerShip->Inertia *= 1.1f;
				}

				// Move the player ship away from the boss to avoid overcolliding.
				while(FVector2D::Distance(PlayerShip->UnwrappedNewPosition, HitPt) < 20.0f)
				{
					PlayerShip->Move(1.0f / 60, WrapPositionToViewport);
				}
			}
			break;
		}
	}
#endif // #if(FEATURE_MINIBOSS == 1)


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

						PlayerShip->AdjustDoubleShotsLeft(DoubleGunsPowerupIncrease);
						PlaySound(GainDoubleGunPowerupSound);
						PlayAnimation(DoubleGunReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;

					case EPowerup::Shields:

						PlayerShip->AdjustShieldsLeft(ShieldPowerupIncrease);
						PlaySound(GainShieldPowerupSound);
						PlayAnimation(ShieldReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;

					case EPowerup::Invincibility:
						PlaySound(GainShieldPowerupSound); // todo: specific sound
						PlayerShip->AdjustInvincibilityLeft(MaxInvincibilityTime);
						PlayerShip->TimeUntilNextInvincibilityWarnFlash = MaxInvincibilityWarnTime;
						PlayAnimation(InvincibilityReadoutFlash, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
						break;
				}
				
				KillPowerup(PowerupIndex);

				break;
			}
		}
	}


	// Check if enemy ship collided with an asteroid

	for(int32 EnemyIndex = EnemyShips.NumShips() - 1; EnemyIndex >= 0; EnemyIndex--)
	{
		auto& EnemyShip = EnemyShips.GetShip(EnemyIndex);

		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, EnemyShip.GetPosition(), EnemyShip.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(EnemyShip.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + EnemyShip.GetRadius())
			{
				// Enemy ship collided with a rock.

				EnemyShips.KillShip(*this, EnemyIndex);
				Asteroids.Kill(*this, AsteroidIndex, DontCreditPlayerForKill);

				break;
			}
		}
	}

	for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, Scavenger.GetPosition(), Scavenger.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(Scavenger.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + Scavenger.GetRadius())
			{
				// Scavenger collided with a rock.

				EnemyShips.KillScavenger(*this, ScavengerIndex);
				Asteroids.Kill(*this, AsteroidIndex, DontCreditPlayerForKill);

				break;
			}
		}
	}


	for(int32 BossIndex = EnemyShips.NumBosses() - 1; BossIndex >= 0; BossIndex--)
	{
		auto& Boss = EnemyShips.GetBoss(BossIndex);

		int32 ShieldSegmentIndex;
		FVector2D HitPt;

		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			auto Part = Boss.CheckCollision(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, Asteroid.GetRadius(), ShieldSegmentIndex, HitPt);

			if(Part == INDEX_NONE)
			{
				continue;
			}

			Asteroids.Kill(*this, AsteroidIndex, DontCreditPlayerForKill);

			if(Part == 0)
			{
				// Asteroid hit boss center.
				EnemyShips.KillBoss(*this, BossIndex);
				break;
			}

			// Asteroid collided with a boss shield.
			// The bigger the asteroid, the greater the health impact.
			float HealthDrop = 1.0f;

			switch(Asteroid.Value)
			{
				case ValueMediumAsteroid: HealthDrop = 0.50f; break;
				case ValueSmallAsteroid:  HealthDrop = 0.25f; break;
			}

			// The faster the asteroid was moving, the greater the health impact.
			HealthDrop = FMath::Min(1.0f, HealthDrop * Asteroid.GetSpeed() / 200);
			SpawnExplosion(PlayerShipLineEnd, FVector2D(0));
			PlaySound(ShieldBonkSound, 0.5f);
			float PartHealth = Boss.GetShieldSegmentHealth(Part, ShieldSegmentIndex);
			PartHealth = FMath::Max(0.0f, PartHealth - HealthDrop);
			Boss.SetShieldSegmentHealth(Part, ShieldSegmentIndex, PartHealth);

			break;
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


bool UPlayViewBase::IsPlayerPresent() const
{
	return (PlayerShip && PlayerShip->IsVisible());
}


void UPlayViewBase::OnFireTorpedo()
{
	// Called when the 'fire torpedo' button is pressed.

	if(!IsPlayerPresent())
	{
		return;
	}

	PlayerShip->FireTorpedo(*this);
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
