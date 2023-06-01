// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#include "EnemyShips.h"
#include "Constants.h"
#include "Arena.h"
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

	Daylon::Uninstall(Ships[Index]);

	Ships.RemoveAtSwap(Index);
}


void FEnemyShips::RemoveBoss(int32 Index) 
{
	if(!Bosses.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid enemy boss index %d"), Index);
		return;
	}

	Daylon::UninstallImpl(Bosses[Index]);

	Bosses.RemoveAtSwap(Index);
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
		Daylon::Uninstall(PowerupPtr);
	}

	Daylon::Uninstall(Scavengers[Index]);

	Scavengers.RemoveAtSwap(Index);
}


void FEnemyShips::RemoveAll()
{
	while(!Ships.IsEmpty())
	{
		RemoveShip(0);
	}

	while(!Bosses.IsEmpty())
	{
		RemoveBoss(0);
	}

	while(!Scavengers.IsEmpty())
	{
		RemoveScavenger(0);
	}
}


void FEnemyShips::KillShip(int32 Index)
{
	if(!Ships.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid enemy ship index %d"), Index);
		return;
	}

	auto& Ship = *Ships[Index].Get();

	Arena->GetExplosions().SpawnOne(Ship.GetPosition(), EnemyShipExpolosionParams, Ship.Inertia);
	Arena->PlaySound(Arena->GetExplosionSound(Ship.Value == ValueBigEnemy ? 0 : 1));
	RemoveShip(Index);
}


void FEnemyShips::KillBoss(int32 Index)
{
	if(!Bosses.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid enemy boss index %d"), Index);
		return;
	}

	auto& Boss = *Bosses[Index].Get();

	// Spawn explosion for the ship at the center.

	Arena->GetExplosions().SpawnOne(Boss.GetPosition(), MiniBossExplosionParams, Boss.Inertia);

	Arena->PlaySound(Arena->GetExplosionSound(0)); // todo: use specific sound


	// Spawn explosion for any surviving shields.
	// todo: might be cool to blow the shields 1/4 or 1/2 second later.

	// Copy shield data into Params2.Particles array.

	FVector2D                   P1, P2;
	FDaylonLineParticle         Particle;
	FDaylonLineParticlesParams  Params2;

	for(auto& ShieldPtr : Boss.Shields)
	{
		for(int32 SegmentIndex = 0; SegmentIndex < ShieldPtr->GetNumSides(); SegmentIndex++)
		{
			auto Health = ShieldPtr->GetSegmentHealth(SegmentIndex);
			if(Health <= 0.0f)
			{
				continue;
			}

			ShieldPtr->GetSegmentGeometry(SegmentIndex, P1, P2);

			Particle.Color = FLinearColor(Health, Health, Health, 1.0f);
			Particle.Angle = UDaylonUtils::Vector2DToAngle(P2 - P1);
			Particle.P = (P1 + P2) / 2;
			Particle.Spin = FMath::RandRange(0, 7) == 0 ? FMath::FRandRange(-240.0f, 240.0f) : FMath::FRandRange(-120.0f, 120.0f);
			Particle.Length = (P2 - P1).Length();

			Params2.Particles.Add(Particle);
		}
	}

	Params2.LineThickness       = Boss.GetShieldThickness();
	Params2.MinParticleVelocity = 30.0f;
	Params2.MaxParticleVelocity = 80.0f;
	Params2.MinParticleLifetime =  0.5f;
	Params2.MaxParticleLifetime =  2.5f;
	Params2.FinalOpacity        = 0.25f;
	
	Arena->GetShieldExplosions().SpawnOne(Boss.GetPosition(), Params2, Boss.Inertia);

	RemoveBoss(Index);
}


// To easily place powerups along a series of concentric circles around a destroyed scavenger,
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


void FEnemyShips::KillScavenger(int32 Index)
{
	if(!Scavengers.IsValidIndex(Index))
	{
		UE_LOG(LogEnemyShips, Error, TEXT("Invalid scavenger index %d"), Index);
		return;
	}

	auto& Scavenger = *Scavengers[Index].Get();

	// Have scavenger drop any powerups it was carrying.

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
			DroppedPowerupPtr->SetPosition(Arena->WrapPosition(Scavenger.GetPosition() + CircleP));
			Arena->GetPowerups().Add(DroppedPowerupPtr);
		}
		Scavenger.AcquiredPowerups.Empty();
	}

	// The more full the scavenger, the more powerful the explosion.

	const float ExplosionScale = FMath::Clamp(NumAcquiredPowerups, 0, 10) * 0.1f;

	FDaylonParticlesParams Params;

	Params.MinParticleSize     = FMath::Lerp(2.0f, 6.0f, ExplosionScale);
	Params.MaxParticleSize     = FMath::Lerp(6.0f, 12.0f, ExplosionScale);
	Params.MinParticleVelocity = 30.0f;
	Params.MaxParticleVelocity = FMath::Lerp(120.0f, 180.0f, ExplosionScale);
	Params.MinParticleLifetime = FMath::Lerp(0.33f, 0.67f, ExplosionScale);
	Params.MaxParticleLifetime = FMath::Lerp(1.25f, 2.0f, ExplosionScale);
	Params.FinalOpacity        = 0.25f;
	Params.NumParticles        = FMath::Lerp(60, 120, ExplosionScale);

	Arena->GetExplosions().SpawnOne(Scavenger.GetPosition(), Params, Scavenger.Inertia);

	// todo: have scavenger explosion sound
	Arena->PlaySound(Arena->GetExplosionSound(0));
	RemoveScavenger(Index);
}


