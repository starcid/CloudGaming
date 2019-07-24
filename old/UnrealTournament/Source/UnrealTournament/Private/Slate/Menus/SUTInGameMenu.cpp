// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUTMainMenu.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "../Base/SUTDialogBase.h"
#include "../Dialogs/SUTSystemSettingsDialog.h"
#include "../Dialogs/SUTPlayerSettingsDialog.h"
#include "../Dialogs/SUTHUDSettingsDialog.h"
#include "../Dialogs/SUTWeaponConfigDialog.h"
#include "../Dialogs/SUTControlSettingsDialog.h"
#include "../Dialogs/SUTInputBoxDialog.h"
#include "../Dialogs/SUTMessageBoxDialog.h"
#include "../Widgets/SUTScaleBox.h"
#include "../Dialogs/SUTMessageBoxDialog.h"
#include "UTGameEngine.h"
#include "../Menus/SUTInGameMenu.h"
#include "../Panels/SUTInGameHomePanel.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "../Dialogs/SUTQuickPickDialog.h"
#include "UTLobbyGameState.h"
#include "BlueprintContextLibrary.h"
#include "PartyContext.h"
#include "UTGameInstance.h"
#include "UTParty.h"
#include "UTGameMode.h"
#include "SUTChatEditBox.h"

#if !UE_SERVER

/****************************** [ Build Sub Menus ] *****************************************/

void SUTInGameMenu::BuildLeftMenuBar()
{
	if (LeftMenuBar.IsValid())
	{
		AUTGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		AUTPlayerState* PS = PlayerOwner->PlayerController ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : NULL;
		bool bIsSpectator = PS && PS->bOnlySpectator;

		if (GS && GS->GetMatchState() != MatchState::WaitingToStart && GS->bTeamGame && !bIsSpectator && GS->bAllowTeamSwitches)
		{
		
			LeftMenuBar->AddSlot()
			.Padding(5.0f,0.0f,0.0f,0.0f)
			.AutoWidth()
			[
				SAssignNew(ChangeTeamButton, SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
				.OnClicked(this, &SUTInGameMenu::OnTeamChangeClick)
				.Visibility(this, &SUTInGameMenu::GetChangeTeamVisibility)
				.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMenuBase","MenuBar_ChangeTeam","CHANGE TEAM"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
						.ColorAndOpacity(this, &SUTInGameMenu::GetChangeTeamLabelColor)
					]
				]
			];
		}


		LeftMenuBar->AddSlot()
		.Padding(5.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SUTButton)
			.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
			.OnClicked(this, &SUTInGameMenu::OnMapVoteClick)
			.Visibility(this, &SUTInGameMenu::GetMapVoteVisibility)
			.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SUTInGameMenu::GetMapVoteTitle)
					.TextStyle(SUTStyle::Get(), "UT.Font.MenuBarText")
				]
			]
		];
	}
	
	FSlateApplication::Get().PlaySound(SUTStyle::PauseSound,0);
}

void SUTInGameMenu::BuildExitMenu(TSharedPtr<SUTComboButton> ExitButton)
{
	ExitButton->AddSubMenuItem(NSLOCTEXT("SUTInGameMenu", "MenuBar_ReturnToMainMenu", "Return to Main Menu"), FOnClicked::CreateSP(this, &SUTInGameMenu::OnReturnToMainMenu));
	ExitButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Exit_ReturnToGame", "Close Menu"), FOnClicked::CreateSP(this, &SUTInGameMenu::OnCloseMenu));
	ExitButton->AddSpacer();
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState && GameState->HubGuid.IsValid() )
	{
		ExitButton->AddSubMenuItem(NSLOCTEXT("SUTInGameMenu", "MenuBar_ReturnToLobby", "Return to Hub"), FOnClicked::CreateSP(this, &SUTInGameMenu::OnReturnToLobby));
	}
}

FReply SUTInGameMenu::OnCloseMenu()
{
	CloseMenus();
	return FReply::Handled();
}

