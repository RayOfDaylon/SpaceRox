// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

#pragma once

#include "LocalUtils.h"
#include "Runtime/GeometryCore/Public/Intersection/IntrTriangle2Triangle2.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Algo/Reverse.h"


static UE::Geometry::TIntrTriangle2Triangle2<double> TriTriIntersector;



bool DoesLineSegmentIntersectTriangle(const FVector2D& P1, const FVector2D& P2, const FVector2D Triangle[3])
{
	// Use a degenerate triangle to mimic the line segment.

	UE::Geometry::FTriangle2d Tri0(P1, P2, P2);
	TriTriIntersector.SetTriangle0(Tri0);

	UE::Geometry::FTriangle2d Tri1(Triangle);
	TriTriIntersector.SetTriangle1(Tri1);

	return TriTriIntersector.Test();
}


bool DoesTriangleIntersectTriangle(const FVector2D TriA[3], const FVector2D TriB[3])
{
	UE::Geometry::FTriangle2d Tri0(TriA);
	TriTriIntersector.SetTriangle0(Tri0);

	UE::Geometry::FTriangle2d Tri1(TriB);
	TriTriIntersector.SetTriangle1(Tri1);

	return TriTriIntersector.Test();
}


FVector2D Rotate(const FVector2D& P, float Angle)
{
	// Rotate P around the origin for Angle degrees.

	const auto Theta = FMath::DegreesToRadians(Angle);

	return FVector2D(P.X * cos(Theta) - P.Y * sin(Theta), P.Y * cos(Theta) + P.X * sin(Theta));
}



#if 0
static TArray<FVector2D> LineIntersectsCircle(const FVector2D& p1, const FVector2D& p2, const FVector2D& cp, double r, bool segment)
{
	// Test if a line intersects a circle.
	// Return 0, 1, or 2 points.
	// p1 and p2 are the line endpoints.
	// cp is the circle center.
	// r is the circle radius.
	// segment is true if the line is a finite segment.

	TArray<FVector2D> res;

	auto x0 = cp.X;
	auto y0 = cp.Y;
	auto x1 = p1.X;
	auto y1 = p1.Y;
	auto x2 = p2.X;
	auto y2 = p2.Y;
	auto A = y2 - y1;
	auto B = x1 - x2;
	auto C = x2 * y1 - x1 * y2;
	auto a = sq(A) + sq(B);
	double b, c;
	bool bnz = true;

	if (abs(B) >= eps)
	{
		b = 2 * (A * C + A * B * y0 - sq(B) * x0);
		c = sq(C) + 2 * B * C * y0 - sq(B) * (sq(r) - sq(x0) - sq(y0));
	}
	else
	{
		b = 2 * (B * C + A * B * x0 - sq(A) * y0);
		c = sq(C) + 2 * A * C * x0 - sq(A) * (sq(r) - sq(x0) - sq(y0));
		bnz = false;
	}

	auto d = sq(b) - 4 * a * c; // discriminant

	if (d < 0)
	{
		return res;
	}

	// checks whether a point is within a segment
	auto within = [x1, y1, x2, y2](double x, double y)
	{
		auto d1 = sqrt(sq(x2 - x1) + sq(y2 - y1));  // distance between end-points
		auto d2 = sqrt(sq(x - x1) + sq(y - y1));    // distance from point to one end
		auto d3 = sqrt(sq(x2 - x) + sq(y2 - y));    // distance from point to other end
		auto delta = d1 - d2 - d3;
		return abs(delta) < eps;                    // true if delta is less than a small tolerance
	};

	auto fx = [A, B, C](double x)
	{
		return -(A * x + C) / B;
	};

	auto fy = [A, B, C](double y)
	{
		return -(B * y + C) / A;
	};

	auto rxy = [segment, &res, within](double x, double y)
	{
		if (!segment || within(x, y))
		{
			res.Add(FVector2D(x, y));
		}
	};

	double x, y;
	if (d == 0.0)
	{
		// line is tangent to circle, so just one intersect at most
		if (bnz)
		{
			x = -b / (2 * a);
			y = fx(x);
			rxy(x, y);
		}
		else
		{
			y = -b / (2 * a);
			x = fy(y);
			rxy(x, y);
		}
	}
	else
	{
		// two intersects at most
		d = sqrt(d);
		if (bnz)
		{
			x = (-b + d) / (2 * a);
			y = fx(x);
			rxy(x, y);
			x = (-b - d) / (2 * a);
			y = fx(x);
			rxy(x, y);
		}
		else
		{
			y = (-b + d) / (2 * a);
			x = fy(y);
			rxy(x, y);
			y = (-b - d) / (2 * a);
			x = fy(y);
			rxy(x, y);
		}
	}

	return res;
}


static bool DoesLineSegmentIntersectCircle(const FVector2D& p1, const FVector2D& p2, const FVector2D& cp, double r)
{
	return (LineIntersectsCircle(p1, p2, cp, r, true).Num() != 0);
}
#endif // 0


