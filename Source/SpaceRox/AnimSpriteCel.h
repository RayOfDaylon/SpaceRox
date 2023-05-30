// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once

#include "CoreMinimal.h"
#include "Arena.h"



class FAnimSpriteCel : public Daylon::ImagePlayObject2D
{
	public:

	FInt32Rect  SrcPx;
	FInt32Rect  DstPx;
	float       OriginalLifepsan;
	float       StartAge;
	IArena*     ArenaPtr = nullptr;
	bool        bMadeSound = false;


	void Init(IArena* Arena, const FSlateBrush& InBrush, const FBox2d& SrcUVs, const FInt32Rect& InSrcPx, const FInt32Rect& InDstPx, float InStartAge, float InAge)
	{
		check(Arena);
		ArenaPtr = Arena;

		check(Slot);
		SetRenderTransformPivot(FVector2D(0));
		Slot->SetAutoSize(false);
		Slot->SetAlignment(FVector2D(0));

		SetBrush(InBrush);
		Brush.SetUVRegion(SrcUVs);

		SrcPx = InSrcPx;
		DstPx = InDstPx;
		OriginalLifepsan = LifeRemaining = InAge;
		StartAge = InStartAge;
	}


	virtual void Update(float DeltaTime) override
	{
		StartAge -= DeltaTime;

		if(StartAge > 0.0f)
		{
			SetRenderOpacity(0.0f);
			return;
		}

		if(!bMadeSound)
		{
			ArenaPtr->PlaySound(ArenaPtr->GetTorpedoSound());
			bMadeSound = true;
		}

		LifeRemaining = FMath::Max(0.0f, LifeRemaining - DeltaTime);

		const float T = 1.0f - (LifeRemaining / OriginalLifepsan);
		const float Opacity = T;

		SetRenderOpacity(Opacity);

		FInt32Rect CurrentRect(
			FMath::RoundHalfFromZero(FMath::Lerp((float)SrcPx.Min.X, (float)DstPx.Min.X, T)),
			FMath::RoundHalfFromZero(FMath::Lerp((float)SrcPx.Min.Y, (float)DstPx.Min.Y, T)),
			FMath::RoundHalfFromZero(FMath::Lerp((float)SrcPx.Max.X, (float)DstPx.Max.X, T)),
			FMath::RoundHalfFromZero(FMath::Lerp((float)SrcPx.Max.Y, (float)DstPx.Max.Y, T))
		);

		SetPosition    (CurrentRect.Min);
		SetSizeInSlot  (FVector2D(CurrentRect.Width(), CurrentRect.Height()));
	};
};
