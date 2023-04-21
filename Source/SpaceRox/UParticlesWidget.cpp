// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "UParticlesWidget.h"


UParticlesWidget::UParticlesWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsVariable = false;
}


TSharedRef<SWidget> UParticlesWidget::RebuildWidget()
{
	MyDynamicImage = SNew(SParticlesWidget);

	return MyDynamicImage.ToSharedRef();
}


void UParticlesWidget::SynchronizeProperties ()
{
	Super::SynchronizeProperties();

	MyDynamicImage->SetSize                   (Size);
	MyDynamicImage->SetMinParticleSize        (MinParticleSize);
	MyDynamicImage->SetMaxParticleSize        (MaxParticleSize);
	MyDynamicImage->SetMinParticleVelocity    (MinParticleVelocity);
	MyDynamicImage->SetMaxParticleVelocity    (MaxParticleVelocity);
	MyDynamicImage->SetMinParticleLifetime    (MinParticleLifetime);
	MyDynamicImage->SetMaxParticleLifetime    (MaxParticleLifetime);
	MyDynamicImage->SetNumParticles           (NumParticles);
	MyDynamicImage->SetFinalOpacity           (FinalOpacity);
	MyDynamicImage->SetParticleBrush          (ParticleBrush);

	Reset();
}


void UParticlesWidget::ReleaseSlateResources (bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyDynamicImage.Reset();
}


bool UParticlesWidget::Update                   (float DeltaTime)
{
	check(MyDynamicImage);
	return MyDynamicImage->Update(DeltaTime);
}


void UParticlesWidget::Reset                    ()
{
	check(MyDynamicImage);
	MyDynamicImage->Reset();
}


