// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/GameState.h"
#include "UTGameState.h"
#include "UTLobbyMatchInfo.h"
#include "UTLobbyGameMode.h"
#include "Net/UnrealNetwork.h"
#include "UTEpicDefaultRulesets.h"
#include "UTServerBeaconLobbyClient.h"
#include "UTMutator.h"
#include "UTAnalytics.h"
#include "UTGameEngine.h"

AUTLobbyGameState::AUTLobbyGameState(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AllMapsOnServerCount = -1;
	AvailabelGameRulesetCount = -1;

	static ConstructorHelpers::FObjectFinder<USoundBase> MenuMusicObj(TEXT("/Game/RestrictedAssets/Audio/Music/Music_UTMuenu_New01.Music_UTMuenu_New01")); 
	LobbyMusic = MenuMusicObj.Object;
}

void AUTLobbyGameState::BeginPlay()
{
	Super::BeginPlay();
	ServerMOTD = ServerMOTD.Replace(TEXT("\\n"), TEXT("\n"), ESearchCase::IgnoreCase);

	if (GetWorld()->GetNetMode() != NM_DedicatedServer && LobbyMusic != nullptr)
	{
		UGameplayStatics::SpawnSoundAtLocation( this, LobbyMusic, FVector(0,0,0), FRotator::ZeroRotator, 1.0, 1.0 );		
	}

}

void AUTLobbyGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTLobbyGameState, AvailableMatches);
	DOREPLIFETIME(AUTLobbyGameState, AvailableGameRulesets);
	DOREPLIFETIME(AUTLobbyGameState, AvailabelGameRulesetCount);
	DOREPLIFETIME(AUTLobbyGameState, AllMapsOnServer);
	DOREPLIFETIME(AUTLobbyGameState, AllMapsOnServerCount);
	DOREPLIFETIME(AUTLobbyGameState, NumGameInstances);
	DOREPLIFETIME(AUTLobbyGameState, bCustomContentAvailable);
	DOREPLIFETIME_CONDITION(AUTLobbyGameState, bAllowInstancesToStartWithBots, COND_InitialOnly);
}

void AUTLobbyGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	AUTLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();

	if (Role == ROLE_Authority)
	{
		bCustomContentAvailable = LobbyGameMode ? LobbyGameMode->RedirectReferences.Num() > 0 : 0;
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTLobbyGameState::CheckInstanceHealth, 60.0f, true);	
	
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(RuleWaitHandle, this, &AUTLobbyGameState::RulesetsAreLoaded, 1.5f, true);	
		}
	}
}

void AUTLobbyGameState::RulesetsAreLoaded()
{

	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetWorld()->GetGameInstance());

	// If we aren't authority, or if the GameEngine hasn't received it's title files yet, just exit out.
	if (UTGameInstance == nullptr || Role != ROLE_Authority || !UTGameInstance->bReceivedTitleFiles) return;

	// Clear the timer that got us here
	GetWorldTimerManager().ClearTimer(RuleWaitHandle);


	// Grab all of the available map assets.
	TArray<FAssetData> MapAssets;
	FindAllPlayableMaps(MapAssets);

	for (int32 i=0; i < UTGameInstance->GameRulesets.Num(); i++)
	{
		if (!UTGameInstance->GameRulesets[i].bHideFromUI)
		{
			bool bExistsAlready = false;
			for (int32 j=0; j < AvailableGameRulesets.Num(); j++)
			{
				if ( AvailableGameRulesets[j]->Data.UniqueTag.Equals(UTGameInstance->GameRulesets[i].UniqueTag, ESearchCase::IgnoreCase) || AvailableGameRulesets[j]->Data.Title.ToLower() == UTGameInstance->GameRulesets[i].Title.ToLower() )
				{
					bExistsAlready = true;
					break;
				}
			}

			if ( !bExistsAlready )
			{
				FActorSpawnParameters Params;
				Params.Owner = this;
				AUTReplicatedGameRuleset* NewReplicatedRuleset = GetWorld()->SpawnActor<AUTReplicatedGameRuleset>(Params);
				if (NewReplicatedRuleset)
				{
					// Build out the map info
					NewReplicatedRuleset->SetRules(UTGameInstance->GameRulesets[i], MapAssets);

					// If this ruleset doesn't have any maps, then don't use it
					if (NewReplicatedRuleset->MapList.Num() > 0)
					{
						AvailableGameRulesets.Add(NewReplicatedRuleset);
					}
					else
					{
						UE_LOG(UT,Warning,TEXT("Detected a ruleset [%s] that has no maps"), *UTGameInstance->GameRulesets[i].UniqueTag);
						NewReplicatedRuleset->Destroy();
					}
				}
			}
			else
			{
				UE_LOG(UT,Verbose,TEXT("Rule already exists."));
			}
		}
	}
	AvailabelGameRulesetCount = AvailableGameRulesets.Num();
	AllMapsOnServerCount = AllMapsOnServer.Num();
}
	
