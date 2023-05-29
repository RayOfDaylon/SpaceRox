// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "SDaylonParticles.h"
#include "DaylonUtils.h"

#define DEBUG_MODULE      1

#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif


void SDaylonParticles::Construct(const FArguments& InArgs)
{
	Size = InArgs._Size.Get();
	MinParticleSize     = InArgs._MinParticleSize.Get();
	MaxParticleSize     = InArgs._MaxParticleSize.Get();
	MinParticleVelocity = InArgs._MinParticleVelocity.Get();
	MaxParticleVelocity = InArgs._MaxParticleVelocity.Get();
	MinParticleLifetime = InArgs._MinParticleLifetime.Get();
	MaxParticleLifetime = InArgs._MaxParticleLifetime.Get();
	FinalOpacity        = InArgs._FinalOpacity.Get();

	SetNumParticles(InArgs._NumParticles.Get());
}


FVector2D SDaylonParticles::ComputeDesiredSize(float) const 
{
	return Size; 
}


void SDaylonParticles::SetSize                  (const FVector2D& InSize)    { Size = InSize; }
void SDaylonParticles::SetParticleBrush         (const FSlateBrush& InBrush) { ParticleBrush = InBrush; }
void SDaylonParticles::SetMinParticleSize       (float InSize)               { MinParticleSize = InSize; }
void SDaylonParticles::SetMaxParticleSize       (float InSize)               { MaxParticleSize = InSize; }
void SDaylonParticles::SetMinParticleVelocity   (float Velocity)             { MinParticleVelocity = Velocity; }
void SDaylonParticles::SetMaxParticleVelocity   (float Velocity)             { MaxParticleVelocity = Velocity; }
void SDaylonParticles::SetMinParticleLifetime   (float Lifetime)             { MinParticleLifetime = Lifetime; }
void SDaylonParticles::SetMaxParticleLifetime   (float Lifetime)             { MaxParticleLifetime = Lifetime; }
void SDaylonParticles::SetFinalOpacity          (float Opacity)              { FinalOpacity = Opacity; }


void SDaylonParticles::SetNumParticles          (int32 Count)             
{
	check(Count > 0);
	Particles.SetNum(Count);
	Reset();
}


void SDaylonParticles::Reset()
{
	for(auto& Particle : Particles)
	{
		Particle.P             = FVector2D(0); // todo: could randomize this a small distance for more realism
		Particle.Size          = FMath::RandRange(MinParticleSize, MaxParticleSize);
		Particle.Inertia       = UDaylonUtils::RandVector2D() * FMath::RandRange(MinParticleVelocity, MaxParticleVelocity);
		Particle.LifeRemaining = Particle.StartingLifeRemaining = FMath::RandRange(MinParticleLifetime, MaxParticleLifetime);
	}
}


bool SDaylonParticles::Update(float DeltaTime)
{
	bool Alive = false;

	for(auto& Particle : Particles)
	{
		Alive |= (Particle.Update(DeltaTime)); 
	}

	return Alive;
}


int32 SDaylonParticles::OnPaint
(
	const FPaintArgs&          Args,
	const FGeometry&           AllottedGeometry,
	const FSlateRect&          MyCullingRect,
	FSlateWindowElementList&   OutDrawElements,
	int32                      LayerId,
	const FWidgetStyle&        InWidgetStyle,
	bool                       bParentEnabled
) const
{
	for(const auto& Particle : Particles)
	{
		if(Particle.LifeRemaining <= 0.0f)
		{
			continue;
		}

		// Draw the particle.
		
#if 0
		const FPaintGeometry PaintGeometry(
			AllottedGeometry.GetAbsolutePosition() + AllottedGeometry.GetAbsoluteSize() / 2  + Particle.P + 0.5f, 
			FVector2D(ParticleSize), 
			AllottedGeometry.Scale);
#else
		const FPaintGeometry PaintGeometry(
			AllottedGeometry.GetAbsolutePosition() + AllottedGeometry.GetAbsoluteSize() / 2  + (Particle.P * AllottedGeometry.Scale) + 0.5f, 
			FVector2D(Particle.Size) * AllottedGeometry.Scale,
			1.0f);
#endif

		if(IsValid(ParticleBrush.GetResourceObject()))
		{
			// Overdrive starting opacity so that we don't start darkening right away.
			float CurrentOpacity = FMath::Lerp(FinalOpacity, 1.5f, Particle.LifeRemaining / Particle.StartingLifeRemaining);
			CurrentOpacity = FMath::Min(1.0f, CurrentOpacity);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				PaintGeometry,
				&ParticleBrush,
				ESlateDrawEffect::None,
				ParticleBrush.TintColor.GetSpecifiedColor() * RenderOpacity * CurrentOpacity * InWidgetStyle.GetColorAndOpacityTint().A);
		}
	}

	return LayerId;
}


