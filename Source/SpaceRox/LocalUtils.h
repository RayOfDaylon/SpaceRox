// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// LocalUtils - general-purpose stuff that could be used for different games


#pragma once

#include "CoreMinimal.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/CanvasPanelSlot.h"
#include "LocalUtils.generated.h"


const double Epsilon = 1e-14;

FORCEINLINE double Square                        (double x) { return x * x; }

FVector2D   RandVector2D                         ();
float       WrapAngle                            (float Angle);
FVector2D   AngleToVector2D                      (float Angle);
float       Vector2DToAngle                      (const FVector2D& Vector);
FVector2D   Rotate                               (const FVector2D& P, float Angle); 

bool        DoesLineSegmentIntersectCircle       (const FVector2D& P1, const FVector2D& P2, const FVector2D& CP, double R);
bool        DoesLineSegmentIntersectTriangle     (const FVector2D& P1, const FVector2D& P2, const FVector2D Triangle[3]);
bool        DoesTriangleIntersectTriangle        (const FVector2D TriA[3], const FVector2D TriB[3]);

FVector2D   GetWidgetDirectionVector             (const UWidget* Widget);
void        Show                                 (UWidget*, bool Visible = true);
void        Hide                                 (UWidget* Widget);



USTRUCT()
struct FLoopedSound
{
	// A hacked sound loop created by simply playing the same sound
	// over and over if Tick() is called.

	GENERATED_BODY()

	USoundBase* Sound = nullptr;

	float VolumeScale = 1.0f;

	UObject* WorldContextPtr = nullptr;

	void Set(UObject* Context, USoundBase* InSound, float InVolumeScale = 1.0f)
	{
		WorldContextPtr = Context;
		Sound           = InSound;
		VolumeScale     = InVolumeScale;
	}

	void Start();

	void Tick(float DeltaTime);

	protected:

		float TimeRemaining = 0.0f;
};


USTRUCT()
struct FPlayObject
{
	// Stuff common to visible game entities like ships, player, bullets, etc.

	GENERATED_BODY()

	FVector2D Inertia; // Direction and velocity, px/sec
	FVector2D OldPosition;
	FVector2D UnwrappedNewPosition;

	float LifeRemaining = 0.0f;
	float RadiusFactor  = 0.5f;
	float SpinSpeed     = 0.0f; // degrees/second
	int32 Value         = 0;

	UImage* Widget = nullptr; // Associated image widget

	bool IsVisible () const { return (Widget != nullptr ? Widget->IsVisible() : false); }
	bool IsAlive   () const { return (LifeRemaining > 0.0f); }
	bool IsDead    () const { return !IsAlive(); }
	void Show      (bool Visible = true) { ::Show(Widget, Visible); }
	void Hide      () { Show(false); }
	void Kill      () { LifeRemaining = 0.0f; Hide(); }

	
	bool IsValid   () const 
	{ 
		return (Value >= 0) && (Widget != nullptr) && (Cast<UCanvasPanelSlot>(Widget->Slot) != nullptr);
	}

	
	float GetRadius() const
	{
		return GetSize().X * RadiusFactor;
	}


	UCanvasPanelSlot*       GetWidgetSlot () { return Cast<UCanvasPanelSlot>(Widget->Slot); }
	const UCanvasPanelSlot* GetWidgetSlot () const { return Cast<UCanvasPanelSlot>(Widget->Slot); }


	FVector2D GetPosition() const
	{
		if(Widget->Slot == nullptr)
		{
			return FVector2D(0);
		}

		return GetWidgetSlot()->GetPosition();
	}


	void SetPosition(const FVector2D& P)
	{
		if(Widget->Slot == nullptr)
		{
			return;
		}

		return GetWidgetSlot()->SetPosition(P);
	}


	FVector2D GetNextPosition(float DeltaTime) const
	{
		return GetPosition() + (Inertia * DeltaTime);
	}


	FVector2D GetSize() const
	{
		if(Widget->Slot == nullptr)
		{
			return FVector2D(0);
		}

		return GetWidgetSlot()->GetSize();
	}


	float GetSpeed() const
	{
		return Inertia.Length();
	}


	float GetAngle() const
	{
		return (Widget == nullptr ? 0.0f : Widget->GetRenderTransformAngle());
	}


	void SetAngle(float Angle)
	{
		if(Widget == nullptr)
		{
			return;
		}

		Widget->SetRenderTransformAngle(Angle);
	}
};


USTRUCT()
struct FScheduledTask
{
	// A TFunction that runs a specified number of seconds after the task is created.
	// You'll normally want to specify What as a lambda that captures a TWeakObjPtr<UObject-derived class>
	// e.g. This=this, so that you can test the weak pointer before running the lambda.

	GENERATED_BODY()

	float  When = 0.0f;   // Number of seconds into the future.

	TFunction<void()> What;

	bool Tick(float DeltaTime)
	{
		// Return true when function finally executes.

		if(!What)
		{
			return false;
		}

		When -= DeltaTime;

		if(When > 0.0f)
		{
			return false;
		}
		
		What();

		return true;
	}
};


USTRUCT()
struct FDurationTask
{
	// A TFunction that runs every time Tick() is called until the task duration reaches zero, 
	// at which point its completion TFunction will be called.
	// See FScheduledTask for more comments.

	GENERATED_BODY()

	float  Duration = 0.0f;   // number of seconds to run the task for.

	TFunction<void(float)> What;
	TFunction<void()>      Completion;

	bool Tick(float DeltaTime)
	{
		// Return true if task is still active.

		if(!What)
		{
			return false;
		}

		if(Elapsed >= Duration)
		{
			if(Completion)
			{
				Completion();
			}
			return false;
		}

		What(Elapsed);

		Elapsed += DeltaTime;

		return true;
	}

	protected:
		float Elapsed = 0.0f;
};


USTRUCT()
struct FHighScore
{
	GENERATED_BODY()
	
	FString Name;
	int32   Score = 0;

	FHighScore() {}
	FHighScore(int32 InScore, const FString& InName) : Name(InName), Score(InScore)	{}

	bool operator < (const FHighScore& Rhs) const 
	{
		if(Score == Rhs.Score)
		{
			return (Name.Compare(Rhs.Name) < 0);
		}

		return (Score < Rhs.Score); 
	}
};


USTRUCT()
struct FHighScoreTable
{
	GENERATED_BODY()

	int32 MaxEntries    = 10;
	int32 MaxNameLength = 30; // Max. number of characters a name can have. Original Asteroids game used 3 chars.

	TArray<FHighScore> Entries;

	int32  GetLongestNameLength () const;
	bool   CanAdd               (int32 Score) const;
	void   Add                  (int32 Score, const FString& Name);
	void   Clear                () { Entries.Empty(); }
};
