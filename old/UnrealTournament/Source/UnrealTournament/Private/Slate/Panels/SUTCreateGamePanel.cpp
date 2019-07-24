// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTCreateGamePanel.h"
#include "SUWindowsStyle.h"
#include "UTDMGameMode.h"
#include "AssetData.h"
#include "UTLevelSummary.h"
#include "../Widgets/SUTScaleBox.h"
#include "UTMutator.h"
#include "UTGameEngine.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "UTAnalytics.h"
#include "UTLobbyGameState.h"
#include "PartyContext.h"
#include "BlueprintContextLibrary.h"
#include "../SUTUtils.h"

#if !UE_SERVER

#include "UserWidget.h"

void SUTCreateGamePanel::ConstructPanel(FVector2D ViewportSize)
{
	AUTGameState* GameState = GetPlayerOwner()->GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		GameState->GetAvailableGameData(AllGametypes, MutatorListAvailable);
	}

	// Set the Initial Game Type, restoring the previous selection if possible.
	TSubclassOf<AUTGameMode> InitialSelectedGameClass = AUTDMGameMode::StaticClass();
	FString LastGametypePath;
	if (GConfig->GetString(TEXT("CreateGameDialog"), TEXT("LastGametypePath"), LastGametypePath, GGameIni) && LastGametypePath.Len() > 0)
	{
		for (UClass* GameType : AllGametypes)
		{
			if (GameType->GetPathName() == LastGametypePath)
			{
				InitialSelectedGameClass = GameType;
				break;
			}
		}
	}

	// Set the initial selected mutators if possible.
	TArray<FString> LastMutators;
	if (GConfig->GetArray(TEXT("CreateGameDialog"), TEXT("LastMutators"), LastMutators, GGameIni))
	{
		for (const FString& MutPath : LastMutators)
		{
			for (UClass* MutClass : MutatorListAvailable)
			{
				if (MutClass->GetPathName() == MutPath)
				{
					MutatorListEnabled.Add(MutClass);
					MutatorListAvailable.Remove(MutClass);
					break;
				}
			}
		}
	}

	TSharedPtr<SVerticalBox> MainBox;
	TSharedPtr<SUniformGridPanel> ButtonRow;

	LevelScreenshot = new FSlateDynamicImageBrush(GEngine->DefaultTexture, FVector2D(256.0f, 128.0f), NAME_None);

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(0.0f, 0.0f, 0.0f, 5.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[

			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.VAlign(VAlign_Fill)
			[
				BuildGamePanel(InitialSelectedGameClass)
			]
		]
	];
}

SUTCreateGamePanel::~SUTCreateGamePanel()
{
	if (LevelScreenshot != NULL)
	{
		delete LevelScreenshot;
		LevelScreenshot = NULL;
	}
	if (MutatorConfigMenu.IsValid())
	{
		MutatorConfigMenu->RemoveFromViewport();
	}
}

