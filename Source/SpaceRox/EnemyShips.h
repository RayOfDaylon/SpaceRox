// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "EnemyShip.h"
#include "Scavenger.h"


class IArena;

struct FEnemyShips
{
	IArena* Arena = nullptr;

	TArray<TSharedPtr<FEnemyShip>> Ships;
	TArray<TSharedPtr<FEnemyBoss>> Bosses;
	TArray<TSharedPtr<FScavenger>> Scavengers;

	int32  NumSmallEnemyShips = 0;
	int32  NumBigEnemyShips   = 0;

	int32 NumShips      () const { return Ships.Num(); }
	int32 NumBosses     () const { return Bosses.Num(); }
	int32 NumScavengers () const { return Scavengers.Num(); }

	TSharedPtr<FEnemyShip> GetShipPtr      (int32 Index) { return Ships[Index]; }
	FEnemyShip&            GetShip         (int32 Index) { return *GetShipPtr(Index).Get(); }
									       
	TSharedPtr<FEnemyBoss> GetBossPtr      (int32 Index) { return Bosses[Index]; }
	FEnemyBoss&            GetBoss         (int32 Index) { return *GetBossPtr(Index).Get(); }

	TSharedPtr<FScavenger> GetScavengerPtr (int32 Index) { return Scavengers[Index]; }
	FScavenger&            GetScavenger    (int32 Index) { return *GetScavengerPtr(Index).Get(); }

	void RemoveShip      (int32 Index);
	void RemoveBoss      (int32 Index);
	void RemoveScavenger (int32 Index);
	void RemoveAll       ();

	void KillShip        (int32 Index);
	void KillBoss        (int32 Index);
	void KillScavenger   (int32 Index);

	void SpawnShip       ();
	void SpawnBoss	     ();
	void Update          (float DeltaTime);
};


