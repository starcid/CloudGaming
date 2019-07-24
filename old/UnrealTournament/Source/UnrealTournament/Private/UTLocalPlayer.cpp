// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "UTCharacter.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineTitleFileInterface.h"
#include "UTMenuGameMode.h"
#include "UTProfileSettings.h"
#include "UTGameViewportClient.h"
#include "Base/SUTMenuBase.h"
#include "SUTMainMenu.h"
#include "SUTServerBrowserPanel.h"
#include "SUTReplayBrowserPanel.h"
#include "SUTStatsViewerPanel.h"
#include "SUTCreditsPanel.h"
#include "SUTMessageBoxDialog.h"
#include "SUWindowsStyle.h"
#include "SUTStyle.h"
#include "SUTDialogBase.h"
#include "SUTToastBase.h"
#include "SUTWindowBase.h"
#include "SUTInputBoxDialog.h"
#include "SUTLoginDialog.h"
#include "SUTPlayerSettingsDialog.h"
#include "SUTPlayerInfoDialog.h"
#include "SUTWaitingForListenServerDialog.h"
#include "SUTHUDSettingsDialog.h"
#include "SUTQuickChatWindow.h"
#include "SUTWebMessage.h"
#include "SUTJoinInstanceWindow.h"
#include "SUTFriendsPopupWindow.h"
#include "SUTDownloadAllDialog.h"
#include "SUTVideoCompressionDialog.h"
#include "SUTMapVoteDialog.h"
#include "SUTReplayWindow.h"
#include "Menus/SUTReplayMenu.h"
#include "SUTAdminDialog.h"
#include "SUTSpectatorWindow.h"
#include "UTAnalytics.h"
#include "Base64.h"
#include "UTGameEngine.h"
#include "Engine/DemoNetDriver.h"
#include "UTConsole.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "UTVideoRecordingFeature.h"
#include "SUTYoutubeUploadDialog.h"
#include "SUTYoutubeConsentDialog.h"
#include "SUTMatchmakingDialog.h"
#include "UTLobbyGameState.h"
#include "UTLobbyPC.h"
#include "StatNames.h"
#include "UTChallengeManager.h"
#include "UTCharacterContent.h"
#include "Runtime/JsonUtilities/Public/JsonUtilities.h"
#include "SUTInGameHomePanel.h"
#include "UTMcpUtils.h"
#include "UTPlayerState.h"
#include "UTGameInstance.h"
#include "UTMatchmaking.h"
#include "UTMatchmakingPolicy.h"
#include "UTParty.h"
#include "UTPartyGameState.h"
#include "IBlueprintContextModule.h"
#include "SUTChatEditBox.h"
#include "SUTMatchmakingRegionDialog.h"
#include "SUTLeagueMatchResultsDialog.h"
#include "UserWidget.h"
#include "WidgetBlueprintLibrary.h"
#include "BlueprintContextLibrary.h"
#include "MatchmakingContext.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"
#include "UTKillcamPlayback.h"
#include "UTDemoNetDriver.h"
#include "UTAnalytics.h"
#include "QosInterface.h"
#include "QosModule.h"
#include "SUTReportUserDialog.h"
#include "UTUMGWidget_Toast.h"
#include "UTWorldSettings.h"
#include "UTSpectatorCamera.h"
#include "UTReplicatedGameRuleset.h"
#include "SUTDifficultyLevel.h"
#include "UTVoiceChatFeature.h"
#include "SUTDLCWarningDialog.h"

#if WITH_SOCIAL
#include "Social.h"
#endif

UUTLocalPlayer::UUTLocalPlayer(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInitialSignInAttempt = true;
	bAttemptedLauncherJoin = false;
	LastProfileCloudWriteTime = 0.0f;
	LastProgressionCloudWriteTime = 0.0f;
	ProfileCloudWriteCooldownTime = 12.0f;
	bShowSocialNotification = false;
	ServerPingBlockSize = 30;
	bSuppressToastsInGame = false;

	QuickMatchLimitTime = -60.0;
	RosterUpgradeText = FText::GetEmpty();
	CurrentSessionTrustLevel = 2;
	EarnedStars = 0;

	CloudProfileMagicNumberVersion1 = 0xBEEF0001;
	CloudProfileUE4VerForUnversionedProfile = 452;
	McpProfileManager = nullptr;
	bShowingFriendsMenu = false;

	bProgressionReadFromCloud = false;

	UIChatTextBackBufferPosition = 0;

	bIsPendingProgressionLoadFromMCP = false;
	bIsPendingProfileLoadFromMCP = false;

	bAutoRankLockWarningShown = false;
	bJoinSessionInProgress = false;

	KillcamPlayback = ObjectInitializer.CreateDefaultSubobject<UUTKillcamPlayback>(this, TEXT("KillcamPlayback"));
	LoginPhase = ELoginPhase::NotLoggedIn;
	bSuppressDownloadDialog = false;

	bQuickmatchOnLevelChange = false;
	RankedEloRange = 20;
	RankedMinEloRangeBeforeHosting = 100;
	RankedMinEloSearchStep = 20;
	
	QMEloRange = 100;
	QMMinEloRangeBeforeHosting = 300;
	QMMinEloSearchStep = 100;

	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Movement, TUTORIAL_Movement,TEXT("/Game/RestrictedAssets/Tutorials/TUT-MovementTraining"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTTutorialGameMode.UTTutorialGameMode_C?timelimit=0?botfill=1"), TEXT(""), TEXT("Movement Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Weapons, TUTOIRAL_Weapon,TEXT("/Game/RestrictedAssets/Tutorials/TUT-WeaponTraining"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTTutorialGameMode.UTTutorialGameMode_C?timelimit=0?botfill=1"), TEXT(""), TEXT("Weapons Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Pickups, TUTORIAL_Pickups,TEXT("/Game/RestrictedAssets/Tutorials/TUT-PickUpTraining"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTTutorialGameMode.UTTutorialGameMode_C?timelimit=0?botfill=1"), TEXT(""), TEXT("Pickups Tutorial")));

	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_DM, TUTORIAL_DM,TEXT("/Game/RestrictedAssets/Maps/DM-Underland"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTDMGameMode_Tut.UTDMGameMode_Tut_C?timelimit=5?GoalScore=0?botfill=4?Difficulty=2"), TEXT("TutorialMovies/dm-tutorial"), TEXT("Deathmatch Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Flagrun, TUTORIAL_FlagRun,TEXT("/Game/RestrictedAssets/Maps/WIP/FR-Fort"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTFlagrunGame_Tut.UTFlagrunGame_Tut_C?GoalScore=0?botfill=10?Difficulty=2"), TEXT("TutorialMovies/flagrun-tutorial"), TEXT("Flagrun Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Showdown, TUTORIAL_Showdown,TEXT("/Game/RestrictedAssets/Maps/DM-Chill"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTTeamShowdownGame_Tut.UTTeamShowdownGame_Tut_C?GoalScore=0?botfill=6?Difficulty=2"), TEXT("TutorialMovies/showdown-tutorial"), TEXT("Showdown Tutorial")));

	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_CTF, TUTORIAL_CTF,TEXT("/Game/RestrictedAssets/Maps/CTF-Face"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTCTFGameMode_Tut.UTCTFGameMode_Tut_C?timelimit=10?GoalScore=0?botfill=10?Difficulty=2"), TEXT("TutorialMovies/ctf-tutorial"), TEXT("Capture the Flag Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_TDM, TUTORIAL_TDM,TEXT("/Game/RestrictedAssets/Maps/DM-Outpost23"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTTeamDMGameMode_Tut.UTTeamDMGameMode_Tut_C?timelimit=10?GoalScore=0?botfill=10?Difficulty=2"), TEXT("TutorialMovies/tdm-tutorial"), TEXT("Team Deathmatch Tutorial")));
	TutorialData.Add(FTutorialData(ETutorialTags::TUTTAG_Duel, TUTORIAL_Duel,TEXT("/Game/RestrictedAssets/Maps/WIP/DM-ASDF"), TEXT("?game=/Game/RestrictedAssets/Tutorials/Blueprints/UTDuelGame_Tut.UTDuelGame_Tut_C?timelimit=10?GoalScore=0?botfill=2?Difficulty=3"), TEXT("TutorialMovies/duel-tutorial"), TEXT("Duel Tutorial")));

	bLaunchTutorialOnLogin = false;
}

UUTLocalPlayer::~UUTLocalPlayer()
{
	// Terminate the dedicated server if we started one
	if (DedicatedServerProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(DedicatedServerProcessHandle))
	{
		FPlatformProcess::TerminateProc(DedicatedServerProcessHandle);
	}
}

void UUTLocalPlayer::InitializeOnlineSubsystem()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) 
	{
		OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
		OnlineUserCloudInterface = OnlineSubsystem->GetUserCloudInterface();
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		OnlinePresenceInterface = OnlineSubsystem->GetPresenceInterface();
		OnlineFriendsInterface = OnlineSubsystem->GetFriendsInterface();
		OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
		OnlineUserInterface = OnlineSubsystem->GetUserInterface();
		OnlinePartyInterface = OnlineSubsystem->GetPartyInterface();
	}

	if (OnlineIdentityInterface.IsValid())
	{
		OnLoginCompleteDelegate = OnlineIdentityInterface->AddOnLoginCompleteDelegate_Handle(GetControllerId(), FOnLoginCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnLoginComplete));
		OnLoginStatusChangedDelegate = OnlineIdentityInterface->AddOnLoginStatusChangedDelegate_Handle(GetControllerId(), FOnLoginStatusChangedDelegate::CreateUObject(this, &UUTLocalPlayer::OnLoginStatusChanged));
		OnLogoutCompleteDelegate = OnlineIdentityInterface->AddOnLogoutCompleteDelegate_Handle(GetControllerId(), FOnLogoutCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnLogoutComplete));
	}

	if (OnlineUserCloudInterface.IsValid())
	{
		OnWriteUserFileCompleteDelegate = OnlineUserCloudInterface->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnWriteUserFileComplete));
		OnDeleteUserFileCompleteDelegate = OnlineUserCloudInterface->AddOnDeleteUserFileCompleteDelegate_Handle(FOnDeleteUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnDeleteUserFileComplete));
		OnEnumerateUserFilesCompleteDelegate = OnlineUserCloudInterface->AddOnEnumerateUserFilesCompleteDelegate_Handle(FOnEnumerateUserFilesCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnEnumerateUserFilesComplete));
	}

	if (OnlineSessionInterface.IsValid())
	{
		OnEndSessionCompleteDelegate = OnlineSessionInterface->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnEndSessionComplete));
		OnDestroySessionCompleteDelegate = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnDestroySessionComplete));
		OnFindFriendSessionCompleteDelegate = OnlineSessionInterface->AddOnFindFriendSessionCompleteDelegate_Handle(0, FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnFindFriendSessionComplete));
	}

	IOnlineVoicePtr VoiceInt = OnlineSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		SpeakerDelegate = VoiceInt->AddOnPlayerTalkingStateChangedDelegate_Handle(FOnPlayerTalkingStateChangedDelegate::CreateUObject(this, &UUTLocalPlayer::OnPlayerTalkingStateChanged));
	}
}

void UUTLocalPlayer::CleanUpOnlineSubSystyem()
{
	if (OnlineSubsystem)
	{
		if (OnlineIdentityInterface.IsValid())
		{
			OnlineIdentityInterface->ClearOnLoginCompleteDelegate_Handle(GetControllerId(), OnLoginCompleteDelegate);
			OnlineIdentityInterface->ClearOnLoginStatusChangedDelegate_Handle(GetControllerId(), OnLoginStatusChangedDelegate);
			OnlineIdentityInterface->ClearOnLogoutCompleteDelegate_Handle(GetControllerId(), OnLogoutCompleteDelegate);
		}
		if (OnlineUserCloudInterface.IsValid())
		{
			OnlineUserCloudInterface->ClearOnWriteUserFileCompleteDelegate_Handle(OnWriteUserFileCompleteDelegate);
			OnlineUserCloudInterface->ClearOnDeleteUserFileCompleteDelegate_Handle(OnDeleteUserFileCompleteDelegate);
			OnlineUserCloudInterface->ClearOnEnumerateUserFilesCompleteDelegate_Handle(OnEnumerateUserFilesCompleteDelegate);
		}
		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
			OnlineSessionInterface->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
			OnlineSessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegate);
		}

		IOnlineVoicePtr VoiceInt = OnlineSubsystem->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			VoiceInt->ClearOnPlayerTalkingStateChangedDelegate_Handle(SpeakerDelegate);
		}
	}
}

bool UUTLocalPlayer::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// disallow certain commands in shipping builds
#if UE_BUILD_SHIPPING
	if (FParse::Command(&Cmd, TEXT("SHOW")))
	{
		return true;
	}
#endif
	return Super::Exec(InWorld, Cmd, Ar);
}

bool UUTLocalPlayer::IsAFriend(FUniqueNetIdRepl PlayerId)
{
	return (PlayerId.IsValid() && OnlineFriendsInterface.IsValid() && OnlineFriendsInterface->IsFriend(0, *PlayerId, EFriendsLists::ToString(EFriendsLists::InGamePlayers)));
}

FString UUTLocalPlayer::GetNickname() const
{
	FString OfficialName = Super::GetNickname();
	return (OfficialName.IsEmpty() || (OfficialName == TEXT("InvalidMCPUser"))) ? PlayerNickname : OfficialName;
}

FString UUTLocalPlayer::SetClanName(FString NewClanName)
{
	ClanName = NewClanName;
	if (Cast<AUTBasePlayerController>(PlayerController))
	{
		Cast<AUTBasePlayerController>(PlayerController)->ServerChangeClanName(ClanName);
	}
	CurrentProfileSettings->ClanName = ClanName;
	return ClanName;
}

FString UUTLocalPlayer::GetClanName() const
{
	return ClanName;
}

FText UUTLocalPlayer::GetAccountDisplayName() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{

		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				return FText::FromString(UserAccount->GetDisplayName());
			}
		}
	}
	return FText::GetEmpty();
}

FString UUTLocalPlayer::GetAccountName() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				FString Result;
				UserAccount->GetAuthAttribute(TEXT("email"), Result);
				return Result;
			}
		}
	}
	return TEXT("");
}

FText UUTLocalPlayer::GetAccountSummary() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{

		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				return FText::Format(NSLOCTEXT("UTLocalPlayer","AccountSummaryFormat","{0} # of Friends: {1}  # Online: {2}"), FText::FromString(UserAccount->GetDisplayName()), FText::AsNumber(0),FText::AsNumber(0));
			}
		}
	}
	return FText::GetEmpty();
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName SystemInfo
*
* @Trigger Sent when a player joins a map
*
* @Type Sent by the Client
*
* @EventParam OSMajor string Major OS Version
* @EventParam OSMinor string Minor OS Version
* @EventParam CPUVendor string CPU Vendor
* @EventParam CPUBrand string CPU Brand
*
* @Comments
*/
void UUTLocalPlayer::PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID)
{
#if !UE_SERVER
	SUWindowsStyle::Initialize();
	SUTStyle::Initialize();
#endif

	Super::PlayerAdded(InViewportClient, InControllerID);

	if (FUTAnalytics::IsAvailable())
	{
		FString OSMajor;
		FString OSMinor;
		FPlatformMisc::GetOSVersions(OSMajor, OSMinor);
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("OSMajor"), OSMajor));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("OSMinor"), OSMinor));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("CPUVendor"), FPlatformMisc::GetCPUVendor()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("CPUBrand"), FPlatformMisc::GetCPUBrand()));
		FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerController), ParamArray, false);
		FUTAnalytics::GetProvider().RecordEvent( TEXT("SystemInfo"), ParamArray );
	}

	if (!InViewportClient->GetWorld()->IsPlayInEditor())
	{
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			// Initialize the Online Subsystem for this player
			InitializeOnlineSubsystem();

			if ( FParse::Param(FCommandLine::Get(), TEXT("playoffline")))
			{
				LoginPhase = ELoginPhase::Offline;
				bPlayingOffline = true;
				bInitialSignInAttempt = false;
			}
			else if (OnlineIdentityInterface.IsValid())
			{
				// Attempt to Auto-Login to MCP
				if (OnlineIdentityInterface->AutoLogin(GetControllerId()))
				{
					LoginPhase = ELoginPhase::Auth;
				}
				else
				{
					bInitialSignInAttempt = false;
				}
			}
		}
	}

	if (!IsRunningDedicatedServer())
	{
		if (IBlueprintContextModule* BlueprintContext = FModuleManager::GetModulePtr< IBlueprintContextModule >("BlueprintContext"))
		{
			BlueprintContext->LocalPlayerAdded(this);
		}
	}
}

void UUTLocalPlayer::PlayerRemoved()
{
	if (!IsRunningDedicatedServer())
	{
		if (IBlueprintContextModule* BlueprintContext = FModuleManager::GetModulePtr< IBlueprintContextModule >("BlueprintContext"))
		{
			BlueprintContext->LocalPlayerRemoved(this);
		}
	}
}

bool UUTLocalPlayer::IsMenuGame()
{
	if (bNoMidGameMenu) return true;

	if (GetWorld()->GetNetMode() == NM_Standalone)
	{
		AUTMenuGameMode* GM = GetWorld()->GetAuthGameMode<AUTMenuGameMode>();
		return GM != NULL;
	}
	return false;
}

#if !UE_SERVER
void UUTLocalPlayer::OpenWindow(TSharedPtr<SUTWindowBase> WindowToOpen, int32 ZOrder)
{
	// Make sure this window isn't already in the stack.
	if (WindowStack.Find(WindowToOpen) == INDEX_NONE)
	{
		// Toggle on the proper input mode
		if (PlayerController)
		{
			AUTBasePlayerController* BasePlayerController = Cast<AUTBasePlayerController>(PlayerController);
			if (BasePlayerController)
			{
				BasePlayerController->UpdateInputMode();
			}
		}

		GEngine->GameViewport->AddViewportWidgetContent(WindowToOpen.ToSharedRef(), ZOrder);
		
		WindowStack.Add(WindowToOpen);
		WindowToOpen->Open();
	}
}

bool UUTLocalPlayer::CloseWindow(TSharedPtr<SUTWindowBase> WindowToClose)
{
	// Find this window in the stack.
	if (WindowStack.Find(WindowToClose) != INDEX_NONE)
	{
		return WindowToClose->Close();
	}

	return false;
}

void UUTLocalPlayer::WindowClosed(TSharedPtr<SUTWindowBase> WindowThatWasClosed)
{
	if (WindowStack.Find(WindowThatWasClosed) != INDEX_NONE)
	{
		if (GEngine->GameViewport != NULL) // might already be gone if in PIE
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(WindowThatWasClosed.ToSharedRef());
		}
		WindowStack.Remove(WindowThatWasClosed);
	}
}
#endif


void UUTLocalPlayer::ShowMenu(const FString& Parameters)
{
#if !UE_SERVER

	if (PlayerController) PlayerController->FlushPressedKeys();

	if (QuickChatWindow.IsValid())
	{
		CloseQuickChat();
	}

	if (bRecordingReplay)
	{
		static const FName VideoRecordingFeatureName("VideoRecording");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
		{
			UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
			if (VideoRecorder)
			{
				VideoRecorder->CancelRecording();
			}
		}
	}

	// If we have a menu up, hide it before opening the new one
	if (DesktopSlateWidget.IsValid())
	{
		UE_LOG(UT,Log,TEXT("Closing Existing menu so we can open a new one"));
		HideMenu();
	}

	// Create the slate widget if it doesn't exist
	if (!DesktopSlateWidget.IsValid())
	{
		if ( IsMenuGame() )
		{
			SAssignNew(DesktopSlateWidget, SUTMainMenu).PlayerOwner(this);
		}
		else if (IsReplay())
		{
			SAssignNew(DesktopSlateWidget, SUTReplayMenu).PlayerOwner(this);
		}
		else
		{
			AGameState* GameState = GetWorld()->GetGameState<AGameState>();
			if (GameState != nullptr && GameState->GameModeClass != nullptr)
			{
				AUTBaseGameMode* UTGameMode = GameState->GameModeClass->GetDefaultObject<AUTBaseGameMode>();
				if (UTGameMode != nullptr)
				{
					DesktopSlateWidget = UTGameMode->GetGameMenu(this);
				}
			}

		}
		if (DesktopSlateWidget.IsValid())
		{
			GEngine->GameViewport->AddViewportWidgetContent( SNew(SWeakWidget).PossiblyNullContent(DesktopSlateWidget.ToSharedRef()),2);
		}

		// Make it visible.
		if ( DesktopSlateWidget.IsValid() )
		{
			// Widget is already valid, just make it visible.
			DesktopSlateWidget->SetVisibility(EVisibility::Visible);
			if (PlayerController)
			{
				AUTBasePlayerController* BasePlayerController = Cast<AUTBasePlayerController>(PlayerController);
				if (BasePlayerController)
				{
					BasePlayerController->UpdateInputMode();
				}

				if (!IsMenuGame())
				{
					// If we are in a single player game, and that game is either in the player intro or the post match state, then
					// clear the menu pause.

					if (GetWorld()->GetNetMode() != NM_Client)
					{
						PlayerController->SetPause(true);
					}
				}
				else
				{
					PlayerController->SetPause(false);
				}
			}
			DesktopSlateWidget->OnMenuOpened(Parameters);
			CenterMouseCursor();
		}
	}

#endif
}
void UUTLocalPlayer::HideMenu()
{
#if !UE_SERVER

	UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
	if (GI && GI->IsMoviePlaying())
	{
		bHideMenuCalledDuringMoviePlayback = true;
		return;
	}
	
	bHideMenuCalledDuringMoviePlayback = false;

	if (ContentLoadingMessage.IsValid())
	{
		UE_LOG(UT,Log,TEXT("Can't close menus during loading"));
		return; // Don't allow someone to close the menu while we are loading....
	}

	if (DesktopSlateWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DesktopSlateWidget.ToSharedRef());
		DesktopSlateWidget->OnMenuClosed();
		DesktopSlateWidget.Reset();
		if (PlayerController)
		{
			PlayerController->SetPause(false);
		}

		if (SpectatorWidget.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(SpectatorWidget, EKeyboardFocusCause::Keyboard);
		}

		if (ReplayWindow.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(ReplayWindow, EKeyboardFocusCause::Keyboard);
		}
	}
	else
	{
		UE_LOG(UT,Log,TEXT("Call to HideMenu() when without a menu being opened."));
	}
	CloseConnectingDialog();
#endif
}

void UUTLocalPlayer::OpenTutorialMenu()
{
#if !UE_SERVER
	if (IsMenuGame() && DesktopSlateWidget.IsValid())
	{
		StaticCastSharedPtr<SUTMainMenu>(DesktopSlateWidget)->OpenTutorialMenu();
	}
#endif
}

void UUTLocalPlayer::MessageBox(FText MessageTitle, FText MessageText)
{
#if !UE_SERVER
	ShowMessage(MessageTitle, MessageText, UTDIALOG_BUTTON_OK, NULL);
#endif
}

#if !UE_SERVER
TSharedPtr<class SUTDialogBase>  UUTLocalPlayer::ShowMessage(FText MessageTitle, FText MessageText, uint16 Buttons, const FDialogResultDelegate& Callback, FVector2D DialogSize)
{
	TSharedPtr<class SUTDialogBase> NewDialog;
	if (DialogSize.IsNearlyZero())
	{
		SAssignNew(NewDialog, SUTMessageBoxDialog)
			.PlayerOwner(this)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}
	else
	{
		SAssignNew(NewDialog, SUTMessageBoxDialog)
			.PlayerOwner(this)
			.bDialogSizeIsRelative(true)
			.DialogSize(DialogSize)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}

	OpenDialog( NewDialog.ToSharedRef(), 2001);
	return NewDialog;
}

TSharedPtr<class SUTDialogBase> UUTLocalPlayer::ShowSupressableConfirmation(FText MessageTitle, FText MessageText, FVector2D DialogSize, bool &InOutShouldSuppress, const FDialogResultDelegate& Callback)
{
	auto OnGetSuppressibleState = [&InOutShouldSuppress]() { return InOutShouldSuppress ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };

	auto OnSetSuppressibleState = [&InOutShouldSuppress](ECheckBoxState CheckBoxState) 
	{ 
		InOutShouldSuppress = CheckBoxState == ECheckBoxState::Checked;
	};

	TSharedPtr<class SUTDialogBase> NewDialog;
	if (DialogSize.IsNearlyZero())
	{
		SAssignNew(NewDialog, SUTMessageBoxDialog)
			.PlayerOwner(this)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(UTDIALOG_BUTTON_OK)
			.OnDialogResult(Callback)
			.IsSuppressible(true)
			.SuppressibleCheckBoxState_Lambda( OnGetSuppressibleState )
			.OnSuppressibleCheckStateChanged_Lambda( OnSetSuppressibleState );
	}
	else
	{
		SAssignNew(NewDialog, SUTMessageBoxDialog)
			.PlayerOwner(this)
			.bDialogSizeIsRelative(true)
			.DialogSize(DialogSize)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(UTDIALOG_BUTTON_OK)
			.IsSuppressible(true)
			.SuppressibleCheckBoxState_Lambda(OnGetSuppressibleState)
			.OnSuppressibleCheckStateChanged_Lambda(OnSetSuppressibleState)
			.OnDialogResult(Callback);
	}

	OpenDialog(NewDialog.ToSharedRef());
	return NewDialog;
}

void UUTLocalPlayer::OpenDialog(TSharedRef<SUTDialogBase> Dialog, int32 ZOrder)
{
	GEngine->GameViewport->AddViewportWidgetContent(Dialog, ZOrder);
	Dialog->ZOrder = ZOrder;
	Dialog->OnDialogOpened();
	OpenDialogs.Add(Dialog);
}

void UUTLocalPlayer::CloseDialog(TSharedRef<SUTDialogBase> Dialog)
{
	OpenDialogs.Remove(Dialog);
	Dialog->OnDialogClosed();
	GEngine->GameViewport->RemoveViewportWidgetContent(Dialog);

	if (DesktopSlateWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(DesktopSlateWidget, EKeyboardFocusCause::Keyboard);
	}
}

#if !UE_SERVER
void UUTLocalPlayer::ShowGameAbandonedDialog()
{
	GameAbandonedDialog = ShowMessage(NSLOCTEXT("UUTLocalPlayer", "RankedGameAbandonedTitle", "Game Abandoned"), NSLOCTEXT("AUTBasePlayerController", "RankedGameAbandoned", "There were not enough players to start that match so it was abandoned."), UTDIALOG_BUTTON_OK);
}
#endif

TSharedPtr<class SUTServerBrowserPanel> UUTLocalPlayer::GetServerBrowser(bool bCreate)
{
	if (!ServerBrowserWidget.IsValid() && bCreate)
	{
		SAssignNew(ServerBrowserWidget, SUTServerBrowserPanel, this);
	}

	return ServerBrowserWidget;
}

TSharedPtr<class SUTReplayBrowserPanel> UUTLocalPlayer::GetReplayBrowser()
{
	if (!ReplayBrowserWidget.IsValid())
	{
		SAssignNew(ReplayBrowserWidget, SUTReplayBrowserPanel, this);
	}

	return ReplayBrowserWidget;
}

TSharedPtr<class SUTStatsViewerPanel> UUTLocalPlayer::GetStatsViewer()
{
	if (!StatsViewerWidget.IsValid())
	{
		SAssignNew(StatsViewerWidget, SUTStatsViewerPanel, this);
	}

	return StatsViewerWidget;
}

TSharedPtr<class SUTCreditsPanel> UUTLocalPlayer::GetCreditsPanel()
{
	if (!CreditsPanelWidget.IsValid())
	{
		SAssignNew(CreditsPanelWidget, SUTCreditsPanel, this);
	}

	return CreditsPanelWidget;
}


#endif

bool UUTLocalPlayer::AreMenusOpen()
{
#if !UE_SERVER
	// TODO: Ugly hack.  We don't have a good way right now to notify the player controller that UMG wants input.  So I hack it here for
	// tutorial menu.  Need to add a better way next build.
	TArray<UUserWidget*> UMGWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), UMGWidgets, UUserWidget::StaticClass(), true);
	bool UMGWidgetsPresent = false;
	for (int32 i = 0; i < UMGWidgets.Num(); i++)
	{
		if (UMGWidgets[i]->GetName().Contains(TEXT("TutFinishScreenWidget_C"),ESearchCase::IgnoreCase))
		{
			UMGWidgetsPresent = true;
			break;
		}

		if (UMGWidgets[i]->IsInteractable())
		{
			UMGWidgetsPresent = true;
		}
	}
	
	return DesktopSlateWidget.IsValid()
		|| LoginDialog.IsValid()
		|| QuickChatWindow.IsValid()
		|| OpenDialogs.Num() > 0
		|| UMGWidgetsPresent;
#endif

	return false;
}



void UUTLocalPlayer::ChangeStatsViewerTarget(FString InStatsID)
{
#if !UE_SERVER
	if (StatsViewerWidget.IsValid())
	{
		StatsViewerWidget->ChangeStatsID(InStatsID);
	}
#endif
}

void UUTLocalPlayer::ShowHUDSettings()
{
#if !UE_SERVER
	if (!HUDSettings.IsValid())
	{
		SAssignNew(HUDSettings, SUTHUDSettingsDialog)
			.PlayerOwner(this);

		OpenDialog( HUDSettings.ToSharedRef() );

		if (PlayerController)
		{
			if (!IsMenuGame())
			{
				PlayerController->SetPause(true);
			}
		}
	}
#endif
}

