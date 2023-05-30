// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "SDaylonParticles.h"
#include "DaylonUtils.h"


class IArena; 

class FExplosion : public Daylon::PlayObject2D<SDaylonParticles>  
{
	public:

		static TSharedPtr<FExplosion> Create(
			FSlateBrush&     Brush,
			const FVector2D& P,
			float            MinParticleSize,
			float            MaxParticleSize,
			float            MinParticleVelocity,
			float            MaxParticleVelocity,
			float            MinParticleLifetime,
			float            MaxParticleLifetime,
			float            FinalOpacity,
			int32            NumParticles,
			const FVector2D& Inertia = FVector2D(0)
		);


		virtual FVector2D GetActualSize() const override { return FVector2D(4); }
};


struct FExplosions
{
	TArray<TSharedPtr<FExplosion>>   Explosions; 
	float                            InertialFactor = 1.0f;


	void SpawnOne(IArena& Arena, const FVector2D& P, const FVector2D& Inertia = FVector2D(0));

	void SpawnOne
	(
		IArena&          Arena,
		const FVector2D& P,
		float            MinParticleSize,
		float            MaxParticleSize,
		float            MinParticleVelocity,
		float            MaxParticleVelocity,
		float            MinParticleLifetime,
		float            MaxParticleLifetime,
		float            FinalOpacity,
		int32            NumParticles,
		const FVector2D& Inertia = FVector2D(0)
	);

	void Update    (IArena& Arena, const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime);

	void RemoveAll (IArena& Arena);
};

// ---------------------------------------------------------------------------------------------------------

class FShieldExplosion : public Daylon::PlayObject2D<SDaylonLineParticles>  
{
	public:

		static TSharedPtr<FShieldExplosion> Create(
			const FVector2D&                   P,
			const TArray<FDaylonLineParticle>& Particles,
			float                              ShieldThickness,
			float                              MinParticleVelocity,
			float                              MaxParticleVelocity,
			float                              MinParticleLifetime,
			float                              MaxParticleLifetime,
			float                              FinalOpacity,
			const FVector2D&                   Inertia = FVector2D(0)
		);


		virtual FVector2D GetActualSize() const override { return FVector2D(4); }
};


struct FShieldExplosions
{
	TArray<TSharedPtr<FShieldExplosion>>   Explosions; 
	float                                  InertialFactor = 1.0f;


	void SpawnOne
	(
		IArena&                            Arena,
		const FVector2D&                   P,
		const TArray<FDaylonLineParticle>& Particles,
		float                              ShieldThickness,
		float                              MinParticleVelocity,
		float                              MaxParticleVelocity,
		float                              MinParticleLifetime,
		float                              MaxParticleLifetime,
		float                              FinalOpacity,
		const FVector2D&                   Inertia = FVector2D(0)
	);

	void Update    (IArena& Arena, const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime);

	void RemoveAll (IArena& Arena);
};
