// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Logging.h"
#include "Constants.h"




// Set to 1 to enable debugging
#define DEBUG_MODULE                0

#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif


void UPlayViewBase::CheckCollisions()
{
	// Build a triangle representing the player ship.

	FVector2D PlayerShipTriangle[3]; // tip, LR corner, LL corner.
	FVector2D PlayerShipLineStart;
	FVector2D PlayerShipLineEnd;

	if(IsPlayerShipPresent())
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
			Triangle = Daylon::Rotate(Triangle, ShipAngle);

			// Use the ship's old position because the current position can cause unwanted self-intersections.
			Triangle += PlayerShip->OldPosition;
		}

		// Get the line segment for the player ship.
		// Line segments are used to better detect collisions involving fast-moving objects.

		PlayerShipLineStart = PlayerShip->OldPosition;
		PlayerShipLineEnd   = PlayerShip->UnwrappedNewPosition;
	}

	// See what the active torpedos have collided with.

	for(auto& TorpedoPtr : Torpedos)
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

		// See if torpedo hit any rocks.

		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			if(Daylon::DoesLineSegmentIntersectCircle(OldP, CurrentP, Asteroid.GetPosition(), Asteroid.GetRadius())
				|| Daylon::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, OldP, Asteroid.GetRadius()))
			{
				// A torpedo hit a rock.

				Torpedo.Kill();
				Asteroids.Kill(AsteroidIndex, Torpedo.FiredByPlayer);

				// We don't need to check any other asteroids.
				break;
			}
		}
		
		if(Torpedo.IsAlive() && !Torpedo.FiredByPlayer && IsPlayerShipPresent())
		{
			// Torpedo didn't hit a rock and the player didn't fire it, so see if it hit the player ship.

			if(Daylon::DoesLineSegmentIntersectTriangle(OldP, CurrentP, PlayerShipTriangle))
			{
				Torpedo.Kill();
				SpawnExplosion(OldP, PlayerShip->Inertia);
				ProcessPlayerShipCollision();
			}
		}

		if(Torpedo.IsAlive() && Torpedo.FiredByPlayer)
		{
			// See if the torpedo hit an enemy ship.

			for(int32 EnemyIndex = EnemyShips.NumShips() - 1; EnemyIndex >= 0; EnemyIndex--)
			{
				auto& EnemyShip = EnemyShips.GetShip(EnemyIndex);

				if(Daylon::DoesLineSegmentIntersectCircle(OldP, CurrentP, EnemyShip.GetPosition(), EnemyShip.GetRadius()))
				{
					Torpedo.Kill();
					IncreasePlayerScoreBy(EnemyShip.Value);
					EnemyShips.KillShip(EnemyIndex);
					break;
				}
			}

			// See if the torpedo hit a scavenger.
			if(Torpedo.IsAlive())
			{
				for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
				{
					auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

					if(Daylon::DoesLineSegmentIntersectCircle(OldP, CurrentP, Scavenger.GetPosition(), Scavenger.GetRadius()))
					{
						Torpedo.Kill();
						IncreasePlayerScoreBy(Scavenger.Value);
						EnemyShips.KillScavenger(ScavengerIndex);
						break;
					}
				}
			}
		}


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
						EnemyShips.KillBoss(BossIndex);
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

	if(IsPlayerShipPresent())
	{
		for(int32 AsteroidIndex = 0; AsteroidIndex < Asteroids.Num(); AsteroidIndex++)
		{
			auto& Asteroid = Asteroids.Get(AsteroidIndex);

			#define SHIP_INTERSECTS_ASTEROID  Daylon::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Asteroid.GetPosition(), Asteroid.GetRadius() + PlayerShip->GetRadius())
			#define ASTEROID_INTERSECTS_SHIP  Daylon::DoesLineSegmentIntersectTriangle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, PlayerShipTriangle)
	
			if(SHIP_INTERSECTS_ASTEROID || ASTEROID_INTERSECTS_SHIP)
			{
				// Player collided with a rock.
				static const TMap<int, float> AsteroidMasses = 
				{
					{ ValueBigAsteroid,    BigAsteroidMass    },
					{ ValueMediumAsteroid, MediumAsteroidMass }, 
					{ ValueSmallAsteroid,  SmallAsteroidMass  }
				};

				if(AsteroidInertiaImpart == 0.0f)
				{
					ProcessPlayerShipCollision();
				}
				else
				{
					const FVector2D AsteroidPosition = Asteroid.GetPosition();
					ProcessPlayerShipCollision(AsteroidMasses[Asteroid.Value] * AsteroidInertiaImpart, &AsteroidPosition, &Asteroid.Inertia);
				}

				Asteroids.Kill(AsteroidIndex, CreditPlayerForKill);

				break;
			}

			#undef SHIP_INTERSECTS_ASTEROID 
			#undef ASTEROID_INTERSECTS_SHIP
		}
	}


	// Check if enemy ship collided with the player

	if(IsPlayerShipPresent())
	{
		for(int32 EnemyIndex = EnemyShips.NumShips() - 1; EnemyIndex >= 0; EnemyIndex--)
		{
			auto& EnemyShip = EnemyShips.GetShip(EnemyIndex);

			if (Daylon::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, EnemyShip.OldPosition, EnemyShip.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, EnemyShip.UnwrappedNewPosition) < EnemyShip.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(EnemyShip.Value);
				EnemyShips.KillShip(EnemyIndex);

				ProcessPlayerShipCollision();

				break;
			}
		}
	}


	// Check if scavenger collided with the player

	if(IsPlayerShipPresent())
	{
		for(int32 ScavengerIndex = EnemyShips.NumScavengers() - 1; ScavengerIndex >= 0; ScavengerIndex--)
		{
			auto& Scavenger = EnemyShips.GetScavenger(ScavengerIndex);

			if (Daylon::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Scavenger.OldPosition, Scavenger.GetRadius())
				|| FVector2D::Distance(PlayerShip->UnwrappedNewPosition, Scavenger.UnwrappedNewPosition) < Scavenger.GetRadius() + PlayerShip->GetRadius())
			{
				// Enemy ship collided with player ship.

				IncreasePlayerScoreBy(Scavenger.Value);
				EnemyShips.KillScavenger(ScavengerIndex);

				ProcessPlayerShipCollision();

				break;
			}
		}
	}


	// Check if player ship collided with a boss

	if(IsPlayerShipPresent())
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
			
			ProcessPlayerShipCollision();

			// Player will have died if it wasn't shielded.

			if(Part == 0) 
			{ 
				IncreasePlayerScoreBy(Boss.Value);
				EnemyShips.KillBoss(BossIndex);
				break;
			} 

			// Player hit a boss' shield.

			SpawnExplosion(PlayerShipLineEnd, FVector2D(0));
			PlaySound(ShieldBonkSound, 0.5f);
			float PartHealth = Boss.GetShieldSegmentHealth(Part, ShieldSegmentIndex);
			PartHealth = FMath::Max(0.0f, PartHealth - 0.25f);
			Boss.SetShieldSegmentHealth(Part, ShieldSegmentIndex, PartHealth);

			if(IsPlayerShipPresent())
			{
				// Player was shielded or invincible (or in god mode)
				// so do an elastic collision.

				auto ShieldImpactNormal = (HitPt - Boss.UnwrappedNewPosition);
				ShieldImpactNormal.Normalize();

				// Have to treat player ship speed below 1.0 as 1.0 to prevent possible infinite loop during inertia scaling.
				const auto BounceForce = ShieldImpactNormal * FMath::Max(1.0f, PlayerShip->GetSpeed());

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


	// Check if player ship collided with a powerup

	if(IsPlayerShipPresent())
	{
		for(int32 PowerupIndex = Powerups.Num() - 1; PowerupIndex >= 0; PowerupIndex--)
		{
			auto& Powerup = *Powerups[PowerupIndex].Get();

			if (Daylon::DoesLineSegmentIntersectCircle(PlayerShipLineStart, PlayerShipLineEnd, Powerup.OldPosition, Powerup.GetRadius())
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

						PlaySound(GainShieldPowerupSound);
						PlayerShip->AdjustShieldsLeft(ShieldPowerupIncrease);
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

			if(Daylon::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, EnemyShip.GetPosition(), EnemyShip.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(EnemyShip.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + EnemyShip.GetRadius())
			{
				// Enemy ship collided with a rock.

				EnemyShips.KillShip(EnemyIndex);
				Asteroids.Kill(AsteroidIndex, DontCreditPlayerForKill);

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

			if(Daylon::DoesLineSegmentIntersectCircle(Asteroid.OldPosition, Asteroid.UnwrappedNewPosition, Scavenger.GetPosition(), Scavenger.GetRadius())
				|| FVector2D::Distance(WrapPositionToViewport(Scavenger.UnwrappedNewPosition), Asteroid.OldPosition) < Asteroid.GetRadius() + Scavenger.GetRadius())
			{
				// Scavenger collided with a rock.

				EnemyShips.KillScavenger(ScavengerIndex);
				Asteroids.Kill(AsteroidIndex, DontCreditPlayerForKill);

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

			Asteroids.Kill(AsteroidIndex, DontCreditPlayerForKill);

			if(Part == 0)
			{
				// Asteroid hit boss center.
				EnemyShips.KillBoss(BossIndex);
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



#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