void UUTLocalPlayer::HideHUDSettings()
{
#if !UE_SERVER

	if (HUDSettings.IsValid())
	{
		CloseDialog(HUDSettings.ToSharedRef());
		HUDSettings.Reset();

		if (!IsMenuGame())
		{
			if (PlayerController)
			{
				PlayerController->SetPause(false);
			}
		}
	} 
#endif
}

bool UUTLocalPlayer::IsLoggedIn() const
{ 
	return OnlineIdentityInterface.IsValid() && OnlineIdentityInterface->GetLoginStatus(GetControllerId());
}


void UUTLocalPlayer::LoginOnline(FString EpicID, FString Auth, bool bIsRememberToken, bool bSilentlyFail)
{
	if ( !OnlineIdentityInterface.IsValid() ) return;

	FString Override;
	if ( FParse::Value(FCommandLine::Get(),TEXT("-id="),Override))
	{
		EpicID = Override;
	}

	if ( FParse::Value(FCommandLine::Get(),TEXT("-pass="),Override))
	{
		Auth=Override;
		bIsRememberToken=false;
	}

	if (EpicID == TEXT("") && !FParse::Param(FCommandLine::Get(), TEXT("skiplastid")))
	{
		EpicID = LastEpicIDLogin;
	}

	// Save this for later.
	PendingLoginUserName = EpicID;
	bSilentLoginFail = bSilentlyFail;

	if (EpicID == TEXT("") || Auth == TEXT(""))
	{
		GetAuth();
		return;
	}
	else if (!bSilentlyFail)
	{
		ShowAuth();
	}

	FOnlineAccountCredentials AccountCreds(TEXT("epic"), EpicID, Auth);
	if (bIsRememberToken)
	{
		AccountCreds.Type = TEXT("refresh");
	}

	// Begin the Login Process...
	if (!OnlineIdentityInterface->Login(GetControllerId(), AccountCreds))
	{
#if !UE_SERVER
		// We should never fail here unless something has gone horribly wrong
		if (bSilentLoginFail)
		{
			UE_LOG(UT, Warning, TEXT("Could not connect to the online subsystem. Please check your connection and try again."));
		}
		else
		{
			ShowMessage(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "UnknownLoginFailuire", "Could not connect to the online subsystem.  Please check your connection and try again."), UTDIALOG_BUTTON_OK, NULL);
		}
		return;
#endif
	}
	else
	{
		LoginPhase = ELoginPhase::Auth;

#if !UE_SERVER
		if (LoginDialog.IsValid()) LoginDialog->BeginLogin();
#endif
		bLoginAttemptInProgress = true;
	}
}

void UUTLocalPlayer::Logout()
{
	bIsPendingProfileLoadFromMCP = false;
	bIsPendingProgressionLoadFromMCP = false;

	if (IsLoggedIn() && OnlineIdentityInterface.IsValid())
	{
		UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
		if (UTGameInstance)
		{
			UUTParty* Party = UTGameInstance->GetParties();
			if (Party)
			{
				TSharedPtr<const FUniqueNetId> UniqueNetId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
				if (UniqueNetId.IsValid())
				{
					Party->LeavePersistentParty(*UniqueNetId);
				}
			}
		}

		// Begin the Login Process....
		if (!OnlineIdentityInterface->Logout(GetControllerId()))
		{
#if !UE_SERVER
			// We should never fail here unless something has gone horribly wrong
			ShowMessage(NSLOCTEXT("MCPMessages","OnlineError","Online Error"), NSLOCTEXT("MCPMessages","UnknownLogoutFailuire","Could not log out from the online subsystem.  Please check your connection and try again."), UTDIALOG_BUTTON_OK, NULL);
			return;
#endif
		}
	}

#if WITH_SOCIAL
	ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->Logout();
#endif
}


FString UUTLocalPlayer::GetOnlinePlayerNickname()
{
	return IsLoggedIn() ? OnlineIdentityInterface->GetPlayerNickname(0) : TEXT("None");
}

void UUTLocalPlayer::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UniqueID, const FString& ErrorMessage)
{
	bLoginAttemptInProgress = false;

	if (bWasSuccessful)
	{
		bPlayingOffline = false;
		bInitialSignInAttempt = false;

		// Save the creds for the next auto-login

		TSharedPtr<FUserOnlineAccount> Account = OnlineIdentityInterface->GetUserAccount(UniqueID);
		if (Account.IsValid())
		{
			FString RememberMeToken;
			FString Token;
			Account->GetAuthAttribute(TEXT("refresh_token"), RememberMeToken);

			if ( Account->GetAuthAttribute(TEXT("ut:developer"), Token) )			CommunityRole = EUnrealRoles::Developer;
			else if ( Account->GetAuthAttribute(TEXT("ut:contributor"), Token) )	CommunityRole = EUnrealRoles::Contributor;
			else if ( Account->GetAuthAttribute(TEXT("ut:concepter"), Token) )		CommunityRole = EUnrealRoles::Concepter;
			else if ( Account->GetAuthAttribute(TEXT("ut:prototyper"), Token) )		CommunityRole = EUnrealRoles::Prototyper;
			else if ( Account->GetAuthAttribute(TEXT("ut:marketplace"), Token) )	CommunityRole = EUnrealRoles::Marketplace;
			else if ( Account->GetAuthAttribute(TEXT("ut:ambassador"), Token) )		CommunityRole = EUnrealRoles::Ambassador;
			else 
			{
				CommunityRole = EUnrealRoles::Gamer;
			}

			LastEpicIDLogin = PendingLoginUserName;
			LastEpicRememberMeToken = RememberMeToken;
			SaveConfig();

			// Now download initial profiles.
			TSharedPtr<const FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdString(UniqueID));
#if WITH_PROFILE
			GetMcpProfileManager()->Init(UserId, UserId, Account->GetDisplayName(), FUTProfilesLoaded::CreateUObject(this, &UUTLocalPlayer::OnProfileManagerInitComplete));
#endif

			AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerController);
			if (UTPC)
			{
				if (OnlineIdentityInterface.IsValid() && OnlineIdentityInterface->GetLoginStatus(GetControllerId()))
				{
					if (UserId.IsValid())
					{
						UTPC->ServerReceiveStatsID(UserId->ToString());
						if (UTPC->PlayerState)
						{
							UTPC->PlayerState->SetUniqueId(UserId); 
						}

						CreatePersistentParty();
					}
				}
			}
		}

		PendingLoginUserName = TEXT("");

		RemoveCosmeticsFromDefaultURL();
	}
	else
	{

		// We have enough credentials to auto-login.  So try it, but silently fail if we cant.
		if (bInitialSignInAttempt)
		{
			if (LastEpicIDLogin != TEXT("") && LastEpicRememberMeToken != TEXT("") && !FParse::Param(FCommandLine::Get(), TEXT("skiplastid")))
			{
				bInitialSignInAttempt = false;
				LoginOnline(LastEpicIDLogin, LastEpicRememberMeToken, true, true);
			}
		}
		// Otherwise if this is the first attempt, then silently fair
		else 
		{
			if (!bSilentLoginFail)
			{
				// Broadcast the failure to the UI.
				PlayerOnlineStatusChanged.Broadcast(this, ELoginStatus::NotLoggedIn, UniqueID);
				GetAuth(ErrorMessage);
			}

#if UE_SERVER
			LoginPhase = ELoginPhase::NotLoggedIn;
#else
			LoginPhase = LoginDialog.IsValid() ? ELoginPhase::InDialog : ELoginPhase::NotLoggedIn;
#endif
		}
	}


}

// When MCP gets the XMPP login, it wipes parties of 1, other games have multiple step logins that mask this race condition, UT does not.
// Avoid race condition with lazy SetTimer for now until XMPP delegates can be created
void UUTLocalPlayer::CreatePersistentParty()
{
	GetWorld()->GetTimerManager().SetTimer(PersistentPartyCreationHandle, this, &UUTLocalPlayer::DelayedCreatePersistentParty, 2.0f, false);
}

void UUTLocalPlayer::DelayedCreatePersistentParty()
{
	// Null gamestate will blow up party creation in the engine
	if (GetWorld()->GetGameState() == nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(PersistentPartyCreationHandle, this, &UUTLocalPlayer::DelayedCreatePersistentParty, 2.0f, false);
		return;
	}

	if (OnlineIdentityInterface.IsValid() && OnlineIdentityInterface->GetLoginStatus(GetControllerId()))
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
			if (UTGameInstance)
			{
				UUTParty* UTParty = UTGameInstance->GetParties();
				if (UTParty)
				{
					UTParty->CreatePersistentParty(*UserId, UPartyDelegates::FOnCreateUPartyComplete::CreateUObject(this, &ThisClass::PersistentPartyCreated));
				}
			}
		}
	}
}

void UUTLocalPlayer::PersistentPartyCreated(const FUniqueNetId& LocalUserId, const ECreatePartyCompletionResult Result)
{
	if (!bAttemptedLauncherJoin)
	{
		bAttemptedLauncherJoin = true;
		FString FriendId;
		if (FParse::Value(FCommandLine::Get(), TEXT("invitefrom="), FriendId) && !FriendId.IsEmpty())
		{
			UE_LOG(LogParty, Display, TEXT("Attempting to join party of inviter %s"), *FriendId);
			UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UPartyContext::StaticClass()));
			if (PartyContext)
			{
				TSharedPtr<const FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdString(FriendId));
				PartyContext->JoinParty(*UserId);
			}
			else
			{
				UE_LOG(LogParty, Warning, TEXT("Party context missing!"));
			}
		}
	}
}

void UUTLocalPlayer::GetAuth(FString ErrorMessage)
{
#if !UE_SERVER
	if (GetWorld()->IsPlayInEditor())
	{
		return;
	}

	if (LoginDialog.IsValid())
	{
		LoginDialog->SetErrorText(FText::FromString(ErrorMessage));
		return;
	}
	ShowAuth();
#endif
}

void UUTLocalPlayer::ShowAuth()
{
#if !UE_SERVER
	if (!LoginDialog.IsValid())
	{
		SAssignNew(LoginDialog, SUTLoginDialog)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::AuthDialogClosed))
		.UserIDText(PendingLoginUserName)
		.PlayerOwner(this);

		GEngine->GameViewport->AddViewportWidgetContent(LoginDialog.ToSharedRef(), 500);
		LoginDialog->SetInitialFocus();
	}

	if (LoginPhase == ELoginPhase::NotLoggedIn || LoginPhase == ELoginPhase::LoggedIn || LoginPhase == ELoginPhase::Offline)
	{
		LoginPhase = ELoginPhase::InDialog;
	}
#endif
}

void UUTLocalPlayer::OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UniqueID)
{
	UE_LOG(UT,Warning,TEXT("***[LoginStatusChanged]*** - User %i - %i"), LocalUserNum, int32(LoginStatus));
	
	// If we have logged out, or started using the local profile, then clear the online profile.
	if (LoginStatus == ELoginStatus::NotLoggedIn || LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		bIsPendingProfileLoadFromMCP = false;
		bIsPendingProgressionLoadFromMCP = false;

		// Clear out the MCP storage
		MCPPulledData.bValid = false;

		CurrentProfileSettings = NULL;
		LoadLocalProfileSettings();
		FUTAnalytics::LoginStatusChanged(FString());
		
		OnPlayerLoggedOut().Broadcast();

		// Close the friends panel if it's showing.
		if (bShowingFriendsMenu)
		{
			ToggleFriendsAndChat();
		}

		if (bPendingLoginCreds)
		{
			bPendingLoginCreds = false;
			LoginOnline(PendingLoginName, PendingLoginPassword);
			PendingLoginPassword = TEXT("");
		}

		ReturnToMainMenu();		
	}
	else if (LoginStatus == ELoginStatus::LoggedIn)
	{

		LoginPhase = ELoginPhase::GettingProfile;
		LeagueRecords.Empty();
		MMREntries.Empty();
		
		UpdatePresence(LastPresenceUpdate, bLastAllowInvites,bLastAllowInvites,bLastAllowInvites,false);
		ReadCloudFileListing();
		// query entitlements for UI
		IOnlineEntitlementsPtr EntitlementsInterface = OnlineSubsystem->GetEntitlementsInterface();
		if (EntitlementsInterface.IsValid())
		{
			EntitlementsInterface->QueryEntitlements(UniqueID, TEXT("ut"));
		}
		FUTAnalytics::LoginStatusChanged(UniqueID.ToString());

		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerController);
		if (PC)
		{
			PC->ClientGenericInitialization();
		}

		OnPlayerLoggedIn().Broadcast();
	}


	PlayerOnlineStatusChanged.Broadcast(this, LoginStatus, UniqueID);
}

void UUTLocalPlayer::ShowRankedReconnectDialog(const FString& UniqueID)
{
#if !UE_SERVER
	if (!UniqueID.IsEmpty() && !LastRankedMatchPlayerId.IsEmpty() && !LastRankedMatchSessionId.IsEmpty() && UniqueID == LastRankedMatchPlayerId)
	{
		FDateTime LastRankedMatchTime;
		if ( !LastRankedMatchTimeString.IsEmpty() && FDateTime::Parse(LastRankedMatchTimeString, LastRankedMatchTime) )
		{
			if ((FDateTime::Now() - LastRankedMatchTime).GetTotalMinutes() <= 5.0f)
			{
				UE_LOG(UT,Warning,TEXT(""));
				UE_LOG(UT,Warning,TEXT("RECONNECT DEBUG: id Check = '%s' vs '%s'"), *UniqueID, *LastRankedMatchPlayerId);
				UE_LOG(UT,Warning,TEXT("RECONNECT DEBUG: SessionId Check = '%s'"), *LastRankedMatchSessionId);
				UE_LOG(UT,Warning,TEXT("RECONNECT DEBUG: Date Check = '%s' vs '%s' = %f"), *FDateTime::Now().ToString(), *LastRankedMatchTime.ToString(),(FDateTime::Now() - LastRankedMatchTime).GetTotalMinutes());
				UE_LOG(UT,Warning,TEXT(""));

				// Ask player if they want to try to rejoin last ranked game
				ShowMessage(NSLOCTEXT("UTLocalPlayer", "RankedReconnectTitle", "Reconnect To Last Match?"),
					NSLOCTEXT("UTLocalPlayer", "RankedReconnect", "Would you like to reconnect to the last match?"),
					UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO,
					FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::RankedReconnectResult));
			}
		}
	}
#endif
}

#if !UE_SERVER
void UUTLocalPlayer::RankedReconnectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UMatchmakingContext::StaticClass()));
		if (MatchmakingContext)
		{
			MatchmakingContext->AttemptReconnect(LastRankedMatchSessionId);
		}
	}

	LastRankedMatchPlayerId.Empty();
	LastRankedMatchSessionId.Empty();
	LastRankedMatchTimeString.Empty();
	SaveConfig();
}
#endif

void UUTLocalPlayer::ReadCloudFileListing()
{
	if (OnlineUserCloudInterface.IsValid() && OnlineIdentityInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			OnlineUserCloudInterface->EnumerateUserFiles(*UserId.Get());
		}
	}
}

void UUTLocalPlayer::OnEnumerateUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& InUserId)
{
	UE_LOG(UT, Verbose, TEXT("OnEnumerateUserFilesComplete %d"), bWasSuccessful ? 1 : 0);
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		UTEngine->CloudContentChecksums.Empty();

		if (OnlineUserCloudInterface.IsValid() && OnlineIdentityInterface.IsValid())
		{
			TArray<FCloudFileHeader> UserFiles;
			OnlineUserCloudInterface->GetUserFileList(InUserId, UserFiles);
			for (int32 i = 0; i < UserFiles.Num(); i++)
			{
				TArray<uint8> DecodedHash;
				FBase64::Decode(UserFiles[i].Hash, DecodedHash);
				FString Hash = BytesToHex(DecodedHash.GetData(), DecodedHash.Num());
				UE_LOG(UT, Verbose, TEXT("%s %s"), *UserFiles[i].FileName, *Hash);
				UTEngine->CloudContentChecksums.Add(FPaths::GetBaseFilename(UserFiles[i].FileName), Hash);
			}
		}		

		LoadProfileSettings();

	}
}

void UUTLocalPlayer::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	UE_LOG(UT,Verbose,TEXT("***[Logout Complete]*** - User %i"), LocalUserNum);
	// TO-DO: Add a Toast system for displaying stuff like this

	LoginPhase = ELoginPhase::Offline;
	bPlayingOffline = true;

	GetWorld()->GetTimerManager().ClearTimer(ProfileWriteTimerHandle);

#if !UE_SERVER
	if (ServerBrowserWidget.IsValid())
	{
		if (DesktopSlateWidget.IsValid())
		{
			DesktopSlateWidget->ShowHomePanel();
			ServerBrowserWidget.Reset();
		}
	}
#endif
	// Reset the progression
	CurrentProgression = NewObject<UUTProgressionStorage>(GetTransientPackage(),UUTProgressionStorage::StaticClass());

	// If we have pending login creds then try to log right back in.
	if (bPendingLoginCreds)
	{
		bPendingLoginCreds = false;
		LoginOnline(PendingLoginName, PendingLoginPassword);
		PendingLoginPassword = TEXT("");
	}
	else
	{
		ShowToast(NSLOCTEXT("UTLocalPlayer","LoggedOutMsg","You have logged out!"));
	}

}

#if !UE_SERVER

void UUTLocalPlayer::CloseAuth()
{
	if (LoginDialog.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LoginDialog.ToSharedRef());
		LoginDialog.Reset();

		// Look to see if we are in the menu level.  If we are, then open the main menu if it's not already open.
		if (GetWorld()->GetNetMode() == NM_Standalone)
		{
			AUTMenuGameMode* MenuGame = GetWorld()->GetAuthGameMode<AUTMenuGameMode>();
			if (MenuGame != nullptr)
			{
				AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerController);
				MenuGame->ShowMenu(PC);
			}
		}
	}
}

void UUTLocalPlayer::AuthDialogClosed(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
}

void UUTLocalPlayer::AttemptLogin()
{
	if (LoginDialog.IsValid())
	{
		// Look to see if we are already logged in.
		if ( IsLoggedIn() )
		{
			bPendingLoginCreds = true;
			PendingLoginName = LoginDialog->GetEpicID();
			PendingLoginPassword = LoginDialog->GetPassword();

			// If we are in an active session, warn that this will cause you to go back to the main menu.
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
			if (UserId.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UserId))
			{
				ShowMessage(NSLOCTEXT("UTLocalPlayer", "SwitchLoginsTitle", "Change Users..."), NSLOCTEXT("UTLocalPlayer", "SwitchLoginsMsg", "Switching users will cause you to return to the main menu and leave any game you are currently in.  Are you sure you wish to do this?"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::OnSwitchUserResult),FVector2D(0.25,0.25));					
			}
			else
			{
				Logout();
			}
			return;
		}

		else
		{
			FString UserName = LoginDialog->GetEpicID();
			FString Password = LoginDialog->GetPassword();
			LoginOnline(UserName, Password,false);
		}
	}
}

void UUTLocalPlayer::OnSwitchUserResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		// If we are in an active session, then we have to force a return to the main menu.  If we are not in an active session (ie: setting at the main menu)
		// we can just logout/login..
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
		if (UserId.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UserId))
		{
			// kill the current menu....
			HideMenu();
			ReturnToMainMenu();	
		}
		else
		{
			Logout();
		}
	}
	else
	{
		bPendingLoginCreds = false;
		PendingLoginPassword = TEXT("");
	}
}

#endif

FDelegateHandle UUTLocalPlayer::RegisterPlayerOnlineStatusChangedDelegate(const FPlayerOnlineStatusChanged::FDelegate& NewDelegate)
{
	return PlayerOnlineStatusChanged.Add(NewDelegate);
}

void UUTLocalPlayer::RemovePlayerOnlineStatusChangedDelegate(FDelegateHandle DelegateHandle)
{
	PlayerOnlineStatusChanged.Remove(DelegateHandle);
}

FDelegateHandle UUTLocalPlayer::RegisterChatArchiveChangedDelegate(const FChatArchiveChanged::FDelegate& NewDelegate)
{
	return ChatArchiveChanged.Add(NewDelegate);
}

void UUTLocalPlayer::RemoveChatArchiveChangedDelegate(FDelegateHandle DelegateHandle)
{
	ChatArchiveChanged.Remove(DelegateHandle);
}


void UUTLocalPlayer::ShowAdminMessage(FString Message)
{
#if !UE_SERVER
	FText FinalMessage = FText::Format(NSLOCTEXT("UTLocalPlayer","AdminMessageFormat","Admin: {0}"), FText::FromString(Message));
	ShowToast(FinalMessage, 6);
#endif

}

void UUTLocalPlayer::ShowToast(FText ToastText, float Lifetime, bool Stack)
{
#if !UE_SERVER

	if ((GetWorld() == nullptr) || (GetWorld()->GetNetMode() == ENetMode::NM_Client && bSuppressToastsInGame)) return;

	// Look at the toast stack and see if it already exists on it.

	bool bFound = false;
	for (int32 i=0; i < ToastStack.Num(); i++)
	{
		UUTUMGWidget_Toast* Toast = Cast<UUTUMGWidget_Toast>(ToastStack[i]);
		if (Toast && Toast->Message.ToString().Equals(ToastText.ToString(), ESearchCase::IgnoreCase))
		{
			bFound = true;
			break;
		}
	}

	if (Stack || bFound == false)
	{
		// Build the Toast to Show...
		UUTUMGWidget_Toast* Toast = Cast<UUTUMGWidget_Toast>(OpenUMGWidget(TEXT("/Game/RestrictedAssets/UI/UMGMenuElements/UTGenericToastWidget.UTGenericToastWidget")));
		if (Toast != nullptr)
		{
			Toast->Message = ToastText;
			Toast->Duration = Lifetime;
			ToastStack.Add(Toast);
		}
	}
#endif
}

void UUTLocalPlayer::ToastCompleted(UUTUMGWidget_Toast* Toast)
{
#if !UE_SERVER
	ToastStack.Remove(Toast);
#endif
}


FString UUTLocalPlayer::GetProfileFilename()
{
	return TEXT("user_profile_2");
}

FString UUTLocalPlayer::GetProgressionFilename()
{
	return TEXT("user_progression_1");
}


/*
 *	If the player is currently logged in, trigger a load of their profile settings from the MCP.  
 */
void UUTLocalPlayer::LoadProfileSettings()
{
	if (GetWorld()->IsPlayInEditor())
	{
		return;
	}

	if ( IsLoggedIn() )
	{
		LoginPhase = ELoginPhase::GettingProfile;

		TSharedPtr<const FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserID.IsValid())
		{
			if (OnlineUserCloudInterface.IsValid())
			{
				bIsPendingProfileLoadFromMCP = true;

				OnReadProfileCompleteDelegate = OnlineUserCloudInterface->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnReadProfileComplete));
				OnlineUserCloudInterface->ReadUserFile(*UserID, GetProfileFilename());
			}
		}
	}
}

void UUTLocalPlayer::ClearProfileSettings()
{
#if !UE_SERVER
	if (IsLoggedIn())
	{
		ShowMessage(NSLOCTEXT("UUTLocalPlayer","ClearCloudWarnTitle","!!! WARNING !!!"), NSLOCTEXT("UUTLocalPlayer","ClearCloudWarnMessage","You are about to clear all of your settings and profile data in the cloud as well as clear your active game and input ini files locally. The game will then shut down.\n\nAre you sure you want to do this??"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ClearProfileWarnResults));
	}
#endif
}

void UUTLocalPlayer::ClearProfileWarnResults(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (IsLoggedIn() && ButtonID == UTDIALOG_BUTTON_YES)
	{
		TSharedPtr<const FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (OnlineUserCloudInterface.IsValid() && UserID.IsValid())
		{
			OnlineUserCloudInterface->DeleteUserFile(*UserID, GetProfileFilename(), true, true);
			FString Path = FPaths::GameSavedDir() + GetProfileFilename() + TEXT(".local");
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
		}
	}
}

void UUTLocalPlayer::LoadProgression()
{
	UE_LOG(UT, Verbose, TEXT("LoadProgression() %d"), IsLoggedIn() ? 1 : 0);

	if (IsLoggedIn())
	{
		LoginPhase = ELoginPhase::GettingProgression;

		TSharedPtr<const FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserID.IsValid())
		{
			if (OnlineUserCloudInterface.IsValid())
			{
				bIsPendingProgressionLoadFromMCP = true;
				OnReadProgressionCompleteDelegate = OnlineUserCloudInterface->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnReadProgressionComplete));
				OnlineUserCloudInterface->ReadUserFile(*UserID, GetProgressionFilename());
			}
		}
	}
}

void UUTLocalPlayer::OnDeleteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
#if !UE_SERVER
	// We successfully cleared the cloud, rewrite everything
	if (bWasSuccessful && FileName == GetProfileFilename())
	{
		FString PlaformName = FPlatformProperties::PlatformName();
		FString Filename = FString::Printf(TEXT("%s%s/Input.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete Input.ini"));
		}

		Filename = FString::Printf(TEXT("%s%s/Game.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete Game.ini"));
		}

		Filename = FString::Printf(TEXT("%s%s/GameUserSettings.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete GameUserSettings.ini"));
		}


		FPlatformMisc::RequestExit( 0 );
	}
#endif
}

UUTProfileSettings* UUTLocalPlayer::CreateProfileSettingsObject(const TArray<uint8>& Buffer)
{
	UUTProfileSettings* ProfileSettings = NewObject<UUTProfileSettings>(GetTransientPackage(),UUTProfileSettings::StaticClass());

	// Serialize the object
	FMemoryReader MemoryReader(Buffer, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
			
	// FObjectAndNameAsStringProxyArchive does not have versioning, but we need it
	// In 4.12, Serialization has been modified and we need the FArchive to use the right serialization path
	uint32 PossibleMagicNumber;
	Ar << PossibleMagicNumber;
	if (CloudProfileMagicNumberVersion1 == PossibleMagicNumber)
	{
		int32 CloudProfileUE4Ver;
		Ar << CloudProfileUE4Ver;
		Ar.SetUE4Ver(CloudProfileUE4Ver);
	}
	else
	{
		// Very likely this is from an unversioned cloud profile, set it to the last released serialization version
		Ar.SetUE4Ver(CloudProfileUE4VerForUnversionedProfile);
		// Rewind to the beginning as the magic number was not in the archive
		Ar.Seek(0);
	}

	ProfileSettings->Serialize(Ar);
	return ProfileSettings;
}


//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName CloudProfileLoaded
*
* @Trigger Sent when a player's profile is loaded from MCP
*
* @Type Sent by the Client
*
* @EventParam PlayerID Unique ID of the player
* @EventParam HatPath string hat used
* @EventParam LeaderHatPath string leader hat used
* @EventParam LocalXP int64 Profile saved amount of XP
*
* @Comments
*/
void UUTLocalPlayer::OnReadProfileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	OnlineUserCloudInterface->ClearOnReadUserFileCompleteDelegate_Handle(OnReadProfileCompleteDelegate);

	if (FileName == GetProfileFilename())
	{
		bIsPendingProfileLoadFromMCP = false;

		// We were attempting to read the profile.. see if it was successful.	

		if (bWasSuccessful && OnlineUserCloudInterface.IsValid())	
		{
			// Next Step is to read the MMR
			ReadMMRFromBackend();

			TArray<uint8> FileContents;
			OnlineUserCloudInterface->GetFileContents(InUserId, FileName, FileContents);

			CurrentProfileSettings = CreateProfileSettingsObject(FileContents);
			bool bNeedToSaveProfile = CurrentProfileSettings->VersionFixup();

			if (FUTAnalytics::IsAvailable())
			{
				TArray<FAnalyticsEventAttribute> ParamArray;
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("HatPath"), CurrentProfileSettings->HatPath));
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("LeaderHatPath"), CurrentProfileSettings->LeaderHatPath));
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("LocalXP"), CurrentProfileSettings->LocalXP));
				FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerController), ParamArray, false);

				FUTAnalytics::GetProvider().RecordEvent(TEXT("CloudProfileLoaded"), ParamArray);
				
				FUTAnalytics::FireEvent_UTGraphicsSettings(Cast<AUTPlayerController>(PlayerController));
			}

			FString CmdLineSwitch = TEXT("");
			bool bClearProfile = FParse::Param(FCommandLine::Get(), TEXT("ClearProfile"));

			// Check to make sure the profile settings are valid and that we aren't forcing them
			// to be cleared.  If all is OK, then apply these settings.
			if (CurrentProfileSettings->SettingsRevisionNum >= VALID_PROFILESETTINGS_VERSION && !bClearProfile)
			{
				CurrentProfileSettings->ApplyAllSettings(this);
				SaveLocalProfileSettings();
				// It's possible for the MCP data to get here before the profile, so we have to check for daily challenges 
				// in two places.
				UUTGameEngine* GameEngine = Cast<UUTGameEngine>(GEngine);
				if (GameEngine && GameEngine->GetChallengeManager().IsValid())
				{
					if ( GameEngine->GetChallengeManager()->CheckDailyChallenge(CurrentProgression) )
					{
						bNeedToSaveProfile = true;
						SaveProgression();
					}
				}

				if (bNeedToSaveProfile)
				{
					SaveProfileSettings();
				}

				// If there's no matchmaking region, run a QoS eval
				if (CurrentProfileSettings->MatchmakingRegion.IsEmpty())
				{
					FQosModule::Get().GetQosInterface()->BeginQosEvaluation(GetWorld(), FUTAnalytics::GetProviderPtr(), FSimpleDelegate::CreateUObject(this, &ThisClass::QoSComplete));
				}

				return;
			}
			else
			{
				CurrentProfileSettings->ResetProfile(EProfileResetType::All);
			}

			if (bNeedToSaveProfile)
			{
				SaveProfileSettings();
			}

		}
		else 
		{
			// This player doesn't have a valid profile so assume a new player and send them through the tutorial
		//	bLaunchTutorialOnLogin = true;

			CurrentProfileSettings = NewObject<UUTProfileSettings>(GetTransientPackage(),UUTProfileSettings::StaticClass());
			CurrentProfileSettings->ResetProfile(EProfileResetType::All);
			ReadMMRFromBackend();
		}

		PlayerNickname = GetAccountDisplayName().ToString();
		SaveConfig();
		SaveProfileSettings();

		// If there's no matchmaking region, run a QoS eval
		if (CurrentProfileSettings->MatchmakingRegion.IsEmpty())
		{
			FQosModule::Get().GetQosInterface()->BeginQosEvaluation(GetWorld(), FUTAnalytics::GetProviderPtr(), FSimpleDelegate::CreateUObject(this, &ThisClass::QoSComplete));
		}
	}
}