bool DoesLineSegmentIntersectCircle(const FVector2D& P1, const FVector2D& P2, const FVector2D& CP, double R)
{
	// Test if a line intersects a circle.
	// p1 and p2 are the line endpoints.
	// cp is the circle center.
	// r is the circle radius.

	if(P1 == P2)
	{
		return (FVector2D::Distance(P1, CP) < R);
	}

	const auto x0 = CP.X;
	const auto y0 = CP.Y;
	const auto x1 = P1.X;
	const auto y1 = P1.Y;
	const auto x2 = P2.X;
	const auto y2 = P2.Y;
	const auto A = y2 - y1;
	const auto B = x1 - x2;
	const auto C = x2 * y1 - x1 * y2;
	const auto a = Square(A) + Square(B);

	double b, c;

	bool bnz = true;

	if (abs(B) >= Epsilon)
	{
		b = 2 * (A * C + A * B * y0 - Square(B) * x0);
		c = Square(C) + 2 * B * C * y0 - Square(B) * (Square(R) - Square(x0) - Square(y0));
	}
	else
	{
		b = 2 * (B * C + A * B * x0 - Square(A) * y0);
		c = Square(C) + 2 * A * C * x0 - Square(A) * (Square(R) - Square(x0) - Square(y0));
		bnz = false;
	}

	auto d = Square(b) - 4 * a * c; // discriminant

	if (d < 0)
	{
		return false;
	}

	// checks whether a point is within a segment
	auto within = [x1, y1, x2, y2](double x, double y)
	{
		auto d1 = sqrt(Square(x2 - x1) + Square(y2 - y1));  // distance between end-points
		auto d2 = sqrt(Square(x - x1) + Square(y - y1));    // distance from point to one end
		auto d3 = sqrt(Square(x2 - x) + Square(y2 - y));    // distance from point to other end
		auto delta = d1 - d2 - d3;
		return abs(delta) < Epsilon;                // true if delta is less than a small tolerance
	};

	auto fx = [A, B, C](double x)
	{
		return -(A * x + C) / B;
	};

	auto fy = [A, B, C](double y)
	{
		return -(B * y + C) / A;
	};

	bool res = false;

	double x, y;

	if (d == 0.0)
	{
		// line is tangent to circle, so just one intersect at most
		if (bnz)
		{
			x = -b / (2 * a);
			y = fx(x);
			res = (within(x, y));
		}
		else
		{
			y = -b / (2 * a);
			x = fy(y);
			res = (within(x, y));
		}
	}
	else
	{
		// two intersects at most
		d = sqrt(d);
		if (bnz)
		{
			x = (-b + d) / (2 * a);
			y = fx(x);
			if (within(x, y)) return true;
			x = (-b - d) / (2 * a);
			y = fx(x);
			res = (within(x, y));
		}
		else
		{
			y = (-b + d) / (2 * a);
			x = fy(y);
			if (within(x, y)) return true;
			y = (-b - d) / (2 * a);
			x = fy(y);
			res = (within(x, y));
		}
	}

	return res;
}




FVector2D RandVector2D()
{
	FVector2D Result;
	FVector::FReal L;

	do
	{
		// Check random vectors in the unit sphere so result is statistically uniform.
		Result.X = FMath::FRand() * 2.f - 1.f;
		Result.Y = FMath::FRand() * 2.f - 1.f;
		L = Result.SizeSquared();
	} while (L > 1.0f || L < UE_KINDA_SMALL_NUMBER);

	return Result * (1.0f / FMath::Sqrt(L));
}


FVector2D AngleToVector2D(float Angle)
{
	// We place zero degrees pointing up and increasing clockwise.

	Angle = UKismetMathLibrary::DegreesToRadians(Angle - 90.0f);

	return FVector2D(UKismetMathLibrary::Cos(Angle), UKismetMathLibrary::Sin(Angle));
}


float Vector2DToAngle(const FVector2D& Vector)
{
	// We place zero degrees pointing up and increasing clockwise.

	//const auto V = Vector;
	//auto Len = V.Length();
	//V /= Len;

	return FMath::Atan2(Vector.Y, Vector.X) * 180 / PI + 90;
}


float WrapAngle(float Angle)
{
	return (float)UKismetMathLibrary::GenericPercent_FloatFloat(Angle, 360.0);
}


FVector2D GetWidgetDirectionVector(const UWidget* Widget)
{
	if (Widget == nullptr)
	{
		return FVector2D(0.0f);
	}

	return AngleToVector2D(Widget->GetRenderTransformAngle());
}


void Show(UWidget* Widget, bool Visible)
{
	if(Widget == nullptr)
	{
		return;
	}

	Widget->SetVisibility(Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}


void Hide(UWidget* Widget) 
{
	Show(Widget, false); 
}


void FLoopedSound::Start()
{
	TimeRemaining = 0.0f;
	Tick(0.0f);
}


void FLoopedSound::Tick(float DeltaTime)
{
	TimeRemaining -= DeltaTime;

	if(Sound == nullptr || WorldContextPtr == nullptr)
	{
		//UE_LOG(LogGame, Error, TEXT("Looped sound has no sound or context"));
		return;
	}

	if(TimeRemaining <= 0.0f)
	{
		TimeRemaining = Sound->GetDuration();

		UGameplayStatics::PlaySound2D(WorldContextPtr, Sound, VolumeScale);
	}
}



int32 FHighScoreTable::GetLongestNameLength() const
{
	int32 Len = 0;

	for(const auto& Entry : Entries)
	{
		Len = FMath::Max(Len, Entry.Name.Len());
	}
	return Len;
}


bool FHighScoreTable::CanAdd(int32 Score) const 
{
	if(Entries.Num() < MaxEntries)
	{
		return true;
	}

	// We're full, but see if the score beats an existing entry.

	for(const auto& Entry : Entries)
	{
		if(Score > Entry.Score)
		{
			return true;
		}
	}

	return false;
}


void FHighScoreTable::Add(int32 Score, const FString& Name)
{
	if(!CanAdd(Score))
	{
		return;
	}

	Entries.Add(FHighScore(Score, Name));

	Entries.Sort();
	Algo::Reverse(Entries);

	// Cull any entries past the max allowable.
	while(Entries.Num() > MaxEntries)
	{
		Entries.Pop();
	}
}