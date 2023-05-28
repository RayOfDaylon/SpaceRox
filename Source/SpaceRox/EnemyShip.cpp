// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "EnemyShip.h"
#include "DaylonUtils.h"
#include "PlayViewBase.h"
#include "Constants.h"


// Set to 1 to enable debugging
#define DEBUG_MODULE                0


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



TSharedPtr<FEnemyShip> FEnemyShip::Create(UDaylonSpriteWidgetAtlas* Atlas, int Value, float RadiusFactor)
{
	auto Widget = SNew(FEnemyShip);

	Daylon::Install<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetCurrentCel(0);
	Widget->SetSize(Atlas->Atlas.GetCelPixelSize());
	Widget->UpdateWidgetSize();

	Widget->Value = Value;

	return Widget;
}


FEnemyShip::FEnemyShip()
{
	TimeRemainingToNextShot = (Value == ValueBigEnemy ? BigEnemyReloadTime : SmallEnemyReloadTime);
	TimeRemainingToNextMove = 3.0f;
}


void FEnemyShip::Perform(UPlayViewBase& Arena, float DeltaTime)
{
	// Don't call Super::Update(DeltaTime) because we are a static sprite.

	const bool WereBig = (Value == ValueBigEnemy);

	Move(DeltaTime, Arena.WrapPositionToViewport);


	// Fire a torpedo if we've finished reloading.

	TimeRemainingToNextShot -= DeltaTime;

	if(TimeRemainingToNextShot <= 0.0f)
	{
		TimeRemainingToNextShot = (WereBig ? BigEnemyReloadTime : SmallEnemyReloadTime);
		Shoot(Arena);
	}

	TimeRemainingToNextMove -= DeltaTime;

	if(TimeRemainingToNextMove <= 0.0f)
	{
		TimeRemainingToNextMove = 3.0f; // todo: use global constant instead of 3.0

		// Change heading (or stay on current heading).

		const FVector2D Headings[] = 
		{
			{ 1, 0 },
			{ 1, 1 },
			{ 1, -1 }
		};

		FVector2D NewHeading = Headings[FMath::RandRange(0, 2)];

		NewHeading.Normalize();

		const auto Facing = FMath::Sign(Inertia.X);
		Inertia = NewHeading * Inertia.Length();
		Inertia.X *= Facing;
	}
}



void FEnemyShip::Shoot(UPlayViewBase& Arena)
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!Arena.IsPlayerPresent())
	{
		return;
	}

	const int32 TorpedoIdx = Arena.GetAvailableTorpedo();

	if(TorpedoIdx == INDEX_NONE)
	{
		return;
	}

	auto& Torpedo = *Arena.Torpedos[TorpedoIdx].Get();

	Torpedo.FiredByPlayer = false;

	FVector2D Direction;
	float     Speed;

	// Position torpedo a little outside the ship.
	auto LaunchP = OldPosition;

	const bool WereBig = (Value == ValueBigEnemy);

	if(WereBig)
	{
		// Shot vector is random.
		Direction = UDaylonUtils::RandVector2D();
		LaunchP += Direction * (GetSize().X / 2 + 2);
		LaunchP = Arena.WrapPositionToViewport(LaunchP);

		Speed = BigEnemyTorpedoSpeed;
	}
	else
	{
		// Small enemy.
		// Shots switch between shooting at an available asteroid or the player.

		Speed = SmallEnemyTorpedoSpeed;

		FVector2D TargetP(-1);
		//const float Time = FMath::RandRange(0.75f, 1.25f);


		bShootAtPlayer = !bShootAtPlayer;

		if(bShootAtPlayer || Arena.Asteroids.IsEmpty())
		{
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Arena.PlayerShip->GetPosition(), Arena.PlayerShip->Inertia);
		}
		else
		{
			// Shoot at an asteroid.
			const auto& Asteroid = Arena.Asteroids.Get(FMath::RandRange(0, Arena.Asteroids.Num() - 1));
			Direction = UDaylonUtils::ComputeFiringSolution(LaunchP, Speed, Asteroid.GetPosition(), Asteroid.Inertia);
		}
	}

	Torpedo.Spawn(LaunchP, Direction * Speed, MaxTorpedoLifeTime);

	Arena.PlaySound(Arena.TorpedoSound);
}


// ------------------------------------------------------------------------------------------------------------------