void UUTLocalPlayer::OnReadProgressionComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	OnlineUserCloudInterface->ClearOnReadUserFileCompleteDelegate_Handle(OnReadProfileCompleteDelegate);

	if (FileName == GetProgressionFilename())
	{
		bIsPendingProgressionLoadFromMCP = false;

		UE_LOG(UT, Verbose, TEXT("Progression loaded from MCP %d"), bWasSuccessful ? 1 : 0);

		if (bWasSuccessful)
		{

			// Create the current profile.
			if (CurrentProgression == NULL)
			{
				CurrentProgression = NewObject<UUTProgressionStorage>(GetTransientPackage(),UUTProgressionStorage::StaticClass());
			}

			TArray<uint8> FileContents;
			OnlineUserCloudInterface->GetFileContents(InUserId, FileName, FileContents);
			
			// Serialize the object
			FMemoryReader MemoryReader(FileContents, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
			
			// FObjectAndNameAsStringProxyArchive does not have versioning, but we need it
			// In 4.12, Serialization has been modified and we need the FArchive to use the right serialization path
			uint32 PossibleMagicNumber;
			Ar << PossibleMagicNumber;
			if (CloudProfileMagicNumberVersion1 == PossibleMagicNumber)
			{
				int32 CloudProfileUE4Ver;
				Ar << CloudProfileUE4Ver;
				if (Ar.UE4Ver() < CloudProfileUE4Ver)
				{
#if !UE_SERVER
					ShowMessage(NSLOCTEXT("UTLocalPlayer", "ProfileTooNewTitle", "Profile Version Unsupported"), 
						        NSLOCTEXT("UTLocalPlayer", "ProfileTooNew", "Your profile is from a newer version of Unreal Tournament, we could not load your progression data."), 
								UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.4, 0.25));
#endif
					InitializeSocial();
					return;
				}

				Ar.SetUE4Ver(CloudProfileUE4Ver);
			}
			else
			{
				// Very likely this is from an unversioned cloud file, set it to the last released serialization version
				Ar.SetUE4Ver(CloudProfileUE4VerForUnversionedProfile);
				// Rewind to the beginning as the magic number was not in the archive
				Ar.Seek(0);
			}

			CurrentProgression->Serialize(Ar);
			CurrentProgression->VersionFixup();
			
			int32 CurrentTutorialMask = GetTutorialMask();
			CurrentProgression->FixupBestTimes(CurrentTutorialMask);
			if (CurrentTutorialMask != GetTutorialMask())
			{
				SetTutorialMask(CurrentTutorialMask);
			}

			UE_LOG(UT, Verbose, TEXT("Progression: Achievements %d, Stars %d"), CurrentProgression->Achievements.Num(), CurrentProgression->TotalChallengeStars);

			if (PlayerController != NULL)
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
				if (PS != NULL)
				{
					PS->TotalChallengeStars = CurrentProgression->TotalChallengeStars;
				}
			}
		}
		else if (CurrentProfileSettings)
		{
			CurrentProgression = NewObject<UUTProgressionStorage>(GetTransientPackage(),UUTProgressionStorage::StaticClass());
			SaveProgression();
		}

		if (CurrentProfileSettings)
		{
			CurrentProfileSettings->VersionFixup();
		}

		bProgressionReadFromCloud = true;
		ReportStarsToServer();

		InitializeSocial();
	
	}
}



// Only send ELO and stars to the server once all the server responses are complete
void UUTLocalPlayer::ReportStarsToServer()
{
	if (bProgressionReadFromCloud)
	{
		// Set the ranks/etc so the player card is right.
		AUTBasePlayerController* UTBasePlayer = Cast<AUTBasePlayerController>(PlayerController);
		if (UTBasePlayer) UTBasePlayer->ServerReceiveStars(GetTotalChallengeStars());
	}
}

bool UUTLocalPlayer::OwnsItemFor(const FString& Path, int32 VariantId) const
{
#if WITH_PROFILE
	if (IsLoggedIn() && GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile) != NULL)
	{
		TArray<UUTProfileItem*> ItemList;
		GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile)->GetItemsOfType<UUTProfileItem>(ItemList);
		for (UUTProfileItem* Item : ItemList)
		{
			if (Item != NULL && Item->Grants(Path, VariantId))
			{
				return true;
			}
		}
	}
#endif
	return false;
}

#if !UE_SERVER
void UUTLocalPlayer::WelcomeDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		OpenDialog(SNew(SUTPlayerSettingsDialog).PlayerOwner(this));			
	}
}
#endif

void UUTLocalPlayer::ApplyProfileSettings()
{
	if (CurrentProfileSettings != nullptr)
	{
		CurrentProfileSettings->ApplyAllSettings(this);
	}
}

void UUTLocalPlayer::SaveProfileSettings()
{
	if ( CurrentProfileSettings != NULL )
	{
		CurrentProfileSettings->PlayerName = GetNickname();
		CurrentProfileSettings->ClanName = GetClanName();
		CurrentProfileSettings->SettingsRevisionNum = CURRENT_PROFILESETTINGS_VERSION;
		CurrentProfileSettings->bNeedProfileWriteOnLevelChange = false;
		CurrentProfileSettings->SettingsSavedOn = FDateTime::UtcNow();

		if ( IsLoggedIn() )
		{
			// Build a blob of the profile contents
			TArray<uint8> FileContents;
			FMemoryWriter MemoryWriter(FileContents, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
			Ar << CloudProfileMagicNumberVersion1;
			int32 UE4Ver = Ar.UE4Ver();
			Ar << UE4Ver;

			CurrentProfileSettings->Serialize(Ar);

			if (FApp::GetCurrentTime() - LastProfileCloudWriteTime < ProfileCloudWriteCooldownTime)
			{
				GetWorld()->GetTimerManager().SetTimer(ProfileWriteTimerHandle, this, &UUTLocalPlayer::SaveProfileSettings, ProfileCloudWriteCooldownTime - (FApp::GetCurrentTime() - LastProfileCloudWriteTime), false);
			}
			else
			{
				// Save the blob to the cloud
				TSharedPtr<const FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
				if (OnlineUserCloudInterface.IsValid() && UserID.IsValid())
				{
					SavingMask |= 0x01;
					ShowSavingWidget();

					LastProfileCloudWriteTime = FApp::GetCurrentTime();
					OnlineUserCloudInterface->WriteUserFile(*UserID, GetProfileFilename(), FileContents);
				}
			}
		}

		// Update the local version as well
		SaveLocalProfileSettings();
	}


	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
	if (PC && PC->MyUTHUD)
	{
		PC->MyUTHUD->UpdateKeyMappings(true);
	}
}

void UUTLocalPlayer::SaveProgression()
{
	if ( CurrentProgression != NULL && IsLoggedIn() )
	{
		if (FApp::GetCurrentTime() - LastProgressionCloudWriteTime < ProfileCloudWriteCooldownTime)
		{
			GetWorld()->GetTimerManager().SetTimer(ProgressionWriteTimerHandle, this, &UUTLocalPlayer::SaveProgression, ProfileCloudWriteCooldownTime - (FApp::GetCurrentTime() - LastProgressionCloudWriteTime), false);
		}
		else
		{
			SavingMask |= 0x02;
			ShowSavingWidget();
			CurrentProgression->Updated();

			// Build a blob of the profile contents
			TArray<uint8> FileContents;
			FMemoryWriter MemoryWriter(FileContents, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
			Ar << CloudProfileMagicNumberVersion1;
			int32 UE4Ver = Ar.UE4Ver();
			Ar << UE4Ver;
			CurrentProgression->Serialize(Ar);

			// Save the blob to the cloud
			TSharedPtr<const FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			if (OnlineUserCloudInterface.IsValid() && UserID.IsValid())
			{
				LastProgressionCloudWriteTime = FApp::GetCurrentTime();
				OnlineUserCloudInterface->WriteUserFile(*UserID, GetProgressionFilename(), FileContents);
			}
		}
	}
}

bool UUTLocalPlayer::IsPendingMCPLoad() const
{
	return ( bIsPendingProfileLoadFromMCP || bIsPendingProgressionLoadFromMCP );
}

void UUTLocalPlayer::OnWriteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	// Make sure this was our filename
	if (FileName == GetProfileFilename())
	{
		SavingMask = SavingMask & 0xFE;
		if (bWasSuccessful)
		{
			LastProfileCloudWriteTime = FApp::GetCurrentTime();
			FText Saved = NSLOCTEXT("MCP", "ProfileSaved", "Profile Saved");
			//ShowToast(Saved);
		}
		else
		{
			LastProfileCloudWriteTime = GetClass()->GetDefaultObject<UUTLocalPlayer>()->LastProfileCloudWriteTime;
	#if !UE_SERVER
			// Should give a warning here if it fails.
			ShowToast(NSLOCTEXT("MCPMessages", "ProfileSaveErrorText", "UT could not save your profile with the MCP.  Your settings may be lost."), 3.0f);
	#endif
		}
	}
	else if (FileName == GetProgressionFilename())
	{
		SavingMask = SavingMask & 0xFD;
		if (bWasSuccessful)
		{
			FText Saved = NSLOCTEXT("MCP", "ProgressionSaved", "Progression Saved");
			//ShowToast(Saved);
		}
		else
		{
	#if !UE_SERVER
			// Should give a warning here if it fails.
			ShowToast(NSLOCTEXT("MCPMessages", "ProgressionSaveErrorText", "UT could not save your progression with the MCP.  Your progress may be lost."), 3.0f);
	#endif
		}
	}

	HideSavingWidget();
}

void UUTLocalPlayer::SetNickname(FString NewName)
{
	GetNickname();

	// safety in case offline and no name set FIXMESTEVE make sure needed.
	if (PlayerNickname.IsEmpty() && !NewName.IsEmpty())
	{
		PlayerNickname = NewName;
		if (PlayerController)
		{
			PlayerController->ServerChangeName(NewName);
		}
		CurrentProfileSettings->PlayerName = PlayerNickname;
	}
}

void UUTLocalPlayer::SaveChat(FName Type, FString Sender, FString Message, FLinearColor Color, bool bMyChat, uint8 TeamNum)
{
	TSharedPtr<FStoredChatMessage> ArchiveMessage = FStoredChatMessage::Make(Type, Sender, Message, Color, int32(GetWorld()->GetRealTimeSeconds()), bMyChat, TeamNum );
	ChatArchive.Add( ArchiveMessage );
	ChatArchiveChanged.Broadcast(this, ArchiveMessage );
}

FName UUTLocalPlayer::TeamStyleRef(FName InName)
{
	if (PlayerController)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
		if (PC && PC->GetTeamNum() == 0)
		{
			return FName( *(TEXT("Red.") + InName.ToString()));
		}
	}

	return FName( *(TEXT("Blue.") + InName.ToString()));
}

void UUTLocalPlayer::ReadSpecificELOFromBackend(const FString& MatchRatingType)
{
	if (!IsLoggedIn())
	{
		return;
	}

	// get MCP Utils
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), OnlineIdentityInterface->GetUniquePlayerId(GetControllerId()));
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to read ELO from MCP"));
		return;
	}

	McpUtils->GetAccountMmr(MatchRatingType, [this, MatchRatingType](const FOnlineError& Result, const FAccountMmr& Response)
	{
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
			UE_LOG(UT, Warning, TEXT("Failed to read ELO from the server. (%d) %s %s"), Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			UE_LOG(UT, Display, TEXT("%s ELO read %d, %d matches"), *MatchRatingType, Response.Rating, Response.NumGamesPlayed);

			int32 OldRating = 0;
			int32 NewRating = 0;
			
			bool bRankedSession = false;

			FMMREntry NewMMREntry;
			FMMREntry OldMMREntry;
			GetMMREntry(MatchRatingType, OldMMREntry);
			OldRating = OldMMREntry.MMR;
			NewRating = Response.Rating;

			NewMMREntry.MMR = Response.Rating;
			NewMMREntry.MatchesPlayed = Response.NumGamesPlayed;
			UpdateMMREntry(MatchRatingType, NewMMREntry);

			if (MatchRatingType == NAME_RankedDuelSkillRating.ToString())
			{
				bRankedSession = true;
			}
			else if (MatchRatingType == NAME_RankedCTFSkillRating.ToString())
			{
				bRankedSession = true;
			}
			else if (MatchRatingType == NAME_RankedShowdownSkillRating.ToString())
			{
				bRankedSession = true;
			}
			else if (MatchRatingType == NAME_RankedFlagRunSkillRating.ToString())
			{
				bRankedSession = true;
			}

			int32 OldLevel = 0;
			int32 OldBadge = 0;
			int32 NewLevel = 0;
			int32 NewBadge = 0;

			AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
			if (PC)
			{
				AUTGameState* UTGameState = PC->GetWorld()->GetGameState<AUTGameState>();
				if (UTGameState)
				{
					AUTGameMode* DefaultGame = UTGameState && UTGameState->GameModeClass ? UTGameState->GameModeClass->GetDefaultObject<AUTGameMode>() : NULL;
					if (DefaultGame && PC->UTPlayerState)
					{
						DefaultGame->SetEloFor(PC->UTPlayerState, bRankedSession, OldRating, false);
						PC->UTPlayerState->GetBadgeFromELO(DefaultGame, bRankedSession, OldBadge, OldLevel);
						DefaultGame->SetEloFor(PC->UTPlayerState, bRankedSession, NewRating, true);
						PC->UTPlayerState->GetBadgeFromELO(DefaultGame, bRankedSession, NewBadge, NewLevel);
						PC->bBadgeChanged = ((OldLevel != NewLevel) || (OldBadge != NewBadge));
					}
				}
			}
		}

	});
	
	// Refresh ranked league if we played ranked
	if (MatchRatingType == NAME_RankedDuelSkillRating.ToString() ||
		MatchRatingType == NAME_RankedCTFSkillRating.ToString() ||
		MatchRatingType == NAME_RankedFlagRunSkillRating.ToString() ||
		MatchRatingType == NAME_RankedShowdownSkillRating.ToString())
	{
		McpUtils->GetAccountLeague(MatchRatingType, [this, MatchRatingType](const FOnlineError& Result, const FAccountLeague& Response)
		{
			if (!Result.bSucceeded)
			{
				// best we can do is log an error
				UE_LOG(UT, Warning, TEXT("Failed to read League info from the server. (%d) %s %s"), Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
			}
			else
			{
				if (Response.PlacementMatchesAttempted < 10)
				{
					UE_LOG(UT, Display, TEXT("%s league read placement matches: %d"), *MatchRatingType, Response.PlacementMatchesAttempted);
				}
				else
				{
					UE_LOG(UT, Display, TEXT("%s league read tier:%d, division:%d, points:%d"), *MatchRatingType, Response.Tier, Response.Division + 1, Response.Points);
				}
#if !UE_SERVER
				FRankedLeagueProgress LeagueProgress;
				GetLeagueProgress(MatchRatingType, LeagueProgress);
				if (LeagueProgress.LeaguePlacementMatches < 10 && Response.PlacementMatchesAttempted == 10)
				{ 
					// Report your placement!

					FString TierString;
					switch (Response.Tier)
					{
					default:
					case 0:
						TierString = TEXT("Bronze");
						break;
					case 1:
						TierString = TEXT("Silver");
						break;
					case 2:
						TierString = TEXT("Gold");
						break;
					case 3:
						TierString = TEXT("Platinum");
						break;
					case 4:
						TierString = TEXT("Master");
						break;
					}

					if (Response.Tier < 4)
					{
						FText PlacementText = FText::Format(NSLOCTEXT("UTLocalPlayer", "ShowdownPlacement", "You've been placed in {0} {1}."), FText::FromString(TierString), FText::AsNumber(Response.Division + 1));
						LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
							NSLOCTEXT("UTLocalPlayer", "ShowdownPlacementTitle", "You've Been Placed!"),
							PlacementText,
							UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
					}
					else
					{
						LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
							NSLOCTEXT("UTLocalPlayer", "ShowdownPlacementTitle", "You've Been Placed!"),
							NSLOCTEXT("UTLocalPlayer", "ShowdownPlacementMasterTier", "You've been placed in Master Tier!"),
							UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
					}
				}
				else if (Response.PlacementMatchesAttempted < 10)
				{
					// Finished a placement match, encourage the player to keep playing ranked

					FText PlacementText;
					if (Response.PlacementMatchesAttempted < 9)
					{
						PlacementText = FText::Format(NSLOCTEXT("UTLocalPlayer", "ShowdownPlacementPlural", "Only {0} more matches until you are placed."), FText::AsNumber(10 - Response.PlacementMatchesAttempted));
					}
					else
					{
						PlacementText = NSLOCTEXT("UTLocalPlayer", "ShowdownPlacementSingular", "Only one match until you are placed!");
					}
					LeagueMatchResultsDialog = ShowMessage(NSLOCTEXT("UTLocalPlayer", "ShowdownMorePlacementsTitle", "You're Almost There!"),
						PlacementText,
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.4, 0.25));
				}
				else if (!LeagueProgress.bLeaguePromotionSeries && Response.IsInPromotionSeries)
				{
					FText PromoSeriesText;
					if (Response.Division == 4)
					{
						PromoSeriesText = NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesTier", "You've reached a Promotion Series! To reach the next tier, win the next 3 out of 5 games.");
					}
					else
					{
						PromoSeriesText = NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesDivision", "You've reached a Promotion Series! To reach the next division, win the next 2 out of 3 games.");
					}

					// Report that we're in a promo series
					LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
						NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesTitle", "Promotion Series!"),
						PromoSeriesText,
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
				}
				else if (LeagueProgress.bLeaguePromotionSeries && !Response.IsInPromotionSeries)
				{
					// Report if we got promoted or failed the series
					if (Response.Division == LeagueProgress.LeagueDivision && Response.Tier == LeagueProgress.LeagueTier)
					{
						LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
							NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesFailedTitle", "Better Luck Next Time!"),
							NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesFailed", "You didn't get promoted! Don't be discouraged, you'll do better next time!"),
							UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
					}
					else
					{
						LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
							NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesSuccessTitle", "Great Job!"),
							NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesSuccess", "All your hard work has paid off! We've promoted you! Keep up the great work!"),
							UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
					}
				}
				else if (Response.IsInPromotionSeries)
				{
					// Report how many matches still needed to win the promo series
					FText PromoSeriesText;
					int32 WinsLeft = 0;
					if (Response.Division == 4)
					{
						WinsLeft = 5 - Response.PromotionMatchesWon;
					}
					else
					{
						WinsLeft = 3 - Response.PromotionMatchesWon;
					}

					if (WinsLeft > 1)
					{
						PromoSeriesText = FText::Format(NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesProgressPlural", "Only {0} more wins until you are promoted!"), FText::AsNumber(WinsLeft));
					}
					else
					{
						PromoSeriesText = NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesProgressSingular", "You only need 1 more win for a promotion!");
					}

					LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
						NSLOCTEXT("UTLocalPlayer", "ShowdownPromoSeriesProgressTitle", "Promotion Series Progress"),
						PromoSeriesText,
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
				}
				else if (Response.Tier < LeagueProgress.LeagueTier || (LeagueProgress.LeagueTier == Response.Tier && Response.Division < LeagueProgress.LeagueDivision))
				{
					// Report a demotion
					LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
						NSLOCTEXT("UTLocalPlayer", "ShowdownDemotionTitle", "Demoted"),
						NSLOCTEXT("UTLocalPlayer", "ShowdownDemotion", "We're sorry about the demotion, hopefully players in this lower bracket are more your speed!"),
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
				}
				else if (Response.Points > LeagueProgress.LeaguePoints)
				{
					FText WinText = FText::Format(NSLOCTEXT("UTLocalPlayer", "ShowdownWin", "Great Win! You earned {0} league points! You now have {1} league points!"), FText::AsNumber(Response.Points - LeagueProgress.LeaguePoints), FText::AsNumber(Response.Points));

					// Report a regular win
					LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
						NSLOCTEXT("UTLocalPlayer", "ShowdownWinTitle", "You Won!"),
						WinText,
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
				}
				else
				{
					// Report a regular loss
					FText LossText = FText::Format(NSLOCTEXT("UTLocalPlayer", "ShowdownLoss", "That was a rough loss, you lost {0} league points. At least you still have {1} league points!"), FText::AsNumber(LeagueProgress.LeaguePoints - Response.Points), FText::AsNumber(Response.Points));

					LeagueMatchResultsDialog = ShowLeagueMatchResultDialog(Response.Tier, Response.Division,
						NSLOCTEXT("UTLocalPlayer", "ShowdowLossTitle", "Tough Luck!"),
						LossText,
						UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.5, 0.4));
				}
#endif
				FRankedLeagueProgress NewLeagueProgress;
				NewLeagueProgress.LeaguePlacementMatches = Response.PlacementMatchesAttempted;
				NewLeagueProgress.LeaguePoints = Response.Points;
				NewLeagueProgress.LeagueTier = Response.Tier;
				NewLeagueProgress.LeagueDivision = Response.Division;
				NewLeagueProgress.LeaguePromotionMatchesAttempted = Response.PromotionMatchesAttempted;
				NewLeagueProgress.LeaguePromotionMatchesWon = Response.PromotionMatchesWon;
				NewLeagueProgress.bLeaguePromotionSeries = Response.IsInPromotionSeries;
				UpdateLeagueProgress(MatchRatingType, NewLeagueProgress);
			}
		});
	}
}

#if !UE_SERVER
TSharedPtr<class SUTDialogBase> UUTLocalPlayer::ShowLeagueMatchResultDialog(int32 Tier, int32 Division, FText MessageTitle, FText MessageText, uint16 Buttons, const FDialogResultDelegate& Callback, FVector2D DialogSize)
{
	TSharedPtr<class SUTDialogBase> NewDialog;
	if (DialogSize.IsNearlyZero())
	{
		SAssignNew(NewDialog, SUTLeagueMatchResultsDialog)
			.PlayerOwner(this)
			.Tier(Tier)
			.Division(Division)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}
	else
	{
		SAssignNew(NewDialog, SUTLeagueMatchResultsDialog)
			.PlayerOwner(this)
			.Tier(Tier)
			.Division(Division)
			.bDialogSizeIsRelative(true)
			.DialogSize(DialogSize)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}

	OpenDialog(NewDialog.ToSharedRef());
	return NewDialog;
}
#endif

void UUTLocalPlayer::ReadMMRFromBackend()
{

	LoginPhase = ELoginPhase::GettingMMR;

	// get MCP Utils
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), OnlineIdentityInterface->GetUniquePlayerId(GetControllerId()));
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to read MMR from MCP"));
		return;
	}

	TArray<FString> MatchRatingTypes;
	MatchRatingTypes.Add(TEXT("SkillRating"));
	MatchRatingTypes.Add(TEXT("TDMSkillRating"));
	MatchRatingTypes.Add(TEXT("DMSkillRating"));
	MatchRatingTypes.Add(TEXT("CTFSkillRating"));
	MatchRatingTypes.Add(TEXT("ShowdownSkillRating"));
	MatchRatingTypes.Add(TEXT("FlagRunSkillRating"));
	MatchRatingTypes.Add(TEXT("RankedDuelSkillRating"));
	MatchRatingTypes.Add(TEXT("RankedCTFSkillRating"));
	MatchRatingTypes.Add(TEXT("RankedShowdownSkillRating"));
	MatchRatingTypes.Add(TEXT("RankedFlagRunSkillRating"));
	// This should be a weak ptr here, but UTLocalPlayer is unlikely to go away
	TWeakObjectPtr<UUTLocalPlayer> WeakLocalPlayer(this);
	McpUtils->GetBulkAccountMmr(MatchRatingTypes, [WeakLocalPlayer](const FOnlineError& Result, const FBulkAccountMmr& Response)
	{
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
			UE_LOG(UT, Warning, TEXT("Failed to read MMR from the server. (%d) %s %s"), Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			if (!WeakLocalPlayer.IsValid())
			{
				return;
			}

			for (int i = 0; i < Response.RatingTypes.Num(); i++)
			{
				UE_LOG(UT, Display, TEXT("%s MMR read %d, %d matches"), *Response.RatingTypes[i], Response.Ratings[i], Response.NumGamesPlayed[i]);

				bool bRankedSession = false;
				FMMREntry MMREntry;
				MMREntry.MMR = Response.Ratings[i];
				MMREntry.MatchesPlayed = Response.NumGamesPlayed[i];
				WeakLocalPlayer->UpdateMMREntry(Response.RatingTypes[i], MMREntry);
			}

			// We're in the menus, just fill out the player state for the player card right now
			if (WeakLocalPlayer->IsMenuGame() && WeakLocalPlayer->PlayerController)
			{
				AUTPlayerState* UTPS = Cast<AUTPlayerState>(WeakLocalPlayer->PlayerController->PlayerState);
				if (UTPS)
				{

					FMMREntry DuelMMR;
					FMMREntry CTFMMR;
					FMMREntry TDMMMR;
					FMMREntry DMMMR;
					FMMREntry ShowdownMMR;
					FMMREntry FlagRunMMR;
					FMMREntry RankedShowdownMMR;
					FMMREntry RankedFlagRunMMR;
					FMMREntry RankedCTFMMR;
					FMMREntry RankedDuelMMR;

					WeakLocalPlayer->GetMMREntry(NAME_SkillRating.ToString(), DuelMMR);
					WeakLocalPlayer->GetMMREntry(NAME_CTFSkillRating.ToString(), CTFMMR);
					WeakLocalPlayer->GetMMREntry(NAME_TDMSkillRating.ToString(), TDMMMR);
					WeakLocalPlayer->GetMMREntry(NAME_DMSkillRating.ToString(), DMMMR);
					WeakLocalPlayer->GetMMREntry(NAME_ShowdownSkillRating.ToString(), ShowdownMMR);
					WeakLocalPlayer->GetMMREntry(NAME_FlagRunSkillRating.ToString(), FlagRunMMR);
					WeakLocalPlayer->GetMMREntry(NAME_RankedShowdownSkillRating.ToString(), RankedShowdownMMR);
					WeakLocalPlayer->GetMMREntry(NAME_RankedFlagRunSkillRating.ToString(), RankedFlagRunMMR);
					WeakLocalPlayer->GetMMREntry(NAME_RankedCTFSkillRating.ToString(), RankedCTFMMR);
					WeakLocalPlayer->GetMMREntry(NAME_RankedDuelSkillRating.ToString(), RankedDuelMMR);

					UTPS->DuelRank = DuelMMR.MMR;
					UTPS->TDMRank = TDMMMR.MMR;
					UTPS->DMRank = DMMMR.MMR;
					UTPS->CTFRank = CTFMMR.MMR;
					UTPS->ShowdownRank = ShowdownMMR.MMR;
					UTPS->FlagRunRank = FlagRunMMR.MMR;
					UTPS->RankedShowdownRank = RankedShowdownMMR.MMR;
					UTPS->RankedFlagRunRank = RankedFlagRunMMR.MMR;
					UTPS->RankedCTFRank = RankedCTFMMR.MMR;
					UTPS->RankedDuelRank = RankedDuelMMR.MMR;

					UTPS->DuelMatchesPlayed = FMath::Min(255, DuelMMR.MatchesPlayed);
					UTPS->TDMMatchesPlayed = FMath::Min(255, TDMMMR.MatchesPlayed);
					UTPS->DMMatchesPlayed = FMath::Min(255, DMMMR.MatchesPlayed);
					UTPS->CTFMatchesPlayed = FMath::Min(255, CTFMMR.MatchesPlayed);
					UTPS->ShowdownMatchesPlayed = FMath::Min(255, ShowdownMMR.MatchesPlayed);
					UTPS->FlagRunMatchesPlayed = FMath::Min(255, FlagRunMMR.MatchesPlayed);
					UTPS->RankedShowdownMatchesPlayed = FMath::Min(255, RankedShowdownMMR.MatchesPlayed);
					UTPS->RankedFlagRunMatchesPlayed = FMath::Min(255, RankedFlagRunMMR.MatchesPlayed);
					UTPS->RankedCTFMatchesPlayed = FMath::Min(255, RankedCTFMMR.MatchesPlayed);
					UTPS->RankedDuelMatchesPlayed = FMath::Min(255, RankedDuelMMR.MatchesPlayed);
				}
			}
		}

		// This will be bad if the local player goes away here.
		if (WeakLocalPlayer.IsValid()) 
		{
			WeakLocalPlayer->EpicFlagCheck();
			WeakLocalPlayer->LoadProgression();
		}

	});

	ReadLeagueFromBackend(NAME_RankedFlagRunSkillRating.ToString());
	ReadLeagueFromBackend(NAME_RankedShowdownSkillRating.ToString());
	ReadLeagueFromBackend(NAME_RankedCTFSkillRating.ToString());
	ReadLeagueFromBackend(NAME_RankedDuelSkillRating.ToString());
}

