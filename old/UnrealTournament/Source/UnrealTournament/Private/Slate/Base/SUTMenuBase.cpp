// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "../Base/SUTMenuBase.h"
#include "../Base/SUTDialogBase.h"
#include "../Dialogs/SUTSystemSettingsDialog.h"
#include "../Dialogs/SUTPlayerSettingsDialog.h"
#include "../Dialogs/SUTSocialSettingsDialog.h"
#include "../Dialogs/SUTWeaponConfigDialog.h"
#include "../Dialogs/SUTControlSettingsDialog.h"
#include "../Dialogs/SUTInputBoxDialog.h"
#include "../Dialogs/SUTMessageBoxDialog.h"
#include "../Widgets/SUTScaleBox.h"
#include "../Widgets/SUTImage.h"
#include "Runtime/Engine/Classes/Engine/Console.h"
#include "SOverlay.h"
#include "SUTFriendsPopupWindow.h"
#include "UTGameEngine.h"
#include "SUTServerBrowserPanel.h"
#include "SUTStatsViewerPanel.h"
#include "SUTCreditsPanel.h"
#include "UnrealNetwork.h"
#include "SUTProfileItemsDialog.h"
#include "SUTQuickPickDialog.h"
#include "BlueprintContextLibrary.h"
#include "MatchmakingContext.h"

const float ICON_SIZE=38.0f;

#if !UE_SERVER

void SUTMenuBase::Construct(const FArguments& InArgs)
{
	ZOrderIndex = -1;
	PlayerOwner = InArgs._PlayerOwner;
	CreateDesktop();
}

void SUTMenuBase::OnMenuOpened(const FString& Parameters)
{
	if (!SlateParentWindowPtr.IsValid())
	{
		SlateParentWindowPtr = FSlateApplication::Get().FindWidgetWindow(SharedThis(this));
	}

	if (SlateParentWindowPtr.IsValid())
	{
		OnActivateHandle = SlateParentWindowPtr.Pin()->GetOnWindowActivatedEvent().AddSP(this, &SUTMenuBase::HandleWindowActivated);
	}

	AUTGameState* UTGameState = (PlayerOwner.IsValid() && PlayerOwner->GetWorld() != nullptr) ? PlayerOwner->GetWorld()->GetGameState<AUTGameState>() : nullptr;
	if (UTGameState != nullptr) UTGameState->bLocalMenusAreActive = true;

	GameViewportWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);

	if (!PlayerOnlineStatusChangedDelegate.IsValid())
	{
		PlayerOnlineStatusChangedDelegate = PlayerOwner->RegisterPlayerOnlineStatusChangedDelegate(FPlayerOnlineStatusChanged::FDelegate::CreateSP(this, &SUTMenuBase::OwnerLoginStatusChanged));
	}
}

void SUTMenuBase::OnMenuClosed()
{
	if (SlateParentWindowPtr.IsValid())
	{
		SlateParentWindowPtr.Pin()->GetOnWindowActivatedEvent().Remove(OnActivateHandle);
	}

	AUTGameState* UTGameState = ( PlayerOwner.IsValid() && PlayerOwner->GetWorld() != nullptr) ? PlayerOwner->GetWorld()->GetGameState<AUTGameState>() : nullptr;
	if (UTGameState != nullptr) UTGameState->bLocalMenusAreActive = false;

	// Deactivate the current panel
	if (ActivePanel.IsValid())
	{
		ActivePanel->PanelClosed();
	}

	FSlateApplication::Get().ClearUserFocus(0);
	FSlateApplication::Get().ClearKeyboardFocus();
	if (PlayerOwner.IsValid())
	{
		PlayerOwner->RemovePlayerOnlineStatusChangedDelegate(PlayerOnlineStatusChangedDelegate);
		PlayerOnlineStatusChangedDelegate.Reset();
	}
}

void SUTMenuBase::OwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID)
{
	OnOwnerLoginStatusChanged(LocalPlayerOwner, NewStatus, UniqueID);
}

bool SUTMenuBase::SupportsKeyboardFocus() const
{
	return true;
}

