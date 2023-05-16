// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMG/Public/Components/TextBlock.h"
#include "UMG/Public/Components/CanvasPanel.h"
#include "UMG/Public/Components/HorizontalBox.h"
#include "UMG/Public/Components/VerticalBox.h"
#include "UMG/Public/Components/GridPanel.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/EditableTextBox.h"
#include "Runtime/Engine/Classes/Sound/SoundBase.h"
#include "UDaylonParticlesWidget.h"
#include "UDaylonSpriteWidget.h"
#include "DaylonUtils.h"
#include "PlayViewBase.generated.h"





UENUM()
enum class EMenuItem : uint8
{
	StartPlaying   = 0,
	ShowHighScores,
	ShowCredits,
	ShowHelp,
	Exit,

	Count
};



UENUM()
enum class EPowerup : uint8
{
	Nothing     = 0,
	DoubleGuns,
	Shields,
	Invincibility
};


UENUM()
enum class EGameState : uint8
{
	Startup = 0,
	Intro,              // Startup graphics, appears only once
	Active,             // Game is being played
	Over,               // Game is over
	MainMenu,
	Credits,
	HighScores,
	Settings,           // reserved for future use
	Help,
	HighScoreEntry      // Player is entering a high score

	/*
		State transition graph:

		Startup --> Intro

		Intro ---> Menu

		Menu -- (start chosen)       --> Active
		     -- (help chosen)        --> Help           --> Menu
			 -- (credits chosen)     --> Credits        --> Menu
			 -- (high scores chosen) --> High scores    --> Menu

		Active -- (player dies)      --> Over
		       -- (Esc pressed)      --> Menu

		Over -- (high score?)        --> HighScoreEntry --> Menu
		     -- (no high score)      --> Menu
	*/
};



class FTorpedo : public Daylon::ImagePlayObject2D
{
	public:

	bool FiredByPlayer;

	static TSharedPtr<FTorpedo> Create(FSlateBrush& Brush, float RadiusFactor)
	{
		auto Widget = SNew(FTorpedo);

		Daylon::FinishCreating<SImage>(Widget, RadiusFactor);

		Widget->SetBrush(Brush);

		return Widget;
	}
};


class FPlayerShip : public Daylon::SpritePlayObject2D
{
	public:

	bool   IsUnderThrust;
	bool   IsSpawning;
	int32  DoubleShotsLeft;
	float  ShieldsLeft;
	float  InvincibilityLeft;

	static TSharedPtr<FPlayerShip> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S, float RadiusFactor)
	{
		auto Widget = SNew(FPlayerShip);

		Daylon::FinishCreating<SDaylonSpriteWidget>(Widget, RadiusFactor);

		Widget->SetAtlas(Atlas->Atlas);
		Widget->SetSize(S);
		Widget->UpdateWidgetSize();

		return Widget;
	}
};




class FPowerup : public Daylon::SpritePlayObject2D
{
	public:

	EPowerup Kind = EPowerup::Nothing;

	static TSharedPtr<FPowerup> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S);

	virtual void Update(float DeltaTime) override { Daylon::SpritePlayObject2D::Update(DeltaTime); }
};


class FAsteroid : public Daylon::ImagePlayObject2D
{
	public:

	//FPowerup Powerup;
	TSharedPtr<FPowerup> Powerup;

	bool HasPowerup() const { return (Powerup && Powerup->Kind != EPowerup::Nothing); }

	static TSharedPtr<FAsteroid> Create(FSlateBrush& Brush)
	{
		auto Widget = SNew(FAsteroid);

		Daylon::FinishCreating<SImage>(Widget, 0.5f);

		Widget->SetBrush(Brush);

		return Widget;
	}
};


class FScavenger : public Daylon::SpritePlayObject2D
{
	public:

	TArray<TSharedPtr<FPowerup>> AcquiredPowerups;

	TWeakPtr<FPowerup> CurrentTarget;