void UUTLocalPlayer::ReadLeagueFromBackend(const FString& MatchRatingType)
{
	// get MCP Utils
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), OnlineIdentityInterface->GetUniquePlayerId(GetControllerId()));
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to read league data from MCP"));
		return;
	}

	McpUtils->GetAccountLeague(MatchRatingType, [this, MatchRatingType](const FOnlineError& Result, const FAccountLeague& Response)
	{
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
			UE_LOG(UT, Warning, TEXT("Failed to read %s League info from the server. (%d) %s %s"), *MatchRatingType, Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			if (Response.PlacementMatchesAttempted < 10)
			{
				UE_LOG(UT, Display, TEXT("%s league read placement matches: %d"), *MatchRatingType, Response.PlacementMatchesAttempted);
			}
			else
			{
				UE_LOG(UT, Display, TEXT("%s league read tier:%d, division:%d, points:%d"), *MatchRatingType, Response.Tier, Response.Division, Response.Points);
			}

			FRankedLeagueProgress NewLeagueProgress;
			NewLeagueProgress.LeagueTier = Response.Tier;
			NewLeagueProgress.LeagueDivision = Response.Division;
			NewLeagueProgress.LeaguePlacementMatches = Response.PlacementMatchesAttempted;
			NewLeagueProgress.LeaguePoints = Response.Points;
			NewLeagueProgress.bLeaguePromotionSeries = Response.IsInPromotionSeries;
			NewLeagueProgress.LeaguePromotionMatchesAttempted = Response.PromotionMatchesAttempted;
			NewLeagueProgress.LeaguePromotionMatchesWon = Response.PromotionMatchesWon;
			UpdateLeagueProgress(MatchRatingType, NewLeagueProgress);
		}
	});
}

int32 UUTLocalPlayer::GetBaseELORank()
{
	// let UTGame do it if have PlayerState
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
	if (PC && PC->UTPlayerState)
	{
		AUTGameMode* UTGame = AUTGameMode::StaticClass()->GetDefaultObject<AUTGameMode>();
		if (UTGame)
		{
			return UTGame->GetEloFor(PC->UTPlayerState, false);
		}
	}
	return 1500;
}

void UUTLocalPlayer::GetStarsFromXP(int32 XPValue, int32& Star)
{
	Star = (XPValue > 0) ? int32(FMath::Clamp<float>((XPValue / 10.f), 0.f, 5.f)) : -1;
}

int32 UUTLocalPlayer::GetHatVariant() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->HatVariant : FCString::Atoi(*GetDefaultURLOption(TEXT("HatVar")));
}

void UUTLocalPlayer::SetHatVariant(int32 NewVariant)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->HatVariant = NewVariant;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveHatVariant(NewVariant);
		}
	}
}

int32 UUTLocalPlayer::GetEyewearVariant() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->EyewearVariant : FCString::Atoi(*GetDefaultURLOption(TEXT("EyewearVar")));
}

void UUTLocalPlayer::SetEyewearVariant(int32 NewVariant)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->EyewearVariant = NewVariant;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveEyewearVariant(NewVariant);
		}
	}
}

FString UUTLocalPlayer::GetHatPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->HatPath : GetDefaultURLOption(TEXT("Hat"));
}

void UUTLocalPlayer::SetHatPath(const FString& NewHatPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->HatPath = NewHatPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveHatClass(NewHatPath);
		}
	}
}

FString UUTLocalPlayer::GetLeaderHatPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->LeaderHatPath : GetDefaultURLOption(TEXT("LeaderHat"));
}

void UUTLocalPlayer::SetLeaderHatPath(const FString& NewLeaderHatPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->LeaderHatPath = NewLeaderHatPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveLeaderHatClass(NewLeaderHatPath);
		}
	}
}

FString UUTLocalPlayer::GetEyewearPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->EyewearPath : GetDefaultURLOption(TEXT("Eyewear"));
}


void UUTLocalPlayer::SetEyewearPath(const FString& NewEyewearPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->EyewearPath = NewEyewearPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveEyewearClass(NewEyewearPath);
		}
	}
}

FString UUTLocalPlayer::GetCharacterPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->CharacterPath : GetDefaultURLOption(TEXT("Character"));
}

void UUTLocalPlayer::SetCharacterPath(const FString& NewCharacterPath)
{
	AUTPlayerState* PS = Cast<AUTPlayerState>((PlayerController != NULL) ? PlayerController->PlayerState : NULL);
	SetDefaultURLOption(TEXT("Character"), NewCharacterPath);
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->CharacterPath = NewCharacterPath;
	}

	if (PS != NULL)
	{
		PS->ServerSetCharacter(NewCharacterPath);
	}
}

FString UUTLocalPlayer::GetGroupTauntPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->GroupTauntPath : GetDefaultURLOption(TEXT("GroupTaunt"));
}

void UUTLocalPlayer::SetGroupTauntPath(const FString& NewGroupTauntPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->GroupTauntPath = NewGroupTauntPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveGroupTauntClass(NewGroupTauntPath);
		}
	}
}

FString UUTLocalPlayer::GetTauntPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->TauntPath : GetDefaultURLOption(TEXT("Taunt"));
}

void UUTLocalPlayer::SetTauntPath(const FString& NewTauntPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->TauntPath = NewTauntPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveTauntClass(NewTauntPath);
		}
	}
}

FString UUTLocalPlayer::GetTaunt2Path() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->Taunt2Path : GetDefaultURLOption(TEXT("Taunt2"));
}

void UUTLocalPlayer::SetTaunt2Path(const FString& NewTauntPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->Taunt2Path = NewTauntPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveTaunt2Class(NewTauntPath);
		}
	}
}

FString UUTLocalPlayer::GetIntroPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->IntroPath : TEXT("");
}

void UUTLocalPlayer::SetIntroPath(const FString& NewIntroPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->IntroPath = NewIntroPath;
	}

	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveIntroClass(NewIntroPath);
		}
	}
}

FString UUTLocalPlayer::GetDefaultURLOption(const TCHAR* Key) const
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	FString Op = DefaultURL.GetOption(Key, TEXT(""));
	FString Result;
	Op.Split(TEXT("="), NULL, &Result);
	return Result;
}

void UUTLocalPlayer::SetDefaultURLOption(const FString& Key, const FString& Value)
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	DefaultURL.AddOption(*FString::Printf(TEXT("%s=%s"), *Key, *Value));
	DefaultURL.SaveURLConfig(TEXT("DefaultPlayer"), *Key, GGameIni);
}

void UUTLocalPlayer::ClearDefaultURLOption(const FString& Key)
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	// doing it manually instead of RemoveOption() as the latter doesn't properly handle longer keys that have the same starting characters
	int32 KeyLen = Key.Len();
	for (int32 i = DefaultURL.Op.Num() - 1; i >= 0; i--)
	{
		if (DefaultURL.Op[i].Left(KeyLen) == Key)
		{
			const TCHAR* s = *DefaultURL.Op[i];
			if (s[KeyLen - 1] == '=' || s[KeyLen] == '=' || s[KeyLen] == '\0')
			{
				FConfigSection* Sec = GConfig->GetSectionPrivate(TEXT("DefaultPlayer"), 0, 0, GGameIni);
				if (Sec != NULL && Sec->Remove(*Key) > 0)
				{
					GConfig->Flush(0, GGameIni);
				}

				DefaultURL.Op.RemoveAt(i);
			}
		}
	}
}

#if !UE_SERVER
void UUTLocalPlayer::ShowContentLoadingMessage()
{
	if (!ContentLoadingMessage.IsValid())
	{
		SAssignNew(ContentLoadingMessage, SOverlay)
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(700)
					.HeightOverride(64)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Fill)
							.HAlign(HAlign_Fill)
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.Dialog.Background"))
							]
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("MenuMessages","InitMenu","Initializing Menus"))
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
							]
						]
					]
				]
			]
		];
	}

	if (ContentLoadingMessage.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(ContentLoadingMessage.ToSharedRef(), 255);
	}
}

void UUTLocalPlayer::HideContentLoadingMessage()
{
	if (ContentLoadingMessage.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ContentLoadingMessage.ToSharedRef());			
		ContentLoadingMessage.Reset();
	}
}

TSharedPtr<SUTFriendsPopupWindow> UUTLocalPlayer::GetFriendsPopup()
{
	if (!FriendsMenu.IsValid())
	{
		SAssignNew(FriendsMenu, SUTFriendsPopupWindow)
			.PlayerOwner(this);

		if (FriendsMenu.IsValid())
		{
			FriendsMenu->SetOnCloseClicked(FOnClicked::CreateUObject(this, &UUTLocalPlayer::ToggleFriendsAndChat));
		}
	}
	return FriendsMenu;
}

#endif

void UUTLocalPlayer::SetShowingFriendsPopup(bool bShowing)
{
	bShowingFriendsMenu = bShowing;
}

void UUTLocalPlayer::CancelQuickmatch()
{
	PendingQuickmatchType = -1;
	bQuickmatchOnLevelChange = false;

	InvalidateLastSession();
}

void UUTLocalPlayer::ReturnToMainMenu()
{
	StopKillCam();
	CancelQuickmatch();
	HideMenu();

#if !UE_SERVER
	// Under certain situations (when we fail to load a replay immediately after starting watching it), 
	//	the replay menu will show up at the last second, and nothing will close it.
	// This is to make absolutely sure the replay menu doesn't persist into the main menu
	if (ReplayWindow.IsValid())
	{
		CloseReplayWindow();
	}
#endif
	
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerController);
	if (UTPC)
	{
		UTPC->LeaveVoiceChat();

		AUTGameMode* UTGM = GetWorld()->GetAuthGameMode<AUTGameMode>();
		//Check if we were a new playing in the tutorial. If so fire correct analytic.
		if (FUTAnalytics::IsAvailable() &&
			bLaunchTutorialOnLogin &&
			!UTPC->SkipTutorialCheck() &&
			!IsTutorialCompleted(ETutorialTags::TUTTAG_NewPlayerLaunchTutorial) &&
			UTGM && UTGM->bBasicTrainingGame)
		{
			FUTAnalytics::FireEvent_UTCancelOnboarding(UTPC);
		}
	}

	if ( GetWorld() != nullptr )
	{
		if ( IsMenuGame() )
		{
#if !UE_SERVER
			ShowMenu(TEXT(""));
			if (DesktopSlateWidget.IsValid())
			{
				DesktopSlateWidget->ShowHomePanel();
				ServerBrowserWidget.Reset();

			}
#endif
		}
		else
		{
			FString URL = TEXT("ut-entry?closed");

			if (GetWorld()->GetGameState<AUTLobbyGameState>() != nullptr)
			{
				URL += TEXT("?returnfromhub");
			}
			AUTGameMode* UTGame = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if ( GetWorld()->URL.HasOption(TEXT("tutorialmask")) && (!UTGame || !UTGame->bNoTrainingMenu))
			{
				URL += TEXT("?tutorialmenu");
			}

			GetWorld()->ServerTravel(URL,true, true);
		}
	}
	else
	{
		// If we don't have a world, we likely failed to load one during replays
		// Recover by loading default map
		FURL DefaultURL;
		DefaultURL.LoadURLConfig( TEXT( "DefaultPlayer" ), GGameIni );

		FURL URL( &DefaultURL, TEXT( "" ), TRAVEL_Partial );

		FString Error;

		GEngine->Browse( *GetGameInstance()->GetWorldContext(), URL, Error );
	}
}

void UUTLocalPlayer::InvalidateLastSession()
{
	// Reset the last session so we don't have anything to join.
	LastSession.Session.OwningUserId.Reset();
	LastSession.Session.SessionInfo.Reset();
}

#if !UE_SERVER
void UUTLocalPlayer::ConnectPasswordResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID, bool bAsSpectator)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		TSharedPtr<SUTInputBoxDialog> Box = StaticCastSharedPtr<SUTInputBoxDialog>(Widget);
		if (Box.IsValid())
		{
			PendingJoinSessionPassword = Box->GetInputText();

			FString ServerGUID;
			LastSession.Session.SessionSettings.Get(SETTING_SERVERINSTANCEGUID, ServerGUID);
			CachePassword(ServerGUID, PendingJoinSessionPassword);

			JoinSession(LastSession, bAsSpectator, ConnectDesiredTeam, PendingInstanceID );
		}
	}
}
#endif

bool UUTLocalPlayer::JoinSession(const FOnlineSessionSearchResult& SearchResult, bool bSpectate, int32 DesiredTeam, FString InstanceId)
{
	UE_LOG(UT,Log, TEXT("##########################"));
	UE_LOG(UT,Log, TEXT("Joining a New Session"));
	UE_LOG(UT,Log, TEXT("##########################"));

	// If this server is passworded, prompt before hand
	int32 ServerFlags = 0x0000;
	SearchResult.Session.SessionSettings.Get(SETTING_SERVERFLAGS, ServerFlags);

	LastSession = SearchResult;
	bLastSessionWasASpectator = bSpectate;

	SearchResult.Session.SessionSettings.Get(SETTING_GAMEMODE,PendingGameMode);
	PendingInstanceID = InstanceId;
	bWantsToConnectAsSpectator = bSpectate;
	ConnectDesiredTeam = DesiredTeam;
	bCancelJoinSession = false;
	FUniqueNetIdRepl UniqueId = OnlineIdentityInterface->GetUniquePlayerId(0);

#if !UE_SERVER

	bool bServerRequiresPassword = (ServerFlags & SERVERFLAG_RequiresPassword) == SERVERFLAG_RequiresPassword;
	if ( bServerRequiresPassword && PendingJoinSessionPassword.IsEmpty())
	{
		// Attempt to look up the password
		FString ServerGUID;
		SearchResult.Session.SessionSettings.Get(SETTING_SERVERINSTANCEGUID, ServerGUID);
		FString HubGUID;
		SearchResult.Session.SessionSettings.Get(SETTING_HUBGUID, HubGUID);
		
		if (!HubGUID.IsEmpty())
		{
			PendingJoinSessionPassword = RetrievePassword(HubGUID);	
		}
		else
		{
			PendingJoinSessionPassword = RetrievePassword(ServerGUID);	
		}

		if (PendingJoinSessionPassword.IsEmpty())
		{
			OpenDialog(SNew(SUTInputBoxDialog)
				.OnDialogResult( FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ConnectPasswordResult, bSpectate))
				.PlayerOwner(this)
				.DialogTitle(NSLOCTEXT("UTGameViewportClient", "PasswordRequireTitle", "Password is Required"))
				.MessageText(NSLOCTEXT("UTGameViewportClient", "PasswordRequiredText", "This server requires a password:"))
				);
		
			return false;
		}
	}
#endif

UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		FString ServerName;
		SearchResult.Session.SessionSettings.Get(SETTING_SERVERNAME,ServerName);
		UTGameInstance->LevelLoadText = FText::Format(NSLOCTEXT("UTLocalPlayer","ConnectingText","Connecting to {0}..."), FText::FromString(ServerName));
	}	

	UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UPartyContext::StaticClass()));
	if (IsLoggedIn() && PartyContext)
	{
		int32 PartySize = PartyContext->GetPartySize();
		FUniqueNetIdRepl PartyLeaderId = PartyContext->GetPartyLeader();
		SetDefaultURLOption(TEXT("PartySize"), FString::FromInt(PartySize));
		SetDefaultURLOption(TEXT("PartyLeader"), PartyLeaderId.ToString());
	}
	
	bJoinSessionInProgress = true;

	PendingSession = SearchResult;
	if (OnlineSessionInterface->GetSessionSettings(GameSessionName) != nullptr)
	{
		UE_LOG(UT, Log, TEXT("--- Already in a Session -- Deferring while I clean it up"));
		bDelayedJoinSession = true;
		LeaveSession();
	}
	else
	{
		OnJoinSessionCompleteDelegate = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));

		SearchResult.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, CurrentSessionTrustLevel);
		bAttemptingForceJoin = false;
		bCancelJoinSession = false;
		OnlineSessionInterface->JoinSession(0, GameSessionName, SearchResult);
	}
	return true;
}

void UUTLocalPlayer::JoinPendingSession()
{
	if (bDelayedJoinSession)
	{
		bDelayedJoinSession = false;
		bAttemptingForceJoin = false;
		bCancelJoinSession = false;
		PendingSession.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, CurrentSessionTrustLevel);
		OnJoinSessionCompleteDelegate = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));
		OnlineSessionInterface->JoinSession(0, GameSessionName, PendingSession);
	}
}

void UUTLocalPlayer::CancelJoinSession()
{
	// There currently isn't a way to cancel a join session call.  So we just flag it as we are not joining and ignore any successful JoinSessionComplete calls

	bCancelJoinSession = true;
#if !UE_SERVER
	if (ServerBrowserWidget.IsValid())
	{
		ServerBrowserWidget->SetBrowserState(EBrowserState::BrowserIdle);
	}
#endif
}

void UUTLocalPlayer::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Kill any active replays.
	StopKillCam();

	bDelayedJoinSession = false;
	bJoinSessionInProgress = false;
#if !UE_SERVER
	if (ServerBrowserWidget.IsValid())
	{
		ServerBrowserWidget->SetBrowserState(EBrowserState::BrowserIdle);
	}
#endif

	UE_LOG(UT,Log, TEXT("----------- [OnJoinSessionComplete %i"), (Result == EOnJoinSessionCompleteResult::Success));

	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
	}

	// If we are trying to be crammed in to an existing session, we can just exit.
	if (bAttemptingForceJoin)
	{
		UE_LOG(UT,Verbose,TEXT("----- bAttemptingForceJoin "));
		bCancelJoinSession = false;
		bAttemptingForceJoin = false;
		return;
	}

	ChatArchive.Empty();

	// If we succeed, nothing else needs to be done.
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		if (bCancelJoinSession)
		{
			UE_LOG(UT,Verbose,TEXT("----- bCancelJoinSession "));
			InvalidateLastSession();
			bCancelJoinSession = false;
			return;
		}

		// Cache the last session.
		LastSession = PendingSession;
		bLastSessionWasASpectator = bWantsToConnectAsSpectator;

		UUTGameInstance* GameInstance = CastChecked<UUTGameInstance>(GetGameInstance());
		UUTParty* Party = GameInstance->GetParties();
		if (Party && PendingSession.IsValid())
		{
			Party->SetSession(LastSession);
		}

		FString ConnectionString;
		if ( OnlineSessionInterface->GetResolvedConnectString(SessionName, ConnectionString) )
		{
			FString Password = TEXT("");
			if (!PendingJoinSessionPassword.IsEmpty())
			{
				Password = TEXT("?password=") + PendingJoinSessionPassword;

				FString ServerGUID;
				PendingSession.Session.SessionSettings.Get(SETTING_SERVERINSTANCEGUID, ServerGUID);
				CachedPasswords.Add(ServerGUID, PendingJoinSessionPassword);
				PendingJoinSessionPassword = TEXT("");
			}

			ConnectionString += Password;

			if (PendingFriendInviteFriendId != TEXT(""))
			{
				ConnectionString += FString::Printf(TEXT("?Friend=%s"), *PendingFriendInviteFriendId);
				PendingFriendInviteFriendId = TEXT("");
			}

			int32 RankCheck = DEFAULT_RANK_CHECK;
			AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerController->PlayerState);
			if (PlayerState)
			{

				UClass* GameModeClass = LoadClass<AUTGameMode>(NULL, *PendingGameMode, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
				if (GameModeClass)
				{
					AUTBaseGameMode* BaseGameMode = GameModeClass->GetDefaultObject<AUTBaseGameMode>();
					if (BaseGameMode)
					{
						RankCheck = PlayerState->GetRankCheck(BaseGameMode);
					}
				}
			}
			ConnectionString += FString::Printf(TEXT("?RankCheck=%i"), RankCheck);
			ConnectionString += FString::Printf(TEXT("?SpectatorOnly=%i"), bWantsToConnectAsSpectator ? 1 : 0);

			if (ConnectDesiredTeam >= 0)
			{
				ConnectionString += FString::Printf(TEXT("?Team=%i"), ConnectDesiredTeam);
			}

			if (!PendingInstanceID.IsEmpty())
			{
				ConnectionString += FString::Printf(TEXT("?Session=%s"), *PendingInstanceID);
				PendingInstanceID.Empty();
			}

			UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
			UWorld* World = UTGameViewport ? UTGameViewport->GetWorldNoActiveWorldOverride() : GetWorld();
			
			FWorldContext &Context = GEngine->GetWorldContextFromWorldChecked(World);
			Context.LastURL.RemoveOption(TEXT("QuickMatch"));
			Context.LastURL.RemoveOption(TEXT("Friend"));
			Context.LastURL.RemoveOption(TEXT("Session"));
			
			PlayerController->ClientTravel(ConnectionString, ETravelType::TRAVEL_Absolute,false);

			bWantsToConnectAsSpectator = false;
			UE_LOG(UT,Verbose,TEXT("----- Joined "));

			return;
		}
	}


	UE_LOG(UT,Verbose,TEXT("----- Unsuccessful Join"));

	bCancelJoinSession = false;

	// Any failures, return to the main menu.
	bWantsToConnectAsSpectator = false;

	if (Result == EOnJoinSessionCompleteResult::AlreadyInSession)
	{
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "AlreadyInSession", "You are already in a session and can't join another."));
	}
	if (Result == EOnJoinSessionCompleteResult::SessionIsFull)
	{
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "SessionFull", "The session you are attempting to join is full."));
	}

	CloseConnectingDialog();
#if !UE_SERVER
	if (GetWorld()->bIsDefaultLevel && !DesktopSlateWidget.IsValid())
	{
		ReturnToMainMenu();
	}
#endif
}

void UUTLocalPlayer::LeaveSession()
{
	if (OnlineIdentityInterface.IsValid())
	{
		if (OnlineSessionInterface.IsValid() && OnlineSessionInterface->GetSessionSettings(GameSessionName) != nullptr)
		{
			OnlineSessionInterface->EndSession(GameSessionName);
		}
		else if (bPendingLoginCreds)
		{
			Logout();
		}
	}
}

void UUTLocalPlayer::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	OnlineSessionInterface->DestroySession(GameSessionName);
}

void UUTLocalPlayer::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(UT,Warning, TEXT("----------- [OnDestroySessionComplete %i"), bDelayedJoinSession);

	if (bPendingLoginCreds)
	{
		Logout();
	}
	else
	{
		JoinPendingSession();
	}
}

void UUTLocalPlayer::UpdatePresence(FString NewPresenceString, bool bAllowInvites, bool bAllowJoinInProgress, bool bAllowJoinViaPresence, bool bAllowJoinViaPresenceFriendsOnly)
{
	// XMPP code isn't working correctly on linux
#if PLATFORM_LINUX
	return;
#endif

	UE_LOG(UT,Log,TEXT("UpdatePResence %i %i %i %i"), bAllowInvites, bAllowJoinInProgress, bAllowJoinViaPresence, bAllowJoinViaPresenceFriendsOnly);
	if (OnlineIdentityInterface.IsValid() && OnlineSessionInterface.IsValid() && OnlinePresenceInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			FOnlineSessionSettings* GameSettings = OnlineSessionInterface->GetSessionSettings(TEXT("Game"));
			if (GameSettings != NULL)
			{
				GameSettings->bAllowInvites = bAllowInvites;
				GameSettings->bAllowJoinInProgress = bAllowJoinInProgress;
				GameSettings->bAllowJoinViaPresence = bAllowJoinViaPresence;
				GameSettings->bAllowJoinViaPresenceFriendsOnly = bAllowJoinViaPresenceFriendsOnly;
				OnlineSessionInterface->UpdateSession(TEXT("Game"), *GameSettings, false);

				UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
				if (UTGameInstance)
				{
					UUTParty* Party = UTGameInstance->GetParties();
					if (Party)
					{
						UPartyGameState* PartyGameState = Party->GetPersistentParty();
						if (PartyGameState)
						{
							UE_LOG(UT,Log,TEXT("Calling UpdateAcceptingMembers %i"),bAllowJoinViaPresence);
							PartyGameState->SetAcceptingMembers(bAllowJoinViaPresence, EJoinPartyDenialReason::Busy);
						}
					}
				}
			}

			TSharedPtr<FOnlineUserPresence> CurrentPresence;
			OnlinePresenceInterface->GetCachedPresence(*UserId, CurrentPresence);
			if (CurrentPresence.IsValid())
			{
				CurrentPresence->Status.StatusStr = NewPresenceString;
				OnlinePresenceInterface->SetPresence(*UserId, CurrentPresence->Status, IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnPresenceUpdated));
			}
			else
			{
				FOnlineUserPresenceStatus NewStatus;
				NewStatus.State = EOnlinePresenceState::Online;
				NewStatus.StatusStr = NewPresenceString;
				OnlinePresenceInterface->SetPresence(*UserId, NewStatus, IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnPresenceUpdated));
			}
		}
		else
		{
			LastPresenceUpdate = NewPresenceString;
			bLastAllowInvites = bAllowInvites;
		}
	}
}

bool UUTLocalPlayer::IsPlayerShowingSocialNotification() const
{
	return bShowSocialNotification;
}

void UUTLocalPlayer::OnPresenceUpdated(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
	UE_LOG(UT,Verbose,TEXT("OnPresenceUpdated %s"), (bWasSuccessful ? TEXT("Successful") : TEXT("Failed")));
}

void UUTLocalPlayer::OnPresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	UE_LOG(UT,Verbose,TEXT("Presence Received %s %i"), *UserId.ToString(), Presence->bIsJoinable);
}

void UUTLocalPlayer::HandleFriendsJoinGame(const FUniqueNetId& FriendId, const FUniqueNetId& SessionId)
{
	JoinFriendSession(FriendId, SessionId);
}

bool UUTLocalPlayer::AllowFriendsJoinGame()
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		UUTMatchmaking* Matchmaking = UTGameInstance->GetMatchmaking();
		if (Matchmaking && Matchmaking->IsMatchmaking())
		{
			return false;
		}
	}

	AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState && UTGameState->bRestrictPartyJoin)
	{
		return false;	
	}

	// determine when to disable "join game" option in friends/chat UI
	return true;
}

void UUTLocalPlayer::HandleFriendsNotificationAvail(bool bAvailable)
{
	bShowSocialNotification = bAvailable;
}

void UUTLocalPlayer::HandleFriendsActionNotification(TSharedRef<FFriendsAndChatMessage> FriendsAndChatMessage)
{
#if WITH_SOCIAL
	if (FriendsAndChatMessage->GetMessageType() == EMessageType::FriendAccepted ||
		FriendsAndChatMessage->GetMessageType() == EMessageType::FriendInvite ||
		(FriendsAndChatMessage->GetMessageType() == EMessageType::ChatMessage && !bShowingFriendsMenu))
	{
		bShowSocialNotification = FriendsAndChatMessage->GetMessageType() != EMessageType::FriendAccepted;
		ShowToast(FText::FromString(FriendsAndChatMessage->GetMessage()),1.5f, true);
	}

	// SUTPartyInviteWidget will show the invite if we're in menu game
	if (FriendsAndChatMessage->GetMessageType() == EMessageType::GameInvite && !IsMenuGame())
	{
		bShowSocialNotification = true;
		ShowToast(FText::FromString(FriendsAndChatMessage->GetMessage()),1.5f, true);
	}
#endif
}

void UUTLocalPlayer::JoinFriendSession(const FUniqueNetId& FriendId, const FUniqueNetId& SessionId)
{
	UE_LOG(UT, Log, TEXT("##########################"));
	UE_LOG(UT, Log, TEXT("Joining a Friend Session"));
	UE_LOG(UT, Log, TEXT("##########################"));

	//@todo samz - use FindSessionById instead of FindFriendSession with a pending SessionId
	PendingFriendInviteSessionId = SessionId.ToString();
	PendingFriendInviteFriendId = FriendId.ToString();
	OnlineSessionInterface->FindFriendSession(0, FriendId);
}

void UUTLocalPlayer::OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	if (bWasSuccessful)
	{
		if (SearchResult.Session.SessionInfo.IsValid())
		{
			bAttemptingForceJoin = false;
			JoinSession(SearchResult, false);
		}
		else
		{
			PendingFriendInviteFriendId = TEXT("");
			MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "InvalidFriendSession", "Friend no longer in session."));
		}
	}
	else
	{
		PendingFriendInviteFriendId = TEXT("");
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "NoFriendSession", "Couldn't find friend session to join."));
	}
	PendingFriendInviteSessionId = FString();
}

FName UUTLocalPlayer::GetCountryFlag()
{
#if WITH_PROFILE
	UUtMcpProfile* McpProfile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	if (McpProfile)
	{
		return McpProfile->GetCountryFlag();
	}
#endif
	if (PlayerController)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS)
		{
			return PS->CountryFlag;
		}
	}
	return NAME_None;
}

FName UUTLocalPlayer::GetAvatar()
{
#if WITH_PROFILE
	UUtMcpProfile* McpProfile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	if (McpProfile)
	{
		return McpProfile->GetAvatar();
	}
#endif

	if (PlayerController)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS) return PS->Avatar;
	}
	return NAME_None;
}