FReply SUTMenuBase::OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InKeyboardFocusEvent )
{
	if ( ActivePanel.IsValid() )
	{
		TSharedPtr<SWidget> InitialFocus = ActivePanel->GetInitialFocus();
		if (InitialFocus.IsValid())
		{
			PlayerOwner->FocusWidget(InitialFocus);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Handled().ReleaseMouseCapture();

}

FReply SUTMenuBase::OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.GetKey() == EKeys::Escape)
	{
		if (PlayerOwner->bShowingFriendsMenu)
		{
			PlayerOwner->ToggleFriendsAndChat();
		}
		else if (GWorld->GetWorld()->GetMapName().ToLower() != TEXT("ut-entry"))
		{
			AUTGameState* UTGameState = (PlayerOwner.IsValid() && PlayerOwner->GetWorld() != nullptr) ? PlayerOwner->GetWorld()->GetGameState<AUTGameState>() : nullptr;
			AUTPlayerState* UTPlayerState = (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState) ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : nullptr;
			if (UTGameState && UTPlayerState && ((UTGameState->GetMatchState() != MatchState::WaitingToStart) || UTPlayerState->bIsWarmingUp || UTPlayerState->bOnlySpectator))
			{
				CloseMenus();
			}
		}
		else
		{
			ShowHomePanel();
		}

	}
	else if (InKeyboardEvent.GetKey() == EKeys::F9)
	{
		ConsoleCommand(TEXT("SHOT SHOWUI"));
	}

	return FReply::Handled();
}

void SUTMenuBase::CloseMenus()
{
	if (PlayerOwner != NULL)
	{
		PlayerOwner->HideMenu();
	}
}

FReply SUTMenuBase::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent)
{
	if ( GetDefault<UInputSettings>()->ConsoleKeys.Contains(InKeyboardEvent.GetKey()) )
	{
		PlayerOwner->ViewportClient->ViewportConsole->FakeGotoState(FName(TEXT("Open")));
	}

	return FReply::Handled();
}

FReply SUTMenuBase::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SUTMenuBase::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

FReply SUTMenuBase::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

void SUTMenuBase::ConsoleCommand(FString Command)
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController != NULL)
	{
		PlayerOwner->Exec(PlayerOwner->GetWorld(), *Command, *GLog);
	}
}


FReply SUTMenuBase::OnMenuConsoleCommand(FString Command)
{
	ConsoleCommand(Command);
	CloseMenus();
	return FReply::Handled();
}

void SUTMenuBase::ActivatePanel(TSharedPtr<class SUTPanelBase> PanelToActivate)
{
	if ( !Desktop.IsValid() ) return;		// Quick out if no place to put it



	// Don't reactivate the current panel
	if (ActivePanel != PanelToActivate)
	{
		// Deactive the current panel.
		if ( ActivePanel.IsValid() )
		{
			DeactivatePanel(ActivePanel);
		}	
	
		ZOrderIndex = (ZOrderIndex + 1) % 5000;

		// Add the new one.
		SOverlay::FOverlaySlot& Slot = Desktop->AddSlot(ZOrderIndex)
		[
			PanelToActivate.ToSharedRef()
		];
		PanelToActivate->ZOrder = ZOrderIndex;
		ActivePanel = PanelToActivate;
		FSlateApplication::Get().SetKeyboardFocus(ActivePanel, EFocusCause::SetDirectly);
		ActivePanel->OnShowPanel(SharedThis(this));
		
	}
}

void SUTMenuBase::DeactivatePanel(TSharedPtr<class SUTPanelBase> PanelToDeactivate)
{
	PanelToDeactivate->OnHidePanel();
}

void SUTMenuBase::PanelHidden(TSharedPtr<SWidget> Child)
{
	if (Child.IsValid())
	{
		// SO TOTALLY Unsafe.. 
		TSharedPtr<SUTPanelBase> const Panel = StaticCastSharedPtr<SUTPanelBase>(Child);
		Desktop->RemoveSlot(Panel->ZOrder);
		if (Child == ActivePanel)
		{
			ActivePanel.Reset();
		}
	}
}

