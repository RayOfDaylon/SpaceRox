// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#include "EnemyShips.h"
#include "Constants.h"
#include "PlayViewBase.h"
#include "DaylonUtils.h"


#define FEATURE_MULTIPLE_ENEMIES    1
#define FEATURE_SCAVENGERS          1


DECLARE_LOG_CATEGORY_EXTERN(LogEnemyShips, Log, All);
DEFINE_LOG_CATEGORY(LogEnemyShips)



void FEnemyShips::RemoveShip(int32 Index) 
{
	if(!Ships.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid enemy ship index %d"), Index);
		return;
	}

	auto& Ship = *Ships[Index].Get();

	if(Ship.Value == ValueBigEnemy)
	{
		NumBigEnemyShips--;
	}
	else
	{
		NumSmallEnemyShips--;
	}

	check(NumBigEnemyShips >= 0 && NumSmallEnemyShips >= 0);

	Daylon::Destroy(Ships[Index]);

	Ships.RemoveAtSwap(Index);
}


void FEnemyShips::RemoveScavenger(int32 Index)
{
	if(!Scavengers.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid Scavanger Index %d"), Index);
		return;
	}

	auto& Scavenger = *Scavengers[Index].Get();

	for(auto PowerupPtr : Scavenger.AcquiredPowerups)
	{
		Daylon::Destroy(PowerupPtr);
	}

	Daylon::Destroy(Scavengers[Index]);

	Scavengers.RemoveAtSwap(Index);
}


void FEnemyShips::RemoveAll ()
{
	while(!Ships.IsEmpty())
	{
		RemoveShip(0);
	}

	while(!Scavengers.IsEmpty())
	{
		RemoveScavenger(0);
	}
}


void FEnemyShips::KillShip(UPlayViewBase& Arena, int32 Index)
{
	if(!Ships.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid enemy ship index %d"), Index);
		return;
	}

	auto& Ship = *Ships[Index].Get();

	//Arena.SpawnExplosion(Ship.GetPosition());
/*
	float MinParticleSize,
	float MaxParticleSize,
	float MinParticleVelocity,
	float MaxParticleVelocity,
	float MinParticleLifetime,
	float MaxParticleLifetime,
	float FinalOpacity,
	int32 NumParticles
*/
/*
 playership 2nd explosion:
 			4.5f,
			9.0f,
			45.0f,
			240.0f,
			0.5f,
			4.0f,
			0.25f,
			80);

*/ 
	Arena.Explosions.SpawnOne(
		Arena, 
		Ship.GetPosition(),
		 3.0f, 
		 8.0f, 
		30.0f, 
       180.0f, 
		 0.33f, 
		 1.5f, 
		 0.25f, 
		60,
		Ship.Inertia);

	Arena.PlaySound(Arena.ExplosionSounds[Ship.Value == ValueBigEnemy ? 0 : 1]);
	RemoveShip(Index);
}


// To easily place powerups along a series of concentric circles around the destroyed scavenger,
// make an array whose elements provide the circle's radius and an angle along the circle.
struct FPowerupPlacement
{
	float CircleRadius;
	float Angle;
};

const FPowerupPlacement PowerupPlacements[] = 
{
	{ 0.0f,   0.0f },
	{ 1.0f,   0.0f },
	{ 1.0f,  60.0f },
	{ 1.0f, 120.0f },
	{ 1.0f, 180.0f },
	{ 1.0f, 240.0f },
	{ 1.0f, 300.0f },
	{ 2.0f,   0.0f },
	{ 2.0f,  30.0f },
	{ 2.0f,  60.0f },
	{ 2.0f,  90.0f },
	{ 2.0f, 120.0f },
	{ 2.0f, 150.0f },
	{ 2.0f, 180.0f },
	{ 2.0f, 210.0f },
	{ 2.0f, 240.0f },
	{ 2.0f, 270.0f },
	{ 2.0f, 300.0f },
	{ 2.0f, 330.0f }
};


