// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "Powerup.h"
#include "Constants.h"

#define FEATURE_ASTEROID_FADE_IN  0

class IArena;

class FAsteroid : public FPlayObject
{
	public:

		IArena* Arena = nullptr;
		float   Age = 0.0f;

		TSharedPtr<FPowerup> Powerup;

		bool                   HasPowerup  () const;
		TSharedPtr<FAsteroid>  Split       ();


		static TSharedPtr<FAsteroid> Spawn(IArena* InArena, const FDaylonSpriteAtlas& Atlas);
	


#if(FEATURE_ASTEROID_FADE_IN == 1)
		virtual void Update(float DeltaTime) override 
		{
			// Make asteroid fade in after being created.
			//FPlayObject::Update(DeltaTime); // don't animate any cels

			if(Age < 1.0f && Value == ValueBigAsteroid)
			{
				Age += DeltaTime;

				Age = FMath::Min(Age, 1.0f);

				SetRenderOpacity(Age);
			}
		}
#endif
};
