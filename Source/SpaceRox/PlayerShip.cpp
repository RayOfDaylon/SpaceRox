#include "PlayerShip.h"
#include "Arena.h"
#include "Torpedo.h"
#include "Constants.h"
#include "DaylonAudio.h"
#include "DaylonGeometry.h"


TSharedPtr<FPlayerShip> FPlayerShip::Create(const FDaylonSpriteAtlas& Atlas, const FVector2D& S, float RadiusFactor)
{
	auto Widget = SNew(FPlayerShip);

	Daylon::Install<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas);
	Widget->SetSize(S);
	Widget->UpdateWidgetSize();

	return Widget;
}


void FPlayerShip::ReleaseResources()
{
	if(Shield)
	{
		Daylon::Uninstall(Shield);
		Shield.Reset();
	}

	if(InvincibilityShield)
	{
		Daylon::Uninstall(InvincibilityShield);
		InvincibilityShield.Reset();
	}
}


void FPlayerShip::Initialize(IArena* InArena)
{
	check(InArena);

	Arena              = InArena;
	IsUnderThrust      = false;
	IsSpawning         = false;
	DoubleShotsLeft    = Arena->IsGodModeActive() ? 10000 : 0;
	ShieldsLeft        = 0.0f;
	InvincibilityLeft  = 0.0f;

	Start  (ViewportSize / 2, FVector2D(0), 1.0f);
	Hide   ();
}


void FPlayerShip::InitializeDefenses()
{
	Shield = SNew(Daylon::SpritePlayObject2D);

	Daylon::Install<SDaylonSprite>(Shield, 0.5f);

	Shield->SetAtlas(Arena->GetDefensesAtlas());
	Shield->SetCurrentCel(ShieldDefenseAtlasCel);
	Shield->SetSize(Arena->GetDefensesAtlas().GetCelPixelSize());
	Shield->UpdateWidgetSize();

	Shield->Start  (ViewportSize / 2, FVector2D(0), 1.0f);
	Shield->Hide   ();


	InvincibilityShield = SNew(Daylon::SpritePlayObject2D);

	Daylon::Install<SDaylonSprite>(InvincibilityShield, 0.5f);

	InvincibilityShield->SetAtlas(Arena->GetDefensesAtlas());
	InvincibilityShield->SetCurrentCel(InvincibilityDefenseAtlasCel);
	InvincibilityShield->SetSize(Arena->GetDefensesAtlas().GetCelPixelSize());
	InvincibilityShield->UpdateWidgetSize();
	InvincibilityShield->Start(ViewportSize / 2, FVector2D(0), 1.0f);
	InvincibilityShield->Hide();
}



void FPlayerShip::Perform(float DeltaTime)
{
	// Update rotation.

	const float Amt = PlayerRotationSpeed * DeltaTime;

	//UE_LOG(LogGame, Log, TEXT("Rotation force: %.5f"), Arena->RotationForce);

	SetAngle(Daylon::WrapAngle(GetAngle() + Amt * Arena->GetPlayerRotationForce()));


	// Change sprite cel only if the thrust state actually changed.

	const bool bThrustStateChanged = (IsUnderThrust != Arena->IsPlayerThrustActive());

	IsUnderThrust = Arena->IsPlayerThrustActive();

	if(Arena->IsPlayerThrustActive())
	{
		if(bThrustStateChanged)
		{
			SetCurrentCel(PlayerShipThrustingAtlasCel);
			Arena->GetPlayerShipThrustSoundLoop().Start();
		}
		else
		{
			// Still thrusting.
			Arena->GetPlayerShipThrustSoundLoop().Tick(DeltaTime);
		}

		const float Thrust = PlayerThrustForce * DeltaTime;

		const FVector2D Force = GetDirectionVector() * Thrust;

		Inertia += Force;

		// Limit speed to avoid breaking collision detector.

		if(Inertia.Length() > MaxPlayerShipSpeed)
		{
			Inertia.Normalize();
			Inertia *= MaxPlayerShipSpeed;
		}
	}
	else
	{
		if(bThrustStateChanged)
		{
			SetCurrentCel(PlayerShipNormalAtlasCel);
		}
	}

	Move(DeltaTime, Arena->GetWrapPositionFunction());


	// Update shield power levels.

	Shield->Show(Arena->IsPlayerShieldActive() && ShieldsLeft > 0.0f);

	if(Shield->IsVisible())
	{
		// We have to budge the shield texture by two px to look nicely centered around the player ship.
		Shield->SetPosition(GetPosition() + Daylon::Rotate(FVector2D(0, 2), GetAngle()));
		AdjustShieldsLeft(-DeltaTime);
	}


	if(InvincibilityLeft <= 0.0f)
	{
		InvincibilityShield->Hide();
	}
	else if(InvincibilityLeft <= WarnWhenInvincibilityGoingAway)
	{
		// Flash the invincibility shield to indicate that it has only a few seconds of power left.
		TimeUntilNextInvincibilityWarnFlash -= DeltaTime;

		if(TimeUntilNextInvincibilityWarnFlash <= 0.0f)
		{
			TimeUntilNextInvincibilityWarnFlash = MaxInvincibilityWarnTime;	
		}
		InvincibilityShield->Show(TimeUntilNextInvincibilityWarnFlash < MaxInvincibilityWarnTime * 0.5f);
	}
	else
	{
		InvincibilityShield->Show();
	}


	if(InvincibilityShield->IsVisible())
	{
		InvincibilityShield->SetAngle(GetAngle());
		InvincibilityShield->SetPosition(GetPosition() + Daylon::Rotate(FVector2D(0, -2), GetAngle()));
	}

	if(InvincibilityLeft > 0.0f)
	{
		AdjustInvincibilityLeft(-DeltaTime);
	}
}