void FEnemyShips::KillScavenger(UPlayViewBase& Arena, int32 Index)
{
	if(!Scavengers.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid scavenger index %d"), Index);
		return;
	}

	auto& Scavenger = *Scavengers[Index].Get();

	// Have scavenger drop any powerups it was carrying.

	// For now, place them in a line trailing away from the scavenger.
	// todo: use a spiral. Each groove needs to be DroppedPowerupPtr->GetRadius*2 distant.

	const int32 NumAcquiredPowerups = Scavenger.AcquiredPowerups.Num();

	if(NumAcquiredPowerups > 0)
	{
		// All powerups have the same radius.
		const auto PowerupDiameter = Scavenger.AcquiredPowerups[0]->GetRadius() * 2;

		//FVector2D Direction = Scavenger.Inertia;
		//Direction.Normalize();
		//Direction *= -1; // Start from the scavenger's position and work backwards.

		for(int32 PowerupIndex = 0; PowerupIndex < NumAcquiredPowerups; PowerupIndex++)
		{
			if(PowerupIndex >= sizeof(PowerupPlacements) / sizeof(PowerupPlacements[0]))
			{
				continue;
			}
			const auto& Placement = PowerupPlacements[PowerupIndex];

			auto DroppedPowerupPtr = Scavenger.AcquiredPowerups[PowerupIndex];
			DroppedPowerupPtr->Show();
			//DroppedPowerupPtr->SetPosition(Arena.WrapPositionToViewport(Scavenger.GetPosition() + (Direction * DroppedPowerupPtr->GetRadius() * 2.5f * PowerupIndex)));
			const FVector2D CircleP = UDaylonUtils::AngleToVector2D(Placement.Angle) * PowerupDiameter * Placement.CircleRadius;
			DroppedPowerupPtr->SetPosition(Arena.WrapPositionToViewport(Scavenger.GetPosition() + CircleP));
			Arena.Powerups.Add(DroppedPowerupPtr);
		}
		Scavenger.AcquiredPowerups.Empty();
	}

	// The more full the scavenger, the more powerful the explosion.

	const float ExplosionScale = FMath::Clamp(NumAcquiredPowerups, 0, 10) * 0.1f;

	Arena.Explosions.SpawnOne(
		Arena,
		Scavenger.GetPosition(), 
		 FMath::Lerp(2.0f, 6.0f, ExplosionScale), 
		 FMath::Lerp(6.0f, 12.0f, ExplosionScale), 
		30.0f, 
       FMath::Lerp(120.0f, 180.0f, ExplosionScale),
		 FMath::Lerp(0.33f, 0.67f, ExplosionScale), 
		 FMath::Lerp(1.25f, 2.0f, ExplosionScale), 
		 0.25f, 
		FMath::Lerp(60, 120, ExplosionScale),
		Scavenger.Inertia);

	// todo: have scavenger explosion sound
	Arena.PlaySound(Arena.ExplosionSounds[0]);
	RemoveScavenger(Index);
}


