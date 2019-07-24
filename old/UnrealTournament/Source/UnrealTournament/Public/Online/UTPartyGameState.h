// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PartyGameState.h"
#include "UTPartyGameState.generated.h"

class UUTParty;
class UUTPartyMemberState;

enum class EMatchmakingCompleteResult : uint8;
enum class EUTPartyMemberLocation : uint8;

UENUM(BlueprintType)
enum class EUTPartyState : uint8
{
	/** In the menus */
	Menus = 0,
	/** Actively matchmaking, no destination yet */
	Matchmaking,
	/** Destination found and beyond (attempting to join, lobby, game, etc) */
	PostMatchmaking,
	TravelToServer,
	/** Hub, custom dedicated server */
	CustomMatch,
	QuickMatching,
};

/**
 * Current state of the party
 */
USTRUCT()
struct FUTPartyRepState : public FPartyState
{
	GENERATED_USTRUCT_BODY();
	
	/** What the party is doing at the moment */
	UPROPERTY()
	EUTPartyState PartyProgression;
	
	/** Result of matchmaking by the leader */
	UPROPERTY()
	EMatchmakingCompleteResult MatchmakingResult;

	/** Session id to join when set */
	UPROPERTY()
	FString SessionId;

	/** Region that party leader started looking for in matchmaking */
	UPROPERTY()
	FString MatchmakingRegion;

	/** Number of players needed on a matchmaking server */
	UPROPERTY()
	int32 MatchmakingPlayersNeeded;

	/** Will be true if the game is a ranked game */
	UPROPERTY()
	bool bRanked;
	/** Will hold the current playlist ID */
	UPROPERTY()
	int32 PlaylistID;
	
	FUTPartyRepState()
	{
		Reset();
		PartyType = EPartyType::Public;
		bLeaderFriendsOnly = false;
		bLeaderInvitesOnly = false;
		bRanked = false;
		PlaylistID = 0;
	}

	/** Reset party back to defaults */
	virtual void Reset() override;
};

/**
 * Party game state that contains all information relevant to the communication within a party
 * Keeps all players in sync with the state of the party and its individual members
 */
UCLASS(config=Game, notplaceable)
class UNREALTOURNAMENT_API UUTPartyGameState : public UPartyGameState
{
	GENERATED_UCLASS_BODY()

	virtual void RegisterFrontendDelegates() override;
	virtual void UnregisterFrontendDelegates() override;

	/**
	 * Delegate fired when a party state changes
	 * 
	 * @param NewPartyState current state object containing the changes
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnClientPartyStateChanged, EUTPartyState /* NewPartyState */);
	
	/**
	 * Delegate fired when a party state changes
	 * 
	 * @param NewPartyState current state object containing the changes
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeaderPartyStateChanged, EUTPartyState /* NewPartyState */);

	/**
	 * Delegate fired when a matchmaking state changes
	 * 
	 * @param Result result of the last matchmaking attempt by the leader
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnClientMatchmakingComplete, EMatchmakingCompleteResult /* Result */);
	
	/**
	 * Delegate fired when a session id has been determined
	 * 
	 * @param SessionId session id party members should search for
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnClientSessionIdChanged, const FString& /* SessionId */);
	
private:
	/** Passenger view related delegates prior to joining a lobby/game */
	FOnClientPartyStateChanged ClientPartyStateChanged;
	FOnLeaderPartyStateChanged LeaderPartyStateChanged;
	FOnClientMatchmakingComplete ClientMatchmakingComplete;
	FOnClientSessionIdChanged ClientSessionIdChanged;
	
	/**
	 * Cached data for the party, only modifiable by the party leader
	 */
	UPROPERTY()
	FUTPartyRepState PartyState;

	/** Delegates related to party member data changes */
	FOnPartyMemberPropertyChanged LocationChanged;

	/**
	 * Handle matchmaking started delegate
	 */
	void OnPartyMatchmakingStarted(bool bRanked);
	
	/**
	 * Delegate triggered when matchmaking is complete
	 *
	 * @param EndResult in what state matchmaking ended
	 * @param SearchResult the result returned if successful
	 */
	void OnPartyMatchmakingComplete(EMatchmakingCompleteResult EndResult);
	
	/**
	 * Handle lobby connect request made by the party leader to members
	 *
	 * @param SearchResult destination of the party leader
	 */
	void OnConnectToLobby(const FOnlineSessionSearchResult& SearchResult);

	virtual void ComparePartyData(const FPartyState& OldPartyData, const FPartyState& NewPartyData) override;
	virtual bool IsInJoinableGameState() const override;

	friend UUTParty;
	friend UUTPartyMemberState;

public:
	/**
	 * Tell other party members about the location of local players
	 * 
	 * @param NewLocation new "location" within the game (see EUTPartyMemberLocation)
	 */
	void SetLocation(EUTPartyMemberLocation NewLocation);

	void SetSession(const FOnlineSessionSearchResult& InSearchResult);

	void SetPlayersNeeded(int32 PlayersNeeded);
	void SetMatchmakingRegion(const FString& InMatchmakingRegion);

	void SetPartyQuickMatching();
	void SetPartyJoiningQuickMatch();
	void SetPartyCancelQuickMatch();

	void NotifyTravelToServer();

	void ReturnToMainMenu();

	/** @return delegate fired when the location of the player has changed */
	FOnPartyMemberPropertyChanged& OnLocationChanged() { return LocationChanged; }

	FOnLeaderPartyStateChanged& OnLeaderPartyStateChanged() { return LeaderPartyStateChanged; }
	FOnClientPartyStateChanged& OnClientPartyStateChanged() { return ClientPartyStateChanged; }
	FOnClientMatchmakingComplete& OnClientMatchmakingComplete() { return ClientMatchmakingComplete; }
	FOnClientSessionIdChanged& OnClientSessionIdChanged() { return ClientSessionIdChanged; }

	EUTPartyState GetPartyProgression() const { return PartyState.PartyProgression; }
	int32 GetMatchmakingPlayersNeeded() const { return PartyState.MatchmakingPlayersNeeded; }
	FString GetMatchmakingRegion() const { return PartyState.MatchmakingRegion; }
	
	bool IsMatchRanked()
	{
		return PartyState.bRanked;
	}

	int32 GetPlaylistID()
	{
		return PartyState.PlaylistID;
	}

	virtual EApprovalAction ProcessJoinRequest(const FUniqueNetId& RecipientId, const FUniqueNetId& SenderId, EJoinPartyDenialReason& DenialReason) override;

};