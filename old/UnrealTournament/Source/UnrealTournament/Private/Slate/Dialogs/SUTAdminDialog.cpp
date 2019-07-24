// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTAdminDialog.h"
#include "UTRconAdminInfo.h"
#include "../SUWindowsStyle.h"
#include "../SUTUtils.h"

#include "UTGameEngine.h"
#include "UTLobbyMatchInfo.h"
#include "UTLobbyGameState.h"
#include "UTLobbyPC.h"

#if !UE_SERVER

struct FCompareRconPlayerByName	{FORCEINLINE bool operator()( const TSharedPtr< FRconPlayerData> A, const TSharedPtr< FRconPlayerData > B ) const {return ( A->PlayerName < B->PlayerName);}};
struct FCompareBanPlayerByName	{FORCEINLINE bool operator()( const TSharedPtr< FBanInfo> A, const TSharedPtr< FBanInfo> B ) const {return ( A->UserName < B->UserName);}};


void SUTAdminDialog::Construct(const FArguments& InArgs)
{

	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(InArgs._DialogTitle)
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.IsScrollable(false)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(UTDIALOG_BUTTON_OK)
						);


	AdminInfo = InArgs._AdminInfo;
	TSharedPtr<SHorizontalBox> ButtonBox;

	if (DialogContent.IsValid())
	{
		DialogContent->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(10.0,10.0,10.0,10.0)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.Medium"))
			
			]
			+SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox).HeightOverride(42)
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SAssignNew(ButtonBox,SHorizontalBox)
						]
					]
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0)
				[
					SAssignNew(Switcher, SWidgetSwitcher)
				]
			]
		];
	}

	if (Switcher.IsValid())
	{
		AddPlayerPanel(ButtonBox);
		if ( GetPlayerOwner()->GetWorld()->GetGameState<AUTLobbyGameState>() != NULL)
		{
			AddMatchPanel(ButtonBox);
		}
		AddBanPanel(ButtonBox);
	}
}

void SUTAdminDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	//GCing AdminInfo since we will not be able to call AdminInfo->NoLongerNeeded
	if (AdminInfo.IsValid())
	{
		AUTRconAdminInfo* AdminInfoPointer = AdminInfo.Get();
		Collector.AddReferencedObject(AdminInfoPointer);
	}
}

void SUTAdminDialog::AddPlayerPanel(TSharedPtr<SHorizontalBox> ButtonBox)
{
	TSharedPtr<SUTTabButton> TabButton;

	bool bInLobby = GetPlayerOwner()->GetWorld()->GetGameState<AUTLobbyGameState>() != NULL;
	ButtonBox->AddSlot()
	.AutoWidth()
	[
		SAssignNew(TabButton, SUTTabButton)
		.IsToggleButton(true)
		.WidgetTag(0)
		.OnClicked(this, &SUTAdminDialog::ChangeTab, ButtonList.Num())
		.Text( NSLOCTEXT("SUTAdminDialog", "PlayerList","Player List"))
		.ButtonStyle(SUTStyle::Get(), "UT.TabButton")
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
	];

	ButtonList.Add(TabButton);
	TabButton->BePressed();

	Switcher->AddSlot()
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(PlayerList, SListView<TSharedPtr<FRconPlayerData>>)
				.ItemHeight(24)
				.ListItemsSource(&SortedPlayerList)
				.OnGenerateRow(this, &SUTAdminDialog::OnGenerateWidgetForPlayerList)
				.SelectionMode(ESelectionMode::Single)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.Style(SUTStyle::Get(), "UT.List.Header")

					+ SHeaderRow::Column("PlayerName")
						.FillWidth(0.2f)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerName", "Player Name"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("PlayerID")
						.FillWidth(0.4f)
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContentPadding(FMargin(5.0))
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerID", "Player ID"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("PlayerIP")
						.FillWidth(0.2f)
						.HAlignCell(HAlign_Left)
						//OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerIP", "Player IP"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("Rank")
						.HAlignCell(HAlign_Left)
						.FillWidth(0.1f)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerRank", "Rank"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("Spectator")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.FillWidth(0.1f)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "Spectator", "Spectator"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("InMatch")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.FillWidth(0.1f)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "InMatch", "In Instance"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

				)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(42)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(FMargin(5.0f, 0.0f))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::KickBanClicked, false)
							.Text( NSLOCTEXT("SUTAdminDialog", "Kick","KICK USER"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
							.Visibility(this, &SUTAdminDialog::GetSelfVis)
						]
						+SHorizontalBox::Slot()
						.Padding(FMargin(10.0, 0.0))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::KickBanClicked, true)
							.Text( NSLOCTEXT("SUTAdminDialog", "Ban","BAN USER"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
							.Visibility(this, &SUTAdminDialog::GetSelfVis)
						]
						+SHorizontalBox::Slot()
						.Padding(FMargin(10.0,0.0))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::MessageUserClicked)
							.Text( NSLOCTEXT("SUTAdminDialog", "MsgUser","MESSAGE USER"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
						]
						+SHorizontalBox::Slot()
						.Padding(FMargin(10.0,0.0))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::MessageAllUsersClicked)
							.Text( NSLOCTEXT("SUTAdminDialog", "MsgAll","MESSAGE ALL"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
						]
						+SHorizontalBox::Slot()
						[
							SNew(SBorder)
							.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
							[
								SAssignNew(UserMessage, SEditableTextBox)
								.Style(SUTStyle::Get(),"UT.EditBox")
								.ClearKeyboardFocusOnCommit(false)
								.MinDesiredWidth(300.0f)
								.Text(FText::GetEmpty())
							]
						]
					]
				]
			]
		
		]
	];
}

