// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once


#include "CoreMinimal.h"
#include "SDaylonParticles.h"
#include "DaylonUtils.h"


class UPlayViewBase;


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


	void SpawnOne
	(
		UPlayViewBase&   Arena,
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

	void Update    (UPlayViewBase& Arena, const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime);

	void RemoveAll (UPlayViewBase& Arena);
};
