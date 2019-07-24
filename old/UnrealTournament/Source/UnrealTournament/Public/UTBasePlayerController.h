// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTeamInterface.h"
#include "OnlineSessionInterface.h"
#include "UTServerBeaconClient.h"

#include "UTBasePlayerController.generated.h"

class UUTGameViewportClient;

UENUM(BlueprintType)
namespace EInputMode
{
	enum Type
	{
		EIM_None,
		EIM_GameOnly,
		EIM_GameAndUI,
		EIM_UIOnly,
	};
}

class AUTRconAdminInfo;
class UUTProfileSettings;

UCLASS()
class UNREALTOURNAMENT_API AUTBasePlayerController : public APlayerController , public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	AUTPlayerState* UTPlayerState;

	virtual void SetupInputComponent() override;

	virtual void Destroyed() override;

	virtual void InitInputSystem() override;

	virtual void SetName(const FString& S);

	/** Change name of server */
	UFUNCTION(reliable, server, WithValidation)
		void ServerChangeClanName(const FString& S);

	/** Tries to set the player's clan tag to the given name. */
	UFUNCTION(exec)
		virtual void ClanName(const FString& S);

	/**	Will popup the in-game menu	 **/
	UFUNCTION(exec, BlueprintCallable, Category = "UI")
	virtual void ShowMenu(const FString& Parameters);

	virtual void execShowMenu();

	UFUNCTION(exec)
	virtual void HideMenu();

	void InitPlayerState();

	UFUNCTION()
	virtual void Talk();

	UFUNCTION()
	virtual void TeamTalk();

	UFUNCTION(exec)
	virtual void Say(FString Message);

	UFUNCTION(exec)
	virtual void TeamSay(FString Message);

	UFUNCTION(reliable, server, WithValidation)
	virtual void ServerSay(const FString& Message, bool bTeamMessage);

	UFUNCTION(exec)
	void LobbySay(FString Message);

	UFUNCTION(reliable, server, WithValidation)
	void ServerLobbySay(const FString& Message);

	virtual bool AllowSay(AUTPlayerState* Speaker, const FString& Message, FName Destination);

	UFUNCTION(reliable, client)
	virtual void ClientSay(class AUTPlayerState* Speaker, const FString& Message, FName Destination);

	UFUNCTION(exec)
	virtual void FriendSay(FString Message);

	UFUNCTION(reliable, server, WithValidation)
	virtual void ServerFriendSay(const FString& Message, const TArray<FUniqueNetIdRepl>& FriendIds);

	virtual uint8 GetTeamNum() const;
	// not applicable
	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

#if !UE_SERVER
	virtual void ShowMessage(FText MessageTitle, FText MessageText, uint16 Buttons, const FDialogResultDelegate& Callback = FDialogResultDelegate());
#endif

	// A quick function so I don't have to keep adding one when I want to test something.  @REMOVEME: Before the final version
	UFUNCTION(exec)
	virtual void DebugTest(FString TestCommand);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDebugTest(const FString& TestCommand);