void AUTLobbyGameState::CheckInstanceHealth()
{
	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		AUTLobbyMatchInfo* MatchInfo = AvailableMatches[i];
		if (MatchInfo)
		{
			if (MatchInfo->GameInstanceProcessHandle.IsValid())
			{
				if ( (MatchInfo->CurrentState == ELobbyMatchState::InProgress || MatchInfo->CurrentState == ELobbyMatchState::Recycling) && !FPlatformProcess::IsProcRunning(MatchInfo->GameInstanceProcessHandle) )
				{
					UE_LOG(UT, Log, TEXT("Recycling game instance that no longer has a process"));
					MatchInfo->SetLobbyMatchState(ELobbyMatchState::Recycling);
					ProcessesToGetReturnCode.Add(MatchInfo->GameInstanceProcessHandle);
					MatchInfo->SetLobbyMatchState(ELobbyMatchState::Recycling);
					MatchInfo->GameInstanceProcessHandle.Reset();
				}
			}
			else
			{
				if (MatchInfo->CurrentState == ELobbyMatchState::InProgress && !MatchInfo->bDedicatedMatch)
				{
					UE_LOG(UT,Warning,TEXT("Terminating an invalid match that is inprogress"));
					MatchInfo->SetLobbyMatchState(ELobbyMatchState::Recycling);
				}
			}
		}
	}

	for (int32 i=0; i<GameInstances.Num();i++)
	{
		if ( (GameInstances[i].MatchInfo && GameInstances[i].MatchInfo->CurrentState == ELobbyMatchState::Dead) ||
			 (GameInstances[i].MatchInfo && GameInstances[i].MatchInfo->CurrentState == ELobbyMatchState::Launching && GetWorld()->GetRealTimeSeconds() - GameInstances[i].MatchInfo->InstanceLaunchTime > 600) )
		{
			RemoveMatch(GameInstances[i].MatchInfo);
		}
	}

	for (int32 i = ProcessesToGetReturnCode.Num() - 1; i >= 0; i--)
	{
		int32 ReturnCode = 0;
		if (FPlatformProcess::GetProcReturnCode(ProcessesToGetReturnCode[i], &ReturnCode))
		{
			UE_LOG(UT, Log, TEXT("Waited on process successfully"));
			ProcessesToGetReturnCode.RemoveAt(i);
		}
		else
		{
			UE_LOG(UT, Log, TEXT("Failed to get process return code, will try again"));
		}
	}
}


void AUTLobbyGameState::BroadcastChat(AUTLobbyPlayerState* SenderPS, FName Destination, const FString& Message)
{
	for (int32 i = 0; i < PlayerArray.Num(); i++)
	{
		AUTLobbyPlayerState* PS = Cast<AUTLobbyPlayerState>(PlayerArray[i]);
		if (PS)	
		{
			AUTBasePlayerController* BasePC = Cast<AUTBasePlayerController>(PS->GetOwner());
			if (BasePC != NULL)
			{
				if (Destination != ChatDestinations::Match || (SenderPS->CurrentMatch && PS->CurrentMatch == SenderPS->CurrentMatch))
				{
					BasePC->ClientSay(SenderPS, Message, Destination);
				}
			}
		}
	}
}

AUTLobbyMatchInfo* AUTLobbyGameState::FindMatchPlayerIsIn(FString PlayerID)
{
	for (int32 i=0;i<AvailableMatches.Num();i++)
	{
		if ( AvailableMatches[i] )
		{
			if (AvailableMatches[i]->CurrentState == ELobbyMatchState::WaitingForPlayers ||
				AvailableMatches[i]->CurrentState != ELobbyMatchState::Launching ||
				AvailableMatches[i]->CurrentState != ELobbyMatchState::InProgress)	
			{
				for (int32 j=0;j<AvailableMatches[i]->Players.Num();j++)
				{
					if (AvailableMatches[i]->Players[j].IsValid())
					{
						if (AvailableMatches[i]->Players[j]->UniqueId.IsValid() && AvailableMatches[i]->Players[j]->UniqueId.ToString() == PlayerID)
						{
							return AvailableMatches[i];
						}
					}
				}

				for (int32 j=0;j<AvailableMatches[i]->PlayersInMatchInstance.Num();j++)
				{
					if (AvailableMatches[i]->PlayersInMatchInstance[j].PlayerID.IsValid() && AvailableMatches[i]->PlayersInMatchInstance[j].PlayerID.ToString() == PlayerID)
					{
						return AvailableMatches[i];
					}
				}
			}


		}

	}

	return NULL;
}

void AUTLobbyGameState::CheckForAutoPlacement(AUTLobbyPlayerState* NewPlayer)
{ 
	// We are looking to join a player's match.. see if we can find the player....
	if (!NewPlayer->DesiredFriendToJoin.IsEmpty())
	{
		AUTLobbyMatchInfo* FriendsMatch = FindMatchPlayerIsIn(NewPlayer->DesiredFriendToJoin);
		if (FriendsMatch)
		{
			JoinMatch(FriendsMatch, NewPlayer);
		}
	}

	// Otherwise, look to see if we want to join a match.
	else if (!NewPlayer->DesiredMatchIdToJoin.IsEmpty())
	{
		AttemptDirectJoin(NewPlayer, NewPlayer->DesiredMatchIdToJoin, NewPlayer->bDesiredJoinAsSpectator);
	}
}

TWeakObjectPtr<AUTReplicatedGameRuleset> AUTLobbyGameState::FindRuleset(FString TagToFind)
{
	for (int32 i=0; i < AvailableGameRulesets.Num(); i++)
	{
		if ( AvailableGameRulesets[i] != nullptr && AvailableGameRulesets[i]->Data.UniqueTag.Equals(TagToFind, ESearchCase::IgnoreCase) )
		{
			return AvailableGameRulesets[i];
		}
	}

	return NULL;
}