FReply SUTInGameMenu::OnReturnToLobby()
{
	const bool bIsPartyLeader = PlayerOwner->IsPartyLeader();
	if (!bIsPartyLeader)
	{
		UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
		if (PartyContext)
		{
			PartyContext->LeaveParty();
		}
	}

	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState && GameState->HubGuid.IsValid() )
	{
		CloseMenus();
		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
		if (PC)
		{
			WriteQuitMidGameAnalytics();
			PlayerOwner->CloseMapVote();
			PC->ConnectToServerViaGUID(GameState->HubGuid.ToString(),-1, false, true);
		}
	}

	return FReply::Handled();
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName QuitMidGame
*
* @Trigger Fires when a user quits an in-progress game
*
* @Type Sent by the client
*
* @EventParam FPS string float User's average FPS in game
* @EventParam Kills int32 number of kills the user got this game
* @EventParam Deaths int32 number of deaths the user got this game
*
* @Comments
*/
void SUTInGameMenu::WriteQuitMidGameAnalytics()
{
	if (FUTAnalytics::IsAvailable() && PlayerOwner->GetWorld()->GetNetMode() != NM_Standalone)
	{
		AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		if (GameState && GameState->HasMatchStarted() && !GameState->HasMatchEnded())
		{
			AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
			if (PC)
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(PC->PlayerState);
				if (PS)
				{
					extern float ENGINE_API GAverageFPS;
					TArray<FAnalyticsEventAttribute> ParamArray;
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("FPS"), GAverageFPS));
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("Kills"), PS->Kills));
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("Deaths"), PS->Deaths));
					FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerOwner->PlayerController), ParamArray, true);

					FUTAnalytics::GetProvider().RecordEvent(TEXT("QuitMidGame"), ParamArray);
				}
			}
		}
	}
	
	if (FUTAnalytics::IsAvailable())
	{
		AUTGameMode* UTGameMode = Cast<AUTGameMode>(PlayerOwner->GetWorld()->GetAuthGameMode());
		if (UTGameMode && (UTGameMode->TutorialMask != 0))
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
			FUTAnalytics::FireEvent_UTTutorialQuit(UTPC, PlayerOwner->GetWorld()->GetMapName());
		}
	}
}

FReply SUTInGameMenu::OnReturnToMainMenu()
{
	bool bIsRankedGame = false;
	bool bIsQuickMatch = false;
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		if (GameState->bRankedSession)
		{
			bIsRankedGame = true;
		}
		if (GameState->bIsQuickMatch)
		{
			bIsQuickMatch = true;
		}
	}

	const bool bIsPartyLeader = PlayerOwner->IsPartyLeader();
	if (!bIsPartyLeader || bIsRankedGame || bIsQuickMatch)
	{
		UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
		if (PartyContext)
		{
			PartyContext->LeaveParty();
		}
	}
	else if (bIsPartyLeader)
	{
		UUTGameInstance* GameInstance = Cast<UUTGameInstance>(PlayerOwner->GetGameInstance());
		if (GameInstance)
		{
			UUTParty* Party = GameInstance->GetParties();
			if (Party)
			{
				UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
				if (PartyState)
				{
					PartyState->ReturnToMainMenu();
				}
			}
		}
	}

	WriteQuitMidGameAnalytics();
	PlayerOwner->CloseMapVote();
	CloseMenus();
	PlayerOwner->ReturnToMainMenu();
	return FReply::Handled();
}


void SUTInGameMenu::SetInitialPanel()
{
	SAssignNew(HomePanel, SUTInGameHomePanel, PlayerOwner);

	if (HomePanel.IsValid())
	{
		ActivatePanel(HomePanel);
	}
}

FReply SUTInGameMenu::OpenHUDSettings()
{
	CloseMenus();
	PlayerOwner->ShowHUDSettings();
	return FReply::Handled();
}

FReply SUTInGameMenu::OnMapVoteClick()
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState )
	{
		PlayerOwner->OpenMapVote(GameState);
	}
	return FReply::Handled();
}

FText SUTInGameMenu::GetMapVoteTitle() const
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState )
	{
		return FText::Format(NSLOCTEXT("SUTInGameMenu","MapVoteFormat","MAP VOTE ({0})"), FText::AsNumber(GameState->VoteTimer));
	}
	return NSLOCTEXT("SUTMenuBase","MenuBar_MapVote","MAP VOTE");
}


void SUTInGameMenu::ShowExitDestinationMenu()
{
	TSharedPtr<SUTQuickPickDialog> QP;

	SAssignNew(QP, SUTQuickPickDialog)
		.PlayerOwner(PlayerOwner)
		.OnPickResult(this, &SUTInGameMenu::OnDestinationResult)
		.DialogTitle(NSLOCTEXT("SUTMenuBase","Test","Choose your Destination..."));

	QP->AddOption(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ExitMenu", "MainMenuTitle", "MAIN MENU"))
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large")
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ExitMenu", "MainMenuMessage", "Leave the current online game and return to the main menu."))
		.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
		.AutoWrapText(true)
		]
	);

	QP->AddOption(
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ExitMenu", "ExitGameTitle", "EXIT GAME"))
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large")
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ExitMenu", "ExitGameMessage", "Exit the game and return to the desktop."))
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
			.AutoWrapText(true)
		]
	, true
	);		

	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->bIsInstanceServer)
	{
		QP->AddOption(
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ExitMenu", "HubTitle", "RETURN TO HUB"))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large")
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ExitMenu", "HubMessage", "Exit the current online game and return to the main hub."))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
				.AutoWrapText(true)
			]
		);		
	}

	PlayerOwner->OpenDialog(QP.ToSharedRef(), 200);
}

