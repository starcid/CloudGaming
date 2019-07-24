// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 *	This is the base class for the panels that co-exist with the UT Menus.  The panels will fill the entire space beneath the menu
 *  and have an activation/deactivation chain.  The menu manages which panel is open and the behavior of the back button.
 **/

#pragma once
#include "SlateBasics.h"
#include "SUTMenuBase.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTPanelBase : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUTPanelBase)
	{}
	SLATE_END_ARGS()

public:
	/** needed for every widget */
	void Construct(const FArguments& InArgs, TWeakObjectPtr<UUTLocalPlayer> PlayerOwner);

	virtual void ConstructPanel(FVector2D CurrentViewportSize);
	virtual void OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow);
	virtual void OnHidePanel();

	inline TWeakObjectPtr<class UUTLocalPlayer> GetPlayerOwner()
	{
		return PlayerOwner;
	}

	inline TSharedPtr<SUTMenuBase> GetParentWindow()
	{
		return ParentWindow;
	}

	// I can't believe how slate manages slots.
	int32 ZOrder;

	AUTPlayerState* GetOwnerPlayerState();

	void ConsoleCommand(FString Command);

	// Used by generic lists to generate string widgets for each item
	TSharedRef<SWidget> GenerateStringListWidget(TSharedPtr<FString> InItem);

	// Return true to show the back button in the conatining desktop
	virtual bool ShouldShowBackButton()
	{
		if (PlayerOwner.IsValid() && PlayerOwner->IsKillcamReplayActive())
		{
			return false;
		}

		return true;
	}

	// A TAG that can quickly describe this panel
	FName Tag;


	virtual TSharedPtr<SWidget> GetInitialFocus();

	// Will be called when the menu contain this panel is closed for good.
	virtual void PanelClosed()
	{
		OnHidePanel();
	}

protected:

	// Will be true if this panel is closing
	bool bClosing;

	// The Player Owner that owns this panel
	TWeakObjectPtr<UUTLocalPlayer> PlayerOwner;

	// The Window that contains this panel.  NOTE: this will only be valid if this panel is contained within an SUTMenuBase.
	TSharedPtr<SUTMenuBase> ParentWindow;


};

#endif