void SUTAdminDialog::AddMatchPanel(TSharedPtr<SHorizontalBox> ButtonBox)
{
	TSharedPtr<SUTTabButton> TabButton;

	bool bInLobby = GetPlayerOwner()->GetWorld()->GetGameState<AUTLobbyGameState>() != NULL;

	ButtonBox->AddSlot()
	.AutoWidth()
	.Padding(FMargin(20.0,0.0,0.0,0.0))
	[
		SAssignNew(TabButton, SUTTabButton)
		.IsToggleButton(true)
		.WidgetTag(1)
		.OnClicked(this, &SUTAdminDialog::ChangeTab, ButtonList.Num())
		.Text( NSLOCTEXT("SUTAdminDialog", "MatchList","Matches"))
		.ButtonStyle(SUTStyle::Get(), "UT.TabButton")
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
	];

	ButtonList.Add(TabButton);

	Switcher->AddSlot()
	[
		SNew(SSplitter )
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.90)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(MatchList, SListView<TSharedPtr<FRconMatchData>>)
				.ItemHeight(24)
				.ListItemsSource(&SortedMatchList)
				.OnGenerateRow(this, &SUTAdminDialog::OnGenerateWidgetForMatchList)
				.OnSelectionChanged(this, &SUTAdminDialog::OnMatchListSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.Style(SUTStyle::Get(), "UT.List.Header")

					+ SHeaderRow::Column("MatchState")
						.HAlignCell(HAlign_Left)
						//OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "MatchState", "State"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("OwnerName")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "OwnerName", "Owner Name"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("Ruleset")
						.HAlignCell(HAlign_Left)
						//OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "Ruleset", "Ruleset"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("Map")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "Map", "Map"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("NumPlayers")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(bInLobby ? NSLOCTEXT("SUTAdminDialog", "NumPlayers", "# Players") : NSLOCTEXT("SUTAdminDialog", "Spectator", "Spectator"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("JIP")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(bInLobby ? NSLOCTEXT("SUTAdminDialog", "JIP", "JIP") : NSLOCTEXT("SUTAdminDialog", "Spectator", "Spectator"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("CanSpec")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(bInLobby ? NSLOCTEXT("SUTAdminDialog", "CanSpec", "Can Spec") : NSLOCTEXT("SUTAdminDialog", "Spectator", "Spectator"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("Flags")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(bInLobby ? NSLOCTEXT("SUTAdminDialog", "Flags", "Flags") : NSLOCTEXT("SUTAdminDialog", "Spectator", "Spectator"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]
				)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(42)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(FMargin(5.0f, 0.0f))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::KillMatch)
							.Text( NSLOCTEXT("SUTAdminDialog", "KillMatch","Kill Match"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
						]
					]
				]
			]
		
		]
		+ SSplitter::Slot()
		.Value(0.1)
		[
			SAssignNew(MatchInfoBox, SHorizontalBox)
		]

	];

	UpdateMatchInfo();
}

