// Copyright 2023 Daylon Graphics Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UMG/Public/Blueprint/WidgetTree.h"
#include "UMG/Public/Components/Widget.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/CanvasPanel.h"
#include "UMG/Public/Components/CanvasPanelSlot.h"
#include "UDaylonSpriteWidget.h"
#include "DaylonUtils.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDaylon, Log, All);


// Set to 1 to enable play objects to use Slate widgets directly instead of UMG.
// todo: currently crashes, and our object arrays end up having 'fat' elements
// when we should just be storing pointers.
#define INCLUDE_SLATE_ONLY_SYSTEM 0


UCLASS()
class DAYLONGRAPHICSLIBRARY_API UDaylonUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	protected:

	static UWidgetTree*  WidgetTree;
	static UCanvasPanel* RootCanvas;


	public:

	static const double Epsilon;

	template<class WidgetT> static WidgetT* MakeWidget()
	{
		auto Widget = WidgetTree->ConstructWidget<WidgetT>();
		check(Widget);
		return Widget;
	}


	static void          SetWidgetTree   (UWidgetTree* InWidgetTree) { WidgetTree = InWidgetTree; }
	static UWidgetTree*  GetWidgetTree   () { return WidgetTree; }
	static void          SetRootCanvas   (UCanvasPanel* InCanvas) { RootCanvas = InCanvas; }
	static UCanvasPanel* GetRootCanvas   () { return RootCanvas; }

	static float       Normalize         (float N, float Min, float Max);
	static FVector2D   AngleToVector2D   (float Angle);
	static FVector2D   RandVector2D      ();
	static FVector2D   Rotate            (const FVector2D& P, float Angle); 
	static float       Vector2DToAngle   (const FVector2D& Vector);
	static float       WrapAngle         (float Angle);

	FORCEINLINE static double Square     (double x) { return x * x; }

	static FVector2D   ComputeFiringSolution                (const FVector2D& LaunchP, float TorpedoSpeed, const FVector2D& TargetP, const FVector2D& TargetInertia);
	static bool        DoesLineSegmentIntersectCircle       (const FVector2D& P1, const FVector2D& P2, const FVector2D& CP, double R);
	static bool        DoesLineSegmentIntersectTriangle     (const FVector2D& P1, const FVector2D& P2, const FVector2D Triangle[3]);
	static bool        DoesTriangleIntersectTriangle        (const FVector2D TriA[3], const FVector2D TriB[3]);
	static bool        DoCirclesIntersect                   (const FVector2D& C1, float R1, const FVector2D& C2, float R2);
	static FVector2D   RandomPtWithinBox                    (const FBox2d& Box);

	static FVector2D   GetWidgetDirectionVector             (const UWidget* Widget);
	static void        Show                                 (UWidget*, bool Visible = true);
	static void        Hide                                 (UWidget* Widget);
	static void        Show                                 (SWidget*, bool Visible = true);
	static void        Hide                                 (SWidget* Widget);

};


namespace Daylon
{
	UENUM()
	enum class EListNavigationDirection : int32
	{
		// These values are casted to int
		Backwards = -1,
		Forwards  =  1,
	};


	/*
	UENUM()
	enum class ERotationDirection : int32
	{
		// These values are casted to int
		CounterClockwise = -1,
		Clockwise        =  1,
	};*/


	struct DAYLONGRAPHICSLIBRARY_API FLoopedSound
	{
		// A hacked sound loop created by simply playing the same sound
		// over and over if Tick() is called.

		USoundBase* Sound = nullptr;

		float VolumeScale  = 1.0f;
		float VolumeScale2 = 1.0f;

		UObject* WorldContextPtr = nullptr;

		void Set(UObject* Context, USoundBase* InSound, float InVolumeScale = 1.0f)
		{
			WorldContextPtr = Context;
			Sound           = InSound;
			VolumeScale     = InVolumeScale;
		}

		void Start(float InVolumeScale = 1.0f);

		void Tick(float DeltaTime);

		protected:

			float TimeRemaining = 0.0f;
	};


