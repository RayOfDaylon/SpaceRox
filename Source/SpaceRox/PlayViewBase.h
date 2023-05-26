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
#include "Runtime/SlateCore/Public/Widgets/SOverlay.h"
#include "Runtime/Engine/Classes/Sound/SoundBase.h"
#include "UDaylonParticlesWidget.h"
#include "UDaylonSpriteWidget.h"
#include "DaylonUtils.h"
#include "PlayObject.h"
#include "Powerup.h"
#include "PlayerShip.h"
#include "EnemyShip.h"
#include "Scavenger.h"
#include "Asteroid.h"
#include "Torpedo.h"
#include "Asteroids.h"
#include "EnemyShips.h"
#include "Explosions.h"
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

	public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TArray<TObjectPtr<USoundBase>> ExplosionSounds;


	protected:

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
	TObjectPtr<USoundBase> MenuItemSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ForwardSound;


	public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> ShieldBonkSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> TorpedoSound;


	protected:

	// -- Textures --------------------------------------------------------------

	public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* SmallRockAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* MediumRockAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* DefensesAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* BigEnemyAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* SmallEnemyAtlas;

	// The animation flipbook for the scavenger enemy 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* ScavengerAtlas;

	// The animation flipbook for the first miniboss 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* Miniboss1Atlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	FSlateBrush TorpedoBrush;

	protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* PlayerShipAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* LargeRockAtlas;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	UDaylonSpriteWidgetAtlas* TorpedoAtlas;



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

	public:
	// Make player omnipotent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Testing)
	bool bGodMode = false;

	
	// -- Design-time widgets -----------------------------------------------------------


	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UCanvasPanel* RootCanvas;

	protected:

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
	UVerticalBox* PowerupReadouts;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* PlayerShieldReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DoubleGunReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* InvincibilityReadout;

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

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* InvincibilityReadoutFlash;


	// -- Blueprint accessible properties --------------------------------------------------

	public:

	UPROPERTY(Transient, BlueprintReadWrite)
	float RotationForce = 0.0f;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bThrustActive = false;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bShieldActive = false;

	protected:


	// -- Class methods --------------------------------------------------

	void      TransitionToState          (EGameState State);
	void      StopRunning                (const FString& Reason, bool bFatal = false);

	void      PreloadSounds              ();
	void      PreloadSound               (USoundBase* Sound);

	void      InitializeScore            ();
	void      InitializePlayerShipCount  ();
	void      InitializeVariables        ();
	void      InitializeAtlases          ();
	void      InitializeSoundLoops       ();
	void      CreatePlayerShip           ();
	void      CreateTorpedos             ();

	void      SpawnAsteroids             (int32 NumAsteroids);
	void      SpawnPowerup               (TSharedPtr<FPowerup>& PowerupPtr, const FVector2D& P);
	
	void      RemovePowerup              (int32 PowerupIndex);
	void      RemovePowerups             ();
	void      RemoveTorpedos             ();

	void      KillPowerup                (int32 PowerupIndex);
	void      KillPlayerShip             ();

	void      StartWave                  ();
	void      AddPlayerShips             (int32 Amount);
	void      UpdatePlayerScoreReadout   ();

	bool      IsWaitingToSpawnPlayer     () const;
	bool      IsSafeToSpawnPlayer        () const;

	void      LoadHighScores             ();
	void      SaveHighScores             ();
	void      PopulateHighScores         ();

	void      ExecuteMenuItem            (EMenuItem Item);
	void      UpdateMenuReadout          ();
	void      NavigateMenu               (Daylon::EListNavigationDirection Direction);

	void      UpdateRoundedReadout(UTextBlock* Readout, float Value, int32& OldValue);

	public:

	static FVector2D WrapPositionToViewport  (const FVector2D& P);
	static FVector2D MakeInertia             (const FVector2D& InertiaOld, float MinDeviation, float MaxDeviation);

	bool      IsPlayerPresent            () const;
	int32     GetAvailableTorpedo        () const;
	void      PlaySound                  (USoundBase* Sound, float VolumeScale = 1.0f);
	void      UpdatePlayerShipReadout    (EPowerup PowerupKind);
	void      IncreasePlayerScoreBy      (int32 Amount);

	void      SpawnExplosion             (const FVector2D& P, const FVector2D& Inertia);

	TSharedPtr<FPlayerShip>           PlayerShip;
	TArray<TSharedPtr<FTorpedo>>      Torpedos;
	FAsteroids                        Asteroids;
	FExplosions                       Explosions; 

	//TArray<TSharedPtr<SDaylonPolyShield>>     PolyShieldPtrs;

	//TSharedPtr<FEnemyBoss>           EnemyBoss;


	protected:
	// -- Called every frame -----------------------------------------------------------

	void ProcessWaveTransition     (float DeltaTime);
	void ProcessPlayerShipSpawn    (float DeltaTime);

	void CheckCollisions           ();
	void ProcessPlayerCollision    ();

	//void UpdateAsteroids           (float DeltaTime);
	void UpdateTorpedos            (float DeltaTime);
	void UpdatePowerups            (float DeltaTime);
	void UpdateTasks               (float DeltaTime);


	// -- Member variables -----------------------------------------------------------

	public:

	Daylon::TBindableValue<int32>   PlayerScore;
	Daylon::FLoopedSound            PlayerShipThrustSoundLoop;
	Daylon::FLoopedSound            BigEnemyShipSoundLoop;
	Daylon::FLoopedSound            SmallEnemyShipSoundLoop;
	EGameState                      GameState;
	TArray<Daylon::FScheduledTask>  ScheduledTasks;
	TArray<Daylon::FDurationTask>   DurationTasks;
	TArray<TSharedPtr<FPowerup>>    Powerups; // Not including those inside asteroids
	float                           TimeUntilNextEnemyShip;
	float                           TimeUntilNextBoss;
	float                           TimeUntilNextScavenger;


	protected:

	FEnemyShips                     EnemyShips;

	Daylon::FHighScoreTable         HighScores;
	Daylon::FHighScore              MostRecentHighScore;
	UTextBlock*                     MostRecentHighScoreTextBlock[2];


	EMenuItem    SelectedMenuItem;
	int32        NumPlayerShips;
	int32        WaveNumber;
	float        TimeUntilNextWave;
	float        ThrustSoundTimeRemaining;
	float        StartMsgAnimationAge;
	float        TimeUntilIntroStateEnds;
	float        TimeUntilNextPlayerShip;
	float        TimeUntilGameOverStateEnds;
	float        MruHighScoreAnimationAge;
	bool         IsInitialized;
	bool         bHighScoreWasEntered;
};

