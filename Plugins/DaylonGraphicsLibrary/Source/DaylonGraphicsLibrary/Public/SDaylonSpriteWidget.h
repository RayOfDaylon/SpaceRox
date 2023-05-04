// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#pragma once

#include "SlateCore/Public/Widgets/SLeafWidget.h"
#include "SDaylonSpriteWidget.generated.h"



// SSpriteWidget - a drawing widget with a custom Paint event.


USTRUCT(BlueprintType)
struct DAYLONGRAPHICSLIBRARY_API FSpriteAtlas
{
	GENERATED_USTRUCT_BODY()

	FSpriteAtlas() {}

	// Texture containing a flipbook of one or more cels.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSlateBrush AtlasBrush;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int32 CelsAcross = 1;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int32 CelsDown = 1;

	// Total number of cels to use (starting at cel 0,0)
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int32 NumCels = 1;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	float FrameRate = 30.0f;
};


class DAYLONGRAPHICSLIBRARY_API SDaylonSpriteWidget : public SLeafWidget
{
	public:
		SLATE_BEGIN_ARGS(SDaylonSpriteWidget)
			: 
			  _Size                (FVector2D(16))
			{
				_Clipping = EWidgetClipping::OnDemand;
			}

			SLATE_ATTRIBUTE(FVector2D, Size)

			SLATE_END_ARGS()

			SDaylonSpriteWidget() {}

			~SDaylonSpriteWidget() {}

			void Construct(const FArguments& InArgs);


			void SetSize                  (const FVector2D& InSize);
			void SetAtlas                 (const FSpriteAtlas& InAtlas);

			void Update                   (float DeltaTime);
			void Reset                    ();


			virtual int32 OnPaint
			(
				const FPaintArgs&          Args,
				const FGeometry&           AllottedGeometry,
				const FSlateRect&          MyCullingRect,
				FSlateWindowElementList&   OutDrawElements,
				int32                      LayerId,
				const FWidgetStyle&        InWidgetStyle,
				bool                       bParentEnabled
			) const override;

			virtual FVector2D ComputeDesiredSize(float) const override;


		protected:

			FVector2D              Size;
			mutable FSpriteAtlas   Atlas;

			float              CurrentAge = 0.0f;
			int32              CurrentCelIndex = 0;

			virtual bool ComputeVolatility() const override { return true; }

};