void AUTLobbyGameState::JoinMatch(AUTLobbyMatchInfo* MatchInfo, AUTLobbyPlayerState* NewPlayer, bool bAsSpectator)
{
	if (!NewPlayer->bIsRconAdmin)
	{
		if (MatchInfo->bDedicatedMatch)
		{
			MatchInfo->AddPlayer(NewPlayer);
			NewPlayer->ClientConnectToInstance(MatchInfo->GameInstanceGUID, NewPlayer->DesiredTeamNum, bAsSpectator);
			return;	
		}
		if (MatchInfo->IsBanned(NewPlayer->UniqueId))
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage","Banned","You do not have permission to enter this match."));
			return;
		}

		if (MatchInfo->IsPrivateMatch())
		{
			// Look to see if this player has the key to the match
	
			if (!NewPlayer->PartyLeader.Equals(MatchInfo->OwnerId.ToString(),ESearchCase::IgnoreCase) && MatchInfo->AllowedPlayerList.Find(NewPlayer->UniqueId.ToString()) == INDEX_NONE)
			{
				NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage","Private","Sorry, but the match you are trying to join is private."));
				return;
			}
		}
	}

	AUTGameMode* UTGame = MatchInfo->CurrentRuleset.IsValid() ? MatchInfo->CurrentRuleset->GetDefaultGameModeObject() : AUTGameMode::StaticClass()->GetDefaultObject<AUTGameMode>();
	int32 PlayerRankCheck = NewPlayer->GetRankCheck(UTGame);
	if (!bAsSpectator && !MatchInfo->SkillTest(PlayerRankCheck)) 
	{
		if (PlayerRankCheck > MatchInfo->RankCheck)
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage", "MatchTooGood", "Your skill rating is too high for this match."));
		}
		else
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage", "MatchTooLow", "Your skill rating is too low for this match."));
		}
		return;
	}

	if (MatchInfo->CurrentState == ELobbyMatchState::Launching)
	{
/*
		if (bAsSpectator)
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage","MatchIsStartingSpectate","The match you are trying to spectate is starting.  Please try again after it launches."));	
		}
		else
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage","MatchIsStarting","The match you are trying to join is starting.  Please try again after it launches."));	
		}
		return;
*/
	}
	else if (MatchInfo->CurrentState == ELobbyMatchState::InProgress)
	{
		if (!MatchInfo->bJoinAnytime && !bAsSpectator)
		{
			NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage", "MatchIsNotJoinable", "The match you are trying to join does not allow join in progress."));
			return;
		}
		AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
		if (GM && MatchInfo->MatchHasRoom(bAsSpectator) )
		{
			MatchInfo->AddPlayer(NewPlayer);
			NewPlayer->ClientConnectToInstance(MatchInfo->GameInstanceGUID, NewPlayer->DesiredTeamNum, bAsSpectator);
			return;
		}

		NewPlayer->ClientMatchError(NSLOCTEXT("LobbyMessage","MatchIsFull","The match you are trying to join is full."));	
		return;
	}

	MatchInfo->AddPlayer(NewPlayer, false, bAsSpectator);
}

void AUTLobbyGameState::RemoveFromAMatch(AUTLobbyPlayerState* PlayerOwner)
{
	AUTLobbyMatchInfo* Match;
	Match = PlayerOwner->CurrentMatch;
	if (Match)
	{
		// Trigger all players being removed
		if (Match->RemovePlayer(PlayerOwner))
		{
			RemoveMatch(Match);
		}
	}
}

void AUTLobbyGameState::RemoveMatch(AUTLobbyMatchInfo* MatchToRemove)
{
	// Look to see if anyone else is in the match...

	// Kill any game instances.
	TerminateGameInstance(MatchToRemove);

	// Match is dead....
	AvailableMatches.Remove(MatchToRemove);
}

void AUTLobbyGameState::AdminKillMatch(AUTLobbyMatchInfo* MatchToRemove)
{
	if (MatchToRemove->IsInProgress() && MatchToRemove->InstanceBeacon)
	{
		MatchToRemove->InstanceBeacon->Instance_ForceShutdown();
	}
	else
	{
		if (MatchToRemove->Players.Num() >0)
		{
			RemoveFromAMatch(MatchToRemove->Players[0].Get()); // Kill the host which will kill the match
		}
		else
		{
			RemoveMatch(MatchToRemove);
		}
	}
}


void AUTLobbyGameState::SortPRIArray()
{
}

void AUTLobbyGameState::SetupLobbyBeacons()
{
	UWorld* const World = GetWorld();

	// Always create a new beacon host
	LobbyBeacon_Listener = World->SpawnActor<AUTServerBeaconLobbyHostListener>(AUTServerBeaconLobbyHostListener::StaticClass());

	if (LobbyBeacon_Listener && LobbyBeacon_Listener->InitHost())
	{
		LobbyBeacon_Listener->PauseBeaconRequests(false);
		LobbyBeacon_Object = World->SpawnActor<AUTServerBeaconLobbyHostObject>(AUTServerBeaconLobbyHostObject::StaticClass());

		if (LobbyBeacon_Object)
		{
			LobbyBeacon_Listener->RegisterHost(LobbyBeacon_Object);
			GameInstanceListenPort = LobbyBeacon_Listener->ListenPort;
			
			UE_LOG(UT,Log,TEXT("---------------------------------------"));
			UE_LOG(UT,Log,TEXT("Listing for instances on port %i"), GameInstanceListenPort);
			UE_LOG(UT,Log,TEXT("---------------------------------------"));

			if (AUTServerBeaconLobbyHostListener::StaticClass()->GetDefaultObject<AUTServerBeaconLobbyHostListener>()->ListenPort != GameInstanceListenPort)
			{
				UE_LOG(UT,Warning,TEXT("Game could not bind to the proper listen port."));
			}

			return;
		}
	}

	UE_LOG(UT,Log,TEXT("Could not create Lobby Beacons"));
}

