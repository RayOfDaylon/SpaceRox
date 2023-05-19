// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "Powerup.h"


class UPlayViewBase;


class FAsteroid : public FPlayObject
{
	public:

		TSharedPtr<FPowerup> Powerup;

		bool HasPowerup() const;

		static TSharedPtr<FAsteroid> Create(UDaylonSpriteWidgetAtlas* Atlas);
	
		TSharedPtr<FAsteroid> Split(UPlayViewBase& Arena);
};