void SUTMenuBase::CreateDesktop()
{
	bNeedsPlayerOptions = false;
	bNeedsWeaponOptions = false;
	TickCountDown = 0;
	
	LeftMenuBar = NULL;
	RightMenuBar = NULL;
	ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				BuildBackground()
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SNew(SBox)
					.HeightOverride(48)
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(48)
								[
									// Left Menu
									SNew(SOverlay)
									+ SOverlay::Slot()
									.HAlign(HAlign_Left)
									.VAlign(VAlign_Center)
									[
										BuildDefaultLeftMenuBar()
									]
									+ SOverlay::Slot()
										.HAlign(HAlign_Right)
										.VAlign(VAlign_Center)
										[
											BuildDefaultRightMenuBar()
										]
								]
							]
						]
					]
				]

				+ SVerticalBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SAssignNew(DesktopOverlay, SOverlay)
						+ SOverlay::Slot()
						[
							SAssignNew(Desktop, SOverlay)
						]
					]
			]
		];

	SetInitialPanel();
}

void SUTMenuBase::SetInitialPanel()
{
}

/****************************** [ Build Sub Menus ] *****************************************/

void SUTMenuBase::BuildLeftMenuBar() {}
void SUTMenuBase::BuildRightMenuBar() {}

TSharedRef<SWidget> SUTMenuBase::BuildDefaultLeftMenuBar()
{
	SAssignNew(LeftMenuBar, SHorizontalBox);
	if (LeftMenuBar.IsValid())
	{
		LeftMenuBar->AddSlot()
		.Padding(5.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SAssignNew(HomeButton, SUTButton)
			.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
			.OnClicked(this, &SUTMenuBase::OnShowHomePanel)
			.Visibility(this, &SUTMenuBase::GetBackVis)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SUTImage)
						.Image(SUTStyle::Get().GetBrush("UT.Icon.Back")).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
					]
				]
			]
		];

		if (ShouldShowBrowserIcon())
		{
			LeftMenuBar->AddSlot()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
				.OnClicked(this, &SUTMenuBase::OnShowServerBrowserPanel)
				[

					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SUTImage)
							.Image(SUTStyle::Get().GetBrush("UT.Icon.Browser")).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
						]
					]
				]
			];
		}

		BuildLeftMenuBar();
	}

	return LeftMenuBar.ToSharedRef();
}

FText SUTMenuBase::GetBrowserButtonText() const
{
	return PlayerOwner->GetWorld()->GetNetMode() == ENetMode::NM_Standalone ? NSLOCTEXT("SUTMenuBase","MenuBar_HUBS","Play Online") : NSLOCTEXT("SUTMenuBase","MenuBar_Browser","Server Browser");
}

TSharedRef<SWidget> SUTMenuBase::BuildDefaultRightMenuBar()
{
	// Build the Right Menu Bar
	if (!RightMenuBar.IsValid())
	{
		SAssignNew(RightMenuBar, SHorizontalBox);
	}
	else
	{
		RightMenuBar->ClearChildren();
	}

	if (RightMenuBar.IsValid())
	{
		BuildRightMenuBar();

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			BuildAboutSubMenu()
		];

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			BuildOnlinePresence()
		];

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			BuildOptionsSubMenu()
		];

		RightMenuBar->AddSlot()
		.Padding(70.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SUTButton)
			.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
			.OnClicked(this, &SUTMenuBase::MinimizeClicked)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SUTImage)
						.Image(SUTStyle::Get().GetBrush("UT.Icon.Minimize")).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
					]
				]
			]
		];

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SUTButton)
			.OnClicked(this, &SUTMenuBase::ToggleFullscreenClicked)
			.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
			.ToolTipText(NSLOCTEXT("ToolTips","TPFullScreen","Toggles between fullscreen and widowed mode."))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SUTImage)
						.Image(this, &SUTMenuBase::GetFullvsWindowButtonImage).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
					]
				]

			]
		];

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SUTButton)
			.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
			.OnClicked(this, &SUTMenuBase::ExitClicked)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SUTImage)
						.Image(SUTStyle::Get().GetBrush("UT.Icon.Exit")).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
					]
				]
			]
		];
	}
	return RightMenuBar.ToSharedRef();
}