void AUTLobbyGameState::LaunchGameInstance(AUTLobbyMatchInfo* MatchOwner, FString GameURL)
{
	AUTLobbyGameMode* LobbyGame = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();

	if (LobbyGame && MatchOwner && MatchOwner->CurrentRuleset.IsValid())
	{
		if (MatchOwner->GameInstanceProcessHandle.IsValid())
		{
			UE_LOG(UT, Warning, TEXT("Attempted to Launch a game instance when an instance already has a proc id.  Ignoring."));
			return;
		}

		GameInstanceID++;
		if (GameInstanceID == 0) GameInstanceID = 1;	// Always skip 0.

		// Apply additional options.
		if (!ForcedInstanceGameOptions.IsEmpty()) GameURL += ForcedInstanceGameOptions;

		GameURL += FString::Printf(TEXT("?InstanceID=%i?HostPort=%i"), GameInstanceID, GameInstanceListenPort);

		if (MatchOwner->bRankLocked)
		{
			GameURL += FString::Printf(TEXT("?RankCheck=%i"), MatchOwner->RankCheck);
		}

		if (MatchOwner->bPrivateMatch)
		{
			GameURL += TEXT("?Private=1");
		}

		if (!LobbyGame->ServerPassword.IsEmpty())
		{
			GameURL += FString::Printf(TEXT("?ServerPassword=%s"), *LobbyGame->ServerPassword);
		}

		GameURL += FString::Printf(TEXT("?HubGUID=%s"), *LobbyGame->ServerInstanceGUID.ToString());

		int32 InstancePort = LobbyGame->StartingInstancePort + (LobbyGame->InstancePortStep * GameInstances.Num());

		FGuid LaunchGuid = FGuid::NewGuid();
		FString InstanceLogFile = FString::Printf(TEXT("Instance_%s.log"), *LaunchGuid.ToString());

		FString ExecPath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
		FString Options = FString::Printf(TEXT("UnrealTournament %s -server -port=%i -log -log=%s"), *GameURL, InstancePort, *InstanceLogFile);
		
		// Add in additional command line params
		if (!AdditionalInstanceCommandLine.IsEmpty()) Options += TEXT(" ") + AdditionalInstanceCommandLine;
		
		FString SLevel;
		if (FParse::Value(FCommandLine::Get(), TEXT("SLevel="), SLevel))
		{
			Options += TEXT(" -SLevel=") + SLevel;
		}

		FString BuildIdOverride;
		if (FParse::Value(FCommandLine::Get(), TEXT("BuildIdOverride="), BuildIdOverride))
		{
			Options += TEXT(" -BuildIdOverride=") + BuildIdOverride;
		}

		FString StatsPort;
		if (FParse::Value(FCommandLine::Get(), TEXT("StatsPort="), StatsPort))
		{
			int32 NewPort = FCString::Atoi(*StatsPort) + 1;
			bool bFound = false;
			for (int32 i=0; i < GameInstances.Num(); i++)
			{
				if (GameInstances[i].MatchInfo->StatsPort == NewPort)
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				MatchOwner->StatsPort = NewPort;
				Options += FString::Printf(TEXT(" -statsport=%i"), NewPort);
			}
		}

		FString EpicAppValue;
		if (FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), EpicAppValue))
		{
			Options += TEXT(" -EpicApp=") + EpicAppValue;
		}

		UE_LOG(UT,Verbose,TEXT("Launching %s with Params %s"), *ExecPath, *Options);

		if (FUTAnalytics::IsAvailable())
		{
			FUTAnalytics::FireEvent_UTHubNewInstance(MatchOwner, MatchOwner->OwnerId.ToString());
		}

		MatchOwner->GameInstanceProcessHandle = FPlatformProcess::CreateProc(*ExecPath, *Options, true, false, false, NULL, 0, NULL, NULL);

		if (MatchOwner->GameInstanceProcessHandle.IsValid())
		{
			GameInstances.Add(FGameInstanceData(MatchOwner, InstancePort));
			NumGameInstances = GameInstances.Num();
			MatchOwner->SetLobbyMatchState(ELobbyMatchState::Launching);
			MatchOwner->InstanceLaunchTime = GetWorld()->GetRealTimeSeconds();
			MatchOwner->GameInstanceID = GameInstanceID;
		}
		else
		{
			UE_LOG(UT,Warning,TEXT("Could not start an instance (the ProcHandle is Invalid)"));
			if (MatchOwner)
			{
				TWeakObjectPtr<AUTLobbyPlayerState> OwnerPS = MatchOwner->GetOwnerPlayerState();
				if (OwnerPS.IsValid())
				{
					OwnerPS->ClientMatchError(NSLOCTEXT("LobbyMessage", "FailedToStart", "Unfortunately, the server had trouble starting a new instance.  Please try again in a few moments."));
				}
			}
		}
	}

	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		GM->UpdateLobbySession();
	}
}

void AUTLobbyGameState::TerminateGameInstance(AUTLobbyMatchInfo* MatchOwner, bool bAborting)
{
	if (FUTAnalytics::IsAvailable() && MatchOwner)
	{
		FUTAnalytics::FireEvent_UTHubInstanceClosing(MatchOwner, MatchOwner->OwnerId.ToString());
	}

	if (MatchOwner->GameInstanceProcessHandle.IsValid())
	{
		// if we have an active game instance that is coming up but we have not started the travel to it yet, Kill the instance.
		if (FPlatformProcess::IsProcRunning(MatchOwner->GameInstanceProcessHandle))
		{
			FPlatformProcess::TerminateProc(MatchOwner->GameInstanceProcessHandle);
		}
			
		ProcessesToGetReturnCode.Add(MatchOwner->GameInstanceProcessHandle);
		MatchOwner->SetLobbyMatchState(bAborting ? ELobbyMatchState::WaitingForPlayers : ELobbyMatchState::Recycling);

		MatchOwner->GameInstanceProcessHandle.Reset();
		MatchOwner->GameInstanceID = 0;
	}

	int32 InstanceIndex = -1;
	for (int32 i = 0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo == MatchOwner)
		{
			GameInstances.RemoveAt(i);
			NumGameInstances = GameInstances.Num();
			break;
		}
	}

	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		GM->UpdateLobbySession();
	}
}


void AUTLobbyGameState::GameInstance_Ready(uint32 InGameInstanceID, FGuid GameInstanceGUID, const FString& MapName, AUTServerBeaconLobbyClient* InstanceBeacon)
{
	for (int32 i=0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID)
		{
			GameInstances[i].MatchInfo->GameInstanceReady(GameInstanceGUID);
			GameInstances[i].MatchInfo->InitialMap = MapName;
			GameInstances[i].MatchInfo->InstanceBeacon = InstanceBeacon;

			// Load the map info
			GameInstances[i].MatchInfo->GetMapInformation();

			break;
		}
	}
}

void AUTLobbyGameState::GameInstance_MatchUpdate(uint32 InGameInstanceID, const FMatchUpdate& MatchUpdate)
{
	for (int32 i = 0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID)
		{
			GameInstances[i].MatchInfo->ProcessMatchUpdate(MatchUpdate);
			break;
		}
	}
}