TSharedRef<SWidget> SUTCreateGamePanel::BuildGamePanel(TSubclassOf<AUTGameMode> InitialSelectedGameClass)
{
	SAssignNew(GamePanel,SVerticalBox)
	+ SVerticalBox::Slot().FillHeight(1.0f)
	.Padding(FMargin(60.0f, 20.0f, 60.0f, 20.0f))
	[
		SNew(SVerticalBox)

		// Game type and map selection
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0, 0, 0, 15))
		[
			// Selection Controls
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(500)
				[
					// Game Type
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0, 0, 0, 20))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign( VAlign_Center )
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(200)
							[
								SNew(STextBlock)
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
								.Text(NSLOCTEXT("SUTCreateGamePanel", "Gametype", "Game Type:"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(10.0f,0.0f,0.0f,0.0f))
						[
							SNew(SBox)
							.WidthOverride(290)
							[
								SAssignNew(GameList, SComboBox<UClass*>)
								.InitiallySelectedItem(InitialSelectedGameClass)
								.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
								.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
								.OptionsSource(&AllGametypes)
								.OnGenerateWidget(this, &SUTCreateGamePanel::GenerateGameNameWidget)
								.OnSelectionChanged(this, &SUTCreateGamePanel::OnGameSelected)
								.Content()
								[
									SAssignNew(SelectedGameName, STextBlock)
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
									.Text(InitialSelectedGameClass.GetDefaultObject()->DisplayName)
									.MinDesiredWidth(200.0f)
								]
							]
						]
					]

					// Map
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0, 0, 0, 20))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.FillWidth(.4f)
						[
							SNew(SBox)
							.WidthOverride(200)
							[
								SNew(STextBlock)
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
								.Text(NSLOCTEXT("SUTCreateGamePanel", "Map", "Map:"))
							]
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(10.0f,0.0f,0.0f,0.0f))
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(290)
							[
								SAssignNew(MapList, SComboBox< TWeakObjectPtr<AUTReplicatedMapInfo> >)
								.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
								.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
								.OptionsSource(&AllMaps)
								.OnGenerateWidget(this, &SUTCreateGamePanel::GenerateMapNameWidget)
								.OnSelectionChanged(this, &SUTCreateGamePanel::OnMapSelected)
								.Content()
								[
									SAssignNew(SelectedMap, STextBlock)
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
									.ColorAndOpacity(FSlateColor(FLinearColor(0, 0, 0, 1.0f)))
									.Text(NSLOCTEXT("SUTCreateGamePanel", "NoMaps", "No Maps Available"))
									.MinDesiredWidth(200.0f)
								]
							]
						]
					]
					// Author
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin( 0, 0, 0, 5))
					[
						SAssignNew(MapAuthor, STextBlock)
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
						.Text(FText::Format(NSLOCTEXT("SUTCreateGamePanel", "Author", "Author: {0}"), FText::FromString(TEXT("-"))))
					]
					// Recommended players
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin( 0, 0, 0, 5))
					[
						SAssignNew(MapRecommendedPlayers, STextBlock)
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
					]
				]
			]
			// Map screenshot.
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(20.0f,0.0f,0.0f,0.0f)
				[
					SNew(SBox)
					.WidthOverride(512)
					.HeightOverride(256)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(LevelScreenshot)
						]
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(this, &SUTCreateGamePanel::GetMapBorder)
							.Visibility(this, &SUTCreateGamePanel::GetMapBorderVis)
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot().FillHeight(1.0).VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(SBox).WidthOverride(80.0f).HeightOverride(80.0f)
										[
											SNew(SImage)
											.Image(SUTStyle::Get().GetBrush("UT.Icon.LockedContent"))
											.Visibility(this, &SUTCreateGamePanel::GetLockImageVis)
										]
									]
								]
							]
						]
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox).WidthOverride(750)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(20, 0, 20, 0))
					[
						SNew(SBox)
						.HeightOverride(256)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(MapDesc, STextBlock)
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
								.Text(FText())
								.AutoWrapText(true)
							]
						]
					]
				]
			]
		]

		// Game Settings and mutators
		+SVerticalBox::Slot()
		.Padding(FMargin(0.0f,15.0f,0.0f,0.0f))
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			// Game Settings
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0, 0, 60, 0))
			[
				SNew(SBox)
				.WidthOverride(800).HeightOverride(600)
				[
					SNew(SVerticalBox)
					// Heading
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0, 0, 0, 20))
					[
						SNew(STextBlock)
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.BoldText")
						.Text(NSLOCTEXT("SUTCreateGamePanel", "GameSettings", "Game Settings:"))
					]
					// Game config panel
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(400)
						[
							SAssignNew(GameConfigPanel, SVerticalBox)
						]
					]

					+SVerticalBox::Slot().AutoHeight().Padding(0.0f,50.0f,0.0f,0.0f)
					[
						SNew(SBox).HeightOverride(400)
						[
							SAssignNew(BotSkillBox, SVerticalBox)
						]
					]
				]
			]
			// Mutators
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(30.0f,0.0f,0.0f,0.0f)
			[
				AddMutatorMenu()
			]
		]
	];

	if (MutatorGrid.IsValid())
	{
		MutatorGrid->SetRowFill(0, 1);
		MutatorGrid->SetColumnFill(0, .5);
		MutatorGrid->SetColumnFill(2, .5);
	}

	OnGameSelected(InitialSelectedGameClass, ESelectInfo::Direct);

	return GamePanel.ToSharedRef();
}

