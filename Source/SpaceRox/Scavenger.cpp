// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "Scavenger.h"
#include "Constants.h"


TSharedPtr<FScavenger> FScavenger::Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S)
{
	auto Widget = SNew(FScavenger);

	Daylon::FinishCreating<SDaylonSprite>(Widget, 0.5f);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetSize(S);
	Widget->UpdateWidgetSize();

	Widget->Value = ValueScavenger;

	return Widget;
}
