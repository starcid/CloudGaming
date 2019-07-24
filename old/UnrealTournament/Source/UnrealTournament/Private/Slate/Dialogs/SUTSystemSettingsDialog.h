// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SWidgetSwitcher.h"
#include "../Base/SUTDialogBase.h"
#include "../Widgets/SUTTabButton.h"
#include "UTAudioSettings.h"

#if !UE_SERVER

class SSlateConsoleVarDelegate : public TSharedFromThis<SSlateConsoleVarDelegate>
{
	const TCHAR* VarName;
	IConsoleVariable* CVar;
	const FString StartingValue;
	const FVector2D ValueRange;
	const bool bIntegerValue;
	// if >= 0, calls to Set*() to ValueRange.X will set it to this instead
	const float SpecialMinValue;
public:
	SSlateConsoleVarDelegate(const TCHAR* InCVarName, const FVector2D& InValueRange = FVector2D::ZeroVector, float InSpecialMinValue = -1.0f)
		: VarName(InCVarName), CVar(IConsoleManager::Get().FindConsoleVariable(InCVarName)), StartingValue(CVar->GetString()), ValueRange(InValueRange), bIntegerValue(CVar->AsVariableInt() != NULL), SpecialMinValue(InSpecialMinValue)
	{
		CVar->ClearFlags(ECVF_SetByConsoleVariablesIni); // workaround for badly thought out priority system
	}

	bool IsInteger() const
	{
		return bIntegerValue;
	}

	inline FText GetTooltip() const
	{
		return FText::FromString(FString(CVar->GetHelp()));
	}
	inline const TCHAR* GetVarName() const
	{
		return VarName;
	}

	void RestoreValue() const
	{
		CVar->Set(*StartingValue);
		CVar->ClearFlags(ECVF_SetByCode);
	}

	float GetFloat() const
	{
		return CVar->GetFloat();
	}
	void SetFloat(float InValue)
	{
		if (InValue == ValueRange.X && SpecialMinValue >= 0.0f)
		{
			InValue = SpecialMinValue;
		}
		CVar->Set(InValue);
	}
	int32 GetInt() const
	{
		return CVar->GetInt();
	}
	void SetInt(int32 InValue)
	{
		if (InValue == FMath::TruncToInt(ValueRange.X) && SpecialMinValue >= 0.0f)
		{
			InValue = FMath::TruncToInt(SpecialMinValue);
		}
		CVar->Set(InValue);
	}
	bool GetBool() const
	{
		return (CVar->GetInt() != 0);
	}
	void SetBool(bool InValue)
	{
		CVar->Set(InValue ? 1 : 0);
	}
	FString GetString() const
	{
		return CVar->GetString();
	}
	void SetString(const FString& InValue)
	{
		CVar->Set(*InValue);
	}
	ECheckBoxState GetCheckbox() const
	{
		return GetBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	void SetCheckbox(ECheckBoxState NewState)
	{
		SetBool(NewState == ECheckBoxState::Checked);
	}
	/** used when value is on a slider */
	void SetFromSlider(float InSliderValue)
	{
		float AdjustedValue = ValueRange.IsZero() ? InSliderValue : FMath::Lerp<float>(ValueRange.X, ValueRange.Y, InSliderValue);
		if (InSliderValue <= 0.0f && SpecialMinValue > 0.0f)
		{
			AdjustedValue = SpecialMinValue;
		}
		if (bIntegerValue)
		{
			CVar->Set(FMath::RoundToInt(AdjustedValue));
		}
		else
		{
			CVar->Set(AdjustedValue);
		}
	}
	float GetForSlider() const
	{
		float Value = CVar->GetFloat();
		if (SpecialMinValue >= 0.0f && Value == SpecialMinValue)
		{
			return 0.0f;
		}
		else
		{
			return ValueRange.IsZero() ? Value : ((Value - ValueRange.X) / (ValueRange.Y - ValueRange.X));
		}
	}
};

class UNREALTOURNAMENT_API SUTSystemSettingsDialog : public SUTDialogBase
{
public:
	SLATE_BEGIN_ARGS(SUTSystemSettingsDialog)
	: _DialogSize(FVector2D(1100,900))
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

	bool bAdvancedMode;
	FDelegateHandle AutodetectHandle;

	virtual TSharedRef<class SWidget> BuildCustomButtonBar() override;

	TArray< TSharedPtr<FString> > VOIPInputOptions;
	TSharedPtr<STextBlock> VOIPInputOptionsText;
	int32 VOIPInputOptionsInitialSelection;

	TArray< TSharedPtr<FString> > VOIPOptions;
	TSharedPtr<STextBlock> VOIPOptionsText;

	TArray< TSharedPtr<FString> > ResList;
	TSharedPtr<STextBlock> SelectedRes;
	TArray< TSharedPtr<FString> > DisplayModeList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > DisplayModeComboBox;
	TSharedPtr<STextBlock> SelectedDisplayMode;
	TSharedPtr<SCheckBox> SmoothFrameRate;
	TSharedPtr<SCheckBox> VSync;
	TSharedPtr<SEditableTextBox> FrameRateCap;
	TArray< TSharedPtr<FString> > MatchmakingRegionList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > MatchmakingRegion;
	TSharedPtr<STextBlock> SelectedMatchmakingRegion;
	bool bChangedMatchmakingRegion;
	TSharedPtr<SCheckBox> KeyboardLightingCheckbox;

	TArray<TSharedPtr<FString>> BotSpeechList;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> BotSpeechCombo;
	TSharedPtr<STextBlock> SelectedBotSpeech;
	void OnBotSpeechSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TSharedPtr<SCheckBox> VoiceChatCheckBox;
	TSharedPtr<SSlider> VoiceChatPlaybackVolume;
	TSharedPtr<STextBlock> VoiceChatPlaybackVolumeLabel;
	void OnVoiceChatPlaybackVolumeChanged(float NewValue);

	TSharedPtr<SCheckBox> HRTFCheckBox;
	TSharedPtr<SSlider> SoundVolumes[EUTSoundClass::MAX];
	TSharedPtr<STextBlock> SoundVolumesLabels[EUTSoundClass::MAX];

	TSharedPtr<SComboBox< TSharedPtr<FString> > >PushToTalkCombo;
	TSharedPtr<SComboBox< TSharedPtr<FString> > >VoiceInputDeviceCombo;

	void OnSoundVolumeChangedMaster(float NewValue);
	void OnSoundVolumeChangedMusic(float NewValue);
	void OnSoundVolumeChangedGameMusic(float NewValue);
	void OnSoundVolumeChangedStingers(float NewValue);
	void OnSoundVolumeChangedSFX(float NewValue);
	void OnSoundVolumeChangedVoice(float NewValue);
	void OnSoundVolumeChangedVOIP(float NewValue);

	/** list of display values for general scalability setting that are all set the same way (e.g. low/medium/high) */
	TArray< TSharedPtr<FString> > GeneralScalabilityList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > TextureRes;
	TSharedPtr<STextBlock> SelectedTextureRes;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > ViewDistance;
	TSharedPtr<STextBlock> SelectedViewDistance;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > ShadowQuality;
	TSharedPtr<STextBlock> SelectedShadowQuality;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > PPQuality;
	TSharedPtr<STextBlock> SelectedPPQuality;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > EffectQuality;
	TSharedPtr<STextBlock> SelectedEffectQuality;
	TSharedPtr<SSlider> DecalLifetime;
	TSharedPtr<STextBlock> DecalLifetimeLabel;

	TArray< TSharedPtr<FString> > AAModeList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > AAMode;
	TSharedPtr<STextBlock> SelectedAAMode;

	TSharedPtr<SSlider> ScreenPercentageSlider;
	TSharedPtr<STextBlock> ScreenPercentageLabel;
	FVector2D ScreenPercentageRange;

	TArray<TSharedRef<SSlateConsoleVarDelegate>> CVarDelegates;

	TSharedPtr< SComboBox< TSharedPtr<FString> > > ComFilterCombo;
	TArray<TSharedPtr<FString> > ComFilterItems;
	TSharedPtr<STextBlock> SelectedComFilter;

	TEnumAsByte<EComFilter::Type> GetComFilterSelection();

	// widgets only shown in advanced mode
	TArray<TSharedRef<SWidget>> AdvancedWidgets;

	/** range of values for decal lifetime which will be normalized to 0.0 - 1.0 for the slider widget
	 * note that the max value (1.0 on the slider) becomes infinite lifetime
	 */
	FVector2D DecalLifetimeRange;

	TSharedPtr<SWidgetSwitcher> TabWidget;
	TSharedPtr<SUTTabButton> GeneralSettingsTabButton;
	TSharedPtr<SUTTabButton> GraphicsSettingsTabButton;
	TSharedPtr<SUTTabButton> AudioSettingsTabButton;

	TSharedRef<SWidget> BuildGeneralTab();
	TSharedRef<SWidget> BuildGraphicsTab();
	TSharedRef<SWidget> BuildAudioTab();

	FReply OnTabClickGeneral();
	FReply OnTabClickGraphics();
	FReply OnTabClickAudio();

	virtual FReply OnButtonClick(uint16 ButtonID);	

	FReply OnAutodetectClick();
	FReply OKClick();
	FReply CancelClick();
	void OnResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnDisplayModeSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnTextureResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnViewDistanceSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnShadowQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnPPQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnEffectQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnMatchmakingRegionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnVOIPChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnVOIPInputChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnAdvancedCheckChanged(ECheckBoxState NewState);
	void UpdateAdvancedWidgets();

	void OnAAModeSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	int32 ConvertAAModeToComboSelection(int32 AAMode);
	int32 ConvertComboSelectionToAAMode(const FString& Selection);

	void OnDecalLifetimeChange(float NewValue);
	FString GetDecalLifetimeLabelText(float SliderValue);

	void OnScreenPercentageChange(float NewValue);
	FString GetScreenPercentageLabelText(float SliderValue);

	void OnSettingsAutodetected(const Scalability::FQualityLevels& DetectedQuality);

	FText GetVSyncText() const;

	EVisibility AutoDetectSettingsVisibility() const;

	SVerticalBox::FSlot& AddSectionHeader(const FText& SectionDesc);
	SVerticalBox::FSlot& AddGeneralScalabilityWidget(const FString& Desc, TSharedPtr< SComboBox< TSharedPtr<FString> > >& ComboBox, TSharedPtr<STextBlock>& SelectedItemWidget, void (SUTSystemSettingsDialog::*SelectionFunc)(TSharedPtr<FString>, ESelectInfo::Type), int32 SettingValue, const TAttribute<FText>& TooltipText = TAttribute<FText>());
	SVerticalBox::FSlot& AddAAModeWidget(const FString& Desc, TSharedPtr< SComboBox< TSharedPtr<FString> > >& ComboBox, TSharedPtr<STextBlock>& SelectedItemWidget, void (SUTSystemSettingsDialog::*SelectionFunc)(TSharedPtr<FString>, ESelectInfo::Type), int32 SettingValue, const TAttribute<FText>& TooltipText = TAttribute<FText>());
	SVerticalBox::FSlot& AddGeneralSliderWidget(const FString& Desc, TSharedPtr<SSlider>& SliderWidget, float SettingValue, const TAttribute<FText>& TooltipText = TAttribute<FText>());
	SVerticalBox::FSlot& AddConsoleVarSliderWidget(TSharedRef<SSlateConsoleVarDelegate> CVar, const FText& Label);
	SVerticalBox::FSlot& AddConsoleVarCheckboxWidget(TSharedRef<SSlateConsoleVarDelegate> CVar, const FText& Label);
	SVerticalBox::FSlot& AddGeneralSliderWithLabelWidget(TSharedPtr<SSlider>& SliderWidget, TSharedPtr<STextBlock>& LabelWidget, void(SUTSystemSettingsDialog::*SelectionFunc)(float), const FString& InitialLabel, float SettingValue, const TAttribute<FText>& TooltipText = TAttribute<FText>());
	SVerticalBox::FSlot& AddTopLevelConsoleVarCheckboxWidget(TSharedRef<SSlateConsoleVarDelegate> CVar, const FText& Label);

	void OnComFilterSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

};

#endif