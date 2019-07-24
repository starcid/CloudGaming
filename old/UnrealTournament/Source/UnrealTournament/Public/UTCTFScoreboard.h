// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTeamScoreboard.h"
#include "UTCTFScoreboard.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTCTFScoreboard : public UUTTeamScoreboard
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FText ScoringPlaysHeader;
	UPROPERTY()
	FText AssistedByText;
	UPROPERTY()
	FText UnassistedText;
	UPROPERTY()
	FText CaptureText;
	UPROPERTY()
	FText ScoreText;
	UPROPERTY()
	FText NoScoringText;
	UPROPERTY()
	FText PeriodText[3];

	UPROPERTY()
		bool bGroupRoundPairs;

	UPROPERTY()
		FText ScoringPlayScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		FText CH_Caps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		FText CH_Assists;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		FText CH_Returns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	float ColumnHeaderCapsX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	float ColumnHeaderAssistsX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	float ColumnHeaderReturnsX;

	UPROPERTY()
		float TimeLineOffset;

	virtual bool ShouldDrawScoringStats() override;
	virtual void PreDraw(float DeltaTime, AUTHUD* InUTHUDOwner, UCanvas* InCanvas, FVector2D InCanvasCenter) override;

protected:
	virtual void DrawScoreHeaders(float RenderDelta, float& YOffset);
	virtual void DrawPlayerScore(AUTPlayerState* PlayerState, float XOffset, float YOffset, float Width, FLinearColor DrawColor) override;

	virtual void DrawStatsLeft(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom) override;
	virtual void DrawStatsRight(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom) override;

	/** Draw the list of flag captures with details. */
	virtual void DrawScoringPlays(float RenderDelta, float& YOffset, float XOffset, float ScoreWidth, float PageBottom);
	virtual void DrawScoringPlayInfo(const struct FCTFScoringPlay& Play, float CurrentScoreHeight, float SmallYL, float MedYL, float DeltaTime, float& YPos, float XOffset, float ScoreWidth, FFontRenderInfo TextRenderInfo, bool bIsSmallPlay);
	virtual int32 GetSmallPlaysCount(int32 NumPlays) const;

	virtual void DrawTeamStats(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom, const FStatsFontInfo& StatsFontInfo) override;
};