TSharedRef<SWidget> SUTCreateGamePanel::AddMutatorMenu()
{
	return SNew(SBox)
		.WidthOverride(700).HeightOverride(400)
		[
			SNew(SVerticalBox)
			// Heading
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 20))
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.BoldText")
				.Text(NSLOCTEXT("SUTCreateGamePanel", "Mutators", "Mutators:"))
			]

			// Mutator enabler
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(350)
				[
					SAssignNew(MutatorGrid, SGridPanel)
					// All mutators
					+ SGridPanel::Slot(0, 0)
					[
						SNew(SBorder)
						[
							SNew(SBorder)
							.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Dark"))
							.Padding(FMargin(5))
							[
								SAssignNew( AvailableMutators, SListView<UClass*> )
								.SelectionMode( ESelectionMode::Single )
								.ListItemsSource( &MutatorListAvailable )
								.OnGenerateRow( this, &SUTCreateGamePanel::GenerateMutatorListRow )
							]
						]
					]
					// Mutator switch buttons
					+ SGridPanel::Slot(1, 0)
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						.Padding(FMargin(10, 0, 10, 0))
						[
							SNew(SVerticalBox)
							// Add button
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0, 0, 0, 10))
							[
								SNew(SButton)
								.HAlign(HAlign_Left)
								.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
								.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
								.OnClicked(this, &SUTCreateGamePanel::AddMutator)
								[
									SNew(STextBlock)
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText.Black")
									.Text(NSLOCTEXT("SUTCreateGamePanel", "MutatorAdd", "-->"))
								]
							]
							// Remove Button
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SButton)
								.HAlign(HAlign_Left)
								.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
								.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
								.OnClicked(this, &SUTCreateGamePanel::RemoveMutator)
								[
									SNew(STextBlock)
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText.Black")
									.Text(NSLOCTEXT("SUTCreateGamePanel", "MutatorRemove", "<--"))
								]
							]
						]
					]
					// Enabled mutators
					+ SGridPanel::Slot(2, 0)
					[
						SNew(SBorder)
						[
							SNew(SBorder)
							.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Dark"))
							.Padding(FMargin(5))
							[
								SAssignNew(EnabledMutators, SListView<UClass*>)
								.SelectionMode(ESelectionMode::Single)
								.ListItemsSource(&MutatorListEnabled)
								.OnGenerateRow(this, &SUTCreateGamePanel::GenerateMutatorListRow)
							]
						]
					]
					// Configure mutator button
					+ SGridPanel::Slot(2, 1)
					.Padding(FMargin(0, 5.0f, 0, 5.0f))
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
						.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
						.OnClicked(this, &SUTCreateGamePanel::ConfigureMutator)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
							.Text(NSLOCTEXT("SUTCreateGamePanel", "ConfigureMutator", "Configure Mutator"))
						]
					]
				]
			]
		];
}

void SUTCreateGamePanel::OnMapSelected(TWeakObjectPtr<AUTReplicatedMapInfo> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedMapInfo = NewSelection;

		SelectedMap->SetText(NewSelection.IsValid() ? FText::FromString(NewSelection->Title) : NSLOCTEXT("SUTCreateGamePanel", "NoMaps", "No Maps Available"));

		int32 OptimalPlayerCount = SelectedGameClass.GetDefaultObject()->bTeamGame ? NewSelection->OptimalTeamPlayerCount : NewSelection->OptimalPlayerCount;
	
		MapAuthor->SetText(FText::Format(NSLOCTEXT("SUTCreateGamePanel", "Author", "Author: {0}"), FText::FromString(NewSelection->Author)));
		MapRecommendedPlayers->SetText(FText::Format(NSLOCTEXT("SUTCreateGamePanel", "OptimalPlayers", "Recommended Players: {0}"), FText::AsNumber(OptimalPlayerCount)));
		MapDesc->SetText(NewSelection->Description);

		if (NewSelection->MapScreenshotReference.IsEmpty())
		{
			*LevelScreenshot = FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
		}
		else
		{
			LevelShot = LoadObject<UTexture2D>(nullptr, *NewSelection->MapScreenshotReference);
			if (LevelShot)
			{
				*LevelScreenshot = FSlateDynamicImageBrush(LevelShot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
			}
			else
			{
				*LevelScreenshot = FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
			}
			
		}

		if (!SelectedMapInfo->bHasRights)
		{
			PlayerOwner->ShowMessage(
				NSLOCTEXT("SUTGameSetupDialog", "RightsTitle", "Epic Store"), 
				NSLOCTEXT("SUTGameSetupDialog", "RightText", "The map you have selected is available in the Epic Store.  Do you wish to load the store to acquire the map?"), 
				UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, 
				FDialogResultDelegate::CreateRaw(this, &SUTCreateGamePanel::OnStoreDialogResult));								
		}
	}
}


void SUTCreateGamePanel::OnStoreDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		FString URL = TEXT("com.epicgames.launcher://ut/marketplace");
		FString Command = TEXT("");
		FString Error = TEXT("");
		FPlatformProcess::LaunchURL(*URL, *Command, &Error);
		FPlatformMisc::RequestMinimize();
		if (PlayerOwner.IsValid())
		{
			PlayerOwner->ShowMessage(
				NSLOCTEXT("SUTGameSetupDialog", "ReturnFromStore", "Returned from store..."), 
				NSLOCTEXT("SUT6GameSetupDialog", "StoreWait", "Click OK when ready."), 
				UTDIALOG_BUTTON_OK, 
				FDialogResultDelegate::CreateRaw(this, &SUTCreateGamePanel::OnStoreReturnResult), FVector2D(0.25f, 0.25f));								
		}
	}
	else if (MapList.IsValid() && AllMaps.Num() > 0)
	{
		for (int32 i=0; i < AllMaps.Num(); i++)
		{
			if (AllMaps[i].IsValid() && AllMaps[i]->bHasRights)
			{
				MapList->SetSelectedItem(AllMaps[i]);
				break;
			}
		}
	}
}

void SUTCreateGamePanel::OnStoreReturnResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	SelectedMapInfo->bHasRights = true;
}


TSharedRef<SWidget> SUTCreateGamePanel::GenerateGameNameWidget(UClass* InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
			.Text(InItem->GetDefaultObject<AUTGameMode>()->DisplayName)
		];
}

TSharedRef<SWidget> SUTCreateGamePanel::GenerateMapNameWidget(TWeakObjectPtr<AUTReplicatedMapInfo> InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
			.Text(FText::FromString(InItem->Title))
		];
}

