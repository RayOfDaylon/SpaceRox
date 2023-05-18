// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#include "SDaylonSprite.h"


#define DEBUG_MODULE      0

#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif


void FDaylonSpriteAtlas::InitCache()
{
	UVSize       = FVector2D(1.0 / CelsAcross, 1.0 / CelsDown);
	CelPixelSize = AtlasBrush.GetImageSize() * UVSize;
}


bool FDaylonSpriteAtlas::IsValidCelIndex(int32 Index) const
{
	return (Index >= 0 && Index < NumCels);
}


int32 FDaylonSpriteAtlas::CalcCelIndex(int32 CelX, int32 CelY) const
{
	return (CelY * CelsAcross + CelX);
}


FVector2D FDaylonSpriteAtlas::GetCelPixelSize() const
{
	return CelPixelSize;
}


FVector2D FDaylonSpriteAtlas::GetUVSize() const
{
	return UVSize;
}


FBox2d FDaylonSpriteAtlas::GetUVsForCel(int32 Index) const
{
	if(!IsValidCelIndex(Index))
	{
		return FBox2d(FVector2D(0), FVector2D(0));
	}

	return GetUVsForCel(Index % CelsAcross, Index / CelsAcross);
}


FBox2d FDaylonSpriteAtlas::GetUVsForCel(int32 CelX, int32 CelY) const
{
	//const FVector2D UV(CelX / (double)CelsAcross, CelY / (double)CelsDown);
	const FVector2D UV(CelX * UVSize.X, CelY * UVSize.Y);

	return FBox2D(UV, UV + UVSize);
}


// --------------------------------------------------------------------------------------


void SDaylonSprite::Construct(const FArguments& InArgs)
{
	Size = InArgs._Size.Get();
}


FVector2D SDaylonSprite::ComputeDesiredSize(float) const 
{
	return Size; 
}


void SDaylonSprite::SetSize(const FVector2D& InSize)
{
	Size = InSize; 
}


void SDaylonSprite::SetAtlas(const FDaylonSpriteAtlas& InAtlas) 
{
	Atlas = InAtlas;

	Atlas.InitCache();

	Reset();
}


void SDaylonSprite::Reset()
{
	SetCurrentCel(0);

	CurrentAge = 0.0f;
}


void SDaylonSprite::SetCurrentCel(int32 Index)
{
	if(!Atlas.IsValidCelIndex(Index))
	{
		return;
	}

	CurrentCelIndex = Index;
}


void SDaylonSprite::SetCurrentCel(int32 CelX, int32 CelY)
{
	return SetCurrentCel(Atlas.CalcCelIndex(CelX, CelY));
}


void SDaylonSprite::Update(float DeltaTime)
{
	const auto SecondsPerFrame = 1.0f / Atlas.FrameRate;

	SetCurrentCel((FMath::RoundToInt(CurrentAge / SecondsPerFrame)) % Atlas.NumCels);

	CurrentAge += DeltaTime;

	const auto AnimDuration = Atlas.NumCels * SecondsPerFrame;

	while(CurrentAge > AnimDuration)
	{
		CurrentAge -= Atlas.NumCels / Atlas.FrameRate;
	}

	//UE_LOG(LogSlate, Log, TEXT("sprite widget::update: currentage = %.3f, currentcelindex = %d"), CurrentAge, CurrentCelIndex);
}


int32 SDaylonSprite::OnPaint
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

	const FBox2D uvRegion = Atlas.GetUVsForCel(CurrentCelIndex);

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
