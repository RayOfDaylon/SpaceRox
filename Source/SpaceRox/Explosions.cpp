#include "Explosions.h"
#include "Arena.h"


TSharedPtr<FExplosion> FExplosion::Create
(
	FSlateBrush&                  Brush,
	const FVector2D&              P,
	const FDaylonParticlesParams& Params,
	const FVector2D&              Inertia
)
{
	auto Widget = SNew(FExplosion)
		.MinParticleSize     (Params.MinParticleSize)
		.MaxParticleSize     (Params.MaxParticleSize)
		.MinParticleVelocity (Params.MinParticleVelocity)
		.MaxParticleVelocity (Params.MaxParticleVelocity)
		.MinParticleLifetime (Params.MinParticleLifetime)
		.MaxParticleLifetime (Params.MaxParticleLifetime)
		.FinalOpacity        (Params.FinalOpacity)
		.NumParticles        (Params.NumParticles);

	Daylon::Install<SDaylonParticles>(Widget, 0.5f);

	Widget->Inertia = Inertia;
	UDaylonUtils::Show(&Widget.Get());
	Widget->SetRenderTransformPivot(FVector2D(0.5f));
	Widget->SetPosition(P);
	Widget->UpdateWidgetSize();
	Widget->SetParticleBrush(Brush);

	return Widget;
}


void FExplosions::SpawnOne(const FVector2D& P, const FVector2D& Inertia)
{
	// Spawn a default explosion.

	SpawnOne(P, DefaultExplosionParams, Inertia);
}


void FExplosions::SpawnOne(const FVector2D& P, const FDaylonParticlesParams& Params, const FVector2D& Inertia)
{
	auto ExplosionPtr = FExplosion::Create(Arena->GetExplosionParticleBrush(), P, Params, Inertia * InertialFactor);

	Explosions.Add(ExplosionPtr);
}


void FExplosions::Update(const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime)
{
	for(int32 Index = Explosions.Num() - 1; Index >= 0; Index--)
	{
		auto ExplosionPtr = Explosions[Index];

		ExplosionPtr->Move(DeltaTime, WrapFunction);

		auto Widget = StaticCastSharedPtr<SDaylonParticles>(ExplosionPtr);
		
		if(!Widget->Update(DeltaTime))
		{
			Daylon::UninstallImpl(ExplosionPtr);
			Explosions.RemoveAtSwap(Index);
		}
	}
}


void FExplosions::RemoveAll()
{
	while(!Explosions.IsEmpty())
	{
		Daylon::UninstallImpl(Explosions.Pop());
	}
}

// ------------------------------------------------------------------------------------------------------------------

TSharedPtr<FShieldExplosion> FShieldExplosion::Create
(
	const FVector2D&                   P,
	const FDaylonLineParticlesParams&  Params,
	const FVector2D&                   Inertia
)
{
	auto Widget = SNew(FShieldExplosion);

	Daylon::Install<SDaylonLineParticles>(Widget, 0.5f);

	Widget->Inertia = Inertia;
	UDaylonUtils::Show(&Widget.Get());
	Widget->SetRenderTransformPivot(FVector2D(0.5f));
	Widget->SetPosition(P);
	Widget->UpdateWidgetSize();
	Widget->Set(Params);

	return Widget;
}


void FShieldExplosions::SpawnOne
(
	const FVector2D&                   P,
	const FDaylonLineParticlesParams&  Params,
	const FVector2D&                   Inertia
)
{
	auto ExplosionPtr = FShieldExplosion::Create(P, Params, Inertia * InertialFactor);

	Explosions.Add(ExplosionPtr);
}


void FShieldExplosions::Update(const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime)
{
	for(int32 Index = Explosions.Num() - 1; Index >= 0; Index--)
	{
		auto ExplosionPtr = Explosions[Index];

		ExplosionPtr->Move(DeltaTime, WrapFunction);

		auto Widget = StaticCastSharedPtr<SDaylonLineParticles>(ExplosionPtr);
		
		if(!Widget->Update(DeltaTime))
		{
			Daylon::UninstallImpl(ExplosionPtr);
			Explosions.RemoveAtSwap(Index);
		}
	}
}


void FShieldExplosions::RemoveAll()
{
	while(!Explosions.IsEmpty())
	{
		Daylon::UninstallImpl(Explosions.Last(0));
		Explosions.Pop();
	}
}