void SUTCreateGamePanel::OnGameSelected(UClass* NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection == NULL) return;
	if (NewSelection != SelectedGameClass)
	{
		// clear existing game type info and cancel any changes that were made
		if (SelectedGameClass != NULL)
		{
			SelectedGameClass.GetDefaultObject()->ReloadConfig();
			GameConfigPanel->ClearChildren();
			GameConfigProps.Empty();
		}

		int32 MinimumPlayers = 1;
		UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
		if (PartyContext)
		{
			MinimumPlayers = PartyContext->GetPartySize();
		}

		SelectedGameClass = NewSelection;
		SelectedGameClass.GetDefaultObject()->CreateConfigWidgets(GameConfigPanel, false, GameConfigProps, MinimumPlayers);
		SelectedGameName->SetText(SelectedGameClass.GetDefaultObject()->DisplayName.ToString());

		// generate map list
		AUTGameMode* GameDefaults = SelectedGameClass.GetDefaultObject();

		// Get the list of prefixes
		TArray<FString> PrefixList;
		PrefixList.Add(GameDefaults->GetMapPrefix());

		AllMaps.Empty();

		AUTGameState* GameState = GetPlayerOwner()->GetWorld()->GetGameState<AUTGameState>();
		if (GameState)
		{

			AUTLobbyGameState* LobbyGameState = Cast<AUTLobbyGameState>(GameState);
			if (LobbyGameState && GetPlayerOwner()->GetWorld()->GetNetMode() == NM_Client)
			{
				// We are a lobby game this means we should be the client as well.  In this instance, we want to use the
				// collection of replicated MapInfos to get the list of maps that are playable.
				TArray<AUTReplicatedMapInfo*> MapInfos;
				LobbyGameState->GetMapList(PrefixList, MapInfos);
				for (int32 i=0 ; i < MapInfos.Num(); i++)
				{
					if (MapInfos[i])
					{
						AllMaps.Add( MapInfos[i] );
					}
				}
			}
			else
			{
				TArray<FAssetData> MapAssets;
				GameState->ScanForMaps(PrefixList, MapAssets);

				for (int32 i = 0; i < MapAssets.Num(); i++)
				{
					AllMaps.Add( GameState->CreateMapInfo(MapAssets[i] ) );
				}

			}
		}

		AllMaps.Sort([](const TWeakObjectPtr<AUTReplicatedMapInfo>& A, const TWeakObjectPtr<AUTReplicatedMapInfo>& B)
					{
						bool bHasTitleA = !A->Title.IsEmpty();
						bool bHasTitleB = !B->Title.IsEmpty();

						if (bHasTitleA && !bHasTitleB)
						{
							return true;
						}

						if (A->bIsMeshedMap)
						{
							if (!B->bIsMeshedMap)
							{
								return true;
							}
							else if (A->bIsEpicMap && !B->bIsEpicMap)
							{
								return true;
							}
							else if (!A->bIsEpicMap && B->bIsEpicMap)
							{
								return false;
							}
						}
						else if (B->bIsMeshedMap)
						{
							return false;
						}
						else if (A->bIsEpicMap && !B->bIsEpicMap)
						{
							return true;
						}

						return A->Title < B->Title;

					});

		MapList->RefreshOptions();
		if (AllMaps.Num() > 0)
		{
			TWeakObjectPtr<AUTReplicatedMapInfo> DesiredFirstMap;
			DesiredFirstMap.Reset();
			// remember last selection
			for (TWeakObjectPtr<AUTReplicatedMapInfo> TestMap : AllMaps)
			{
				if (TestMap->MapPackageName == SelectedGameClass.GetDefaultObject()->UILastStartingMap)
				{
					DesiredFirstMap = TestMap;
					break;
				}
				else if (!DesiredFirstMap.IsValid() && TestMap->bHasRights)
				{
					DesiredFirstMap = TestMap;
				}
			}

			if (DesiredFirstMap.IsValid())
			{
				MapList->SetSelectedItem(DesiredFirstMap);
			}
		}
		else
		{
			MapList->SetSelectedItem(NULL);
		}
		//OnMapSelected(MapList->GetSelectedItem(), ESelectInfo::Direct);
	}
}
void SUTCreateGamePanel::Cancel()
{
	// revert config changes
	SelectedGameClass.GetDefaultObject()->ReloadConfig();
	AUTGameState::StaticClass()->GetDefaultObject()->ReloadConfig();
	if (MutatorConfigMenu.IsValid())
	{
		MutatorConfigMenu->RemoveFromViewport();
	}
}

TSharedRef<ITableRow> SUTCreateGamePanel::GenerateMutatorListRow(UClass* MutatorType, const TSharedRef<STableViewBase>& OwningList)
{
	checkSlow(MutatorType->IsChildOf(AUTMutator::StaticClass()));

	AUTMutator* DefaultMutatorObj = MutatorType->GetDefaultObject<AUTMutator>();
	FString MutatorName = TEXT("");
	if (DefaultMutatorObj != nullptr)
	{
		MutatorName = MutatorType->GetDefaultObject<AUTMutator>()->DisplayName.IsEmpty() ? MutatorType->GetName() : MutatorType->GetDefaultObject<AUTMutator>()->DisplayName.ToString();
	}

	return SNew(STableRow<UClass*>, OwningList)
		.Padding(5)
		[
			MutatorName.IsEmpty() ? SNullWidget::NullWidget : SNew(STextBlock)
																	.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
																	.Text(FText::FromString(MutatorName))
		]; 
}


