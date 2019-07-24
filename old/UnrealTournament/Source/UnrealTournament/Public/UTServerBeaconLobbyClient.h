// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineBeaconClient.h"
#include "UTServerBeaconClient.h"
#include "UTReplicatedMapInfo.h"
#include "UTServerBeaconLobbyClient.generated.h"


/**
* This because allows the 
*/
UCLASS(transient, notplaceable, config = Engine)
class UNREALTOURNAMENT_API AUTServerBeaconLobbyClient : public AOnlineBeaconClient
{
	GENERATED_UCLASS_BODY()

//	virtual FString GetBeaconType() override { return TEXT("UTLobbyBeacon"); }

	virtual void InitLobbyBeacon(FURL LobbyURL, uint32 LobbyInstanceID, FGuid InstanceGUID, FString AccessKey);
	virtual void OnConnected();
	virtual void OnFailure();
	
	virtual void UpdateMatch(const FMatchUpdate& MatchUpdate);
	virtual void UpdatePlayer(AUTBaseGameMode* GameMode, AUTPlayerState* PlayerState, bool bLastUpdate);
	virtual void StartGame(const FMatchUpdate& MatchUpdate);
	virtual void EndGame(const FMatchUpdate& FinalMatchUpdate);
	virtual void Empty();

	/**
	 *	NotifyInstanceIsReady will be called when the game instance is ready.  It will replicate to the lobby server and tell the lobby to have all of the clients in this match transition
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_NotifyInstanceIsReady(uint32 InstanceID, FGuid InstanceGUID, const FString& MapName);

	/**
	 * Tells the Lobby to update it's description on the stats
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_UpdateMatch(uint32 InstanceID, const FMatchUpdate& MatchUpdate);

	/**
	 *	Allows the instance to update the lobby regarding a given player.
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_UpdatePlayer(uint32 InstanceID, FRemotePlayerInfo PlayerInfo, bool bLastUpdate);

	/**
	 *	Tells the Lobby that this instance has started (not in warmup or prematch)
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_StartGame(uint32 InstanceID, const FMatchUpdate& MatchUpdate);

	/**
	 *	Tells the Lobby that this instance is at Game Over
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_EndGame(uint32 InstanceID, const FMatchUpdate& MatchUpdate);

	/**
	 *	Tells the Lobby server that this instance is empty and it can be recycled.
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_InstanceEmpty(uint32 InstanceID);


	/**
	 *	When we connect, if this is a connection between a hub and a dedicated instance, this function
	 * will be called to let the hub know I want to be a dedicated instance.
	 **/
	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_IsDedicatedInstance(FGuid InstanceGUID, const FString& InHubKey, const FString& ServerName, const FString& ServerGameMode, const FString& ServerDescription, int32 MaxPlayers, bool bTeamGame);

	/**
	 *	Called from the hub, this will let a dedicated instance know it's been authorized and is connected to 
	 *  the Hub.
	 **/
	UFUNCTION(client, reliable)
	virtual void AuthorizeDedicatedInstance(FGuid HubGuid, int32 InstanceID);

	UFUNCTION(client, reliable)
	virtual void Instance_ReceieveRconMessage(const FString& TargetUniqueId, const FString& AdminMessage);

	UFUNCTION(client, reliable)
	virtual void Instance_ReceiveUserMessage(const FString& TargetUniqueId, const FString& Message);

	UFUNCTION(client, reliable)
	virtual void Instance_ForceShutdown();

	UFUNCTION(client, reliable)
	virtual void Instance_Kick(const FString& TargetUniqueId);

	UFUNCTION(client, reliable)
	virtual void Instance_AuthorizeAdmin(const FString& AdminId, bool bIsAdmin);

	UFUNCTION()
	virtual void Lobby_RequestFirstBanFromServer(uint32 InstanceID);

	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_RequestNextBanFromServer(uint32 InstanceID, int32 LastIndex);

	UFUNCTION(client, reliable)
	virtual void Instance_ReceiveBan(FUniqueNetIdRepl BanId, int32 Index, int32 Total, bool bFinished);

	UFUNCTION(server, reliable, WithValidation)
	virtual void Lobby_ReceiveUserMessage(const FString& Message, const FString&SenderName);
protected:

	// Will be set to true when the game instance is empty and has asked the lobby to kill it
	bool bInstancePendingKill;

	uint32 GameInstanceID;
	FGuid GameInstanceGUID;

	bool bDedicatedInstance;

	FString HubKey;

	UPROPERTY()
	TArray<AUTReplicatedMapInfo*> AllowedMaps;

	UPROPERTY()
	TArray<FUniqueNetIdRepl> BannedIds;

};