void SUTInGameMenu::QuitConfirmation()
{
	ShowExitDestinationMenu();
}

void SUTInGameMenu::OnDestinationResult(int32 PickedIndex)
{
	if (PlayerOwner.IsValid())
	{
		PlayerOwner->StopKillCam();
	}

	switch(PickedIndex)
	{
		case 0: OnReturnToMainMenu(); break;
		case 1 : PlayerOwner->ConsoleCommand(TEXT("quit")); break;
		case 2 : OnReturnToLobby(); break;
	}
}

void SUTInGameMenu::ShowHomePanel()
{
	// If we are on the home panel, look to see if it's time to go backwards.
	if (ActivePanel == HomePanel)
	{
		AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		bool bInstanceServer = (GameState && GameState->bIsInstanceServer);

		FText Msg;

		if (bInstanceServer)
		{
			Msg = NSLOCTEXT("SUTInGameMenu", "SUTInGameMenuBackLobby", "Are you sure you want to leave the game and return to the hub?");
		}
		else
		{
			if (PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>())
			{
				Msg = NSLOCTEXT("SUTInGameMenu", "SUTInGameMenuBack", "Are you sure you want to leave the hub and return to the hub browser?");
			}
			else
			{
				Msg = NSLOCTEXT("SUTInGameMenu", "SUTInGameMenuBackDefault", "Are you sure you want to leave the game and return to the main menu?");
			}
		}

		if (PlayerOwner->GetWorld()->GetNetMode() != NM_Standalone && PlayerOwner->IsInAnActiveParty())
		{
			if (PlayerOwner->IsPartyLeader())
			{
				Msg = FText::Format(NSLOCTEXT("SUTInGameMenu","LeaderFormat","You are the leader of your party.  If you leave now, your party will follow.  {0}"), Msg);	
			}
			else if (PlayerOwner->IsInAnActiveParty())
			{
				Msg = FText::Format(NSLOCTEXT("SUTInGameMenu","FollowerFormat","You are in a party. If you leave, you will leave your current party.  {0}"), Msg);	
			}
		}


		SAssignNew(MessageDialog, SUTMessageBoxDialog)
			.PlayerOwner(PlayerOwner)
			.DialogTitle(NSLOCTEXT("SUTInGameMenu", "SUTInGameMenuBackTitle", "Confirmation"))
			.OnDialogResult(this, &SUTInGameMenu::BackResult)
			.MessageText( Msg )
			.ButtonMask(UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO);

		PlayerOwner->OpenDialog(MessageDialog.ToSharedRef());
	}
	else
	{
		SUTMenuBase::ShowHomePanel();
	}
}

void SUTInGameMenu::BackResult(TSharedPtr<SCompoundWidget> Dialog, uint16 ButtonPressed)
{
	if (ButtonPressed == UTDIALOG_BUTTON_YES)
	{
		AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		if (GameState && GameState->bIsInstanceServer)
		{
			OnReturnToLobby();
		}
		else
		{
			OnReturnToMainMenu();
		}
	}
}

EVisibility SUTInGameMenu::GetMapVoteVisibility() const
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->GetMatchState() == MatchState::MapVoteHappening)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

EVisibility SUTInGameMenu::GetChangeTeamVisibility() const
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->bAllowTeamSwitches)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

void SUTInGameMenu::OnMenuOpened(const FString& Parameters)
{
	SUTMenuBase::OnMenuOpened(Parameters);
	PlayerOwner->FocusWidget(PlayerOwner->GetChatWidget());
}

bool SUTInGameMenu::SkipWorldRender()
{
	return false;
}

FReply SUTInGameMenu::OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.GetKey() == EKeys::Escape)
	{
		TSharedPtr<SUTChatEditBox> ChatBox = PlayerOwner->GetChatWidget();
		if (ChatBox.IsValid())
		{
			ChatBox->SetText(FText::GetEmpty());
		}
	}
	return SUTMenuBase::OnKeyUp(MyGeometry, InKeyboardEvent);
}

void SUTInGameMenu::OnMenuClosed()
{
	if (MessageDialog.IsValid())
	{
		PlayerOwner->CloseDialog(MessageDialog.ToSharedRef());
	}

	SUTMenuBase::OnMenuClosed();
}

FReply SUTInGameMenu::OnTeamChangeClick()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
		PC->ServerSwitchTeam();
	}
	return FReply::Handled();
}

FSlateColor SUTInGameMenu::GetChangeTeamLabelColor() const
{
	return ChangeTeamButton.IsValid() ? ChangeTeamButton->GetLabelColor() : FSlateColor(FLinearColor::White);
}


#endif