EVisibility SUTMenuBase::GetSocialBangVisibility() const
{
	if (PlayerOwner.IsValid())
	{
		if (PlayerOwner->IsPlayerShowingSocialNotification())
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

TSharedRef<SWidget> SUTMenuBase::BuildOptionsSubMenu()
{
	TSharedPtr<SUTComboButton> DropDownButton = NULL;

	SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth().VAlign(VAlign_Fill).Padding(0.0f)
	[
		SAssignNew(DropDownButton, SUTComboButton)
		.HasDownArrow(false)
		.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
		.ContentPadding(0.0f)
		.ButtonContent()
		[
			SNew(SUTImage).WidthOverride(38).HeightOverride(38)
			.Image(SUTStyle::Get().GetBrush("UT.Icon.Settings"))
		]
	];

	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_PlayerSettings", "Player Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenPlayerSettings));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_SocialSettings", "Social Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenSocialSettings));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_WeaponSettings", "Weapon Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenWeaponSettings));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_HUDSettings", "HUD Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenHUDSettings));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_SystemSettings", "System Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenSystemSettings));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_ControlSettings", "Control Settings"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenControlSettings));
	DropDownButton->AddSpacer();
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Options_ClearCloud", "Reset Profile"), FOnClicked::CreateSP(this, &SUTMenuBase::ClearCloud), true);

	return DropDownButton.ToSharedRef();

}

TSharedRef<SWidget> SUTMenuBase::BuildAboutSubMenu()
{
	TSharedPtr<SUTComboButton> DropDownButton = NULL;

	SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth().VAlign(VAlign_Fill).Padding(0.0f)
	[
		SAssignNew(DropDownButton, SUTComboButton)
		.HasDownArrow(false)
		.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
		.ContentPadding(0.0f)
		.ButtonContent()
		[
			SNew(SUTImage).WidthOverride(38).HeightOverride(38)
			.Image(SUTStyle::Get().GetBrush("UT.Icon.About"))
		]
	];


	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_About_BuildNotes", "Build Notes"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenBuildNotes));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_About_TPSReport", "Third Party Software"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenTPSReport));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_About_Credits", "Community Contributors"), FOnClicked::CreateSP(this, &SUTMenuBase::OpenCredits));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_About_UTSite", "UnrealTournament.com"), FOnClicked::CreateSP(this, &SUTMenuBase::OnMenuHTTPButton, FString(TEXT("http://www.unrealtournament.com/"))));

#if UE_BUILD_DEBUG
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_About_WR", "Widget Reflector"), FOnClicked::CreateSP(this, &SUTMenuBase::ShowWidgetReflector));
#endif
	DropDownButton->RebuildMenuContent();
	return DropDownButton.ToSharedRef();
}

void SUTMenuBase::BuildExitMenu(TSharedPtr<SUTComboButton> ExitButton)
{
}


FReply SUTMenuBase::OpenPlayerSettings()
{
	if (TickCountDown <= 0)
	{
		PlayerOwner->ShowContentLoadingMessage();
		bNeedsPlayerOptions = true;
		TickCountDown = 3;
	}

	return FReply::Handled();
}

FReply SUTMenuBase::OpenSocialSettings()
{
	PlayerOwner->OpenDialog(SNew(SUTSocialSettingsDialog).PlayerOwner(PlayerOwner));
	return FReply::Handled();
}


FReply SUTMenuBase::OpenWeaponSettings()
{
	if (TickCountDown <= 0)
	{
		PlayerOwner->ShowContentLoadingMessage();
		bNeedsWeaponOptions = true;
		TickCountDown = 3;
	}

	return FReply::Handled();
}

FReply SUTMenuBase::OpenSystemSettings()
{
	PlayerOwner->OpenDialog(SNew(SUTSystemSettingsDialog).PlayerOwner(PlayerOwner).DialogTitle(NSLOCTEXT("SUTMenuBase","System","System Settings")));
	return FReply::Handled();
}

FReply SUTMenuBase::OpenControlSettings()
{
	PlayerOwner->OpenDialog(SNew(SUTControlSettingsDialog).PlayerOwner(PlayerOwner).DialogTitle(NSLOCTEXT("SUTMenuBase","Controls","Control Settings")));
	return FReply::Handled();
}

FReply SUTMenuBase::OpenProfileItems()
{
	PlayerOwner->OpenDialog(SNew(SUTProfileItemsDialog).PlayerOwner(PlayerOwner));
	return FReply::Handled();
}

