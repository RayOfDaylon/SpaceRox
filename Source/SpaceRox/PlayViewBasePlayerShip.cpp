// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Logging.h"



// Set to 1 to enable debugging
#define DEBUG_MODULE                0


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



bool UPlayViewBase::IsPlayerShipPresent() const
{
	return (PlayerShip && PlayerShip->IsVisible());
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
	PlayerShip = FPlayerShip::Create(PlayerShipAtlas->Atlas, FVector2D(32), 0.4f);

	PlayerShip->DoubleShotsLeft   .Bind([this](int32){ UpdatePlayerShipReadout(EPowerup::DoubleGuns);    });
	PlayerShip->ShieldsLeft       .Bind([this](int32){ UpdatePlayerShipReadout(EPowerup::Shields);       });
	PlayerShip->InvincibilityLeft .Bind([this](float){ UpdatePlayerShipReadout(EPowerup::Invincibility); });

	PlayerShip->Initialize(this);
}


void UPlayViewBase::OnAimPlayerShip(const FVector2D& Direction)
{
	// Aiming with a gamepad joystick doesn't really work that well; 
	// one keeps expecting it to also include thrust. We could do 
	// that, so we may revisit this as a todo item.

	if(GameState == EGameState::Active)
	{
		PlayerShip->SetAngle(Daylon::Vector2DToAngle(Direction));
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
			UImage* Image = Daylon::MakeWidget<UImage>();
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

	if(IsWaitingToSpawnPlayerShip())
	{
		TimeUntilNextPlayerShip -= DeltaTime;
		return;
	}

	if(NumPlayerShips == 0)
	{
		// Mandatory wait period ended but we have no ships, so end the game.
		TransitionToState(EGameState::Over);
		return;
	}

	// The mandatory wait period has ended but don't spawn until there's enough clear space around the screen center.
	if(!IsSafeToSpawnPlayerShip())
	{
		return;
	}

	PlayerShip->Start(ViewportSize / 2, FVector2D(0), 1.0f);

	PlayerShip->IsSpawning = false;
}


bool UPlayViewBase::IsSafeToSpawnPlayerShip() const
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

	return (!Daylon::PlayObjectsIntersectBox(Asteroids.Asteroids,   SafeZone) && 
	        !Daylon::PlayObjectsIntersectBox(EnemyShips.Ships,      SafeZone) &&
			!Daylon::PlayObjectsIntersectBox(EnemyShips.Scavengers, SafeZone));
}


bool UPlayViewBase::IsWaitingToSpawnPlayerShip() const
{
	return (TimeUntilNextPlayerShip > 0.0f);
}


void UPlayViewBase::KillPlayerShip()
{
	PlayerShip->SpawnExplosion();

	PlaySound(PlayerShipDestroyedSound);

	PlayerShip->Hide();

	AddPlayerShips(-1);

	PlayerShip->IsSpawning = true;
	TimeUntilNextPlayerShip = MaxTimeUntilNextPlayerShip;

	// ProcessPlayerShipSpawn() will handle the wait til next spawn and transition to game over, if needed. 
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
			Daylon::UpdateRoundedReadout(PlayerShieldReadout, PlayerShip->ShieldsLeft, ShieldsLeft);
			break;

		case EPowerup::Invincibility:
			Daylon::UpdateRoundedReadout(InvincibilityReadout, PlayerShip->InvincibilityLeft, InvincibilityLeft);
			break;
	}
}


void UPlayViewBase::ProcessPlayerShipCollision(float Mass, const FVector2D* Inertia)
{
	check(PlayerShip);

	if(!PlayerShip->ProcessCollision(Mass, Inertia))
	{
		KillPlayerShip();
	}
}



#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
