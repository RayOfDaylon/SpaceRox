// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "DaylonUtils.h"


class IArena;


class FPlayerShip : public FPlayObject
{
	public:

	IArena* Arena = nullptr;

	bool   IsUnderThrust;
	bool   IsSpawning;

	Daylon::TBindableValue<int32> DoubleShotsLeft;
	Daylon::TBindableValue<float> ShieldsLeft;
	Daylon::TBindableValue<float> InvincibilityLeft;

	float  TimeUntilNextInvincibilityWarnFlash;


	TSharedPtr<Daylon::SpritePlayObject2D>     Shield;
	TSharedPtr<Daylon::SpritePlayObject2D>     InvincibilityShield;


	static TSharedPtr<FPlayerShip> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor);

	void Initialize              (IArena*);
	void AdjustDoubleShotsLeft   (int32 Amount);
	void AdjustShieldsLeft       (float Amount);
	void AdjustInvincibilityLeft (float Amount);
	void ReleaseResources        ();
	void InitializeDefenses      ();
	bool ProcessCollision        ();
	void SpawnExplosion          ();
	void FireTorpedo             ();
	void Perform                 (float DeltaTime);
};

