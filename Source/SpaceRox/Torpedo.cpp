// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "Torpedo.h"


TSharedPtr<FTorpedo> FTorpedo::Create(UDaylonSpriteWidgetAtlas* Atlas, float RadiusFactor)
{
	auto Widget = SNew(FTorpedo);

	Daylon::Install<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetCurrentCel(0);
	Widget->SetSize(Atlas->Atlas.GetCelPixelSize());
	Widget->UpdateWidgetSize();

	return Widget;
}
