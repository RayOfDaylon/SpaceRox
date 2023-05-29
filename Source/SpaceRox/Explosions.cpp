#include "Explosions.h"
#include "PlayViewBase.h"


TSharedPtr<FExplosion> FExplosion::Create
(
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
	const FVector2D& Inertia
)
{
	auto Widget = SNew(FExplosion)
		.MinParticleSize     ( MinParticleSize)
		.MaxParticleSize     ( MaxParticleSize)
		.MinParticleVelocity ( MinParticleVelocity)
		.MaxParticleVelocity ( MaxParticleVelocity)
		.MinParticleLifetime ( MinParticleLifetime)
		.MaxParticleLifetime ( MaxParticleLifetime)
		.FinalOpacity        ( FinalOpacity)
		.NumParticles(NumParticles);

	Daylon::Install<SDaylonParticles>(Widget, 0.5f);

	Widget->Inertia = Inertia;
	Widget->SetVisibility(EVisibility::HitTestInvisible);
	Widget->SetRenderTransformPivot(FVector2D(0.5f));
	Widget->SetPosition(P);
	Widget->UpdateWidgetSize();
	Widget->SetParticleBrush(Brush);

	return Widget;
}


void FExplosions::SpawnOne
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
	const FVector2D& Inertia
)
{
	auto ExplosionPtr = FExplosion::Create(
		Arena.TorpedoBrush,
		P,
		MinParticleSize,
		MaxParticleSize,
		MinParticleVelocity,
		MaxParticleVelocity,
		MinParticleLifetime,
		MaxParticleLifetime,
		FinalOpacity,
		NumParticles,
		Inertia * InertialFactor
	);

	Explosions.Add(ExplosionPtr);
}


void FExplosions::Update(UPlayViewBase& Arena, const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime)
{
	for(int32 Index = Explosions.Num() - 1; Index >= 0; Index--)
	{
		auto ExplosionPtr = Explosions[Index];

		ExplosionPtr->Move(DeltaTime, WrapFunction);

		auto Widget = StaticCastSharedPtr<SDaylonParticles>(ExplosionPtr);
		
		if(!Widget->Update(DeltaTime))
		{
			Arena.RootCanvas->GetCanvasWidget()->RemoveSlot(ExplosionPtr.ToSharedRef());
			Explosions.RemoveAtSwap(Index);
		}
	}
}


void FExplosions::RemoveAll(UPlayViewBase& Arena)
{
	while(!Explosions.IsEmpty())
	{
		Arena.RootCanvas->GetCanvasWidget()->RemoveSlot(Explosions.Last(0).ToSharedRef());
		Explosions.Pop();
	}
}

// ------------------------------------------------------------------------------------------------------------------

TSharedPtr<FShieldExplosion> FShieldExplosion::Create
(
	const FVector2D&                   P,
	const TArray<FDaylonLineParticle>& Particles,
	float                              ShieldThickness,
	float                              MinParticleVelocity,
	float                              MaxParticleVelocity,
	float                              MinParticleLifetime,
	float                              MaxParticleLifetime,
	float                              FinalOpacity,
	const FVector2D&                   Inertia
)
{
	auto Widget = SNew(FShieldExplosion)
		.MinParticleVelocity (MinParticleVelocity)
		.MaxParticleVelocity (MaxParticleVelocity)
		.MinParticleLifetime (MinParticleLifetime)
		.MaxParticleLifetime (MaxParticleLifetime)
		.LineThickness       (ShieldThickness)
		.FinalOpacity        (FinalOpacity);

	Daylon::Install<SDaylonLineParticles>(Widget, 0.5f);

	Widget->Inertia = Inertia;
	Widget->SetVisibility(EVisibility::HitTestInvisible);
	Widget->SetRenderTransformPivot(FVector2D(0.5f));
	Widget->SetPosition(P);
	Widget->UpdateWidgetSize();
	Widget->SetParticles(Particles);

	return Widget;
}


void FShieldExplosions::SpawnOne
(
	UPlayViewBase&                     Arena,
	const FVector2D&                   P,
	const TArray<FDaylonLineParticle>& Particles,
	float                              ShieldThickness,
	float                              MinParticleVelocity,
	float                              MaxParticleVelocity,
	float                              MinParticleLifetime,
	float                              MaxParticleLifetime,
	float                              FinalOpacity,
	const FVector2D&                   Inertia
)
{
	auto ExplosionPtr = FShieldExplosion::Create(
		P,
		Particles,
		ShieldThickness,
		MinParticleVelocity,
		MaxParticleVelocity,
		MinParticleLifetime,
		MaxParticleLifetime,
		FinalOpacity,
		Inertia * InertialFactor
	);

	Explosions.Add(ExplosionPtr);
}


void FShieldExplosions::Update(UPlayViewBase& Arena, const TFunction<FVector2D(const FVector2D&)>& WrapFunction, float DeltaTime)
{
	for(int32 Index = Explosions.Num() - 1; Index >= 0; Index--)
	{
		auto ExplosionPtr = Explosions[Index];

		ExplosionPtr->Move(DeltaTime, WrapFunction);

		auto Widget = StaticCastSharedPtr<SDaylonLineParticles>(ExplosionPtr);
		
		if(!Widget->Update(DeltaTime))
		{
			Arena.RootCanvas->GetCanvasWidget()->RemoveSlot(ExplosionPtr.ToSharedRef());
			Explosions.RemoveAtSwap(Index);
		}
	}
}


void FShieldExplosions::RemoveAll(UPlayViewBase& Arena)
{
	while(!Explosions.IsEmpty())
	{
		Daylon::UninstallImpl(Explosions.Last(0));
		//Arena.RootCanvas->GetCanvasWidget()->RemoveSlot(Explosions.Last(0).ToSharedRef());
		Explosions.Pop();
	}
}