TSharedPtr<FEnemyBoss> FEnemyBoss::Create(UDaylonSpriteWidgetAtlas* Atlas, float S, int32 Value, int32 NumShields, float SpinSpeed)
{
	check(Atlas);
	check(NumShields > 0);
	check(SpinSpeed >= 0.0f);

	auto Widget = SNew(FEnemyBoss);

	Widget->Value = Value;

	auto SlotSprite = Widget->AddSlot();
	Widget->SpriteSlot = SlotSprite.GetSlot();
	Widget->SpriteSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	Widget->SpriteSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

	SlotSprite[SAssignNew(Widget->Sprite, SDaylonSprite).Size(S)];

	Widget->Sprite->SetAtlas(Atlas->Atlas);
	Daylon::Install<SOverlay>(Widget, 0.5f);

	Widget->NumShields = NumShields;
		

	bool ShieldSpinDir = FMath::RandBool();

	for(int32 Index = 0; Index < NumShields; Index++, ShieldSpinDir = !ShieldSpinDir)
	{
		auto ShieldSlot = Widget->AddSlot();
		Widget->ShieldSlots.Add(ShieldSlot.GetSlot());

		TSharedPtr<SDaylonPolyShield> Tmp;
		ShieldSlot[SAssignNew(Tmp, SDaylonPolyShield).Size(60 + Index * 30).NumSides(6 + Index * 3).Thickness(3).SpinSpeed(SpinSpeed * (ShieldSpinDir ? 1 : -1))];
		Widget->Shields.Add(Tmp);
	}

	//auto SlotOuterShield = Widget->AddSlot();
	//Widget->SlotOuterShieldPtr = SlotOuterShield.GetSlot();
	//SlotOuterShield[SAssignNew(Widget->OuterShield, SDaylonPolyShield).Size(90).NumSides(9).Thickness(3).SpinSpeed(100)];

	/*
	StaticCastSharedRef<SOverlay>(Widget)
	+ SOverlay::Slot()
	[
		SNew(SDaylonSprite)
	]
	+ SOverlay::Slot()
	[
		SNew(SDaylonPolyShield).Size(60)
	]
	;
	*/
	return Widget;
}


void FEnemyBoss::Update(float DeltaTime)
{
	for(auto ShieldPtr : Shields)
	{
		ShieldPtr->Update(DeltaTime);
	}

	Sprite->Update(DeltaTime);
}


int32 FEnemyBoss::CheckCollision(const FVector2D& P1, const FVector2D &P2, int32& ShieldSegmentIndex) const
{
	// P1 and P2 are in scene space.
	// Return INDEX_NONE if no part got hit.
	// Return 0 if center sprite was hit
	// Return 1 ... NumShields if a shield got hit, from center out,
	// and put shield segment index into ShieldSegmentIndex.

	const auto LocalP1 = P1 - GetPosition();
	const auto LocalP2 = P2 - GetPosition();

	// Check center first.
	if(UDaylonUtils::DoesLineSegmentIntersectCircle(LocalP1, LocalP2, FVector2D(0), Sprite->GetSize().X / 2))
	{
		return 0;
	}

	// Check shields, starting with the inner one.
	int32 ShieldIndex = 1;

	for(auto ShieldPtr : Shields)
	{
		ShieldSegmentIndex = ShieldPtr->GetHitSegment(LocalP1, LocalP2);

		if(ShieldSegmentIndex != INDEX_NONE)
		{
			return ShieldIndex;
		}

		ShieldIndex++;
	}

	return INDEX_NONE;
}


int32 FEnemyBoss::CheckCollision(const FVector2D& P1, const FVector2D &P2, float Radius, int32& ShieldSegmentIndex, FVector2D& HitPt) const
{
	// P1 and P2 are in scene space.
	// Radius is the radius of the object that may have hit us.
	// Return INDEX_NONE if no part got hit.
	// Return 0 if center sprite was hit
	// Return 1 ... NumShields if a shield got hit, from center out,
	// and put shield segment index into ShieldSegmentIndex.
	// Return specific collision point in HitPt.

	const auto LocalP1 = P1 - GetPosition();
	const auto LocalP2 = P2 - GetPosition();

	// Check if center was hit.

	auto LocalAvgP = (LocalP1 + LocalP2) / 2;

	// For now, use the average of P1, P2 as the hit point.
	HitPt = (P1 + P2) / 2;

	if(UDaylonUtils::DoCirclesIntersect(LocalAvgP, Radius, FVector2D(0), GetRadius()))
	{
		return 0;
	}

	// Check shields, starting with the inner one.

	int32 ShieldIndex = 1;

	for(auto ShieldPtr : Shields)
	{
		if(UDaylonUtils::DoCirclesIntersect(LocalAvgP, Radius, FVector2D(0), ShieldPtr->GetSize().X / 2))
		{
			// Determine the segment by casting a ray from our center.
			ShieldSegmentIndex = ShieldPtr->GetHitSegment(FVector2D(0), LocalAvgP * 10.0f);

			if(ShieldSegmentIndex != INDEX_NONE)
			{
				return ShieldIndex;
			}
		}

		ShieldIndex++;
	}

	return INDEX_NONE;
}


