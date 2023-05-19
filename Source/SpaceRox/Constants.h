// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


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
								           
const int32 ValueBigAsteroid               =    20;
const int32 ValueMediumAsteroid            =    50;
const int32 ValueSmallAsteroid             =   100;
const int32 ValueBigEnemy                  =   200;
const int32 ValueSmallEnemy                =  1000;
const int32 ValueScavenger                 =   500;
							           
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

const float MaxTimeUntilNextScavenger      = 7.0f;
const float MaxScavengerSpeed              = 200.0f;

const float ShieldPowerupIncrease          = 20.0f; // Number of seconds of shield life given when shield powerup gained.
const int32 DoubleGunsPowerupIncrease      = 100;   // Number of double shots given when double guns powerup gained.
const float MaxInvincibilityTime           = 30.0f; // Number of seconds player ship is invincible after powerup gained.       
const float MaxInvincibilityWarnTime       = 0.5f;  // Number of seconds between succesive "low invincibility" on/off flashes.

const float ShieldBonkDamage               = 4.0f;  // Number of seconds to take from shield life when shields impact something.

const float PowerupOpacity                 = 0.5f;
const bool  PowerupsCanMove                = false;

const int32 PlayerShipNormalAtlasCel       = 0;
const int32 PlayerShipThrustingAtlasCel    = 1;
const int32 ShieldDefenseAtlasCel          = 0;
const int32 InvincibilityDefenseAtlasCel   = 1;

