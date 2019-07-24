// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SWidgetSwitcher.h"
#include "../Base/SUTDialogBase.h"
#include "../Widgets/SUTTabButton.h"
#include "../Widgets/SUTComboButton.h"
#include "UTLobbyMatchInfo.h"
#include "UTAudioSettings.h"
#include "UTLevelSummary.h"
#include "UTReplicatedGameRuleset.h"
#include "../Panels/SUTCreateGamePanel.h"
#include "UTGameEngine.h"
#include "../Widgets/SUTSlider.h"

#if !UE_SERVER

struct FTabButtonInfo
{
	TSharedPtr<SUTTabButton> Button;
	FName Category;

	FTabButtonInfo()
	{
		Button.Reset();
		Category = NAME_None;
	}

	FTabButtonInfo(TSharedPtr<SUTTabButton> InButton, FName InCategory)
	{
		Button = InButton;
		Category = InCategory;
	}
};

struct FRuleSubsetInfo
{
	TWeakObjectPtr<class AUTReplicatedGameRuleset> Ruleset;
	TSharedPtr<SUTTabButton> Button;
	bool bSelected;

	FRuleSubsetInfo()
	{
		Ruleset.Reset();
		bSelected = false;
		Button.Reset();
	}

	FRuleSubsetInfo(TWeakObjectPtr<class AUTReplicatedGameRuleset> InRuleset, TSharedPtr<SUTTabButton> InButton, bool bInitiallySelected)
	{
		Ruleset = InRuleset;
		Button = InButton;
		bSelected = bInitiallySelected;
	}

};

class UNREALTOURNAMENT_API SUTGameSetupDialog : public SUTDialogBase, public FGCObject
{
private:
	struct FMapPlayListInfo
	{
		TWeakObjectPtr<AUTReplicatedMapInfo> MapInfo;
		
		UTexture2D* MapTexture;
		FSlateDynamicImageBrush* MapImage;
		TSharedPtr<SUTComboButton> Button;
		TSharedPtr<SImage> ImageWidget;
		TSharedPtr<SImage> CheckMark;
		bool bSelected;

		FMapPlayListInfo()
		{
			MapTexture = NULL;
			CheckMark.Reset();
			Button.Reset();
			bSelected = false;
		}

		FMapPlayListInfo(TWeakObjectPtr<AUTReplicatedMapInfo> inMapInfo, bool bInitiallySelected)
		{
			MapInfo = inMapInfo;
			MapTexture = NULL;

			// Create the default Image Brush.  This will be replaced in time.
			MapImage = new FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, FVector2D(256.0, 128.0), NAME_None);
			
			bSelected = bInitiallySelected;
			CheckMark.Reset();
			Button.Reset();
		}

		void SetWidgets(TSharedPtr<SUTComboButton> InButton, TSharedPtr<SImage> InImageWidget, TSharedPtr<SImage> InCheckMark)
		{
			Button = InButton;
			CheckMark = InCheckMark;
			ImageWidget = InImageWidget;

			if (bSelected)
			{
				Button->BePressed();
				CheckMark->SetVisibility(EVisibility::Visible);
			}
			else
			{
				Button->UnPressed();
				CheckMark->SetVisibility(EVisibility::Hidden);
			}
		}

	};

	struct FCompareMapByName	
	{
		FORCEINLINE bool operator()( const FMapPlayListInfo& A, const FMapPlayListInfo& B ) const 
		{
			bool bHasTitleA = !A.MapInfo->Title.IsEmpty();
			bool bHasTitleB = !B.MapInfo->Title.IsEmpty();

			if (bHasTitleA && !bHasTitleB)
			{
				return true;
			}

			if (A.MapInfo->bIsMeshedMap)
			{
				if (!B.MapInfo->bIsMeshedMap)
				{
					return true;
				}
				else if (A.MapInfo->bIsEpicMap && !B.MapInfo->bIsEpicMap)
				{
					return true;
				}
				else if (!A.MapInfo->bIsEpicMap && B.MapInfo->bIsEpicMap)
				{
					return false;
				}
			}
			else if (B.MapInfo->bIsMeshedMap)
			{
				return false;
			}
			else if (A.MapInfo->bIsEpicMap && !B.MapInfo->bIsEpicMap)
			{
				return true;
			}

			return A.MapInfo->Title < B.MapInfo->Title;
		}
	};


public:
	SLATE_BEGIN_ARGS(SUTGameSetupDialog)
	: _DialogTitle(NSLOCTEXT("SUTGameSetupDialog", "Title", "CREATE A MATCH"))
	, _DialogSize(FVector2D(1920, 1080))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _ButtonMask(UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_CANCEL)
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)			
	SLATE_ARGUMENT(TArray<class AUTReplicatedGameRuleset*>, GameRuleSets)
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

	TArray<FMapPlayListInfo> MapPlayList;
	TWeakObjectPtr<class AUTReplicatedGameRuleset> SelectedRuleset;

	// Will return true if this settings dialog is on the custom tab.  
	bool IsCustomSettings()
	{
		return CurrentCategory == FName(TEXT("Custom"));
	}

	void GetCustomGameSettings(FString& GameMode, FString& StartingMap, FString& Description, FString& GameModeName, TArray<FString>&GameOptions, int32& DesiredPlayerCount, int32& bTeamGame);

	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

protected:

	TArray<class AUTReplicatedGameRuleset*> GameRulesets;

	// When created from a hub, this will be valid.
	TWeakObjectPtr<class AUTLobbyMatchInfo> TargetMatchInfo;
	
	TSharedPtr<class SSplitter> HorzSplitter;
	TSharedPtr<SHorizontalBox> TabButtonPanel;

	TSharedPtr<SVerticalBox> MainBox;

	TSharedPtr<SGridPanel> RulesPanel;
	TArray<FTabButtonInfo> Tabs;

	FName CurrentCategory;

	TArray<FRuleSubsetInfo> RuleSubset;

	TSharedPtr<SButton> GameModeButtons;
	TSharedPtr<SButton> MapsButton;

	TSharedPtr<SVerticalBox> MapBox;
	TSharedPtr<SVerticalBox> RulesInfoBox;
	TSharedPtr<SVerticalBox> HideBox;

	TSharedPtr<SUTCreateGamePanel> CustomPanel;
	TSharedPtr<SVerticalBox> CustomBox;

	FText GetMatchRulesTitle() const;
	FText GetMatchRulesDescription() const;

	void BuildCategories();
	void BuildRuleList(FName Category);
	TSharedRef<SWidget> BuildRuleTab(FName Tag);
	FReply OnTabButtonClick(int32 ButtonIndex);
	FReply OnRuleClick(int32 RuleIndex);
	FReply OnMapClick(int32 MapIndex);

	void BuildMapList();
	void BuildMapPanel();

	void OnSubMenuSelect(int32 MenuCmdId, TSharedPtr<SUTComboButton> Sender);
	void TextureLoadComplete(const FName& InPackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result);

	// Will be true if this dialog was opened while connected to a hub.
	bool bHubMenu;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		for (int32 i = 0; i < MapPlayList.Num(); i++)
		{
			if (MapPlayList[i].MapTexture != NULL)
			{
				Collector.AddReferencedObject(MapPlayList[i].MapTexture);
			}
		}
	}

	virtual FReply OnButtonClick(uint16 ButtonID);	

	void OnStoreDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	void SelectMap(int32 MapIndex);
	void OnStoreReturnResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);

	int32 DesiredMapIndex;

	TSharedRef<SWidget> BuildBotSkill();

	virtual void AddButtonsToLeftOfButtonBar(uint32& ButtonCount);
	
	TSharedPtr<SCheckBox> cbRankLocked;
	TSharedPtr<SCheckBox> cbSpectatable;
	TSharedPtr<SCheckBox> cbPrivateMatch;

	TSharedPtr<SUTSlider> sBotSkill;
	TSharedPtr<SCheckBox> cbUseBots;
	TSharedPtr<SCheckBox> cbRequireFull;

	TSharedPtr<SCheckBox> cbHostControl;


	bool bBeginnerMatch;
	bool bUserHasBeenWarned;
	void RankCheckChanged(ECheckBoxState NewState);

	FText GetBotSkillText() const;

	int32 CurrentTabIndex;

	void OnGameNameTextCommited(const FText &NewText,ETextCommit::Type CommitType);
	FText GameName;

	TSharedPtr<SEditableTextBox> GameNameText;

	TSharedRef<SWidget> BuildSessionName();
	TSharedPtr<SHorizontalBox> BotSkillLevelBox;

	bool GetBotSkillEnabled() const;
	FSlateColor GetBotSkillColor() const;

	TSharedPtr<SBox> BotSkillBox;

	EVisibility GetAllowBotsVis() const;
	EVisibility GetBotSkillVis() const;
	EVisibility GetRequireFullVis() const;

public:
	FString GetSelectedMap();
	bool bGameNameChanged;
	FText GetGameNameText() const;

	void ConfigureMatch(ECreateInstanceTypes::Type InstanceType);

	virtual TSharedRef<class SWidget> BuildCustomButtonBar();

};



#endif