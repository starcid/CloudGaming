// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTMatchmakingPolicy.h"
#include "UTPartyBeaconClient.h"
#include "UTPartyMemberState.h"
#include "UTParty.h"
#include "UTMatchmaking.h"
#include "UTPartyGameState.h"

void FUTPartyRepState::Reset()
{
	FPartyState::Reset();
	PartyProgression = EUTPartyState::Menus;
	MatchmakingResult = EMatchmakingCompleteResult::NotStarted;
	SessionId.Empty();
	MatchmakingRegion.Empty();
	MatchmakingPlayersNeeded = 0;
}

UUTPartyGameState::UUTPartyGameState(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PartyMemberStateClass = UUTPartyMemberState::StaticClass();

	InitPartyState(&PartyState);
	ReservationBeaconClientClass = AUTPartyBeaconClient::StaticClass();
}

void UUTPartyGameState::RegisterFrontendDelegates()
{
	UWorld* World = GetWorld();

	Super::RegisterFrontendDelegates();

	UUTParty* UTParty = Cast<UUTParty>(GetOuter());
	if (UTParty)
	{
		TSharedPtr<const FOnlinePartyId> ThisPartyId = GetPartyId();
		const FOnlinePartyTypeId ThisPartyTypeId = GetPartyTypeId();

		if (ensure(ThisPartyId.IsValid()) && ThisPartyTypeId == UUTParty::GetPersistentPartyTypeId())
		{
			// Persistent party features (move to subclass eventually)
			UUTGameInstance* GameInstance = GetTypedOuter<UUTGameInstance>();
			check(GameInstance);
			UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
			if (Matchmaking)
			{
				Matchmaking->OnMatchmakingStarted().AddUObject(this, &ThisClass::OnPartyMatchmakingStarted);
				Matchmaking->OnMatchmakingComplete().AddUObject(this, &ThisClass::OnPartyMatchmakingComplete);
				Matchmaking->OnConnectToLobby().AddUObject(this, &ThisClass::OnConnectToLobby);
			}
		}
	}
}

void UUTPartyGameState::UnregisterFrontendDelegates()
{
	Super::UnregisterFrontendDelegates();

	UWorld* World = GetWorld();

	UUTGameInstance* GameInstance = GetTypedOuter<UUTGameInstance>();
	if (GameInstance)
	{
		UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
		if (Matchmaking)
		{
			Matchmaking->OnMatchmakingStarted().RemoveAll(this);
			Matchmaking->OnMatchmakingComplete().RemoveAll(this);
			Matchmaking->OnConnectToLobby().RemoveAll(this);
		}
	}
}

void UUTPartyGameState::OnPartyMatchmakingStarted(bool bRanked)
{
	if (IsLocalPartyLeader())
	{

		UUTGameInstance* GameInstance = GetTypedOuter<UUTGameInstance>();
		if (GameInstance)
		{
			UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
			if (Matchmaking)
			{
				PartyState.PlaylistID = Matchmaking->GetPlaylistID();
			}
		}

		PartyState.bRanked = bRanked;

		if (bRanked)
		{
			PartyState.PartyProgression = EUTPartyState::Matchmaking;
		}
		else
		{
			PartyState.PartyProgression = EUTPartyState::QuickMatching;
		}

		OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
		UpdatePartyData(OwningUserId);
		SetAcceptingMembers(false, EJoinPartyDenialReason::Busy);
	}
}

void UUTPartyGameState::OnPartyMatchmakingComplete(EMatchmakingCompleteResult Result)
{

	UE_LOG(LogParty, Verbose, TEXT("OnPartyMatchmakingComplete: Result = %i, IsLocalPartyLeader = %i"), int32(Result), IsLocalPartyLeader());


	if (IsLocalPartyLeader())
	{
		PartyState.MatchmakingResult = Result;

		if (Result == EMatchmakingCompleteResult::Success)
		{
			PartyState.PartyProgression = EUTPartyState::PostMatchmaking;
			OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
			//DoLeaderMatchmakingCompleteAnalytics();
		}
		else
		{
			UpdateAcceptingMembers();
			PartyState.PartyProgression = EUTPartyState::Menus;
			OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
		}

		UpdatePartyData(OwningUserId);
	}
	else
	{
		if (Result == EMatchmakingCompleteResult::Cancelled)
		{
			// Should only come from leader promotion or leader matchmaking cancellation
			// Party members can't cancel while passenger
		}
		else if (Result != EMatchmakingCompleteResult::Success)
		{
			UUTParty* UTParty = Cast<UUTParty>(GetOuter());
			if (ensure(UTParty))
			{
				UTParty->LeaveAndRestorePersistentParty();
			}
			
			UWorld* World = GetWorld();
			if (World)
			{
				for (FLocalPlayerIterator It(GEngine, World); It; ++It)
				{
					UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(*It);
					if (LP)
					{
						LP->MessageBox(NSLOCTEXT("UTPartyGameState", "FailedMatchmakingTitle", "Failed To Connect"), 
							           NSLOCTEXT("UTPartyGameState", "FailedMatchmaking", "Failed to connect to the party leader's game. You have left the party. If it was a LAN game, you will need to join manually through the server browser as LAN games aren't supported with parties."));
					}
				}
			}
		}
	}
}

