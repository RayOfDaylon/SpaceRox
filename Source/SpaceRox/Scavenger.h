// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "Powerup.h"


class UPlayViewBase;

/*
class FEnemyShip : public FPlayObject
{
	public:

	float TimeRemainingToNextShot = 0.0f;
	float TimeRemainingToNextMove = 0.0f;
	bool  bShootAtPlayer          = false;


	static TSharedPtr<FEnemyShip> Create(UDaylonSpriteWidgetAtlas* Atlas, int Value, float RadiusFactor);

	FEnemyShip();

	void Perform(UPlayViewBase& Arena, float DeltaTime);

	void Shoot(UPlayViewBase& Arena);
};
*/


class FScavenger : public FPlayObject
{
	public:

	TArray<TSharedPtr<FPowerup>> AcquiredPowerups;

	TWeakPtr<FPowerup> CurrentTarget;


	static TSharedPtr<FScavenger> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S);
};
