// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTMapVoteDialog.h"
#include "../SUWindowsStyle.h"
#include "../SUTUtils.h"
#include "UTGameEngine.h"
#include "UTLobbyMatchInfo.h"
#include "UTEpicDefaultRulesets.h"
#include "UTLobbyGameState.h"
#include "UTReplicatedMapInfo.h"
#include "../Panels/SUTPlayerListPanel.h"
#include "../Panels/SUTTextChatPanel.h"

#if !UE_SERVER

void SUTMapVoteDialog::Construct(const FArguments& InArgs)
{
	DefaultLevelScreenshot = new FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, FVector2D(256.0, 128.0), NAME_None);
	GameState = InArgs._GameState;

	FVector2D Size = InArgs._DialogSize;

	if (InArgs._PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone)
	{
		Size.X -= 900;
	}

	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(InArgs._DialogTitle)
							.DialogSize(Size)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.IsScrollable(false)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(0x000)
							.OnDialogResult(InArgs._OnDialogResult)
						);

	TSharedPtr<SVerticalBox> LeftPanel;
	TSharedPtr<SVerticalBox> RightPanel;

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
				.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
			
			]
			+SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(15.0,15.0,15.0,15.0)
					[
						SNew(SBox).WidthOverride(1730).HeightOverride(820)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0,0.0,6.0,0.0)
							[
								SNew(SBox).WidthOverride(820)
								[
									SAssignNew(LeftPanel,SVerticalBox)
								]
							]
							+SHorizontalBox::Slot()
							.Padding(6.0,0.0,10.0,0.0)
							.AutoWidth()
							[
								SNew(SBox).WidthOverride(900)
								[
									SAssignNew(RightPanel,SVerticalBox)
								]
							]
						]
					]		
				]
			]
		];

		LeftPanel->AddSlot().FillHeight(1.0).HAlign(HAlign_Center)
		[
			SAssignNew(MapBox,SScrollBox)
		];


		if (!PlayerListPanel.IsValid())
		{
			SAssignNew(PlayerListPanel, SUTPlayerListPanel)
				.PlayerOwner(GetPlayerOwner());
				//.OnPlayerClicked(FPlayerClicked::CreateSP(this, &SUTLobbyInfoPanel::PlayerClicked));
		}

		if (!TextChatPanel.IsValid())
		{
			SAssignNew(TextChatPanel, SUTTextChatPanel)
				.PlayerOwner(GetPlayerOwner());
				//.OnChatDestinationChanged(FChatDestinationChangedDelegate::CreateSP(this, &SUTLobbyInfoPanel::ChatDestionationChanged));

			if (TextChatPanel.IsValid())
			{
				TextChatPanel->AddDestination(NSLOCTEXT("LobbyChatDestinations","Game","Game"), ChatDestinations::Local,0.0,true);
				
				if (GameState->bTeamGame)
				{
					TextChatPanel->AddDestination(NSLOCTEXT("LobbyChatDestinations","Team","Team"), ChatDestinations::Team, 4.0, false);
				}

				TextChatPanel->RouteBufferedChat();
			}
		}

		// Make sure the two panels are connected.
		if (PlayerListPanel.IsValid() && TextChatPanel.IsValid())
		{
			PlayerListPanel->ConnectedChatPanel = TextChatPanel;
			TextChatPanel->ConnectedPlayerListPanel = PlayerListPanel;
		}


		RightPanel->AddSlot()
		.FillHeight(1.0)
		.HAlign(HAlign_Left)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(900)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.0)
						[
							TextChatPanel.ToSharedRef()
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox).WidthOverride(208)
							[
								PlayerListPanel.ToSharedRef()
							]
						]
					]
				]
			]
		];


	}
	BuildMapList();

	if (PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone && TopPanel.IsValid())
	{
		RightPanel->SetVisibility(EVisibility::Collapsed);
	}
}