void FPlayerShip::SpawnExplosion()
{
	const auto P           = GetPosition();
	const auto ShipInertia = Inertia * 0; // Don't use any inertia 

	Arena->GetExplosions().SpawnOne(P, PlayerShipFirstExplosionParams, ShipInertia);

	Arena->ScheduleExplosion(PlayerShipSecondExplosionDelay, P, ShipInertia, PlayerShipSecondExplosionParams);
}


bool FPlayerShip::ProcessCollision(float MassOther, const FVector2D* PositionOther, FVector2D* InertiaOther)
{
	// Return true if we didn't get destroyed.

	bool StillAlive = false;

	if(Arena->IsGodModeActive() || InvincibilityLeft > 0.0f) // todo: reduce invincibility the same way shields are
	{
		StillAlive = true;
	}
	else if(Shield->IsVisible())
	{
		AdjustShieldsLeft(-ShieldBonkDamage);
		StillAlive = true;
	}

	if(StillAlive)
	{
		Arena->PlaySound(Arena->GetShieldBonkSound(), 0.5f);

		if(MassOther != 0.0f)
		{
			check(PositionOther != nullptr);
			check(InertiaOther != nullptr);

			// Don't let the inertia of the other object be modified, it causes medium and large asteroids
			// to break up into smaller rocks that move too fast.
			// todo: that isn't a problem this function should be worried about, so we need 
			// to move the responsibility elsewhere.

			auto TmpInertiaOther = *InertiaOther;

			Daylon::ComputeCollisionInertia(
				PlayerShipMass, 
				MassOther, 
				1.0, // restitution coefficient, 1 = perfectly elastic collision
				GetPosition(),
				*PositionOther,
				Inertia,
				TmpInertiaOther);
		}
	}

	return StillAlive;
}


void FPlayerShip::AdjustShieldsLeft(float Amount)
{
	ShieldsLeft = FMath::Max(0.0f, ShieldsLeft + Amount);
}


void FPlayerShip::AdjustInvincibilityLeft(float Amount)
{
	InvincibilityLeft = FMath::Max(0.0f, InvincibilityLeft + Amount);
}


void FPlayerShip::AdjustDoubleShotsLeft(int32 Amount)
{
	DoubleShotsLeft = FMath::Max(0, DoubleShotsLeft + Amount);
}


void FPlayerShip::FireTorpedo()
{
	const FVector2D PlayerFwd = GetDirectionVector();

	const auto TorpedoInertia = (PlayerFwd * MaxTorpedoSpeed) + Inertia;

	// Position torpedo at nose of player ship.


	auto TorpedoPtr = Arena->GetAvailableTorpedo();

	if(!TorpedoPtr)
	{
		return;
	}

	auto& Torpedo = *TorpedoPtr.Get();


	Arena->PlaySound(Arena->GetTorpedoSound());

    if(DoubleShotsLeft == 0)
	{
		// Find an available torpedo, spawn it at the nose of the player ship,
		// and give it an inertia which is player ship intertia + player ship fwd * MaxTorpedoSpeed

		//auto& Torpedo = *Arena->Torpedos[TorpedoIndex].Get();

		Torpedo.FiredByPlayer = true;

		auto P = GetPosition();
		P += PlayerFwd * (GetSize().Y / 2 + /*Inertia.Length()*/ 2.0); // The last offset is so that the bullet doesn't start off accidentally overlapping the player ship
		P = Arena->WrapPosition(P);

		Torpedo.Start(P, TorpedoInertia, MaxTorpedoLifeTime);
	}
	else
	{
		AdjustDoubleShotsLeft(-1);

		Torpedo.FiredByPlayer = true;

		auto P = PlayerFwd * (GetSize().Y / 4);// * Daylon::FRandRange(0.5f, 2.0f);
		P = Daylon::Rotate(P, 90.0f);
		P += GetPosition();
		//P += PlayerFwd * Daylon::FRandRange(0.0f, 10.0f);
		P = Arena->WrapPosition(P);

		Torpedo.Start(P, TorpedoInertia, MaxTorpedoLifeTime);

		auto TorpedoPtr2 = Arena->GetAvailableTorpedo();

		if(!TorpedoPtr2)
		{
			return;
		}

		auto& Torpedo2 = *TorpedoPtr2.Get();

		Torpedo2.FiredByPlayer = true;

		P = PlayerFwd * (GetSize().Y / 4);// * Daylon::FRandRange(0.5f, 2.0f);
		P = Daylon::Rotate(P, -90.0f);
		P += GetPosition();
		//P += PlayerFwd * Daylon::FRandRange(0.0f, 10.0f);
		P = Arena->WrapPosition(P);

		Torpedo2.Start(P, TorpedoInertia, MaxTorpedoLifeTime);
	}
}

