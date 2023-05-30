#include "PlayerShip.h"
#include "Arena.h"
#include "Torpedo.h"
#include "Constants.h"


TSharedPtr<FPlayerShip> FPlayerShip::Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor)
{
	auto Widget = SNew(FPlayerShip);

	Daylon::Install<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetSize(S);
	Widget->UpdateWidgetSize();

	return Widget;
}


void FPlayerShip::ReleaseResources(IArena& Arena)
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


void FPlayerShip::Initialize(IArena& Arena)
{
	IsUnderThrust      = false;
	IsSpawning         = false;
	DoubleShotsLeft    = Arena.IsGodModeActive() ? 10000 : 0;
	ShieldsLeft        = 0.0f;
	InvincibilityLeft  = 0.0f;

	Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	Hide   ();
}


void FPlayerShip::InitializeDefenses(IArena& Arena)
{
	Shield = SNew(Daylon::SpritePlayObject2D);

	Daylon::Install<SDaylonSprite>(Shield, 0.5f);

	Shield->SetAtlas(Arena.GetDefensesAtlas().Atlas);
	Shield->SetCurrentCel(ShieldDefenseAtlasCel);
	Shield->SetSize(Arena.GetDefensesAtlas().Atlas.GetCelPixelSize());
	Shield->UpdateWidgetSize();

	Shield->Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	Shield->Hide   ();


	InvincibilityShield = SNew(Daylon::SpritePlayObject2D);

	Daylon::Install<SDaylonSprite>(InvincibilityShield, 0.5f);

	InvincibilityShield->SetAtlas(Arena.GetDefensesAtlas().Atlas);
	InvincibilityShield->SetCurrentCel(InvincibilityDefenseAtlasCel);
	InvincibilityShield->SetSize(Arena.GetDefensesAtlas().Atlas.GetCelPixelSize());
	InvincibilityShield->UpdateWidgetSize();
	InvincibilityShield->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);
	InvincibilityShield->Hide();
}



void FPlayerShip::Perform(IArena& Arena, float DeltaTime)
{
	// Update rotation.

	const float Amt = PlayerRotationSpeed * DeltaTime;

	//UE_LOG(LogGame, Log, TEXT("Rotation force: %.5f"), Arena.RotationForce);

	SetAngle(UDaylonUtils::WrapAngle(GetAngle() + Amt * Arena.GetRotationForce()));


	// Change sprite cel only if the thrust state actually changed.

	const bool bThrustStateChanged = (IsUnderThrust != Arena.IsThrustActive());

	IsUnderThrust = Arena.IsThrustActive();

	if(Arena.IsThrustActive())
	{
		if(bThrustStateChanged)
		{
			SetCurrentCel(PlayerShipThrustingAtlasCel);
			Arena.GetPlayerShipThrustSoundLoop().Start();
		}
		else
		{
			// Still thrusting.
			Arena.GetPlayerShipThrustSoundLoop().Tick(DeltaTime);
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

	Move(DeltaTime, Arena.GetWrapPositionFunction());


	// Update shield power levels.

	Shield->Show(Arena.IsShieldActive() && ShieldsLeft > 0.0f);

	if(Shield->IsVisible())
	{
		// We have to budge the shield texture by two px to look nicely centered around the player ship.
		Shield->SetPosition(GetPosition() + UDaylonUtils::Rotate(FVector2D(0, 2), GetAngle()));
		AdjustShieldsLeft(-DeltaTime);
	}


	if(InvincibilityLeft <= 0.0f)
	{
		InvincibilityShield->Hide();
	}
	else if(InvincibilityLeft <= 5.0f) // todo: use constant
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
		InvincibilityShield->SetPosition(GetPosition() + UDaylonUtils::Rotate(FVector2D(0, -2), GetAngle()));
	}

	if(InvincibilityLeft > 0.0f)
	{
		AdjustInvincibilityLeft(-DeltaTime);
	}
}


void FPlayerShip::SpawnExplosion(IArena& Arena)
{
	const auto P           = GetPosition();
	const auto ShipInertia = Inertia * 0; // Don't use any inertia 

	const static FDaylonParticlesParams Params =
	{
		3.0f,
		6.0f,
		30.0f,
		160.0f,
		0.5f,
		3.0f,
		0.25f,
		80
	};

	Arena.GetExplosions().SpawnOne(Arena, P, Params, ShipInertia);

	// Set up second explosion event for 3/4 second later

	const static FDaylonParticlesParams Params2 =
	{
		4.5f,
		9.0f,
		45.0f,
		240.0f,
		0.5f,
		4.0f,
		0.25f,
		80
	};

	Arena.ScheduleExplosion(0.66f, P, ShipInertia, Params2);
}


bool FPlayerShip::ProcessCollision(IArena& Arena)
{
	// Return true if we didn't get destroyed.

	if(Arena.IsGodModeActive() || InvincibilityLeft > 0.0f)
	{
		Arena.PlaySound(Arena.GetShieldBonkSound());
		return true;
	}

	if(Shield->IsVisible())
	{
		AdjustShieldsLeft(-ShieldBonkDamage);
		Arena.PlaySound(Arena.GetShieldBonkSound());
		return true;
	}

	return false;
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


void FPlayerShip::FireTorpedo(IArena& Arena)
{
	const FVector2D PlayerFwd = GetDirectionVector();

	const auto TorpedoInertia = (PlayerFwd * MaxTorpedoSpeed) + Inertia;

	// Position torpedo at nose of player ship.


	auto TorpedoPtr = Arena.GetAvailableTorpedo();

	if(!TorpedoPtr)
	{
		return;
	}

	auto& Torpedo = *TorpedoPtr.Get();


	Arena.PlaySound(Arena.GetTorpedoSound());

    if(DoubleShotsLeft == 0)
	{
		// Find an available torpedo, spawn it at the nose of the player ship,
		// and give it an inertia which is player ship intertia + player ship fwd * MaxTorpedoSpeed

		//auto& Torpedo = *Arena.Torpedos[TorpedoIndex].Get();

		Torpedo.FiredByPlayer = true;

		auto P = GetPosition();
		P += PlayerFwd * (GetSize().Y / 2 + /*Inertia.Length()*/ 2.0); // The last offset is so that the bullet doesn't start off accidentally overlapping the player ship
		P = Arena.WrapPosition(P);

		Torpedo.Spawn(P, TorpedoInertia, MaxTorpedoLifeTime);
	}
	else
	{
		AdjustDoubleShotsLeft(-1);

		Torpedo.FiredByPlayer = true;

		auto P = PlayerFwd * (GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, 90.0f);
		P += GetPosition();
		//P += PlayerFwd * FMath::RandRange(0.0f, 10.0f);
		P = Arena.WrapPosition(P);

		Torpedo.Spawn(P, TorpedoInertia, MaxTorpedoLifeTime);

		auto TorpedoPtr2 = Arena.GetAvailableTorpedo();

		if(!TorpedoPtr2)
		{
			return;
		}

		auto& Torpedo2 = *TorpedoPtr2.Get();

		Torpedo2.FiredByPlayer = true;

		P = PlayerFwd * (GetSize().Y / 4);// * FMath::RandRange(0.5f, 2.0f);
		P = UDaylonUtils::Rotate(P, -90.0f);
		P += GetPosition();
		//P += PlayerFwd * FMath::RandRange(0.0f, 10.0f);
		P = Arena.WrapPosition(P);

		Torpedo2.Spawn(P, TorpedoInertia, MaxTorpedoLifeTime);
	}
}

