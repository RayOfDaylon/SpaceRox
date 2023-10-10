// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Logging.h"
#include "Constants.h"




// Set to 1 to enable debugging
#define DEBUG_MODULE                1



#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif

// Collision code from https://www.plasmaphysics.org.uk/programs/coll2d_cpp.htm

//******************************************************************************
//   This program is a 'remote' 2D-collision detector for two balls on linear
//   trajectories and returns, if applicable, the location of the collision for 
//   both balls as well as the new velocity vectors (assuming a partially elastic
//   collision as defined by the restitution coefficient).
//   The equations on which the code is based have been derived at
//   http://www.plasmaphysics.org.uk/collision2d.htm
//
//   In  'f' (free) mode no positions but only the initial velocities
//   and an impact angle are required.
//   All variables apart from 'mode' and 'error' are of Double Precision
//   Floating Point type.
//
//   The Parameters are:
//
//    mode  (char) (if='f' alpha must be supplied; otherwise arbitrary)
//    alpha (impact angle) only required in mode='f'; 
//                     should be between -PI/2 and PI/2 (0 = head-on collision))
//    R    (restitution coefficient)  between 0 and 1 (1=perfectly elastic collision)
//    m1   (mass of ball 1)
//    m2   (mass of ball 2)
//    r1   (radius of ball 1)        not needed for 'f' mode
//    r2   (radius of ball 2)                "
//  & x1   (x-coordinate of ball 1)          "
//  & y1   (y-coordinate of ball 1)          "
//  & x2   (x-coordinate of ball 2)          "
//  & y2   (y-coordinate of ball 2)          "
//  & vx1  (velocity x-component of ball 1) 
//  & vy1  (velocity y-component of ball 1)         
//  & vx2  (velocity x-component of ball 2)         
//  & vy2  (velocity y-component of ball 2)
//  & error (int)  (0: no error
//                  1: balls do not collide
//                  2: initial positions impossible (balls overlap))
//
//   Note that the parameters with an ampersand (&) are passed by reference,
//   i.e. the corresponding arguments in the calling program will be updated;
//   however, the coordinates and velocities will only be updated if 'error'=0.
//
//   All variables should have the same data types in the calling program
//   and all should be initialized before calling the function even if
//   not required in the particular mode.
//
//   This program is free to use for everybody. However, you use it at your own
//   risk and I do not accept any liability resulting from incorrect behaviour.
//   I have tested the program for numerous cases and I could not see anything 
//   wrong with it but I can not guarantee that it is bug-free under any 
//   circumstances.
//
//   I would appreciate if you could report any problems to me
//   (for contact details see  http://www.plasmaphysics.org.uk/feedback.htm ).
//
//   Thomas Smid, January  2004
//                December 2005 (corrected faulty collision detection; 
//                               a few minor changes to improve speed;
//                               added simplified code without collision detection)
//                December 2009 (generalization to partially inelastic collisions)
//*********************************************************************************

