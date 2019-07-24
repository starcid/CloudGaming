// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/PlayerController.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyHUD.h"
#include "UTLocalPlayer.h"
#include "UTLobbyPlayerState.h"
#include "UTLobbyPC.h"
#include "UTLobbyMatchInfo.h"
#include "UTCharacterMovement.h"
#include "UTAnalytics.h"
#include "Online.h"
#include "UTOnlineGameSearchBase.h"
#include "OnlineSubsystemTypes.h"
#include "Dialogs/SUTDownloadAllDialog.h"
#include "UTGameViewportClient.h"

AUTLobbyPC::AUTLobbyPC(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RedirectCount = -1;
	bReceivedServerContentList = false;
}

/* Cache a copy of the PlayerState cast'd to AUTPlayerState for easy reference.  Do it both here and when the replicated copy of APlayerState arrives in OnRep_PlayerState */
void AUTLobbyPC::InitPlayerState()
{
	Super::InitPlayerState();
	UTLobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerState);
	UTLobbyPlayerState->ChatDestination = ChatDestinations::Global;
	UTPlayerState = UTLobbyPlayerState;
}

void AUTLobbyPC::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UTLobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerState);
	UTPlayerState = UTLobbyPlayerState;

	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		UTLobbyPlayerState->Server_ReadyToBeginDataPush();
		LP->UpdatePresence(TEXT("In Hub"), true, true, true, false);
#if !UE_SERVER
		if (LP && LP->ViewportClient)
		{
			UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(LP->ViewportClient);
			if (UTGameViewport && !UTGameViewport->KickReason.IsEmpty())
			{
				LP->ShowMessage(NSLOCTEXT("UTGameViewportClient","NetworkErrorDialogTitle","Network Error"), UTGameViewport->KickReason, UTDIALOG_BUTTON_OK);			
				UTGameViewport->KickReason = FText::GetEmpty();
			}
		}
#endif
	}
}

void AUTLobbyPC::PlayerTick( float DeltaTime )
{
	Super::PlayerTick(DeltaTime);

	if (!bInitialReplicationCompleted)
	{
		if (UTLobbyPlayerState && GetWorld()->GetGameState())
		{
			bInitialReplicationCompleted = true;
			UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
			if (LP)
			{
				LP->ShowMenu(TEXT(""));
			}
		}
	}
}

void  AUTLobbyPC::ServerRestartPlayer_Implementation()
{
}

bool AUTLobbyPC::CanRestartPlayer()
{
	return false;
}

void AUTLobbyPC::MatchChat(FString Message)
{
	Chat(ChatDestinations::Match, Message);
}

void AUTLobbyPC::GlobalChat(FString Message)
{
	Chat(ChatDestinations::Global, Message);
}

void AUTLobbyPC::Chat(FName Destination, FString Message)
{
	// Send the Chat to the server so it can be routed to the right place.  
	// TODO - Once we have MCP friends support, look at routing friends chat directly through the MCP

	Message = Message.Left(128);
	ServerChat(Destination, Message);
}

void AUTLobbyPC::ServerChat_Implementation(const FName Destination, const FString& Message)
{
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{

		// Look to see if this is a direct message..

		if (Message.Left(1) == TEXT("@"))
		{
			// Remove the @
			FString TrimmedMessage = Message.Right(Message.Len()-1);
			DirectSay(TrimmedMessage);
			return;
		}

		LobbyGameState->BroadcastChat(UTLobbyPlayerState, Destination, Message);
	}
}

bool AUTLobbyPC::ServerChat_Validate(const FName Destination, const FString& Message)
{
	return true;
}

void AUTLobbyPC::ReceivedPlayer()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		ServerSetAvatar(UTLocalPlayer->GetAvatar());
	}

	SendStatsIDToServer();

	if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone)
	{
		InitializeHeartbeatManager();
	}
}

void AUTLobbyPC::ServerDebugTest_Implementation(const FString& TestCommand)
{
}

void AUTLobbyPC::SetLobbyDebugLevel(int32 NewLevel)
{
	AUTLobbyHUD* H = Cast<AUTLobbyHUD>(MyHUD);
	if (H) H->LobbyDebugLevel = NewLevel;
}

void AUTLobbyPC::MatchChanged(AUTLobbyMatchInfo* CurrentMatch)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->UpdatePresence(CurrentMatch == NULL ? TEXT("In Hub") : TEXT("Setting up a Match"), true, true, true, false);
	}
}

void AUTLobbyPC::HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// If we are in a match and we get a network failure, leave that match.
	if (UTLobbyPlayerState)
	{
		UTLobbyPlayerState->ServerDestroyOrLeaveMatch();
	}
}

