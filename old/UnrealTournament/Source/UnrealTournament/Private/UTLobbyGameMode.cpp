// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/GameMode.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyGameState.h"
#include "UTLobbyPC.h"
#include "UTLobbyPlayerState.h"
#include "UTLobbyHUD.h"
#include "UTGameMessage.h"
#include "UTAnalytics.h"
#include "UTGameSessionNonRanked.h"
#include "Menus/SUTLobbyMenu.h"



AUTLobbyGameMode::AUTLobbyGameMode(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// use our custom HUD class
	HUDClass = AUTLobbyHUD::StaticClass();

	GameStateClass = AUTLobbyGameState::StaticClass();
	PlayerStateClass = AUTLobbyPlayerState::StaticClass();
	PlayerControllerClass = AUTLobbyPC::StaticClass();
	DefaultPlayerName = FText::FromString(TEXT("Player"));
	GameMessageClass = UUTGameMessage::StaticClass();

	MaxPlayersInLobby=200;
	bAllowInstancesToStartWithBots=false;
	DisplayName = NSLOCTEXT("UTLobbyGameMode", "HUB", "HUB");
}

// Parse options for this game...
void AUTLobbyGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	UE_LOG(UT,Log,TEXT("===================="));
	UE_LOG(UT,Log,TEXT("  Init Lobby Game"));
	UE_LOG(UT,Log,TEXT("===================="));

	GetWorld()->bShouldSimulatePhysics = false;

	Super::InitGame(MapName, Options, ErrorMessage);

	if (GameSession)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession)
		{
			UTGameSession->MaxPlayers = MaxPlayersInLobby;
		}
	}

	// I should move this code up in to UTBaseGameMode and probably will (the code hooks are all there) but
	// for right now I want to limit this to just Lobbies.

	MinAllowedRank = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("MinAllowedRank"), MinAllowedRank));
	if (MinAllowedRank > 0)
	{
		UE_LOG(UT,Log,TEXT("  Minimum Allowed ELO Rank is: %i"), MinAllowedRank)
	}

	MaxAllowedRank = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("MaxAllowedRank"), MaxAllowedRank));
	if (MaxAllowedRank > 0)
	{
		UE_LOG(UT,Log,TEXT("  Maximum Allowed ELO Rank is: %i"), MaxAllowedRank)
	}

	MaxInstances = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("MaxInstances"), MaxInstances));
}





void AUTLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	UTLobbyGameState = Cast<AUTLobbyGameState>(GameState);
	if (UTLobbyGameState != NULL)
	{
		UTLobbyGameState->bAllowInstancesToStartWithBots = bAllowInstancesToStartWithBots;
		UTLobbyGameState->bTrainingGround = bTrainingGround;

		// Setupo the beacons to listen for updates from Game Server Instances
		UTLobbyGameState->SetupLobbyBeacons();
	}
	else
	{
		UE_LOG(UT,Error, TEXT("UTLobbyGameState is NULL %s"), *GameStateClass->GetFullName());
	}

	// Register our Lobby with the lobby Server
	if (GameSession != NULL && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		GameSession->RegisterServer();
	}

	if (FUTAnalytics::IsAvailable() && (GetNetMode() == NM_DedicatedServer))
	{
		FUTAnalytics::FireEvent_UTHubBootUp(this);
	}
}

void AUTLobbyGameMode::StartMatch()
{
	SetMatchState(MatchState::InProgress);
}

void AUTLobbyGameMode::RestartPlayer(AController* aPlayer)
{
	return;
}

void AUTLobbyGameMode::OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState)
{
	Super::OverridePlayerState(PC, OldPlayerState);

	// if we're in this function GameMode swapped PlayerState objects so we need to update the precasted copy
	AUTLobbyPC* UTPC = Cast<AUTLobbyPC>(PC);
	if (UTPC != NULL)
	{
		UTPC->UTLobbyPlayerState = Cast<AUTLobbyPlayerState>(UTPC->PlayerState);
	}
}