void UUTLocalPlayer::SetCountryFlagAndAvatar(FName NewFlag, FName NewAvatar)
{
#if WITH_PROFILE
	UUtMcpProfile* McpProfile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	if (McpProfile)
	{
		McpProfile->SetAvatarAndFlag(NewAvatar.ToString(), NewFlag.ToString());
	}
#endif

	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
	if (PC != NULL)
	{
		PC->ServerReceiveCountryFlag(NewFlag);
	}

	AUTBasePlayerController * BasePC = Cast<AUTBasePlayerController>(PlayerController);
	if (BasePC != NULL)
	{
		BasePC->ServerSetAvatar(NewAvatar);
	}
}

#if !UE_SERVER
void UUTLocalPlayer::StartQuickMatch(int32 PlaylistId)
{
	bQuickmatchOnLevelChange = false;
	PendingQuickmatchType = -1;

	UUTPlaylistManager* UTPlaylistManager = Cast<UUTGameInstance>(GetGameInstance())->GetPlaylistManager();

	if (PlaylistId >= 0 && IsLoggedIn() && OnlineSessionInterface.IsValid() && UTPlaylistManager != nullptr)
	{
		// If this is the vs. Flag run, then popup a dialog and ask for a difficulty	
		FUTGameRuleset* Ruleset = UTPlaylistManager->GetRuleset(PlaylistId);
		if (Ruleset)
		{
			if (Ruleset->UniqueTag == EEpicDefaultRuleTags::FlagRunVSAI)	
			{
				DifficultyLevelDialog = SNew(SUTDifficultyLevel).PlayerOwner(this).OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::DifficultyResult));
				if (DifficultyLevelDialog.IsValid())
				{
					OpenDialog(DifficultyLevelDialog.ToSharedRef());
					return;
				}
			}

			if (!UTPlaylistManager->IsValidPlaylist(PlaylistId))
			{
				UE_LOG(UT, Warning, TEXT("Quickmatch attempt with an invalid playlist id of %i"), PlaylistId);
				return;
			}

			UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UMatchmakingContext::StaticClass()));
			if (MatchmakingContext)
			{
				MatchmakingContext->StartMatchmaking(PlaylistId);

				if (FUTAnalytics::IsAvailable())
				{
					FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("QuickMatch - %s"), *Ruleset->UniqueTag));
				}
			}
		}
	}
	else
	{
		MessageBox(NSLOCTEXT("Generic","LoginNeededTitle","Login Needed"), NSLOCTEXT("Generic","LoginNeededMessage","You need to login before you can do that."));
	}
}

FText UUTLocalPlayer::PlayListIDToText(int32 PlayListId)
{
	//TODO: Extend the play list system to have a menu friendly name

	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance && UTGameInstance->GetPlaylistManager())
	{
		FString FriendlyName;
		FUTGameRuleset* Ruleset = UTGameInstance->GetPlaylistManager()->GetRuleset(PlayListId);
		if (Ruleset)
		{
			return FText::FromString(TEXT("a ") + Ruleset->Title + TEXT(" " ));
		}
	}

	return FText::GetEmpty();

}

void UUTLocalPlayer::DifficultyResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID != UTDIALOG_BUTTON_CANCEL && DifficultyLevelDialog.IsValid())
	{
		StartQuickMatch(DifficultyLevelDialog->GetDifficulty() == 1 ? 17 : 16);
		DifficultyLevelDialog.Reset();
	}
}

void UUTLocalPlayer::CloseQuickMatch()
{
	if (MatchmakingDialog.IsValid())
	{
		CloseDialog(MatchmakingDialog.ToSharedRef());
	}
}

#endif

void UUTLocalPlayer::CheckIfBrowserisDone()
{
#if !UE_SERVER
	if (!ServerBrowserWidget.IsValid() || !ServerBrowserWidget->IsRefreshing())
	{
		GetWorld()->GetTimerManager().ClearTimer(BrowerCheckHandle);	
		StartQuickMatch(PendingQuickmatchType);
	}
#endif
}


void UUTLocalPlayer::ShowConnectingDialog()
{
#if !UE_SERVER
	if (!ConnectingDialog.IsValid())
	{
		FDialogResultDelegate Callback;
		Callback.BindUObject(this, &UUTLocalPlayer::ConnectingDialogCancel);

		TSharedPtr<SUTDialogBase> NewDialog; // important to make sure the ref count stays until OpenDialog()
		SAssignNew(NewDialog, SUTMessageBoxDialog)
			.PlayerOwner(this)
			.DialogTitle(NSLOCTEXT("UT", "ConnectingTitle", "Connecting..."))
			.MessageText(NSLOCTEXT("UT", "ConnectingText", "Connecting to server, please wait..."))
			.ButtonMask(UTDIALOG_BUTTON_CANCEL)
			.OnDialogResult(Callback);

		ConnectingDialog = NewDialog;
		OpenDialog(NewDialog.ToSharedRef());
	}
#endif
}
void UUTLocalPlayer::CloseConnectingDialog()
{
#if !UE_SERVER
	if (ConnectingDialog.IsValid())
	{
		CloseDialog(ConnectingDialog.Pin().ToSharedRef());
	}
#endif
}
void UUTLocalPlayer::ConnectingDialogCancel(TSharedPtr<SCompoundWidget> Dialog, uint16 ButtonID)
{
#if !UE_SERVER
	GEngine->Exec(GetWorld(), TEXT("Cancel"));
#endif
}

bool UUTLocalPlayer::IsInSession()
{ 
	TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
	return (UserId.IsValid() && OnlineSessionInterface.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName,*UserId));
}

void UUTLocalPlayer::ShowPlayerInfo(const FString& TargetId, const FString PlayerName)
{
#if !UE_SERVER
	if (DesktopSlateWidget.IsValid() && !IsMenuGame() && GetWorld()->GetGameState<AUTLobbyGameState>() == nullptr && GetWorld()->GetNetMode() != NM_Standalone)
	{
		HideMenu();
	}
	OpenDialog(
			SNew(SUTPlayerInfoDialog)
				.PlayerOwner(this)
				.TargetUniqueId(TargetId)
				.TargetName(PlayerName)
	);
#endif
}

int32 UUTLocalPlayer::GetFriendsList(TArray< FUTFriend >& OutFriendsList)
{
	OutFriendsList.Empty();
	int32 RetVal = 0;

	if (OnlineFriendsInterface.IsValid() && OnlineUserInterface.IsValid())
	{
		TArray< TSharedRef< FOnlineFriend > > FriendsList;
		if (OnlineFriendsInterface->GetFriendsList(0, TEXT("default"), FriendsList))
		{
			for (auto Friend : FriendsList)
			{
				TSharedPtr<FOnlineUser> User = OnlineUserInterface->GetUserInfo(0, *Friend->GetUserId());
				if (User.IsValid())
				{
					OutFriendsList.Add(FUTFriend(Friend->GetUserId()->ToString(), User->GetDisplayName(), true, Friend->GetPresence().bIsOnline, Friend->GetPresence().bIsPlayingThisGame));
				}
			}

			OutFriendsList.Sort([](const FUTFriend& A, const FUTFriend& B) -> bool
			{
				return A.DisplayName < B.DisplayName;
			});
		}
	}

	return RetVal;
}

int32 UUTLocalPlayer::GetRecentPlayersList(TArray< FUTFriend >& OutRecentPlayersList)
{
	OutRecentPlayersList.Empty();

	int32 RetVal = 0;

	if (OnlineIdentityInterface.IsValid() && OnlineFriendsInterface.IsValid() && OnlineUserInterface.IsValid())
	{
		TArray< TSharedRef< FOnlineRecentPlayer > > RecentPlayersList;
		if (OnlineFriendsInterface->GetRecentPlayers(*OnlineIdentityInterface->GetUniquePlayerId(GetControllerId()), TEXT("ut"), RecentPlayersList))
		{
			for (auto RecentPlayer : RecentPlayersList)
			{
				TSharedPtr<FOnlineUser> User = OnlineUserInterface->GetUserInfo(0, *RecentPlayer->GetUserId());
				if (User.IsValid())
				{
					OutRecentPlayersList.Add(FUTFriend(RecentPlayer->GetUserId()->ToString(), User->GetDisplayName(), true, false, false));
				}
			}
		}
	}

	return RetVal;
}

void UUTLocalPlayer::RequestFriendship(TSharedPtr<const FUniqueNetId> FriendID)
{
	if (OnlineFriendsInterface.IsValid() && FriendID.IsValid())
	{
		OnlineFriendsInterface->SendInvite(0, *FriendID.Get(), EFriendsLists::ToString(EFriendsLists::Default));
	}
}

void UUTLocalPlayer::SendFriendRequest(AUTPlayerState* DesiredPlayerState)
{
	if (DesiredPlayerState != nullptr)
	{
		RequestFriendship(DesiredPlayerState->UniqueId.GetUniqueNetId());
	}
}

bool UUTLocalPlayer::ContentExists(const FPackageRedirectReference& Redirect)
{

	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport)
	{
		return UTGameViewport->CheckIfRedirectExists(Redirect);
	}

	return false;
}

void UUTLocalPlayer::AcquireContent(TArray<FPackageRedirectReference>& Redirects)
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport)
	{
		for (int32 i = 0; i < Redirects.Num(); i++)
		{
			if (!Redirects[i].PackageName.IsEmpty() && !ContentExists(Redirects[i]))
			{
				UTGameViewport->DownloadRedirect(Redirects[i]);
			}
		}
	}
}

bool UUTLocalPlayer::RequiresDLCWarning()
{
	// Look at the trust level of the current session
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(TEXT("Game"));

		if (Settings != nullptr)
		{
			int32 TrustLevel = 2;
			if (Settings->Get(SETTING_TRUSTLEVEL, TrustLevel))
			{
				// Epic servers never need to show the warning
				if (TrustLevel == 0)
				{
					return false;
				}
			}

			// Look to see if we have already accepted the warning for this server
			FString ServerInstanceGUID;
			if ( Settings->Get(SETTING_SERVERINSTANCEGUID, ServerInstanceGUID) )
			{
				if (AcceptedDLCServers.Find(ServerInstanceGUID) != INDEX_NONE)
				{
					return false;
				}
			}
		}
	}

	return true;
}

void UUTLocalPlayer::ShowDLCWarning()
{
#if !UE_SERVER
	// If we are already showing the dialog, then just exit
	if (DLCWarningDialog.IsValid()) return;


	if (RequiresDLCWarning())
	{
		int32 TrustLevel = 2;

		// Look at the trust level of the current session
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
			FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(TEXT("Game"));

			if (Settings != nullptr)
			{
				Settings->Get(SETTING_TRUSTLEVEL, TrustLevel);
			}
		}

		SAssignNew(DLCWarningDialog, SUTDLCWarningDialog)
			.PlayerOwner(this)
			.MessageText(NSLOCTEXT("UTLocalPlayer","ContentWarningText","This community server is sending you custom content that will offer a different experience from official servers. Continue download?"))
			.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ContentAcceptResult));

		OpenDialog(DLCWarningDialog.ToSharedRef());
	}
#endif
}

bool UUTLocalPlayer::IsShowingDLCWarning()
{
#if !UE_SERVER
	return DLCWarningDialog.IsValid();
#else
	return false;
#endif
}

#if !UE_SERVER
void UUTLocalPlayer::ContentAcceptResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	DLCWarningDialog.Reset();

	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		// Find the ServerInstanceGUID and add it to the white list
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
			FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(TEXT("Game"));

			if (Settings != nullptr)
			{
				// Look to see if we have already accepted the warning for this server
				FString ServerInstanceGUID;
				if ( Settings->Get(SETTING_SERVERINSTANCEGUID, ServerInstanceGUID) )
				{
					AcceptedDLCServers.Add(ServerInstanceGUID);
				}
			}
		}
	}
	else
	{
		UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
		if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
		{
			UTGameViewport->CancelAllRedirectDownloads();		

			AUTLobbyPC* PC = Cast<AUTLobbyPC>(PlayerController);
			if (PC)
			{
				PC->ResetAllRedirects();
			}
		}
	}
}
#endif


FText UUTLocalPlayer::GetDownloadStatusText()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		return UTGameViewport->DownloadStatusText;
	}

	return FText::GetEmpty();
}

FText UUTLocalPlayer::GetDownloadFilename()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		return FText::FromString(UTGameViewport->Download_CurrentFile);
	}

	return FText::GetEmpty();
}

int32 UUTLocalPlayer::GetNumberOfPendingDownloads()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		return UTGameViewport->Download_NumFilesLeft;
	}

	return 0;
}

int32 UUTLocalPlayer::GetDownloadBytes()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		return UTGameViewport->Download_NumBytes;
	}

	return 0;
}

float UUTLocalPlayer::GetDownloadProgress()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		return UTGameViewport->Download_Percentage;
	}

	return 0.0f;
}

bool UUTLocalPlayer::IsDownloadInProgress()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	return UTGameViewport ? UTGameViewport->IsDownloadInProgress() : false;
}

void UUTLocalPlayer::CancelDownload()
{
	if (IsDownloadInProgress())
	{
		UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
		if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
		{
			UTGameViewport->CancelAllRedirectDownloads();		
		}

		HideRedirectDownload();
	}
}

void UUTLocalPlayer::HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	AUTBasePlayerController* BasePlayerController = Cast<AUTBasePlayerController>(PlayerController);
	if (BasePlayerController)
	{
		BasePlayerController->HandleNetworkFailureMessage(FailureType, ErrorString);
	}
}

void UUTLocalPlayer::OpenMapVote(AUTGameState* GameState)
{
#if !UE_SERVER
	if (!MapVoteMenu.IsValid())
	{

		if (GameState == NULL)
		{
			GameState = GetWorld()->GetGameState<AUTGameState>();
			if (GameState == NULL) return;
		}

		SAssignNew(MapVoteMenu,SUTMapVoteDialog).PlayerOwner(this).GameState(GameState);
		OpenDialog( MapVoteMenu.ToSharedRef(), 200 );
	}
#endif
}

void UUTLocalPlayer::CloseMapVote()
{
#if !UE_SERVER
	if (MapVoteMenu.IsValid())
	{
		CloseDialog(MapVoteMenu.ToSharedRef());		
		MapVoteMenu.Reset();
	}
#endif
}

void UUTLocalPlayer::OpenReplayWindow()
{
#if !UE_SERVER
	UDemoNetDriver* DemoDriver = GetWorld()->DemoNetDriver;
	if (DemoDriver)
	{
		// If the demo net driver changed, reopen the window, we're in a different replay.
		if (ReplayWindow.IsValid() && ReplayWindow->GetDemoNetDriver() != DemoDriver)
		{
			CloseReplayWindow();
		}

		if (!ReplayWindow.IsValid())
		{
			SAssignNew(ReplayWindow, SUTReplayWindow)
				.PlayerOwner(this)
				.DemoNetDriver(DemoDriver);

			UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
			if (ReplayWindow.IsValid() && UTGVC != nullptr)
			{
				UTGVC->AddViewportWidgetContent_NoAspect(ReplayWindow.ToSharedRef(), 0);
				ReplayWindow->SetVisibility(EVisibility::SelfHitTestInvisible);
				ReplayWindow->GrabKeyboardFocus();
			}
		}
	}
#endif
}

void UUTLocalPlayer::CloseReplayWindow()
{
#if !UE_SERVER
	UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
	if (ReplayWindow.IsValid() && UTGVC != nullptr)
	{
		UTGVC->RemoveViewportWidgetContent_NoAspect(ReplayWindow.ToSharedRef());
		ReplayWindow.Reset();
	}
#endif
}

void UUTLocalPlayer::ToggleReplayWindow()
{
#if !UE_SERVER
	if (IsReplay())
	{
		if (!ReplayWindow.IsValid())
		{
			OpenReplayWindow();
		}
		else
		{
			CloseReplayWindow();
		}
	}
#endif
}

bool UUTLocalPlayer::IsReplay()
{
	if (GetWorld()->DemoNetDriver == nullptr)
	{
		return false;
	}

	UUTDemoNetDriver* UTDemoNetDriver = Cast<UUTDemoNetDriver>(GetWorld()->DemoNetDriver);
	if (UTDemoNetDriver && UTDemoNetDriver->bIsLocalReplay)
	{
		return false;
	}

	return true;
}

#if !UE_SERVER

void UUTLocalPlayer::RecordReplay(float RecordTime)
{
	if (!bRecordingReplay)
	{
		CloseReplayWindow();

		bRecordingReplay = true;

		static const FName VideoRecordingFeatureName("VideoRecording");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
		{
			UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
			if (VideoRecorder)
			{
				VideoRecorder->OnRecordingComplete().AddUObject(this, &UUTLocalPlayer::RecordingReplayComplete);
				VideoRecorder->StartRecording(RecordTime);
			}
		}
	}
}

void UUTLocalPlayer::RecordingReplayComplete()
{
	bRecordingReplay = false;

	static const FName VideoRecordingFeatureName("VideoRecording");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
	{
		UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
		if (VideoRecorder)
		{
			VideoRecorder->OnRecordingComplete().RemoveAll(this);
		}
	}

	// Pause the replay streamer
	AWorldSettings* const WorldSettings = GetWorld()->GetWorldSettings();
	if (WorldSettings->Pauser == nullptr)
	{
		WorldSettings->Pauser = (PlayerController != nullptr) ? PlayerController->PlayerState : nullptr;
	}

	// Show a dialog asking player if they want to compress
	ShowMessage(NSLOCTEXT("VideoMessages", "CompressNowTitle", "Compress now?"),
				NSLOCTEXT("VideoMessages", "CompressNow", "Your video recorded successfully.\nWould you like to compress the video now? It may take several minutes."),
				UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ShouldVideoCompressDialogResult));
}

void UUTLocalPlayer::ShouldVideoCompressDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		// Pick a proper filename for the video
		FString BasePath = FPaths::ScreenShotDir();
		if (IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true))
		{
			RecordedReplayFilename = BasePath / TEXT("anim.webm");
			static int32 WebMIndex = 0;
			const int32 MaxTestWebMIndex = 65536;
			for (int32 TestWebMIndex = WebMIndex + 1; TestWebMIndex < MaxTestWebMIndex; ++TestWebMIndex)
			{
				const FString TestFileName = BasePath / FString::Printf(TEXT("UTReplay%05i.webm"), TestWebMIndex);
				if (IFileManager::Get().FileSize(*TestFileName) < 0)
				{
					WebMIndex = TestWebMIndex;
					RecordedReplayFilename = TestFileName;
					break;
				}
			}

			static const FName VideoRecordingFeatureName("VideoRecording");
			if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
			{
				UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
				if (VideoRecorder)
				{
					// Open a dialog that shows a nice progress bar of the compression
					OpenDialog(SNew(SUTVideoCompressionDialog)
								.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::VideoCompressDialogResult))
								.DialogTitle(NSLOCTEXT("VideoMessages", "Compressing", "Compressing"))
								.PlayerOwner(this)
								.VideoRecorder(VideoRecorder)
								.VideoFilename(RecordedReplayFilename)
								);
				}
			}
		}
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::VideoCompressDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		OpenDialog(SNew(SUTInputBoxDialog)
			.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ShouldVideoUploadDialogResult))
			.PlayerOwner(this)
			.DialogSize(FVector2D(700, 400))
			.bDialogSizeIsRelative(false)
			.DefaultInput(TEXT("UT Automated Upload"))
			.DialogTitle(NSLOCTEXT("VideoMessages", "UploadNowTitle", "Upload to YouTube?"))
			.MessageText(NSLOCTEXT("VideoMessages", "UploadNow", "Your video compressed successfully.\nWould you like to upload the video to YouTube now?\n\nPlease enter a video title in the text box."))
			.ButtonMask(UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO)
			);
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::ShouldVideoUploadDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		TSharedPtr<SUTInputBoxDialog> Box = StaticCastSharedPtr<SUTInputBoxDialog>(Widget);
		if (Box.IsValid())
		{
			RecordedReplayTitle = Box->GetInputText();
		}

		if (YoutubeAccessToken.IsEmpty())
		{
			GetYoutubeConsentForUpload();
		}
		else if (!YoutubeRefreshToken.IsEmpty())
		{
			// Show a dialog here to stop the user for doing anything
			YoutubeDialog = ShowMessage(NSLOCTEXT("VideoMessages", "YoutubeTokenTitle", "AccessingYoutube"),
				NSLOCTEXT("VideoMessages", "YoutubeToken", "Contacting YouTube..."), 0);

			FHttpRequestPtr YoutubeTokenRefreshRequest = FHttpModule::Get().CreateRequest();
			YoutubeTokenRefreshRequest->SetURL(TEXT("https://accounts.google.com/o/oauth2/token"));
			YoutubeTokenRefreshRequest->OnProcessRequestComplete().BindUObject(this, &UUTLocalPlayer::YoutubeTokenRefreshComplete);
			YoutubeTokenRefreshRequest->SetVerb(TEXT("POST"));
			YoutubeTokenRefreshRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));

			// ClientID and ClientSecret UT Youtube app on PLK google account
			FString ClientID = TEXT("465724645978-10npjjgfbb03p4ko12ku1vq1ioshts24.apps.googleusercontent.com");
			FString ClientSecret = TEXT("kNKauX2DKUq_5cks86R8rD5E");
			FString TokenRequest = TEXT("client_id=") + ClientID + TEXT("&client_secret=") + ClientSecret + 
				                   TEXT("&refresh_token=") + YoutubeRefreshToken + TEXT("&grant_type=refresh_token");

			YoutubeTokenRefreshRequest->SetContentAsString(TokenRequest);
			YoutubeTokenRefreshRequest->ProcessRequest();
		}
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::YoutubeConsentResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		if (!YoutubeDialog.IsValid())
		{
			// Show a dialog here to stop the user for doing anything
			YoutubeDialog = ShowMessage(NSLOCTEXT("VideoMessages", "YoutubeTokenTitle", "AccessingYoutube"),
				NSLOCTEXT("VideoMessages", "YoutubeToken", "Contacting YouTube..."), 0);

			FHttpRequestPtr YoutubeTokenRequest = FHttpModule::Get().CreateRequest();
			YoutubeTokenRequest->SetURL(TEXT("https://accounts.google.com/o/oauth2/token"));
			YoutubeTokenRequest->OnProcessRequestComplete().BindUObject(this, &UUTLocalPlayer::YoutubeTokenRequestComplete);
			YoutubeTokenRequest->SetVerb(TEXT("POST"));
			YoutubeTokenRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));

			// ClientID and ClientSecret UT Youtube app on PLK google account
			FString ClientID = TEXT("465724645978-10npjjgfbb03p4ko12ku1vq1ioshts24.apps.googleusercontent.com");
			FString ClientSecret = TEXT("kNKauX2DKUq_5cks86R8rD5E");
			FString TokenRequest = TEXT("code=") + YoutubeConsentDialog->UniqueCode + TEXT("&client_id=") + ClientID
				+ TEXT("&client_secret=") + ClientSecret + TEXT("&redirect_uri=urn:ietf:wg:oauth:2.0:oob&grant_type=authorization_code");

			YoutubeTokenRequest->SetContentAsString(TokenRequest);
			YoutubeTokenRequest->ProcessRequest();
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Already getting Youtube Consent"));
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Failed to get Youtube consent"));
	}
}

void UUTLocalPlayer::YoutubeTokenRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (YoutubeDialog.IsValid())
	{
		CloseDialog(YoutubeDialog.ToSharedRef());
		YoutubeDialog.Reset();
	}

	if (HttpResponse.IsValid())
	{
		if (HttpResponse->GetResponseCode() == 200)
		{
			TSharedPtr<FJsonObject> YoutubeTokenJson;
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
			if (FJsonSerializer::Deserialize(JsonReader, YoutubeTokenJson) && YoutubeTokenJson.IsValid())
			{
				YoutubeTokenJson->TryGetStringField(TEXT("access_token"), YoutubeAccessToken);
				YoutubeTokenJson->TryGetStringField(TEXT("refresh_token"), YoutubeRefreshToken);

				UE_LOG(UT, Log, TEXT("YoutubeTokenRequestComplete %s %s"), *YoutubeAccessToken, *YoutubeRefreshToken);

				SaveConfig();

				UploadVideoToYoutube();
			}
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Failed to get token from Youtube\n%s"), *HttpResponse->GetContentAsString());
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Failed to get token from Youtube. Request failed."));
	}
}

void UUTLocalPlayer::YoutubeTokenRefreshComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (YoutubeDialog.IsValid())
	{
		CloseDialog(YoutubeDialog.ToSharedRef());
		YoutubeDialog.Reset();
	}

	if (HttpResponse.IsValid() && HttpResponse->GetResponseCode() == 200)
	{
		UE_LOG(UT, Log, TEXT("YouTube Token refresh succeeded"));

		TSharedPtr<FJsonObject> YoutubeTokenJson;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, YoutubeTokenJson) && YoutubeTokenJson.IsValid())
		{
			YoutubeTokenJson->TryGetStringField(TEXT("access_token"), YoutubeAccessToken);
			SaveConfig();

			UploadVideoToYoutube();
		}
	}
	else
	{
		if (HttpResponse.IsValid())
		{
			UE_LOG(UT, Log, TEXT("YouTube Token might've expired, doing full consent\n%s"), *HttpResponse->GetContentAsString());
		}
		else
		{
			UE_LOG(UT, Log, TEXT("YouTube Token might've expired, doing full consent"));
		}

		// Refresh token might have been expired
		YoutubeAccessToken.Empty();
		YoutubeRefreshToken.Empty();

		GetYoutubeConsentForUpload();
	}
}

void UUTLocalPlayer::GetYoutubeConsentForUpload()
{
	// Get youtube consent
	OpenDialog(
		SAssignNew(YoutubeConsentDialog, SUTYoutubeConsentDialog)
		.PlayerOwner(this)
		.DialogSize(FVector2D(0.8f, 0.8f))
		.DialogPosition(FVector2D(0.5f, 0.5f))
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "YoutubeConsent", "Allow UT to post to YouTube?"))
		.ButtonMask(UTDIALOG_BUTTON_CANCEL)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeConsentResult))
		);
}

void UUTLocalPlayer::UploadVideoToYoutube()
{
	// Get youtube consent
	OpenDialog(
		SNew(SUTYoutubeUploadDialog)
		.PlayerOwner(this)
		.ButtonMask(UTDIALOG_BUTTON_CANCEL)
		.VideoFilename(RecordedReplayFilename)
		.AccessToken(YoutubeAccessToken)
		.VideoTitle(RecordedReplayTitle)
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "YoutubeUpload", "Uploading To Youtube"))
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadResult))
		);
}

void UUTLocalPlayer::YoutubeUploadResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteTitle", "Upload To Youtube Complete"),
					NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadComplete", "Your upload to Youtube completed successfully. It will be available in a few minutes."),
					UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
	}
	else
	{
		bool bNeedsYoutubeSignup = false;
		SUTYoutubeUploadDialog* UploadDialog = (SUTYoutubeUploadDialog*)Widget.Get();
		TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(UploadDialog->UploadFailMessage);
		TSharedPtr< FJsonObject > JsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		{
			if (JsonObject.IsValid())
			{
				const TSharedPtr<FJsonObject>* ErrorObject;
				if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
				{
					const TArray<TSharedPtr<FJsonValue>>* ErrorArray;
					if ((*ErrorObject)->TryGetArrayField(TEXT("errors"), ErrorArray))
					{
						for (int32 Idx = 0; Idx < ErrorArray->Num(); Idx++)
						{
							FString ErrorReason;
							if ((*ErrorArray)[Idx]->AsObject()->TryGetStringField(TEXT("reason"), ErrorReason))
							{
								if (ErrorReason == TEXT("youtubeSignupRequired"))
								{
									bNeedsYoutubeSignup = true;
								}
							}
						}
					}
				}
			}
		}

		if (bNeedsYoutubeSignup)
		{
			ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadNeedSignupTitle", "Upload To Youtube Failed"),
						NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadNeedSignup", "Your account does not currently have a YouTube channel.\nPlease create one and try again."),
						UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
		}
		else
		{		
			ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteFailedTitle", "Upload To Youtube Failed"),
						NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteFailed", "Your upload to Youtube did not complete successfully."),
						UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
		}
	}
}

void UUTLocalPlayer::YoutubeUploadCompleteResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	OpenReplayWindow();
}

#endif

void UUTLocalPlayer::VerifyGameSession(const FString& ServerSessionId)
{
	if (bJoinSessionInProgress || IsReplay())
	{
		return;
	}

	if (OnlineSessionInterface.IsValid())
	{
		// Get our current Session Id.
		FNamedOnlineSession* Session = OnlineSessionInterface->GetNamedSession(FName(TEXT("Game")));
		if (Session == NULL || !Session->SessionInfo.IsValid() || Session->SessionInfo->GetSessionId().ToString() != ServerSessionId)
		{
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			if (UserId.IsValid())
			{
				TSharedPtr<const FUniqueNetId> ServerId = MakeShareable(new FUniqueNetIdString(ServerSessionId));		
				TSharedPtr<const FUniqueNetId> EmptyId = MakeShareable(new FUniqueNetIdString(""));				
				FOnSingleSessionResultCompleteDelegate CompletionDelegate;
				CompletionDelegate.BindUObject(this, &UUTLocalPlayer::OnFindSessionByIdComplete);
				OnlineSessionInterface->FindSessionById(*UserId, *ServerId, *EmptyId, CompletionDelegate);
			}
		}
	}
}

void UUTLocalPlayer::OnFindSessionByIdComplete(int32 LocalUserNum, bool bWasSucessful, const FOnlineSessionSearchResult& SearchResult)
{
	if (bWasSucessful)
	{
		bAttemptingForceJoin = true;
		bCancelJoinSession = false;
		OnlineSessionInterface->JoinSession(0, GameSessionName, SearchResult);
	}
}

