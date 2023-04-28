// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/UMG/Public/Components/Widget.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"

#include "SSpriteWidget.h"
#include "USpriteWidget.generated.h"

// Warning: do NOT use this widget at design time.


UCLASS(BluePrintType)
class SPACEROX_API USpriteWidgetAtlas : public UDataAsset
{
	GENERATED_BODY()

	public:

		// The texture holding the sprite cels.
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FSpriteAtlas Atlas;
};


UCLASS(meta=(DisplayName="Sprite Widget"))
class SPACEROX_API USpriteWidget : public UWidget
{
	GENERATED_UCLASS_BODY()

	public:  

		UPROPERTY(EditAnywhere, BlueprintReadonly)
		USpriteWidgetAtlas* TextureAtlas;

		FVector2D   Size = FVector2D(16);

		void Tick    (float DeltaTime);
		void Reset   ();

		virtual void SynchronizeProperties () override;
		virtual void ReleaseSlateResources (bool bReleaseChildren) override;


	protected:

		virtual TSharedRef<SWidget> RebuildWidget() override;

		TSharedPtr<SSpriteWidget> MySprite;
};
