// Copyright 2023 Daylon Graphics Ltd. All Rights Reserved.

// SpaceRox - an Atari Asteroids clone developed with Unreal Engine.

#include "PlayViewBase.h"
#include "UMG/Public/Blueprint/WidgetBlueprintLibrary.h"
#include "UMG/Public/Components/GridSlot.h"


DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);


// Set to 1 to enable debugging
#define DEBUG_MODULE                0


#if(DEBUG_MODULE == 1)
#pragma optimize("", off)
#endif



void UPlayViewBase::OnEnterHighScore(const FString& Name)
{
	FString Str = Name.TrimStartAndEnd();

	if(Str.IsEmpty() || Str.Len() > HighScores.MaxNameLength)
	{
		// Stay inside high score entry screen until name has valid length.
		PlaySound(ErrorSound);

		// Keep the entry field focused.
		HighScoreNameEntry->SetFocus();
		return;
	}
	
	// Keep the focus on the game window
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(GetOwningPlayer(), true);

	bHighScoreWasEntered = true;

	MostRecentHighScore.Set(PlayerScore, Str);

	HighScores.Add(PlayerScore, Str);

	SaveHighScores();

	TransitionToState(EGameState::HighScores);
}


void UPlayViewBase::SaveHighScores()
{
	FString Text;

	for(const auto& Entry : HighScores.Entries)
	{
		Text += FString::Format(TEXT("{0} {1}\n"), { Entry.Score, Entry.Name });
	}

	const FString Filespec = FPaths::ProjectSavedDir() / TEXT("highscores.txt");

	if(!FFileHelper::SaveStringToFile(Text, *Filespec))
	{
		UE_LOG(LogGame, Error, TEXT("Could not save high scores to %s"), *Filespec);
	}
	else
	{
		UE_LOG(LogGame, Log, TEXT("High scores saved to %s"), *Filespec);
	}
}


void UPlayViewBase::LoadHighScores()
{
	HighScores.Clear();

#if 0
	HighScores.Add(      0, TEXT("Big loser") );
	HighScores.Add(     50, TEXT("Hey I hit something") );
	HighScores.Add(    200, TEXT("Triple digits, yeah") );
	HighScores.Add(   5000, TEXT("Killstreaker") );
	HighScores.Add(  12345, TEXT("Bonus getter") );
	HighScores.Add( 492000, TEXT("Ace") );
#else
	
	FString Text;
	const FString Filespec = FPaths::ProjectSavedDir() / TEXT("highscores.txt");

	if(!FFileHelper::LoadFileToString(Text, *Filespec))
	{
		return;
	}

	//DebugSavePath->SetVisibility(ESlateVisibility::Visible);
	//DebugSavePath->SetText(FText::FromString(Filespec));


	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);

	for(auto& Line : Lines)
	{
		Line.TrimStartAndEndInline();

		TArray<FString> Parts;
		Line.ParseIntoArrayWS(Parts);

		if(Parts.Num() < 2 || !Parts[0].IsNumeric())
		{
			continue;
		}

		const int32 Score = FCString::Atoi(*Parts[0]);

		// Assemble parts[1..n] into name
		FString Name;

		for(int32 Index = 1; Index < Parts.Num(); Index++)
		{
			Name += Parts[Index];
			Name += TEXT(" ");
		}
		Name.TrimEndInline();

		// Because the uppercase glyphs in the Hyperspace font kern too much,
		// force all highscore names to be lowercase.
		Name.ToLowerInline();

		HighScores.Add(Score, Name);
	}
#endif
}


void UPlayViewBase::PopulateHighScores()
{
	if(HighScoresReadout == nullptr)
	{
		return;
	}

	HighScoresReadout->ClearChildren();

	// We want one line per score.

	int32 RowIndex = 0;
	bool  MruScoreHighlighted = false;

	MostRecentHighScoreTextBlock[0] = nullptr;
	MostRecentHighScoreTextBlock[1] = nullptr;

	for(const auto& Entry : HighScores.Entries)
	{
		auto EntryColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));

		const bool IsMostRecentHighScore = (!MruScoreHighlighted && Entry == MostRecentHighScore);

		if(IsMostRecentHighScore)
		{
			EntryColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}

		auto TextBlockScore = Daylon::MakeWidget<UTextBlock>();
		TextBlockScore->SetFont(HighScoreReadoutFont);
		TextBlockScore->SetColorAndOpacity(EntryColor);
		TextBlockScore->SetText(FText::FromString(FString::Format(TEXT("{0}"), { Entry.Score })));

		auto ScoreSlot = HighScoresReadout->AddChildToGrid(TextBlockScore);
		ScoreSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		ScoreSlot->SetRow(RowIndex);
		ScoreSlot->SetColumn(0);

		auto TextBlockName = Daylon::MakeWidget<UTextBlock>();
		TextBlockName->SetFont(HighScoreReadoutFont);
		TextBlockName->SetColorAndOpacity(EntryColor);
		TextBlockName->SetText(FText::FromString(Entry.Name));

		auto NameSlot = HighScoresReadout->AddChildToGrid(TextBlockName);
		NameSlot->SetPadding(FMargin(30.0f, 0.0f, 0.0f, 0.0f));
		NameSlot->SetRow(RowIndex);
		NameSlot->SetColumn(1);

		if(IsMostRecentHighScore)
		{
			MostRecentHighScoreTextBlock[0] = TextBlockScore;
			MostRecentHighScoreTextBlock[1] = TextBlockName;
		}

		RowIndex++;
	}
}


#if(DEBUG_MODULE == 1)
#pragma optimize("", on)
#endif

#undef DEBUG_MODULE
