// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "DaylonUtils.h"
#include "UDaylonParticlesWidget.h"

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

#define DEBUG_MODULE      0

// Make wave start with three rocks (big/medium/small) at rest.
#define TEST_ASTEROIDS              0

#define FEATURE_SPINNING_ASTEROIDS  1

#define FEATURE_MULTIPLE_ENEMIES    1


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif

// Constants.
// todo: put them where they can be easily modded at design time.

const FVector2D ViewportSize               = FVector2D(1920, 1080);
								           
const bool  CreditPlayerForKill            = true;
const bool  DontCreditPlayerForKill        = !CreditPlayerForKill;
const bool  ReasonIsFatal                  = true;

const int32 MaxPlayerScore                 = 9'999'990;
const int32 ExpertPlayerScore              = 100'000;  // The score at which e.g. enemy ships will respawn fastest.
const int32 InitialPlayerShipCount         =  3;
const int32 PlayerShipBonusAt              = 10000;
const int32 MaxPlayerShipsDisplayable      = 10;     // We don't want the player ships readout to be impractically wide.
const float MaxTimeUntilNextPlayerShip     =  4.0f;  // Actual time may be longer because of asteroid intersection avoidance.
							           
const float MaxPlayerShipSpeed             = 2000.0f; // px/sec
const float PlayerThrustForce              = 650.0f;
const float PlayerRotationSpeed            = 300.0f;  // degrees per second
								           
const float MaxTorpedoSpeed                = 600.0f;  // px per second
const float MaxTorpedoLifeTime             =   1.5f;  // seconds
const int32 TorpedoCount                   =  30;     // make room for player and enemy torpedos which are in the same array
								           
const int32 MaxInitialAsteroids            =  24;
const float MinAsteroidSpeed               =  50.0f;  // px per second
const float MaxAsteroidSpeed               = 100.0f;  // px per second
const float MinAsteroidSpinSpeed           = -20.0f;  // degrees per second
const float MaxAsteroidSpinSpeed           =  20.0f;  // degrees per second
const float AsteroidSpinScale              =   2.0f;  // Child asteroids spin this much faster than their parent
const float MinAsteroidSplitAngle          =  5.0f;   // Children of rock death move away deviating from parent inertia at least this much.
const float MaxAsteroidSplitAngle          = 35.0f;   // Ditto, but this is the max angular deviation, in degrees.
const float MinAsteroidSplitInertia        =  0.1f;   // Child rock could have as little as this much of parent's inertia.
const float MaxAsteroidSplitInertia        =  3.0f;   // Ditto, max as much as this.
								           
const int32 ValueBigAsteroid               =    20;
const int32 ValueMediumAsteroid            =    50;
const int32 ValueSmallAsteroid             =   100;
const int32 ValueBigEnemy                  =   200;
const int32 ValueSmallEnemy                =  1000;
							           
const float MaxIntroStateLifetime          =  5.0f;  // How long the initial intro screen is visible before the main menu appears.
const float TimeBetweenWaves               =  3.0f;  // Number of seconds between each wave.
const float MaxTimeUntilGameOverStateEnds  =  5.0f;  // Time to wait between game over screen and idle screen.

const float MaxTimeUntilNextEnemyShip      = 20.0f;  // Let each wave start with a breather.
const float MaxTimeUntilEnemyRespawn       = 10.0f;  // Longest delay between successive enemy ship spawns. Favored when player score is low.
const float MinTimeUntilEnemyRespawn       =  2.0f;  // Shortest delay between successive enemy ship spawns. Favored more as player score increases.

const float BigEnemyTorpedoSpeed           = MaxTorpedoSpeed;
const float BigEnemyReloadTime             =   1.25f;
const float BigEnemySpeed                  = 200.0f;

const float SmallEnemyTorpedoSpeed         = BigEnemyTorpedoSpeed * 1.1f;
const float SmallEnemyReloadTime           = 1.0f;
const float SmallEnemySpeed                = 300.0f;

const float ShieldPowerupIncrease          = 20.0f; // Number of seconds of shield life given when shield powerup gained.
const int32 DoubleGunsPowerupIncrease      = 100;   // Number of double shots given when double guns powerup gained.

const float PowerupOpacity                 = 0.5f;
const bool  PowerupsCanMove                = false;


static FVector2D WrapPositionToViewport(const FVector2D& P)
{
	return FVector2D(UKismetMathLibrary::FWrap(P.X, 0.0, ViewportSize.X), UKismetMathLibrary::FWrap(P.Y, 0.0, ViewportSize.Y));
}


