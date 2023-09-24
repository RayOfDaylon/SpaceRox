// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "Asteroid.h"
#include "Constants.h"


class UPlayViewBase;
class IArena;


class FAsteroids
{
	public:

		IArena* Arena = nullptr;

		TArray<TSharedPtr<FAsteroid>>     Asteroids;

		FAsteroids()
		{
			Asteroids.Reserve(MaxInitialAsteroids * 4);
		}

		void              Add        (TSharedPtr<FAsteroid> AsteroidPtr) { Asteroids.Add(AsteroidPtr); }
		bool              IsEmpty    () const { return Asteroids.IsEmpty(); }
		int32             Num        () const { return Asteroids.Num(); }

		FAsteroid&        Get        (int32 Index) { return *Asteroids[Index].Get(); }
		const FAsteroid&  Get        (int32 Index) const { return *Asteroids[Index].Get(); }

		void              Remove     (int32 Index);
		void              RemoveAll  ();

		void              Update     (float DeltaTime);
		void              Kill       (int32 Index, bool KilledByPlayer);
};