void SUTAdminDialog::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	for (int32 i = 0; i < SortedPlayerList.Num(); i++)
	{
		SortedPlayerList[i]->bPendingDelete = true;
	}

	for (int32 i = 0; i < SortedMatchList.Num(); i++)
	{
		SortedMatchList[i]->bPendingDelete = true;
	}

	bool bPlayersNeedUpdate = false;
	if (AdminInfo.IsValid())
	{
		for (int32 i = 0; i < AdminInfo->PlayerData.Num(); i++)
		{
			bool bFound = false;
			for (int32 j = 0; j < SortedPlayerList.Num(); j++)
			{
				if (SortedPlayerList[j]->PlayerID == AdminInfo->PlayerData[i].PlayerID)
				{
					bFound = true;
					SortedPlayerList[j]->bPendingDelete = false;
					break;
				}
			}

			if (!bFound)
			{
				bPlayersNeedUpdate = true;
				SortedPlayerList.Add( FRconPlayerData::Make(AdminInfo->PlayerData[i]));
			}
		}

		for (int32 i = SortedPlayerList.Num()-1; i >=0 ;i--)
		{
			if (SortedPlayerList[i]->bPendingDelete)
			{
				SortedPlayerList.RemoveAt(i,1);
				bPlayersNeedUpdate = true;
			}
		}

		if (bPlayersNeedUpdate) PlayerList->RequestListRefresh();


		if (SortedBanList.Num() != AdminInfo->BanData.Num())
		{
			SortedBanList.Empty();
			for (int32 i=0; i < AdminInfo->BanData.Num(); i++)
			{
				SortedBanList.Add( FBanInfo::Make(AdminInfo->BanData[i]));
			}
			BanList->RequestListRefresh();
		}

	}


	AUTLobbyGameState* LobbyGameState = GetPlayerOwner()->GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		bool bNeedsMatchUpdate = false;

		// Update the Match Infos	

		for (int32 i=0; i < LobbyGameState->AvailableMatches.Num();i++)
		{
			bool bFound = false;
			for (int32 j=0; j < SortedMatchList.Num(); j++)
			{
				if (SortedMatchList[j].IsValid() && LobbyGameState->AvailableMatches[i] && SortedMatchList[j]->MatchInfo.Get() == LobbyGameState->AvailableMatches[i])
				{
					bFound = true;
					SortedMatchList[j]->bPendingDelete = false;
					break;
				}
			}

			if (!bFound)
			{
				SortedMatchList.Add(FRconMatchData::Make(LobbyGameState->AvailableMatches[i]));
				bNeedsMatchUpdate = true;
			}
		}

		for (int32 i = SortedMatchList.Num()-1; i >= 0 ; i--)
		{
			if (SortedMatchList[i].IsValid() && SortedMatchList[i]->bPendingDelete)
			{
				SortedMatchList.RemoveAt(i,1);
				bNeedsMatchUpdate = true;
			}
		}
	
		if (bNeedsMatchUpdate) MatchList->RequestListRefresh();
	}

}

FReply SUTAdminDialog::KickBanClicked(bool bBan)
{
	TArray<TSharedPtr<FRconPlayerData>> Selected = PlayerList->GetSelectedItems();
	if (Selected.Num() > 0)
	{
		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetPlayerOwner()->PlayerController);
		if (PC)
		{
			PC->RconKick(Selected[0]->PlayerID, bBan);
		}
	}
	return FReply::Handled();
}


FReply SUTAdminDialog::MessageUserClicked()
{
	TArray<TSharedPtr<FRconPlayerData>> Selected = PlayerList->GetSelectedItems();
	if (Selected.Num() > 0)
	{
		FString Message = UserMessage->GetText().ToString();
		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetPlayerOwner()->PlayerController);
		if (PC)
		{
			PC->RconMessage(Selected[0]->PlayerID, Message);
		}
	}
	return FReply::Handled();
}



FReply SUTAdminDialog::MessageAllUsersClicked()
{
	FString Message = UserMessage->GetText().ToString();
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetPlayerOwner()->PlayerController);
	if (PC)
	{
		PC->RconMessage(TEXT(""), Message);
	}

	return FReply::Handled();
}


TSharedRef<ITableRow> SUTAdminDialog::OnGenerateWidgetForPlayerList( TSharedPtr<FRconPlayerData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SAdminPlayerRow, OwnerTable).PlayerData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

TSharedRef<ITableRow> SUTAdminDialog::OnGenerateWidgetForMatchList( TSharedPtr<FRconMatchData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SAdminMatchRow, OwnerTable).MatchData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

FReply SUTAdminDialog::ChangeTab(int32 WidgetTag)
{
	Switcher->SetActiveWidgetIndex(WidgetTag);	 
	for (int32 i=0; i< ButtonList.Num(); i++)
	{
		if (ButtonList[i]->WidgetTag != WidgetTag)
		{
			ButtonList[i]->UnPressed();
		}
		else
		{
			ButtonList[i]->BePressed();
		}
		
	}

	return FReply::Handled();
}


void SUTAdminDialog::OnDialogClosed()
{
	if (AdminInfo.IsValid())
	{
		AdminInfo->NoLongerNeeded();
	}

	GetPlayerOwner()->AdminDialogClosed();
}

void SUTAdminDialog::OnMatchListSelectionChanged(TSharedPtr<FRconMatchData> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateMatchInfo();
}


void SUTAdminDialog::UpdateMatchInfo()
{
	if (!MatchInfoBox.IsValid()) return;
	MatchInfoBox->ClearChildren();

	// Create the player list..

	TSharedPtr<SVerticalBox> VertBox;
	SAssignNew(VertBox,SVerticalBox);

	TArray<FMatchPlayerListStruct> ColumnA;
	TArray<FMatchPlayerListStruct> ColumnB;
	FString Spectators;

	TArray<int32> Scores;
	float GameTime = 0.0f;

	// Holds a list of mutators running on this map.
	FString RulesList = TEXT("");

	bool bTeamGame = false;

	TArray<TSharedPtr<FRconMatchData>> Selected = MatchList->GetSelectedItems();
	if (Selected.Num() < 1) return;

	AUTLobbyMatchInfo* MatchInfo = Selected[0]->MatchInfo.Get();
	if (MatchInfo)
	{
		MatchInfo->FillPlayerColumnsForDisplay(ColumnA, ColumnB, Spectators);
		if (MatchInfo->CurrentRuleset.IsValid())
		{
			bTeamGame = MatchInfo->CurrentRuleset->Data.bTeamGame;
			RulesList = MatchInfo->CurrentRuleset->Data.Description;
		}
		else if (MatchInfo->bDedicatedMatch)
		{
			bTeamGame = MatchInfo->bDedicatedTeamGame;
			RulesList = MatchInfo->DedicatedServerDescription;
		}

		Scores = MatchInfo->MatchUpdate.TeamScores;
		GameTime = MatchInfo->MatchUpdate.GameTime;
	}

	// Generate a info widget from the UTLobbyMatchInfo

	int32 Max = FMath::Max<int32>(ColumnA.Num(), ColumnB.Num());

	// First we add the player list...

	if (bTeamGame)
	{
		VertBox->AddSlot()
		.Padding(0.0,0.0,0.0,5.0)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(0.0,0.0,5.0,0.0)
			.AutoWidth()
			[
				SNew(SBox).WidthOverride(200)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight().Padding(5.0f,0.0f,0.0f,0.0f)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMatchPanel","RedTeam","Red Team"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
					]
				]
			]
			+SHorizontalBox::Slot()
			.Padding(5.0,0.0,0.0,0.0)
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SBox).WidthOverride(200)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight().Padding(0.0f,0.0f,5.0f,0.0f)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMatchPanel","BlueTeam","Blue Team"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
					]
				]
			]
		];
	}
	else
	{
		VertBox->AddSlot()
		.Padding(0.0,0.0,0.0,5.0)
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SUTMatchPanel","Players","Players in Match"))
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
		];
	}

	TSharedPtr<SBox> ListBox;
	TSharedPtr<SVerticalBox> ListVBox;
	SAssignNew(ListBox, SBox).MinDesiredHeight(150)
	[
		SAssignNew(ListVBox, SVerticalBox)				
	];

	if (Max > 0)
	{
		for (int32 i = 0; i < Max; i++)
		{
			ListVBox->AddSlot()
			.AutoHeight()
			.Padding(0.0,0.0,0.0,2.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0.0,0.0,5.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush( (i%2==0 ? FName(TEXT("UT.ListBackground.Even")) : FName(TEXT("UT.ListBackground.Odd")))  ))
						]
						+SOverlay::Slot()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight()
								.Padding(5.0,0.0,0.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnA.Num() ? ColumnA[i].PlayerName : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
								.Padding(0.0,0.0,5.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnA.Num() ? ColumnA[i].PlayerScore : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
						]
					]
				]
				+SHorizontalBox::Slot()
				.Padding(5.0,0.0,0.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush( (i%2==0 ? FName(TEXT("UT.ListBackground.Even")) : FName(TEXT("UT.ListBackground.Odd")))  ))
						]
						+SOverlay::Slot()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight()
								.Padding(5.0,0.0,0.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnB.Num() ? ColumnB[i].PlayerName : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
								.Padding(0.0,0.0,5.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnB.Num() ? ColumnB[i].PlayerScore : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]
							]
						]
					]
				]
			];
		}
	}
	else
	{
		ListVBox->AddSlot()
		.AutoHeight()
		.Padding(0.0,0.0,0.0,2.0)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(0.0,0.0,5.0,0.0)
			.FillWidth(1.0)
			.HAlign(HAlign_Center)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight()
				.Padding(5.0,0.0,0.0,0.0)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUTMatchPanel","NoPlayers","No one is playing, join now!"))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
				]
			]
		];
	}

	VertBox->AddSlot().AutoHeight()
	[
		ListBox.ToSharedRef()
	];

	if (!Spectators.IsEmpty())
	{
		// Add Spectators

		VertBox->AddSlot()
		.HAlign(HAlign_Center)
		.Padding(0.0f,10.0f,0.0f,5.0)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SUTMatchPanel","Spectators","Spectators"))
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
		];

		VertBox->AddSlot()
		.HAlign(HAlign_Center)
		.Padding(5.0f,0.0f,5.0f,5.0)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Spectators))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny")
				.AutoWrapText(true)
			]
		];
	}


	if (Scores.Num() == 2)
	{
		FText ScoreText = FText::Format( NSLOCTEXT("SUTMatchPanel","ScoreTextFormat","<UT.Font.TeamScore.Red>{0}</> - <UT.Font.TeamScore.Blue>{1}</>"), FText::AsNumber(Scores[0]), FText::AsNumber(Scores[1]) );
		VertBox->AddSlot()
		.AutoHeight()
		.Padding(0.0,10.0,0.0,0.0)
		.HAlign(HAlign_Center)
		[
			SNew(SRichTextBlock)
			.Text(ScoreText)
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large")
			.Justification(ETextJustify::Center)
			.DecoratorStyleSet(&SUTStyle::Get())
			.AutoWrapText(false)
		];
	}

	VertBox->AddSlot()
	.Padding(0.0,0.0,0.0,10.0)
	.AutoHeight()
	.HAlign(HAlign_Center)
	[
		SNew(STextBlock)
		.Text(UUTGameEngine::ConvertTime(FText::GetEmpty(), FText::GetEmpty(), GameTime, true, true, true))
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
	];



	if (!RulesList.IsEmpty())
	{
		VertBox->AddSlot().AutoHeight().Padding(0.0,0.0,0.0,5.0).HAlign(HAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperLight"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUTMatchPanel","RulesTitle","Game Rules"))
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tiny.Bold")
				]
			]
		];

		VertBox->AddSlot().AutoHeight().Padding(5.0,0.0,5.0,5.0)
		[
			SNew(SRichTextBlock)
			.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tiny")
			.Justification(ETextJustify::Left)
			.DecoratorStyleSet( &SUWindowsStyle::Get() )
			.AutoWrapText( true )
			.Text(FText::FromString(RulesList))
		];
	}

	MatchInfoBox->AddSlot()
	[
		VertBox.ToSharedRef()
	];
}

