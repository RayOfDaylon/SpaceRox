// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMG/Public/Components/TextBlock.h"
#include "UMG/Public/Components/CanvasPanel.h"
#include "UMG/Public/Components/HorizontalBox.h"
#include "UMG/Public/Components/VerticalBox.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/EditableTextBox.h"
#include "Runtime/Engine/Classes/Sound/SoundBase.h"
#include "UParticlesWidget.h"
#include "LocalUtils.h"
#include "PlayViewBase.generated.h"


UENUM()
enum class ERotationDirection : int32
{
	// These values are casted to int
	CounterClockwise = -1,
	Clockwise        =  1,
};


UENUM()
enum class EListNavigationDirection : int32
{
	// These values are casted to int
	Backwards = -1,
	Forwards  =  1,
};


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
	Active,				// Game is being played
	Over,				// Game is over
	MainMenu,
	Credits,
	HighScores,
	Settings,           // reserved for future use
	Help,
	HighScoreEntry		// Player is entering a high score

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



USTRUCT()
struct FTorpedo : public FPlayObject
{
	GENERATED_BODY()

	bool FiredByPlayer;
};


USTRUCT()
struct FEnemyShip : public FPlayObject
{
	GENERATED_BODY()

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
	TObjectPtr<USoundBase> PlayerShipDestroyedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> EnemyShipSmallSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<USoundBase> EnemyShipBigSound;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> BigRockBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> MediumRockBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objects)
	TArray<FSlateBrush> SmallRockBrushes;


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
	UVerticalBox* HighScoresContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* HighScoreEntryContent;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* HighScoreNameEntry;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* HighScoresReadout;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DebugSavePath;


	// -- Design-time animations -----------------------------------------------------------

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* MenuOutro;


	// -- Blueprint accessible properties --------------------------------------------------

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bRotateRightActive = false;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bRotateLeftActive = false;

	UPROPERTY(Transient, BlueprintReadWrite)
	bool bThrustActive = false;


	// -- Class methods --------------------------------------------------

	void      StopRunning                (const FString& Reason, bool bFatal = false);

	void      PreloadSounds              ();
	void      PreloadSound               (USoundBase* Sound);
	void      PlaySound                  (USoundBase* Sound, float VolumeScale = 1.0f);
	void      InitializeScore            ();
	void      InitializePlayerShipCount  ();
	void      InitializeVariables        ();
	void      CreatePlayerShip           ();
	void      CreateTorpedos             ();
	void      LaunchTorpedoFromEnemy     (const FEnemyShip& Shooter, bool IsBigEnemy);
	FVector2D WrapPositionToViewport     (const FVector2D& P) const;

	void      TransitionToState          (EGameState State);

	template<class WidgetT> WidgetT* MakeWidget();

	void      Spawn                      (FPlayObject& PlayObject, const FVector2D& P, const FVector2D& Inertia, float LifeRemaining);
	void      SpawnAsteroids             (int32 NumAsteroids);
	void      SpawnExplosion             (const FVector2D& P);
	void      SpawnPlayerShipExplosion   (const FVector2D& P);
	void      RemoveAsteroid             (int32 Index);
	void      RemoveAsteroids            ();

	void      KillAsteroid               (int32 AsteroidIndex, bool KilledByPlayer);
	void      KillEnemyShip              (int32 EnemyIndex);
	void      KillPlayerShip             ();
	void      DestroyWidget              (FPlayObject& PlayObject);
	void      DestroyWidget              (UWidget* Widget);

	int32     GetAvailableTorpedo        () const;
	void      IncreasePlayerScoreBy      (int32 Amount);
	void      StartWave                  ();
	void      AddPlayerShips             (int32 Amount);
	void      UpdatePlayerScoreReadout   ();
	bool      IsWaitingToSpawnPlayer     () const;
	bool      IsSafeToSpawnPlayer        () const;
	void      RemoveEnemyShip            (int32 EnemyIndex);
	void      RemoveEnemyShips           ();

	void      LoadHighScores             ();
	void      SaveHighScores             ();
	void      PopulateHighScores         ();
	void      ExecuteMenuItem            (EMenuItem Item);
	void      UpdateMenuReadout          ();
	void      NavigateMenu               (EListNavigationDirection Direction);


	// -- Called every frame -----------------------------------------------------------

	void MovePlayObject            (FPlayObject& PlayObject, float DeltaTime);
	void ProcessWaveTransition     (float DeltaTime);
	void ProcessPlayerShipSpawn    (float DeltaTime);
	void UpdateExplosions          (float DeltaTime);
	//void AnimateStartMessage     (float DeltaTime);

	void CheckCollisions           ();

	void UpdatePlayerRotation      (float DeltaTime, ERotationDirection Direction);
	void UpdatePlayerRightRotation (float DeltaTime);
	void UpdatePlayerLeftRotation  (float DeltaTime);
	void UpdatePlayerPosition      (float DeltaTime);
	void UpdateEnemyShips          (float DeltaTime);
	void UpdateAsteroids           (float DeltaTime);
	void UpdateTorpedos            (float DeltaTime);
	void UpdateTasks               (float DeltaTime);


	// -- Member variables -----------------------------------------------------------

	FLoopedSound              PlayerShipThrustSoundLoop;
	FLoopedSound              BigEnemyShipSoundLoop;
	FLoopedSound              SmallEnemyShipSoundLoop;

	FPlayObject               PlayerShipObj;
	TArray<FPlayObject>       Asteroids;
	TArray<FEnemyShip>        EnemyShips;
	TArray<FTorpedo>          Torpedos;
	TArray<UParticlesWidget*> Explosions;
	FHighScoreTable           HighScores;

	TArray<FScheduledTask>    ScheduledTasks;
	TArray<FDurationTask>     DurationTasks;

	EMenuItem                 SelectedMenuItem;
	int32                     NumPlayerShips;
	int32                     PlayerScore;
	int32                     WaveNumber;
	float                     TimeUntilNextWave;
	float                     ThrustSoundTimeRemaining;
	float                     StartMsgAnimationAge;
	float                     IntroAge;
	float                     TimeUntilNextPlayerShip;
	float                     TimeUntilNextEnemyShip;
	float                     TimeUntilGameOverStateEnds;
	bool                      IsInitialized;
	bool                      bPlayerShipUnderThrust;
	bool                      bSpawningPlayer;
	bool                      bEnemyShootsAtPlayer;
	bool                      bHighScoreWasEntered;
	EGameState                GameState;
};