void SUTMapVoteDialog::BuildMapList()
{
	if (MapBox.IsValid() && GameState.IsValid() && GameState->GetMatchState() == MatchState::MapVoteHappening)
	{
		if (GameState->IsMapVoteListReplicationCompleted())
		{
			// Store the current # of votes so we can check later to see if additional maps have been replicated or removed.
			NumMapInfos = GameState->MapVoteList.Num();
			MapBox->ClearChildren();

			if ( PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone)
			{
				MapBox->AddSlot()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMapVoteDialog","StandAlone","Select the next map to play"))
						.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.BodyTextStyle")
					]
				];
			}
			else
			{
				MapBox->AddSlot()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMapVoteDialog","Lead","Maps with the most votes:"))
						.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.BodyTextStyle")
					]
				];
			}

			MapBox->AddSlot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0,0.0,0.0,60.0)
				[
					SAssignNew(TopPanel, SGridPanel)
				]
				+SVerticalBox::Slot()
				.Padding(0.0,0.0,0.0,10.0)
				[
					SAssignNew(MapPanel, SGridPanel)
				]
			];

			BuildTopVotes();
			BuildAllVotes();
		}
		else
		{
			MapBox->ClearChildren();
			if ( !GameState->IsMapVoteListReplicationCompleted())
			{
				MapBox->AddSlot()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMapVoteDialog","ReceivingMapList","Receiving Map List From Server..."))
						.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.BodyTextStyle")
					]
				];
			}
		
		}
	}

	if (PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone && TopPanel.IsValid())
	{
		TopPanel->SetVisibility(EVisibility::Collapsed);
	}

}

void SUTMapVoteDialog::BuildTopVotes()
{
	if ( GameState->VoteTimer <= 10 ) return;

	// Figure out the # of columns needed.
	int32 NoColumns = FMath::Min<int32>(GameState->MapVoteList.Num(), MAP_COLUMNS);

	LeadingVoteButtons.Empty();
	TopPanel->ClearChildren();

	// The top row holds the top 6 maps in the lead.  Now if you have more than 6 maps with equal votes it will only show the first 6.
	// If no map has been voted on yet, it shows an empty space.

	for (int32 i=0; i < NoColumns; i++)
	{
		TSharedPtr<SImage> ImageWidget;
		TSharedPtr<STextBlock> VoteCountText;
		TSharedPtr<STextBlock> MapTitle;
		TSharedPtr<SBorder> BorderWidget;
		TSharedPtr<SUTButton> VoteButton;

		TopPanel->AddSlot(i, 0).Padding(5.0,5.0,5.0,5.0)
		[
			SNew(SBox).WidthOverride(256)
			.HeightOverride(158)							
			[
				SAssignNew(VoteButton, SUTButton)
				.OnClicked(this, &SUTMapVoteDialog::OnLeadingMapClick, i)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ComplexButton")
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.WidthOverride(256)
						.HeightOverride(128)							
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SAssignNew(ImageWidget,SImage)
								.Visibility(EVisibility::Hidden)
							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().FillHeight(1.0)
								.Padding(5.0,5.0,0.0,0.0)
								[
									SNew(SOverlay)
									+SOverlay::Slot()
									[
										SNew(SVerticalBox)
										+SVerticalBox::Slot()
										.AutoHeight()
										[
											SNew(SHorizontalBox)
											+SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SBox)
												.WidthOverride(50)
												.HeightOverride(50)
												[
													SAssignNew(BorderWidget,SBorder)
													.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Black"))
													.Visibility(EVisibility::Hidden)
												]
											]
										]
									]
									+SOverlay::Slot()
									[
										SAssignNew(VoteCountText,STextBlock)
										.Text(FText::AsNumber(0))
										.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.BodyTextStyle")
										.Visibility(EVisibility::Hidden)
									]
								]
							]
						]
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.AutoHeight()
					[
						SAssignNew(MapTitle, STextBlock)
						.Text(NSLOCTEXT("SUTMapVoteDialog","WaitingForVoteText","????"))
						.TextStyle(SUWindowsStyle::Get(),"UT.Hub.MapsText")
						.ColorAndOpacity(FLinearColor::Black)
					]
				]
			]
		];
		LeadingVoteButtons.Add( FVoteButton(NULL, NULL, VoteButton, ImageWidget, MapTitle, VoteCountText, BorderWidget) );
	}
	UpdateTopVotes();
}