FReply SUTAdminDialog::KillMatch()
{
	TArray<TSharedPtr<FRconMatchData>> Selected = MatchList->GetSelectedItems();
	if (Selected.Num() < 1) return FReply::Handled();

	AUTLobbyMatchInfo* MatchInfo = Selected[0]->MatchInfo.Get();
	AUTLobbyPC* PC = Cast<AUTLobbyPC>(GetPlayerOwner()->PlayerController);
	if (PC && MatchInfo)
	{
		PC->ServerRconKillMatch(MatchInfo);		
	}
	return FReply::Handled();
}


void SUTAdminDialog::AddBanPanel(TSharedPtr<SHorizontalBox> ButtonBox)
{
	if (!AdminInfo.IsValid()) return;

	TSharedPtr<SUTTabButton> TabButton;

	ButtonBox->AddSlot()
	.Padding(FMargin(20.0,0.0,0.0,0.0))
	.AutoWidth()
	[
		SAssignNew(TabButton, SUTTabButton)
		.IsToggleButton(true)
		.WidgetTag(2)
		.OnClicked(this, &SUTAdminDialog::ChangeTab, ButtonList.Num())
		.Text( NSLOCTEXT("SUTAdminDialog", "BanList","Ban List"))
		.ButtonStyle(SUTStyle::Get(), "UT.TabButton")
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
	];

	ButtonList.Add(TabButton);

	Switcher->AddSlot()
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(BanList, SListView<TSharedPtr<FBanInfo>>)
				.ItemHeight(24)
				.ListItemsSource(&SortedBanList)
				.OnGenerateRow(this, &SUTAdminDialog::OnGenerateWidgetForBanList)
				.SelectionMode(ESelectionMode::Single)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.Style(SUTStyle::Get(), "UT.List.Header")

					+ SHeaderRow::Column("BanListPlayerName")
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerName", "Player Name"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]

					+ SHeaderRow::Column("BanListUniqueID")
						.HAlignCell(HAlign_Left)
						//.OnSort(this, &SUTServerBrowserPanel::OnSort)
						.HeaderContentPadding(FMargin(5.0))
						.HeaderContent()
						[
							SNew(SHorizontalBox)+SHorizontalBox::Slot().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTAdminDialog", "PlayerID", "Player ID"))
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
							]
						]
				)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(42)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(FMargin(5.0f, 0.0f))
						.AutoWidth()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTAdminDialog::RemoveBanClicked)
							.Text( NSLOCTEXT("SUTAdminDialog", "RemoveBan","REMOVE BAN"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton")
						]
					]
				]
			]
		
		]
	];
}

TSharedRef<ITableRow> SUTAdminDialog::OnGenerateWidgetForBanList( TSharedPtr<FBanInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SAdminBanRow, OwnerTable).BanData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

FReply SUTAdminDialog::RemoveBanClicked()
{
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetPlayerOwner()->PlayerController);
	if (PC != nullptr && BanList.IsValid() )
	{
		TArray<TSharedPtr<FBanInfo>> SelectedBans = BanList->GetSelectedItems();
		if (SelectedBans.Num() > 0)
		{
			PC->RconUnban(SelectedBans[0]->UniqueID);
			if (AdminInfo.IsValid())
			{
				for (int32 i=0; i < AdminInfo->BanData.Num(); i++)
				{
					if (AdminInfo->BanData[i].UniqueID.Equals(SelectedBans[0]->UniqueID, ESearchCase::IgnoreCase))
					{
						AdminInfo->BanData.RemoveAt(i);
						break;
					}
				}
			}

			SortedBanList.Remove(SelectedBans[0]);
		}
		BanList->RebuildList();
	}


	return FReply::Handled();
}

EVisibility SUTAdminDialog::GetSelfVis() const
{
	AUTPlayerState* UTPlayerState = PlayerOwner.IsValid() && PlayerOwner->PlayerController ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : nullptr;
	TArray<TSharedPtr<FRconPlayerData>> Selected = PlayerList->GetSelectedItems();
	if (UTPlayerState && Selected.Num() > 0) 
	{
		if (Selected[0]->PlayerID == UTPlayerState->UniqueId.ToString())
		{
			return EVisibility::Collapsed;
		}
	}

	return EVisibility::Visible;

}

#endif