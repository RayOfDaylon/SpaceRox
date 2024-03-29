#include "Asteroid.h"
#include "Constants.h"
#include "Arena.h"
#include "DaylonRNG.h"


TSharedPtr<FAsteroid> FAsteroid::Spawn(IArena* InArena, const FDaylonSpriteAtlas& Atlas)
{
	check(InArena);

	auto Widget = SNew(FAsteroid);

	Daylon::Install<SDaylonSprite>(Widget, 0.5f);

	Widget->Arena = InArena;
	Widget->SetAtlas(Atlas);
	// We use 4K textures so halve that image size for the sprite widget size in our HD Slate space
	Widget->SetSize(Atlas.AtlasBrush.GetImageSize() / 2);
	Widget->UpdateWidgetSize();
	Widget->SetCurrentCel(Daylon::RandRange(0, Atlas.NumCels - 1));

	return Widget;
}


bool FAsteroid::HasPowerup() const 
{
	return (Powerup && Powerup->Kind != EPowerup::Nothing); 
}


TSharedPtr<FAsteroid> FAsteroid::Split()
{
	// Apparently we always split into two children.
	// For efficiency, we reformulate the parent rock into one of the kids.
	// For the new inertias, we want the kids to generally be faster but 
	// once in a while, one of the kids can be slower.

	const FDaylonSpriteAtlas* NewAsteroidAtlasPtr = nullptr;

	switch(Value)
	{
		case ValueBigAsteroid:
			NewAsteroidAtlasPtr = &Arena->GetMediumAsteroidAtlas();
			Value = ValueMediumAsteroid;
			break;

		case ValueMediumAsteroid:
			NewAsteroidAtlasPtr = &Arena->GetSmallAsteroidAtlas();
			Value = ValueSmallAsteroid;
			break;
	}

	check(NewAsteroidAtlasPtr);
	SetAtlas(*NewAsteroidAtlasPtr);

	SetSize(NewAsteroidAtlasPtr->GetCelPixelSize());
	UpdateWidgetSize();

	SetCurrentCel(Daylon::RandRange(0, NewAsteroidAtlasPtr->NumCels - 1));


	auto NewAsteroidPtr = FAsteroid::Spawn(Arena, *NewAsteroidAtlasPtr);
	auto& NewAsteroid   = *NewAsteroidPtr.Get();

	NewAsteroid.Value = Value;


	const bool BothKidsFast = Daylon::RandRange(0, 10) < 9;

	NewAsteroid.Inertia = Daylon::DeviateVector(Inertia, MinAsteroidSplitAngle, MaxAsteroidSplitAngle);
	NewAsteroid.Inertia *= Daylon::FRandRange(1.2f, 3.0f);

	NewAsteroid.LifeRemaining = 1.0f;
	NewAsteroid.SpinSpeed     = SpinSpeed * AsteroidSpinScale;// Daylon::FRandRange(MinAsteroidSpinSpeed, MaxAsteroidSpinSpeed);

	Inertia = Daylon::DeviateVector(Inertia, -MinAsteroidSplitAngle, -MaxAsteroidSplitAngle);
	Inertia *= (BothKidsFast ? Daylon::FRandRange(1.2f, 3.0f) : Daylon::FRandRange(0.25f, 1.0f));

	SpinSpeed *= AsteroidSpinScale;

	NewAsteroid.Show();

	NewAsteroid.OldPosition = 
	NewAsteroid.UnwrappedNewPosition = UnwrappedNewPosition;
	NewAsteroid.SetPosition(NewAsteroid.UnwrappedNewPosition);

	return NewAsteroidPtr;
}
