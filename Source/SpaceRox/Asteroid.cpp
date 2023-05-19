#include "Asteroid.h"
#include "Constants.h"
#include "PlayViewBase.h"


TSharedPtr<FAsteroid> FAsteroid::Create(UDaylonSpriteWidgetAtlas* Atlas)
{
	auto Widget = SNew(FAsteroid);

	Daylon::FinishCreating<SDaylonSprite>(Widget, 0.5f);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetSize(Atlas->Atlas.AtlasBrush.GetImageSize() / 2);
	Widget->UpdateWidgetSize();
	Widget->SetCurrentCel(FMath::RandRange(0, Atlas->Atlas.NumCels - 1));

	return Widget;
}


bool FAsteroid::HasPowerup() const 
{
	return (Powerup && Powerup->Kind != EPowerup::Nothing); 
}


TSharedPtr<FAsteroid> FAsteroid::Split(UPlayViewBase& Arena)
{
	// Apparently we always split into two children.
	// For efficiency, we reformulate the parent rock into one of the kids.
	// For the new inertias, we want the kids to generally be faster but 
	// once in a while, one of the kids can be slower.

	UDaylonSpriteWidgetAtlas* NewAsteroidAtlas = nullptr;

	switch(Value)
	{
		case ValueBigAsteroid:
			NewAsteroidAtlas = Arena.MediumRockAtlas;
			Value = ValueMediumAsteroid;
			SetAtlas(Arena.MediumRockAtlas->Atlas);
			break;

		case ValueMediumAsteroid:
			NewAsteroidAtlas = Arena.SmallRockAtlas;
			Value = ValueSmallAsteroid;
			SetAtlas(Arena.SmallRockAtlas->Atlas);
			break;
	}

	SetSize(NewAsteroidAtlas->Atlas.GetCelPixelSize());
	UpdateWidgetSize();

	SetCurrentCel(FMath::RandRange(0, NewAsteroidAtlas->Atlas.NumCels - 1));


	auto NewAsteroidPtr = FAsteroid::Create(NewAsteroidAtlas);
	auto& NewAsteroid   = *NewAsteroidPtr.Get();

	NewAsteroid.Value = Value;


	const bool BothKidsFast = FMath::RandRange(0, 10) < 9;

	NewAsteroid.Inertia = Arena.MakeInertia(Inertia, MinAsteroidSplitAngle, MaxAsteroidSplitAngle);
	NewAsteroid.Inertia *= FMath::RandRange(1.2f, 3.0f);

	NewAsteroid.LifeRemaining = 1.0f;
	NewAsteroid.SpinSpeed     = SpinSpeed * AsteroidSpinScale;// FMath::RandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);

	Inertia = Arena.MakeInertia(Inertia, -MinAsteroidSplitAngle, -MaxAsteroidSplitAngle);
	Inertia *= (BothKidsFast ? FMath::RandRange(1.2f, 3.0f) : FMath::RandRange(0.25f, 1.0f));

	SpinSpeed *= AsteroidSpinScale;

	NewAsteroid.Show();

	NewAsteroid.OldPosition = 
	NewAsteroid.UnwrappedNewPosition = UnwrappedNewPosition;
	NewAsteroid.SetPosition(NewAsteroid.UnwrappedNewPosition);

	return NewAsteroidPtr;
}