FReply SUTMenuBase::ClearCloud()
{
	if (PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->ClearProfileSettings();				
	}
	else
	{
		PlayerOwner->ShowMessage(NSLOCTEXT("SUTMainMenu","NotLoggedInTitle","Not Logged In"), NSLOCTEXT("SUTMainMenu","NoLoggedInMsg","You need to be logged in to clear your cloud settings!"), UTDIALOG_BUTTON_OK);
	}
	return FReply::Handled();
}

FReply SUTMenuBase::OpenBuildNotes()
{
	PlayerOwner->ShowWebMessage(NSLOCTEXT("UTLocalPlayer","ThanksForUpdating","New Features"), PlayerOwner->GetBuildNotesURL());
	return FReply::Handled();
}

FReply SUTMenuBase::OpenTPSReport()
{
	PlayerOwner->OpenDialog(
							SNew(SUTMessageBoxDialog)
							.PlayerOwner(PlayerOwner)
							.DialogSize(FVector2D(0.7, 0.8))
							.bDialogSizeIsRelative(true)
							.DialogTitle(NSLOCTEXT("SUTMenuBase", "TPSReportTitle", "Third Party Software"))
							.MessageText(NSLOCTEXT("SUWindowsDesktop", "TPSReportText", "TPSText"))
							//.MessageTextStyleName("UWindows.Standard.Dialog.TextStyle.Legal")
							.MessageTextStyleName("UT.Common.NormalText")
							.ButtonMask(UTDIALOG_BUTTON_OK)
							);
	return FReply::Handled();
}

FReply SUTMenuBase::OpenCredits()
{
	TSharedPtr<class SUTCreditsPanel> CreditsPanel = PlayerOwner->GetCreditsPanel();
	if (CreditsPanel.IsValid())
	{
		ActivatePanel(CreditsPanel);
	}
	return FReply::Handled();
}

FReply SUTMenuBase::OnMenuHTTPButton(FString URL)
{
	FString Error;
	FPlatformProcess::LaunchURL(*URL, NULL, &Error);
	if (Error.Len() > 0)
	{
		PlayerOwner->OpenDialog(
								SNew(SUTMessageBoxDialog)
								.PlayerOwner(PlayerOwner)
								.DialogTitle(NSLOCTEXT("SUTMenuBase", "HTTPBrowserError", "Error Launching Browser"))
								.MessageText(FText::FromString(Error))
								.ButtonMask(UTDIALOG_BUTTON_OK)
								);
	}
	return FReply::Handled();
}


FReply SUTMenuBase::OnShowStatsViewer()
{
	TSharedPtr<class SUTStatsViewerPanel> StatsViewer = PlayerOwner->GetStatsViewer();
	if (StatsViewer.IsValid())
	{
		StatsViewer->SetQueryWindow(TEXT("alltime"));
		//StatsViewer->SetQueryWindow(TEXT("monthly"));
		//StatsViewer->SetQueryWindow(TEXT("weekly"));
		//StatsViewer->SetQueryWindow(TEXT("daily"));
		ActivatePanel(StatsViewer);
	}
	return FReply::Handled();
}

FReply SUTMenuBase::OnCloseClicked()
{
	return FReply::Handled();
}