public:

	UFUNCTION(BlueprintCallable, Category = PlayerController)
	virtual UUTLocalPlayer* GetUTLocalPlayer();

	/**
	 *	User a GUID to find a server via the MCP and connect to it.  NOTE.. DesiredTeam = 0, 1, 255 or -1 for don't set the team
	 **/
	virtual void ConnectToServerViaGUID(FString ServerGUID, int32 DesiredTeam, bool bSpectate=false, bool bVerifyServerFirst = false);

	/**
	 *	Used by the hub system to cancel a pending connect if the player is downloading content.  Used for aborting.
	 **/

	virtual void CancelConnectViaGUID();

	UFUNCTION(Client, Reliable)
	virtual void ClientMatchmakingGameComplete();

	UFUNCTION(Client, Reliable)
	virtual void ClientReturnToLobby(bool bKicked = false, bool bIdle = false);

	UFUNCTION(Client, Reliable)
	virtual void ClientRankedGameAbandoned();

	UFUNCTION()
	virtual void ClientReturnedToMenus();

	// Allows the game to cause the client to set it's presence.
	UFUNCTION(Client, reliable)
	virtual void ClientSetPresence(const FString& NewPresenceString, bool bAllowInvites, bool bAllowJoinInProgress, bool bAllowJoinViaPresence, bool bAllowJoinViaPresenceFriendsOnly);

	UFUNCTION(client, reliable)
	virtual void ClientGenericInitialization();

	UFUNCTION(server, reliable, WithValidation)
	virtual void ServerReceiveStars(int32 TotalStars);

	UFUNCTION(client, reliable)
	virtual void ClientRequireContentItemListBegin(const FString& CloudId);

	UFUNCTION(client, reliable)
	virtual void ClientRequireContentItem(const FString& PakFile, const FString& MD5);

	UFUNCTION(client, reliable)
	virtual void ClientRequireContentItemListComplete();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetAvatar(FName NewAvatar);

	virtual void ReceivedPlayer();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerReceiveStatsID(const FString& NewStatsID);

	void SendStatsIDToServer();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerReceiveCosmetics(const FString& CosmeticString);

	void SendCosmeticsToServer();

protected:
	FOnFindSessionsCompleteDelegate OnFindGUIDSessionCompleteDelegate;
	FDelegateHandle OnFindGUIDSessionCompleteDelegateHandle;
	
	FOnCancelFindSessionsCompleteDelegate OnCancelGUIDFindSessionCompleteDelegate;
	FDelegateHandle OnCancelGUIDFindSessionCompleteDelegateHandle;

	TSharedPtr<class FUTOnlineGameSearchBase> GUIDSessionSearchSettings;

	FString GUIDJoin_CurrentGUID;
	bool GUIDJoinWantsToSpectate;
	int32 GUIDJoinAttemptCount;
	int32 GUIDJoinDesiredTeam;
	bool bGUIDJoinVerifyFirst;

	virtual void OnPingBeaconResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo);
	virtual void OnPingBeaconFailure(AUTServerBeaconClient* Sender);
	
	UPROPERTY()
	AUTServerBeaconClient* PingBeacon;

public:
	void StartGUIDJoin();

protected:
	void AttemptGUIDJoin();
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnCancelGUIDFindSessionComplete(bool bWasSuccessful);
	
public:
	UFUNCTION(Exec)
	virtual void UTLogOut();

	UFUNCTION(Exec)
	virtual void RconAuth(FString Password);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconAuth(const FString& Password);

	UFUNCTION(Exec)
	virtual void RconNormal();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconNormal();

	UFUNCTION(Exec)
	virtual void RconDBExec(FString Command);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconDBExec(const FString& Command);

	UFUNCTION(Exec)
	virtual void RconExec(FString Command);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconExec(const FString& Command);

	UFUNCTION(Exec)
	virtual void RconKick(FString NameOrUIDStr, bool bBan = false, FString Reason = TEXT(""));

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconKick(const FString& NameOrUIDStr, bool bBan, const FString& Reason);

	UFUNCTION(Exec)
	virtual void RconUnban(const FString& UIDStr);

	UFUNCTION(server, Reliable, WithValidation)
	virtual void ServerRconUnban(const FString& UIDStr);

	UFUNCTION(Exec)
	virtual void RconMessage(const FString& DestinationId, const FString &Message);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRconMessage(const FString& DestinationId, const FString &Message);

	// Let the game's player controller know there was a network failure message.
	virtual void HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString);

	/**Check to see if this PC can chat. Called on Client and server independantly*/
	bool AllowTextMessage(FString& Msg, bool bIsTeamMessage);

	/**The accumulation of time added per message. Once overflowed the player must wait for this to return to 0*/
	float ChatOverflowTime;
	bool bOverflowed;
	FText SpamText;
	FString LastChatMessage;


	UFUNCTION(Client, Reliable)
	virtual void ClientCloseAllUI(bool bExceptDialogs = false);

	/**This is overridden to avoid the Slate focus issues occuring with each widget managing their own input mode.
	Instead of setting this manually, we will update the input mode based on the state of the game in UpdateInputMode()*/
	virtual void SetInputMode(const FInputModeDataBase& InData) override {}