FReply SUTCreateGamePanel::AddMutator()
{
	TArray<UClass*> Selection = AvailableMutators->GetSelectedItems();
	if (Selection.Num() > 0 && Selection[0] != NULL)
	{
		const AUTMutator* ConflictCDO = NULL;
		const AUTMutator* NewMutatorCDO = Selection[0]->GetDefaultObject<AUTMutator>();
		for (UClass* EnabledMut : MutatorListEnabled)
		{
			const AUTMutator* EnabledMutCDO = EnabledMut->GetDefaultObject<AUTMutator>();
			if (EnabledMutCDO->GroupNames.ContainsByPredicate([&](const FName& TestName) { return NewMutatorCDO->GroupNames.Contains(TestName); }))
			{
				ConflictCDO = EnabledMutCDO;
				break;
			}
		}
		if (ConflictCDO != NULL)
		{
			GetPlayerOwner()->ShowMessage(NSLOCTEXT("SUTCreateGamePanel", "MutatorConflictTitle", "Mutator Conflict"), FText::Format(NSLOCTEXT("SUTCreateGamePanel", "MutatorConflictText", "The selected mutator conflicts with the already enabled mutator \"{0}\""), ConflictCDO->DisplayName), UTDIALOG_BUTTON_OK);
		}
		else
		{
			MutatorListEnabled.Add(Selection[0]);
			MutatorListAvailable.Remove(Selection[0]);
			AvailableMutators->RequestListRefresh();
			EnabledMutators->RequestListRefresh();
		}
	}
	return FReply::Handled();
}
FReply SUTCreateGamePanel::RemoveMutator()
{
	TArray<UClass*> Selection = EnabledMutators->GetSelectedItems();
	if (Selection.Num() > 0 && Selection[0] != NULL)
	{
		MutatorListAvailable.Add(Selection[0]);
		MutatorListEnabled.Remove(Selection[0]);
		AvailableMutators->RequestListRefresh();
		EnabledMutators->RequestListRefresh();
	}
	return FReply::Handled();
}
FReply SUTCreateGamePanel::ConfigureMutator()
{
	if (!MutatorConfigMenu.IsValid() || !MutatorConfigMenu->IsInViewport())
	{
		TArray<UClass*> Selection = EnabledMutators->GetSelectedItems();
		if (Selection.Num() > 0 && Selection[0] != NULL)
		{
			checkSlow(Selection[0]->IsChildOf(AUTMutator::StaticClass()));
			AUTMutator* Mut = Selection[0]->GetDefaultObject<AUTMutator>();
			if (Mut->ConfigMenu != NULL)
			{
				MutatorConfigMenu = CreateWidget<UUserWidget>(GetPlayerOwner()->GetWorld(), Mut->ConfigMenu);
				if (MutatorConfigMenu != NULL)
				{
					MutatorConfigMenu->AddToViewport(400);
				}
			}
		}
	}
	return FReply::Handled();
}