// ------------------------------------------------------------------------------------------------------------------------------------------

void SDaylonLineParticles::Construct(const FArguments& InArgs)
{
	Size                = InArgs._Size.Get();
	MinParticleVelocity = InArgs._MinParticleVelocity.Get();
	MaxParticleVelocity = InArgs._MaxParticleVelocity.Get();
	MinParticleLifetime = InArgs._MinParticleLifetime.Get();
	MaxParticleLifetime = InArgs._MaxParticleLifetime.Get();
	LineThickness       = InArgs._LineThickness.Get();
	FinalOpacity        = InArgs._FinalOpacity.Get();
}


void SDaylonLineParticles::SetParticles(const TArray<FDaylonLineParticle>& InParticles) 
{ 
	Particles = InParticles;

	for(auto& Particle : Particles)
	{
		// Make each shield segment fly off along its vector to the center with a little random deviation.
		const auto Angle = UDaylonUtils::Vector2DToAngle(Particle.P) + FMath::RandRange(-15.0f, 15.0f);

		Particle.Inertia       = /*UDaylonUtils::RandVector2D()*/ 
			UDaylonUtils::AngleToVector2D(Angle) 
			* FMath::RandRange(MinParticleVelocity, MaxParticleVelocity);
			
		Particle.LifeRemaining = Particle.StartingLifeRemaining = FMath::RandRange(MinParticleLifetime, MaxParticleLifetime);
	}
}


bool SDaylonLineParticles::Update(float DeltaTime)
{
	bool Alive = false;

	for(auto& Particle : Particles)
	{
		Alive |= Particle.Update(DeltaTime);
	}

	return Alive;
}


int32 SDaylonLineParticles::OnPaint
(
	const FPaintArgs&          Args,
	const FGeometry&           AllottedGeometry,
	const FSlateRect&          MyCullingRect,
	FSlateWindowElementList&   OutDrawElements,
	int32                      LayerId,
	const FWidgetStyle&        InWidgetStyle,
	bool                       bParentEnabled
) const
{
	for(const auto& Particle : Particles)
	{
		if(Particle.LifeRemaining <= 0.0f)
		{
			continue;
		}

		// Draw the line particle.

		// Overdrive starting opacity so that we don't start darkening right away.
		float CurrentOpacity = FMath::Lerp(FinalOpacity, 1.5f, Particle.LifeRemaining / Particle.StartingLifeRemaining);
		CurrentOpacity = FMath::Min(1.0f, CurrentOpacity);

		FLinearColor Color = Particle.Color;
		Color.A = CurrentOpacity;

		TArray<FVector2f> Points;

		const auto AngleVec = UDaylonUtils::AngleToVector2D(Particle.Angle) * Particle.Length / 2;

		const auto P1 = Particle.P + AngleVec;
		const auto P2 = Particle.P - AngleVec;

		Points.Add(UE::Slate::CastToVector2f(P1));
		Points.Add(UE::Slate::CastToVector2f(P2));

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, LineThickness);
	}

	return LayerId;
}


FVector2D SDaylonLineParticles::ComputeDesiredSize(float) const 
{
	return Size; 
}


#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif
