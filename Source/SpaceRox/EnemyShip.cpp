// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "EnemyShip.h"
#include "DaylonUtils.h"
#include "PlayViewBase.h"
#include "Constants.h"


TSharedPtr<FEnemyShip> FEnemyShip::Create(UDaylonSpriteWidgetAtlas* Atlas, int Value, float RadiusFactor)
{
	auto Widget = SNew(FEnemyShip);

	Daylon::FinishCreating<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetCurrentCel(0);
	Widget->SetSize(Atlas->Atlas.GetCelPixelSize());
	Widget->UpdateWidgetSize();

	Widget->Value = Value;

	return Widget;
}


FEnemyShip::FEnemyShip()
{
	TimeRemainingToNextShot = (Value == ValueBigEnemy ? BigEnemyReloadTime : SmallEnemyReloadTime);
	TimeRemainingToNextMove = 3.0f;
}


void FEnemyShip::Perform(UPlayViewBase& Arena, float DeltaTime)
{
	// Don't call Super::Update(DeltaTime) because we are a static sprite.

	const bool WereBig = (Value == ValueBigEnemy);

	Move(DeltaTime, Arena.WrapPositionToViewport);


	// Fire a torpedo if we've finished reloading.

	TimeRemainingToNextShot -= DeltaTime;

	if(TimeRemainingToNextShot <= 0.0f)
	{
		TimeRemainingToNextShot = (WereBig ? BigEnemyReloadTime : SmallEnemyReloadTime);
		Shoot(Arena);
	}

	TimeRemainingToNextMove -= DeltaTime;

	if(TimeRemainingToNextMove <= 0.0f)
	{
		TimeRemainingToNextMove = 3.0f; // todo: use global constant instead of 3.0

		// Change heading (or stay on current heading).

		const FVector2D Headings[] = 
		{
			{ 1, 0 },
			{ 1, 1 },
			{ 1, -1 }
		};

		FVector2D NewHeading = Headings[FMath::RandRange(0, 2)];

		NewHeading.Normalize();

		const auto Facing = FMath::Sign(Inertia.X);
		Inertia = NewHeading * Inertia.Length();
		Inertia.X *= Facing;
	}
}



void FEnemyShip::Shoot(UPlayViewBase& Arena)
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!Arena.IsPlayerPresent())
	{
		return;
	}

	const int32 TorpedoIdx = Arena.GetAvailableTorpedo();

	if(TorpedoIdx == INDEX_NONE)
	{
		return;
	}

	auto& Torpedo = *Arena.Torpedos[TorpedoIdx].Get();

	Torpedo.FiredByPlayer = false;

	FVector2D Direction;
	float     Speed;

	// Position torpedo a little outside the ship.
	auto LaunchP = OldPosition;

	const bool WereBig = (Value == ValueBigEnemy);

	if(WereBig)
	{
		// Shot vector is random.
		Direction = UDaylonUtils::RandVector2D();
		LaunchP += Direction * (GetSize().X / 2 + 2);
		LaunchP = Arena.WrapPositionToViewport(LaunchP);

		Speed = BigEnemyTorpedoSpeed;
	}
	else
	{
		// Small enemy.
		// Shots switch between shooting at an available asteroid or the player.

		Speed = SmallEnemyTorpedoSpeed;

		FVector2D TargetP(-1);
		//const float Time = FMath::RandRange(0.75f, 1.25f);


		bShootAtPlayer = !bShootAtPlayer;

		if(bShootAtPlayer || Arena.Asteroids.IsEmpty())
		{
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Arena.PlayerShip->GetPosition(), Arena.PlayerShip->Inertia);
		}
		else
		{
			// Shoot at an asteroid.
			const auto& Asteroid = *Arena.Asteroids[FMath::RandRange(0, Arena.Asteroids.Num() - 1)].Get();
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Asteroid.GetPosition(), Asteroid.Inertia);
		}
	}

	Torpedo.Spawn(LaunchP, Direction * Speed, MaxTorpedoLifeTime);

	Arena.PlaySound(Arena.TorpedoSound);
}