float FEnemyBoss::GetShieldSegmentHealth(int32 ShieldNumber, int32 SegmentIndex) const
{
	const auto ShieldIndex = ShieldNumber - 1;

	if(!Shields.IsValidIndex(ShieldIndex))
	{
		return 0.0f;
	}

	return Shields[ShieldIndex]->GetSegmentHealth(SegmentIndex);
}


void FEnemyBoss::SetShieldSegmentHealth(int32 ShieldNumber, int32 SegmentIndex, float Health)
{
	const auto ShieldIndex = ShieldNumber - 1;

	if(!Shields.IsValidIndex(ShieldIndex))
	{
		return;
	}

	Shields[ShieldIndex]->SetSegmentHealth(SegmentIndex, Health);
}


void FEnemyBoss::SpawnExplosion(UPlayViewBase& Arena)
{
	const auto P = GetPosition();
	const auto ShipInertia = Inertia;

	Arena.Explosions.SpawnOne(Arena, P, 
		4.5f,
		9.0f,
		45.0f,
		240.0f,
		0.5f,
		4.0f,
		0.25f,
		80,
		ShipInertia);
}


void FEnemyBoss::Perform(UPlayViewBase& Arena, float DeltaTime)
{
	TimeRemainingToNextMove -= DeltaTime;

	if(TimeRemainingToNextMove <= 0.0f)
	{
		// Alter heading by some random vector.

		TimeRemainingToNextMove = FMath::FRandRange(2.0f, 3.0f);

		// Make new direction not differ so much from current direction.
		const auto OldAngle = UDaylonUtils::Vector2DToAngle(Inertia);
		const auto NewAngle = OldAngle + FMath::FRandRange(-70.0f, 70.0f);

		Inertia = UDaylonUtils::AngleToVector2D(NewAngle) * GetSpeed();
	}

	Move(DeltaTime, Arena.WrapPositionToViewport);


	TimeRemainingToNextShot -= DeltaTime;

	if(TimeRemainingToNextShot <= 0.0f)
	{
		TimeRemainingToNextShot = FMath::FRandRange(1.0f, 2.0f);
		Shoot(Arena);
	}
}


void FEnemyBoss::Shoot(UPlayViewBase& Arena)
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!Arena.IsPlayerPresent())
	{
		return;
	}

	const int32 TorpedoIdx = Arena.GetAvailableTorpedo();

	if(TorpedoIdx == INDEX_NONE)
	{
		return;
	}

	auto& Torpedo = *Arena.Torpedos[TorpedoIdx].Get();

	Torpedo.FiredByPlayer = false;


	// Interpolate between a random shot and an accurate shot.


	auto DirectionToPlayer = Arena.PlayerShip->GetPosition() - GetPosition();
	DirectionToPlayer.Normalize();
	const auto FiringPoint = Arena.WrapPositionToViewport(GetPosition() + DirectionToPlayer * (Shields.Last(0)->GetSize().X / 2 + 10.0f));

	// The random direction has to be away from us. It can vary by -90 to +90 degrees from DirectionToPlayer.
	const auto AngleToPlayer = UDaylonUtils::Vector2DToAngle(DirectionToPlayer);
	const auto RandomAngle = AngleToPlayer + FMath::FRandRange(-90.0f, 90.0f);

	const auto PerfectDirection = UDaylonUtils::ComputeFiringSolution(FiringPoint, BossTorpedoSpeed, Arena.PlayerShip->GetPosition(), Arena.PlayerShip->Inertia);
	const auto PerfectAngle = UDaylonUtils::Vector2DToAngle(PerfectDirection);

	const float Aim = FMath::Min(1.0f, UDaylonUtils::Normalize(Arena.PlayerScore, ScoreForBossSpawn, ScoreForBossAimPerfect));
	const auto Angle = FMath::Lerp(RandomAngle, PerfectAngle, Aim);

	const auto Direction = UDaylonUtils::AngleToVector2D(Angle);

	Torpedo.Spawn(FiringPoint, Direction * BossTorpedoSpeed, MaxTorpedoLifeTime);

	Arena.PlaySound(Arena.TorpedoSound);
}


#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif
