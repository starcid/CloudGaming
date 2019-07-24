// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SWidgetSwitcher.h"
#include "../Base/SUTDialogBase.h"
#include "UTPlayerInput.h"
#include "../SKeyBind.h"
#include "SNumericEntryBox.h"
#include "../Widgets/SUTTabButton.h"

#if !UE_SERVER

struct FKeyBindTracker
{
	TSharedPtr<SKeyBind> PrimaryKeyBindWidget;
	TSharedPtr<SKeyBind> SecondaryKeyBindWidget;
	FKeyConfigurationInfo* KeyConfig;

	FKeyBindTracker()
	{
		PrimaryKeyBindWidget.Reset();
		SecondaryKeyBindWidget.Reset();
		KeyConfig = nullptr;
	}

	FKeyBindTracker(FKeyConfigurationInfo* inKeyConfig)
		: KeyConfig(inKeyConfig)
	{
		PrimaryKeyBindWidget.Reset();
		SecondaryKeyBindWidget.Reset();
	}

	static TSharedRef<FKeyBindTracker> Make(FKeyConfigurationInfo* inKeyConfig)
	{
		return MakeShareable( new FKeyBindTracker( inKeyConfig ) );
	}

};

class UNREALTOURNAMENT_API SUTControlSettingsDialog : public SUTDialogBase
{
public:

	SLATE_BEGIN_ARGS(SUTControlSettingsDialog)
	: _DialogSize(FVector2D(1500,900))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _ButtonMask(UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_CANCEL)
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
protected:

	virtual TSharedRef<class SWidget> BuildCustomButtonBar();
	virtual FReply OnButtonClick(uint16 ButtonID);	

	FReply OKClick();
	FReply CancelClick();
	FReply OnBindDefaultClick();

	TSharedPtr<SVerticalBox> ControlList;
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SWidgetSwitcher> TabWidget;
	TSharedPtr<SButton> ResetToDefaultsButton;

	TArray<TSharedPtr<FKeyBindTracker>> BindList;

	FReply OnTabClickKeyboard()
	{
		ResetToDefaultsButton->SetVisibility(EVisibility::All);
		TabWidget->SetActiveWidgetIndex(0);
		MovementSettingsTabButton->UnPressed();
		KeyboardSettingsTabButton->BePressed();
		MouseSettingsTabButton->UnPressed();

		return FReply::Handled();
	}
	FReply OnTabClickMouse()
	{
		ResetToDefaultsButton->SetVisibility(EVisibility::Hidden);
		TabWidget->SetActiveWidgetIndex(1);
		MovementSettingsTabButton->UnPressed();
		KeyboardSettingsTabButton->UnPressed();
		MouseSettingsTabButton->BePressed();
		return FReply::Handled();
	}
	FReply OnTabClickMovement()
	{
		TabWidget->SetActiveWidgetIndex(2);
		ResetToDefaultsButton->SetVisibility(EVisibility::Hidden);
		MovementSettingsTabButton->BePressed();
		KeyboardSettingsTabButton->UnPressed();
		MouseSettingsTabButton->UnPressed();
		return FReply::Handled();
	}

	//mouse settings
	TSharedPtr<SSlider> MouseSensitivity;
	TSharedPtr<SCheckBox> MouseSmoothing;
	TSharedPtr<SCheckBox> MouseInvert;
	TSharedPtr<SEditableTextBox> MouseSensitivityEdit;
	void EditSensitivity(const FText& Input, ETextCommit::Type);

	TSharedPtr<SCheckBox> MouseAccelerationCheckBox;
	TSharedPtr<SEditableTextBox> MouseAccelerationEdit;
	TSharedPtr<SSlider> MouseAcceleration;
	FVector2D MouseAccelerationRange;
	void EditAcceleration(const FText& Input, ETextCommit::Type);

	TSharedPtr<SCheckBox> MouseAccelerationMaxCheckBox;
	TSharedPtr<SEditableTextBox> MouseAccelerationMaxEdit;
	TSharedPtr<SSlider> MouseAccelerationMax;
	FVector2D MouseAccelerationMaxRange;
	void EditAccelerationMax(const FText& Input, ETextCommit::Type);

	//movement settings
	TSharedPtr<SCheckBox> SlideFromRun;
	TSharedPtr<SCheckBox> SingleTapWallDodge;
	TSharedPtr<SCheckBox> SingleTapAfterJump;
	TSharedPtr<SNumericEntryBox<float> > MaxDodgeClickTime;
	TSharedPtr<SNumericEntryBox<float> > MaxDodgeTapTime;
	TSharedPtr<SCheckBox> EnableDoubleTapDodge;
	TSharedPtr<SCheckBox> DeferFireInput;


	float MaxDodgeClickTimeValue;
	float MaxDodgeTapTimeValue;

	void SetMaxDodgeClickTimeValue(float InMaxDodgeClickTimeValue, ETextCommit::Type)
	{
		MaxDodgeClickTimeValue = InMaxDodgeClickTimeValue;
	}
	void SetMaxDodgeTapTimeValue(float InMaxDodgeTapTimeValue, ETextCommit::Type)
	{
		MaxDodgeTapTimeValue = InMaxDodgeTapTimeValue;
	}
	TOptional<float> GetMaxDodgeClickTimeValue() const
	{
		return TOptional<float>(MaxDodgeClickTimeValue);
	}
	TOptional<float> GetMaxDodgeTapTimeValue() const
	{
		return TOptional<float>(MaxDodgeTapTimeValue);
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void OnKeyBindingChanged( FKey PreviousKey, FKey NewKey, TSharedPtr<FKeyBindTracker> BindingThatChanged, bool bPrimaryKey );

	TSharedPtr <SUTTabButton> KeyboardSettingsTabButton;
	TSharedPtr <SUTTabButton> MouseSettingsTabButton;
	TSharedPtr <SUTTabButton> MovementSettingsTabButton;
	
	TSharedRef<SWidget> BuildKeyboardTab();
	TSharedRef<SWidget> BuildMouseTab();
	TSharedRef<SWidget> BuildMovementTab();

	FSlateColor GetLabelColorAndOpacity(TSharedPtr<FKeyBindTracker> Tracker) const;

	void SaveControlSettings();
	virtual void EmptyBindResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
};

#endif
