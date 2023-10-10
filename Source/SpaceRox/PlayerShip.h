// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"
#include "DaylonBindableValue.h"
#include "DaylonPlayObject2D.h"


class IArena;


class FPlayerShip : public FPlayObject
{
	public:

	IArena* Arena = nullptr;

	bool                                    IsUnderThrust;
	bool                                    IsSpawning;
	float                                   TimeUntilNextInvincibilityWarnFlash;

	Daylon::TBindableValue<int32>           DoubleShotsLeft;
	Daylon::TBindableValue<float>           ShieldsLeft;
	Daylon::TBindableValue<float>           InvincibilityLeft;

	TSharedPtr<Daylon::SpritePlayObject2D>  Shield;
	TSharedPtr<Daylon::SpritePlayObject2D>  InvincibilityShield;


	static TSharedPtr<FPlayerShip> Create(const FDaylonSpriteAtlas& Atlas, const FVector2D& S, float RadiusFactor);

	void Initialize              (IArena*);
	void AdjustDoubleShotsLeft   (int32 Amount);
	void AdjustShieldsLeft       (float Amount);
	void AdjustInvincibilityLeft (float Amount);
	void ReleaseResources        ();
	void InitializeDefenses      ();
	bool ProcessCollision        (float MassOther = 0.0f, const FVector2D* PositionOther = nullptr, const FVector2D* InertiaOther = nullptr);
	void SpawnExplosion          ();
	void FireTorpedo             ();
	void Perform                 (float DeltaTime);
};