	struct DAYLONGRAPHICSLIBRARY_API FScheduledTask
	{
		// A TFunction that runs a specified number of seconds after the task is created.
		// You'll normally want to specify What as a lambda that captures a TWeakObjPtr<UObject-derived class>
		// e.g. This=this, so that you can test the weak pointer before running the lambda.


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


	struct DAYLONGRAPHICSLIBRARY_API FDurationTask
	{
		// A TFunction that runs every time Tick() is called until the task duration reaches zero, 
		// at which point its completion TFunction will be called.
		// See FScheduledTask for more comments.


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


	// A type that acts like a given type except that when the value is modified,
	// it invokes a delegate.
	template <typename T>
	class TBindableValue
	{
		protected:

			T                             Value;
			TFunction<void(const T& Val)> Delegate;


		public:

			TBindableValue()
			{
			}


			TBindableValue(const T& Val, TFunction<void(const T& Val)> Del) : Value(Val), Delegate(Del) 
			{
			}


			operator T& () { return Value; }
			operator const T& () const { return Value; }

			// Some calls to operator T& won't compile e.g. as variadic args, but these should work.
			T& GetValue() { return (T&)*this; }
			const T& GetValue() const { return (T&)*this; }

			void Bind(TFunction<void(const T& Val)> Del)
			{
				Delegate = Del;

				if(Delegate)
				{
					Delegate(Value);
				}
			}



			TBindableValue& operator = (const T& Val)
			{
				if(Value != Val)
				{
					Value = Val;

					if(Delegate)
					{
						Delegate(Value);
					}
				}

				return *this;
			}

			TBindableValue& operator += (const T& Val) { *this = *this + Val; return *this;	 }
			TBindableValue& operator -= (const T& Val) { *this = *this - Val; return *this;	 }
			TBindableValue& operator *= (const T& Val) { *this = *this * Val; return *this;	 }
			TBindableValue& operator /= (const T& Val) { *this = *this / Val; return *this; }

			bool operator <  (const T& Val) const { return (Value < Val); }
			bool operator <= (const T& Val) const { return (Value <= Val); }
			bool operator == (const T& Val) const { return (Value == Val); }
			bool operator >= (const T& Val) const { return (Value >= Val); }
			bool operator >  (const T& Val) const { return (Value > Val); }
			bool operator != (const T& Val) const { return (Value != Val); }
	};


	struct DAYLONGRAPHICSLIBRARY_API FHighScore
	{
	
		FString Name;
		int32   Score = 0;

		
		FHighScore() {}
		FHighScore(int32 InScore, const FString& InName) : Name(InName), Score(InScore)	{}

		
		void Set(int32 InScore, const FString& InName)
		{
			Name  = InName;
			Score = InScore;
		}


		bool operator == (const FHighScore& Rhs) const
		{
			return (Name == Rhs.Name && Score == Rhs.Score);
		}


		bool operator < (const FHighScore& Rhs) const
		{
			if(Score == Rhs.Score)
			{
				return (Name.Compare(Rhs.Name) < 0);
			}

			return (Score < Rhs.Score); 
		}
	};


	struct DAYLONGRAPHICSLIBRARY_API FHighScoreTable
	{
		int32 MaxEntries    = 10;
		int32 MaxNameLength = 30; // Max. number of characters a name can have. Original Asteroids game used 3 chars.

		TArray<FHighScore> Entries;

		int32  GetLongestNameLength () const;
		bool   CanAdd               (int32 Score) const;
		void   Add                  (int32 Score, const FString& Name);
		void   Clear                () { Entries.Empty(); }
	};




	// -------------------------------------------------------------------------------------------------------------------
	// Slate-only 2D play object


	template <class SWidgetT>
	class PlayObject2D : public SWidgetT
	{
		// Stuff common to visible game entities like ships, player, bullets, etc.

		protected:

		SConstraintCanvas::FSlot* Slot = nullptr; // We need this because we cannot get a slot directly from an SWidget.

		FWidgetTransform RenderTransform;


		public:

		FVector2D Inertia; // Direction and velocity, px/sec
		FVector2D OldPosition;
		FVector2D UnwrappedNewPosition;

