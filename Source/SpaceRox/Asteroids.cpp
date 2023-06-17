#include "Asteroids.h"
#include "Arena.h"

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
		Daylon::Uninstall(Asteroid.Powerup);
	}

	Daylon::Uninstall(Asteroids[Index]);

	Asteroids.RemoveAtSwap(Index);
}


void FAsteroids::RemoveAll()
{
	for(int32 Index = Asteroids.Num() - 1; Index >= 0; Index--)
	{
		Remove(Index);		
	}
}


void FAsteroids::Update(float DeltaTime)
{
	check(Arena);

	for (auto& Elem : Asteroids)
	{
		auto& Asteroid = *Elem.Get();

		if (Asteroid.LifeRemaining > 0.0f)
		{
			Asteroid.Move(DeltaTime, Arena->GetWrapPositionFunction());

#if(FEATURE_ASTEROID_FADE_IN == 1)
			Asteroid.Update(DeltaTime);
#endif
			if(Asteroid.HasPowerup())
			{
				Asteroid.Powerup->SetPosition(Asteroid.GetPosition());
				Asteroid.Powerup->Update(DeltaTime);
			}
#if(FEATURE_SPINNING_ASTEROIDS == 1)
			Asteroid.SetAngle(Daylon::WrapAngle(Asteroid.GetAngle() + Asteroid.SpinSpeed * DeltaTime));
#endif
		}
	}
}


void FAsteroids::Kill(int32 AsteroidIndex, bool KilledByPlayer)
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
		Arena->IncreasePlayerScoreBy(Asteroid.Value);
	}

	Arena->GetExplosions().SpawnOne(Asteroid.UnwrappedNewPosition, Asteroid.Inertia);

	int32 SoundIndex = 0;
	
	if(Asteroid.Value == ValueMediumAsteroid)
	{
		SoundIndex = 1;
	}
	else if(Asteroid.Value == ValueSmallAsteroid)
	{
		SoundIndex = 2;
	}

	Arena->PlaySound(Arena->GetExplosionSound(SoundIndex));


	// If asteroid was small, just delete it.
	if(Asteroid.Value == ValueSmallAsteroid)
	{
		// Release any powerup the asteroid was holding.

		if(Asteroid.HasPowerup())
		{
			auto PowerupIndex = Arena->GetPowerups().Add(Asteroid.Powerup);
			Asteroid.Powerup.Reset();

			if(PowerupsCanMove)
			{
				Arena->GetPowerups()[PowerupIndex]->Inertia = Asteroid.Inertia;
			}
		}

		Remove(AsteroidIndex);

		return;
	}

	// Asteroid was not small, so split it up.

	Add(Asteroid.Split());
}