void UUTLocalPlayer::CloseAllUI(bool bExceptDialogs)
{
#if !UE_SERVER
	UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
	if (GI && GI->IsMoviePlaying())
	{
		bDelayedCloseUIExcludesDialogs = bExceptDialogs;
		bCloseUICalledDuringMoviePlayback = true;
		return;
	}

	for (int32 i=0; i < UMGWidgetStack.Num(); i++)
	{
		CloseUMGWidget(UMGWidgetStack[i]);
	}

	ToastStack.Empty();
#endif

	bCloseUICalledDuringMoviePlayback = false;

	ChatArchive.Empty();

#if !UE_SERVER
	CloseAuth();
#endif

	if (GetWorld() && GetWorld()->WorldType == EWorldType::Game)
	{
		GEngine->GameViewport->RemoveAllViewportWidgets();
	}

#if !UE_SERVER

	TArray<TSharedPtr<SUTDialogBase>> DialogsToClose;
	for (TSharedPtr<SUTDialogBase> Dialog : OpenDialogs)
	{
		if (!bExceptDialogs || (Dialog.IsValid() && !Dialog->bRemainOpenThroughTravel()) )
		{
			DialogsToClose.Add(Dialog);
		}
		else
		{
			// ReAdd this dialog to the viewport.  This is a hacky solution until we have time to
			// go through and make sure noone is opening dialogs/windows/etc that aren't in the stacks
			GEngine->GameViewport->AddViewportWidgetContent(Dialog.ToSharedRef(), Dialog->ZOrder);
		}
	}

	for (TSharedPtr<SUTDialogBase> Dialog : DialogsToClose)
	{
		CloseDialog(Dialog.ToSharedRef());
	}
	
	if (DesktopSlateWidget.IsValid())
	{
		DesktopSlateWidget->OnMenuClosed();
		DesktopSlateWidget.Reset();
	}

	// These should all be proper closes
	ServerBrowserWidget.Reset();
	ReplayBrowserWidget.Reset();
	StatsViewerWidget.Reset();
	CreditsPanelWidget.Reset();
	ContentLoadingMessage.Reset();
	FriendsMenu.Reset();
	ReplayWindow.Reset();
	
	CloseSpectatorWindow();
	CloseQuickChat();
	CloseWebMessage();
	CloseSavingWidget();
	CloseAbuseDialog();

	while (WindowStack.Num() > 0)
	{
		CloseWindow(WindowStack[0]);
	}

#endif
}

void UUTLocalPlayer::AttemptJoinInstance(TSharedPtr<FServerData> ServerData, FString InstanceId, bool bSpectate)
{
#if !UE_SERVER

	SAssignNew(JoinInstanceDialog, SUTJoinInstanceWindow, this)
		.ServerData(ServerData)
		.InstanceId(InstanceId)
		.bSpectator(bSpectate);

	if (JoinInstanceDialog.IsValid())
	{
		OpenWindow(JoinInstanceDialog,510);
		JoinInstanceDialog->TellSlateIWantKeyboardFocus();
	}
#endif
}
void UUTLocalPlayer::CloseJoinInstanceDialog()
{
#if !UE_SERVER
	if (JoinInstanceDialog.IsValid())
	{
		CloseWindow(JoinInstanceDialog);
		JoinInstanceDialog.Reset();
	}
#endif

}


int32 UUTLocalPlayer::GetTotalChallengeStars()
{
	int32 TotalStars = 0;
	if (CurrentProgression)
	{
		for (int32 i = 0 ; i < CurrentProgression->ChallengeResults.Num(); i++)
		{
			TotalStars += CurrentProgression->ChallengeResults[i].Stars;
		}
	}

	return TotalStars;
}

int32 UUTLocalPlayer::GetChallengeStars(FName ChallengeTag)
{
	if (CurrentProgression)
	{
		for (int32 i = 0 ; i < CurrentProgression->ChallengeResults.Num(); i++)
		{
			if (CurrentProgression->ChallengeResults[i].Tag == ChallengeTag)
			{
				return CurrentProgression->ChallengeResults[i].Stars;
			}
		}
	}

	return 0;
}

int32 UUTLocalPlayer::GetRewardStars(FName RewardTag)
{
	// Count all of the stars for this reward type.

	int32 TotalStars = 0;

	UUTGameEngine* UTGameEngine = Cast<UUTGameEngine>(GEngine);
	if (UTGameEngine )
	{
		TWeakObjectPtr<UUTChallengeManager> ChallengeManager = UTGameEngine->GetChallengeManager();
		if (ChallengeManager.IsValid())
		{
			for (auto It = ChallengeManager->Challenges.CreateConstIterator(); It; ++It)
			{
				const FUTChallengeInfo Challenge = It.Value();
				if (Challenge.RewardTag == RewardTag)
				{
					FName ChallengeTag = It.Key();
					TotalStars += GetChallengeStars(ChallengeTag);
				}
			}
		}
	}
	return TotalStars;
}

FString UUTLocalPlayer::GetChallengeDate(FName ChallengeTag)
{
	if (CurrentProgression)
	{
		for (int32 i = 0 ; i < CurrentProgression->ChallengeResults.Num(); i++)
		{
			if (CurrentProgression->ChallengeResults[i].Tag == ChallengeTag)
			{
				FDateTime LastUpdate = CurrentProgression->ChallengeResults[i].LastUpdate;
				return LastUpdate.ToString(TEXT("%m.%d.%y @ %h:%M:%S%a"));
			}
		}
	}
	return TEXT("Never");
}

void UUTLocalPlayer::AwardAchievement(FName AchievementName)
{
	static FName NAME_RequiredAchievement(TEXT("RequiredAchievement"));
	static FName NAME_CosmeticName(TEXT("CosmeticName"));
	static FName NAME_DisplayName(TEXT("DisplayName"));
	if (CurrentProgression != NULL && !CurrentProgression->Achievements.Contains(AchievementName))
	{
		CurrentProgression->Achievements.Add(AchievementName);

		TArray<FAssetData> PossibleUnlocks;
		GetAllBlueprintAssetData(AUTCosmetic::StaticClass(), PossibleUnlocks, true);
		GetAllBlueprintAssetData(AUTCharacterContent::StaticClass(), PossibleUnlocks, true);
		for (const FAssetData& Item : PossibleUnlocks)
		{
			const FString* ReqAchievement = Item.TagsAndValues.Find(NAME_RequiredAchievement);
			if (ReqAchievement != NULL && FName(**ReqAchievement) == AchievementName)
			{
				const FString* DisplayName = Item.TagsAndValues.Find(NAME_DisplayName);
				if (DisplayName == NULL)
				{
					DisplayName = Item.TagsAndValues.Find(NAME_CosmeticName);
				}
				if (DisplayName != NULL)
				{
					ShowToast(FText::Format(NSLOCTEXT("UT", "AchievementAward", "Unlocked {0}"), FText::FromString(*DisplayName)));
				}
			}
		}
	}
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName ChallengeComplete
*
* @Trigger Sent when a player completes a challenge
*
* @Type Sent by the Client
*
* @EventParam ChallengeTag string Challenge Tag name
* @EventParam Stars int32 Stars this challenge is worth
* @EventParam TotalStars int32 Total earned stars
*
* @Comments
*/

void UUTLocalPlayer::ChallengeCompleted(FName ChallengeTag, int32 Stars)
{
	EarnedStars = 0;
	if (CurrentProgression && Stars > 0)
	{
		bool bFound = false;
		for (int32 i = 0 ; i < CurrentProgression->ChallengeResults.Num(); i++)
		{
			if (CurrentProgression->ChallengeResults[i].Tag == ChallengeTag)
			{
				if (CurrentProgression->ChallengeResults[i].Stars < Stars)
				{
					EarnedStars = Stars - CurrentProgression->ChallengeResults[i].Stars;
					CurrentProgression->ChallengeResults[i].Update(Stars);
					CurrentProgression->NeedsUpdate();
				}

				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			EarnedStars = Stars;
			CurrentProgression->ChallengeResults.Add(FUTChallengeResult(ChallengeTag, Stars));
			CurrentProgression->NeedsUpdate();
		}

		// Look up the Challenge info for this challenge...

		UUTGameEngine* UTGameEngine = Cast<UUTGameEngine>(GEngine);
		if (UTGameEngine )
		{
			TWeakObjectPtr<UUTChallengeManager> ChallengeManager = UTGameEngine->GetChallengeManager();
			if (ChallengeManager.IsValid())
			{
				const FUTChallengeInfo* Challenge = ChallengeManager->GetChallenge(ChallengeTag);
				if (Challenge)
				{
					int32 TotalStars = GetRewardStars(Challenge->RewardTag);
					FText ChallengeToast = FText::Format(NSLOCTEXT("Challenge", "GainedStars", "Challenge Completed!  You earned {0} stars."), FText::AsNumber(Stars));
					ShowToast(ChallengeToast);

					if (Challenge->RewardTag == NAME_REWARD_GoldStars)
					{
						if (TotalStars >= 5)
						{
							AwardAchievement(AchievementIDs::ChallengeStars5);
						}
						if (TotalStars >= 15)
						{
							AwardAchievement(AchievementIDs::ChallengeStars15);
						}
						if (TotalStars >= 25)
						{
							AwardAchievement(AchievementIDs::ChallengeStars25);
						}
						if (TotalStars >= 40)
						{
							AwardAchievement(AchievementIDs::ChallengeStars35);
						}
						if (TotalStars >= 60)
						{
							AwardAchievement(AchievementIDs::ChallengeStars45);
						}

						bool bEarnedRosterUpgrade = (TotalStars / 5 != (TotalStars - EarnedStars) / 5) && UUTChallengeManager::StaticClass()->GetDefaultObject<UUTChallengeManager>()->PlayerTeamRoster.Roster.IsValidIndex(4 + (TotalStars - Stars) / 5);
						if (bEarnedRosterUpgrade)
						{
							FText OldTeammate = FText::FromName(UUTChallengeManager::StaticClass()->GetDefaultObject<UUTChallengeManager>()->PlayerTeamRoster.Roster[(TotalStars - Stars) / 5]);
							FText NewTeammate = FText::FromName(UUTChallengeManager::StaticClass()->GetDefaultObject<UUTChallengeManager>()->PlayerTeamRoster.Roster[4 + (TotalStars - Stars) / 5]);
							RosterUpgradeText = FText::Format(NSLOCTEXT("Challenge", "RosterUpgrade", "Roster Upgrade!  {0} replaces {1}."), NewTeammate, OldTeammate);
							ShowToast(RosterUpgradeText);
						}
					}
					else if (Challenge->RewardTag == NAME_REWARD_HalloweenStars)
					{
						if (TotalStars >= 5)
						{
							AwardAchievement(AchievementIDs::ChallengePumpkins5);
						}
						if (TotalStars >= 10)
						{
							AwardAchievement(AchievementIDs::ChallengePumpkins10);
						}
						if (TotalStars >= 15)
						{
							AwardAchievement(AchievementIDs::ChallengePumpkins15);
						}
					}
				}
			}
		}

		int32 AllStars = GetTotalChallengeStars();
		CurrentProgression->TotalChallengeStars = AllStars;
		AUTPlayerState* PS = PlayerController ? Cast<AUTPlayerState>(PlayerController->PlayerState) : NULL;
		if (PS)
		{
			PS->TotalChallengeStars = AllStars;
		}

		SaveProgression();
		PushChallengeStarsToMCP();

		if (FUTAnalytics::IsAvailable())
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("ChallengeTag"), ChallengeTag.ToString()));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Stars"), Stars));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("TotalStars"), AllStars));
			FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerController), ParamArray, false);

			FUTAnalytics::GetProvider().RecordEvent(TEXT("ChallengeComplete"), ParamArray);
		}
	}
}

bool UUTLocalPlayer::QuickMatchCheckFull()
{
#if !UE_SERVER
	if (MatchmakingDialog.IsValid())
	{
		if (FUTAnalytics::IsAvailable())
		{
			FUTAnalytics::FireEvent_UTMatchMakingFailed(Cast<AUTPlayerController>(PlayerController), LastMatchmakingSessionId);
		}

		FTimerHandle TmpHandle;
		GetWorld()->GetTimerManager().SetTimer(TmpHandle, this, &UUTLocalPlayer::RestartQuickMatch, 0.5f, false);
		return true;
	}
#endif
	return false;
}

void UUTLocalPlayer::RestartQuickMatch()
{
#if !UE_SERVER
	StartQuickMatch(CurrentQuickMatchType);
#endif
}


bool UUTLocalPlayer::IsRankedMatchmakingEnabled(int32 PlaylistId)
{
	int32 MatchesPlayed = 0;
	if (PlayerController && PlayerController->PlayerState)
	{
		AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (UTPlayerState)
		{

			MatchesPlayed =	UTPlayerState->DuelMatchesPlayed + UTPlayerState->TDMMatchesPlayed + UTPlayerState->DMMatchesPlayed
								+ UTPlayerState->CTFMatchesPlayed + UTPlayerState->ShowdownMatchesPlayed + UTPlayerState->FlagRunMatchesPlayed;
		}
	}

	return ActiveRankedPlaylists.Contains(PlaylistId) && MatchesPlayed >= 10;
}

void UUTLocalPlayer::ShowAdminDialog(AUTRconAdminInfo* AdminInfo)
{
#if !UE_SERVER

	if (ViewportClient->ViewportConsole)
	{
		ViewportClient->ViewportConsole->FakeGotoState(NAME_None);
	}
	
	if (!AdminDialog.IsValid())
	{
		SAssignNew(AdminDialog,SUTAdminDialog)
			.PlayerOwner(this)
			.AdminInfo(AdminInfo);

		if ( AdminDialog.IsValid() ) 
		{
			OpenDialog(AdminDialog.ToSharedRef(),200);
		}
	}
#endif
}

void UUTLocalPlayer::AdminDialogClosed()
{
#if !UE_SERVER
	if (AdminDialog.IsValid())
	{
		AdminDialog.Reset();
	}
#endif
}


void UUTLocalPlayer::RequestServerSendAllRedirects()
{
#if !UE_SERVER
	AUTLobbyPC* PC = Cast<AUTLobbyPC>(PlayerController);
	if (PC)
	{
		PC->RequestServerSendAllRedirects();
	}
#endif
}

#if !UE_SERVER

int32 UUTLocalPlayer::NumDialogsOpened()
{
	int32 Num = OpenDialogs.Num();
	if (LoginDialog.IsValid()) Num++;
	return Num;
}
#endif

bool UUTLocalPlayer::SkipWorldRender()
{
#if !UE_SERVER

	if (DesktopSlateWidget.IsValid() && DesktopSlateWidget->SkipWorldRender())
	{
		return true;
	}

	for (auto& Dialog : OpenDialogs)
	{
		if (Dialog.IsValid() && Dialog.Get()->bSkipWorldRender)
		{
			return true;
		}
	}
#endif
	return false;
}

void UUTLocalPlayer::OpenSpectatorWindow()
{
#if !UE_SERVER
	if (!SpectatorWidget.IsValid())
	{
		SAssignNew(SpectatorWidget, SUTSpectatorWindow, this)
			.bShadow(false);

		if (SpectatorWidget.IsValid())
		{
			OpenWindow(SpectatorWidget);
		}
	}
#endif
}
void UUTLocalPlayer::CloseSpectatorWindow()
{
#if !UE_SERVER
	if (SpectatorWidget.IsValid())
	{
		CloseWindow(SpectatorWidget);
		SpectatorWidget.Reset();
	}
#endif
}

bool UUTLocalPlayer::IsFragCenterNew()
{
	if (MCPPulledData.bValid)
	{
		return FragCenterCounter != MCPPulledData.FragCenterCounter;
	}

	return false;
}

void UUTLocalPlayer::UpdateFragCenter()
{
	if (IsFragCenterNew())
	{
		FragCenterCounter = MCPPulledData.FragCenterCounter;
		SaveConfig();
	}
}

FUniqueNetIdRepl UUTLocalPlayer::GetGameAccountId() const
{
	if (OnlineIdentityInterface.IsValid())
	{
		// Not multi-screen compatible
		return FUniqueNetIdRepl(OnlineIdentityInterface->GetUniquePlayerId(0));
	}
	else
	{
		check(0);
		return FUniqueNetIdRepl();
	}
}

bool UUTLocalPlayer::IsEarningXP() const
{
	return true; // we rely on the backend to cap or disallow XP as appropriate
}

void UUTLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Loading the local profile while we wait for a connection
		LoadLocalProfileSettings();
	}

	if (!IsTemplate())
	{
#if WITH_PROFILE
		if (McpProfileManager == nullptr)
		{
			McpProfileManager = NewObject<UUtMcpProfileManager>(this);
			ActiveMcpProfileManager = McpProfileManager;
		}
#endif
	}
}

#if WITH_PROFILE

UUtMcpProfileManager* UUTLocalPlayer::GetMcpProfileManager(const FString& AccountId)
{
	if ((AccountId == GetMcpProfileManager()->GetAccountId()) || AccountId.IsEmpty())
	{
		return GetMcpProfileManager();
	}

	// Decided to not use map because not expecting very many shared profiles, plus maps are not GC'd.
	for (auto Manager : SharedMcpProfileManager)
	{
		if (AccountId == Cast<UUtMcpProfileManager>(Manager)->GetAccountId())
		{
			return Cast<UUtMcpProfileManager>(Manager);
		}
	}

	/*
	// To prevent many checks from happening while testing non MCP maps, this is used to create a dummy profile manager.
	if (NullProfileManager == nullptr)
	{
		UE_LOG(UT, Error, TEXT("UUtLocalPlayer: Invalid Profile Manager request %s"), *AccountId);
		NullProfileManager = NewObject<UUtMcpProfileManager>(this);
		NullProfileManager->OfflineInit();
		ActiveMcpProfileManager = NullProfileManager;
	}
	return NullProfileManager;
	*/

	return nullptr;
}

#endif

void UUTLocalPlayer::OnProfileManagerInitComplete(bool bSuccess, const FText& ErrorText)
{
	if (bSuccess)
	{
		UpdateSharedProfiles();
	}
	else
	{
		// If failure, it is handled here, otherwise success callback when done is in UpdateSharedProfiles() - so that it can be call independently.
		UpdateSharedProfilesComplete.ExecuteIfBound(false, ErrorText);
		UpdateSharedProfilesComplete.Unbind();
	}
}

void UUTLocalPlayer::UpdateSharedProfiles()
{
#if WITH_PROFILE
	UUtMcpProfile* McpProfile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	TSharedPtr<const FJsonValue> SharedAccounts = McpProfile->GetStat(TEXT("sharedAccounts"));
	if (SharedAccounts.IsValid())
	{
		const TArray< TSharedPtr<FJsonValue> >& AccountsArray = SharedAccounts->AsArray();

		int32 SharedProfileIndex = SharedMcpProfileManager.Num();
		if (AccountsArray.Num() > SharedProfileIndex)
		{
			FString McpAccountId = AccountsArray[SharedProfileIndex]->AsString();
			
			UUtMcpProfileManager* NewManager = NewObject<UUtMcpProfileManager>(this);
			SharedMcpProfileManager.Add(NewManager);
			TSharedPtr< const FUniqueNetId > ProfileNetID = MakeShareable(new FUniqueNetIdString(McpAccountId));

			TSharedPtr<const FUniqueNetId> UserId = nullptr;
			if (OnlineIdentityInterface.IsValid())
			{
				UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			}

			NewManager->Init(ProfileNetID, UserId, TEXT("Clan"), FUTProfilesLoaded::CreateUObject(this, &UUTLocalPlayer::OnProfileManagerInitComplete));
			return;

		}
	}

	// bind the delegate that will tell us about profile notifications
	McpProfile->OnHandleNotification().BindUObject(this, &UUTLocalPlayer::HandleProfileNotification);

	// All profiles are loaded, not sure what to stuff into LoginResults here
	FText LoginResults;
	UpdateSharedProfilesComplete.ExecuteIfBound(true, LoginResults);
	UpdateSharedProfilesComplete.Unbind();
#endif
}

void UUTLocalPlayer::UpdateSharedProfiles(const FUTProfilesLoaded& Callback)
{
	UpdateSharedProfilesComplete = Callback;
	UpdateSharedProfiles();
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName XPProgress
*
* @Trigger Sent when a player gains XP
*
* @Type Sent by the Client
*
* @EventParam XP int64 New XP amount
*
* @Comments
*/
void UUTLocalPlayer::HandleProfileNotification(const FOnlineNotification& Notification)
{
	if (Notification.TypeStr == TEXT("XPProgress"))
	{
		FXPProgressNotifyPayload Payload;
		Notification.ParsePayload(Payload, Notification.TypeStr);
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
		if (PC != nullptr)
		{
			PC->XPBreakdown = FNewScoreXP(float(Payload.XP - Payload.PrevXP));
			int64 ProfileXP = 0.0f;
#if WITH_PROFILE
			if (GetMcpProfileManager())
			{
				GetMcpProfileManager()->RefreshActiveProfile(EUtMcpProfile::Profile, nullptr);
			}
#endif
		}

		

		if (FUTAnalytics::IsAvailable())
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("XP"), Payload.XP));
			FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerController), ParamArray, false);

			FUTAnalytics::GetProvider().RecordEvent(TEXT("XPProgress"), ParamArray);
		}
	}
	else if (Notification.TypeStr == TEXT("LevelUpReward"))
	{
#if WITH_PROFILE
		FLevelUpRewardNotifyPayload Payload;
		Notification.ParsePayload(Payload, Notification.TypeStr);
		const UUTProfileItem* ProfileItem = Cast<UUTProfileItem>(GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile)->GetItemTemplateObject(UUTProfileItem::StaticClass(), Payload.RewardID));
		if (ProfileItem != NULL)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
			if (PC != NULL)
			{
				PC->LevelRewards.Add(ProfileItem);
			}
			else
			{
				ShowToast(FText::Format(NSLOCTEXT("UTLocalPlayer", "GotItem", "Received item {0}"), ProfileItem->DisplayName));
			}
		}
#endif
	}
}

void UUTLocalPlayer::CachePassword(FString ServerGUID, FString Password)
{
	if (CachedPasswords.Contains(ServerGUID))
	{
		CachedPasswords[ServerGUID] = Password;
	}
	else
	{
		CachedPasswords.Add(ServerGUID, Password);
	}
}

FString UUTLocalPlayer::RetrievePassword(FString ServerGUID)
{
	if (CachedPasswords.Contains(ServerGUID))
	{
		return FString::Printf(TEXT("%s"), *CachedPasswords[ServerGUID]);
	}
	return TEXT("");
}

void UUTLocalPlayer::RemoveCachedPassword(const FString& ServerID)
{
	if (CachedPasswords.Contains(ServerID)) CachedPasswords.Remove(ServerID);
}

FString UUTLocalPlayer::StripOptionsFromAddress(FString HostAddress) const
{
	const TCHAR OptionCharacter = '?';

	int OptionStartIndex = 0;
	HostAddress.FindChar(OptionCharacter, OptionStartIndex);

	if (OptionStartIndex > 0)
	{
		return HostAddress.Left(OptionStartIndex);
	}

	return HostAddress;
}

void UUTLocalPlayer::Reconnect(bool bSpectator)
{
	if (!LastRankedMatchSessionId.IsEmpty())
	{
		UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UMatchmakingContext::StaticClass()));
		if (MatchmakingContext)
		{
			MatchmakingContext->AttemptReconnect(LastRankedMatchSessionId);
		}
		LastRankedMatchPlayerId.Empty();
		LastRankedMatchSessionId.Empty();
		LastRankedMatchTimeString.Empty();
		SaveConfig();
	}
	else if (!ReturnDestinationGuidString.IsEmpty())
	{
		AUTBasePlayerController* BasePlayerController = Cast<AUTBasePlayerController>(PlayerController);
		if (BasePlayerController != nullptr)
		{
			BasePlayerController->ConnectToServerViaGUID(ReturnDestinationGuidString, false);
		}
	}
	else if (LastSession.IsValid())
	{
		bAttemptingForceJoin = false;
		JoinSession(LastSession, bLastSessionWasASpectator);
	}
	else
	{
		FString Password = TEXT("?password=") + RetrievePassword(LastConnectToIP);
		ConsoleCommand(TEXT("open ") + LastConnectToIP + Password);
	}
}

void UUTLocalPlayer::EpicFlagCheck()
{
	if (CurrentProfileSettings != NULL && CommunityRole == EUnrealRoles::Developer && 
			GetCountryFlag() == FName(TEXT("Unreal")) && !CurrentProfileSettings->bForcedToEpicAtLeastOnce)
	{
		SetCountryFlagAndAvatar(FName(TEXT("Epic")), GetAvatar());
		CurrentProfileSettings->bForcedToEpicAtLeastOnce = true;
		SaveProfileSettings();
	}

	// Set the ranks/etc so the player card is right.
	AUTBasePlayerController* UTBasePlayer = Cast<AUTBasePlayerController>(PlayerController);
	if (UTBasePlayer != NULL)
	{
		UTBasePlayer->ServerReceiveStars(GetTotalChallengeStars());
		// TODO: should this be in BasePlayerController?
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(UTBasePlayer);
		if (UTPC != NULL)
		{
			UTPC->ServerReceiveCountryFlag(GetCountryFlag());
		}
		else
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTBasePlayer->PlayerState);
			if (UTPS != NULL)
			{
				UTPS->CountryFlag = GetCountryFlag();
			}
		}
	}
}

void UUTLocalPlayer::StartMatchmaking(int32 PlaylistId)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	UUTMatchmaking* Matchmaking = UTGameInstance->GetMatchmaking(); 
	if (ensure(Matchmaking))
	{
		Matchmaking->TimeMatchmakingStarted = GetWorld()->RealTimeSeconds;

		FMatchmakingParams MatchmakingParams;
		MatchmakingParams.ControllerId = GetControllerId();
		MatchmakingParams.StartWith = EMatchmakingStartLocation::Game;
		MatchmakingParams.PlaylistId = PlaylistId;

		if (GetProfileSettings() && !GetProfileSettings()->MatchmakingRegion.IsEmpty())
		{
			MatchmakingParams.DatacenterId = GetProfileSettings()->MatchmakingRegion;
		}
		else
		{
			MatchmakingParams.DatacenterId = TEXT("NA");
		}

		// Fix up USA to NA
		if (MatchmakingParams.DatacenterId == TEXT("USA"))
		{
			MatchmakingParams.DatacenterId = TEXT("NA");
		}

		UUTParty* Party = UTGameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PersistentParty = Cast<UUTPartyGameState>(Party->GetPersistentParty());
			if (PersistentParty)
			{
				PersistentParty->SetMatchmakingRegion(MatchmakingParams.DatacenterId);

				if (UTGameInstance && UTGameInstance->GetPlaylistManager())
				{
					int32 TeamCount = 0;
					int32 TeamSize = 0;
					int32 MaxPartySize = 0;
					if (UTGameInstance->GetPlaylistManager()->GetMaxTeamInfoForPlaylist(PlaylistId, TeamCount, TeamSize, MaxPartySize))
					{
						PersistentParty->SetPlayersNeeded((TeamSize * TeamCount) - PersistentParty->GetPartySize());
						PersistentParty->SetPartyMaxSize(MaxPartySize);
					}
				}
			}
		}

		if (UTGameInstance && UTGameInstance->GetPlaylistManager())
		{
			MatchmakingParams.bRanked = UTGameInstance->GetPlaylistManager()->IsPlaylistRanked(PlaylistId);
			MatchmakingParams.bSkipEloChecks = UTGameInstance->GetPlaylistManager()->ShouldPlaylistSkipElo(PlaylistId);
		}

		if (MatchmakingParams.bRanked)
		{
			MatchmakingParams.EloRange = RankedEloRange;
			MatchmakingParams.MinimumEloRangeBeforeHosting = RankedMinEloRangeBeforeHosting;
			MatchmakingParams.EloSearchStep = RankedMinEloSearchStep;
		}
		else
		{
			MatchmakingParams.EloRange = QMEloRange;
			MatchmakingParams.MinimumEloRangeBeforeHosting = QMMinEloRangeBeforeHosting;
			MatchmakingParams.EloSearchStep = QMMinEloSearchStep;
		}

		bool bSuccessfullyStarted = Matchmaking->FindGatheringSession(MatchmakingParams);

		if (bSuccessfullyStarted && FUTAnalytics::IsAvailable())
		{
			FUTAnalytics::FireEvent_UTMatchMakingStart(Cast<AUTPlayerController>(PlayerController), &MatchmakingParams);
		}
	}
}

void UUTLocalPlayer::AttemptMatchmakingReconnect(const FString& OldSessionId)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	UUTMatchmaking* Matchmaking = UTGameInstance->GetMatchmaking();
	if (ensure(Matchmaking))
	{
		FMatchmakingParams MatchmakingParams;
		MatchmakingParams.ControllerId = GetControllerId();
		MatchmakingParams.StartWith = EMatchmakingStartLocation::FindSingle;
		MatchmakingParams.SessionId = OldSessionId;
		MatchmakingParams.Flags = EMatchmakingFlags::NoReservation;

		MatchmakingReconnectResultHandle = Matchmaking->OnMatchmakingComplete().AddUObject(this, &ThisClass::AttemptMatchmakingReconnectResult);
		bool bSuccessfullyStarted = Matchmaking->FindSessionAsClient(MatchmakingParams);
	}
}