void AUTLobbyGameState::GameInstance_PlayerUpdate(uint32 InGameInstanceID, const FRemotePlayerInfo& PlayerInfo, bool bLastUpdate)
{
	// Find the match
	for (int32 i = 0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID)
		{
			// Look through the players in the match
			AUTLobbyMatchInfo* Match = GameInstances[i].MatchInfo;
			AUTLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
			if (Match)
			{
				for (int32 j=0; j < Match->PlayersInMatchInstance.Num(); j++)
				{
					if (Match->PlayersInMatchInstance[j].PlayerID == PlayerInfo.PlayerID)
					{
						if (bLastUpdate)
						{
							Match->PlayersInMatchInstance.RemoveAt(j,1);

							// Update the Game state.
							if (LobbyGameMode)
							{
								AUTGameSession* UTGameSession = Cast<AUTGameSession>(LobbyGameMode->GameSession);
								if (UTGameSession) UTGameSession->UpdateGameState();
							}
							return;
						}
						else
						{

							Match->PlayersInMatchInstance[j].Update(PlayerInfo);
							return;
						}
						break;
					}
				}
	
				// A player not in the instance table.. add them
				Match->PlayersInMatchInstance.Add( FRemotePlayerInfo(PlayerInfo) );

				// Update the Game state.
				if (LobbyGameMode)
				{
					AUTGameSession* UTGameSession = Cast<AUTGameSession>(LobbyGameMode->GameSession);
					if (UTGameSession) UTGameSession->UpdateGameState();
				}
			}
		}
	}
}

void AUTLobbyGameState::GameInstance_StartGame(uint32 InGameInstanceID, const FMatchUpdate& MatchUpdate)
{
	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		for (int32 i = 0; i < GameInstances.Num(); i++)
		{
			if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID)
			{
				GameInstances[i].MatchInfo->ProcessStartMatch(MatchUpdate);
				break;
			}
		}
	}
}


void AUTLobbyGameState::GameInstance_EndGame(uint32 InGameInstanceID, const FMatchUpdate& FinalMatchUpdate)
{
	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		GM->UpdateLobbySession();

		for (int32 i = 0; i < GameInstances.Num(); i++)
		{
			if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID)
			{
				GameInstances[i].MatchInfo->ProcessMatchUpdate(FinalMatchUpdate);
				break;
			}
		}
	}
}

void AUTLobbyGameState::GameInstance_Empty(uint32 InGameInstanceID)
{
	for (int32 i = 0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->GameInstanceID == InGameInstanceID && !GameInstances[i].MatchInfo->bDedicatedMatch)
		{
			// Set the match info's state to recycling so all returning players will be directed properly.
			GameInstances[i].MatchInfo->SetLobbyMatchState(ELobbyMatchState::Recycling);

			// Terminate the game instance
			TerminateGameInstance(GameInstances[i].MatchInfo);
			break;
		}
	}
}

bool AUTLobbyGameState::IsMatchStillValid(AUTLobbyMatchInfo* TestMatch)
{
	for (int32 i=0;i<AvailableMatches.Num();i++)
	{
		if (AvailableMatches[i] && AvailableMatches[i] == TestMatch) return true;
	}

	return false;
}


bool AUTLobbyGameState::CanLaunch()
{
	AUTLobbyGameMode* GM = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	return (GM && GM->GetNumMatches() < GM->MaxInstances);
}


void AUTLobbyGameState::ScanAssetRegistry(TArray<FAssetData>& MapAssets)
{
	UE_LOG(UT,Verbose,TEXT("Beginning Lobby Scan of Asset Registry to generate custom settings list...."));

	TArray<UClass*> AllowedGameModesClasses;
	TArray<UClass*> AllowedMutatorsClasses;

	AUTGameState::GetAvailableGameData(AllowedGameModesClasses, AllowedMutatorsClasses);
	for (int32 i = 0; i < AllowedGameModesClasses.Num(); i++)
	{
		AllowedGameData.Add(FAllowedData(EGameDataType::GameMode, AllowedGameModesClasses[i]->GetPathName()));
	}

	for (int32 i = 0; i < AllowedMutatorsClasses.Num(); i++)
	{
		AllowedGameData.Add(FAllowedData(EGameDataType::Mutator, AllowedMutatorsClasses[i]->GetPathName()));
	}

	// Next , Grab the maps...
	GetAllAssetData(UWorld::StaticClass(), MapAssets);
	for (const FAssetData& Asset : MapAssets)
	{
		FString MapPackageName = Asset.PackageName.ToString();
		if (!MapPackageName.StartsWith(TEXT("/Engine/")) && IFileManager::Get().FileSize(*FPackageName::LongPackageNameToFilename(MapPackageName, FPackageName::GetMapPackageExtension())) > 0)
		{
			AllowedGameData.Add(FAllowedData( EGameDataType::Map, MapPackageName));
		}
	}

	UE_LOG(UT,Verbose,TEXT("Scan Complete!!"))

}

void AUTLobbyGameState::ClientAssignGameData(FAllowedData Data)
{
	if (Data.DataType == EGameDataType::GameMode) AllowedGameModes.Add(Data);
	else if (Data.DataType == EGameDataType::Mutator) AllowedMutators.Add(Data);
	else if (Data.DataType == EGameDataType::Map) AllowedMaps.Add(Data);
	else
	{
		UE_LOG(UT,Log,TEXT("ClientAssignGameData received bad data"));
	}
}

void AUTLobbyGameState::GetAvailableGameData(TArray<UClass*>& GameModes, TArray<UClass*>& MutatorList)
{
	Super::GetAvailableGameData(GameModes, MutatorList);
	// Now process it against the allowed list....
	
	int32 GIndex = 0;
	while (GIndex < GameModes.Num())
	{
		bool bFound = false;
		for (int32 i=0; i < AllowedGameModes.Num(); i++)
		{
			if (GameModes[GIndex]->GetPathName() == AllowedGameModes[i].PackageName)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			GIndex++;
		}
		else
		{
			GameModes.RemoveAt(GIndex);
		}
	}

	int32 MIndex = 0;
	while (MIndex < MutatorList.Num())
	{
		bool bFound = false;
		for (int32 i=0; i < AllowedMutators.Num(); i++)
		{
			if (MutatorList[MIndex]->GetPathName() == AllowedMutators[i].PackageName)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			MIndex++;
		}
		else
		{
			MutatorList.RemoveAt(MIndex);
		}
	}
}

AUTReplicatedMapInfo* AUTLobbyGameState::GetMapInfo(FString MapPackageName)
{
	for (int32 j = 0; j < AllMapsOnServer.Num(); j++)
	{
		if ( (AllMapsOnServer[j] && AllMapsOnServer[j]->MapPackageName == MapPackageName) ||
			 (AllMapsOnServer[j] && AllMapsOnServer[j]->MapAssetName == MapPackageName) )

		{
			return AllMapsOnServer[j];
		}
	}

	return NULL;
}

