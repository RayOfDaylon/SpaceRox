// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "Powerup.h"


TSharedPtr<FPowerup> FPowerup::Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S)
{
	auto Widget = SNew(FPowerup);

	Daylon::Install<SDaylonSprite>(Widget, 0.5f);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetSize(S);
	Widget->UpdateWidgetSize();

	return Widget;
}