void FEnemyShips::SpawnShip()
{
	// Generate a big enemy ship vs. small one based on player score.
	// The higher the score, the likelier a small enemy will appear.
	// Regardless of score, there's always a 1/3 chance of a big enemy ship.

	const int32 ScoreTmp = FMath::Max(0, Arena->GetPlayerScore() - 5000);

	float BigEnemyProbability = pow(FMath::Lerp(1.0f, BigEnemyLowestProbability,  FMath::Min(1.0f, ScoreTmp / 65'000.0f)), 2.0f);
	BigEnemyProbability = FMath::Max(BigEnemyLowestProbability, BigEnemyProbability);

	const bool IsBigEnemy = (FMath::FRand() <= BigEnemyProbability);

	auto EnemyShipPtr = FEnemyShip::Create(Arena,
		IsBigEnemy ? Arena->GetBigEnemyAtlas() : Arena->GetSmallEnemyAtlas(), 
		IsBigEnemy ? ValueBigEnemy             : ValueSmallEnemy,
		0.375f);


	// Choose a random Y-pos to appear at. Leave room to avoid ship appearing clipped.
	FVector2D P(0.0, FMath::RandRange(EnemyShipPtr->GetSize().Y + 2, ViewportSize.Y - (EnemyShipPtr->GetSize().Y + 2)));

	auto Inertia = FVector2D(1, 0) * FMath::FRandRange(MinEnemyShipSpeed, MaxEnemyShipSpeed);

	if(FMath::RandBool())
	{
		Inertia.X *= -1; // make enemy ship travel from right to left.
		P.X = ViewportSize.X - 1.0f; // avoid immediate removal
	}

	EnemyShipPtr->Spawn(Arena->WrapPosition(P), Inertia, 0.0f);

	Ships.Add(EnemyShipPtr);

	if(IsBigEnemy)
	{
		NumBigEnemyShips++;
	}
	else
	{
		NumSmallEnemyShips++;
	}

	// Taper enemy ship volume quieter as player score increases.
	float VolumeScale = FMath::Clamp(UDaylonUtils::Normalize(Arena->GetPlayerScore(), 100'000, 30'000), 0.0f, 1.0f);
	VolumeScale = FMath::Lerp(0.5f, 1.0f, VolumeScale);

	if(IsBigEnemy)
	{
		if(NumBigEnemyShips == 1)
		{
			Arena->GetBigEnemySoundLoop().Start(VolumeScale);
		}
	}
	else
	{
		if(NumSmallEnemyShips == 1)
		{
			Arena->GetSmallEnemySoundLoop().Start(VolumeScale);
		}
	}
}


void FEnemyShips::SpawnBoss()
{
	// The higher the player score, the more likely the boss will have multiple shields.

	// Don't spawn if score too low.
	const int32 ScoreTmp = Arena->GetPlayerScore() - ScoreForBossSpawn;

	if(ScoreTmp < 0)
	{
		return;
	}

	float DualShieldProbability = pow(FMath::Lerp(1.0f, 0.1f,  FMath::Min(1.0f, (float)ScoreTmp / ScoreForBossToHaveDualShields)), 2.0f);
	DualShieldProbability = FMath::Max(0.1f, DualShieldProbability);

	const bool IsDualShielded = (FMath::FRand() > DualShieldProbability);

	const int32 NumShields = IsDualShielded ? 2 : 1;

	TSharedPtr<FEnemyBoss> BossShipPtr;

	switch(NumShields)
	{
		case 1: BossShipPtr = FEnemyBoss::Create(Arena, Arena->GetMiniboss1Atlas(), 32, ValueMiniBoss1, NumShields); break;
		case 2: BossShipPtr = FEnemyBoss::Create(Arena, Arena->GetMiniboss2Atlas(), 32, ValueMiniBoss2, NumShields); break;
	}

	// Like an asteroid, start at some random edge place with a random inertia.

	FVector2D P(0);

	if(FMath::RandBool())
	{
		P.X = FMath::RandRange(0.0, ViewportSize.X);
	}
	else
	{
		P.Y = FMath::RandRange(0.0, ViewportSize.Y);
	}

	BossShipPtr->SetPosition(P);
	BossShipPtr->Inertia = UDaylonUtils::RandVector2D() * FMath::FRandRange(MinMinibossSpeed, MaxMinibossSpeed);

	Bosses.Add(BossShipPtr);
}


void FEnemyShips::Update(float DeltaTime)
{
	check(NumBigEnemyShips + NumSmallEnemyShips == Ships.Num());

	for(int32 ShipIndex = Ships.Num() - 1; ShipIndex >= 0; ShipIndex--)
	{
		auto& EnemyShip = GetShip(ShipIndex);

		// If we've reached the opposite side of the viewport, remove us.
		const auto P2 = Arena->WrapPosition(EnemyShip.UnwrappedNewPosition);

		if(P2.X != EnemyShip.UnwrappedNewPosition.X)
		{
			RemoveShip(ShipIndex);
			continue;
		}

		EnemyShip.Perform(DeltaTime);
	}

#if(FEATURE_MULTIPLE_ENEMIES == 0)
	if(EnemyShips.IsEmpty())
#endif
	{
		// See if we need to spawn an enemy ship.
		// Time between enemy ship spawns depends on asteroid density (no ships if too many rocks)
		// and while low density, spawn once every ten seconds down to two seconds depending on player score.

		if(Arena->AdjustTimeUntilNextEnemyShip(-DeltaTime) <= 0.0f)
		{
			SpawnShip();

			// Reset the timer.
			Arena->SetTimeUntilNextEnemyShip(FMath::Lerp(MaxTimeUntilEnemyRespawn, MinTimeUntilEnemyRespawn, (float)FMath::Min(ExpertPlayerScore, Arena->GetPlayerScore()) / ExpertPlayerScore));
			// score = 10'000 --> 9 seconds
			//         20'000 --> 8 seconds
			//         50'000 --> 5 seconds
			//        100'000+ --> 1 second
			// might want to apply a gamma curve to speed up spawning at lower scores
		}
	}

	if(NumBigEnemyShips > 0)
	{
		Arena->GetBigEnemySoundLoop().Tick(DeltaTime);
	}

	if(NumSmallEnemyShips > 0)
	{
		Arena->GetSmallEnemySoundLoop().Tick(DeltaTime);
	}


	for(auto& BossPtr : Bosses)
	{
		BossPtr->Update(DeltaTime);
		BossPtr->Perform(DeltaTime);
	}

	if(Arena->AdjustTimeUntilNextBoss(-DeltaTime) <= 0.0f)
	{
		SpawnBoss();
		Arena->SetTimeUntilNextBoss(FMath::Lerp(MaxTimeUntilBossRespawn, MinTimeUntilBossRespawn, (float)FMath::Min(ExpertPlayerScore, Arena->GetPlayerScore()) / ExpertPlayerScore));
	}


#if(FEATURE_SCAVENGERS == 1)
	
	for(int32 ScavengerIndex = Scavengers.Num() - 1; ScavengerIndex >= 0; ScavengerIndex--)
	{
		auto& Scavenger = GetScavenger(ScavengerIndex);

		Scavenger.Update(DeltaTime);

		if(Scavenger.CurrentTarget.IsValid())
		{
			// Keep moving toward target.
			Scavenger.Move(DeltaTime, Arena->GetWrapPositionFunction());
		}
		else
		{
			// No current target.

			if(!Arena->GetPowerups().IsEmpty())
			{
				// Powerups exist, assign ourselves one as a target.
				// Pick the one closest to us.
				// Note that we don't care if multiple scavengers target the same powerup.

				double ShortestDistance = 1.0e7;
				int32 NearestPowerupIndex = 0;
				int32 Index = 0;

				for(auto PowerupPtr : Arena->GetPowerups())
				{
					const auto Distance = FVector2D::Distance(Scavenger.GetPosition(), PowerupPtr.Get()->GetPosition());
					
					if(Distance < ShortestDistance)
					{
						ShortestDistance = Distance;
						NearestPowerupIndex = Index;
					}
					Index++;
				}

				Scavenger.CurrentTarget = Arena->GetPowerups()[NearestPowerupIndex];

				// Point the scavenger at the powerup.
				Scavenger.Inertia = (Arena->GetPowerups()[NearestPowerupIndex].Get()->GetPosition() - Scavenger.GetPosition());
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

				Scavenger.Move(DeltaTime, Arena->GetWrapPositionFunction());

				// If we've reached the opposite side of the viewport, remove us.
				const auto P2 = Arena->WrapPosition(Scavenger.UnwrappedNewPosition);

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
		if(Arena->AdjustTimeUntilNextScavenger(-DeltaTime) <= 0.0f)
		{
			Arena->SetTimeUntilNextScavenger(MaxTimeUntilNextScavenger);

			auto ScavengerPtr = FScavenger::Create(Arena->GetScavengerAtlas(), FVector2D(32));

			ScavengerPtr->SetPosition(FVector2D(0, FMath::FRandRange(ViewportSize.Y * 0.1, ViewportSize.Y * 0.9)));
			ScavengerPtr->Inertia.Set(MaxScavengerSpeed, 0);
			ScavengerPtr->SetAngle(UDaylonUtils::Vector2DToAngle(ScavengerPtr->Inertia));

			Scavengers.Add(ScavengerPtr);
		}
	}

#endif
}