TSharedRef<SWidget> SUTMenuBase::BuildOnlinePresence()
{
	if ( PlayerOwner->IsLoggedIn() )
	{

		TSharedPtr<SHorizontalBox> Box;

		SAssignNew(Box, SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
				.OnClicked(this, &SUTMenuBase::OnShowStatsViewer)
				.ToolTipText(NSLOCTEXT("ToolTips","TPMyStats","Show stats for this player, friends, and recent opponents."))
				[

					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SUTImage)
							.Image(SUTStyle::Get().GetBrush("UT.Icon.Stats")).WidthOverride(38).HeightOverride(38)
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
				.OnClicked(this, &SUTMenuBase::OpenProfileItems)
				.ToolTipText(NSLOCTEXT("ToolTips", "TPMyItems", "Show collectible items you own."))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SUTImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.UpArrow")).WidthOverride(38).HeightOverride(38)
						]
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10.0f, 0.0f, 20.0f, 0.0f)
			[

				SNew(SBox)
				.HeightOverride(56)
				[
					SAssignNew(PlayerButton, SUTComboButton)
					.HasDownArrow(false)
					.TextStyle(SUTStyle::Get(), "UT.Font.MenuBarText")
					.ToolTipText(NSLOCTEXT("ToolTips","TPMyPlayerCard","Show this player's player card."))
					.ContentPadding(FMargin(35.0,0.0,35.0,0.0))
					.ContentHAlign(HAlign_Left)
					.IsEnabled(this, &SUTMenuBase::IsPlayerCardDataLoaded)
					.ButtonContent()
					[
						BuildPlayerInfo()
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SMenuAnchor)
				.Method(EPopupMethod::UseCurrentWindow)
				[
					SNew(SUTButton)
					.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
					.OnClicked(this, &SUTMenuBase::ToggleFriendsAndChat)
#if PLATFORM_LINUX
					.ToolTipText(NSLOCTEXT("ToolTips", "TPFriendsNotSupported", "Friends list not supported yet on this platform."))
#else
					.ToolTipText(NSLOCTEXT("ToolTips","TPFriends","Show / Hide your friends list."))
#endif
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot().VAlign(VAlign_Center).AutoWidth()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								[
									SNew(SUTImage)
									.Image(SUTStyle::Get().GetBrush("UT.Icon.Online")).WidthOverride(38).HeightOverride(38)
								]
								+ SOverlay::Slot()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Top)
								[
									SNew(SImage)
									.Visibility(this, &SUTMenuBase::GetSocialBangVisibility)
									.Image(SUTStyle::Get().GetBrush("UT.Icon.SocialBang"))
								]
							]
						]
					]
				]
			];

		if (PlayerButton.IsValid())
		{
			PlayerButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_ShowPlayerCard", "Show Player Card"), FOnClicked::CreateSP(this, &SUTMenuBase::OnShowPlayerCard));
			PlayerButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Logout", "Log Out"), FOnClicked::CreateSP(this, &SUTMenuBase::OnLogout),true);
		}

		return Box.ToSharedRef();
	
	}
	else
	{
		return 	SNew(SUTButton)
		.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
		.Text(NSLOCTEXT("SUTMenuBase","MenuBar_SignIn","Sign In"))
		.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
		.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
		.OnClicked(this, &SUTMenuBase::OnOnlineClick)		
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(36)
					.HeightOverride(36)
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush("UT.Icon.SignIn"))
					]
				]
			]
		];
	}
}

void SUTMenuBase::OnOwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID)
{
	// Rebuilds the right menu bar
	BuildDefaultRightMenuBar();
}

FReply SUTMenuBase::OnOnlineClick()
{
	PlayerOwner->GetAuth();
	return FReply::Handled();
}

FReply SUTMenuBase::OnShowServerBrowser()
{
	return OnShowServerBrowserPanel();
}


FReply SUTMenuBase::OnShowServerBrowserPanel()
{
	TSharedPtr<class SUTServerBrowserPanel> Browser = PlayerOwner->GetServerBrowser();
	if (Browser.IsValid())
	{
		ActivatePanel(Browser);
	}

	return FReply::Handled();
}

FReply SUTMenuBase::ToggleFriendsAndChat()
{
	if (PlayerOwner != nullptr) PlayerOwner->ToggleFriendsAndChat();
	return FReply::Handled();
}

void SUTMenuBase::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// NOTE: TickCountDown is in frames, not time.  We have to delay 3 frames before opening a blocking menu to insure the
	// meesage gets displayed.

	if (TickCountDown > 0)
	{
		TickCountDown--;

		if (TickCountDown <= 0)
		{
			OpenDelayedMenu();
		}
	}

	// Every 10 seconds, pull the player's XP

	if (XPDelay <= 0)
	{
		XPDelay = 10.0f;

		XP = 0.0f;

	#if WITH_PROFILE
		// use profile if available, in case new data was received from MCP since the server set the replicated value
		UUtMcpProfile* Profile = PlayerOwner->GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
		if (Profile != NULL)
		{
			XP = Profile->GetXP();
		}
	#endif

		XPLevel = GetLevelForXP(XP);
		int32 XPLevelStart = GetXPForLevel(XPLevel);
		int32 XPLevelEnd = GetXPForLevel(XPLevel + 1);
		int32 XPLevelRange = XPLevelEnd - XPLevelStart;

		XPLevelPercent = (XPLevelRange > 0) ? (float)(XP - XPLevelStart) / (float)XPLevelRange : 0.0f;
	}
	else
	{
		XPDelay -= InDeltaTime;
	}
}