		float LifeRemaining = 0.0f;
		float RadiusFactor  = 0.5f;
		float SpinSpeed     = 0.0f; // degrees/second
		int32 Value         = 0;


		virtual ~PlayObject2D() {}


		virtual FVector2D GetActualSize() const = 0;


		void SetSlot(SConstraintCanvas::FSlot* InSlot) 
		{
			check(Slot == nullptr);

			Slot = InSlot; 
		}


		bool IsValid   () const 
		{ 
			return ((Slot != nullptr) && (Value >= 0));
		}


		virtual void Update    (float DeltaTime) {}

		bool IsAlive   () const { return (LifeRemaining > 0.0f); }
		bool IsDead    () const { return !IsAlive(); }
		void Show      (bool Visible = true) { UDaylonUtils::Show(this, Visible); }
		void Hide      () { Show(false); }
		void Kill      () { LifeRemaining = 0.0f; Hide(); }
		
		bool IsVisible () const 
		{
			return (IsValid() 
				? (GetVisibility() != EVisibility::Collapsed && GetVisibility() != EVisibility::Hidden) 
				: false);
		}


		void UpdateWidgetSize() 
		{
			if(!IsValid())
			{
				return;
			}

			SetSizeInSlot(GetActualSize());
		}

	
		float GetRadius() const
		{
			return GetSize().X * RadiusFactor;
		}


		FVector2D GetPosition() const
		{
			if(!IsValid())
			{
				return FVector2D(0);
			}

			const auto Margin = Slot->GetOffset();

			return FVector2D(Margin.Left, Margin.Top);
		}


		void SetPosition(const FVector2D& P)
		{
			if(!IsValid())
			{
				return;
			}

			auto Margin = Slot->GetOffset();
			Margin.Left = P.X;
			Margin.Top  = P.Y;

			Slot->SetOffset(Margin);
		}


		FVector2D GetNextPosition(float DeltaTime) const
		{
			return GetPosition() + (Inertia * DeltaTime);
		}


		FVector2D GetSize() const
		{
			if(!IsValid())
			{
				return FVector2D(0);
			}

			const auto Margin = Slot->GetOffset();

			return FVector2D(Margin.Right, Margin.Bottom);
		}


		void SetSizeInSlot(const FVector2D& S) 
		{
			if(!IsValid())
			{
				return;
			}

			auto Margin = Slot->GetOffset();

			Margin.Right  = S.X;
			Margin.Bottom = S.Y;

			Slot->SetOffset(Margin);
		}


		float GetSpeed() const
		{
			return Inertia.Length();
		}


		float GetAngle() const
		{
			return RenderTransform.Angle;
		}


		void SetAngle(float Angle)
		{
			if(!IsValid())
			{
				return;
			}

			RenderTransform.Angle = Angle;
			SetRenderTransform(RenderTransform.ToSlateRenderTransform());
		}


		FVector2D GetDirectionVector() const
		{
			if(!IsValid())
			{
				return FVector2D(0.0f);
			}

			return UDaylonUtils::AngleToVector2D(GetAngle());
		}


		void Move(float DeltaTime, const TFunction<FVector2D(const FVector2D&)>& WrapFunction)
		{
			const auto P = GetPosition();

			OldPosition          = P;
			UnwrappedNewPosition = P + Inertia * DeltaTime;

			SetPosition(WrapFunction(UnwrappedNewPosition));
		}


		void Spawn(const FVector2D& P, const FVector2D& InInertia, float InLifeRemaining)
		{
			if(!IsValid())
			{
				//UE_LOG(LogDaylon, Error, TEXT("FPlayObjectEx::Spawn: invalid widget!"));
				return;
			}

			OldPosition          =
			UnwrappedNewPosition = P;
			Inertia              = InInertia;
			LifeRemaining        = InLifeRemaining;

			SetPosition(P);
			Show();
		}

	};


	class ImagePlayObject2D : public PlayObject2D<SImage>
	{
		protected:

		FSlateBrush Brush;

		
		public:

		virtual FVector2D GetActualSize() const override { return Brush.GetImageSize(); }