void SUTMapVoteDialog::UpdateTopVotes()
{
	if ( GameState->VoteTimer <= 10 ) return;

	TArray<AUTReplicatedMapInfo*> LeadingVotes;

	for (int32 i=0; i < GameState->MapVoteList.Num(); i++)
	{
		if ( GameState->MapVoteList[i] && (GameState->MapVoteList[i]->VoteCount > 0 || GameState->VoteTimer <= 10) )	
		{
			bool bAdd = true;
			if (LeadingVotes.Num() > 0)
			{
				for (int32 j = 0; j < LeadingVotes.Num();j ++)
				{
					if (GameState->MapVoteList[i]->VoteCount > LeadingVotes[j]->VoteCount)
					{
						LeadingVotes.Insert(GameState->MapVoteList[i],j);
						bAdd = false;
						break;
					}
				}
			}

			if (bAdd) LeadingVotes.Add(GameState->MapVoteList[i]);
		}
	}

	if (LeadingVotes.Num() == 0) return;

	for (int32 i=0; i < LeadingVoteButtons.Num(); i++)
	{
		bool bClear = true;
		if ( i < LeadingVotes.Num() && LeadingVotes[i] != nullptr )
		{
			TWeakObjectPtr<AUTReplicatedMapInfo> MapVoteInfo = LeadingVotes[i];
			if (MapVoteInfo.IsValid())
			{

				bClear = false;
				if ( LeadingVoteButtons[i].MapVoteInfo.Get() != MapVoteInfo.Get() )
				{
					LeadingVoteButtons[i].MapVoteInfo = MapVoteInfo;
					FSlateDynamicImageBrush* MapBrush = DefaultLevelScreenshot;

					if (MapVoteInfo->MapScreenshotReference != TEXT(""))
					{
						if (MapVoteInfo->MapBrush == nullptr)
						{
							FString Package = MapVoteInfo->MapScreenshotReference;
							const int32 Pos = Package.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart);
							if ( Pos != INDEX_NONE )
							{
								Package = Package.Left(Pos);
							}

							LoadPackageAsync(Package, FLoadPackageAsyncDelegate::CreateSP(this, &SUTMapVoteDialog::LeaderTextureLoadComplete), 0);
						}
						else
						{
							LeadingVoteButtons[i].MapImage->SetImage(MapVoteInfo->MapBrush);
							LeadingVoteButtons[i].MapImage->SetVisibility(EVisibility::Visible);
						}

					}
					else
					{
						LeadingVoteButtons[i].MapImage->SetImage(MapBrush);
					}

					LeadingVoteButtons[i].MapTitle->SetText(MapVoteInfo->Title);
					LeadingVoteButtons[i].MapTitle->SetVisibility(EVisibility::Visible);

					LeadingVoteButtons[i].VoteCountText->SetText(FText::AsNumber(MapVoteInfo->VoteCount));
					LeadingVoteButtons[i].VoteCountText->SetVisibility(EVisibility::Visible);
					LeadingVoteButtons[i].BorderWidget->SetVisibility(EVisibility::Visible);
				}
				else if (MapVoteInfo->bNeedsUpdate)
				{
					LeadingVoteButtons[i].VoteCountText->SetText(FText::AsNumber(MapVoteInfo->VoteCount));
				}
			}
		}

		if (bClear && LeadingVoteButtons[i].MapVoteInfo.IsValid())
		{
			LeadingVoteButtons[i].MapVoteInfo.Reset();
			LeadingVoteButtons[i].MapImage->SetVisibility(EVisibility::Hidden);
			LeadingVoteButtons[i].MapTitle->SetText(NSLOCTEXT("SUTMapVoteDialog","WaitingForVoteText","????"));
			LeadingVoteButtons[i].VoteCountText->SetVisibility(EVisibility::Hidden);
			LeadingVoteButtons[i].BorderWidget->SetVisibility(EVisibility::Hidden);
		}
	}
}

bool SUTMapVoteDialog::MapVoteSortCompare(AUTReplicatedMapInfo* A, AUTReplicatedMapInfo* B)
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
}