/**
 *	Finds all of the playerable maps on the server and builds the needed ReplicatedMapInfo for that map.
 **/
void AUTLobbyGameState::FindAllPlayableMaps(TArray<FAssetData>& MapAssets)
{
	if (Role == ROLE_Authority)
	{
		ScanAssetRegistry(MapAssets);
		for (const FAssetData& Asset : MapAssets)
		{
			AUTReplicatedMapInfo* MapInfo = CreateMapInfo(Asset);
			if (MapInfo)
			{
				AllMapsOnServer.Add( MapInfo );
			}
		}
	}
}

/**
 * 
 **/
void AUTLobbyGameState::GetMapList(const TArray<FString>& AllowedMapPrefixes, TArray<AUTReplicatedMapInfo*>& MapList, bool bUseCache)
{
	for (int32 i = 0; i < AllMapsOnServer.Num(); i++)
	{
		if ( AllMapsOnServer[i] )
		{
			bool bFound = AllowedMapPrefixes.Num() == 0;
			for (int32 j = 0; j < AllowedMapPrefixes.Num(); j++)
			{
				if ( AllMapsOnServer[i]->MapAssetName.StartsWith(AllowedMapPrefixes[j] + TEXT("-")) )
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				MapList.AddUnique(AllMapsOnServer[i]);
			}
		}
	}
}

AUTReplicatedMapInfo* AUTLobbyGameState::CreateMapInfo(const FAssetData& MapAsset)
{
	// Look to see if there is a cached version of this map info first before creating one.
	for (int32 i = 0; i < AllMapsOnServer.Num(); i++)
	{
		if (AllMapsOnServer[i]->MapPackageName == MapAsset.PackageName.ToString())
		{
			return AllMapsOnServer[i];
		}
	}

	return Super::CreateMapInfo(MapAsset);	
}


void AUTLobbyGameState::AuthorizeDedicatedInstance(AUTServerBeaconLobbyClient* Beacon, FGuid InstanceGUID, const FString& HubKey, const FString& InServerName, const FString& ServerGameMode, const FString& InServerDescription, int32 MaxPlayers, bool InbTeamGame)
{
	// Look through the current matches to see if this key is in use

	AUTLobbyGameMode* LobbyGame = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (LobbyGame == nullptr) return;


	UE_LOG(UT,Verbose,TEXT("...Checking for existing instance!"));

	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		if (AvailableMatches[i]->bDedicatedMatch && AvailableMatches[i]->AccessKey.Equals(HubKey, ESearchCase::CaseSensitive))
		{
			// There is an existing match.  We need to kill it and add this new one.  This can happen if the
			// dedicated instance crashes.

			UE_LOG(UT,Verbose,TEXT("...Found one and removed."));
			RemoveMatch(AvailableMatches[i]);
			break;
		}
	}

	UE_LOG(UT,Verbose,TEXT("...Check For Key Access (%i)."),AccessKeys.Num());;

	// Look to see if this hub has a valid access key
	for (int32 i=0; i < AccessKeys.Num(); i++)
	{
		UE_LOG(UT,Verbose,TEXT("... Testing (%s) vs (%s)."),*AccessKeys[i], *HubKey);
		if (AccessKeys[i].Equals(HubKey, ESearchCase::CaseSensitive))
		{
			UE_LOG(UT,Verbose,TEXT("... Adding Instance"),*AccessKeys[i], *HubKey);
			// Yes.. take the key and build a dedicated instance.

			if ( AddDedicatedInstance(InstanceGUID, HubKey, InServerName, ServerGameMode, InServerDescription, MaxPlayers, InbTeamGame) )
			{
				UE_LOG(UT,Verbose,TEXT("... !!! Authorized !!!"),*AccessKeys[i], *HubKey);
				// authorize the instance and pass my ServerInstanceGUID
				Beacon->AuthorizeDedicatedInstance(LobbyGame->ServerInstanceGUID, GameInstanceID);
			}
			break;
		}
	}
}

bool AUTLobbyGameState::AddDedicatedInstance(FGuid InstanceGUID, const FString& AccessKey, const FString& InServerName, const FString& ServerGameMode, const FString& InServerDescription, int32 MaxPlayers, bool InbTeamGame)
{
	// Create the Match info...
	AUTLobbyMatchInfo* NewMatchInfo = GetWorld()->SpawnActor<AUTLobbyMatchInfo>();
	if (NewMatchInfo)
	{	
		GameInstanceID++;
		if (GameInstanceID == 0) GameInstanceID = 1;	// Always skip 0.

		UE_LOG(UT,Verbose,TEXT("... Creating Dedicated Instance data for InstanceId %i"), GameInstanceID)

		AvailableMatches.Add(NewMatchInfo);
		GameInstances.Add(FGameInstanceData(NewMatchInfo, 7777));
		NumGameInstances = GameInstances.Num();

		NewMatchInfo->SetOwner(this);
		NewMatchInfo->GameInstanceID = GameInstanceID;
		NewMatchInfo->bDedicatedMatch = true;
		NewMatchInfo->AccessKey = AccessKey;
		NewMatchInfo->DedicatedServerName = InServerName;
		NewMatchInfo->DedicatedServerGameMode = ServerGameMode;
		NewMatchInfo->DedicatedServerDescription = InServerDescription;
		NewMatchInfo->CustomGameName = InServerDescription;
		NewMatchInfo->DedicatedServerMaxPlayers = MaxPlayers;
		NewMatchInfo->bDedicatedTeamGame = InbTeamGame;
		NewMatchInfo->GameInstanceGUID = InstanceGUID.ToString();
		NewMatchInfo->SetLobbyMatchState(ELobbyMatchState::InProgress);
		return true;
	}			

	return false;
}

