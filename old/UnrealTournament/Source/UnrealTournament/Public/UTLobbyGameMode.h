// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTLobbyPC.h"
#include "UTGameState.h"
#include "UTLobbyGameState.h"
#include "UTLobbyPlayerState.h"
#include "UTBaseGameMode.h"
#include "UTServerBeaconLobbyHostListener.h"
#include "UTServerBeaconLobbyHostObject.h"
#include "UTLobbyGameMode.generated.h"

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTLobbyGameMode : public AUTBaseGameMode
{
	GENERATED_UCLASS_BODY()

public:
	/** Cached reference to our game state for quick access. */
	UPROPERTY()
	AUTLobbyGameState* UTLobbyGameState;		

	// Once a hub has been alive for this many hours, it will attempt to auto-restart
	// itself when noone is one it.  In HOURS.
	UPROPERTY(GlobalConfig)
	int32 ServerRefreshCheckpoint;

	UPROPERTY(GlobalConfig)
	FString LobbyPassword;

	UPROPERTY(GlobalConfig)
	int32 StartingInstancePort;

	UPROPERTY(GlobalConfig)
	int32 InstancePortStep;

	UPROPERTY(GlobalConfig)
	FString AutoLaunchGameMode;

	UPROPERTY(GlobalConfig)
	FString AutoLaunchGameOptions;

	UPROPERTY(GlobalConfig)
	FString AutoLaunchMap;

	// The Maximum # of instances allowed.  Set to 0 to have no cap 
	UPROPERTY(GlobalConfig)
	int32 MaxInstances;

	UPROPERTY(GlobalConfig)
	bool bAllowInstancesToStartWithBots;

	UPROPERTY()
	TSubclassOf<class UUTLocalMessage>  GameMessageClass;

	UPROPERTY(GlobalConfig)
	int32 LobbyMaxTickRate;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState();
	virtual void StartMatch();
	virtual void RestartPlayer(AController* aPlayer);
	
	virtual void PostLogin( APlayerController* NewPlayer );
	virtual FString InitNewPlayer(class APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT(""));
	virtual void Logout(AController* Exiting);
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player);
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const;
	virtual void OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState);

	virtual bool IsLobbyServer() { return true; }

	virtual void AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC);

#if !UE_SERVER

	/**
	 *	Returns the Menu to popup when the user requests a menu
	 **/
	virtual TSharedRef<SUTMenuBase> GetGameMenu(UUTLocalPlayer* PlayerOwner) const;

#endif
	virtual FName GetNextChatDestination(AUTPlayerState* PlayerState, FName CurrentChatDestination);

protected:

	// The actual instance query port to use.
	int32 InstanceQueryPort;

public:
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);
	virtual void GetInstanceData(TArray<TSharedPtr<FServerInstanceData>>& InstanceData);

	virtual int32 GetNumPlayers();
	virtual int32 GetNumMatches();

	// Attempts to make sure the Lobby has the proper information
	virtual void UpdateLobbySession();

	virtual void DefaultTimer();

	UPROPERTY(Config)
	int32 MaxPlayersInLobby;

	virtual void SendRconMessage(const FString& DestinationId, const FString &Message);
	virtual void RconKick(const FString& NameOrUIDStr, bool bBan, const FString& Reason);
	virtual void RconAuth(AUTBasePlayerController* Admin, const FString& Password);
	virtual void RconNormal(AUTBasePlayerController* Admin);

	virtual void ReceivedRankForPlayer(AUTPlayerState* UTPlayerState);
	
	virtual void MakeJsonReport(TSharedPtr<FJsonObject> JsonObject);

	virtual bool SupportsInstantReplay() const override;

};
