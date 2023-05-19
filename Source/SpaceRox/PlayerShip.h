// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"


class UPlayViewBase;


class FPlayerShip : public FPlayObject
{
	public:

	bool   IsUnderThrust;
	bool   IsSpawning;
	int32  DoubleShotsLeft;
	float  ShieldsLeft;
	float  InvincibilityLeft;
	float  TimeUntilNextInvincibilityWarnFlash;


	TSharedPtr<Daylon::SpritePlayObject2D>     Shield;
	TSharedPtr<Daylon::SpritePlayObject2D>     InvincibilityShield;


	static TSharedPtr<FPlayerShip> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor);

	void ReleaseResources   (UPlayViewBase& Arena);

	void Initialize         (UPlayViewBase& Arena);
	void InitializeDefenses (UPlayViewBase& Arena);

	void Perform            (UPlayViewBase& Arena, float DeltaTime);
	bool ProcessCollision   (UPlayViewBase& Arena);
	void AdjustShieldsLeft  (UPlayViewBase& Arena, float Amount);

	void SpawnExplosion     (UPlayViewBase& Arena);
};

