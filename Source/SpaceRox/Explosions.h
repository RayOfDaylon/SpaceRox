// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "SDaylonParticles.h"
#include "SDaylonLineParticles.h"
#include "DaylonPlayObject2D.h"


class IArena; 

class FExplosion : public Daylon::PlayObject2D<SDaylonParticles>  
{
	public:

		static TSharedPtr<FExplosion>  Create  (FSlateBrush& Brush, const FVector2D& P, const FDaylonParticlesParams& Params, const FVector2D& Inertia = FVector2D(0));

		virtual FVector2D  GetActualSize  () const override { return FVector2D(4); }
};


struct FExplosions
{
	IArena* Arena = nullptr;

	TArray<TSharedPtr<FExplosion>>   Explosions; 
	float                            InertialFactor = 1.0f;


	void  SpawnOne   (const FVector2D& P, const FVector2D& Inertia = FVector2D(0));
	void  SpawnOne   (const FVector2D& P, const FDaylonParticlesParams& Params, const FVector2D& Inertia = FVector2D(0));
	void  Update     (const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime);
	void  RemoveAll  ();
};

// ---------------------------------------------------------------------------------------------------------

class FShieldExplosion : public Daylon::PlayObject2D<SDaylonLineParticles>  
{
	public:

		static TSharedPtr<FShieldExplosion> Create(
			const FVector2D&                   P,
			const FDaylonLineParticlesParams&  Params,
			const FVector2D&                   Inertia = FVector2D(0)
		);


		virtual FVector2D  GetActualSize  () const override { return FVector2D(4); }
};


struct FShieldExplosions
{
	IArena* Arena = nullptr;

	TArray<TSharedPtr<FShieldExplosion>>   Explosions; 
	float                                  InertialFactor = 1.0f;


	void SpawnOne
	(
		const FVector2D&                   P,
		const FDaylonLineParticlesParams&  Params,
		const FVector2D&                   Inertia = FVector2D(0)
	);

	void  Update     (const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime);
	void  RemoveAll  ();
};