AUTLobbyMatchInfo* AUTLobbyGameState::FindMatch(FGuid MatchIDIn)
{
	for (int32 i=0; i<AvailableMatches.Num(); i++)
	{
		if (AvailableMatches[i]->UniqueMatchID == MatchIDIn) return AvailableMatches[i];
	}

	for (int32 i=0; i<GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->UniqueMatchID == MatchIDIn && GameInstances[i].MatchInfo->CurrentState == ELobbyMatchState::InProgress) return GameInstances[i].MatchInfo;
	}

	return NULL;
}

void AUTLobbyGameState::RequestInstanceJoin(AUTServerBeaconClient* Beacon, const FString& InstanceId, bool bSpectator, int32 Rank)
{
	AUTLobbyMatchInfo* DestinationMatchInfo = NULL;
	// Look to see if this instance is still in the start up phase...
	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		if (AvailableMatches[i] && (AvailableMatches[i]->CurrentState == ELobbyMatchState::WaitingForPlayers || AvailableMatches[i]->CurrentState == ELobbyMatchState::Launching))
		{
			if (AvailableMatches[i]->UniqueMatchID.ToString() == InstanceId)
			{
				DestinationMatchInfo = AvailableMatches[i];
				break;
			}
		}
	}

	if (DestinationMatchInfo == NULL)
	{

		for (int32 i=0; i < GameInstances.Num(); i++)
		{
			if (GameInstances[i].MatchInfo->UniqueMatchID.ToString() == InstanceId)
			{
				DestinationMatchInfo = GameInstances[i].MatchInfo;
				break;
			}
		}
	}

	if (DestinationMatchInfo && (DestinationMatchInfo->CurrentState == ELobbyMatchState::InProgress || DestinationMatchInfo->CurrentState != ELobbyMatchState::WaitingForPlayers || DestinationMatchInfo->CurrentState != ELobbyMatchState::Launching) )
	{
		if (bSpectator || DestinationMatchInfo->SkillTest(Rank,false))
		{
			if (!DestinationMatchInfo->bPrivateMatch)
			{
				if (!bSpectator || DestinationMatchInfo->bSpectatable)
				{
					// Add checks for passworded and friends only matches..

					if (DestinationMatchInfo->CurrentState == ELobbyMatchState::InProgress)
					{
						if (DestinationMatchInfo->bJoinAnytime || bSpectator)
						{
							Beacon->ClientRequestInstanceResult(EInstanceJoinResult::JoinDirectly, DestinationMatchInfo->GameInstanceGUID);									
						}
						else
						{
							Beacon->ClientRequestInstanceResult(EInstanceJoinResult::MatchLocked, TEXT(""));
						}
					}
					else
					{
						Beacon->ClientRequestInstanceResult(EInstanceJoinResult::JoinViaLobby, DestinationMatchInfo->UniqueMatchID.ToString());									
					}
				}
				else
				{
					Beacon->ClientRequestInstanceResult(EInstanceJoinResult::MatchLocked, TEXT(""));
				}
			}
			else
			{
				Beacon->ClientRequestInstanceResult(EInstanceJoinResult::MatchLocked, TEXT(""));
			}
		}
		else
		{
			Beacon->ClientRequestInstanceResult(EInstanceJoinResult::MatchRankFail, TEXT(""));
		}
	}
	else
	{
		// Let the user know the match no longer exists....
		Beacon->ClientRequestInstanceResult(EInstanceJoinResult::MatchNoLongerExists, TEXT(""));
	}
}

void AUTLobbyGameState::FillOutRconPlayerList(TArray<FRconPlayerData>& PlayerList)
{
	Super::FillOutRconPlayerList(PlayerList);

	for (int32 MatchIndex = 0; MatchIndex < AvailableMatches.Num(); MatchIndex++)
	{
		AUTLobbyMatchInfo* Match = AvailableMatches[MatchIndex];
		if (Match)
		{
			for (int32 i = 0 ; i < Match->PlayersInMatchInstance.Num(); i++)
			{
				FString PlayerID = Match->PlayersInMatchInstance[i].PlayerID.ToString();
				bool bFound = false;
				for (int32 j = 0; j < PlayerList.Num(); j++)
				{
					if (PlayerList[j].PlayerID == PlayerID)
					{
						PlayerList[j].bPendingDelete = false;
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					int32 Rank = Match->PlayersInMatchInstance[i].RankCheck;
					FString PlayerIP = TEXT("N/A")
					;
					PlayerList.Add( FRconPlayerData(Match->PlayersInMatchInstance[i].PlayerName, PlayerID, PlayerIP, Rank, Match->GameInstanceGUID, true) );
				}

			}
		}
	}
}

bool AUTLobbyGameState::SendSayToInstance(const FString& PlayerName, FString& Message)
{
	bool bSent = false;

	// Try and this user.	
	for (int32 MatchIndex = 0; MatchIndex < AvailableMatches.Num(); MatchIndex++)
	{
		AUTLobbyMatchInfo* Match = AvailableMatches[MatchIndex];
		if (Match && Match->InstanceBeacon)
		{
			for (int32 i = 0 ; i < Match->PlayersInMatchInstance.Num(); i++)
			{
				bool bIsUser = false;

				FString TestPlayerName = Match->PlayersInMatchInstance[i].PlayerName;
				FString TestUID = Match->PlayersInMatchInstance[i].PlayerID.ToString();

				if (Message.Left(TestPlayerName.Len()).Equals(TestPlayerName,ESearchCase::IgnoreCase))
				{
					Message = Message.Right(Message.Len() - TestPlayerName.Len()).Trim();
					bIsUser = true;
				}
				else if (Message.Left(TestPlayerName.Len()).Equals(TestUID,ESearchCase::IgnoreCase)) 
				{
					Message = Message.Right(Message.Len() - TestUID.Len()).Trim();
					bIsUser = true;
				}

				if (bIsUser)
				{
					bSent = true;
					Match->InstanceBeacon->Instance_ReceiveUserMessage(Match->PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%s says \"%s\""), *PlayerName, *Message));
					Message = FString::Printf(TEXT("to %s \"%s\""), *TestPlayerName, *Message);
					break;
				}
			}
		}
	}

	return bSent;
}

