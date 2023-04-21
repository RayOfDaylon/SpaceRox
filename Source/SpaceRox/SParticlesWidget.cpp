// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "SParticlesWidget.h"
#include "LocalUtils.h"


void SParticlesWidget::Construct(const FArguments& InArgs)
{
	Size = InArgs._Size.Get();
	MinParticleSize     = InArgs._MinParticleSize.Get();
	MaxParticleSize     = InArgs._MaxParticleSize.Get();
	MinParticleLifetime = InArgs._MinParticleLifetime.Get();
	MinParticleVelocity = InArgs._MinParticleVelocity.Get();
	MaxParticleVelocity = InArgs._MaxParticleVelocity.Get();
	MaxParticleLifetime = InArgs._MaxParticleLifetime.Get();
	FinalOpacity        = InArgs._FinalOpacity.Get();

	SetNumParticles(InArgs._NumParticles.Get());
}


FVector2D SParticlesWidget::ComputeDesiredSize(float) const 
{
	return Size; 
}


void SParticlesWidget::SetSize                  (const FVector2D& InSize)    { Size = InSize; }
void SParticlesWidget::SetParticleBrush         (const FSlateBrush& InBrush) { ParticleBrush = InBrush; }
void SParticlesWidget::SetMinParticleSize       (float InSize)               { MinParticleSize = InSize; }
void SParticlesWidget::SetMaxParticleSize       (float InSize)               { MaxParticleSize = InSize; }
void SParticlesWidget::SetMinParticleVelocity   (float Velocity)             { MinParticleVelocity = Velocity; }
void SParticlesWidget::SetMaxParticleVelocity   (float Velocity)             { MaxParticleVelocity = Velocity; }
void SParticlesWidget::SetMinParticleLifetime   (float Lifetime)             { MinParticleLifetime = Lifetime; }
void SParticlesWidget::SetMaxParticleLifetime   (float Lifetime)             { MaxParticleLifetime = Lifetime; }
void SParticlesWidget::SetFinalOpacity          (float Opacity)              { FinalOpacity = Opacity; }


void SParticlesWidget::SetNumParticles          (int32 Count)             
{
	check(Count > 0);
	Particles.SetNum(Count);
	Reset();
}


void SParticlesWidget::Reset()
{
	for(auto& Particle : Particles)
	{
		Particle.P             = FVector2D(0); // todo: could randomize this a small distance for more realism
		Particle.Size          = FMath::RandRange(MinParticleSize, MaxParticleSize);
		Particle.Interia       = RandVector2D() * FMath::RandRange(MinParticleVelocity, MaxParticleVelocity);
		Particle.LifeRemaining = Particle.StartingLifeRemaining = FMath::RandRange(MinParticleLifetime, MaxParticleLifetime);
	}
}


bool SParticlesWidget::Update(float DeltaTime)
{
	bool Alive = false;

	for(auto& Particle : Particles)
	{
		Particle.P += Particle.Interia * DeltaTime;
		Particle.LifeRemaining -= DeltaTime;
		Alive |= (Particle.LifeRemaining > 0.0f); 
	}

	return Alive;
}


int32 SParticlesWidget::OnPaint
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