void UUTLocalPlayer::AttemptMatchmakingReconnectResult(EMatchmakingCompleteResult Result)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	UUTMatchmaking* Matchmaking = UTGameInstance->GetMatchmaking();
	if (ensure(Matchmaking))
	{
		Matchmaking->OnMatchmakingComplete().Remove(MatchmakingReconnectResultHandle);
	}

	HideMatchmakingDialog();

	if (Result == EMatchmakingCompleteResult::Success)
	{
		if (ensure(Matchmaking))
		{
			Matchmaking->TravelToServer();
		}
	}
	else if (Result != EMatchmakingCompleteResult::Cancelled)
	{
#if !UE_SERVER
		// Show error message about reconnect failing
		ShowMessage(NSLOCTEXT("UUTLocalPlayer", "FailedToReconnectTitle", "Failed To Reconnect"),
			NSLOCTEXT("UUTLocalPlayer", "FailedToReconnect", "Failed To reconnect matchmaking game. It is most likely already complete."),
			UTDIALOG_BUTTON_OK);
#endif
	}
}

bool UUTLocalPlayer::IsPartyLeader()
{
	UUTGameInstance* GameInstance = CastChecked<UUTGameInstance>(GetGameInstance());
	UUTParty* Party = GameInstance->GetParties();
	if (Party)
	{
		UPartyGameState* PersistentParty = Party->GetPersistentParty();
		if (PersistentParty)
		{
			TSharedPtr<const FUniqueNetId> PartyLeaderId = PersistentParty->GetPartyLeader();
			FUniqueNetIdRepl LocalPlayerId = GetGameAccountId();
			if (PartyLeaderId.IsValid() && *LocalPlayerId == *PartyLeaderId)
			{
				return true;
			}

			return false;
		}
	}

	return true;
}

void UUTLocalPlayer::ShowRegionSelectDialog(int32 InPlaylistId)
{
#if !UE_SERVER
	OpenDialog(
		SAssignNew(MatchmakingRegionDialog, SUTMatchmakingRegionDialog)
		.PlayerOwner(this)
		.DialogSize(FVector2D(0.6f, 0.4f))
		.DialogPosition(FVector2D(0.5f, 0.5f))
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "MatchmakingRegion", "Select A Region For Matchmaking"))
		.ButtonMask(UTDIALOG_BUTTON_OK)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::RegionSelectResult, InPlaylistId)));
#endif
}

#if !UE_SERVER
void UUTLocalPlayer::RegionSelectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID, int32 InPlaylistId)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UMatchmakingContext::StaticClass()));
		if (MatchmakingContext)
		{
			MatchmakingContext->StartMatchmaking(InPlaylistId);
		}
	}
}
#endif

void UUTLocalPlayer::ShowMatchmakingDialog()
{
#if !UE_SERVER
	if (MatchmakingDialog.IsValid())
	{
		return;
	}

	if (GameAbandonedDialog.IsValid())
	{
		CloseDialog(GameAbandonedDialog.ToSharedRef());
		GameAbandonedDialog.Reset();
	}

	if (LeagueMatchResultsDialog.IsValid())
	{
		CloseDialog(LeagueMatchResultsDialog.ToSharedRef());
		LeagueMatchResultsDialog.Reset();
	}

	OpenDialog(
		SAssignNew(MatchmakingDialog, SUTMatchmakingDialog)
		.PlayerOwner(this)
		.DialogSize(FVector2D(0.6f, 0.4f))
		.DialogPosition(FVector2D(0.5f, 0.5f))
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "MatchmakingSearch", "Matchmaking"))
		.ButtonMask(UTDIALOG_BUTTON_CANCEL)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::MatchmakingResult))
		);
#endif
}

void UUTLocalPlayer::HideMatchmakingDialog()
{
#if !UE_SERVER
	if (MatchmakingDialog.IsValid())
	{
		CloseDialog(MatchmakingDialog.ToSharedRef());
		MatchmakingDialog.Reset();
	}
#endif
}

#if !UE_SERVER
void UUTLocalPlayer::MatchmakingResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_CANCEL)
	{
		UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
		UUTMatchmaking* Matchmaking = UTGameInstance->GetMatchmaking();
		if (ensure(Matchmaking))
		{
			Matchmaking->CancelMatchmaking();
		}

		UUTParty* Party = UTGameInstance->GetParties();
		if (Party)
		{
			// Non party leader leaving should leave the party
			if (!IsPartyLeader())
			{
				Party->LeaveAndRestorePersistentParty();
			}
			else
			{
				Party->RestorePersistentPartyState();
			}
		}

		if (Matchmaking && FUTAnalytics::IsAvailable())
		{
			FUTAnalytics::FireEvent_UTMatchMakingCancelled(Cast<AUTPlayerController>(PlayerController), GetWorld()->RealTimeSeconds - Matchmaking->TimeMatchmakingStarted);
		}
	}

	MatchmakingDialog.Reset();
}
#endif

#if !UE_SERVER

void UUTLocalPlayer::VerifyChatWidget()
{
	if ( !ChatWidget.IsValid() )
	{
		SAssignNew(ChatWidget, SUTChatEditBox, this)
		.Style(SUTStyle::Get(), "UT.ChatEditBox")
		.MinDesiredWidth(500.0f)
		.MaxTextSize(MAX_CHAT_TEXT_SIZE)
		.OnConsoleKeyPressed(FUTChatEditConsoleKeyDelegate::CreateUObject(this, &UUTLocalPlayer::ChatWidgetConsoleKeyPressed));
	}

	if (ChatWidget.IsValid() && GetProfileSettings())
	{

		const FKeyConfigurationInfo* KeyConfig = GetProfileSettings()->FindGameAction(FName(TEXT("ShowConsole")));
		if (KeyConfig)
		{
			ChatWidget->ConsoleKeyName = KeyConfig->PrimaryKey.GetFName();
		}
	}
}

void UUTLocalPlayer::ChatWidgetConsoleKeyPressed()
{
	if (ViewportClient->ViewportConsole)
	{
		ViewportClient->ViewportConsole->FakeGotoState(FName(TEXT("Typing")));
	}

}

TSharedPtr<SUTChatEditBox> UUTLocalPlayer::GetChatWidget()
{
	VerifyChatWidget();
	return ChatWidget;

}

void UUTLocalPlayer::FocusWidget(TSharedPtr<SWidget> WidgetToFocus)
{
	FSlateApplication::Get().SetKeyboardFocus(WidgetToFocus, EKeyboardFocusCause::Keyboard);
	GetSlateOperations() = FReply::Handled().ReleaseMouseCapture().SetUserFocus(WidgetToFocus.ToSharedRef(), EFocusCause::SetDirectly);
}


#endif

FText UUTLocalPlayer::GetUIChatTextBackBuffer(int Direction)
{
	if (UIChatTextBackBuffer.Num() > 0)
	{
		UIChatTextBackBufferPosition = FMath::Clamp<int32>(UIChatTextBackBufferPosition + Direction, 0, UIChatTextBackBuffer.Num()-1);
		return UIChatTextBackBuffer[UIChatTextBackBufferPosition];
	}
	return FText::GetEmpty();
}

void UUTLocalPlayer::UpdateUIChatTextBackBuffer(const FText& NewText)
{
	if (!NewText.IsEmpty())
	{
		for (int32 i=0; i < UIChatTextBackBuffer.Num(); i++)
		{
			if (UIChatTextBackBuffer[i].EqualToCaseIgnored(NewText))
			{
				return;
			}
		}

		if (UIChatTextBackBuffer.Num() >= 30)	
		{
			UIChatTextBackBuffer.RemoveAt(0);
		}

		UIChatTextBackBuffer.Add(NewText);
		UIChatTextBackBufferPosition = UIChatTextBackBuffer.Num();
	}
}

void UUTLocalPlayer::ShowQuickChat(FName ChatDestination)
{
#if !UE_SERVER
	if (!QuickChatWindow.IsValid())
	{
		SAssignNew(QuickChatWindow, SUTQuickChatWindow, this)
			.InitialChatDestination(ChatDestination);

		if (QuickChatWindow.IsValid())
		{
			OpenWindow(QuickChatWindow);
			FSlateApplication::Get().SetAllUserFocus(QuickChatWindow.ToSharedRef(), EKeyboardFocusCause::SetDirectly);
			FSlateApplication::Get().SetKeyboardFocus(QuickChatWindow);
		}
	}
#endif
}

void UUTLocalPlayer::CloseQuickChat()
{
#if !UE_SERVER
	if (QuickChatWindow.IsValid())
	{
		CloseWindow(QuickChatWindow);
		QuickChatWindow.Reset();
	}
#endif
}

#if !UE_SERVER
TSharedPtr<SUTQuickChatWindow> UUTLocalPlayer::GetQuickChatWidget()
{
	return QuickChatWindow;
}
#endif

void UUTLocalPlayer::OnPlayerTalkingStateChanged(TSharedRef<const FUniqueNetId> TalkerId, bool bIsTalking)
{
	if (GetWorld())
	{
		AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
		if (UTGameState)
		{
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				if (UTGameState->PlayerArray[i] && UTGameState->PlayerArray[i]->UniqueId.ToString() == TalkerId->ToString())
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					if (PS) PS->bIsTalking = bIsTalking;
				}
			}
		}
	}
}

bool UUTLocalPlayer::GetMMREntry(const FString& RatingName, FMMREntry& OutMMRRating)
{
	if (MMREntries.Contains(RatingName))
	{
		OutMMRRating = MMREntries[RatingName];
		return true;
	}

	return false;
}

void UUTLocalPlayer::UpdateMMREntry(const FString& RatingName, const FMMREntry& InMMRRating)
{
	if (MMREntries.Contains(RatingName))
	{
		MMREntries[RatingName] = InMMRRating;
	}
	else
	{
		MMREntries.Add(RatingName, InMMRRating);
	}
}

bool UUTLocalPlayer::GetLeagueProgress(const FString& LeagueName, FRankedLeagueProgress& OutLeagueProgress)
{
	if (LeagueRecords.Contains(LeagueName))
	{
		OutLeagueProgress = LeagueRecords[LeagueName];
		return true;
	}

	return false;
}

void UUTLocalPlayer::UpdateLeagueProgress(const FString& LeagueName, const FRankedLeagueProgress& InLeagueProgress)
{
	if (LeagueRecords.Contains(LeagueName))
	{
		LeagueRecords[LeagueName] = InLeagueProgress;
	}
	else
	{
		LeagueRecords.Add(LeagueName, InLeagueProgress);
	}
}


bool UUTLocalPlayer::HasChatText()
{
#if !UE_SERVER
	VerifyChatWidget();
	if (!ChatWidget->GetText().IsEmpty())
	{
		return true;
	}
#endif
	return  false;
}

void UUTLocalPlayer::LoadLocalProfileSettings()
{
	FString Path = FPaths::GameSavedDir() + GetProfileFilename() + TEXT(".local");

	// We only load the local profile if there isn't a profile.  It will be overwritten upon login
	if (CurrentProfileSettings == NULL)
	{
		CurrentProfileSettings = NewObject<UUTProfileSettings>(GetTransientPackage(), UUTProfileSettings::StaticClass());

		TArray<uint8> FileContents;
		if ( FFileHelper::LoadFileToArray(FileContents, *Path) )
		{

			// Serialize the object
			FMemoryReader MemoryReader(FileContents, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);

			// FObjectAndNameAsStringProxyArchive does not have versioning, but we need it
			// In 4.12, Serialization has been modified and we need the FArchive to use the right serialization path
			uint32 PossibleMagicNumber;
			Ar << PossibleMagicNumber;
			if (CloudProfileMagicNumberVersion1 == PossibleMagicNumber)
			{
				int32 CloudProfileUE4Ver;
				Ar << CloudProfileUE4Ver;
				Ar.SetUE4Ver(CloudProfileUE4Ver);
			}
			else
			{
				// Very likely this is from an unversioned cloud profile, set it to the last released serialization version
				Ar.SetUE4Ver(CloudProfileUE4VerForUnversionedProfile);
				// Rewind to the beginning as the magic number was not in the archive
				Ar.Seek(0);
			}

			CurrentProfileSettings->Serialize(Ar);
			CurrentProfileSettings->VersionFixup();
		}
		else
		{
			CurrentProfileSettings->ResetProfile(EProfileResetType::All);
		}
		CurrentProfileSettings->ApplyAllSettings(this);
	}
}
void UUTLocalPlayer::SaveLocalProfileSettings()
{
	FString Path = FPaths::GameSavedDir() + GetProfileFilename() + TEXT(".local");

	// Build a blob of the profile contents
	TArray<uint8> FileContents;
	FMemoryWriter MemoryWriter(FileContents, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	Ar << CloudProfileMagicNumberVersion1;
	int32 UE4Ver = Ar.UE4Ver();
	Ar << UE4Ver;

	CurrentProfileSettings->Serialize(Ar);
	FFileHelper::SaveArrayToFile(FileContents, *Path);
}

#if !UE_SERVER
void UUTLocalPlayer::OnUpgradeResults(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		FString URL = TEXT("com.epicgames.launcher://ut");
		FString Command = TEXT("");
		FString Error = TEXT("");
		FPlatformProcess::LaunchURL(*URL, *Command, &Error);
		FPlatformMisc::RequestExit( 0 );
	}
}
#endif

void UUTLocalPlayer::ClearPendingLoginUserName()
{
	PendingLoginUserName = TEXT("");
}

void UUTLocalPlayer::LoginProcessComplete()
{
#if !UE_SERVER
	// Close down the login dialog if it's open.
	if (LoginDialog.IsValid())
	{
		LoginDialog->EndLogin(true);
	}
#endif

	if (IsLoggedIn())
	{
		FText WelcomeToast = FText::Format(NSLOCTEXT("MCP","MCPWelcomeBack","Welcome back {0}"), FText::FromString(*GetOnlinePlayerNickname()));
		ShowToast(WelcomeToast);
		
		// If we have a pending session, then join it.
		JoinPendingSession();

		TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		ShowRankedReconnectDialog(UserId->ToString());

		FinalizeLogin();

	}
}

void UUTLocalPlayer::FinalizeLogin()
{
	LoginPhase = ELoginPhase::LoggedIn;
	if (bLaunchTutorialOnLogin) LaunchTutorial(ETutorialTags::TUTTAG_NewPlayerLaunchTutorial);
}

void UUTLocalPlayer::QoSComplete()
{
	FString MatchmakingRegion = FQosModule::Get().GetQosInterface()->GetRegionId();
	UE_LOG(UT, Log, TEXT("QoS complete, recommended region %s"), *MatchmakingRegion);
	if (CurrentProfileSettings)
	{
		if (MatchmakingRegion == TEXT("EU") || MatchmakingRegion == TEXT("NA"))
		{
			CurrentProfileSettings->MatchmakingRegion = MatchmakingRegion;
			SaveProfileSettings();
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("QoS complete, but invalid region %s"), *MatchmakingRegion);
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Matchmaking region chosen, but no profile settings to apply"));
	}
}

void UUTLocalPlayer::SetTutorialFinished(int32 TutorialMask)
{
	for (int32 i=0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Mask == TutorialMask)
		{
			SetTutorialFinished(TutorialData[i].Tag);
			break;
		}
	}
}

void UUTLocalPlayer::SetTutorialFinished(FName TutorialTag)
{
	for (int32 i=0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == TutorialTag)
		{
			int32 TutorialMask = TutorialData[i].Mask;
			if ( !IsTutorialMaskCompleted(TutorialMask) )
			{
				SetTutorialMask(TutorialMask);

				// Look to see if it's time to give a toast.  We need a better type of achievement toast.  Maybe play UMG here or something

				if (TutorialMask == 0x01)	// We have completed the first tutorial...
				{
					ShowToast(NSLOCTEXT("Unlocks","FirstTimer","Achievement: First Timer!"),6.0f);			
				}
				else if (TutorialMask <= TUTORIAL_Pickups && IsTutorialMaskCompleted(TUTORIAL_SkillMoves) )
				{
					ShowToast(NSLOCTEXT("Unlocks","OfflineChallengesUnlocked","Achievement: Got the Skills"),6.0f);			
				}

				if (TutorialMask > TUTORIAL_Pickups)
				{
					if (TutorialMask == TUTORIAL_DM)
					{
						ShowToast(NSLOCTEXT("Unlocks","DMQuickMatchUnlockedDM","Achievement: Fragger\nDeathmatch Quickplay Unlocked!"),6.0f);			
					}
					if (TutorialMask == TUTORIAL_FlagRun)
					{
						ShowToast(NSLOCTEXT("Unlocks","DMQuickMatchUnlockedFR","Achievement: Running Man\nBlitz Quickplay Unlocked!"),6.0f);			
					}
					if (TutorialMask == TUTORIAL_Showdown)
					{
						ShowToast(NSLOCTEXT("Unlocks","DMQuickMatchUnlockedSD","Achievement: Mano E' Mano\nShowdown Quickplay Unlocked!"),6.0f);			
					}
					if (TutorialMask == TUTORIAL_CTF)
					{
						ShowToast(NSLOCTEXT("Unlocks","CTFQuickMatchUnlocked","Achievement: Flag Runner\nCapture the Flag Quickplay Unlocked!"),6.0f);			
					}
				}
			}
			break;
		}
	}
}

bool UUTLocalPlayer::IsSystemMenuOpen()
{
#if !UE_SERVER
	return DesktopSlateWidget.IsValid();
#endif

	return false;
}

void UUTLocalPlayer::InitializeSocial()
{

#if WITH_SOCIAL
	// Init the Friends And Chat system
	FFriendsAndChatLoginOptions LoginOptions(0, true);
	ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->Login(OnlineSubsystem, LoginOptions);
	ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->SetAnalyticsProvider(FUTAnalytics::GetProviderPtr());

	if (!ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnJoinGame().IsBoundToObject(this))
 	{
		ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnJoinGame().AddUObject(this, &UUTLocalPlayer::HandleFriendsJoinGame);
 	}

	// PLK - find replacement for this API
	/*
	if (!ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->AllowFriendsJoinGame().IsBoundToObject(this))
	{
		ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->AllowFriendsJoinGame().BindUObject(this, &UUTLocalPlayer::AllowFriendsJoinGame);
	}*/

	if (!ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnNotificationsAvailable().IsBoundToObject(this))
 	{
 		ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnNotificationsAvailable().AddUObject(this, &UUTLocalPlayer::HandleFriendsNotificationAvail);
 	}
	if (!ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnSendNotification().IsBoundToObject(this))
 	{
		ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->GetNotificationService()->OnSendNotification().AddUObject(this, &UUTLocalPlayer::HandleFriendsActionNotification);
 	}
	
#endif
		
	if (GetGameInstance())
	{
		GetGameInstance()->GetTimerManager().SetTimer(SocialInitializationTimerHandle, this, &UUTLocalPlayer::SocialInitialized, 0.25f, true);
	}
}

void UUTLocalPlayer::SocialInitialized()
{
#if !UE_SERVER

#if WITH_SOCIAL
	if (!ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->IsLoggedIn())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(SocialInitializationTimerHandle);
#endif

	// Make sure popup is created so we dont lose any messages
	GetFriendsPopup();
#endif

	PushChallengeStarsToMCP();

	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		if (LoginPhase != ELoginPhase::LoggedIn)
		{
			LoginPhase = ELoginPhase::GettingTitleUpdate;
		}

		// Grab the title files from the MCP
		UTGameInstance->AcquireTitleFilesFromMCP();
	}
}

void UUTLocalPlayer::PushChallengeStarsToMCP()
{
	// Check to see if the star counts line up.

#if WITH_PROFILE
	UUtMcpProfile* Profile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (Profile != NULL && UTEngine && UTEngine->GetChallengeManager().IsValid())
	{

		TWeakObjectPtr<UUTChallengeManager> ChallengeManager = UTEngine->GetChallengeManager();
		TArray<const FUTChallengeInfo*> Challenges;

		ChallengeManager->GetChallenges(Challenges, EChallengeFilterType::All, GetProgressionStorage());

		int32 GoldStars = 0;
		int32 BlueStars = 0;

		for (int32 i = 0 ; i < Challenges.Num(); i++)
		{
			const FUTChallengeInfo* Challenge = Challenges[i];

			// Track it's overall star count....

			if (Challenge->RewardTag == NAME_REWARD_GoldStars)
			{
				GoldStars += GetChallengeStars(Challenge->Tag);
			}
			else if (Challenge->RewardTag == NAME_REWARD_BlueStars)
			{
				BlueStars += GetChallengeStars(Challenge->Tag);
			}
		}

		if (Profile->GetChallengeStars( NAME_REWARD_GoldStars) != GoldStars || Profile->GetChallengeStars(NAME_REWARD_BlueStars) != BlueStars )
		{
			Profile->SetStars(GoldStars,BlueStars);
		}
	}
#endif
}


bool UUTLocalPlayer::SkipTutorialCheck()
{
#if WITH_PROFILE
	UUtMcpProfile* Profile = GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
	if (Profile != NULL && Profile->GetXP() > 0)
	{
		return true;
	}
#endif

	if (LastSession.Session.OwningUserId.IsValid() || LastSession.Session.SessionInfo.IsValid())
	{
		return true;
	}

	// Add any additional checks here to abort the tutorial check

	return false;
}

void UUTLocalPlayer::ShowRedirectDownload()
{
#if !UE_SERVER
	if (!bSuppressDownloadDialog && !DownloadAllDialog.IsValid())
	{
		SAssignNew(DownloadAllDialog, SUTDownloadAllDialog)
			.PlayerOwner(this);

		if (DownloadAllDialog.IsValid())
		{
			OpenDialog(DownloadAllDialog.ToSharedRef(),240);
		}
	}
#endif
}

void UUTLocalPlayer::HideRedirectDownload()
{
#if !UE_SERVER
	if (DownloadAllDialog.IsValid())
	{
		CloseDialog(DownloadAllDialog.ToSharedRef());
		DownloadAllDialog.Reset();
	}
#endif
}

bool UUTLocalPlayer::IsInAnActiveParty() const
{
	if (GetWorld())
	{
		UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UPartyContext::StaticClass()));
		if (PartyContext)
		{
			return PartyContext->GetPartySize() > 1;
		}
	}
	
	return false;
}

bool UUTLocalPlayer::IsMenuOptionLocked(FName MenuCommand) const
{
	return false;
}

EVisibility UUTLocalPlayer::IsMenuOptionLockVisible(FName MenuCommand) const
{
	return IsMenuOptionLocked(MenuCommand) ? EVisibility::Visible : EVisibility::Collapsed;
}

bool UUTLocalPlayer::IsMenuOptionEnabled(FName MenuCommand) const
{
	return IsMenuOptionLocked(MenuCommand);

}

FText UUTLocalPlayer::GetMenuCommandTooltipText(FName MenuCommand) const 
{
	if (MenuCommand == EMenuCommand::MC_QuickPlayDM)			return NSLOCTEXT("SUTHomePanel", "QuickPlayDM","Join an online deathmatch game against players close to your skill level.");
	else if (MenuCommand == EMenuCommand::MC_QuickPlayCTF)		return NSLOCTEXT("SUTHomePanel", "QuickPlayCTF","Join an online capture the flag game against players close to your skill level");
	else if (MenuCommand == EMenuCommand::MC_QuickPlayFlagrun)	return NSLOCTEXT("SUTHomePanel", "QuickPlayFlagrun","Join an online Blitz game against players close to your skill level.");
	else if (MenuCommand == EMenuCommand::MC_QuickPlayShowdown)	return NSLOCTEXT("SUTHomePanel", "QuickPlayFlagrunPVE","Join an online co-op Blitz game against AI opponents.");
	else if (MenuCommand == EMenuCommand::MC_Challenges)		return NSLOCTEXT("SUTHomePanel", "QuickPlayChallenges","Test your skills against bots in challenge matches and earn stars.");
	else if (MenuCommand == EMenuCommand::MC_FindAMatch)		return NSLOCTEXT("SUTHomePanel", "QuickPlayFindAMatch","Head online and find games to play.");
	else if (MenuCommand == EMenuCommand::MC_Tutorial)		return NSLOCTEXT("SUTHomePanel", "BasicTrainingPanel", "Improve your skills and learn new tactics in Basic Training.");
	else if (MenuCommand == EMenuCommand::MC_InstantAction)		return NSLOCTEXT("SUTHomePanel", "InstantActionPanel", "Customize your own single player match experience vs. bots.");
	return FText::GetEmpty();
}

void UUTLocalPlayer::LaunchTutorial(FName TutorialName)
{
	LastTutorial = TutorialName;

	if (TutorialName == ETutorialTags::TUTTAG_Progress)
	{
		// Look to see where the player is in the tutorial progression and pick the next tutorial.

		if ( !IsTutorialMaskCompleted(TUTOIRAL_Weapon) )			TutorialName = ETutorialTags::TUTTAG_Weapons;
		else if (!IsTutorialMaskCompleted(TUTORIAL_Pickups) )	TutorialName = ETutorialTags::TUTTAG_Pickups;
		else if (!IsTutorialMaskCompleted(TUTORIAL_DM) )			TutorialName = ETutorialTags::TUTTAG_DM;
		else if (!IsTutorialMaskCompleted(TUTORIAL_FlagRun) )	TutorialName = ETutorialTags::TUTTAG_Flagrun;
		else if (!IsTutorialMaskCompleted(TUTORIAL_Showdown) )	TutorialName = ETutorialTags::TUTTAG_Showdown;
		else if (!IsTutorialMaskCompleted(TUTORIAL_CTF) )		TutorialName = ETutorialTags::TUTTAG_CTF;
		else if (!IsTutorialMaskCompleted(TUTORIAL_Duel) )		TutorialName = ETutorialTags::TUTTAG_Duel;

		else TutorialName =	ETutorialTags::TUTTAG_Movement;
	}

	for (int32 i = 0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == TutorialName)
		{
			UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
			if (GI)
			{
				GI->LoadingMovieToPlay = TutorialData[i].LoadingMovie;
				GI->LevelLoadText = FText::FromString(TutorialData[i].LoadingText);
				GI->bSuppressLoadingText = true;
			}

			FString URL = TutorialData[i].Map + TutorialData[i].LaunchArgs + FString::Printf(TEXT("?TutorialMask=%i"),TutorialData[i].Mask);
			if (bLaunchTutorialOnLogin)
			{
				URL += "?NoTutMenu=1";
			}
			GetWorld()->ServerTravel(URL,true,false);
			return;
		}
	}
}

bool UUTLocalPlayer::LaunchPendingQuickmatch()
{
	if (bQuickmatchOnLevelChange)
	{
#if !UE_SERVER
		StartQuickMatch(PendingQuickmatchType);
#endif
		return true;
	}
	return false;
}

void UUTLocalPlayer::RestartTutorial()
{
	if (LastTutorial != NAME_None)
	{
		LaunchTutorial(LastTutorial);
	}
}

void UUTLocalPlayer::NextTutorial()
{
	for (int32 i = 0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == LastTutorial)
		{
			int32 NextTutorialIndex = (i + 1) % TutorialData.Num();
			if (TutorialData.IsValidIndex(NextTutorialIndex))
			{
				LaunchTutorial(TutorialData[NextTutorialIndex].Tag);
				break;
			}
		}
	}
}

void UUTLocalPlayer::PrevTutorial()
{
	for (int32 i = 0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == LastTutorial)
		{
			int32 PrevTutorialIndex = (i - 1) % TutorialData.Num();
			if (TutorialData.IsValidIndex(PrevTutorialIndex))
			{
				LaunchTutorial(TutorialData[PrevTutorialIndex].Tag);
				break;
			}
		}
	}
}


FText UUTLocalPlayer::GetNextTutorialName()
{
	for (int32 i = 0; i < TutorialData.Num()-1; i++)
	{
		if (TutorialData[i].Tag == LastTutorial)
		{
			return FText::FromString(TutorialData[i+1].LoadingText);
		}
	}
	return FText::GetEmpty();
}

FText UUTLocalPlayer::GetPrevTutorialName()
{
	for (int32 i = 1; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == LastTutorial)
		{
			return FText::FromString(TutorialData[i-1].LoadingText);
		}
	}
	return FText::GetEmpty();
}




FText UUTLocalPlayer::GetTutorialSectionText(TEnumAsByte<ETutorialSections::Type> Section) const
{
	if (Section == ETutorialSections::SkillMoves)
	{
		if ( IsTutorialMaskCompleted(TUTORIAL_SkillMoves) )
		{
			return NSLOCTEXT("TutorialText","Completed","!! COMPLETED !!");
		}
		
		return NSLOCTEXT("TutorialText","SkillMoves","Learn the basic skills needed to compete in the tournament.");
	}

	if (Section == ETutorialSections::Gameplay)
	{
		if ( IsTutorialMaskCompleted(TUTORIAL_Gameplay) )
		{
			return NSLOCTEXT("TutorialText","Completed","!! COMPLETED !!");
		}
		
		return NSLOCTEXT("TutorialText","Gameplay","Learn how to play the most popular game modes before you play online.");
	}

	if (Section == ETutorialSections::Hardcore)
	{
		if ( IsTutorialMaskCompleted(TUTORIAL_Hardcore) )
		{
			return NSLOCTEXT("TutorialText","Completed","!! COMPLETED !!");
		}
		return NSLOCTEXT("TutorialText","Classic","Classic game modes that have stood the test of time.");
	}

	return FText::GetEmpty();
}

bool UUTLocalPlayer::IsTutorialMaskCompleted(int32 TutorialMask) const
{
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("skiptutcheck")))
	{
		return true;
	}
#endif

	return ( GetTutorialMask() & TutorialMask ) == TutorialMask;
}