static FVector2D MakeInertia(const FVector2D& InertiaOld, float MinDeviation, float MaxDeviation)
{
	FVector2D NewInertia = UDaylonUtils::Rotate(InertiaOld, FMath::RandRange(MinDeviation, MaxDeviation));

	return NewInertia;
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


void UPlayViewBase::InitializePlayerShip()
{
	PlayerShip.IsUnderThrust   = false;
	PlayerShip.IsSpawning      = false;
	PlayerShip.DoubleShotsLeft = 0;
	PlayerShip.ShieldsLeft     = 0.0f;

	PlayerShip.Create (PlayerShipBrush, 0.4f);
	PlayerShip.Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	PlayerShip.Hide   ();
}


void UPlayViewBase::InitializePlayerShield()
{
	PlayerShield.Create (ShieldBrush, 0.5f);
	PlayerShield.Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	PlayerShield.Hide   ();
}


void UPlayViewBase::CreateTorpedos()
{
	for(int32 Index = 0; Index < TorpedoCount; Index++)
	{
		FTorpedo Torpedo;

		Torpedo.Inertia = FVector2D(0);
		Torpedo.LifeRemaining = 0.0f;

		Torpedo.Create(TorpedoBrush, 0.5f);
		Torpedo.Hide();

		Torpedos.Add(Torpedo);
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

	GameState                     = EGameState::Startup;
	SelectedMenuItem              = EMenuItem::StartPlaying;
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

	InitializeVariables    ();
	PreloadSounds          ();
	CreateTorpedos         ();
	InitializePlayerShip   ();
	InitializePlayerShield ();

	PlayerShipThrustSoundLoop.Set (this, ThrustSound);
	BigEnemyShipSoundLoop.Set     (this, EnemyShipBigSound);
	SmallEnemyShipSoundLoop.Set   (this, EnemyShipSmallSound);

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
		PlayerShip.SetAngle(UDaylonUtils::Vector2DToAngle(Direction));
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
			PlayerShip.Hide();

			UDaylonUtils::Hide (MenuContent);
			UDaylonUtils::Hide (PlayerScoreReadout);
			UDaylonUtils::Hide (PlayerShipsReadout);
			UDaylonUtils::Hide (PlayerShieldReadout);
			UDaylonUtils::Hide (ShieldLabel);
			UDaylonUtils::Hide (DoubleGunReadout);
			UDaylonUtils::Hide (DoubleGunLabel);
			UDaylonUtils::Hide (GameOverMessage);
			UDaylonUtils::Show (IntroContent);

			break;


		case EGameState::MainMenu:

			UDaylonUtils::Hide (IntroContent);
			UDaylonUtils::Hide (HelpContent);
			UDaylonUtils::Hide (CreditsContent);
			UDaylonUtils::Hide (HighScoresContent);
			UDaylonUtils::Hide (PlayerScoreReadout);
			UDaylonUtils::Hide (PlayerShipsReadout);
			UDaylonUtils::Hide (PlayerShieldReadout);
			UDaylonUtils::Hide (ShieldLabel);
			UDaylonUtils::Hide (DoubleGunReadout);
			UDaylonUtils::Hide (DoubleGunLabel);
			UDaylonUtils::Hide (GameOverMessage);
			UDaylonUtils::Hide (HighScoreEntryContent);
			
			RemoveExplosions();
			RemoveTorpedos();

			PlayerShip.Hide(); // to be safe
			PlayerShield.Hide();

			RemoveEnemyShips();

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

			InitializeScore           ();
			InitializePlayerShipCount ();
			RemoveAsteroids           ();
			RemoveEnemyShips          ();

			WaveNumber = 0;

			UDaylonUtils::Show  (PlayerScoreReadout);
			UDaylonUtils::Show  (PlayerShipsReadout);
			UDaylonUtils::Show  (PlayerShieldReadout);
			UDaylonUtils::Show  (ShieldLabel);
			UDaylonUtils::Show  (DoubleGunReadout);
			UDaylonUtils::Show  (DoubleGunLabel);

			UpdatePowerupReadout(EPowerup::DoubleGuns);
			UpdatePowerupReadout(EPowerup::Shields);

			PlayerShip.Spawn(ViewportSize / 2, FVector2D(0), 1.0f);

			break;


		case EGameState::Over:

			if(PreviousState != EGameState::Active)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering game over state"), (int32)PreviousState);
			}

			PlayerShield.Hide();
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

			RemoveAsteroids();
			RemoveEnemyShips();

			UDaylonUtils::Show(PlayerScoreReadout);
			UDaylonUtils::Hide(GameOverMessage);
			PlayerShield.Hide();
			
			HighScoreEntryContent->SetVisibility(ESlateVisibility::Visible);
			HighScoreNameEntry->SetFocus();

			break;


		case EGameState::Credits:
			
			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering credits state"), (int32)PreviousState);
			}

			RemoveEnemyShips();
			UDaylonUtils::Show(CreditsContent);

			break;


		case EGameState::Help:
			
			if(PreviousState != EGameState::MainMenu)
			{
				UE_LOG(LogGame, Warning, TEXT("Invalid previous state %d when entering help state"), (int32)PreviousState);
			}

			RemoveAsteroids();
			RemoveEnemyShips();
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


	switch(GameState)
	{
		case EGameState::Intro:

			if(TimeUntilIntroStateEnds <= 0.0f)
			{
				TransitionToState(EGameState::MainMenu);
			}

			TimeUntilIntroStateEnds -= InDeltaTime;

			break;


		case EGameState::MainMenu:

			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);

			break;


		case EGameState::Active:

			if(PlayerShip.IsVisible())
			{
				UpdatePlayerRotation  (InDeltaTime);
				UpdatePlayerShip      (InDeltaTime);
			}

			UpdateEnemyShips          (InDeltaTime);
			UpdateAsteroids           (InDeltaTime);
			UpdatePowerups            (InDeltaTime);
			UpdateTorpedos            (InDeltaTime);
			UpdateExplosions          (InDeltaTime);

			CheckCollisions();

			ProcessWaveTransition     (InDeltaTime);

			if(PlayerShip.IsSpawning)
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
			Image->SetBrush(PlayerShipBrush);
			Image->Brush.SetImageSize(FVector2D(24));
			
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

	if(!PlayerShip.IsSpawning)
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

	PlayerShip.Spawn(ViewportSize / 2, FVector2D(0), 1.0f);

	PlayerShip.IsSpawning = false;
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


bool UPlayViewBase::IsSafeToSpawnPlayer() const
{
	if(!PlayerShip.IsSpawning)
	{
		UE_LOG(LogGame, Error, TEXT("Checking if safe to spawn player when player is not waiting to spawn"));
		return false;
	}

	// If any asteroid or enemy ship intersects a box in the center of the screen, return false.

	// Make the safezone smaller if the player has enough shields. 
	// This makes it easier to respawn when there are lots of things around
	// without having to potentially wait a really long time.

	const auto SafeZoneDivisor = (PlayerShip.ShieldsLeft > 3.0f) ? 8 : 4;

	const auto ScreenCenter = ViewportSize / 2;
	const auto SafeZoneSize = ViewportSize / SafeZoneDivisor;

	UE::Geometry::FAxisAlignedBox2d SafeZone(
		ScreenCenter - SafeZoneSize / 2,
		ScreenCenter + SafeZoneSize / 2);

	return (!ObjectsIntersectBox(Asteroids, SafeZone) && !ObjectsIntersectBox(EnemyShips, SafeZone));
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

	auto& Asteroid = Asteroids[Index];

	if(Asteroid.HasPowerup())
	{
		Asteroid.Powerup.DestroyWidget();
	}

	Asteroid.DestroyWidget();

	Asteroids.RemoveAt(Index);
}


void UPlayViewBase::RemoveAsteroids()
{
	for(int32 Index = Asteroids.Num() - 1; Index >= 0; Index--)
	{
		RemoveAsteroid(Index);		
	}
}


void UPlayViewBase::SpawnPowerup(FPowerup& Powerup, const FVector2D& P)
{
	Powerup.Kind = FMath::RandBool() ? EPowerup::DoubleGuns : EPowerup::Shields;

	UDaylonSpriteWidgetAtlas* Atlas = nullptr;

	switch(Powerup.Kind)
	{
		case EPowerup::DoubleGuns: Atlas = DoubleGunsPowerupAtlas; break;
		case EPowerup::Shields:    Atlas = ShieldPowerupAtlas;     break;
	}

	check(Atlas);

	// Make the powerups dark so they don't get confused with asteroids.
	Powerup.Create(Atlas, 0.5f, FLinearColor(1.0f, 1.0f, 1.0f, PowerupOpacity), FVector2D(32));

	Powerup.Show();
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

		FAsteroid Asteroid;

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
		Asteroid.LifeRemaining = 1.0f;
		Asteroid.SpinSpeed     = FMath::RandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);
		//Asteroid.Widget        = UDaylonUtils::MakeWidget<UImage>();

		//Asteroid.Widget->SetRenderTransformPivot(FVector2D(0.5f));

		FSlateBrush AsteroidBrush;

		switch(AsteroidSize)
		{
			case 0:
				AsteroidBrush = BigRockBrushes[FMath::RandRange(0, 3)]; 
				Asteroid.Value = ValueBigAsteroid;

				if(Index % 4 == 0)
				{
					SpawnPowerup(Asteroid.Powerup, P);
				}
				break;

			case 1: 
				AsteroidBrush = MediumRockBrushes[FMath::RandRange(0, 3)]; 
				Asteroid.Value = ValueMediumAsteroid;
				break;

			case 2: 
				AsteroidBrush = SmallRockBrushes[FMath::RandRange(0, 3)]; 
				Asteroid.Value = ValueSmallAsteroid;
				break;
		}

		Asteroid.Create(AsteroidBrush, 0.5f);
		Asteroid.Spawn(P, Inertia, 1.0f);

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
}


