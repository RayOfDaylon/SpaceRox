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
	Shields
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



struct FTorpedo : public Daylon::FImagePlayObject
{
	bool FiredByPlayer;
};


struct FPlayerShip : public Daylon::FImagePlayObject
{
	bool  IsUnderThrust;
	bool  IsSpawning;
	int32 DoubleShotsLeft;
	float ShieldsLeft;
};


struct FPowerup : public Daylon::FSpritePlayObject
{
	EPowerup Kind = EPowerup::Nothing;


	virtual void Tick(float DeltaTime) override
	{
		if(Widget != nullptr)
		{
			Widget->Tick(DeltaTime);
		}
	}
};


struct FAsteroid : public Daylon::FImagePlayObject
{
	FPowerup Powerup;

	bool HasPowerup() const { return (Powerup.Kind != EPowerup::Nothing); }
};



struct FEnemyShip : public Daylon::FImagePlayObject
{
	float TimeRemainingToNextShot = 0.0f;
	float TimeRemainingToNextMove = 0.0f;
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


	// -- UImage brushes (textures) -------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush PlayerShipBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush PlayerShipThrustingBrush;

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


	// -- Fonts -------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scoring)
	FSlateFontInfo HighScoreReadoutFont;


	// -- Testing-related properties -------------------------------------------------

	// Player score to start game at, to immediately increase difficulty (use zero for shipping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	int32 StartingScore = 0;

	// Number of asteroids to spawn at wave start (use zero to not override)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	int32 NumAsteroidsOverride = 0;

	// Make player omnipotent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	bool bGodMode = false;

	
	// -- Design-time widgets -----------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UCanvasPanel* RootCanvas;

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

	void      StopRunning                (const FString& Reason, bool bFatal = false);

	void      PreloadSounds              ();
	void      PreloadSound               (USoundBase* Sound);
	void      PlaySound                  (USoundBase* Sound, float VolumeScale = 1.0f);
	void      InitializeScore            ();
	void      InitializePlayerShipCount  ();
	void      InitializeVariables        ();
	void      InitializePlayerShip       ();
	void      InitializePlayerShield     ();
	void      CreateTorpedos             ();
	void      LaunchTorpedoFromEnemy     (const FEnemyShip& Shooter, bool IsBigEnemy);

	void      TransitionToState          (EGameState State);

	//template<class WidgetT> WidgetT* MakeWidget();

	void      SpawnAsteroids             (int32 NumAsteroids);
	void      SpawnEnemyShip             ();
	void      SpawnPowerup               (FPowerup& Powerup, const FVector2D& P);
	void      SpawnExplosion             (const FVector2D& P);
	void      SpawnPlayerShipExplosion   (const FVector2D& P);
	void      RemoveAsteroid             (int32 Index);
	void      RemoveAsteroids            ();
	void      RemoveEnemyShip            (int32 EnemyIndex);
	void      RemoveEnemyShips           ();
	void      RemoveExplosions           ();
	void      RemovePowerup              (int32 PowerupIndex);
	void      RemovePowerups             ();
	void      RemoveTorpedos             ();

	void      KillAsteroid               (int32 AsteroidIndex, bool KilledByPlayer);
	void      KillEnemyShip              (int32 EnemyIndex);
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

	bool      IsWaitingToSpawnPlayer     () const;
	bool      IsSafeToSpawnPlayer        () const;

	void      LoadHighScores             ();
	void      SaveHighScores             ();
	void      PopulateHighScores         ();
	void      ExecuteMenuItem            (EMenuItem Item);
	void      UpdateMenuReadout          ();
	void      NavigateMenu               (Daylon::EListNavigationDirection Direction);


	// -- Called every frame -----------------------------------------------------------

	//void MovePlayObject            (IPlayObject& PlayObject, float DeltaTime);
	void ProcessWaveTransition     (float DeltaTime);
	void ProcessPlayerShipSpawn    (float DeltaTime);
	//void AnimateStartMessage     (float DeltaTime);

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

	Daylon::FLoopedSound      PlayerShipThrustSoundLoop;
	Daylon::FLoopedSound      BigEnemyShipSoundLoop;
	Daylon::FLoopedSound      SmallEnemyShipSoundLoop;

	FPlayerShip                  PlayerShip;
	Daylon::FImagePlayObject     PlayerShield;

	TArray<FAsteroid>               Asteroids;
	TArray<FEnemyShip>              EnemyShips;
	TArray<FTorpedo>                Torpedos;
	TArray<FPowerup>                Powerups;
	TArray<UDaylonParticlesWidget*> Explosions;
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
};
