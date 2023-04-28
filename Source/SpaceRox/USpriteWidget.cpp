// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "USpriteWidget.h"


USpriteWidget::USpriteWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsVariable = false;
}


TSharedRef<SWidget> USpriteWidget::RebuildWidget()
{
	MySprite = SNew(SSpriteWidget);

	return MySprite.ToSharedRef();
}


void USpriteWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MySprite->SetSize (Size);
	MySprite->SetAtlas(TextureAtlas->Atlas);

	Reset();
}


void USpriteWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MySprite.Reset();
}


void USpriteWidget::Tick(float DeltaTime)
{
	check(MySprite);
	MySprite->Update(DeltaTime);
}


void USpriteWidget::Reset()
{
	check(MySprite);
	MySprite->Reset();
}


