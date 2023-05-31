// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "Powerup.h"


class IArena;

class FAsteroid : public FPlayObject
{
	public:

		IArena* Arena = nullptr;

		TSharedPtr<FPowerup> Powerup;

		bool HasPowerup() const;

		static TSharedPtr<FAsteroid> Create(IArena* InArena, const FDaylonSpriteAtlas& Atlas);
	
		TSharedPtr<FAsteroid> Split();
};
