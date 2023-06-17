// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "EnemyShip.h"
#include "DaylonUtils.h"
#include "Torpedo.h"
#include "Arena.h"
#include "PlayerShip.h"
#include "Constants.h"


// Set to 1 to enable debugging
#define DEBUG_MODULE                0


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif


static FVector2D GetFiringAngle(float TorpedoSpeed, const FVector2D& P, const FVector2D& TargetP, const FVector2D& TargetInertia, float Quality)
{
	auto DirectionToTarget = TargetP - P;
	DirectionToTarget.Normalize();

	// The random direction has to be away from us. It can vary by -90 to +90 degrees from DirectionToTarget.
	const auto AngleToTarget = Daylon::Vector2DToAngle(DirectionToTarget);
	const auto RandomAngle   = AngleToTarget + FMath::FRandRange(-90.0f, 90.0f);

	const auto PerfectDirection = Daylon::ComputeFiringSolution(P, TorpedoSpeed, TargetP, TargetInertia);
	const auto PerfectAngle     = Daylon::Vector2DToAngle(PerfectDirection);

	return Daylon::AngleToVector2D(FMath::Lerp(RandomAngle, PerfectAngle, Quality));
}


TSharedPtr<FEnemyShip> FEnemyShip::Create(IArena* InArena, const FDaylonSpriteAtlas& Atlas, int Value, float RadiusFactor)
{
	check(InArena);

	auto Widget = SNew(FEnemyShip);

	Daylon::Install<SDaylonSprite>(Widget, RadiusFactor);

	Widget->Arena = InArena;
	Widget->SetAtlas(Atlas);
	Widget->SetCurrentCel(0);
	Widget->SetSize(Atlas.GetCelPixelSize());
	Widget->UpdateWidgetSize();

	Widget->Value = Value;

	return Widget;
}


FEnemyShip::FEnemyShip()
{
	TimeRemainingToNextShot = (Value == ValueBigEnemy ? BigEnemyReloadTime : SmallEnemyReloadTime);
	TimeRemainingToNextMove = 3.0f;
}


void FEnemyShip::Perform(float DeltaTime)
{
	// Don't call Super::Update(DeltaTime) because we are a static sprite.

	const bool WereBig = (Value == ValueBigEnemy);

	Move(DeltaTime, Arena->GetWrapPositionFunction());


	// Fire a torpedo if we've finished reloading.

	TimeRemainingToNextShot -= DeltaTime;

	if(TimeRemainingToNextShot <= 0.0f)
	{
		TimeRemainingToNextShot = (WereBig ? BigEnemyReloadTime : SmallEnemyReloadTime);
		Shoot();
	}

	TimeRemainingToNextMove -= DeltaTime;

	if(TimeRemainingToNextMove <= 0.0f)
	{
		TimeRemainingToNextMove = FMath::FRandRange(MinTimeTilNextEnemyShipMove, MaxTimeTilNextEnemyShipMove);

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


void FEnemyShip::Shoot()
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!Arena->IsPlayerShipPresent())
	{
		return;
	}

	auto TorpedoPtr = Arena->GetAvailableTorpedo();

	if(!TorpedoPtr)
	{
		return;
	}

	auto& Torpedo = *TorpedoPtr.Get();

	Torpedo.FiredByPlayer = false;

	FVector2D Direction;
	float     Speed;

	// Position torpedo a little outside the ship.
	auto LaunchP = OldPosition;

	const bool WereBig = (Value == ValueBigEnemy);

	if(WereBig)
	{
		Speed = BigEnemyTorpedoSpeed;

		const float Aim = FMath::Clamp(Daylon::Normalize(Arena->GetPlayerScore(), ScoreForBigEnemyAimWorst, ScoreForBigEnemyAimPerfect), 0.0f, 1.0f);

		Direction = GetFiringAngle(Speed, LaunchP, Arena->GetPlayerShip().GetPosition(), Arena->GetPlayerShip().Inertia, Aim);

		// Shift the launch point outside the ship. This will screw up perfect aims but they will still be close enough to rattle the player.
		LaunchP += Direction * (GetSize().X / 2 + 2);
		LaunchP = Arena->WrapPosition(LaunchP);
	}
	else
	{
		// Small enemy.
		// Shots switch between shooting at an available asteroid or the player.

		Speed = SmallEnemyTorpedoSpeed;

		FVector2D TargetP(-1);
		//const float Time = FMath::RandRange(0.75f, 1.25f);


		bShootAtPlayer = !bShootAtPlayer;

		if(bShootAtPlayer || Arena->GetAsteroids().IsEmpty())
		{
			const float Aim = FMath::Clamp(Daylon::Normalize(Arena->GetPlayerScore(), ScoreForSmallEnemyAimWorst, ScoreForSmallEnemyAimPerfect), 0.0f, 1.0f);

			Direction = GetFiringAngle(Speed, LaunchP, Arena->GetPlayerShip().GetPosition(), Arena->GetPlayerShip().Inertia, Aim);
		}
		else
		{
			// Shoot at an asteroid.
			const auto& Asteroid = Arena->GetAsteroids().Get(FMath::RandRange(0, Arena->GetAsteroids().Num() - 1));
			Direction = Daylon::ComputeFiringSolution(LaunchP, Speed, Asteroid.GetPosition(), Asteroid.Inertia);
		}
	}

	Torpedo.Spawn(LaunchP, Direction * Speed, MaxTorpedoLifeTime);

	Arena->PlaySound(Arena->GetTorpedoSound());
}