void SUTCreateGamePanel::GetCustomGameSettings(FString& GameMode, FString& StartingMap, FString& Description, FString& GameModeName, TArray<FString>&GameOptions, int32& DesiredPlayerCount, int32& bTeamGame)
{
	StartingMap = MapList->GetSelectedItem().IsValid() ? MapList->GetSelectedItem().Get()->MapPackageName : TEXT("");

	AUTGameMode* DefaultGameMode = SelectedGameClass->GetDefaultObject<AUTGameMode>();
	if (DefaultGameMode)
	{
		// The TAttributeProperty's that link the options page to the game mode will have already set their values
		// so just save config on the default object and return the game.

		DefaultGameMode->UILastStartingMap = StartingMap;


		GameMode = SelectedGameClass->GetPathName();
		GConfig->SetString(TEXT("CreateGameDialog"), TEXT("LastGametypePath"), *GameMode, GGameIni);

		GameModeName = DefaultGameMode->DisplayName.ToString();

		Description = FString::Printf(TEXT("A custom %s match with custom settings.\n"), *DefaultGameMode->DisplayName.ToString());			

		DefaultGameMode->GetGameURLOptions(GameConfigProps, GameOptions, DesiredPlayerCount);
		bTeamGame = DefaultGameMode->bTeamGame;
		for (int32 i = 0; i < GameOptions.Num(); i++)
		{
			Description += FString::Printf(TEXT("\n%s"), *GameOptions[i]);
		}

		TArray<FString> LastMutators;
		if (MutatorListEnabled.Num() > 0)
		{
			FString MutatorOption = TEXT("");
			LastMutators.Add(MutatorListEnabled[0]->GetPathName());

			MutatorOption += FString::Printf(TEXT("?mutator=%s"), *GetMutatorShortName(MutatorListEnabled[0]->GetPathName()));
			GetCustomMutatorOptions(MutatorListEnabled[0], Description, GameOptions);

			for (int32 i = 1; i < MutatorListEnabled.Num(); i++)
			{
				MutatorOption += TEXT(",") + GetMutatorShortName(MutatorListEnabled[i]->GetPathName());
				GetCustomMutatorOptions(MutatorListEnabled[i], Description, GameOptions);
				LastMutators.Add(MutatorListEnabled[i]->GetPathName());
			}

			GameOptions.Add(MutatorOption);
		}

		GConfig->SetArray(TEXT("CreateGameDialog"), TEXT("LastMutators"), LastMutators, GGameIni);
		DefaultGameMode->SaveConfig();

		Description = Description.Replace(TEXT("="),TEXT(" = "));
	}
}


void SUTCreateGamePanel::GetCustomMutatorOptions(UClass* MutatorClass, FString& Description, TArray<FString>&GameOptions)
{
	AUTMutator* DefaultMutator = MutatorClass->GetDefaultObject<AUTMutator>();
	if (DefaultMutator)
	{
		Description += FString::Printf(TEXT("\nMutator=%s"), *DefaultMutator->DisplayName.ToString());
		DefaultMutator->GetGameURLOptions(GameOptions);
	}
}

bool SUTCreateGamePanel::IsReadyToPlay()
{
	return (MutatorConfigMenu == nullptr || !MutatorConfigMenu->IsInViewport()) && SelectedGameClass != nullptr && MapList->GetSelectedItem().IsValid();
}

const FSlateBrush* SUTCreateGamePanel::GetMapBorder() const
{
	if ( SelectedMapInfo.IsValid() )
	{
		if (SelectedMapInfo->bIsEpicMap && !SelectedMapInfo->bIsMeshedMap)
		{
			return SUTStyle::Get().GetBrush("UT.MapOverlay.Epic.WIP");
		}
		else
		{
			return !SelectedMapInfo->bIsMeshedMap ? 
				SUTStyle::Get().GetBrush("UT.MapOverlay.Community.WIP") :
				SUTStyle::Get().GetBrush("UT.MapOverlay.Community");
		}

	}

	return LevelScreenshot;
}

EVisibility SUTCreateGamePanel::GetMapBorderVis() const
{
	if (SelectedMapInfo.IsValid() && (!SelectedMapInfo->bIsEpicMap || !SelectedMapInfo->bIsMeshedMap))
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

EVisibility SUTCreateGamePanel::GetLockImageVis() const
{
	if (SelectedMapInfo.IsValid() && !SelectedMapInfo->bHasRights)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

void SUTCreateGamePanel::SetBoxSkill(TSharedRef<SCompoundWidget> AllowBotsWidgets, TSharedRef<SHorizontalBox> BotSkillWidgets, TSharedRef<SCompoundWidget> RequireFilled)
{
	if (BotSkillBox.IsValid())
	{
		BotSkillBox->AddSlot().AutoHeight()
		[
			RequireFilled
		];

		BotSkillBox->AddSlot().AutoHeight()
		[
			AllowBotsWidgets
		];

		BotSkillBox->AddSlot().AutoHeight()
		[
			BotSkillWidgets
		];
	}
}



#endif