void FEnemyShips::SpawnShip(UPlayViewBase& Arena)
{
	// Generate a big enemy ship vs. small one based on player score.
	// The higher the score, the likelier a small enemy will appear.
	// Regardless of score, there's always a 10% chance of a big enemy ship.

	const int32 ScoreTmp = FMath::Max(0, Arena.PlayerScore - 5000);

	float BigEnemyProbability = pow(FMath::Lerp(1.0f, 0.1f,  FMath::Min(1.0f, ScoreTmp / 65'000.0f)), 2.0f);
	BigEnemyProbability = FMath::Max(0.1f, BigEnemyProbability);

	const bool IsBigEnemy = (FMath::FRand() <= BigEnemyProbability);


	auto EnemyShipPtr = FEnemyShip::Create(
		IsBigEnemy ? Arena.BigEnemyAtlas : Arena.SmallEnemyAtlas, 
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

	EnemyShipPtr->Spawn(Arena.WrapPositionToViewport(P), Inertia, 0.0f);

	Ships.Add(EnemyShipPtr);

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
			Arena.BigEnemyShipSoundLoop.Start();
		}
	}
	else
	{
		if(NumSmallEnemyShips == 1)
		{
			Arena.SmallEnemyShipSoundLoop.Start();
		}
	}
}


void FEnemyShips::Update(UPlayViewBase& Arena, float DeltaTime)
{
	check(NumBigEnemyShips + NumSmallEnemyShips == Ships.Num());

	for(int32 ShipIndex = Ships.Num() - 1; ShipIndex >= 0; ShipIndex--)
	{
		auto& EnemyShip = GetShip(ShipIndex);

		// If we've reached the opposite side of the viewport, remove us.
		const auto P2 = Arena.WrapPositionToViewport(EnemyShip.UnwrappedNewPosition);

		if(P2.X != EnemyShip.UnwrappedNewPosition.X)
		{
			RemoveShip(ShipIndex);
			continue;
		}

		EnemyShip.Perform(Arena, DeltaTime);
	}

#if(FEATURE_MULTIPLE_ENEMIES == 0)
	if(EnemyShips.IsEmpty())
#endif
	{
		// See if we need to spawn an enemy ship.
		// Time between enemy ship spawns depends on asteroid density (no ships if too many rocks)
		// and while low density, spawn once every ten seconds down to two seconds depending on player score.

		Arena.TimeUntilNextEnemyShip -= DeltaTime;

		if(Arena.TimeUntilNextEnemyShip <= 0.0f)
		{
			SpawnShip(Arena);

			// Reset the timer.
			Arena.TimeUntilNextEnemyShip = FMath::Lerp(MaxTimeUntilEnemyRespawn, MinTimeUntilEnemyRespawn, (float)FMath::Min(ExpertPlayerScore, Arena.PlayerScore.GetValue()) / ExpertPlayerScore);
			// score = 10'000 --> 9 seconds
			//         20'000 --> 8 seconds
			//         50'000 --> 5 seconds
			//        100'000+ --> 1 second
			// might want to apply a gamma curve to speed up spawning at lower scores
		}
	}

	if(NumBigEnemyShips > 0)
	{
		Arena.BigEnemyShipSoundLoop.Tick(DeltaTime);
	}

	if(NumSmallEnemyShips > 0)
	{
		Arena.SmallEnemyShipSoundLoop.Tick(DeltaTime);
	}


#if(FEATURE_SCAVENGERS == 1)
	
	for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		auto& Scavenger = GetScavenger(ScavengerIndex);

		Scavenger.Update(DeltaTime);

		if(Scavenger.CurrentTarget.IsValid())
		{
			// Keep moving toward target.
			Scavenger.Move(DeltaTime, Arena.WrapPositionToViewport);
		}
		else
		{
			// No current target.

			if(!Arena.Powerups.IsEmpty())
			{
				// Powerups exist, assign ourselves one as a target.
				// Pick the one closest to us.
				// Note that we don't care if multiple scavengers target the same powerup.

				double ShortestDistance = 1.0e7;
				int32 NearestPowerupIndex = 0;
				int32 Index = 0;

				for(auto PowerupPtr : Arena.Powerups)
				{
					const auto Distance = FVector2D::Distance(Scavenger.GetPosition(), PowerupPtr.Get()->GetPosition());
					
					if(Distance < ShortestDistance)
					{
						ShortestDistance = Distance;
						NearestPowerupIndex = Index;
					}
					Index++;
				}

				Scavenger.CurrentTarget = Arena.Powerups[NearestPowerupIndex];

				// Point the scavenger at the powerup.
				Scavenger.Inertia = (Arena.Powerups[NearestPowerupIndex].Get()->GetPosition() - Scavenger.GetPosition());
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

				Scavenger.Move(DeltaTime, Arena.WrapPositionToViewport);

				// If we've reached the opposite side of the viewport, remove us.
				const auto P2 = Arena.WrapPositionToViewport(Scavenger.UnwrappedNewPosition);

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
		Arena.TimeUntilNextScavenger -= DeltaTime;

		if(Arena.TimeUntilNextScavenger <= 0.0f)
		{
			Arena.TimeUntilNextScavenger = MaxTimeUntilNextScavenger;

			auto ScavengerPtr = FScavenger::Create(Arena.ScavengerAtlas, FVector2D(32));

			ScavengerPtr->SetPosition(FVector2D(0, FMath::FRandRange(ViewportSize.Y * 0.1, ViewportSize.Y * 0.9)));
			ScavengerPtr->Inertia.Set(MaxScavengerSpeed, 0);
			ScavengerPtr->SetAngle(UDaylonUtils::Vector2DToAngle(ScavengerPtr->Inertia));

			Scavengers.Add(ScavengerPtr);
		}
	}

#endif
}
