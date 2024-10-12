// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "DaylonRNG.h"


class UPlayViewBase;

enum class EPowerup : uint8
{
	Nothing     = 0,
	DoubleGuns,
	Shields,
	Invincibility,
	LAST = Invincibility
};


class FPowerup : public FPlayObject
{
	public:

		EPowerup Kind = EPowerup::Nothing;

		static TSharedPtr<FPowerup>  Create  (const FDaylonSpriteAtlas& Atlas, const FVector2D& S);

		virtual void  Update  (float DeltaTime) override { FPlayObject::Update(DeltaTime); }
};


class FPowerupFactory
{
	Daylon::MTRand Rng;
	TArray<int32>  MinXpsForPowerup;

	public:

		FPowerupFactory() 
		{
			Rng.seed(0); 
			MinXpsForPowerup.Init(0, (int32)EPowerup::LAST);
		}


		void SetMinXpFor(EPowerup Kind, int32 MinXp)
		{
			const auto Idx = (int32)Kind;

			check(Idx >= 1 && Idx <= (int32)EPowerup::LAST);

			MinXpsForPowerup[Idx - 1] = MinXp;
		}


		EPowerup Produce(int32 Xp)
		{
			// If the given Xp doesn't qualify for any powerup, return nothing.

			bool CanProduce = false;

			for(auto MinXp : MinXpsForPowerup)
			{
				if(Xp >= MinXp)
				{
					CanProduce = true;
					break;
				}
			}

			if(!CanProduce)
			{
				return EPowerup::Nothing;
			}

			// Randomly produce a powerup.

			for(;;)
			{
				const auto Kind = (EPowerup)Daylon::RandRange(Rng, 1, (int32)EPowerup::LAST);

				if(Xp >= MinXpsForPowerup[(int32)Kind - 1])
				{
					return Kind;
				}
			}

			return EPowerup::Nothing; // Note: we never reach here
		}

};
