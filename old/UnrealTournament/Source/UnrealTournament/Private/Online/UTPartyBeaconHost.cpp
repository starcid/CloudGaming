// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "Online/UTPartyBeaconHost.h"
#include "Online/UTPartyBeaconClient.h"
#include "Online/UTPartyBeaconState.h"

AUTPartyBeaconHost::AUTPartyBeaconHost(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	UTState(NULL),
	bReservationsLocked(false)
{
	ClientBeaconActorClass = AUTPartyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

TSubclassOf<UPartyBeaconState> AUTPartyBeaconHost::GetPartyBeaconHostClass() const
{
	return UUTPartyBeaconState::StaticClass();
}

bool AUTPartyBeaconHost::InitHostBeacon(int32 InTeamCount, int32 InTeamSize, int32 InMaxReservations, FName InSessionName, int32 InForceTeamNum)
{
	if (Super::InitHostBeacon(InTeamCount, InTeamSize, InMaxReservations, InSessionName, InForceTeamNum))
	{
		UTState = Cast<UUTPartyBeaconState>(State);
	}

	return UTState != NULL;
}

bool AUTPartyBeaconHost::InitFromBeaconState(UPartyBeaconState* PrevState)
{
	if (Super::InitFromBeaconState(PrevState))
	{
		UTState = Cast<UUTPartyBeaconState>(State);
	}

	return UTState != NULL;
}

void AUTPartyBeaconHost::ProcessEmptyServerReservationRequest(APartyBeaconClient* Client, const FString& SessionId, const FEmptyServerReservation& ReservationData, const FPartyReservation& ReservationRequest)
{
	UE_LOG(LogBeacon, Verbose, TEXT("ProcessEmptyServerReservationRequest %s PlaylistId: %d Leader: %s PartySize: %d from (%s)"),
		Client ? *Client->GetName() : TEXT("NULL"),
		ReservationData.PlaylistId,
		ReservationRequest.PartyLeader.IsValid() ? *ReservationRequest.PartyLeader->ToString() : TEXT("INVALID"),
		ReservationRequest.PartyMembers.Num(),
		Client ? *Client->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	if (Client)
	{
		EPartyReservationResult::Type Result = EPartyReservationResult::BadSessionId;
		if (DoesSessionMatch(SessionId))
		{
			Result = AddEmptyServerReservationRequest(ReservationData, ReservationRequest);
		}

		UE_LOG(LogBeacon, Verbose, TEXT("ProcessEmptyServerReservationRequest result: %s"), EPartyReservationResult::ToString(Result));
		if (UE_LOG_ACTIVE(LogBeacon, Verbose) &&
			(Result != EPartyReservationResult::ReservationAccepted))
		{
			DumpReservations();
		}

		if (Result != EPartyReservationResult::RequestPending)
		{
			Client->ClientReservationResponse(Result);
		}
	}
}

void AUTPartyBeaconHost::DumpReservations() const
{
	UE_LOG(LogBeacon, Display, TEXT("UT Party Beacon:"), *GetBeaconType());
	APartyBeaconHost::DumpReservations();
}

void AUTPartyBeaconHost::RedraftTeams(const TArray<FPlayerGameData>& PlayerData)
{
	if (UTState)
	{
		const int32 NumTeams = GetNumTeams();
		if (NumTeams > 1)
		{
			const int32 MaxTeamSize = GetMaxPlayersPerTeam();

			// Create a list of all parties with highest player count, highest skill count first
			TArray<FPartyReservationTotalSize> Parties;

			for (int32 PartyIndex = 0; PartyIndex < UTState->Reservations.Num(); PartyIndex++)
			{
				int32 TotalSkill = 0;
				for (int32 i = 0; i < UTState->Reservations[PartyIndex].PartyMembers.Num(); i++)
				{
					for (int PlayerIdx = 0; PlayerIdx < PlayerData.Num(); PlayerIdx++)
					{
						if (PlayerData[PlayerIdx].UniqueId == UTState->Reservations[PartyIndex].PartyMembers[i].UniqueId)
						{
							TotalSkill += PlayerData[PlayerIdx].Skill;
						}
					}
				}

				FPartyReservationTotalSize PartyRes(PartyIndex, UTState->Reservations[PartyIndex].PartyMembers.Num(), TotalSkill);
				Parties.Add(PartyRes);
			}
			Parties.Sort([](const FPartyReservationTotalSize& A, const FPartyReservationTotalSize& B)
			{
				if (A.PartySize == B.PartySize)
				{
					return A.Skill > B.Skill;
				}

				return A.PartySize > B.PartySize;
			});

			// Create a list of teams
			TArray<FTeamBalanceInfo> TeamsCandidates;
			for (int32 i = 0; i < NumTeams; i++)
			{
				TeamsCandidates.Add(FTeamBalanceInfo(i));
			}

			for (int32 PartyIndex = 0; PartyIndex < Parties.Num(); PartyIndex++)
			{
				for (int32 TeamIndex = 0; TeamIndex < TeamsCandidates.Num(); TeamIndex++)
				{
					if (TeamsCandidates[TeamIndex].TotalPlayers + Parties[PartyIndex].PartySize <= MaxTeamSize)
					{
						// Update reservation with new team
						UTState->Reservations[Parties[PartyIndex].ReservationIndex].TeamNum = TeamsCandidates[TeamIndex].TeamIndex;

						// Update team info
						TeamsCandidates[TeamIndex].TotalPlayers += Parties[PartyIndex].PartySize;
						TeamsCandidates[TeamIndex].TotalSkill += Parties[PartyIndex].Skill;

						break;
					}
				}

				// Sort the team list again so most open slots, lowest skill first
				TeamsCandidates.Sort([](const FTeamBalanceInfo& A, const FTeamBalanceInfo& B)
				{
					if (A.TotalPlayers == B.TotalPlayers)
					{
						return A.TotalSkill < B.TotalSkill;
					}

					return A.TotalPlayers < B.TotalPlayers;
				});
			}
		}
	}
}

EPartyReservationResult::Type AUTPartyBeaconHost::AddEmptyServerReservationRequest(const FEmptyServerReservation& ReservationData, const FPartyReservation& ReservationRequest)
{
	EPartyReservationResult::Type Result = EPartyReservationResult::GeneralError;

	if (!UTState || GetBeaconState() == EBeaconState::DenyRequests)
	{
		return EPartyReservationResult::ReservationDenied;
	}

	bool bValidRequest = ReservationData.IsValid();
	bool bValidParty = ReservationRequest.IsValid();

	// Check for well formed request first
	if (bValidRequest && bValidParty)
	{
		// Check for existing reservation next
		const int32 ExistingReservationIdx = UTState->GetExistingReservation(ReservationRequest.PartyLeader);
		if (ExistingReservationIdx == INDEX_NONE)
		{
			// Only one reservation can configure the server, all further requests are denied
			if (!UTState->ReservationData.IsValid() && UTState->GetNumConsumedReservations() == 0)
			{
				if (UTState->DoesReservationFit(ReservationRequest))
				{
					if (UTState->AreTeamsAvailable(ReservationRequest))
					{
						// ValidatePlayers delegate not checked here because the session owner can't ban themselves
						// Party members will be checked on login

						UTState->SetUserConfiguration(ReservationData, ReservationRequest.PartyLeader.GetUniqueNetId());

						const TSharedPtr<const FUniqueNetId>& NewWorldOwner = UTState->GetGameSessionOwner();
						ServerConfigurationRequest.ExecuteIfBound(NewWorldOwner, ReservationData);

						// Wait for the configuration delegate to possibly modify team definitions
						if (UTState->AddReservation(ReservationRequest))
						{
							// Keep track of newly added players
							for (int32 Count = 0; Count < ReservationRequest.PartyMembers.Num(); Count++)
							{
								NewPlayerAdded(ReservationRequest.PartyMembers[Count]);
							}

							SendReservationUpdates();

							NotifyReservationEventNextFrame(ReservationChanged);
							if (UTState->IsBeaconFull())
							{
								NotifyReservationEventNextFrame(ReservationsFull);
							}

							Result = EPartyReservationResult::ReservationAccepted;
							bRankedServer = ReservationData.bRanked;
						}
						else
						{
							// Reset on failure
							FEmptyServerReservation ResetReservationData;
							UTState->SetUserConfiguration(ResetReservationData, NULL);
							Result = EPartyReservationResult::IncorrectPlayerCount;
						}
					}
					else
					{
						// Team can't accommodate party
						Result = EPartyReservationResult::PartyLimitReached;
					}
				}
				else
				{
					// Game can't accommodate party
					Result = EPartyReservationResult::IncorrectPlayerCount;
				}
			}
			else
			{
				// Wasn't the first party to this empty server
				Result = EPartyReservationResult::ReservationDenied;
			}
		}
		else
		{
			// Reservation already found
			Result = EPartyReservationResult::ReservationDuplicate;
		}
	}
	else
	{
		// Badly formed reservation
		Result = EPartyReservationResult::ReservationInvalid;
	}

	return Result;
}

void AUTPartyBeaconHost::LockReservations(bool bNewLockState)
{
	UE_LOG(LogBeacon, Verbose, TEXT("Setting reservation lock state to %s"), bNewLockState ? TEXT("true") : TEXT("false"));
	// warn on new reservations?
	bReservationsLocked = bNewLockState;
}

void AUTPartyBeaconHost::HandlePlayerLogout(const FUniqueNetIdRepl& PlayerId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("HandlePlayerLogout %s while %s"), *PlayerId.ToString(), bReservationsLocked ? TEXT("locked") : TEXT("unlocked"));

	if (bReservationsLocked)
	{
		return;
	}

	if (UTState)
	{
		const TSharedPtr<const FUniqueNetId>& SessionOwner = UTState->GetGameSessionOwner();
		if (SessionOwner.IsValid() && PlayerId.IsValid() &&
			*PlayerId == *SessionOwner)
		{
			// Reset the session owner
			UTState->GameSessionOwner = FUniqueNetIdRepl();
			
			for (const FPartyReservation& PartyReservation : UTState->Reservations)
			{
				for (const FPlayerReservation& PlayerReservation : PartyReservation.PartyMembers)
				{
					// See if we can find a replacement session owner
					if (*PlayerId != *PlayerReservation.UniqueId)
					{
						// Promote anyone for now
						UTState->ChangeSessionOwner(PlayerReservation.UniqueId.GetUniqueNetId().ToSharedRef());
						break;
					}
				}
			}
		}
	}
	
	// Lastly remove the player and notify the game
	Super::HandlePlayerLogout(PlayerId);
}

void AUTPartyBeaconHost::ProcessReservationRequest(APartyBeaconClient* Client, const FString& SessionId, const FPartyReservation& ReservationRequest)
{
	if (UTState)
	{
		// Make sure this server has been configured before handling normal reservations
		if (UTState->ReservationData.IsValid())
		{
			APartyBeaconHost::ProcessReservationRequest(Client, SessionId, ReservationRequest);
			return;
		}
	}

	UE_LOG(LogBeacon, Verbose, TEXT("ProcessReservationRequest %s denying request from (%s), server not configured yet."),
		Client ? *Client->GetName() : TEXT("NULL"),
		Client ? *Client->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));
	Client->ClientReservationResponse(EPartyReservationResult::ReservationDenied);
}


void AUTPartyBeaconHost::OnBeaconReservationsFull()
{
	EvaluatePermissionToProceedToGame();
}

void AUTPartyBeaconHost::OnBeaconReservationChange()
{
	EvaluatePermissionToProceedToGame();
}

void AUTPartyBeaconHost::OnBeaconReservationDuplicate()
{
	EvaluatePermissionToProceedToGame();
}

void AUTPartyBeaconHost::EvaluatePermissionToProceedToGame()
{
	const int32 NumReservations = GetNumConsumedReservations();
	const int32 MaxNumReservations = GetMaxReservations();	
	
	if (!bRankedServer || NumReservations == MaxNumReservations)
	{
		for (AOnlineBeaconClient* ClientActor : ClientActors)
		{
			AUTPartyBeaconClient* PartyBeaconClient = Cast<AUTPartyBeaconClient>(ClientActor);
			if (PartyBeaconClient)
			{
				PartyBeaconClient->ClientAllowedToProceedFromReservation();
			}
		}
	}
}