void collision2D
(
	char mode,
	double alpha, 
	double R,
    double m1, 
	double m2, 
	double r1, 
	double r2,
    double& x1, 
	double& y1, 
	double& x2, 
	double& y2,
    double& vx1, 
	double& vy1, 
	double& vx2, 
	double& vy2,
    int& error
)
{
    double  r12,m21,d,gammav,gammaxy,dgamma,dr,dc,sqs,t,
            dvx2,a,x21,y21,vx21,vy21,pi2,vx_cm,vy_cm;

//     ***initialize some variables ****
    pi2=2*acos(-1.0E0);
    error=0;
    r12=r1+r2;
    m21=m2/m1;
    x21=x2-x1;
    y21=y2-y1;
    vx21=vx2-vx1;
    vy21=vy2-vy1;

    vx_cm = (m1*vx1+m2*vx2)/(m1+m2) ;
    vy_cm = (m1*vy1+m2*vy2)/(m1+m2) ;

//     ****  return old positions and velocities if relative velocity =0 ****
    if ( vx21==0 && vy21==0 ) {error=1; return;}


//     *** calculate relative velocity angle             
    gammav=atan2(-vy21,-vx21);




//******** this block only if initial positions are given *********

    if (mode != 'f') 
	{
		d=sqrt(x21*x21 +y21*y21);
       
	//     **** return if distance between balls smaller than sum of radii ***
		if (d<r12) {error=2; return;}

	//     *** calculate relative position angle and normalized impact parameter ***
		gammaxy=atan2(y21,x21);
		dgamma=gammaxy-gammav;
			if (dgamma>pi2) {dgamma=dgamma-pi2;}
			else if (dgamma<-pi2) {dgamma=dgamma+pi2;}
		dr=d*sin(dgamma)/r12;
       
	//     **** return old positions and velocities if balls do not collide ***
		if (  (fabs(dgamma)>pi2/4 && fabs(dgamma)<0.75*pi2) || fabs(dr)>1 )   
			{error=1; return;}


	//     **** calculate impact angle if balls do collide ***
		alpha=asin(dr);

       
	//     **** calculate time to collision ***
		dc=d*cos(dgamma);
		if (dc>0) {sqs=1.0;} else {sqs=-1.0;}
		t=(dc-sqs*r12*sqrt(1-dr*dr))/sqrt(vx21*vx21+ vy21*vy21);
	//    **** update positions ***
		x1=x1+vx1*t;
		y1=y1+vy1*t;
		x2=x2+vx2*t;
		y2=y2+vy2*t;
	}

//******** END 'this block only if initial positions are given' *********
      
       
       
//     ***  update velocities ***

    a=tan( gammav +alpha);

    dvx2=-2*(vx21 +a*vy21) /((1+a*a)*(1+m21));
       
    vx2=vx2+dvx2;
    vy2=vy2+a*dvx2;
    vx1=vx1-m21*dvx2;
    vy1=vy1-a*m21*dvx2;

//     ***  velocity correction for inelastic collisions ***
	   
    vx1=(vx1-vx_cm)*R + vx_cm;
    vy1=(vy1-vy_cm)*R + vy_cm;
    vx2=(vx2-vx_cm)*R + vx_cm;
    vy2=(vy2-vy_cm)*R + vy_cm;
       

    return;
}



//******************************************************************************
//  Simplified Version
//  The advantage of the 'remote' collision detection in the program above is 
//  that one does not have to continuously track the balls to detect a collision. 
//  The program needs only to be called once for any two balls unless their 
//  velocity changes. However, if somebody wants to use a separate collision 
//  detection routine for whatever reason, below is a simplified version of the 
//  code which just calculates the new velocities, assuming the balls are already 
//  touching (this condition is important as otherwise the results will be incorrect)
//****************************************************************************


void collision2Ds
(
	double m1, 
	double m2, 
	double R,
    double x1, 
	double y1, 
	double x2, 
	double y2,
    double& vx1, 
	double& vy1, 
	double& vx2, 
	double& vy2
)
{
    double  m21,dvx2,a,x21,y21,vx21,vy21,fy21,sign,vx_cm,vy_cm;


    m21  = m2/m1;
    x21  = x2-x1;
    y21  = y2-y1;
    vx21 = vx2-vx1;
    vy21 = vy2-vy1;

    vx_cm = (m1*vx1+m2*vx2)/(m1+m2) ;
    vy_cm = (m1*vy1+m2*vy2)/(m1+m2) ;   


//     *** return old velocities if balls are not approaching ***
    if ( (vx21*x21 + vy21*y21) >= 0) 
	{
		return;
	}


//     *** I have inserted the following statements to avoid a zero divide; 
//         (for single precision calculations, 
//          1.0E-12 should be replaced by a larger value). **************  
  
    fy21=1.0E-12*fabs(y21);                            

    if ( fabs(x21)<fy21 ) 
	{  
        if (x21<0) { sign=-1; } else { sign=1;}  
        x21=fy21*sign; 
    } 

//     ***  update velocities ***
    a    = y21/x21;
    dvx2 = -2*(vx21 +a*vy21)/((1+a*a)*(1+m21)) ;
    vx2  = vx2+dvx2;
    vy2  = vy2+a*dvx2;
    vx1  = vx1-m21*dvx2;
    vy1  = vy1-a*m21*dvx2;

//     ***  velocity correction for inelastic collisions ***
    vx1 = (vx1-vx_cm)*R + vx_cm;
    vy1 = (vy1-vy_cm)*R + vy_cm;
    vx2 = (vx2-vx_cm)*R + vx_cm;
    vy2 = (vy2-vy_cm)*R + vy_cm;
}

// -------------------------------------------------------------------------------------


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