void SUTMenuBase::OpenDelayedMenu()
{
	if (bNeedsPlayerOptions)
	{
		PlayerOwner->OpenDialog(SNew(SUTPlayerSettingsDialog).PlayerOwner(PlayerOwner));
		bNeedsPlayerOptions = false;
		PlayerOwner->HideContentLoadingMessage();
	}
	else if (bNeedsWeaponOptions)
	{
		PlayerOwner->OpenDialog(SNew(SUTWeaponConfigDialog).PlayerOwner(PlayerOwner));
		bNeedsWeaponOptions = false;
		PlayerOwner->HideContentLoadingMessage();
	}
}

bool SUTMenuBase::IsPlayerCardDataLoaded() const
{
	return PlayerOwner->IsLoggedIn() && !PlayerOwner->IsPendingMCPLoad();
}

void SUTMenuBase::ShowHomePanel()
{
	if (HomePanel.IsValid())
	{
		ActivatePanel(HomePanel);
	}
	else if (ActivePanel.IsValid())
	{
		DeactivatePanel(ActivePanel);
	}
}


FReply SUTMenuBase::OnShowHomePanel()
{
	ShowHomePanel();
	return FReply::Handled();
}

FReply SUTMenuBase::ShowWidgetReflector()
{
	ConsoleCommand(TEXT("WidgetReflector"));
	return FReply::Handled();
}

TSharedRef<SWidget> SUTMenuBase::BuildBackground()
{
	return SNullWidget::NullWidget;
}

FReply SUTMenuBase::OpenHUDSettings()
{
	PlayerOwner->ShowHUDSettings();
	return FReply::Handled();
}

FReply SUTMenuBase::OnLogout()
{
	if (PlayerOwner->IsMenuGame())
	{
		PlayerOwner->ShowMessage(NSLOCTEXT("SUTPlayerInfoDialog", "SignOuttConfirmationTitle", "Sign Out?"), NSLOCTEXT("SUTPlayerInfoDialog", "SignOuttConfirmationMessageMenu", "You are about to sign out of this account.  Are you sure?"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateSP(this, &SUTMenuBase::SignOutConfirmationResult));
	}
	else
	{
		PlayerOwner->ShowMessage(NSLOCTEXT("SUTPlayerInfoDialog", "SignOuttConfirmationTitle", "Sign Out?"), NSLOCTEXT("SUTPlayerInfoDialog", "SignOuttConfirmationMessageGame", "You are about to sign out of this account.  Doing so will return you to the main menu.  Are you sure?"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateSP(this, &SUTMenuBase::SignOutConfirmationResult));
	}
	return FReply::Handled();
}

void SUTMenuBase::SignOutConfirmationResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		PlayerOwner->Logout();
	}
}



FReply SUTMenuBase::OnShowPlayerCard()
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
	{
		AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
		if (PlayerState)
		{
			PlayerOwner->ShowPlayerInfo(PlayerState->UniqueId.ToString(), PlayerState->PlayerName);
		}
	}
	return FReply::Handled();
}

const FSlateBrush* SUTMenuBase::GetFullvsWindowButtonImage() const
{
	if (PlayerOwner->ViewportClient->IsFullScreenViewport())
	{
		return SUTStyle::Get().GetBrush("UT.Icon.Windowed");
	}
	else
	{
		return SUTStyle::Get().GetBrush("UT.Icon.Fullscreen");
	}
}

FReply SUTMenuBase::ToggleFullscreenClicked()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());	

	if (UserSettings)
	{
		if (PlayerOwner->ViewportClient->IsFullScreenViewport())
		{
			UserSettings->SetFullscreenMode(EWindowMode::Windowed);
		}
		else
		{
			UserSettings->SetFullscreenMode(EWindowMode::Fullscreen);
		}
		UserSettings->ApplyResolutionSettings(false);
	}

	return FReply::Handled();
}

