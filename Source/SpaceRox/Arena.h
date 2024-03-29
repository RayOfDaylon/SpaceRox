// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

// Abstract play arena interface, mix-in for UPlayViewBase.
// Keeps other classes from having to know UPlayViewBase directly.


#pragma once

#include "CoreMinimal.h"
#include "UDaylonSpriteWidget.h"
#include "Explosions.h"
#include "Powerup.h"
#include "Asteroids.h"



class FTorpedo;
class FPlayerShip;
struct FDaylonParticlesParams;

namespace Daylon { struct FScheduledTask; struct FLoopedSound; }


class IArena
{
	public:

	    virtual TFunction<FVector2D(const FVector2D&)> GetWrapPositionFunction() const = 0;

		virtual FVector2D                     WrapPosition                 (const FVector2D& P) = 0;
																		   
		virtual void                          AddScheduledTask             (Daylon::FScheduledTask&) = 0;
		virtual void                          ScheduleExplosion            (float When, const FVector2D& P, const FVector2D& Inertia, const FDaylonParticlesParams& Params) = 0;
																		   
		virtual Daylon::FLoopedSound&         GetBigEnemySoundLoop         () = 0;
		virtual Daylon::FLoopedSound&         GetSmallEnemySoundLoop       () = 0;
										      							   
		virtual bool                          CanExplosionOccur            () const = 0;
																		   
		virtual const FDaylonSpriteAtlas&     GetBigEnemyAtlas             () const = 0;
		virtual const FDaylonSpriteAtlas&     GetSmallEnemyAtlas           () const = 0;
		virtual const FDaylonSpriteAtlas&     GetMiniboss1Atlas            () const = 0;
		virtual const FDaylonSpriteAtlas&     GetMiniboss2Atlas            () const = 0;
		virtual const FDaylonSpriteAtlas&     GetScavengerAtlas            () const = 0;
		virtual const FDaylonSpriteAtlas&     GetMediumAsteroidAtlas       () const = 0;
		virtual const FDaylonSpriteAtlas&     GetSmallAsteroidAtlas        () const = 0;
		virtual const FDaylonSpriteAtlas&     GetDefensesAtlas             () const = 0;
																		   
		virtual FAsteroids&                   GetAsteroids                 () = 0;
		virtual FExplosions&                  GetExplosions                () = 0;
		virtual FShieldExplosions&            GetShieldExplosions          () = 0;
		virtual TArray<TSharedPtr<FPowerup>>& GetPowerups                  () = 0;
		virtual FSlateBrush&                  GetExplosionParticleBrush    () = 0;
		virtual USoundBase*                   GetExplosionSound            (int32 Index) = 0;
		virtual USoundBase*                   GetTorpedoSound              () = 0;
		virtual USoundBase*                   GetShieldBonkSound           () = 0;
																		   
		virtual TSharedPtr<FTorpedo>          GetAvailableTorpedo          () = 0;
																		   
		virtual void                          PlaySound                    (USoundBase* Sound, float VolumeScale = 1.0f) = 0;

		virtual bool                          IsPlayerShipPresent          () const = 0;
		virtual FPlayerShip&                  GetPlayerShip                () = 0;
		virtual int32                         GetPlayerScore               () const = 0;
		virtual float                         GetPlayerRotationForce       () const = 0;
		virtual bool                          IsPlayerThrustActive         () const = 0;
		virtual bool                          IsPlayerShieldActive         () const = 0;
		virtual bool                          IsGodModeActive              () const = 0;
		virtual Daylon::FLoopedSound&         GetPlayerShipThrustSoundLoop () = 0;
		virtual void                          IncreasePlayerScoreBy        (int32 Amount) = 0;

		virtual float                         AdjustTimeUntilNextEnemyShip (float Amount) = 0;
		virtual float                         GetTimeUntilNextEnemyShip    () const = 0;
		virtual void                          SetTimeUntilNextEnemyShip    (float Value) = 0;

		virtual float                         AdjustTimeUntilNextScavenger (float Amount) = 0;
		virtual float                         GetTimeUntilNextScavenger    () const = 0;
		virtual void                          SetTimeUntilNextScavenger    (float Value) = 0;

		virtual float                         AdjustTimeUntilNextBoss      (float Amount) = 0;
		virtual float                         GetTimeUntilNextBoss         () const = 0;
		virtual void                          SetTimeUntilNextBoss         (float Value) = 0;
};