bool UUTLocalPlayer::IsTutorialCompleted(FName TutorialName) const
{
	bool bCompleted = false;
	for (int32 i=0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == TutorialName)
		{
			bCompleted = IsTutorialMaskCompleted(TutorialData[i].Mask);
			break;
		}
	}
	return bCompleted;
}

FText UUTLocalPlayer::GetBestTutorialTime(FName TutorialName)
{
	float BestTime = 0.0f;
	for (int32 i=0; i < TutorialData.Num(); i++)
	{
		if (TutorialData[i].Tag == TutorialName)
		{
			if (CurrentProgression)
			{
				CurrentProgression->GetBestTime(TutorialName, BestTime);
			}
			break;
		}
	}

	return UUTGameEngine::ConvertTime(FText::GetEmpty(), FText::GetEmpty(), BestTime, true, true, true);
}


FReply UUTLocalPlayer::ToggleFriendsAndChat()
{
#if PLATFORM_LINUX
	// Need launcher so this doesn't work on linux right now
	return FReply::Handled();
#endif
#if !UE_SERVER
	if (bShowingFriendsMenu)
	{
		TSharedPtr<SUTFriendsPopupWindow> Popup = GetFriendsPopup();
		if (Popup.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(Popup.ToSharedRef());
			SetShowingFriendsPopup(false);
		}
	}
	else
	{
		TSharedPtr<SUTFriendsPopupWindow> Popup = GetFriendsPopup();
		if (Popup.IsValid())
		{
			GEngine->GameViewport->AddViewportWidgetContent(Popup.ToSharedRef(), 6000);
			SetShowingFriendsPopup(true);
		}
	}
#endif
	return FReply::Handled();
}


bool UUTLocalPlayer::HasProgressionKey(FName RequiredKey)
{
	// Eventually, this key list will be pulled from the MCP or somewhere.  But until them, here is a big if/then/else block :)

	if (RequiredKey == FName(TEXT("PROGRESSION_KEY_NoLongerANoob")))
	{
		return IsTutorialMaskCompleted(TUTORIAL_Movement);
	}

	return false;
}

bool UUTLocalPlayer::HasProgressionKeys(TArray<FName> RequiredKeys)
{
	for (int32 i = 0; i < RequiredKeys.Num(); i++)
	{
		if (!HasProgressionKey(RequiredKeys[i]))
		{
			return false;
		}
	}

	return true;
}

void UUTLocalPlayer::ShowWebMessage(FText Caption, FString Url)
{
#if !UE_SERVER
	if (!WebMessageDialog.IsValid())
	{
		SAssignNew(WebMessageDialog, SUTWebMessage)
			.PlayerOwner(this);

		OpenDialog(WebMessageDialog.ToSharedRef(), 2000);
		WebMessageDialog->Browse(Caption, Url);
	}
#endif
}

void UUTLocalPlayer::CloseWebMessage()
{
#if !UE_SERVER
	if (WebMessageDialog.IsValid())
	{
		CloseDialog(WebMessageDialog.ToSharedRef());
		WebMessageDialog.Reset();
	}
#endif
}

void UUTLocalPlayer::ReportAbuse(TWeakObjectPtr<class AUTPlayerState> Troll)
{
#if !UE_SERVER
	if (!AbuseDialog.IsValid())
	{
		SAssignNew(AbuseDialog, SUTReportUserDialog)
			.PlayerOwner(this)
			.Troll(Troll);

		OpenDialog(AbuseDialog.ToSharedRef(), 1000);
	}
#endif
}
void UUTLocalPlayer::CloseAbuseDialog()
{
#if !UE_SERVER
	if (AbuseDialog.IsValid())
	{
		CloseDialog(AbuseDialog.ToSharedRef());
		AbuseDialog.Reset();
	}
#endif
}

void UUTLocalPlayer::ShowSavingWidget()
{
#if !UE_SERVER
	if (SavingWidget == nullptr)
	{
		SavingWidget = OpenUMGWidget(TEXT("/Game/RestrictedAssets/UI/UMGMenuElements/UTSavingWidget.UTSavingWidget"));
	}
#endif
}

void UUTLocalPlayer::HideSavingWidget()
{
	if (SavingWidget != nullptr && SavingMask == 0x00)
	{
		SavingWidget->SimpleEvent(FName(TEXT("Close")));
	}
}

void UUTLocalPlayer::CloseSavingWidget()
{
#if !UE_SERVER
	if (SavingWidget != nullptr)
	{
		CloseUMGWidget(SavingWidget);
		SavingWidget = nullptr;
		SavingMask = 0x00;
	}
#endif
}


UUTUMGWidget* UUTLocalPlayer::OpenUMGWidget(const FString& UMGClass)
{
	UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
	if (GI != nullptr) 
	{
		// Attempt to look up the class as given
		UClass* UMGWidgetClass = LoadClass<UUserWidget>(NULL, *UMGClass, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);

		// If that failed, try adding the _C definition incase someone forgot it
		if (UMGWidgetClass == nullptr)
		{
			FString FixedClassName = UMGClass + TEXT("_C");
			UMGWidgetClass = LoadClass<UUserWidget>(NULL, *FixedClassName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		}

		if (UMGWidgetClass)
		{
			UUTUMGWidget* FinalWidget = CreateWidget<UUTUMGWidget>(GI, UMGWidgetClass);
			if (FinalWidget != nullptr)
			{
				return OpenExistingUMGWidget(FinalWidget);
			}
		}
	}
	return nullptr;
}

UUTUMGWidget* UUTLocalPlayer::OpenExistingUMGWidget(UUTUMGWidget* WidgetToOpen)
{
	if (WidgetToOpen != nullptr)
	{
		int32 Cnt = 0;
		for (int32 i=0; i < UMGWidgetStack.Num(); i++)
		{	
			if (UMGWidgetStack[i]->GetClass() == WidgetToOpen->GetClass())
			{
				Cnt++;
			}
		}

		if (WidgetToOpen->bUniqueUMG && Cnt > 0) return nullptr;	// Don't add widgets if they are unique and already exist.

		// If there are multiples of this widget open, offset them.
		GEngine->GameViewport->AddViewportWidgetContent(WidgetToOpen->TakeWidget(), WidgetToOpen->DisplayZOrder - Cnt);
		
		// Clamp the stack.
		Cnt = FMath::Clamp<int32>(Cnt,0,4);
		if (Cnt > 0)
		{
			WidgetToOpen->SetPositionInViewport(WidgetToOpen->StackingOffset * Cnt,true);
		}

		if (WidgetToOpen->WidgetTag == NAME_None)
		{
			WidgetToOpen->WidgetTag = FName(*WidgetToOpen->GetName());
		}

		WidgetToOpen->AssociateLocalPlayer(this);
		WidgetToOpen->WidgetOpened();

		UMGWidgetStack.Add(WidgetToOpen);

	}
	return WidgetToOpen;
}

UUTUMGWidget* UUTLocalPlayer::FindUMGWidget(const FName SearchTag)
{

	for (int32 i=0; i < UMGWidgetStack.Num(); i++)
	{
		if (UMGWidgetStack[i] != nullptr && UMGWidgetStack[i]->WidgetTag == SearchTag)
		{
			return UMGWidgetStack[i];
		}
	}

	return nullptr;
}

void UUTLocalPlayer::CloseUMGWidgetByTag(const FName Tag)
{
	UUTUMGWidget* WidgetToClose = FindUMGWidget(Tag);	
	if (WidgetToClose != nullptr)
	{
		CloseUMGWidget(WidgetToClose);
	}
}

void UUTLocalPlayer::CloseUMGWidget(UUTUMGWidget* WidgetToClose)
{
	if (WidgetToClose != nullptr)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(WidgetToClose->TakeWidget());
		WidgetToClose->WidgetClosed();

		UMGWidgetStack.Remove(WidgetToClose);
	}
}

int32 UUTLocalPlayer::GetTutorialMask() const
{
	int32 TutorialMask = 0x00;
	if (CurrentProfileSettings)
	{
		TutorialMask |= CurrentProfileSettings->TutorialMask;
	}
	if (CurrentProgression)
	{
		TutorialMask |= CurrentProgression->TutorialMask;
	}

	return TutorialMask;
}

void UUTLocalPlayer::SetTutorialMask(int32 BitToSet)
{
	int32 TutorialMask = GetTutorialMask();
	TutorialMask = TutorialMask | BitToSet;

	if (CurrentProgression)
	{
		CurrentProgression->TutorialMask = TutorialMask;
	}
	if (CurrentProfileSettings)
	{
		CurrentProfileSettings->TutorialMask = TutorialMask;
	}

	SaveProfileSettings();
	SaveProgression();
}

void UUTLocalPlayer::ClearTutorialMask(int32 BitToClear)
{
	int32 TutorialMask = GetTutorialMask();
	TutorialMask = TutorialMask & (~BitToClear);
	
	if (CurrentProgression)
	{
		CurrentProgression->TutorialMask = TutorialMask;
	}
	if (CurrentProfileSettings)
	{
		CurrentProfileSettings->TutorialMask = TutorialMask;
	}

	SaveProfileSettings();
	SaveProgression();

}

void UUTLocalPlayer::RemoveCosmeticsFromDefaultURL()
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	if (DefaultURL.HasOption(TEXT("Hat")) ||
		DefaultURL.HasOption(TEXT("LeaderHat")) ||
		DefaultURL.HasOption(TEXT("Eyewear")) ||
		DefaultURL.HasOption(TEXT("GroupTaunt")) ||
		DefaultURL.HasOption(TEXT("Taunt")) ||
		DefaultURL.HasOption(TEXT("Taunt2")))
	{
		ClearDefaultURLOption(TEXT("Hat"));
		ClearDefaultURLOption(TEXT("LeaderHat"));
		ClearDefaultURLOption(TEXT("Eyewear"));
		ClearDefaultURLOption(TEXT("GroupTaunt"));
		ClearDefaultURLOption(TEXT("Taunt"));
		ClearDefaultURLOption(TEXT("Taunt2"));
		ClearDefaultURLOption(TEXT("HatVar"));
		ClearDefaultURLOption(TEXT("EyewearVar"));
	}
}

FSceneView* UUTLocalPlayer::CalcSceneView(class FSceneViewFamily* ViewFamily, FVector& OutViewLocation, FRotator& OutViewRotation, FViewport* Viewport, class FViewElementDrawer* ViewDrawer, EStereoscopicPass StereoPass)
{
	if (PlayerController != nullptr)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerController);
		if (UTPC == nullptr)
		{
			AActor* ActorPlayerController = Cast<AActor>(PlayerController);
			if (ActorPlayerController->GetActorLocation().IsNearlyZero())
			{
				AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
				if (WS)
				{
					FVector CamLoc(0.f);
					FRotator CamRot(0.f);
					if (WS->GetLoadingCameraPosition(CamLoc, CamRot))
					{
						ActorPlayerController->SetActorLocationAndRotation(CamLoc, CamRot);
						PlayerController->SetControlRotation(CamRot);
					}
				}
			}
		}
	}

	return Super::CalcSceneView(ViewFamily, OutViewLocation, OutViewRotation, Viewport, ViewDrawer, StereoPass);
}

void UUTLocalPlayer::CheckForNewUpdate()
{
	if (LoginPhase == ELoginPhase::LoggedIn)
	{
		UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
		if (GI)
		{
			GI->CheckForNewUpdate();
		}
	}
}

FString UUTLocalPlayer::GetBuildNotesURL()
{
	if (MCPPulledData.bValid && !MCPPulledData.BuildNotesURL.IsEmpty())
	{
		return MCPPulledData.BuildNotesURL;
	}

	return TEXT("https://www.epicgames.com/unrealtournament/");
	
}

void UUTLocalPlayer::UpdateCheck()
{
#if !UE_SERVER

	int32 MyVersion = (int32) FNetworkVersion::GetLocalNetworkVersion();

	if (MCPPulledData.CurrentVersionNumber > MyVersion )
	{
		ShowWebMessage(NSLOCTEXT("UTLocalPlayer","NeedtoUpdateTitle","New Version Available"), GetBuildNotesURL());
	}
	else if (IsMenuGame())
	{
		if (MCPPulledData.CurrentVersionNumber > LastLoadedVersionNumber)
		{
			// Delete the web cache to insure the new version is loaded.  CEFBrowser should give you a LoadURLIgnoringCache but it doesn't
			FString WebCacheIndex = FPaths::GameSavedDir() + TEXT("/webcache/index");
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*WebCacheIndex);

			
			// Open a Web page with better info
			ShowWebMessage(NSLOCTEXT("UTLocalPlayer","ThanksForUpdating","New Features"), GetBuildNotesURL());
					
			LastLoadedVersionNumber = MCPPulledData.CurrentVersionNumber;
			SaveConfig();
		}
	}
#endif
}

void UUTLocalPlayer::CreateNewCustomMatch(ECreateInstanceTypes::Type InstanceType, const FString& GameMode, const FString& StartingMap, const FString& Description, const TArray<FString>& GameOptions,  int32 DesiredPlayerCount, bool bTeamGame, bool bRankLocked, bool bSpectatable, bool _bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled)
{
	AUTReplicatedGameRuleset* CustomRuleset = AUTBaseGameMode::CreateCustomReplicateGameRuleset(GetWorld(), GetWorld()->GetGameState<AUTGameState>(), GameMode, StartingMap, Description, GameOptions, DesiredPlayerCount, bTeamGame);

	if (CustomRuleset != nullptr)
	{
		return CreateNewMatch(InstanceType, CustomRuleset, StartingMap, bRankLocked, bSpectatable, _bPrivateMatch, bBeginnerMatch, bUseBots, BotDifficulty, bRequireFilled);
	}
}

void UUTLocalPlayer::CreateNewMatch(ECreateInstanceTypes::Type InstanceType, AUTReplicatedGameRuleset* Ruleset, const FString& StartingMap, bool bRankLocked, bool bSpectatable, bool _bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled)
{
	FString URL = Ruleset->GenerateURL(StartingMap, bUseBots, BotDifficulty, bRequireFilled);
	
	if (FUTAnalytics::IsAvailable())
	{
		URL += FUTAnalytics::AnalyticsLoggedGameOptionTrue;
	}

	if (FUTAnalytics::IsAvailable())
	{
		if (Ruleset->bCustomRuleset)
		{
			FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("MainMenu - Custom Game - %s"),*Ruleset->Data.GameMode));
		}

	}

	if (InstanceType == ECreateInstanceTypes::LAN)
	{
		if (FUTAnalytics::IsAvailable())
		{
			if (Ruleset->bCustomRuleset)
			{
				FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("LanGame - Custom Game - %s"),*Ruleset->Data.GameMode));
			}
			else
			{
				FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("LanGame - Predefined Game Type - %s"),*Ruleset->Data.GameMode));
			}
		}

		URL += TEXT("?MaxPlayerWait=180");

		AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PlayerState)
		{
			URL += FString::Printf(TEXT("?HostId=%s"), *PlayerState->UniqueId.ToString());
		}

#if UE_BUILD_SHIPPING
		FString ExecPath = TEXT("..\\..\\..\\WindowsServer\\Engine\\Binaries\\Win64\\UE4Server-Win64-Shipping.exe");
#elif UE_BUILD_TEST
		FString ExecPath = TEXT("..\\..\\..\\WindowsServer\\Engine\\Binaries\\Win64\\UE4Server-Win64-Test.exe");
#elif UE_BUILD_DEBUG
		FString ExecPath = TEXT("..\\..\\..\\WindowsServer\\Engine\\Binaries\\Win64\\UE4Server-Win64-Debug.exe");
#else
		FString ExecPath = TEXT("..\\..\\..\\WindowsServer\\Engine\\Binaries\\Win64\\UE4Server.exe");
#endif

#if WITH_EDITOR
		ExecPath = FPaths::EngineDir() + TEXT("\\Binaries\\Win64\\UE4Editor.exe");
#endif

		FString Options = FString::Printf(TEXT("unrealtournament %s -log -server -LAN -altpaks -AUTH_PASSWORD="), *URL);

		if (OnlineIdentityInterface.IsValid())
		{
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			if (UserId.IsValid())
			{
				Options += FString::Printf(TEXT(" -cloudID=%s"), *UserId->ToString());
			}
		}

		FString AppName = GetEpicAppName();
		if (!AppName.IsEmpty())
		{
			Options += FString::Printf(TEXT(" -EPICAPP=%s"), *AppName);
		}

		DedicatedServerProcessHandle = FPlatformProcess::CreateProc(*ExecPath, *(Options + FString::Printf(TEXT(" -ClientProcID=%u"), FPlatformProcess::GetCurrentProcessId())), true, true, false, NULL, 0, NULL, NULL);
		if (DedicatedServerProcessHandle.IsValid())
		{
			GEngine->SetClientTravel(PlayerController->GetWorld(), TEXT("127.0.0.1"), TRAVEL_Absolute);
#if !UE_SERVER
			TSharedPtr<SUTWaitingForListenServerDialog> Dialog;
			SAssignNew(Dialog, SUTWaitingForListenServerDialog).PlayerOwner(this).OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::WaitingForListenServerDialogClosed));

			OpenDialog(Dialog.ToSharedRef(),20000);
#endif
		}
	}
	else if (InstanceType == ECreateInstanceTypes::Standalone)
	{
		CheckLoadingMovie(Ruleset->Data.GameMode);

		if (FUTAnalytics::IsAvailable())
		{
			if (Ruleset->bCustomRuleset)
			{
				FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("MainMenu - Custom Game Type - %s"),*Ruleset->Data.GameMode));
			}
			else
			{
				FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerController), FString::Printf(TEXT("MainMenu - Predefined Game Type - %s"),*Ruleset->Data.GameMode));
			}
		}

		CloseAllUI();
		ConsoleCommand(TEXT("Start ") + URL);
	}
}

void UUTLocalPlayer::TrackGamePlayed(const FString& GameMode)
{
	// Attempt to build a full class name if needed
	FString GameModeClassName = UGameplayStatics::ParseOption(GameMode, TEXT("Game"));
	if ( GameModeClassName.IsEmpty() )
	{
		// It wasn't a full url so assume it's just a class name
		GameModeClassName = GameMode;
	}
	else
	{
		// Resolve aliases
		GameModeClassName = UGameMapsSettings::GetGameModeForName(GameModeClassName);
	}

	for (int32 i=0; i < GameModeCounts.Num(); i++)
	{
		if (GameModeCounts[i].GameModeClass.Equals(GameModeClassName, ESearchCase::IgnoreCase))		
		{
			GameModeCounts[i].PlayCount++;
			SaveConfig();
			return;
		}
	}

	// Not found, so add one.  NOTE: The constructor for FUTGameModeCountStorage will set the play count to 1 by default this way
	GameModeCounts.Add(FUTGameModeCountStorage(GameModeClassName));
	SaveConfig();
}

// Look to see if we have completed the tutorial for this 
void UUTLocalPlayer::CheckLoadingMovie(const FString& GameMode)
{
	FString GameModeClassName = UGameplayStatics::ParseOption(GameMode, TEXT("Game"));
	if ( GameModeClassName.IsEmpty() )
	{
		// It wasn't a full url so assume it's just a class name
		GameModeClassName = GameMode;
	}
	else
	{
		// Resolve aliases
		GameModeClassName = UGameMapsSettings::GetGameModeForName(GameModeClassName);
	}

	int32 PlayCount = 0;
	int32 FindIndex = INDEX_NONE;
	for (int32 i=0; i < GameModeCounts.Num(); i++)
	{
		if (GameModeCounts[i].GameModeClass.Equals(GameModeClassName, ESearchCase::IgnoreCase))		
		{
			FindIndex = i;
			break;
		}
	}

	PlayCount = FindIndex != INDEX_NONE ? GameModeCounts[FindIndex].PlayCount : 0;

	FString TutorialMovie = TEXT("");
	int32 DesiredTutorial = 0x00;
	if (GameMode.Find(TEXT(".UTDMGameMode"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=DM"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
	{
		DesiredTutorial = TUTORIAL_DM;
		TutorialMovie = TEXT("TutorialMovies/dm-tutorial");
	}
	else if (GameMode.Find(TEXT(".UTTeamDMGameMode"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=TDM"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
	{
		DesiredTutorial = TUTORIAL_TDM;
		TutorialMovie = TEXT("TutorialMovies/tdm-tutorial");
	}
	else if (GameMode.Find(TEXT(".UTCTFGameMode"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=CTF"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
	{
		DesiredTutorial = TUTORIAL_CTF;
		TutorialMovie = TEXT("TutorialMovies/ctf-tutorial");
	}
	else if (GameMode.Find(TEXT(".UTDuelGame"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=Duel"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
	{
		DesiredTutorial = TUTORIAL_Duel;
		TutorialMovie = TEXT("TutorialMovies/duel-tutorial");
	}
	else if (GameMode.Find(TEXT(".UTShowdownGame"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=TEAMSHOWDOWN"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
	{
		DesiredTutorial = TUTORIAL_Showdown;
		TutorialMovie = TEXT("TutorialMovies/showdown-tutorial");
	}
	else if (GameMode.Find(TEXT(".UTFlagRunGame"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE || GameMode.Find(TEXT("=FlagRun"), ESearchCase::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
	{
		DesiredTutorial = TUTORIAL_FlagRun;
		TutorialMovie = TEXT("TutorialMovies/flagrun-tutorial");
	}
			
	if (DesiredTutorial != 0x00)
	{
		if (CurrentProfileSettings && CurrentProfileSettings->TutorialVideoWatchCount.Contains(DesiredTutorial) )
		{
			if ( CurrentProfileSettings->TutorialVideoWatchCount[DesiredTutorial] > 3 )
			{
				return;
			}
			CurrentProfileSettings->TutorialVideoWatchCount[DesiredTutorial] = CurrentProfileSettings->TutorialVideoWatchCount[DesiredTutorial] + 1;
			SaveProfileSettings();
		}
		else
		{
			CurrentProfileSettings->TutorialVideoWatchCount.Add(DesiredTutorial,1);
			SaveProfileSettings();
		}

	
		// Look to see if this tutorial has been completed
		if ((GetProfileSettings()->TutorialMask & DesiredTutorial) != DesiredTutorial && PlayCount < 3)
		{
			// Set the loading movie
			UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
			if (GI)
			{
				GI->LoadingMovieToPlay = TutorialMovie;
				GI->bSuppressLoadingText = true;
			}
		}
	}
}

void UUTLocalPlayer::WaitingForListenServerDialogClosed(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	// Terminate the dedicated server if we started one
	if (DedicatedServerProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(DedicatedServerProcessHandle))
	{
		FPlatformProcess::TerminateProc(DedicatedServerProcessHandle);
	}
}

bool UUTLocalPlayer::IsKillcamReplayActive()
{
	return (KillcamPlayback != nullptr && KillcamPlayback->IsEnabled());
}

void UUTLocalPlayer::RegainFocus()
{
#if !UE_SERVER
	if (DesktopSlateWidget.IsValid())
	{
		DesktopSlateWidget->HandleWindowActivated();
	}
#endif
}

void UUTLocalPlayer::StopKillCam()
{
	if (IsKillcamReplayActive() && KillcamPlayback )
	{
		KillcamPlayback->KillcamStop();
	}
}

void UUTLocalPlayer::SetPartyType(EPartyType InPartyType, bool bLeaderFriendsOnly, bool bLeaderInvitesOnly)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		UUTParty* Party = UTGameInstance->GetParties();
		if ( Party && IsPartyLeader() )
		{
			UUTPartyGameState* PartyGameState = Party->GetUTPersistentParty();
			if (PartyGameState)
			{
				PartyGameState->SetPartyType(InPartyType, bLeaderFriendsOnly, bLeaderInvitesOnly);		
			}
		}
	}
}

#if !UE_SERVER
TSharedPtr<SWidget> UUTLocalPlayer::GetBestWidgetToFocus()
{
	TSharedPtr<SWidget> Result;
	if (OpenDialogs.Num() > 0)
	{
		TSharedPtr<SUTDialogBase> TopMost;
		for (int32 i=0; i < OpenDialogs.Num(); i++)
		{
			if (OpenDialogs[i].IsValid() && (!TopMost.IsValid() || OpenDialogs[i]->ZOrder >= TopMost->ZOrder) )
			{
				TopMost = OpenDialogs[i];
			}
		}

		if (TopMost.IsValid())
		{
			Result = TopMost->GetBestWidgetToFocus();
		}
	}

	return Result.IsValid() ? Result : ViewportClient->GetGameViewportWidget();
}
#endif


void UUTLocalPlayer::CenterMouseCursor()
{
	UUTGameViewportClient* UTViewportClient = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTViewportClient)
	{
		FViewport* Viewport = UTViewportClient->Viewport;
		if (Viewport)
		{
			FVector2D ViewportSize;
			UTViewportClient->GetViewportSize(ViewportSize);
			ViewportSize *= 0.5f;
			Viewport->SetMouse(ViewportSize.X, ViewportSize.Y);
		}
	}
}

void UUTLocalPlayer::ProcessTitleFile(const FString& Filename, const TArray<uint8> FileContents)
{
	if (Filename == GetMCPStorageFilename())
	{
		FString JsonString = TEXT("");

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (FParse::Param(FCommandLine::Get(), TEXT("localUTMCPS")))
		{
			FString Path = FPaths::GameContentDir() + TEXT("EpicInternal/MCP/UnrealTournmentMCPStorage.json");
			FFileHelper::LoadFileToString(JsonString, *Path);
		}
#endif
		if (JsonString.IsEmpty())
		{
			JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());
		}

		if (JsonString != TEXT(""))
		{
			FMCPPulledData PulledData;
			if ( FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &PulledData, 0,0) )
			{
				MCPPulledData = PulledData;
				MCPPulledData.bValid = true;

				UUTGameEngine* GameEngine = Cast<UUTGameEngine>(GEngine);
				if ( GameEngine && GameEngine->GetChallengeManager().IsValid() )
				{
					GameEngine->GetChallengeManager()->UpdateChallengeFromMCP(MCPPulledData);

					// Check to see if we have a new daily challenge.  And if we do, update the profile.
					if (GameEngine->GetChallengeManager()->CheckDailyChallenge(CurrentProgression))
					{
						SaveProfileSettings();
					}
				}
			}
		}

		UpdateCheck();
	}
	else if (Filename == GetOnlineSettingsFilename())
	{
		ActiveRankedPlaylists.Empty();

		FString JsonString = TEXT("");
		JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());

		if (JsonString != TEXT(""))
		{
			FUTOnlineSettings UTOnlineSettings;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &UTOnlineSettings, 0, 0))
			{
				ActiveRankedPlaylists = UTOnlineSettings.ActiveRankedPlaylists;

				RankedEloRange = UTOnlineSettings.RankedEloRange;
				RankedMinEloRangeBeforeHosting = UTOnlineSettings.RankedMinEloRangeBeforeHosting;
				RankedMinEloSearchStep = UTOnlineSettings.RankedMinEloSearchStep;
				QMEloRange = UTOnlineSettings.QMEloRange;
				QMMinEloRangeBeforeHosting = UTOnlineSettings.QMMinEloRangeBeforeHosting;
				QMMinEloSearchStep = UTOnlineSettings.QMMinEloSearchStep;
			}
		}

		OnRankedPlaylistsChanged.Broadcast();
	}
	else if (Filename == GetMCPAnnouncementFilename())
	{
		FString JsonString = TEXT("");
		JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());

		if (JsonString != TEXT(""))
		{
			FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &MCPAnnouncements, 0, 0);
		}
	}
}

void UUTLocalPlayer::GameMutePlayer(const FString& PlayerId)
{
	int32 Index = GameMuteList.Find(PlayerId);
	if ( Index != INDEX_NONE)
	{
		GameMuteList.RemoveAt(Index,1);
	}
	else
	{
		GameMuteList.Add(PlayerId);
	}

	UpdateVoiceMuteList();
}

void UUTLocalPlayer::UpdateVoiceMuteList()
{
	static const FName VoiceChatFeatureName("VoiceChat");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VoiceChatFeatureName))
	{
		UTVoiceChatFeature* VoiceChat = &IModularFeatures::Get().GetModularFeature<UTVoiceChatFeature>(VoiceChatFeatureName);
		AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();

		if (VoiceChat && VoiceChat->IsLoggedIn() && VoiceChat->IsConnected() && !VoiceChat->GetChannelName().IsEmpty() && UTGameState)
		{
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				if (UTGameState->PlayerArray[i] != nullptr)
				{
					AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					FString IdString = UTPlayerState->UniqueId.ToString();
					if (UTPlayerState != nullptr)
					{
						bool bShouldBeMuted = false;

						if ( (GameMuteList.Find(IdString) != INDEX_NONE)  ||
							 (CurrentProfileSettings->ComFilter == EComFilter::NoComs) ||
							 (CurrentProfileSettings->ComFilter == EComFilter::FriendComs && !UTPlayerState->bIsFriend) ||
							 (CurrentProfileSettings->ComFilter == EComFilter::TeamComs && !UTGameState->bTeamGame) )
						{
							bShouldBeMuted = true;
						}

						bool bIsMuted = VoiceChat->IsPlayerMuted(IdString);

						if ( bIsMuted && !bShouldBeMuted)
						{
							VoiceChat->UnMutePlayer(IdString);
						}
						else if (!bIsMuted && bShouldBeMuted)
						{
							VoiceChat->MutePlayer(IdString);
						}
					}
				}
			}
		}
	}
}

bool UUTLocalPlayer::IsPlayerGameMuted(AUTPlayerState* PlayerToCheck)
{
	return PlayerToCheck && GameMuteList.Find(PlayerToCheck->UniqueId.ToString()) != INDEX_NONE;
}