void AUTLobbyPC::Say(FString Message)
{
	if ( Message.Equals(TEXT("@match"),ESearchCase::IgnoreCase) )
	{
		if (UTLobbyPlayerState->CurrentMatch)
		{
			UE_LOG(UT,Log,TEXT("======================================="));
			UE_LOG(UT,Log,TEXT("Current Match [%s]"), *UTLobbyPlayerState->CurrentMatch->CurrentState.ToString());
			UE_LOG(UT,Log,TEXT("======================================="));
			if (UTLobbyPlayerState->CurrentMatch->CurrentRuleset.IsValid())
			{
				UE_LOG(UT,Log,TEXT("--- Ruleset [%s]"), *UTLobbyPlayerState->CurrentMatch->CurrentRuleset->Data.Title);
			}
			else
			{
				UE_LOG(UT,Log,TEXT("--- Ruleset is invalid"));
			}

			for (int32 i=0; i < UTLobbyPlayerState->CurrentMatch->Players.Num();i++)
			{
				if (UTLobbyPlayerState->CurrentMatch->Players[i].IsValid())
				{
					UE_LOG(UT,Log,TEXT("--- Player %i = %s"), i, *UTLobbyPlayerState->CurrentMatch->Players[i]->PlayerName);
				}
			}
		}
		return;
	}
	if (Message.Equals(TEXT("@maps"),ESearchCase::IgnoreCase) )
	{
		UE_LOG(UT,Log,TEXT("======================================="));
		UE_LOG(UT,Log,TEXT("Maps on Server"));
		UE_LOG(UT,Log,TEXT("======================================="));

		AUTLobbyGameState* GameState = GetWorld()->GetGameState<AUTLobbyGameState>();
		if (GameState)
		{
			for (int32 i = 0; i < GameState->AllMapsOnServer.Num(); i++)
			{
				FString Title = GameState->AllMapsOnServer[i]->Title;  if (Title.IsEmpty()) Title = TEXT("none");
				FString Package = GameState->AllMapsOnServer[i]->Redirect.PackageName; if (Package.IsEmpty()) Package = TEXT("none");
				FString Url = GameState->AllMapsOnServer[i]->Redirect.PackageURL; if (Url.IsEmpty()) Url = TEXT("none");
				FString MD5 = GameState->AllMapsOnServer[i]->Redirect.PackageChecksum; if (MD5.IsEmpty()) MD5 = TEXT("none");
				UE_LOG(UT,Log,TEXT("  Map Title: %s  Redirect: %s / %s / %s"), *Title, *Package, *Url,*MD5);
			}
		}
		return;
	}

	Super::Say(Message);
}

bool AUTLobbyPC::ServerRconKillMatch_Validate(AUTLobbyMatchInfo* MatchToKill) { return true; }
void AUTLobbyPC::ServerRconKillMatch_Implementation(AUTLobbyMatchInfo* MatchToKill)
{
	if (UTPlayerState == nullptr || !UTPlayerState->bIsRconAdmin)
	{
		ClientSay(UTPlayerState, TEXT("Rcon not authenticated"), ChatDestinations::System);
		return;
	}
	
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->AdminKillMatch(MatchToKill);
	}
}


#if !UE_SERVER
void AUTLobbyPC::RequestServerSendAllRedirects()
{
	// If we have already requested all of the content, then we can skip this
	if (!bReceivedServerContentList)
	{
		ServerSendRedirectCount();
	}
	else
	{
		ShowMessage(NSLOCTEXT("UTLocalPlayer", "HasAllContentTitle", "Acquire Content"), NSLOCTEXT("UTLobbyPC", "HasAllMessage", "You have already requested all of the content on this server."), UTDIALOG_BUTTON_OK, NULL);
	}
}
#endif

bool AUTLobbyPC::ServerSendRedirectCount_Validate() { return true; }
void AUTLobbyPC::ServerSendRedirectCount_Implementation()
{
	AUTLobbyGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	ClientReceiveRedirectCount(GameMode ? GameMode->RedirectReferences.Num() : 0);
}
bool AUTLobbyPC::ServerSendAllRedirects_Validate() { return true; }
void AUTLobbyPC::ServerSendAllRedirects_Implementation()
{
	AUTLobbyGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
	if (GameMode)
	{
		for (int32 i=0; i < GameMode->RedirectReferences.Num();i++)
		{
			ClientReceiveRedirect(GameMode->RedirectReferences[i]);
		}
	}
}

void AUTLobbyPC::ClientReceiveRedirectCount_Implementation(int32 NewCount)
{
	bReceivedServerContentList = true;
	if (NewCount != RedirectCount)
	{
		if (NewCount > 0)
		{
			ServerSendAllRedirects();	
		}

		RedirectCount = NewCount;
	}
}

void AUTLobbyPC::ResetAllRedirects()
{
	bReceivedServerContentList = false;
	RedirectCount = 0;
	AllRedirects.Empty();
}

void AUTLobbyPC::ClientReceiveRedirect_Implementation(const FPackageRedirectReference& Redirect)
{
	AllRedirects.Add(FPackageRedirectReference(Redirect));

	if (AllRedirects.Num() == RedirectCount)
	{
		UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
		if (UTLocalPlayer)
		{
			UTLocalPlayer->AcquireContent(AllRedirects);
		}
	}
}

bool AUTLobbyPC::ForwardDirectSay(AUTPlayerState* SenderPlayerState, FString& Message)
{
	// look to see if there is a remote player..

	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState && LobbyGameState->SendSayToInstance(PlayerState->PlayerName, Message))
	{
		return true;
	}

	return false;
}

void AUTLobbyPC::ServerReceiveStatsID_Implementation(const FString& NewStatsID)
{
	Super::ServerReceiveStatsID_Implementation(NewStatsID);

	if (FUTAnalytics::IsAvailable() && GetWorld() && (GetNetMode() == NM_DedicatedServer))
	{
		FUTAnalytics::FireEvent_UTHubPlayerJoinLobby(GetWorld()->GetAuthGameMode<AUTBaseGameMode>(), UTPlayerState);
	}
}