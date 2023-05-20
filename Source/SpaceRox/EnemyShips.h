// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "EnemyShip.h"
#include "Scavenger.h"


class UPlayViewBase;

struct FEnemyShips
{
	TArray<TSharedPtr<FEnemyShip>> Ships;
	TArray<TSharedPtr<FScavenger>> Scavengers;

	int32  NumSmallEnemyShips = 0;
	int32  NumBigEnemyShips = 0;

	int32 NumShips() const { return Ships.Num(); }
	int32 NumScavengers() const { return Scavengers.Num(); }

	TSharedPtr<FEnemyShip> GetShipPtr(int32 Index) { return Ships[Index]; }
	FEnemyShip&            GetShip(int32 Index) { return *GetShipPtr(Index).Get(); }

	TSharedPtr<FScavenger> GetScavengerPtr(int32 Index) { return Scavengers[Index]; }
	FScavenger&            GetScavenger(int32 Index) { return *GetScavengerPtr(Index).Get(); }

	void RemoveShip      (int32 Index);
	void RemoveScavenger (int32 Index);
	void RemoveAll       ();

	void KillShip        (UPlayViewBase& Arena, int32 Index);
	void KillScavenger   (UPlayViewBase& Arena, int32 Index);

	void SpawnShip       (UPlayViewBase& Arena);
	void Update          (UPlayViewBase& Arena, float DeltaTime);
};
