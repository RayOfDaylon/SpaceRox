#include "PlayerShip.h"
#include "PlayViewBase.h"
#include "Constants.h"


TSharedPtr<FPlayerShip> FPlayerShip::Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor)
{
	auto Widget = SNew(FPlayerShip);

	Daylon::FinishCreating<SDaylonSprite>(Widget, RadiusFactor);

	Widget->SetAtlas(Atlas->Atlas);
	Widget->SetSize(S);
	Widget->UpdateWidgetSize();

	return Widget;
}


void FPlayerShip::ReleaseResources(UPlayViewBase& Arena)
{
	if(Shield)
	{
		Daylon::Destroy(Shield);
		Shield.Reset();
	}

	if(InvincibilityShield)
	{
		Daylon::Destroy(InvincibilityShield);
		InvincibilityShield.Reset();
	}
}


void FPlayerShip::Initialize(UPlayViewBase& Arena)
{
	IsUnderThrust      = false;
	IsSpawning         = false;
	DoubleShotsLeft    = Arena.bGodMode ? 10000 : 0;
	ShieldsLeft        = 0.0f;
	InvincibilityLeft  = 0.0f;

	Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	Hide   ();
}


void FPlayerShip::InitializeDefenses(UPlayViewBase& Arena)
{
	Shield = SNew(Daylon::SpritePlayObject2D);

	Daylon::FinishCreating<SDaylonSprite>(Shield, 0.5f);

	Shield->SetAtlas(Arena.DefensesAtlas->Atlas);
	Shield->SetCurrentCel(ShieldDefenseAtlasCel);
	Shield->SetSize(Arena.DefensesAtlas->Atlas.GetCelPixelSize());
	Shield->UpdateWidgetSize();

	Shield->Spawn  (ViewportSize / 2, FVector2D(0), 1.0f);
	Shield->Hide   ();


	InvincibilityShield = SNew(Daylon::SpritePlayObject2D);

	Daylon::FinishCreating<SDaylonSprite>(InvincibilityShield, 0.5f);

	InvincibilityShield->SetAtlas(Arena.DefensesAtlas->Atlas);
	InvincibilityShield->SetCurrentCel(InvincibilityDefenseAtlasCel);
	InvincibilityShield->SetSize(Arena.DefensesAtlas->Atlas.GetCelPixelSize());
	InvincibilityShield->UpdateWidgetSize();
	InvincibilityShield->Spawn(ViewportSize / 2, FVector2D(0), 1.0f);
	InvincibilityShield->Hide();
}



void FPlayerShip::Perform(UPlayViewBase& Arena, float DeltaTime)
{
	// Update rotation.

	const float Amt = PlayerRotationSpeed * DeltaTime;

	//UE_LOG(LogGame, Log, TEXT("Rotation force: %.5f"), Arena.RotationForce);

	SetAngle(UDaylonUtils::WrapAngle(GetAngle() + Amt * Arena.RotationForce));


	// Change sprite cel only if the thrust state actually changed.

	const bool bThrustStateChanged = (IsUnderThrust != Arena.bThrustActive);

	IsUnderThrust = Arena.bThrustActive;

	if (Arena.bThrustActive)
	{
		if(bThrustStateChanged)
		{
			SetCurrentCel(PlayerShipThrustingAtlasCel);
			Arena.PlayerShipThrustSoundLoop.Start();
		}
		else
		{
			// Still thrusting.
			Arena.PlayerShipThrustSoundLoop.Tick(DeltaTime);
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

	Move(DeltaTime, Arena.WrapPositionToViewport);


	// Update shield power levels.

	Shield->Show(Arena.bShieldActive && ShieldsLeft > 0.0f);

	if(Shield->IsVisible())
	{
		// We have to budge the shield texture by two px to look nicely centered around the player ship.
		Shield->SetPosition(GetPosition() + UDaylonUtils::Rotate(FVector2D(0, 2), GetAngle()));
		AdjustShieldsLeft(Arena, -DeltaTime);
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
		Arena.AdjustInvincibilityLeft(-DeltaTime);
	}
}


void FPlayerShip::SpawnExplosion(UPlayViewBase& Arena)
{
	const auto P = GetPosition();

	Arena.SpawnExplosion(P, 
		3.0f,
		6.0f,
		30.0f,
		160.0f,
		0.5f,
		3.0f,
		0.25f,
		80);


	// Set up second explosion event for 3/4 second later

	Daylon::FScheduledTask Task;

	Task.When = 0.66f;

	Task.What = [P, ArenaPtr = TWeakObjectPtr<UPlayViewBase>(&Arena)]()
	{
		if(!ArenaPtr.IsValid())
		{
			return;
		}

		if(ArenaPtr->GameState != EGameState::Active)
		{
			return;
		}

		ArenaPtr->SpawnExplosion(P, 
			4.5f,
			9.0f,
			45.0f,
			240.0f,
			0.5f,
			4.0f,
			0.25f,
			80);
	};

	Arena.ScheduledTasks.Add(Task);
}


bool FPlayerShip::ProcessCollision(UPlayViewBase& Arena)
{
	// Return true if we didn't get destroyed.

	if(Arena.bGodMode || InvincibilityLeft > 0.0f)
	{
		Arena.PlaySound(Arena.ShieldBonkSound);
		return true;
	}

	if(Shield->IsVisible())
	{
		AdjustShieldsLeft(Arena, -ShieldBonkDamage);
		Arena.PlaySound(Arena.ShieldBonkSound);
		return true;
	}

	return false;
}


void FPlayerShip::AdjustShieldsLeft(UPlayViewBase& Arena, float Amount)
{
	if(ShieldsLeft < 0.0f)
	{
		ShieldsLeft = 0.0f;
	}

	ShieldsLeft = FMath::Max(0.0f, ShieldsLeft + Amount);
	Arena.UpdatePowerupReadout(EPowerup::Shields); // todo: use delegated value and have arena listen to it
}