	static TSharedPtr<FScavenger> Create(UDaylonSpriteWidgetAtlas* Atlas, const FVector2D& S);
};


class FEnemyShip : public Daylon::ImagePlayObject2D
{
	public:

	float TimeRemainingToNextShot = 0.0f;
	float TimeRemainingToNextMove = 0.0f;


	static TSharedPtr<FEnemyShip> Create(FSlateBrush& Brush, int Value, float RadiusFactor)
	{
		auto Widget = SNew(FEnemyShip);

		Daylon::FinishCreating<SImage>(Widget, RadiusFactor);

		Widget->SetBrush(Brush);

		Widget->Value = Value;

		return Widget;
	}
};


// Base view class of the SpaceRox game arena.
// Parent of a UUserWidget asset whose blueprint handles design aspects and some scripting.
UCLASS()
class SPACEROX_API UPlayViewBase : public UUserWidget
{
	GENERATED_BODY()

	// UUserWidget
	virtual void NativeOnInitialized                () override;
	virtual void NativeTick                         (const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void OnAnimationFinished_Implementation (const UWidgetAnimation* Animation) override;


	protected:

	// -- Blueprint callable functions -------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnFireTorpedo();

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnStartButtonPressed();

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnBackButtonPressed();

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnForwardButtonPressed();

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnAbortButtonPressed();

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnAimPlayerShip(const FVector2D& Direction);

	UFUNCTION(BlueprintCallable, Category = SpaceRox)
	void OnEnterHighScore(const FString& Name);


	// -- Audio properties -------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ErrorSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> PlayerShipBonusSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ThrustSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TArray<TObjectPtr<USoundBase>> ExplosionSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> TorpedoSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> DoubleTorpedoSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> PlayerShipDestroyedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> EnemyShipSmallSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> EnemyShipBigSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> GainDoubleGunPowerupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> GainShieldPowerupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ShieldBonkSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> MenuItemSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ForwardSound;


	// -- Textures --------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* PlayerShipAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush BigEnemyBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush SmallEnemyBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush TorpedoBrush;

	// The way the player ship looks when shielded
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush ShieldBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> BigRockBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> MediumRockBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> SmallRockBrushes;


	// -- USpriteWidgetAtlas animated textures -------------------------------------

	// The animation flipbook for the asteroids' double guns powerup badge 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* DoubleGunsPowerupAtlas;

	// The animation flipbook for the asteroids' shield powerup badge 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* ShieldPowerupAtlas;

	// The animation flipbook for the asteroids' invincibility powerup badge 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* InvincibilityPowerupAtlas;

	// The animation flipbook for the scavenger enemy 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* ScavengerAtlas;


	// -- Fonts -------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scoring)
	FSlateFontInfo HighScoreReadoutFont;


	// -- Testing-related properties -------------------------------------------------

	// Number of seconds to delay starting the intro (useful for video captures, use zero for shipping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	float InitialDelay = 0.0f;

	// Player score to start game at, to immediately increase difficulty (use zero for shipping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	int32 StartingScore = 0;

	// Number of asteroids to spawn at wave start (use zero to not override)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	int32 NumAsteroidsOverride = 0;

	// Number of powerups to spawn at wave start (use zero to not override)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	int32 NumPowerupsOverride = 0;

	// Make player omnipotent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	bool bGodMode = false;

	
	// -- Design-time widgets -----------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UCanvasPanel* RootCanvas;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* TitleGraphic;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* VersionReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* GameTitle;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* GameOverMessage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* IntroContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* MenuContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* CreditsContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* HelpContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* PlayerScoreReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UHorizontalBox* PlayerShipsReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* PlayerShieldReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ShieldLabel;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DoubleGunReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* DoubleGunLabel;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* InvincibilityReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* InvincibilityLabel;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* HighScoresContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* HighScoreEntryContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* HighScoreNameEntry;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UGridPanel* HighScoresReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DebugSavePath;


	// -- Design-time animations -----------------------------------------------------------

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PressToStartFlash;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* MenuOutro;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ShieldReadoutFlash;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* DoubleGunReadoutFlash;


	// -- Blueprint accessible properties --------------------------------------------------

	UPROPERTY(Transient, BlueprintReadWrite)
	float RotationForce = 0.0f;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bThrustActive = false;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bShieldActive = false;


	// -- Class methods --------------------------------------------------

	void      TransitionToState          (EGameState State);
	void      StopRunning                (const FString& Reason, bool bFatal = false);

	void      PreloadSounds              ();
	void      PreloadSound               (USoundBase* Sound);
	void      PlaySound                  (USoundBase* Sound, float VolumeScale = 1.0f);

	void      InitializeScore            ();
	void      InitializePlayerShipCount  ();
	void      InitializeVariables        ();
	void      InitializePlayerShip       ();
	void      InitializePlayerShield     ();
	void      CreatePlayerShip           ();
	void      CreateTorpedos             ();

	void      LaunchTorpedoFromEnemy     (const FEnemyShip& Shooter, bool IsBigEnemy);
	void      SpawnAsteroids             (int32 NumAsteroids);
	void      SpawnEnemyShip             ();
	void      SpawnPowerup               (TSharedPtr<FPowerup>& PowerupPtr, const FVector2D& P);
	void      SpawnExplosion             (const FVector2D& P, float MinParticleSize, float MaxParticleSize, float MinParticleVelocity, float MaxParticleVelocity,
                                          float MinParticleLifetime, float MaxParticleLifetime, float FinalOpacity, int32 NumParticles);
	void      SpawnExplosion             (const FVector2D& P);
	void      SpawnPlayerShipExplosion   (const FVector2D& P);
	void      RemoveAsteroid             (int32 Index);
	void      RemoveAsteroids            ();
	void      RemoveEnemyShip            (int32 EnemyIndex);
	void      RemoveEnemyShips           ();
	void      RemoveScavenger            (int32 ScavangerIndex);
	void      RemoveExplosions           ();
	void      RemovePowerup              (int32 PowerupIndex);
	void      RemovePowerups             ();
	void      RemoveTorpedos             ();

	void      KillAsteroid               (int32 AsteroidIndex, bool KilledByPlayer);
	void      KillEnemyShip              (int32 EnemyIndex);
	void      KillScavenger              (int32 ScavengerIndex);
	void      KillPowerup                (int32 PowerupIndex);
	void      KillPlayerShip             ();

	int32     GetAvailableTorpedo        () const;
	void      IncreasePlayerScoreBy      (int32 Amount);
	void      StartWave                  ();
	void      AddPlayerShips             (int32 Amount);
	void      UpdatePlayerScoreReadout   ();
	void      UpdatePowerupReadout       (EPowerup PowerupKind);
	void      AdjustDoubleShotsLeft      (int32 Amount);
	void      AdjustShieldsLeft          (float Amount);
	void      AdjustInvincibilityLeft    (float Amount);

	bool      IsPlayerPresent            () const;
	bool      IsWaitingToSpawnPlayer     () const;
	bool      IsSafeToSpawnPlayer        () const;

	void      LoadHighScores             ();
	void      SaveHighScores             ();
	void      PopulateHighScores         ();

	void      ExecuteMenuItem            (EMenuItem Item);
	void      UpdateMenuReadout          ();
	void      NavigateMenu               (Daylon::EListNavigationDirection Direction);


	// -- Called every frame -----------------------------------------------------------

	void ProcessWaveTransition     (float DeltaTime);
	void ProcessPlayerShipSpawn    (float DeltaTime);

	void CheckCollisions           ();
	void ProcessPlayerCollision    ();

	void UpdatePlayerRotation      (float DeltaTime);
	void UpdatePlayerShip          (float DeltaTime);
	void UpdateEnemyShips          (float DeltaTime);
	void UpdateAsteroids           (float DeltaTime);
	void UpdateTorpedos            (float DeltaTime);
	void UpdatePowerups            (float DeltaTime);
	void UpdateExplosions          (float DeltaTime);
	void UpdateTasks               (float DeltaTime);


	// -- Member variables -----------------------------------------------------------

	Daylon::FLoopedSound            PlayerShipThrustSoundLoop;
	Daylon::FLoopedSound            BigEnemyShipSoundLoop;
	Daylon::FLoopedSound            SmallEnemyShipSoundLoop;

	TSharedPtr<FPlayerShip>                    PlayerShip;
	TSharedPtr<Daylon::ImagePlayObject2D>      PlayerShield;

	TArray<TSharedPtr<FAsteroid>>              Asteroids;
	TArray<TSharedPtr<FEnemyShip>>             EnemyShips;
	TArray<TSharedPtr<FScavenger>>             Scavengers;
	TArray<TSharedPtr<FTorpedo>>               Torpedos;
	TArray<TSharedPtr<FPowerup>>               Powerups; // Not including those inside asteroids
	TArray<TSharedPtr<SDaylonParticlesWidget>> Explosions; 

	Daylon::FHighScoreTable         HighScores;
	Daylon::FHighScore              MostRecentHighScore;
	UTextBlock*                     MostRecentHighScoreTextBlock[2];

	TArray<Daylon::FScheduledTask>  ScheduledTasks;
	TArray<Daylon::FDurationTask>   DurationTasks;

	EGameState   GameState;
	EMenuItem    SelectedMenuItem;
	int32        NumPlayerShips;
	int32        PlayerScore;
	int32        WaveNumber;
	int32        NumSmallEnemyShips;
	int32        NumBigEnemyShips;
	float        TimeUntilNextWave;
	float        ThrustSoundTimeRemaining;
	float        StartMsgAnimationAge;
	float        TimeUntilIntroStateEnds;
	float        TimeUntilNextPlayerShip;
	float        TimeUntilNextEnemyShip;
	float        TimeUntilGameOverStateEnds;
	float        MruHighScoreAnimationAge;
	bool         IsInitialized;
	bool         bEnemyShootsAtPlayer;
	bool         bHighScoreWasEntered;

	float        TimeUntilNextScavenger;
};


// -- Old UWidget-based play object subclasses ------------------------

/*
struct FTorpedo : public Daylon::FImagePlayObject
{
	bool FiredByPlayer;
};
*/


/*
struct FPlayerShip : public Daylon::FImagePlayObject
{
	bool  IsUnderThrust;
	bool  IsSpawning;
	int32 DoubleShotsLeft;
	float ShieldsLeft;
};
*/

/*
class FPlayerShip : public Daylon::ImagePlayObject2D
{
	public:

	bool   IsUnderThrust;
	bool   IsSpawning;
	int32  DoubleShotsLeft;
	float  ShieldsLeft;

	static TSharedPtr<FPlayerShip> Create(FSlateBrush& Brush, float RadiusFactor)
	{
		auto Widget = SNew(FPlayerShip);

		Daylon::FinishCreating<SImage>(Widget, RadiusFactor);

		Widget->SetBrush(Brush);

		return Widget;
	}
};
*/


/*struct FPowerup : public Daylon::FSpritePlayObject
{
	EPowerup Kind = EPowerup::Nothing;


	virtual void Tick(float DeltaTime) override
	{
		if(Widget != nullptr)
		{
			Widget->Tick(DeltaTime);
		}
	}
};*/


/*struct FAsteroid : public Daylon::FImagePlayObject
{
	FPowerup Powerup;

	bool HasPowerup() const { return (Powerup.Kind != EPowerup::Nothing); }
};*/


/*
struct FEnemyShip : public Daylon::FImagePlayObject
{
	float TimeRemainingToNextShot = 0.0f;
	float TimeRemainingToNextMove = 0.0f;
};
*/