FString AUTLobbyGameMode::InitNewPlayer(class APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	AUTLobbyPlayerState* PS = Cast<AUTLobbyPlayerState>(NewPlayerController->PlayerState);

	// Look for auto-join options.  Don't attempt to auto-join until we receive all of the relevant information from the client though
	if (PS)
	{	
		FString InstanceID = UGameplayStatics::ParseOption(Options,"Session");
		if (!InstanceID.IsEmpty())
		{
			// Can't use the playerstate's version here because it hasn't been set yet.
			PS->bDesiredJoinAsSpectator = FCString::Stricmp(*UGameplayStatics::ParseOption(Options, TEXT("SpectatorOnly")), TEXT("1")) == 0;
			PS->DesiredMatchIdToJoin = InstanceID;
		}

		FString FriendId = UGameplayStatics::ParseOption(Options, TEXT("Friend"));
		if (!FriendId.IsEmpty())
		{
			PS->DesiredFriendToJoin = FriendId;
		}

		FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("Character"));
		if (InOpt.Len() > 0)
		{
			PS->SetCharacter(InOpt);
		}

		PS->PartyLeader = UGameplayStatics::ParseOption(Options, TEXT("PartyLeader"));
		PS->PartySize = UGameplayStatics::GetIntOption(Options, TEXT("PartySize"), 1);
	}

	return Result;
}


void AUTLobbyGameMode::PostLogin( APlayerController* NewPlayer )
{
	Super::PostLogin(NewPlayer);

	UpdateLobbySession();

	// Set my Initial Presence
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(NewPlayer);
	if (PC)
	{
		PC->ClientSay(NULL, UTLobbyGameState->ServerMOTD, ChatDestinations::MOTD);
		// Set my initial presence....
		PC->ClientSetPresence(TEXT("Sitting in a Hub"), true, true, true, false);
	}
}


void AUTLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AUTLobbyPlayerState* LPS = Cast<AUTLobbyPlayerState>(Exiting->PlayerState);
	if (LPS && LPS->CurrentMatch)
	{
		UTLobbyGameState->RemoveFromAMatch(LPS);
	}

	UpdateLobbySession();
}

bool AUTLobbyGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}


TSubclassOf<AGameSession> AUTLobbyGameMode::GetGameSessionClass() const
{
	return AUTGameSessionNonRanked::StaticClass();
}

FName AUTLobbyGameMode::GetNextChatDestination(AUTPlayerState* PlayerState, FName CurrentChatDestination)
{
	if (CurrentChatDestination == ChatDestinations::Global)
	{
		AUTLobbyPlayerState* PS = Cast<AUTLobbyPlayerState>(PlayerState);
		if (PS && PS->CurrentMatch)
		{
			return ChatDestinations::Match;
		}

		return ChatDestinations::Friends;
	}

	if (CurrentChatDestination == ChatDestinations::Match) return ChatDestinations::Friends;

	return ChatDestinations::Global;
}

void AUTLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (MinAllowedRank > 0 || MaxAllowedRank > 0)
	{
		int32 PendingRank = UGameplayStatics::GetIntOption(Options, TEXT("Rank"), 0);
		if (MinAllowedRank > 0 && PendingRank < MinAllowedRank)
		{
			ErrorMessage = TEXT("TOOWEAK");
			return;
		}

		if (MaxAllowedRank > 0 && PendingRank > MaxAllowedRank)
		{
			ErrorMessage = TEXT("TOOSTRONG");
			return;
		}
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AUTLobbyGameMode::GetInstanceData(TArray<TSharedPtr<FServerInstanceData>>& InstanceData)
{
	if (UTLobbyGameState)
	{
		for (int32 i=0; i < UTLobbyGameState->AvailableMatches.Num();i++)
		{
			AUTLobbyMatchInfo* MatchInfo = UTLobbyGameState->AvailableMatches[i];

			if (MatchInfo && MatchInfo->ShouldShowInDock())
			{
				FString GameModeClassname = MatchInfo->CurrentRuleset.IsValid() ? MatchInfo->CurrentRuleset->Data.GameMode : TEXT("");

				int32 CurrentNumPlayers = MatchInfo->NumPlayersInMatch();
				TSharedPtr<FServerInstanceData> Data;
				if (MatchInfo->bDedicatedMatch)
				{
					FString Map = FString::Printf(TEXT("%s (%s)"), *MatchInfo->InitialMap, *MatchInfo->DedicatedServerGameMode);
					// FIXMEJOE - Allow dedicated instances to pass an allowed rank

					Data = FServerInstanceData::Make(MatchInfo->UniqueMatchID, MatchInfo->DedicatedServerName, TEXT(""), GameModeClassname, Map, MatchInfo->DedicatedServerMaxPlayers, MatchInfo->GetMatchFlags(),DEFAULT_RANK_CHECK, false, MatchInfo->bJoinAnytime || !MatchInfo->IsInProgress(), MatchInfo->bSpectatable, MatchInfo->DedicatedServerDescription, MatchInfo->CustomGameName);
				}
				else
				{
					// Attempt to cache the map info
					if (!MatchInfo->InitialMapInfo.IsValid())
					{
						MatchInfo->GetMapInformation();
					}

					FString Map = (MatchInfo->InitialMapInfo.IsValid() ? MatchInfo->InitialMapInfo->Title : MatchInfo->InitialMap);
					Data = FServerInstanceData::Make(MatchInfo->UniqueMatchID, MatchInfo->CurrentRuleset->Data.Title, MatchInfo->CurrentRuleset->Data.UniqueTag, GameModeClassname, Map, MatchInfo->CurrentRuleset->Data.MaxPlayers, MatchInfo->GetMatchFlags(), MatchInfo->RankCheck, MatchInfo->CurrentRuleset->Data.bTeamGame, MatchInfo->bJoinAnytime || !MatchInfo->IsInProgress(), MatchInfo->bSpectatable, MatchInfo->CurrentRuleset->Data.Description, MatchInfo->CustomGameName);
				}

				Data->MatchData = MatchInfo->MatchUpdate;
				Data->MatchData.MatchState = MatchInfo->CurrentState;

				// If this match hasn't started yet, the MatchUpdate will be empty.  So we have to manually fix up the # of players in the match.
				if (!MatchInfo->IsInProgress())
				{
					Data->SetNumPlayers(MatchInfo->NumPlayersInMatch());
					Data->SetNumSpectators(MatchInfo->NumSpectatorsInMatch());
				}

				MatchInfo->GetPlayerData(Data->Players);
				InstanceData.Add(Data);
			}
		}
	}
}

int32 AUTLobbyGameMode::GetNumPlayers()
{
	int32 TotalPlayers = NumPlayers;
	for (int32 i=0;i<UTLobbyGameState->AvailableMatches.Num();i++)
	{
		TotalPlayers += UTLobbyGameState->AvailableMatches[i]->PlayersInMatchInstance.Num();
	}

	return TotalPlayers;
}


int32 AUTLobbyGameMode::GetNumMatches()
{
	int32 Cnt = 0;
	if (UTLobbyGameState && UTLobbyGameState->GameInstances.Num())
	{
		for (int32 i = 0; i < UTLobbyGameState->GameInstances.Num(); i++)
		{
			if (UTLobbyGameState->GameInstances[i].MatchInfo && ( UTLobbyGameState->GameInstances[i].MatchInfo->CurrentState == ELobbyMatchState::InProgress || UTLobbyGameState->GameInstances[i].MatchInfo->CurrentState == ELobbyMatchState::Launching) )
			{
				Cnt++;
			}
		}
	}

	return Cnt;
}

void AUTLobbyGameMode::UpdateLobbySession()
{
	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UpdateGameState();
		}
	}
}

// Lobbies don't want to track inactive players.  We match up players based on a GUID.  
void AUTLobbyGameMode::AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC)
{
	PlayerState->Destroy();
	return;
}