// ------------------------------------------------------------------------------------------------------------------


TSharedPtr<FEnemyBoss> FEnemyBoss::Create(IArena* InArena, const FDaylonSpriteAtlas& Atlas, float S, int32 Value, int32 NumShields, float SpinSpeed)
{
	check(InArena);
	check(NumShields > 0);
	check(SpinSpeed >= 0.0f);

	auto Widget = SNew(FEnemyBoss);

	Widget->Arena = InArena;
	Widget->Value = Value;

	auto SlotSprite = Widget->AddSlot();
	Widget->SpriteSlot = SlotSprite.GetSlot();
	Widget->SpriteSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	Widget->SpriteSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

	SlotSprite[SAssignNew(Widget->Sprite, SDaylonSprite).Size(S)];

	Widget->Sprite->SetAtlas(Atlas);
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
	if(Daylon::DoesLineSegmentIntersectCircle(LocalP1, LocalP2, FVector2D(0), Sprite->GetSize().X / 2))
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

	if(Daylon::DoCirclesIntersect(LocalAvgP, Radius, FVector2D(0), GetRadius()))
	{
		return 0;
	}

	// Check shields, starting with the inner one.

	int32 ShieldIndex = 1;

	for(auto ShieldPtr : Shields)
	{
		if(Daylon::DoCirclesIntersect(LocalAvgP, Radius, FVector2D(0), ShieldPtr->GetSize().X / 2))
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


float FEnemyBoss::GetShieldThickness() const
{
	check(!Shields.IsEmpty());

	return Shields[0]->GetThickness();
}


void FEnemyBoss::GetShieldSegmentGeometry(int32 ShieldNumber, int32 SegmentIndex, FVector2D& P1, FVector2D& P2) const
{
	const auto ShieldIndex = ShieldNumber - 1;

	if(!Shields.IsValidIndex(ShieldIndex))
	{
		return;
	}

	return Shields[ShieldIndex]->GetSegmentGeometry(SegmentIndex, P1, P2);
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


void FEnemyBoss::Perform(float DeltaTime)
{
	TimeRemainingToNextMove -= DeltaTime;

	if(TimeRemainingToNextMove <= 0.0f)
	{
		// Alter heading by some random vector.

		TimeRemainingToNextMove = FMath::FRandRange(2.0f, 3.0f);

		// Make new direction not differ so much from current direction.
		const auto OldAngle = Daylon::Vector2DToAngle(Inertia);
		const auto NewAngle = OldAngle + FMath::FRandRange(-70.0f, 70.0f);

		Inertia = Daylon::AngleToVector2D(NewAngle) * GetSpeed();
	}

	Move(DeltaTime, Arena->GetWrapPositionFunction());


	TimeRemainingToNextShot -= DeltaTime;

	if(TimeRemainingToNextShot <= 0.0f)
	{
		TimeRemainingToNextShot = FMath::FRandRange(1.0f, 2.0f);
		Shoot();
	}
}


void FEnemyBoss::Shoot()
{
	// In Defcon, we had three shooting accuracies: wild, at, and leaded.
	// For now, just use wild and leaded.

	if(!Arena->IsPlayerShipPresent())
	{
		return;
	}

	auto TorpedoPtr = Arena->GetAvailableTorpedo();

	if(!TorpedoPtr)
	{
		return;
	}

	auto& Torpedo = *TorpedoPtr.Get();

	Torpedo.FiredByPlayer = false;


	// Interpolate between a random shot and an accurate shot.

	auto DirectionToPlayer = Arena->GetPlayerShip().GetPosition() - GetPosition();
	DirectionToPlayer.Normalize();
	const auto FiringPoint = Arena->WrapPosition(GetPosition() + DirectionToPlayer * (Shields.Last(0)->GetSize().X / 2 + 10.0f));
	const float Aim = FMath::Min(1.0f, Daylon::Normalize(Arena->GetPlayerScore(), ScoreForBossSpawn, ScoreForBossAimPerfect));

	const auto Direction = GetFiringAngle(BossTorpedoSpeed, FiringPoint, Arena->GetPlayerShip().GetPosition(), Arena->GetPlayerShip().Inertia, Aim);

	Torpedo.Spawn(FiringPoint, Direction * BossTorpedoSpeed, MaxTorpedoLifeTime);

	Arena->PlaySound(Arena->GetTorpedoSound());
}


#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
