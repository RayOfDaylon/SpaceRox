// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "PlayObject.h"


class IArena;


class FEnemyShip : public FPlayObject
{
	public:

	IArena* Arena = nullptr;

	float TimeRemainingToNextShot = 0.0f;
	float TimeRemainingToNextMove = 0.0f;
	bool  bShootAtPlayer          = false;


	static TSharedPtr<FEnemyShip> Spawn(IArena* InArena, const FDaylonSpriteAtlas& Atlas, int Value, float RadiusFactor);

	FEnemyShip();

	void  Perform  (float DeltaTime);
	void  Shoot    ();
};



class FEnemyBoss : public Daylon::PlayObject2D<SOverlay>
{
	virtual FVector2D GetActualSize() const override { return FVector2D(100); }

	public:

	IArena* Arena = nullptr;

	float                                 TimeRemainingToNextShot = 0.0f;
	float                                 TimeRemainingToNextMove = 0.0f;
	int32                                 NumShields = 0;
	TSharedPtr<SDaylonSprite>             Sprite;
	FOverlaySlot*                         SpriteSlot;

	TArray<TSharedPtr<SDaylonPolyShield>> Shields;
	TArray<FOverlaySlot*>                 ShieldSlots;

	static TSharedPtr<FEnemyBoss> Spawn(IArena* InArena, const FDaylonSpriteAtlas& Atlas, float S, int32 Value, int32 NumShields, float SpinSpeed = 100.0f);

	void   Update                    (float DeltaTime);
	int32  CheckCollision            (const FVector2D& P1, const FVector2D &P2, int32& ShieldSegmentIndex) const;
	int32  CheckCollision            (const FVector2D& P1, const FVector2D &P2, float Radius, int32& ShieldSegmentIndex, FVector2D& HitPt) const;
	float  GetShieldSegmentHealth    (int32 ShieldNumber, int32 SegmentIndex) const;
	void   SetShieldSegmentHealth    (int32 ShieldNumber, int32 SegmentIndex, float Health);
	void   GetShieldSegmentGeometry  (int32 ShieldNumber, int32 SegmentIndex, FVector2D& P1, FVector2D& P2) const;
	float  GetShieldThickness        () const;
	void   Perform                   (float DeltaTime);
	void   Shoot                     ();
};