void SUTMapVoteDialog::BuildAllVotes()
{
	// Figure out the # of columns needed.
	int32 NoColumns = FMath::Min<int32>(GameState->MapVoteList.Num(), MAP_COLUMNS);

	VoteButtons.Empty();
	MapPanel->ClearChildren();

	// We need 2 lists.. one sorted by votes (use the main one in the GameState) and one that is sorted alphabetically.
	// We will build that one here.
		
	TArray<AUTReplicatedMapInfo*>  AlphaSortedList;
	for (int32 i=0; i < GameState->MapVoteList.Num(); i++)
	{
		AUTReplicatedMapInfo* VoteInfo = GameState->MapVoteList[i];
		if (VoteInfo)
		{
			bool bAdd = true;
			for (int32 j=0; j < AlphaSortedList.Num(); j++)
			{
				if ( MapVoteSortCompare(VoteInfo, AlphaSortedList[j]) )
				{
					AlphaSortedList.Insert(VoteInfo, j);
					bAdd = false;
					break;
				}
			}

			if (bAdd)
			{
				AlphaSortedList.Add(VoteInfo);
			}
		}
	}

	// Now do the maps sorted

	for (int32 i=0; i < AlphaSortedList.Num(); i++)
	{
		if (GameState->MapVoteList[i])
		{
			int32 Row = i / NoColumns;
			int32 Col = i % NoColumns;

			TWeakObjectPtr<AUTReplicatedMapInfo> MapVoteInfo = AlphaSortedList[i];
			
			if (MapVoteInfo.IsValid())
			{
				TSharedPtr<SImage> ImageWidget;
				TSharedPtr<STextBlock> MapTitle;
				TSharedPtr<STextBlock> VoteCountText;
				TSharedPtr<SUTButton> VoteButton;
				TSharedPtr<SBorder> BorderWidget;

				FSlateDynamicImageBrush* MapBrush = DefaultLevelScreenshot;

				if (MapVoteInfo->MapScreenshotReference != TEXT(""))
				{
					if (MapVoteInfo->MapBrush == nullptr)
					{
						FString Package = MapVoteInfo->MapScreenshotReference;
						const int32 Pos = Package.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart);
						if ( Pos != INDEX_NONE )
						{
							Package = Package.Left(Pos);
						}

						LoadPackageAsync(Package, FLoadPackageAsyncDelegate::CreateSP(this, &SUTMapVoteDialog::TextureLoadComplete), 0);
					}
					else
					{
						MapBrush = MapVoteInfo->MapBrush;
					}
				}

				TSharedPtr<SVerticalBox> VoteCountBox;
				MapPanel->AddSlot(Col, Row).Padding(5.0,5.0,5.0,5.0)
				[
					SNew(SBox)
					.WidthOverride(256)
					.HeightOverride(158)							
					[
						SAssignNew(VoteButton, SUTButton)
						.OnClicked(this, &SUTMapVoteDialog::OnMapClick, MapVoteInfo)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.ComplexButton")
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.WidthOverride(256)
								.HeightOverride(128)							
								[
									SNew(SOverlay)
									+SOverlay::Slot()
									[
										SAssignNew(ImageWidget,SImage)
										.Image(MapBrush)
									]
									+SOverlay::Slot()
									[
										MapVoteInfo->BuildMapOverlay(FVector2D(256.0f, 128.0f), true)
									]
									+SOverlay::Slot()
									[
										SAssignNew(VoteCountBox, SVerticalBox)
										+SVerticalBox::Slot().FillHeight(1.0)
										.Padding(5.0,5.0,0.0,0.0)
										[
											SNew(SOverlay)
											+SOverlay::Slot()
											[
												SNew(SVerticalBox)
												+SVerticalBox::Slot()
												.AutoHeight()
												[
													SNew(SHorizontalBox)
													+SHorizontalBox::Slot()
													.AutoWidth()
													[
														SNew(SBox)
														.WidthOverride(50)
														.HeightOverride(50)
														[
															SAssignNew(BorderWidget,SBorder)
															.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Black"))
														]
													]
												]
											]
											+SOverlay::Slot()
											[
												SAssignNew(VoteCountText,STextBlock)
												.Text(FText::AsNumber(MapVoteInfo->VoteCount))
												.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.BodyTextStyle")
											]
										]
									]
								]
							]
							+SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SAssignNew(MapTitle,STextBlock)
								.Text(FText::FromString(MapVoteInfo->Title))
								.TextStyle(SUWindowsStyle::Get(),"UT.Hub.MapsText")
								.ColorAndOpacity(FLinearColor::Black)
							]
						]
					]
				];

				if (PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone)
				{
					VoteCountBox->SetVisibility(EVisibility::Collapsed);
				}
				VoteButtons.Add( FVoteButton(NULL, MapVoteInfo, VoteButton, ImageWidget, MapTitle, VoteCountText, BorderWidget) );
			}
		}
	}
}

