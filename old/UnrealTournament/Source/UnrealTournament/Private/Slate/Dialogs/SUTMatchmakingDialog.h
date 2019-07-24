// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTMatchmakingDialog : public SUTDialogBase
{
public:
	SLATE_BEGIN_ARGS(SUTMatchmakingDialog)
		: _DialogSize(FVector2D(0.5f, 0.25f))
		, _bDialogSizeIsRelative(true)
		, _DialogPosition(FVector2D(0.5f, 0.5f))
		, _DialogAnchorPoint(FVector2D(0.5f, 0.5f))
		, _ContentPadding(FVector2D(10.0f, 5.0f))
		, _ButtonMask(UTDIALOG_BUTTON_CANCEL)
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)
		SLATE_ARGUMENT(FText, DialogTitle)
		SLATE_ARGUMENT(FVector2D, DialogSize)
		SLATE_ARGUMENT(bool, bDialogSizeIsRelative)
		SLATE_ARGUMENT(FVector2D, DialogPosition)
		SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)
		SLATE_ARGUMENT(FVector2D, ContentPadding)
		SLATE_ARGUMENT(uint16, ButtonMask)
		SLATE_EVENT(FDialogResultDelegate, OnDialogResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnButtonClick(uint16 ButtonID);

	FText GetMatchmakingText() const;
	FText GetMatchmakingTimeElapsedText() const;
	FText GetMatchmakingEstimatedTimeText() const;
	FText GetRegionText() const;

	float TimeDialogOpened;
	float RetryTime;
	float RetryCountdown;
	int32 LastMatchmakingPlayersNeeded;
	
public:
	virtual void Tick(const FGeometry & AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool bRemainOpenThroughTravel()
	{
		return true;
	}

};

#endif