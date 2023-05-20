// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "DaylonUtils.h"


class UPlayViewBase;


class FPlayerShip : public FPlayObject
{
	public:

	bool   IsUnderThrust;
	bool   IsSpawning;

	Daylon::TBindableValue<int32> DoubleShotsLeft;
	Daylon::TBindableValue<float> ShieldsLeft;
	Daylon::TBindableValue<float> InvincibilityLeft;

	float  TimeUntilNextInvincibilityWarnFlash;


	TSharedPtr<Daylon::SpritePlayObject2D>     Shield;
	TSharedPtr<Daylon::SpritePlayObject2D>     InvincibilityShield;


	static TSharedPtr<FPlayerShip> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor);

	void AdjustDoubleShotsLeft   (int32 Amount);
	void AdjustShieldsLeft       (float Amount);
	void AdjustInvincibilityLeft (float Amount);

	void ReleaseResources   (UPlayViewBase& Arena);

	void Initialize         (UPlayViewBase& Arena);
	void InitializeDefenses (UPlayViewBase& Arena);

	void Perform            (UPlayViewBase& Arena, float DeltaTime);
	bool ProcessCollision   (UPlayViewBase& Arena);

	void SpawnExplosion     (UPlayViewBase& Arena);

	void FireTorpedo        (UPlayViewBase& Arena);
};

