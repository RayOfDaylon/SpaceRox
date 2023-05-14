// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#pragma once

#include "SlateCore/Public/Widgets/SLeafWidget.h"
#include "SDaylonSpriteWidget.generated.h"



// SDaylonSpriteWidget - a drawing widget with a custom Paint event.
// If your sprite is animated, call Update().
// If you just want to switch amongst cels, call SetCurrentCel().


USTRUCT(BlueprintType)
struct DAYLONGRAPHICSLIBRARY_API FDaylonSpriteAtlas
{
	GENERATED_USTRUCT_BODY()

	FDaylonSpriteAtlas() {}

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

	// If using the atlas to animate a sprite, set a frame rate.
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	float FrameRate = 30.0f;

	FVector2D UVSize;


	void       InitCache        ();
			   
	bool       IsValidCelIndex  (int32 Index) const;

	int32      CalcCelIndex     (int32 CelX, int32 CelY) const;
			   
	FVector2D  GetUVSize        () const;
			   
	FBox2d     GetUVsForCel     (int32 Index) const;
	FBox2d     GetUVsForCel     (int32 CelX, int32 CelY) const;
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


			void SetSize    (const FVector2D& InSize);
			void SetAtlas   (const FDaylonSpriteAtlas& InAtlas);

			void SetCurrentCel (int32 Index);
			void SetCurrentCel (int32 CelX, int32 CelY);
			void Update        (float DeltaTime);
			void Reset         ();


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

			FVector2D                    Size;
			mutable FDaylonSpriteAtlas   Atlas;

			float              CurrentAge      = 0.0f;
			int32              CurrentCelIndex = 0;

			virtual bool ComputeVolatility() const override { return true; }

};