#if !UE_SERVER
	virtual void Tick(float DeltaTime) override;
	virtual void UpdateInputMode(bool bForce = false);

#endif

	UPROPERTY()
	TEnumAsByte<EInputMode::Type> InputMode;

	virtual void ShowAdminDialog(AUTRconAdminInfo* AdminInfo);
	virtual void ShowAdminMessage(const FString& Message);

	// If set to true, the player controller will pop up a menu as soon as it's safe
	bool bRequestShowMenu;

public:
	UPROPERTY()
	bool bSpectatorMouseChangesView;

	// Will actually query the UParty once persistent parties are enabled
	bool IsPartyLeader() { return true; }

	UFUNCTION(BlueprintCallable, Category = "Profile")
	UUTProfileSettings* GetProfileSettings();

	UFUNCTION(BlueprintCallable, Category = "Profile")
	virtual void SaveProfileSettings();

	UFUNCTION(BlueprintCallable, Category = "Profile")
	virtual void ApplyProfileSettings();


	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Movies")
	virtual void ClientPlayMovie(const FString& MovieName);

	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Movies")
	virtual void ClientStopMovie();

	virtual void ClientEnableNetworkVoice_Implementation(bool bEnable);

	UFUNCTION(exec)
	void StartVOIPTalking();

	UFUNCTION(exec)
	void StopVOIPTalking();

	// Returns true if any UI is active
	UFUNCTION(exec, BlueprintCallable, Category = "UI")
	bool AreMenusOpen();

	UFUNCTION(exec)
	void ExportKeyBinds();

	UFUNCTION(exec)
	void ImportKeyBinds();

	UFUNCTION(BlueprintCallable, Category = OnBoarding)
	void MarkTutorialAsCompleted(int32 TutorialMask);

	bool SkipTutorialCheck();

	UFUNCTION(BlueprintCallable, Category = "Movie")
	void SetLoadingMovieToPlay(const FString& MoviePath, bool bSuppressLoadingText=false);

	UFUNCTION(exec)
	void UTDumpOnlineSessionState();
	UFUNCTION(exec)
	void UTDumpPartyState();

protected:
	// Sends a message directly to a user.  
	virtual void DirectSay(const FString& Message);

	// Forward the direct say to alternate servers
	virtual bool ForwardDirectSay(AUTPlayerState* SenderPlayerState, FString& FinalMessage);

public:

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	virtual void LaunchTutorial(FName TutorialName);

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	virtual void NextTutorial();

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	virtual void PrevTutorial();

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	virtual void RepeatTutorial();

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	virtual FText GetTutorialSectionText(TEnumAsByte<ETutorialSections::Type> Section) const;

	// Kicks the player, but allows enough time for the kick message to replicate.
	UFUNCTION()
	virtual void GuaranteedKick( const FText& KickReason, bool bKickToHubIfPossible = false);

	void ClientWasKicked_Implementation(const FText& KickReason) override;

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	FText GetNextTutorialName();

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	FText GetPrevTutorialName();

	UFUNCTION(exec)
	void ExportGameRulesets(FString Filename);

	UFUNCTION(BlueprintCallable, Category="UT")
	virtual bool IsPlayerGameMuted(AUTPlayerState* PlayerToCheck);


protected:
	FTimerHandle AuthKickHandle;

	void InitializeHeartbeatManager();
	void TimedKick();

	UPROPERTY()
	class UUTHeartbeatManager* HeartbeatManager;

};