void UUTPartyGameState::SetLocation(EUTPartyMemberLocation NewLocation)
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (FLocalPlayerIterator It(GEngine, World); It; ++It)
		{
			ULocalPlayer* LP = Cast<ULocalPlayer>(*It);
			if (LP)
			{
				TSharedPtr<const FUniqueNetId> UniqueId = LP->GetPreferredUniqueNetId();

				FUniqueNetIdRepl PartyMemberId(UniqueId);
				UUTPartyMemberState* PartyMember = Cast<UUTPartyMemberState>(GetPartyMember(PartyMemberId));
				if (PartyMember)
				{
					PartyMember->SetLocation(NewLocation);
				}
			}
		}

		if (NewLocation == EUTPartyMemberLocation::InGame)
		{
			if (PartyConsoleVariables::CVarAcceptJoinsDuringLoad.GetValueOnGameThread())
			{
				UE_LOG(LogParty, Verbose, TEXT("Handling party reservation approvals after load into game."));
				ConnectToReservationBeacon();
			}
		}
	}
}

void UUTPartyGameState::OnConnectToLobby(const FOnlineSessionSearchResult& SearchResult)
{
	if (ensure(SearchResult.IsValid()))
	{
		// Everyone notes the search result for party leader migration
		CurrentSession = SearchResult;

		if (IsLocalPartyLeader())
		{
			PartyState.PartyProgression = EUTPartyState::PostMatchmaking;
			OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);

			const FUniqueNetId& SessionId = SearchResult.Session.SessionInfo->GetSessionId();
			PartyState.SessionId = SessionId.ToString();

			UpdatePartyData(OwningUserId);

			if (ReservationBeaconClient == nullptr)
			{
				// Handle any party join requests now
				UE_LOG(LogParty, Verbose, TEXT("Handling party reservation approvals after connection to lobby."));
				ConnectToReservationBeacon();
			}
		}
	}
}

// This is the non-ranked experience
void UUTPartyGameState::SetSession(const FOnlineSessionSearchResult& SearchResult)
{
	if (ensure(SearchResult.IsValid()))
	{
		// Everyone notes the search result for party leader migration
		CurrentSession = SearchResult;

		if (IsLocalPartyLeader())
		{
			PartyState.PartyProgression = EUTPartyState::CustomMatch;
			OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);

			const FUniqueNetId& SessionId = SearchResult.Session.SessionInfo->GetSessionId();
			PartyState.SessionId = SessionId.ToString();

			UpdatePartyData(OwningUserId);
		}
	}
}

void UUTPartyGameState::ComparePartyData(const FPartyState& OldPartyData, const FPartyState& NewPartyData)
{
	Super::ComparePartyData(OldPartyData, NewPartyData);

	// Client passenger view delegates, leader won't get these because they are driving
	if (!IsLocalPartyLeader())
	{
		const FUTPartyRepState& OldUTPartyData = static_cast<const FUTPartyRepState&>(OldPartyData);
		const FUTPartyRepState& NewUTPartyData = static_cast<const FUTPartyRepState&>(NewPartyData);

		if (OldUTPartyData.MatchmakingResult != NewUTPartyData.MatchmakingResult)
		{
			OnClientMatchmakingComplete().Broadcast(NewUTPartyData.MatchmakingResult);
		}

		if (OldUTPartyData.SessionId != NewUTPartyData.SessionId)
		{
			OnClientSessionIdChanged().Broadcast(NewUTPartyData.SessionId);
		}

		if (OldUTPartyData.PartyProgression != NewUTPartyData.PartyProgression)
		{
			OnClientPartyStateChanged().Broadcast(NewUTPartyData.PartyProgression);
		}
	}
}

void UUTPartyGameState::NotifyTravelToServer()
{
	PartyState.PartyProgression = EUTPartyState::TravelToServer;
	OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::SetPlayersNeeded(int32 PlayersNeeded)
{
	PartyState.MatchmakingPlayersNeeded = PlayersNeeded;
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::SetMatchmakingRegion(const FString& InMatchmakingRegion)
{
	PartyState.MatchmakingRegion = InMatchmakingRegion;
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::ReturnToMainMenu()
{
	PartyState.SessionId.Empty();
	PartyState.PartyProgression = EUTPartyState::Menus;
	OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::SetPartyQuickMatching()
{
	PartyState.PartyProgression = EUTPartyState::QuickMatching;
	OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::SetPartyJoiningQuickMatch()
{
	PartyState.PartyProgression = EUTPartyState::CustomMatch;
	OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
	UpdatePartyData(OwningUserId);
}

void UUTPartyGameState::SetPartyCancelQuickMatch()
{
	PartyState.PartyProgression = EUTPartyState::Menus;
	OnLeaderPartyStateChanged().Broadcast(PartyState.PartyProgression);
	UpdatePartyData(OwningUserId);
}

bool UUTPartyGameState::IsInJoinableGameState() const
{
	if (PartyState.PartyProgression == EUTPartyState::Matchmaking ||
		PartyState.PartyProgression == EUTPartyState::PostMatchmaking)
	{
		return false;
	}

	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->bRestrictPartyJoin)
	{
		return false;
	}

	return Super::IsInJoinableGameState();
}

EApprovalAction UUTPartyGameState::ProcessJoinRequest(const FUniqueNetId& RecipientId, const FUniqueNetId& SenderId, EJoinPartyDenialReason& DenialReason)
{
	EApprovalAction Result = Super::ProcessJoinRequest(RecipientId, SenderId, DenialReason);
	if ( Result == EApprovalAction::Approve)
	{
		AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
		if (UTGameState && UTGameState->bRestrictPartyJoin)
		{
			DenialReason = EJoinPartyDenialReason::Busy;
			return EApprovalAction::Deny;
		}
	}
	return Result;
}
