// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"


class UPlayViewBase;


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



class FEnemyBoss : public Daylon::PlayObject2D<SOverlay>
{
	virtual FVector2D GetActualSize() const override { return FVector2D(100); }

	public:

	int32                                 NumShields = 0;
	TSharedPtr<SDaylonSprite>             Sprite;
	FOverlaySlot*                         SpriteSlot;

	TArray<TSharedPtr<SDaylonPolyShield>> Shields;
	TArray<FOverlaySlot*>                 ShieldSlots;

	static TSharedPtr<FEnemyBoss> Create(UDaylonSpriteWidgetAtlas* Atlas, float S, int32 Value, int32 NumShields, float SpinSpeed = 100.0f);

	void  Update                   (float DeltaTime);
	int32 CheckCollision           (const FVector2D& P1, const FVector2D &P2, int32& ShieldSegmentIndex) const;
	float GetShieldSegmentHealth   (int32 ShieldNumber, int32 SegmentIndex) const;
	void  SetShieldSegmentHealth   (int32 ShieldNumber, int32 SegmentIndex, float Health);
	void  SpawnExplosion           (UPlayViewBase& Arena);
	void  Perform                  (UPlayViewBase& Arena, float DeltaTime);
};