void SUTMapVoteDialog::TextureLoadComplete(const FName& InPackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
{
	if (Result == EAsyncLoadingResult::Succeeded)
	{
		for (int32 i=0 ;i < VoteButtons.Num(); i++)
		{
			if (VoteButtons[i].MapVoteInfo.IsValid())
			{
				FString Screenshot = VoteButtons[i].MapVoteInfo->MapScreenshotReference;
				FString PackageName = InPackageName.ToString();
				if (Screenshot != TEXT("") && Screenshot.Contains(PackageName))
				{
					UTexture2D* Tex = FindObject<UTexture2D>(nullptr, *Screenshot);
					if (Tex)
					{
						VoteButtons[i].MapTexture = Tex;
						VoteButtons[i].MapVoteInfo->MapBrush = new FSlateDynamicImageBrush(Tex, FVector2D(256.0, 128.0), NAME_None);
						VoteButtons[i].MapImage->SetImage(VoteButtons[i].MapVoteInfo->MapBrush);
					}
				}
			}
		}
	}
}

void SUTMapVoteDialog::LeaderTextureLoadComplete(const FName& InPackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
{
	if (Result == EAsyncLoadingResult::Succeeded)
	{
		for (int32 i=0 ;i < LeadingVoteButtons.Num(); i++)
		{
			if (LeadingVoteButtons[i].MapVoteInfo.IsValid())
			{
				FString Screenshot = LeadingVoteButtons[i].MapVoteInfo->MapScreenshotReference;
				FString PackageName = InPackageName.ToString();
				if (Screenshot != TEXT("") && Screenshot.Contains(PackageName))
				{
					UTexture2D* Tex = FindObject<UTexture2D>(nullptr, *Screenshot);
					if (Tex)
					{
						LeadingVoteButtons[i].MapTexture = Tex;
						LeadingVoteButtons[i].MapVoteInfo->MapBrush = new FSlateDynamicImageBrush(Tex, FVector2D(256.0, 128.0), NAME_None);
						LeadingVoteButtons[i].MapImage->SetImage(LeadingVoteButtons[i].MapVoteInfo->MapBrush);
						LeadingVoteButtons[i].MapImage->SetVisibility(EVisibility::Visible);
					}
				}
			}
		}
	}
}

FReply SUTMapVoteDialog::OnLeadingMapClick(int32 ButtonIndex)
{
	AUTPlayerState* OwnerPlayerState = Cast<AUTPlayerState>(GetPlayerOwner()->PlayerController->PlayerState);
	if (OwnerPlayerState && ButtonIndex >=0 && ButtonIndex <= LeadingVoteButtons.Num() && LeadingVoteButtons[ButtonIndex].MapVoteInfo.IsValid())
	{
		OwnerPlayerState->RegisterVote(LeadingVoteButtons[ButtonIndex].MapVoteInfo.Get());

		// Now find the button..

		for (int32 i=0; i < LeadingVoteButtons.Num(); i++)
		{
			if ( i == ButtonIndex )
			{
				if (LeadingVoteButtons[i].VoteButton.IsValid())
				{
					LeadingVoteButtons[i].VoteButton->BePressed();
				}
			}
			else
			{
				if (LeadingVoteButtons[i].VoteButton.IsValid())
				{
					LeadingVoteButtons[i].VoteButton->UnPressed();
				}
			}
		}

		for (int32 i=0; i < VoteButtons.Num(); i++)
		{
			if (VoteButtons[i].VoteButton.IsValid())
			{
				VoteButtons[i].VoteButton->UnPressed();
			}
		}
	}

	return FReply::Handled();
}

FReply SUTMapVoteDialog::OnMapClick(TWeakObjectPtr<AUTReplicatedMapInfo> MapInfo)
{
	AUTPlayerState* OwnerPlayerState = Cast<AUTPlayerState>(GetPlayerOwner()->PlayerController->PlayerState);
	if (OwnerPlayerState)
	{
		OwnerPlayerState->RegisterVote(MapInfo.Get());

		// Now find the button..

		for (int32 i=0; i < VoteButtons.Num(); i++)
		{
			if (VoteButtons[i].MapVoteInfo.Get() == MapInfo.Get())
			{
				if (VoteButtons[i].VoteButton.IsValid())
				{
					VoteButtons[i].VoteButton->BePressed();
				}
			}
			else
			{
				if (VoteButtons[i].VoteButton.IsValid())
				{
					VoteButtons[i].VoteButton->UnPressed();
				}
			}
		}

		for (int32 i=0; i < LeadingVoteButtons.Num(); i++)
		{
			if (LeadingVoteButtons[i].VoteButton.IsValid())
			{
				LeadingVoteButtons[i].VoteButton->UnPressed();
			}
		}



	}

	return FReply::Handled();
}


void SUTMapVoteDialog::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (GameState.IsValid())
	{
		bool bNeedsFullUpdate = false;
		if (GameState->MapVoteList.Num() == NumMapInfos)
		{
			UpdateTopVotes();

			for (int32 i=0; i < VoteButtons.Num(); i++)
			{
				if (VoteButtons[i].MapVoteInfo.IsValid() && VoteButtons[i].MapVoteInfo->bNeedsUpdate)
				{
					VoteButtons[i].VoteCountText->SetText(FText::AsNumber(VoteButtons[i].MapVoteInfo->VoteCount));
					if ((GameState->MapVoteList.Num() > i) && (GameState->MapVoteList[i] != NULL))
					{
						GameState->MapVoteList[i]->bNeedsUpdate = false;
					}
				}
			}
		}
		else
		{
			bNeedsFullUpdate = true;
		}

		if (bNeedsFullUpdate)
		{
			BuildMapList();
		}
	}
}

void SUTMapVoteDialog::AddButtonsToLeftOfButtonBar(uint32& ButtonCount)
{
	if (GameState.IsValid() && PlayerOwner->GetWorld()->GetNetMode() == NM_Standalone)
	{
		ButtonBar->AddSlot(ButtonCount++, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
			.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
			.Text(NSLOCTEXT("SUTInGameMenu", "MenuBar_ReturnToMainMenu", "Return to Main Menu"))
			.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
			.OnClicked(this, &SUTMapVoteDialog::ReturnToMainMenu)
		];
	}
}

TSharedRef<class SWidget> SUTMapVoteDialog::BuildCustomButtonBar()
{
	if (GameState.IsValid() && PlayerOwner->GetWorld()->GetNetMode() != NM_Standalone)
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot().Padding(20.0,0.0,0.0,0.0)
			[
				SNew(STextBlock)
				.Text(this, &SUTMapVoteDialog::GetClockTime)
				.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
				.ColorAndOpacity(FLinearColor::Yellow)
			];
	}

	return SNullWidget::NullWidget;

}