void UPlayViewBase::UpdatePlayerRotation(float DeltaTime)
{
	// Uses a 1D axis value instead of action buttons

	if(!PlayerShip.IsValid())
	{
		UE_LOG(LogGame, Error, TEXT("Invalid player ship"));
		return;
	}

	const float Amt = PlayerRotationSpeed * DeltaTime;

	UE_LOG(LogGame, Log, TEXT("Rotation force: %.5f"), RotationForce);

	PlayerShip.SetAngle(UDaylonUtils::WrapAngle(PlayerShip.GetAngle() + Amt * RotationForce));
}


void UPlayViewBase::UpdatePlayerShip(float DeltaTime)
{
	if(!PlayerShip.IsValid())
	{
		UE_LOG(LogGame, Error, TEXT("Invalid player ship"));
		return;
	}

	// Change widget brush only if the thrust state actually changed.
	const bool bThrustStateChanged = (PlayerShip.IsUnderThrust != bThrustActive);

	PlayerShip.IsUnderThrust = bThrustActive;

	if (bThrustActive)
	{
		if(bThrustStateChanged)
		{
			PlayerShip.Widget->SetBrush(PlayerShipThrustingBrush);
			PlayerShipThrustSoundLoop.Start();
		}
		else
		{
			// Still thrusting.
			PlayerShipThrustSoundLoop.Tick(DeltaTime);
		}

		const float Thrust = PlayerThrustForce * DeltaTime;

		const FVector2D Force = UDaylonUtils::GetWidgetDirectionVector(PlayerShip.Widget) * Thrust;

		PlayerShip.Inertia += Force;

		// Limit speed to avoid breaking collision detector.

		if(PlayerShip.Inertia.Length() > MaxPlayerShipSpeed)
		{
			PlayerShip.Inertia.Normalize();
			PlayerShip.Inertia *= MaxPlayerShipSpeed;
		}
	}
	else
	{
		if(bThrustStateChanged)
		{
			PlayerShip.Widget->SetBrush(PlayerShipBrush);
		}
	}

	PlayerShip.Move(DeltaTime, WrapPositionToViewport);


	PlayerShield.Show(bShieldActive && PlayerShip.ShieldsLeft > 0.0f);

	if(PlayerShield.IsVisible())
	{
		// We have to budge the shield texture by two px to look nicely centered around the player ship.
		PlayerShield.SetPosition(PlayerShip.GetPosition() + UDaylonUtils::Rotate(FVector2D(0, 2), PlayerShip.GetAngle()));
		AdjustShieldsLeft(-DeltaTime);
	}
}