		void SetBrush(const FSlateBrush& InBrush)
		{
			Brush = InBrush;
			SetImage(&Brush);
			UpdateWidgetSize();
		}
	};


	class SpritePlayObject2D : public PlayObject2D<SDaylonSprite>
	{
		public:

		virtual FVector2D GetActualSize() const override { return Size; }

		virtual void Update(float DeltaTime) override { SDaylonSprite::Update(DeltaTime); }

	};


	template <class SWidgetT>
	void Install(TSharedPtr<PlayObject2D<SWidgetT>> Widget, float InRadiusFactor)
	{
		Widget->SetRenderTransformPivot(FVector2D(0.5f));

		auto Canvas = UDaylonUtils::GetRootCanvas()->GetCanvasWidget();

		auto SlotArgs = Canvas->AddSlot();

		Widget->SetSlot(SlotArgs.GetSlot());

		SlotArgs[Widget.ToSharedRef()];
		SlotArgs.AutoSize(true);
		SlotArgs.Alignment(FVector2D(0.5));

		Widget->RadiusFactor = InRadiusFactor;
	}


	DAYLONGRAPHICSLIBRARY_API TSharedPtr<ImagePlayObject2D>          CreateImagePlayObject2D  (const FSlateBrush& Brush, float Radius);
	DAYLONGRAPHICSLIBRARY_API TSharedPtr<Daylon::SpritePlayObject2D> CreateSpritePlayObject2D (const FDaylonSpriteAtlas& Atlas, const FVector2D& S, float Radius);

	DAYLONGRAPHICSLIBRARY_API void UninstallImpl(TSharedPtr<SWidget> Widget);

	DAYLONGRAPHICSLIBRARY_API void Uninstall(TSharedPtr<ImagePlayObject2D> Widget);
	DAYLONGRAPHICSLIBRARY_API void Uninstall(TSharedPtr<SpritePlayObject2D> Widget);


} // namespace Daylon


#if 0

// -------------------------------------------------------------------------------------------------------------------
// Legacy UMG-based 2D play object types.


namespace Daylon
{
	struct IPlayObject
	{
		virtual UCanvasPanelSlot*       GetWidgetSlot () = 0;
		virtual const UCanvasPanelSlot* GetWidgetSlot () const = 0;

		virtual void      UpdateWidgetSize () = 0;
		virtual void      DestroyWidget    () = 0;
									   
		virtual bool      IsValid          () const = 0;
		virtual bool      IsAlive          () const = 0;
		virtual bool      IsDead           () const = 0;
		virtual bool      IsVisible        () const = 0;
		virtual void      Show             (bool Visible = true) = 0;
		virtual void      Hide             () = 0;
		virtual void      Kill             () = 0;
		virtual float     GetRadius        () const = 0;
		virtual FVector2D GetPosition      () const = 0;
		virtual void      SetPosition      (const FVector2D& P) = 0;
		virtual FVector2D GetNextPosition  (float DeltaTime) const = 0;
		virtual FVector2D GetSize          () const = 0;
		virtual float     GetSpeed         () const = 0;
		virtual float     GetAngle         () const = 0;
		virtual void      SetAngle         (float Angle) = 0;
		virtual void      Tick             (float DeltaTime) = 0;
	};



	struct ImageWidgetSpecifics
	{
		FVector2D GetSize(UImage* Image) const { return Image->Brush.GetImageSize(); }
	};


	struct SpriteWidgetSpecifics
	{
		FVector2D GetSize(UDaylonSpriteWidget* Sprite) const { return Sprite->Size; }
	};


	template <class WidgetT, class WidgetSpecificsT>
	struct FPlayObject : public IPlayObject
	{
		// Stuff common to visible game entities like ships, player, bullets, etc.

		WidgetT* Widget = nullptr; // Associated widget

		WidgetSpecificsT WidgetSpecifics;


		void FinishCreating(float InRadiusFactor)
		{
			check(Widget);

			Widget->SetRenderTransformPivot(FVector2D(0.5f));

			auto CanvasSlot = UDaylonUtils::GetRootCanvas()->AddChildToCanvas(Widget);
			CanvasSlot->SetAnchors(FAnchors());
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5));

