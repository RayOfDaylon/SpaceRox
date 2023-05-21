#include "Asteroids.h"
#include "PlayViewBase.h"

#define FEATURE_SPINNING_ASTEROIDS  1


void FAsteroids::Remove(int32 Index)
{
	if(!Asteroids.IsValidIndex(Index))
	{
		//UE_LOG(LogGame, Error, TEXT("RemoveAsteroid: bad index %d"), Index);
		return;
	}

	auto& Asteroid = *Asteroids[Index].Get();

	if(Asteroid.HasPowerup())
	{
		Daylon::Destroy(Asteroid.Powerup);
	}

	Daylon::Destroy(Asteroids[Index]);

	Asteroids.RemoveAtSwap(Index);
}


void FAsteroids::RemoveAll()
{
	for(int32 Index = Asteroids.Num() - 1; Index >= 0; Index--)
	{
		Remove(Index);		
	}
}


void FAsteroids::Update(UPlayViewBase& Arena, float DeltaTime)
{
	for (auto& Elem : Asteroids)
	{
		auto& Asteroid = *Elem.Get();

		if (Asteroid.LifeRemaining > 0.0f)
		{
			Asteroid.Move(DeltaTime, Arena.WrapPositionToViewport);

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


void FAsteroids::Kill(UPlayViewBase& Arena, int32 AsteroidIndex, bool KilledByPlayer)
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
		Arena.IncreasePlayerScoreBy(Asteroid.Value);
	}

	Arena.SpawnExplosion(Asteroid.UnwrappedNewPosition, Asteroid.Inertia);

	int32 SoundIndex = 0;
	
	if(Asteroid.Value == ValueMediumAsteroid)
	{
		SoundIndex = 1;
	}
	else if(Asteroid.Value == ValueSmallAsteroid)
	{
		SoundIndex = 2;
	}

	if(Arena.ExplosionSounds.IsValidIndex(SoundIndex))
	{
		Arena.PlaySound(Arena.ExplosionSounds[SoundIndex]);
	}


	// If asteroid was small, just delete it.
	if(Asteroid.Value == ValueSmallAsteroid)
	{
		// Release any powerup the asteroid was holding.

		if(Asteroid.HasPowerup())
		{
			auto PowerupIndex = Arena.Powerups.Add(Asteroid.Powerup);
			Asteroid.Powerup.Reset();

			if(PowerupsCanMove)
			{
				Arena.Powerups[PowerupIndex]->Inertia = Asteroid.Inertia;
			}
		}

		Remove(AsteroidIndex);

		return;
	}

	// Asteroid was not small, so split it up.

	Add(Asteroid.Split(Arena));
}
