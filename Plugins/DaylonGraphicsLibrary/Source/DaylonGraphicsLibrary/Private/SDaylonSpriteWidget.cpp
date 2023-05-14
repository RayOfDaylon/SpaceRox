// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "SDaylonSpriteWidget.h"


#define DEBUG_MODULE      0

#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif


void SDaylonSpriteWidget::Construct(const FArguments& InArgs)
{
	Size = InArgs._Size.Get();
}


FVector2D SDaylonSpriteWidget::ComputeDesiredSize(float) const 
{
	return Size; 
}


void SDaylonSpriteWidget::SetSize(const FVector2D& InSize)
{
	Size = InSize; 
}


void SDaylonSpriteWidget::SetAtlas(const FDaylonSpriteAtlas& InAtlas) 
{
	Atlas = InAtlas;

	UvSize.Set(1.0 / Atlas.CelsAcross, 1.0 / Atlas.CelsDown);

	Reset();
}


void SDaylonSpriteWidget::Reset()
{
	SetCurrentCel(0);

	CurrentAge = 0.0f;
}


void SDaylonSpriteWidget::SetCurrentCel(int32 Index)
{
	if(Index < 0 || Index >= Atlas.NumCels)
	{
		return;
	}

	CurrentCelIndex = Index;
}


void SDaylonSpriteWidget::Update(float DeltaTime)
{
	const auto SecondsPerFrame = 1.0f / Atlas.FrameRate;

	//CurrentCelIndex = FMath::RoundToInt(CurrentAge / SecondsPerFrame);
	//CurrentCelIndex = CurrentCelIndex % Atlas.NumCels;

	SetCurrentCel((FMath::RoundToInt(CurrentAge / SecondsPerFrame)) % Atlas.NumCels);

	CurrentAge += DeltaTime;

	const auto AnimDuration = Atlas.NumCels * SecondsPerFrame;

	while(CurrentAge > AnimDuration)
	{
		CurrentAge -= Atlas.NumCels / Atlas.FrameRate;
	}

	//UE_LOG(LogSlate, Log, TEXT("sprite widget::update: currentage = %.3f, currentcelindex = %d"), CurrentAge, CurrentCelIndex);
}


int32 SDaylonSpriteWidget::OnPaint
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
	if(!IsValid(Atlas.AtlasBrush.GetResourceObject()))
	{
		return LayerId;
	}

	// Get upper left UV coordinate of current cel.
	const FVector2D UV(
		(CurrentCelIndex % Atlas.CelsAcross) / (float)Atlas.CelsAcross,
		(CurrentCelIndex / Atlas.CelsAcross) / (float)Atlas.CelsDown);

	const FBox2D uvRegion = FBox2D(UV, UV + UvSize);

	if(IsValid(Atlas.AtlasBrush.GetResourceObject()))
	{
		Atlas.AtlasBrush.SetUVRegion(uvRegion);

		if(AllottedGeometry.HasRenderTransform())
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				&Atlas.AtlasBrush,
				ESlateDrawEffect::None,
				Atlas.AtlasBrush.TintColor.GetSpecifiedColor() * RenderOpacity * InWidgetStyle.GetColorAndOpacityTint().A);
		}
		else
		{
			const auto GeomSize = AllottedGeometry.GetAbsoluteSize();
			const FPaintGeometry PaintGeometry(AllottedGeometry.GetAbsolutePosition(), GeomSize, 1.0f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				PaintGeometry,
				&Atlas.AtlasBrush,
				ESlateDrawEffect::None,
				Atlas.AtlasBrush.TintColor.GetSpecifiedColor() * RenderOpacity * InWidgetStyle.GetColorAndOpacityTint().A);
		}

#if 0
		{
			// Draw where P is.
			FLinearColor Red(1.0f, 0.0f, 0.0f, 1.0f);
			const FPaintGeometry PaintGeometry2(AllottedGeometry.GetAbsolutePosition(), FVector2D(4), 1.0f);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				PaintGeometry2,
				&Atlas.AtlasBrush,
				ESlateDrawEffect::None,
				Red * RenderOpacity * InWidgetStyle.GetColorAndOpacityTint().A);
		}
#endif

	}

	return LayerId;
}


#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
