// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"


class UPlayViewBase;

enum class EPowerup : uint8
{
	Nothing     = 0,
	DoubleGuns,
	Shields,
	Invincibility
};


class FPowerup : public FPlayObject
{
	public:

	EPowerup Kind = EPowerup::Nothing;

	static TSharedPtr<FPowerup> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S);

	virtual void Update(float DeltaTime) override { FPlayObject::Update(DeltaTime); }
};