int32 AUTLobbyGameState::NumMatchesInProgress()
{
	int32 Count = 0;
	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		if (AvailableMatches[i] != nullptr)
		{
			if (AvailableMatches[i]->CurrentState == ELobbyMatchState::InProgress || AvailableMatches[i]->CurrentState == ELobbyMatchState::Launching)
			{
				Count++;
			}
		}
	}

	return Count;
}

void AUTLobbyGameState::AttemptDirectJoin(AUTLobbyPlayerState* PlayerState, const FString& SessionID, bool bSpectator)
{
	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		if (AvailableMatches[i]->UniqueMatchID.ToString() == SessionID)
		{
			JoinMatch(AvailableMatches[i], PlayerState,bSpectator);
			return;
		}
	}
}

void AUTLobbyGameState::MakeJsonReport(TSharedPtr<FJsonObject> JsonObject)
{
	Super::MakeJsonReport(JsonObject);

	TArray<TSharedPtr<FJsonValue>> AvailableMatchJson;
	for (int32 i=0; i < AvailableMatches.Num(); i++)
	{
		TSharedPtr<FJsonObject> MatchJson = MakeShareable(new FJsonObject);
		AvailableMatches[i]->MakeJsonReport(MatchJson);
		AvailableMatchJson.Add( MakeShareable( new FJsonValueObject( MatchJson )) );			
	}

	JsonObject->SetArrayField(TEXT("AvailableMatches"),  AvailableMatchJson);

	TArray<TSharedPtr<FJsonValue>> InstancesJson;
	for (int32 i=0; i < GameInstances.Num(); i++)
	{
		TSharedPtr<FJsonObject> IJson = MakeShareable(new FJsonObject);
		if (GameInstances[i].MatchInfo)
		{
			IJson->SetStringField(TEXT("InstanceID"), GameInstances[i].MatchInfo->GameInstanceGUID);
		}
		else
		{
			IJson->SetStringField(TEXT("InstanceID"), TEXT("<INVALID>"));
		}

		IJson->SetNumberField(TEXT("InstancePort"), GameInstances[i].InstancePort);
	}

	JsonObject->SetArrayField(TEXT("GameInstances"),  InstancesJson);
	JsonObject->SetNumberField(TEXT("ProcessesAwaitingReturnCodes"),  ProcessesToGetReturnCode.Num());


	TArray<TSharedPtr<FJsonValue>> RulesetJson;
	for (int32 i=0; i < AvailableGameRulesets.Num(); i++)
	{
		TSharedPtr<FJsonObject> RJson = MakeShareable(new FJsonObject);
		AvailableGameRulesets[i]->MakeJsonReport(RJson);
		RulesetJson.Add( MakeShareable( new FJsonValueObject( RJson )) );			
	}

	JsonObject->SetArrayField(TEXT("Rulesets"),  RulesetJson);
}

void AUTLobbyGameState::GetMatchBans(int32 GameInstanceId, TArray<FUniqueNetIdRepl> &BanList)
{
	for (int32 i=0; i < GameInstances.Num(); i++)
	{
		if (GameInstances[i].MatchInfo->GameInstanceID == GameInstanceID)
		{
			BanList = GameInstances[i].MatchInfo->BannedIDs;			
			return;
		}
	}
}

bool AUTLobbyGameState::IsClientFullyInformed()
{
	if ( GetNetMode() == NM_Client && 
			AllMapsOnServerCount > 0 && AllMapsOnServerCount == AllMapsOnServer.Num() && 
			AvailabelGameRulesetCount > 0 && AvailabelGameRulesetCount == AvailableGameRulesets.Num() )
	{
		// We need to check the actual data because it's possible that the array length get's replicated before the 
		// replicated infos. 
		bool bAllGood = true;
		for (int32 i=0; i < AllMapsOnServer.Num(); i++)
		{
			if (AllMapsOnServer[i] == nullptr)
			{
				bAllGood = false;
				break;
			}
		}

		for (int32 i=0; i < AvailableGameRulesets.Num(); i++)
		{
			if (AvailableGameRulesets[i] == nullptr)
			{
				bAllGood = false;
				break;
			}
		}

		return bAllGood;

	}
	return false;
}

void AUTLobbyGameState::RequestNewCustomMatch(AUTLobbyPlayerState* Creator, ECreateInstanceTypes::Type InstanceType, const FString& CustomName, const FString& GameMode, const FString& StartingMap, const FString& Description, const TArray<FString>& GameOptions,  int32 DesiredPlayerCount, bool _bTeamGame, bool bRankLocked, bool bSpectatable, bool _bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl)
{
	AUTReplicatedGameRuleset* CustomRuleset = AUTBaseGameMode::CreateCustomReplicateGameRuleset(GetWorld(), this, GameMode, StartingMap, Description, GameOptions, DesiredPlayerCount, bTeamGame);
	if (CustomRuleset != nullptr)
	{
		RequestNewMatch(Creator, InstanceType, CustomName, CustomRuleset, StartingMap, bRankLocked, bSpectatable, _bPrivateMatch, bBeginnerMatch, bUseBots, BotDifficulty, bRequireFilled, bHostControl);	
	}
}

void AUTLobbyGameState::RequestNewMatch(AUTLobbyPlayerState* Creator, ECreateInstanceTypes::Type InstanceType, const FString& CustomName, AUTReplicatedGameRuleset* Ruleset, const FString& StartingMap, bool bRankLocked, bool bSpectatable, bool _bPrivateMatch, bool bBeginnerMatch, bool bUseBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl)
{

	if (InstanceType == ECreateInstanceTypes::Lobby)
	{
		// Create a match and replicate all of the relevant information

		AUTLobbyMatchInfo* NewMatchInfo = GetWorld()->SpawnActor<AUTLobbyMatchInfo>();
		if (NewMatchInfo != nullptr)
		{	
			AvailableMatches.Add(NewMatchInfo);
			NewMatchInfo->InitializeMatch(Creator, CustomName, Ruleset, StartingMap, bRankLocked, bSpectatable, _bPrivateMatch, bBeginnerMatch);
			NewMatchInfo->LaunchMatch(bUseBots, BotDifficulty, bRequireFilled, bHostControl);
		}

	}
	
}

