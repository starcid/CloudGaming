	// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTATypes.h"
#include "UTLobbyPlayerState.h"
#include "UTBasePlayerController.h"
#include "UTLobbyPC.generated.h"

UCLASS(config=Game)
class UNREALTOURNAMENT_API AUTLobbyPC : public AUTBasePlayerController
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY()
	AUTLobbyPlayerState* UTLobbyPlayerState;

	UPROPERTY()
	class AUTLobbyHUD* UTLobbyHUD;
		
	virtual void InitPlayerState();
	virtual void OnRep_PlayerState();

	virtual void ServerRestartPlayer_Implementation();
	virtual bool CanRestartPlayer();

	UFUNCTION(exec)
	virtual void MatchChat(FString Message);

	UFUNCTION(exec)
	virtual void GlobalChat(FString Message);

	virtual void Chat(FName Destination, FString Message);

	UFUNCTION(reliable, server, WithValidation)
	virtual void ServerChat(const FName Destination, const FString& Message);

	virtual void ReceivedPlayer();

	virtual void ServerDebugTest_Implementation(const FString& TestCommand);

	virtual void PlayerTick( float DeltaTime );

	UFUNCTION(exec)
	virtual void SetLobbyDebugLevel(int32 NewLevel);

	void MatchChanged(AUTLobbyMatchInfo* CurrentMatch);

	virtual void HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString);

	virtual void Say(FString Message);

	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerRconKillMatch(AUTLobbyMatchInfo* MatchToKill);


#if !UE_SERVER
	virtual void RequestServerSendAllRedirects();
#endif

	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerSendRedirectCount();
	
	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerSendAllRedirects();

	UFUNCTION(client, reliable)
	virtual void ClientReceiveRedirectCount(int32 NewCount);

	UFUNCTION(client, reliable)
	virtual void ClientReceiveRedirect(const FPackageRedirectReference& Redirect);


protected:
	// Will be true when the initial player replication is completed.  At that point it's safe to bring up the menu
	bool bInitialReplicationCompleted;

	int32 RedirectCount;

	virtual bool ForwardDirectSay(AUTPlayerState* SenderPlayerState, FString& Message);

public:
	virtual void ServerReceiveStatsID_Implementation(const FString& NewStatsID) override;

public:
	
	bool bReceivedServerContentList;
	TArray<FPackageRedirectReference> AllRedirects;
	void ResetAllRedirects();

};




