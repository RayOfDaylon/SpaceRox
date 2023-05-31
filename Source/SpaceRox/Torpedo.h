// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"


class UPlayViewBase;


class FTorpedo : public FPlayObject
{
	public:

	bool FiredByPlayer;

	static TSharedPtr<FTorpedo> Create(const FDaylonSpriteAtlas& Atlas, float RadiusFactor);
};

