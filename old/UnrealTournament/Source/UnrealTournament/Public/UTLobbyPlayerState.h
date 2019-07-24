// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTLobbyPlayerState.generated.h"

class AUTLobbyMatchInfo;
class AUTLobbyPC;
class AUTLobbyPlayerState;

#if !UE_SERVER
class SUTStartMatchWindow;
#endif


DECLARE_DELEGATE_OneParam(FCurrentMatchChangedDelegate, AUTLobbyPlayerState*)

UCLASS(notplaceable)
class UNREALTOURNAMENT_API AUTLobbyPlayerState : public AUTPlayerState
{
	GENERATED_UCLASS_BODY()

	virtual void PreInitializeComponents() override;

	// The match this player is currently in.
	UPROPERTY(Replicated, replicatedUsing = OnRep_CurrentMatch )
	AUTLobbyMatchInfo* CurrentMatch;

 	// The match this player is currently in.
	UPROPERTY(Replicated, replicatedUsing = OnRep_CurrentMatch )
	AUTLobbyMatchInfo* LastInvitedMatch;

	// The previous match this player played in.
	UPROPERTY()
	AUTLobbyMatchInfo* PreviousMatch;

	UPROPERTY()
	AUTLobbyMatchInfo* JoiningLeaderMatch;

	// Server-Side - Attempt to create a custom match
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCreateCustomInstance(const FString& CustomName, const FString& GameMode, const FString& StartingMap, bool bIsInParty, const FString& Description, const TArray<FString>& GameOptions,  int32 DesiredPlayerCount, bool bTeamGame, bool bRankLocked, bool bSpectatable, bool bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl);

	// Server-Side - Attempt to create a match based on a given game rule
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerCreateInstance(const FString& CustomName, const FString& RulesetTag, const FString& StartingMap, bool bIsInParty, bool bRankLocked, bool bSpectatable, bool bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl);

	// Server-Side.  Attempt to leave and or destory the existing match this player is in.
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDestroyOrLeaveMatch();

	// Server-Side.  Attept to Join a match
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerJoinMatch(AUTLobbyMatchInfo* MatchToJoin, bool bAsSpectator);

	// Server-Side - Called when this player has been added to a match
	virtual void AddedToMatch(AUTLobbyMatchInfo* Match);


	UFUNCTION(Client, Reliable)
	void ClientAddedToMatch(bool bIsHost);

	// Server-Side - Called when this player has been removed from a match
	virtual void RemovedFromMatch(AUTLobbyMatchInfo* Match);

	// Client-Side - Causes a error message to occur
	UFUNCTION(Client, Reliable)
	virtual void ClientMatchError(const FText& MatchErrorMessage, int32 OptionalInt = 0);

	/**
	 *	Tells a client to connect to a game instance.  InstanceGUID is the server GUID of the game instance this client should connect to.  The client
	 *  will use the OSS to pull the server information and perform the connection.  LobbyGUID is the server GUID of the lobbyt to return to.  The client
	 *  should cache this off and use it to return later.
	 **/
	UFUNCTION(Client, Reliable)
	virtual void ClientConnectToInstance(const FString& GameInstanceGUIDString, int32 InDesiredTeam, bool bAsSpectator);

	// Allows for UI/etc to pickup changes to the current match.
	FCurrentMatchChangedDelegate CurrentMatchChangedDelegate;

protected:

	// This flag is managed on both sides of the pond but isn't replicated.  It's only valid on the server and THIS client.
	bool bIsInMatch;

	UFUNCTION()
	void OnRep_CurrentMatch();

public:
	// The Unique ID of a friend this player wants to join
	FString DesiredFriendToJoin;

	// The Match Id we are trying to join
	FString DesiredMatchIdToJoin;

	// Will be true if this player wants to auto-join a match as a spectator.  Note this only comes from the url when connecting to the hub
	bool bDesiredJoinAsSpectator;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void Server_ReadyToBeginDataPush();

	UFUNCTION(Client, Reliable)
	virtual void Client_BeginDataPush(int32 ExpectedSendCount);


protected:

	// This is a system that pulls all of the needed allowed data for setting up custom rulesets.  When a LPS becomes live on the client, it 
	// will request a data push from the server.  As data comes in, it will fill out the data in the Lobby Game State.  

	int32 TotalBlockCount;
	int32 CurrentBlock;

	// checks if the client is ready for a datapush.  Both the LPS and LGS must exist on the client before the system can begin.
	// Normally this will be the case when Client_BeginDataPush is called, however, replication order isn't guarenteed so we have
	// to make sure it's ready.  If not, set a time and try again in 1/2 a second.
	virtual void CheckDataPushReady();

	// Tells the server to send me a new block.
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void Server_SendDataBlock(int32 Block);

	// Tells the client to receive a block.
	UFUNCTION(Client, Reliable)
	virtual void Client_ReceiveBlock(int32 Block, FAllowedData Data);

#if !UE_SERVER
	TSharedPtr<SUTStartMatchWindow> StartMatchWindow;
#endif

public:
	// We don't need TeamInfo's for the lobby, just store a desired team num for now.  255 will be spectator.
	UPROPERTY(Replicated)
	uint8 DesiredTeamNum;

	virtual uint8 GetTeamNum() const 
	{
		return DesiredTeamNum;
	}

	virtual void InviteToMatch(AUTLobbyMatchInfo* Match);
	virtual void UninviteFromMatch(AUTLobbyMatchInfo* Match);

	// Let's the client know that the server has auto-locked this match
	UFUNCTION(client, Reliable)
	virtual void NotifyBeginnerAutoLock();

	virtual void Tick(float DeltaTime) override;
	virtual void ManageStartMatchUI(AUTLobbyPC* PC);

};