FReply SUTMapVoteDialog::ReturnToMainMenu()
{
	PlayerOwner->CloseMapVote();
	PlayerOwner->ReturnToMainMenu();
	return FReply::Handled();
}

FText SUTMapVoteDialog::GetClockTime() const
{
	if (GameState->VoteTimer > 10)
	{
		return FText::Format(NSLOCTEXT("SUTMapVoteDialog","ClockFormat","Voting ends in {0} seconds..."),  FText::AsNumber(GameState->VoteTimer));
	}
	else
	{
		return FText::Format(NSLOCTEXT("SUTMapVoteDialog","ClockFormat2","Hurry, final voting ends in {0} seconds..."),  FText::AsNumber(GameState->VoteTimer));
	}
}

void SUTMapVoteDialog::OnDialogOpened()
{
	TextChatPanel->OnShowPanel();
}


void SUTMapVoteDialog::OnDialogClosed()
{
	TextChatPanel->OnHidePanel();
	for (int32 i=0; i < VoteButtons.Num(); i++)
	{
		VoteButtons[i].MapImage = nullptr;
	}

	VoteButtons.Empty();
	SUTDialogBase::OnDialogClosed();
}

FReply SUTMapVoteDialog::OnButtonClick(uint16 ButtonID)
{
	OnDialogResult.ExecuteIfBound(SharedThis(this), ButtonID);
	GetPlayerOwner()->CloseMapVote();
	return FReply::Handled();
}

FReply SUTMapVoteDialog::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (PlayerOwner->GetWorld()->GetNetMode() != NM_Standalone && InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnButtonClick(UTDIALOG_BUTTON_CANCEL);
	}

	return FReply::Unhandled();
}


#endif