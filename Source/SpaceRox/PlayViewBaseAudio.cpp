// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.


#include "PlayViewBase.h"
#include "Constants.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

/*
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "Runtime/Core/Public/Logging/LogMacros.h"
*/

DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);


// Set to 1 to enable debugging
#define DEBUG_MODULE                0


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



void UPlayViewBase::PreloadSound(USoundBase* Sound)
{
	// hack: preload sound by playing it at near-zero volume.

	PlaySound(Sound, 0.01f);
}


void UPlayViewBase::PreloadSounds()
{
	PreloadSound(ErrorSound               );
	PreloadSound(PlayerShipDestroyedSound );
	PreloadSound(PlayerShipBonusSound     );
	PreloadSound(ThrustSound              );
	PreloadSound(TorpedoSound             );
	PreloadSound(DoubleTorpedoSound       );
	PreloadSound(GainDoubleGunPowerupSound);
	PreloadSound(GainShieldPowerupSound   );
	PreloadSound(ShieldBonkSound          );
	PreloadSound(EnemyShipSmallSound      );
	PreloadSound(EnemyShipBigSound        );
	PreloadSound(MenuItemSound            );
	PreloadSound(ForwardSound             );

	for(auto Sound : ExplosionSounds)
	{
		PreloadSound(Sound);
	}
}


void UPlayViewBase::InitializeSoundLoops()
{
	PlayerShipThrustSoundLoop.Set (this, ThrustSound);
	BigEnemyShipSoundLoop.Set     (this, EnemyShipBigSound);
	SmallEnemyShipSoundLoop.Set   (this, EnemyShipSmallSound);
}


void UPlayViewBase::PlaySound(USoundBase* Sound, float VolumeScale)
{
	UGameplayStatics::PlaySound2D(this, Sound, VolumeScale);
}



#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