void AUTLobbyGameMode::DefaultTimer()
{
	if (GetWorld()->GetTimeSeconds() > ServerRefreshCheckpoint * 60 * 60)
	{
		if (NumPlayers == 0)
		{

			bool bOkToRestart = true;

			// Look to see if there are any non dedicated instances left standing.
			for (int32 i=0; i < UTLobbyGameState->GameInstances.Num(); i++)
			{
				if (!UTLobbyGameState->GameInstances[i].MatchInfo->bDedicatedMatch)
				{
					bOkToRestart = false;
					break;
				}
			}

			if (bOkToRestart)
			{
				FString MapName = GetWorld()->GetMapName();
				if ( FPackageName::IsShortPackageName(MapName) )
				{
					FPackageName::SearchForPackageOnDisk(MapName, &MapName); 
				}

				AUTGameSessionNonRanked* UTGameSession = Cast<AUTGameSessionNonRanked>(GameSession);
				if (UTGameSession)
				{
					// kill the host beacon before we start the travel so hopefully the port will be released before
					// we are done.
					UTGameSession->DestroyHostBeacon();
				}

				// Shut the hub down and restart.
				FPlatformMisc::RequestExit(false);
			}
		}
	}
}

void AUTLobbyGameMode::SendRconMessage(const FString& DestinationId, const FString &Message)
{	
	Super::SendRconMessage(DestinationId, Message);
	if (UTLobbyGameState)
	{
		for (int32 i=0; i < UTLobbyGameState->AvailableMatches.Num(); i++)
		{
			if (UTLobbyGameState->AvailableMatches[i]->InstanceBeacon)
			{
				UTLobbyGameState->AvailableMatches[i]->InstanceBeacon->Instance_ReceieveRconMessage(DestinationId, Message);
			}
		}
	}
}

void AUTLobbyGameMode::RconKick(const FString& NameOrUIDStr, bool bBan, const FString& Reason)
{
	if (UTLobbyGameState)
	{
		for (int32 i=0; i < UTLobbyGameState->AvailableMatches.Num(); i++)
		{
			if (UTLobbyGameState->AvailableMatches[i]->InstanceBeacon)
			{
				UTLobbyGameState->AvailableMatches[i]->InstanceBeacon->Instance_Kick(NameOrUIDStr);
			}
		}
	}
	Super::RconKick(NameOrUIDStr, bBan, Reason);
}

void AUTLobbyGameMode::RconAuth(AUTBasePlayerController* Admin, const FString& Password)
{
	Super::RconAuth(Admin, Password);
	if (Admin && Admin->UTPlayerState && Admin->UTPlayerState->bIsRconAdmin)
	{
		for (int32 i=0; i < UTLobbyGameState->AvailableMatches.Num(); i++)
		{
			if (UTLobbyGameState->AvailableMatches[i]->InstanceBeacon)
			{
				UTLobbyGameState->AvailableMatches[i]->InstanceBeacon->Instance_AuthorizeAdmin(Admin->UTPlayerState->UniqueId.ToString(), true);
			}
		}
	}

}

void AUTLobbyGameMode::RconNormal(AUTBasePlayerController* Admin)
{
	Super::RconNormal(Admin);

	if (Admin && Admin->UTPlayerState && !Admin->UTPlayerState->bIsRconAdmin)
	{
		for (int32 i=0; i < UTLobbyGameState->AvailableMatches.Num(); i++)
		{
			if (UTLobbyGameState->AvailableMatches[i]->InstanceBeacon)
			{
				UTLobbyGameState->AvailableMatches[i]->InstanceBeacon->Instance_AuthorizeAdmin(Admin->UTPlayerState->UniqueId.ToString(),false);
			}
		}
	}
}

void AUTLobbyGameMode::ReceivedRankForPlayer(AUTPlayerState* UTPlayerState)
{
	AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(UTPlayerState);
	if (LobbyPlayerState)
	{
		UTLobbyGameState->CheckForAutoPlacement(LobbyPlayerState);
	}
}

void AUTLobbyGameMode::MakeJsonReport(TSharedPtr<FJsonObject> JsonObject)
{
	Super::MakeJsonReport(JsonObject);
	JsonObject->SetNumberField(TEXT("TimeUntilRestart"),((ServerRefreshCheckpoint * 60 * 60) - GetWorld()->GetTimeSeconds()));
}

bool AUTLobbyGameMode::SupportsInstantReplay() const
{
	return false;
}

#if !UE_SERVER
TSharedRef<SUTMenuBase> AUTLobbyGameMode::GetGameMenu(UUTLocalPlayer* PlayerOwner) const
{
	return SNew(SUTLobbyMenu).PlayerOwner(PlayerOwner);
}
#endif



