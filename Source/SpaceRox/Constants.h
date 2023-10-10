// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once

#include "SDaylonParticles.h"


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
const int32 MaxPlayerShipsDisplayable      = 10;      // We don't want the player ships readout to be impractically wide.
const float MaxTimeUntilNextPlayerShip     =  4.0f;   // Actual time may be longer because of asteroid intersection avoidance.
							           
const float MaxPlayerShipSpeed             = 1000.0f; // px/sec
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

// Asteroid masses and inertial impart are used to alter player ship inertia during non-fatal collisions.
// If you don't like the player ship to bounce around, leave AsteroidInertiaImpart at zero.
const float BigAsteroidMass                =  1.0f;
const float MediumAsteroidMass             =  0.3f;
const float SmallAsteroidMass              =  0.1f;
const float PlayerShipMass                 = SmallAsteroidMass;
const float AsteroidInertiaImpart          =  0.25f;   // How much inertia to impart to shielded player ship when hit by asteroid.
								           
const int32 ValueBigAsteroid               =    20;
const int32 ValueMediumAsteroid            =    50;
const int32 ValueSmallAsteroid             =   100;
const int32 ValueBigEnemy                  =   200;
const int32 ValueSmallEnemy                =  1000;
const int32 ValueScavenger                 =   500;
const int32 ValueMiniBoss1                 =  1500;
const int32 ValueMiniBoss2                 =  3000;
							           
const float MaxIntroStateLifetime          =  5.0f;  // How long the initial intro screen is visible before the main menu appears.
const float TimeBetweenWaves               =  3.0f;  // Number of seconds between each wave.
const float MaxTimeUntilGameOverStateEnds  =  5.0f;  // Time to wait between game over screen and idle screen.

const float MaxTimeUntilNextEnemyShip      = 20.0f;  // Let each wave start with a breather.
const float MaxTimeUntilEnemyRespawn       = 10.0f;  // Longest delay between successive enemy ship spawns. Favored when player score is low.
const float MinTimeUntilEnemyRespawn       =  2.0f;  // Shortest delay between successive enemy ship spawns. Favored more as player score increases.

const float MinTimeTilNextEnemyShipMove    = 2.5f;
const float MaxTimeTilNextEnemyShipMove    = 3.5f;

const float MinEnemyShipSpeed              = 250.0f;
const float MaxEnemyShipSpeed              = 350.0f;

const float MinMinibossSpeed               =  90.0f;
const float MaxMinibossSpeed               = 175.0f;

const float BigEnemyLowestProbability      = 0.33f;  // Lowest chance of a big enemy spwaning vs. a small enemy.

const int32 ScoreForBigEnemyAimWorst       =       0;  // Player score at which big enemy ships fire worst.
const int32 ScoreForBigEnemyAimPerfect     = 400'000;  // Player score at which big enemy ships fires perfectly.
const int32 ScoreForSmallEnemyAimWorst     =  25'000;  // Player score at which small enemy ships fire worst.
const int32 ScoreForSmallEnemyAimPerfect   = 200'000;  // Player score at which small enemy ships fires perfectly.
const int32 ScoreForBossSpawn              =  30'000;  // Player score must be this high for bosses to appear.
const int32 ScoreForBossAimPerfect         = 150'000; // At this score, bosses aim perfectly.
const int32 ScoreForBossToHaveDualShields  = 100'000;

const float MaxTimeUntilNextBoss           = 22.0f;  // Let each wave start with a breather.
const float MaxTimeUntilBossRespawn        = 20.0f;  
const float MinTimeUntilBossRespawn        = 10.0f;  
const float BossTorpedoSpeed               = MaxTorpedoSpeed;

const float BigEnemyTorpedoSpeed           = MaxTorpedoSpeed;
const float BigEnemyReloadTime             =   1.25f;
const float BigEnemySpeed                  = 200.0f;

const float SmallEnemyTorpedoSpeed         = BigEnemyTorpedoSpeed * 1.1f;
const float SmallEnemyReloadTime           = 1.0f;
const float SmallEnemySpeed                = 300.0f;

const float MaxTimeUntilNextScavenger      = 7.0f;
const float MaxScavengerSpeed              = 200.0f;

const float ShieldPowerupIncrease          = 20.0f; // Number of seconds of shield life given when shield powerup gained.
const int32 DoubleGunsPowerupIncrease      = 100;   // Number of double shots given when double guns powerup gained.
const float MaxInvincibilityTime           = 30.0f; // Number of seconds player ship is invincible after powerup gained.       
const float MaxInvincibilityWarnTime       = 0.5f;  // Number of seconds between succesive "low invincibility" on/off flashes.
const float WarnWhenInvincibilityGoingAway = 5.0f;  // Number of seconds of remaining invincibility to start warning player that it will end

const float ShieldBonkDamage               = 4.0f;  // Number of seconds to take from shield life when shields impact something.

const float PowerupOpacity                 = 0.5f;
const bool  PowerupsCanMove                = false;

const int32 PlayerShipNormalAtlasCel       = 0;
const int32 PlayerShipThrustingAtlasCel    = 1;
const int32 ShieldDefenseAtlasCel          = 0;
const int32 InvincibilityDefenseAtlasCel   = 1;

const float ExplosionInertialFactor        = 0.5f; // Original game had 0.0f, larger values make explosion particles retain target's inertia
const float BossShieldSpinSpeed            = 100.0f; // degrees/sec

const float PlayerShipSecondExplosionDelay = 0.66f;

const FDaylonParticlesParams IntroExplosionParams =
{
	4.5f,   // MinParticleSize
	9.0f,   // MaxParticleSize
	45.0f,  // MinParticleVelocity
	240.0f, // MaxParticleVelocity
	0.5f,   // MinParticleLifetime
	4.0f,   // MaxParticleLifetime
	0.25f,  // FinalOpacity
	80      // NumParticles
};


// Default explosion is used for asteroids and miniboss shield bonks.
// Mimics the original Asteroids arcade explosion; small unfading particles with a small dispersal radius.
const FDaylonParticlesParams DefaultExplosionParams = 
{	
	3.0f, 3.0f, 30.0f, 80.0f, 0.25f, 1.0f, 1.0f, 40 
};


const FDaylonParticlesParams EnemyShipExpolosionParams = 
{
     3.0f, 
     8.0f, 
    30.0f, 
   180.0f, 
     0.33f, 
     1.5f, 
     0.25f, 
    60,
};


const static FDaylonParticlesParams MiniBossExplosionParams = 
{
 	 3.0f, 
 	 8.0f, 
	30.0f, 
    180.0f, 
	0.33f, 
	1.5f, 
	0.25f, 
	60
};


const FDaylonParticlesParams PlayerShipFirstExplosionParams =
{
	3.0f,
	6.0f,
	30.0f,
	160.0f,
	0.5f,
	3.0f,
	0.25f,
	80
};

const FDaylonParticlesParams PlayerShipSecondExplosionParams =
{
	4.5f,
	9.0f,
	45.0f,
	240.0f,
	0.5f,
	4.0f,
	0.25f,
	80
};