			// Autosize(true) doesn't automatically set the slot offsets to the image size, so do it here.
			UpdateWidgetSize();

			RadiusFactor = InRadiusFactor;
		}


		void UpdateWidgetSize() 
		{
			auto Size = WidgetSpecifics.GetSize(Widget); 
		
			auto Margin = GetWidgetSlot()->GetOffsets();

			Margin.Right  = Size.X;
			Margin.Bottom = Size.Y;

			GetWidgetSlot()->SetOffsets(Margin);
		}


		void DestroyWidget()
		{
			if(Widget == nullptr)
			{
				return;
			}

			Widget->GetParent()->RemoveChild(Widget);
			//WidgetTree->RemoveWidget (Widget);
		}


		FVector2D Inertia; // Direction and velocity, px/sec
		FVector2D OldPosition;
		FVector2D UnwrappedNewPosition;

		float LifeRemaining = 0.0f;
		float RadiusFactor  = 0.5f;
		float SpinSpeed     = 0.0f; // degrees/second
		int32 Value         = 0;


		bool IsAlive   () const { return (LifeRemaining > 0.0f); }
		bool IsDead    () const { return !IsAlive(); }
		bool IsVisible () const { return (Widget != nullptr ? Widget->IsVisible() : false); }
		void Show      (bool Visible = true) { UDaylonUtils::Show(Widget, Visible); }
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
			if(Widget == nullptr || Widget->Slot == nullptr)
			{
				return FVector2D(0);
			}

			return GetWidgetSlot()->GetPosition();
		}


		void SetPosition(const FVector2D& P)
		{
			if(Widget == nullptr || Widget->Slot == nullptr)
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
			if(Widget == nullptr || Widget->Slot == nullptr)
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


		virtual void Tick(float DeltaTime) {}


		void Move(float DeltaTime, const TFunction<FVector2D(const FVector2D&)>& WrapFunction)
		{
			const auto P = GetPosition();

			OldPosition          = P;
			UnwrappedNewPosition = P + Inertia * DeltaTime;

			SetPosition(WrapFunction(UnwrappedNewPosition));
		}


		void Spawn(const FVector2D& P, const FVector2D& InInertia, float InLifeRemaining)
		{
			if(Widget == nullptr)
			{
				UE_LOG(LogDaylon, Error, TEXT("FPlayObject::Spawn: no widget member."));
				return;
			}

			OldPosition          =
			UnwrappedNewPosition = P;
			Inertia              = InInertia;
			LifeRemaining        = InLifeRemaining;

			SetPosition(P);
			Show();
		}

	};


	struct DAYLONGRAPHICSLIBRARY_API FImagePlayObject  : public FPlayObject<UImage, ImageWidgetSpecifics> 
	{
		void Create(const FSlateBrush& Brush, float Radius)
		{
			Widget = UDaylonUtils::MakeWidget<UImage>();
			check(Widget);

			Widget->Brush = Brush;

			FinishCreating(Radius);
		}


		void SetBrush(const FSlateBrush& Brush)
		{
			Widget->Brush = Brush;
		}
	};

	
	struct DAYLONGRAPHICSLIBRARY_API FSpritePlayObject : public FPlayObject<UDaylonSpriteWidget, SpriteWidgetSpecifics> 
	{
		void Create(UDaylonSpriteWidgetAtlas* WidgetAtlas, float InRadiusFactor, const FLinearColor& Tint, const FVector2D& Size)
		{
			check(WidgetAtlas);
			check(Size.X > 0 && Size.Y > 0);

			Widget = UDaylonUtils::MakeWidget<UDaylonSpriteWidget>();
			check(Widget);

			Widget->TextureAtlas = WidgetAtlas;
			Widget->TextureAtlas->Atlas.AtlasBrush.TintColor = Tint;
			Widget->Size = Size;

			FinishCreating(InRadiusFactor);

			Widget->SynchronizeProperties();
		}
	};
}
#endif // 0
