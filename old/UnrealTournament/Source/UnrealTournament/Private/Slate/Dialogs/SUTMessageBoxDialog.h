// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTMessageBoxDialog : public SUTDialogBase
{
	SLATE_BEGIN_ARGS(SUTMessageBoxDialog)
	: _DialogSize(FVector2D(700,400))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _ButtonMask(UTDIALOG_BUTTON_OK)
	, _IsSuppressible(false)
	, _MessageTextStyleName(TEXT("UT.Common.NormalText"))
	
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
	SLATE_ARGUMENT(bool, IsSuppressible)
	SLATE_ATTRIBUTE(ECheckBoxState, SuppressibleCheckBoxState)
	SLATE_EVENT(FOnCheckStateChanged, OnSuppressibleCheckStateChanged)
	SLATE_ARGUMENT(FText, MessageText)
	SLATE_ARGUMENT(FString, MessageTextStyleName)

	SLATE_END_ARGS()

	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	virtual TSharedRef<class SWidget> BuildCustomButtonBar() override;
private:
	TAttribute<ECheckBoxState> SuppressibleCheckBoxState;
	FOnCheckStateChanged OnSuppressibleCheckStateChanged;
	bool bIsSuppressible;

public:
	virtual bool bRemainOpenThroughTravel()
	{
		return true;
	}

};

#endif