void UPlayViewBase::UpdateAsteroids(float DeltaTime)
{
	for (auto& Asteroid : Asteroids)
	{
		if (Asteroid.LifeRemaining > 0.0f)
		{
			Asteroid.Move(DeltaTime, WrapPositionToViewport);
			if(Asteroid.HasPowerup())
			{
				Asteroid.Powerup.SetPosition(Asteroid.GetPosition());
				Asteroid.Powerup.Tick(DeltaTime);
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
		Powerup.Move(DeltaTime, WrapPositionToViewport);
		Powerup.Tick(DeltaTime);
	}
}


void UPlayViewBase::SpawnPlayerShipExplosion(const FVector2D& P)
{
	auto Explosion = UDaylonUtils::MakeWidget<UDaylonParticlesWidget>();

	Explosion->SetVisibility(ESlateVisibility::HitTestInvisible);
	Explosion->SetRenderTransformPivot(FVector2D(0.5f));

	Explosion->ParticleBrush = TorpedoBrush;

	Explosion->MinParticleSize     *= 1;
	Explosion->MaxParticleSize     *= 2;
	Explosion->MinParticleVelocity *= 1;
	Explosion->MaxParticleVelocity *= 2;
	Explosion->MinParticleLifetime *= 2;
	Explosion->MaxParticleLifetime *= 3;
	Explosion->FinalOpacity        = 0.25f;
	Explosion->NumParticles        *= 2;

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

		auto Explosion = UDaylonUtils::MakeWidget<UDaylonParticlesWidget>();

		Explosion->SetVisibility(ESlateVisibility::HitTestInvisible);
		Explosion->SetRenderTransformPivot(FVector2D(0.5f));

		Explosion->ParticleBrush = This->TorpedoBrush;

		Explosion->MinParticleSize     *= 1.5f;
		Explosion->MaxParticleSize     *= 3;
		Explosion->MinParticleVelocity *= 1.5f;
		Explosion->MaxParticleVelocity *= 3;
		Explosion->MinParticleLifetime *= 2;
		Explosion->MaxParticleLifetime *= 4;
		Explosion->FinalOpacity        = 0.25f;
		Explosion->NumParticles        *= 2;

		auto CanvasSlot = This->RootCanvas->AddChildToCanvas(Explosion);

		Explosion->SynchronizeProperties();

		CanvasSlot->SetAnchors(FAnchors());
		CanvasSlot->SetAlignment(FVector2D(0.5f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(P);

		This->Explosions.Add(Explosion);
	};

	ScheduledTasks.Add(Task);

	auto CanvasSlot = RootCanvas->AddChildToCanvas(Explosion);

	Explosion->SynchronizeProperties();

	CanvasSlot->SetAnchors(FAnchors());
	CanvasSlot->SetAlignment(FVector2D(0.5f));
	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetPosition(P);

	Explosions.Add(Explosion);
}


void UPlayViewBase::SpawnExplosion(const FVector2D& P)
{
	auto Explosion = UDaylonUtils::MakeWidget<UDaylonParticlesWidget>();

	Explosion->SetVisibility(ESlateVisibility::HitTestInvisible);
	Explosion->SetRenderTransformPivot(FVector2D(0.5f));
	Explosion->ParticleBrush = TorpedoBrush;

	auto CanvasSlot = RootCanvas->AddChildToCanvas(Explosion);

	Explosion->SynchronizeProperties();

	CanvasSlot->SetAnchors(FAnchors());
	CanvasSlot->SetAlignment(FVector2D(0.5f));
	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetPosition(P);

	Explosions.Add(Explosion);
}


void UPlayViewBase::UpdateExplosions(float DeltaTime)
{
	if(Explosions.IsEmpty())
	{
		return;
	}

	TArray<UDaylonParticlesWidget*> NewList;

	for(auto Explosion : Explosions)
	{
		if(Explosion->Update(DeltaTime))
		{
			NewList.Add(Explosion);
		}
		else
		{
			RootCanvas->RemoveChild(Explosion);
		}
	}

	Explosions = NewList;
}


void UPlayViewBase::PlaySound(USoundBase* Sound, float VolumeScale)
{
	UGameplayStatics::PlaySound2D(this, Sound, VolumeScale);
}


void UPlayViewBase::KillPlayerShip()
{
	SpawnPlayerShipExplosion(PlayerShip.UnwrappedNewPosition);

	PlaySound(PlayerShipDestroyedSound);

	PlayerShip.Hide();

	AddPlayerShips(-1);

	PlayerShip.IsSpawning = true;
	TimeUntilNextPlayerShip = MaxTimeUntilNextPlayerShip;

	// ProcessPlayerShipSpawn() will handle the wait til next spawn and transition to game over, if needed. 
}


void UPlayViewBase::KillAsteroid(int32 AsteroidIndex, bool KilledByPlayer)
{
	// Kill the rock. Split it if isn't a small rock.

	if(!Asteroids.IsValidIndex(AsteroidIndex))
	{
		return;
	}

	auto& Asteroid = Asteroids[AsteroidIndex];

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
			Asteroid.Powerup.Kind = EPowerup::Nothing;
			if(PowerupsCanMove)
			{
				Powerups[PowerupIndex].Inertia = Asteroid.Inertia;
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

	FAsteroid NewAsteroid;

	const bool BothKidsFast = FMath::RandRange(0, 10) < 9;

	NewAsteroid.Inertia = MakeInertia(Asteroid.Inertia, MinAsteroidSplitAngle, MaxAsteroidSplitAngle);
	NewAsteroid.Inertia *= FMath::RandRange(1.2f, 3.0f);

	NewAsteroid.LifeRemaining = 1.0f;
	NewAsteroid.SpinSpeed     = Asteroid.SpinSpeed * AsteroidSpinScale;// FMath::RandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);


	//NewAsteroid.Widget = UDaylonUtils::MakeWidget<UImage>();
	//NewAsteroid.Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	//NewAsteroid.Widget->SetRenderTransformPivot(FVector2D(0.5f));

	Asteroid.LifeRemaining = 1.0f;
	Asteroid.Inertia = MakeInertia(Asteroid.Inertia, -MinAsteroidSplitAngle, -MaxAsteroidSplitAngle);

	if(BothKidsFast)
	{
		Asteroid.Inertia *= FMath::RandRange(1.2f, 3.0f);
	}
	else
	{
		Asteroid.Inertia *= FMath::RandRange(0.25f, 1.0f);
	}

	FSlateBrush NewAsteroidBrush;

	switch(Asteroid.Value)
	{
		case ValueBigAsteroid:
			NewAsteroid.Value = ValueMediumAsteroid;
			NewAsteroidBrush = MediumRockBrushes[FMath::RandRange(0, 3)]; 
			Asteroid.Value = ValueMediumAsteroid;
			Asteroid.SetBrush(MediumRockBrushes[FMath::RandRange(0, 3)]); 
			break;

		case ValueMediumAsteroid:
			NewAsteroid.Value = ValueSmallAsteroid;
			NewAsteroidBrush = SmallRockBrushes[FMath::RandRange(0, 3)]; 
			Asteroid.Value = ValueSmallAsteroid;
			Asteroid.SetBrush(SmallRockBrushes[FMath::RandRange(0, 3)]); 
			break;
	}

	// Update size of existing rock.
	Asteroid.UpdateWidgetSize();

	Asteroid.SpinSpeed *= AsteroidSpinScale;

	NewAsteroid.Create(NewAsteroidBrush, 0.5f);
	NewAsteroid.Show();

	NewAsteroid.OldPosition = 
	NewAsteroid.UnwrappedNewPosition = Asteroid.UnwrappedNewPosition;
	NewAsteroid.SetPosition(NewAsteroid.UnwrappedNewPosition);

	// Do this last since the play object is copied.
	Asteroids.Add(NewAsteroid);
}


void UPlayViewBase::KillEnemyShip(int32 EnemyIndex)
{
	if(!EnemyShips.IsValidIndex(EnemyIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid enemy ship index %d"), EnemyIndex);
		return;
	}

	auto& EnemyShip = EnemyShips[EnemyIndex];

	SpawnExplosion(EnemyShip.GetPosition());
	PlaySound(ExplosionSounds[EnemyShip.Value == ValueBigEnemy ? 0 : 1]);
	RemoveEnemyShip(EnemyIndex);
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
			Str = FString::Printf(TEXT("%d"), PlayerShip.DoubleShotsLeft);
			DoubleGunReadout->SetText(FText::FromString(Str));
			break;

		case EPowerup::Shields:
			Str = FString::Printf(TEXT("%d"), FMath::RoundToInt(PlayerShip.ShieldsLeft));
			PlayerShieldReadout->SetText(FText::FromString(Str));
			break;
	}
}


void UPlayViewBase::AdjustDoubleShotsLeft(int32 Amount)
{
	PlayerShip.DoubleShotsLeft += Amount;
	UpdatePowerupReadout(EPowerup::DoubleGuns);
}


void UPlayViewBase::AdjustShieldsLeft(float Amount)
{
	PlayerShip.ShieldsLeft = FMath::Max(0.0f, PlayerShip.ShieldsLeft + Amount);
	UpdatePowerupReadout(EPowerup::Shields);
}


void UPlayViewBase::ProcessPlayerCollision()
{
	if(PlayerShield.IsVisible())
	{
		PlaySound(ShieldBonkSound);
	}
	else 
	{
		KillPlayerShip();
	}
}


void UPlayViewBase::CheckCollisions()
{
	// Build a triangle representing the player ship.

	FVector2D PlayerShipTriangle[3]; // tip, LR corner, LL corner.

	float PlayerShipH = PlayerShip.GetSize().Y;
	float PlayerShipW = PlayerShipH * (73.0f / 95.0f);

	PlayerShipTriangle[0].Set(0.0f,            -PlayerShipH / 2);
	PlayerShipTriangle[1].Set( PlayerShipW / 2, PlayerShipH / 2);
	PlayerShipTriangle[2].Set(-PlayerShipW / 2, PlayerShipH / 2);

	// Triangle is pointing up, vertices relative to its center. We need to rotate it for its current angle.
			 
	auto ShipAngle = PlayerShip.GetAngle();

	// Rotate and translate the triangle to match its current display space.

	for(auto& Triangle : PlayerShipTriangle)
	{
		Triangle = UDaylonUtils::Rotate(Triangle, ShipAngle);

		// Use the ship's old position because the current position can cause unwanted self-intersections.
		Triangle += PlayerShip.OldPosition;
	}

	// Get the line segment for the player ship.
	// Line segments are used to better detect collisions involving fast-moving objects.

	const FVector2D PlayerShipLineStart = PlayerShip.OldPosition;
	const FVector2D PlayerShipLineEnd   = PlayerShip.UnwrappedNewPosition;


	// See what the active torpedos have collided with.

	for (auto& Torpedo : Torpedos)
	{
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

		for(auto& Asteroid : Asteroids)
		{
			AsteroidIndex++;

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
		
		if(Torpedo.IsAlive() && !Torpedo.FiredByPlayer && PlayerShip.IsVisible())
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
				auto& EnemyShip = EnemyShips[EnemyIndex];

				if(UDaylonUtils::DoesLineSegmentIntersectCircle(OldP, CurrentP, EnemyShip.GetPosition(), EnemyShip.GetRadius()))
				{
					Torpedo.Kill();
					IncreasePlayerScoreBy(EnemyShip.Value);
					SpawnExplosion(EnemyShip.GetPosition());
					PlaySound(ExplosionSounds[EnemyShip.Value == ValueBigEnemy ? 0 : 1]);
					RemoveEnemyShip(EnemyIndex);
				}
			}
		}
	} // next torpedo


	// Check if player ship collided with a rock

	if(PlayerShip.IsVisible())
	{
		int32 AsteroidIndex = -1;

		for(auto& Asteroid : Asteroids)
		{
			AsteroidIndex++;

			if(UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Asteroid.GetPosition(), Asteroid.GetRadius() + PlayerShip.GetRadius())
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

	if(PlayerShip.IsVisible())
	{
		for(int32 EnemyIndex = EnemyShips.Num() - 1; EnemyIndex >= 0; EnemyIndex--)
		{
			auto& EnemyShip = EnemyShips[EnemyIndex];

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, EnemyShip.OldPosition, EnemyShip.GetRadius())
				|| FVector2D::Distance(PlayerShip.UnwrappedNewPosition, EnemyShip.UnwrappedNewPosition) < EnemyShip.GetRadius() + PlayerShip.GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(EnemyShip.Value);
				KillEnemyShip(EnemyIndex);

				ProcessPlayerCollision();

				break;
			}
		}
	}


	// Check if player ship collided with a powerup

	if(PlayerShip.IsVisible())
	{
		for(int32 PowerupIndex = Powerups.Num() - 1; PowerupIndex >= 0; PowerupIndex--)
		{
			auto& Powerup = Powerups[PowerupIndex];

			if (UDaylonUtils::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Powerup.OldPosition, Powerup.GetRadius())
				|| FVector2D::Distance(PlayerShip.UnwrappedNewPosition, Powerup.UnwrappedNewPosition) < Powerup.GetRadius() + PlayerShip.GetRadius())
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
				}
				
				KillPowerup(PowerupIndex);

				break;
			}
		}
	}


	// Check if enemy ship collided with an asteroid

	for(int32 EnemyIndex = EnemyShips.Num() - 1; EnemyIndex >= 0; EnemyIndex--)
	{
		auto& EnemyShip = EnemyShips[EnemyIndex];
		int32 AsteroidIndex = -1; 

		for(auto& Asteroid : Asteroids)
		{
			AsteroidIndex++;

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

	for (auto& Torpedo : Torpedos)
	{
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

	for(const auto& Torpedo : Torpedos)
	{
		Result++;

		if(Torpedo.IsDead())
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

	auto& Powerup = Powerups[PowerupIndex];

	Powerup.DestroyWidget();

	Powerups.RemoveAt(PowerupIndex);
}


void UPlayViewBase::RemoveEnemyShip(int32 EnemyIndex)
{
	if(!EnemyShips.IsValidIndex(EnemyIndex))
	{
		UE_LOG(LogGame, Error, TEXT("Invalid enemy ship index %d"), EnemyIndex);
		return;
	}

	auto& EnemyShip = EnemyShips[EnemyIndex];


	if(EnemyShip.Value == ValueBigEnemy)
	{
		NumBigEnemyShips--;
	}
	else
	{
		NumSmallEnemyShips--;
	}

	check(NumBigEnemyShips >= 0 && NumSmallEnemyShips >= 0);


	EnemyShip.DestroyWidget();

	EnemyShips.RemoveAt(EnemyIndex);
}


void UPlayViewBase::RemoveEnemyShips()
{
	while(!EnemyShips.IsEmpty())
	{
		RemoveEnemyShip(0);
	}
}


void UPlayViewBase::RemoveExplosions()
{
	while(!Explosions.IsEmpty())
	{
		RootCanvas->RemoveChild(Explosions.Last(0));
		Explosions.Pop();
	}
}


void UPlayViewBase::RemovePowerups()
{
	while(!Powerups.IsEmpty())
	{
		Powerups.Last(0).DestroyWidget();
		Powerups.Pop();
	}
}


void UPlayViewBase::RemoveTorpedos()
{
	for(auto Torpedo : Torpedos)
	{
		Torpedo.LifeRemaining = 0.0f;
		Torpedo.Hide();
	}
}


void UPlayViewBase::SpawnEnemyShip()
{
	FEnemyShip EnemyShip;

	// Generate a big enemy ship vs. small one based on player score.
	// The higher the score, the likelier a small enemy will appear.
	// Regardless of score, there's always a 10% chance of a big enemy ship.

	const int32 ScoreTmp = FMath::Max(0, PlayerScore - 5000);
	float BigEnemyProbability = pow(FMath::Lerp(1.0f, 0.1f,  FMath::Min(1.0f, ScoreTmp / 65'000.0f)), 2.0f);
	BigEnemyProbability = FMath::Max(0.1f, BigEnemyProbability);

	const bool IsBigEnemy = (FMath::FRand() <= BigEnemyProbability);
	EnemyShip.Value = IsBigEnemy ? ValueBigEnemy : ValueSmallEnemy; // todo: favor small enemies as player score increases.

	EnemyShip.TimeRemainingToNextShot = (IsBigEnemy ? BigEnemyReloadTime : SmallEnemyReloadTime);
	EnemyShip.TimeRemainingToNextMove = 3.0f;
	auto Inertia = FVector2D(1, 0) * 300; // todo: use global constant for speed, maybe min/max it

	// Choose a random Y-pos to appear at. Leave room to avoid ship appearing clipped.
	FVector2D P(0.0, FMath::RandRange(EnemyShip.GetSize().Y + 2, ViewportSize.Y - (EnemyShip.GetSize().Y + 2)));

	if(FMath::RandBool())
	{
		Inertia.X *= -1; // make enemy ship travel from right to left.
		P.X = ViewportSize.X - 1.0f; // avoid immediate removal
	}
			
	EnemyShip.Create(IsBigEnemy ? BigEnemyBrush : SmallEnemyBrush, 0.375f);
	EnemyShip.Spawn(WrapPositionToViewport(P), Inertia, 0.0f);

	EnemyShips.Add(EnemyShip);

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
		auto& EnemyShip = EnemyShips[ShipIndex];

		const bool IsBigEnemy = (EnemyShip.Value == ValueBigEnemy);

		EnemyShip.Move(DeltaTime, WrapPositionToViewport);

		// If we've reached the opposite side of the viewport, remove us.
		const auto P2 = WrapPositionToViewport(EnemyShip.UnwrappedNewPosition);

		if(P2.X != EnemyShip.UnwrappedNewPosition.X)
		{
			// Ship tried to wrap around horizontally.
			RemoveEnemyShip(ShipIndex);
			continue;
		}

		// Fire a torpedo if we've finished reloading.

		EnemyShip.TimeRemainingToNextShot -= DeltaTime;

		if(EnemyShip.TimeRemainingToNextShot <= 0.0f)
		{
			EnemyShip.TimeRemainingToNextShot = (IsBigEnemy ? BigEnemyReloadTime : SmallEnemyReloadTime);
			LaunchTorpedoFromEnemy(EnemyShip, IsBigEnemy);
		}

		EnemyShip.TimeRemainingToNextMove -= DeltaTime;

		if(EnemyShip.TimeRemainingToNextMove <= 0.0f)
		{
			EnemyShip.TimeRemainingToNextMove = 3.0f; // todo: use global constant instead of 3.0

			// Change heading (or stay on current heading).

			const FVector2D Headings[] = 
			{
				{ 1, 0 },
				{ 1, 1 },
				{ 1, -1 }
			};

			FVector2D NewHeading = Headings[FMath::RandRange(0, 2)];

			NewHeading.Normalize();

			const auto Facing = FMath::Sign(EnemyShip.Inertia.X);
			EnemyShip.Inertia = NewHeading * EnemyShip.Inertia.Length();
			EnemyShip.Inertia.X *= Facing;
		}
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
}


void UPlayViewBase::LaunchTorpedoFromEnemy(const FEnemyShip& Shooter, bool IsBigEnemy)
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!PlayerShip.IsVisible())
	{
		return;
	}

	const int32 TorpedoIdx = GetAvailableTorpedo();

	if(TorpedoIdx == INDEX_NONE)
	{
		return;
	}

	auto& Torpedo = Torpedos[TorpedoIdx];

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
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, PlayerShip.GetPosition(), PlayerShip.Inertia);
		}
		else
		{
			// Shoot at an asteroid.
			const auto & Asteroid = Asteroids[FMath::RandRange(0, Asteroids.Num() - 1)];
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Asteroid.GetPosition(), Asteroid.Inertia);
		}
	}

	Torpedo.Spawn(LaunchP, Direction * Speed, MaxTorpedoLifeTime);

	PlaySound(TorpedoSound);
}


void UPlayViewBase::OnFireTorpedo()
{
	// Called when the 'fire torpedo' button is pressed.

	if(!PlayerShip.IsVisible())
	{
		return;
	}

	const FVector2D PlayerFwd = UDaylonUtils::GetWidgetDirectionVector(PlayerShip.Widget);

	const auto Inertia = (PlayerFwd * MaxTorpedoSpeed) + PlayerShip.Inertia;

	// Position torpedo at nose of player ship.


	int32 TorpedoIndex = GetAvailableTorpedo();

	if(TorpedoIndex == INDEX_NONE)
	{
		return;
	}

	PlaySound(TorpedoSound);

    if(PlayerShip.DoubleShotsLeft == 0)
	{
		// Find an available torpedo, spawn it at the nose of the player ship,
		// and give it an inertia which is player ship intertia + player ship fwd * MaxTorpedoSpeed

		auto& Torpedo = Torpedos[TorpedoIndex];

		Torpedo.FiredByPlayer = true;

		auto P = PlayerShip.GetPosition();
		P += PlayerFwd * (PlayerShip.GetSize().Y / 2 + /*PlayerShip.Inertia.Length()*/ 2.0); // The last offset is so that the bullet doesn't start off accidentally overlapping the player ship
		P = WrapPositionToViewport(P);

		Torpedo.Spawn(P, Inertia, MaxTorpedoLifeTime);
	}
	else
	{
		AdjustDoubleShotsLeft(-1);

		auto& Torpedo1 = Torpedos[TorpedoIndex];
		Torpedo1.FiredByPlayer = true;

		auto P = PlayerFwd * (PlayerShip.GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, 90.0f);
		P += PlayerShip.GetPosition();
		//P += PlayerFwd * FMath::RandRange(0.0f, 10.0f);
		P = WrapPositionToViewport(P);

		Torpedo1.Spawn(P, Inertia, MaxTorpedoLifeTime);

		TorpedoIndex = GetAvailableTorpedo();

		if(TorpedoIndex == INDEX_NONE)
		{
			return;
		}

		auto& Torpedo2 = Torpedos[TorpedoIndex];
		Torpedo2.FiredByPlayer = true;

		P = PlayerFwd * (PlayerShip.GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, -90.0f);
		P += PlayerShip.GetPosition();
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

	for(const auto& Entry : HighScores.Entries)
	{
		auto EntryColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));

		if(!MruScoreHighlighted && Entry == MostRecentHighScore)
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
