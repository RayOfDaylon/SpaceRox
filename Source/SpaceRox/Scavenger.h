// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "Powerup.h"


class UPlayViewBase;


class FScavenger : public FPlayObject
{
	public:

		TArray<TSharedPtr<FPowerup>> AcquiredPowerups;

		TWeakPtr<FPowerup> CurrentTarget;

		int XDirection = 1; // -1 to travel from right to left


		static TSharedPtr<FScavenger> Create(const FDaylonSpriteAtlas& Atlas, const FVector2D& S);
};