FReply SUTMenuBase::MinimizeClicked()
{
	FPlatformMisc::RequestMinimize();
	return FReply::Handled();
}

EVisibility SUTMenuBase::GetBackVis() const
{
	if (ActivePanel.IsValid() && ActivePanel->ShouldShowBackButton())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

FReply SUTMenuBase::ExitClicked()
{
	QuitConfirmation();
	return FReply::Handled();
}

void SUTMenuBase::QuitConfirmation()
{
	PlayerOwner->ShowMessage(NSLOCTEXT("SUTMenuBase", "QuitGameSureTitle", "Quit Game"), NSLOCTEXT("SUTMenuBase", "QuitGameSureMessage", "You are about to quit the game.  Are you sure?"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateSP(this, &SUTMenuBase::QuitConfirmationResult));
}

void SUTMenuBase::QuitConfirmationResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		PlayerOwner->ConsoleCommand(TEXT("quit"));
	}
}

TSharedPtr<SUTPanelBase> SUTMenuBase::GetActivePanel()
{
	return ActivePanel;
}

bool SUTMenuBase::SkipWorldRender()
{
	return false;
}

TSharedRef<SWidget> SUTMenuBase::BuildPlayerInfo()
{
	TSharedPtr<SVerticalBox> Container;
	SAssignNew(Container, SVerticalBox)
	+SVerticalBox::Slot().FillHeight(1.0).Padding(0.0f,0.0f,0.0f,0.0f)
	[
		SNew(SHorizontalBox)	
		+SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(PlayerOwner->GetOnlinePlayerNickname()))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
					.ColorAndOpacity(this, &SUTMenuBase::GetLabelColor)
				]
				+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth().Padding(5.0f,0.0f,5.0f,0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SUTMenuBase::GetXPLevelText) 
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						.ColorAndOpacity(this, &SUTMenuBase::GetLabelColor)
					]
					+SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Center)
						[
							SNew(SBox).WidthOverride(75).HeightOverride(8)
							[
								SNew(SProgressBar)
								.Style(SUTStyle::Get(),"UT.ProgressBar.XP")
								.Percent(this, &SUTMenuBase::GetXPLevelPercent)
							]
						]
					]
					+SHorizontalBox::Slot().AutoWidth().Padding(5.0f,3.0f,5.0f,0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SUTMenuBase::GetXPText) 
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Teenie")
						.ColorAndOpacity(this, &SUTMenuBase::GetLabelColor)
					]
				]
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth().Padding(5.0f, 0.0f, 5.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			// The Player's Avatar

			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SUTImage).WidthOverride(ICON_SIZE).HeightOverride(ICON_SIZE)
					.Image(this, &SUTMenuBase::GetAvatarImage)
				]
			]	
		]
	];

	return Container.ToSharedRef();
}

const FSlateBrush* SUTMenuBase::GetAvatarImage() const
{
	FName AvatarName = PlayerOwner->GetAvatar();
	return (AvatarName != NAME_None) ? SUTStyle::Get().GetBrush(AvatarName) : SUTStyle::Get().GetBrush("UT.Icon.PlayerCard");
}

FSlateColor SUTMenuBase::GetLabelColor() const
{
	return PlayerButton.IsValid() ? PlayerButton->GetLabelColor() : FSlateColor(FLinearColor::White);
}


void SUTMenuBase::HandleWindowActivated()
{
	FSlateApplication::Get().SetAllUserFocus(SharedThis(this), EFocusCause::WindowActivate);
}

FText SUTMenuBase::GetXPText() const
{
	return FText::Format(NSLOCTEXT("SUTMenuBase","LevelFormatB","({0}xp)"),FText::AsNumber(XP));
}

FText SUTMenuBase::GetXPLevelText() const
{
	return FText::Format(NSLOCTEXT("SUTMenuBase","LevelFormat","lvl.{0}"),FText::AsNumber(XPLevel));
}

TOptional<float> SUTMenuBase::GetXPLevelPercent() const
{
	return XPLevelPercent;
}


#endif