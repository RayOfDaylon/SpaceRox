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


	static TSharedPtr<FScavenger> Create(const FDaylonSpriteAtlas& Atlas, const FVector2D& S);
};
