// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate/SlateGameResources.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "TAttributeProperty.h"
#include "UTLobbyMatchInfo.h"
#include "../Widgets/SUTComboButton.h"
#include "../Widgets/SUTPopOverAnchor.h"
#include "UTPlayerState.h"
#include "BlueprintContextLibrary.h"
#include "PartyContext.h"

#if !UE_SERVER

class FTrackedMatch : public TSharedFromThis<FTrackedMatch>
{
public:
	// The unqiue id of this match
	FGuid MatchId;

	// If the owner is connected to a hub, this will hold the a pointer to the MatchInfo for this match and will pull live data from it.
	TWeakObjectPtr<AUTLobbyMatchInfo> MatchInfo;

	// If the ownwer is in the server browser, then, the replicated MatchData will be used for display.
	TSharedPtr<FServerInstanceData> MatchData;

	// Holds the # of friends in this match.  NOTE: we track it here instead of the 
	// various replicated places because this information isn't available at creation.
	int32 NumFriends;

	// If true, this recortd will get cleaned up at the end of the tick cycle.
	bool bPendingKill;

	FTrackedMatch()
	{
		MatchInfo.Reset();
		MatchData.Reset();
		bPendingKill = true;
	};

	FTrackedMatch(const TWeakObjectPtr<AUTLobbyMatchInfo> inMatchInfo)
	{
		MatchId = inMatchInfo->UniqueMatchID;
		MatchInfo = inMatchInfo;

		MatchData.Reset();
		bPendingKill = false;
	};

	FTrackedMatch(const TSharedPtr<FServerInstanceData> inMatchData)
	{
		MatchId = inMatchData->InstanceId;
		MatchData = inMatchData;

		MatchInfo.Reset();
		bPendingKill = false;
	};


	static TSharedRef<FTrackedMatch> Make(const TWeakObjectPtr<AUTLobbyMatchInfo> inMatchInfo)
	{
		return MakeShareable( new FTrackedMatch( inMatchInfo ) );
	}

	static TSharedRef<FTrackedMatch> Make(const TSharedPtr<FServerInstanceData> inMatchData)
	{
		return MakeShareable( new FTrackedMatch( inMatchData ) );
	}

	int32 NumPlayers()
	{
		if (MatchInfo.IsValid()) 
		{
			return MatchInfo->NumPlayersInMatch();
		}
		else if (MatchData.IsValid())
		{
			return MatchData->NumPlayers();
		}

		return 0;
	}

	int32 NumSpectators()
	{
		if (MatchInfo.IsValid()) 
		{
			return MatchInfo->NumSpectatorsInMatch();
		}
		else if (MatchData.IsValid())
		{
			return MatchData->NumSpectators();
		}

		return 0;
	}

	FText GetFlags(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner)
	{
		bool bInvited = false;
		AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
		AUTLobbyPlayerState* OwnerPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);

		if (MatchInfo.IsValid())
		{
			if (OwnerPlayerState)
			{
				bInvited = MatchInfo->AllowedPlayerList.Find(OwnerPlayerState->UniqueId.ToString()) != INDEX_NONE;
			}
		}

		// TODO: Add icon references

		int32 Flags = MatchInfo.IsValid() ? MatchInfo->GetMatchFlags() : ( MatchData.IsValid() ? MatchData->Flags : 0);
		FString Final = TEXT("");

		AUTBaseGameMode* BaseGameMode = GetBaseGameMode();

		if ((Flags & MATCH_FLAG_InProgress) == MATCH_FLAG_InProgress)
		{
			FString StateString = TEXT("In Progress");
			if (MatchInfo.IsValid())
			{
				StateString = MatchInfo->MatchUpdate.MatchState != NAME_None ? MatchInfo->MatchUpdate.MatchState.ToString() : TEXT("Launching");
			}
			else if (MatchData.IsValid())
			{
				StateString = MatchData->MatchData.MatchState != NAME_None ? MatchData->MatchData.MatchState.ToString() : TEXT("Launching");
			}


			Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + StateString;
		}
		if ((Flags & MATCH_FLAG_Beginner) == MATCH_FLAG_Beginner)
		{
			Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + TEXT("Beginner");
		}

		if ((Flags & MATCH_FLAG_Private) == MATCH_FLAG_Private) 
		{
			Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + TEXT("Private");
		}

		if ((Flags & MATCH_FLAG_Ranked) == MATCH_FLAG_Ranked)
		{
			int32 MatchRankCheck = MatchInfo.IsValid() ? MatchInfo->RankCheck : (MatchData.IsValid() ? MatchData->RankCheck: DEFAULT_RANK_CHECK);
			int32 PlayerRankCheck = PlayerState->GetRankCheck(BaseGameMode);

			Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + TEXT("Rank Locked");
		}

		if ((Flags & MATCH_FLAG_NoJoinInProgress) == MATCH_FLAG_NoJoinInProgress) Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + TEXT("<img src=\"UT.Icon.Lock.Small\"/> No Join in Progress");
		if (bInvited) Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + TEXT("<UT.Font.NormalText.Tiny.Bold.Gold>!!Invited!!</>");

		if (NumFriends > 0)
		{
			FString Friends = FString::Printf(TEXT("<img src=\"UT.Icon.Friends.Small.Inline\"/> %i Friends Playing"), NumFriends);
			Final = Final + (Final.IsEmpty() ? TEXT("") : TEXT(" -- ")) + Friends;
		}
	
		return FText::FromString(Final);
	}


	FText GetCurrentPlayerCount()
	{
		return FText::AsNumber( NumPlayers() );
	}

	FText GetRuleTitle()
	{
		if ( MatchInfo.IsValid())
		{
			if (!MatchInfo->CustomGameName.IsEmpty())
			{
				return FText::FromString(MatchInfo->CustomGameName);
			}
			else if ( MatchInfo->CurrentRuleset.IsValid() )
			{
				return FText::FromString(MatchInfo->CurrentRuleset->Data.Title);
			}
			else if (MatchInfo->bDedicatedMatch)
			{
				return FText::FromString(MatchInfo->DedicatedServerName);
			}
		}
		else if (MatchData.IsValid())
		{
			if (!MatchData->CustomGameName.IsEmpty()) 
			{
				return FText::FromString(MatchData->CustomGameName);
			}
			else
			{
				FText::FromString(MatchData->RulesTitle);
			}
		}

		return FText::GetEmpty();
	}

	FText GetMap()
	{
		FString FinalMapName = TEXT("");
		FString FinalGameName = TEXT("");

		if ( MatchInfo.IsValid() )
		{
			if (MatchInfo->bDedicatedMatch)
			{
				FinalMapName = MatchInfo->InitialMap;
				FinalGameName = MatchInfo->DedicatedServerGameMode;
			}
			else
			{
				FinalMapName = MatchInfo->InitialMapInfo.IsValid() ? MatchInfo->InitialMapInfo->Title : MatchInfo->InitialMap;
				FinalGameName = MatchInfo->CurrentRuleset->Data.Title;
			}
		}
		else
		{
			FinalMapName = MatchData->MapName;
			FinalGameName = MatchData->RulesTitle;
		}

		return FText::Format(NSLOCTEXT("SUTMatchPanel","GetMapFormat","{0} on {1}"), FText::FromString(FinalGameName), FText::FromString(FinalMapName));
	}

	FText GetMaxPlayers()
	{
		int32 MP = MatchData.IsValid() ? MatchData->MaxPlayers : 0;
		if ( MatchInfo.IsValid() )
		{
			MP = (MatchInfo->CurrentRuleset.IsValid()) ? MatchInfo->CurrentRuleset->Data.MaxPlayers : MatchInfo->DedicatedServerMaxPlayers;
		}

		return FText::Format(NSLOCTEXT("SUTMatchPanel","MaxPlayerFormat","Out of {0}"), FText::AsNumber(MP));
	}

	EVisibility GetLockVis(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner) 
	{
		bool bLocked = false;
		
		int32 Flags = MatchInfo.IsValid() ? MatchInfo->GetMatchFlags() : ( MatchData.IsValid() ? MatchData->Flags : 0);

		AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
		AUTLobbyPlayerState* OwnerPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		AUTBaseGameMode* BaseGameMode = GetBaseGameMode();

		if ( (Flags & MATCH_FLAG_Private) == MATCH_FLAG_Private) 
		{
			if (!MatchInfo.IsValid() || OwnerPlayerState == nullptr || MatchInfo->AllowedPlayerList.Find(OwnerPlayerState->UniqueId.ToString()) == INDEX_NONE)
			{
				bLocked = true;
			}
		}
	
		if ( (Flags & MATCH_FLAG_Ranked) == MATCH_FLAG_Ranked) 
		{
			int32 MatchRankCheck = MatchInfo.IsValid() ? MatchInfo->RankCheck : (MatchData.IsValid() ? MatchData->RankCheck: DEFAULT_RANK_CHECK);
			int32 PlayerRankCheck = PlayerState->GetRankCheck(BaseGameMode);

			if ( !AUTPlayerState::CheckRank(PlayerRankCheck, MatchRankCheck) )
			{
				bLocked = true;
			}
		}

		return bLocked ? EVisibility::Visible : EVisibility::Hidden;
	}


	const FSlateBrush* GetBadge() const
	{
		// Grab the current game 

		int32 Badge =  0;
		int32 Level = 0;

		if (MatchInfo.IsValid() && MatchInfo->CurrentRuleset.IsValid())
		{
			AUTPlayerState::SplitRankCheck(MatchInfo->RankCheck, Badge, Level);
		}
		else if ( MatchData.IsValid() )
		{
			AUTPlayerState::SplitRankCheck(MatchData->RankCheck, Badge, Level);
		}

		FString BadgeStr = FString::Printf(TEXT("UT.RankBadge.%i"), Badge);
		return SUTStyle::Get().GetBrush(*BadgeStr);
	}

	FText GetRank()
	{
		// Grab the current game 

		int32 Badge =  0;
		int32 Level = 0;

		AUTBaseGameMode* BaseGameMode = nullptr;
		if (MatchInfo.IsValid() && MatchInfo->CurrentRuleset.IsValid())
		{
			AUTPlayerState::SplitRankCheck(MatchInfo->RankCheck, Badge, Level);
		}
		else if ( MatchData.IsValid() )
		{
			AUTPlayerState::SplitRankCheck(MatchData->RankCheck, Badge, Level);
		}

		return FText::AsNumber(Level + 1);
	}

	bool CanJoin(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner)
	{

		if (PlayerOwner.IsValid() && PlayerOwner->IsInAnActiveParty() && !PlayerOwner->IsPartyLeader())
		{
			return false;
		}

		//int32 Flags = MatchInfo.IsValid() ? MatchInfo->GetMatchFlags() : ( MatchData.IsValid() ? MatchData->Flags : 0);
		//return ((Flags & MATCH_FLAG_InProgress) != MATCH_FLAG_InProgress) || ((Flags & MATCH_FLAG_NoJoinInProgress) != MATCH_FLAG_NoJoinInProgress);

		int32 Flags = MatchInfo.IsValid() ? MatchInfo->GetMatchFlags() : ( MatchData.IsValid() ? MatchData->Flags : 0);
		
		// If this match is in progress and we do not support join in progress...
		if ( ((Flags & MATCH_FLAG_InProgress) == MATCH_FLAG_InProgress) && ((Flags &MATCH_FLAG_NoJoinInProgress) == MATCH_FLAG_NoJoinInProgress) )
		{
			return false;
		}

		if ( (Flags & MATCH_FLAG_Private) == MATCH_FLAG_Private)
		{
			// Look to see if this player has an invite in to this match
			AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
			if (LobbyPlayerState && LobbyPlayerState->LastInvitedMatch != MatchInfo)
			{
				return false;
			}
		}

		// If this is a ranked match, check the rank
		if ((Flags & MATCH_FLAG_Ranked) == MATCH_FLAG_Ranked)
		{

			AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
			int32 MatchRankCheck = MatchInfo.IsValid() ? MatchInfo->RankCheck : (MatchData.IsValid() ? MatchData->RankCheck: DEFAULT_RANK_CHECK);
			int32 PlayerRankCheck = PlayerState->GetRankCheck(GetBaseGameMode());

			if ( !AUTPlayerState::CheckRank(PlayerRankCheck, MatchRankCheck) )
			{
				return false;
			}
		}

		if (MatchInfo.IsValid())
		{
			return MatchInfo->NumPlayersInMatch() < (MatchInfo->CurrentRuleset.IsValid() ? MatchInfo->CurrentRuleset->Data.MaxPlayers : 16);
		}
		else if (MatchData.IsValid())
		{
			return (MatchData->NumPlayers() < MatchData->MaxPlayers);
		}

		return true;
	}

	bool CanSpectate(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner)
	{
		if (PlayerOwner.IsValid())
		{
			UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
			if (PartyContext)
			{
				const int32 PartySize = PartyContext->GetPartySize();
				if (PartySize > 1)
				{
					return false;
				}
			}
		}

		int32 Flags = MatchInfo.IsValid() ? MatchInfo->GetMatchFlags() : ( MatchData.IsValid() ? MatchData->Flags : 0);
		return (Flags & MATCH_FLAG_NoSpectators) != MATCH_FLAG_NoSpectators;
	}

	AUTBaseGameMode* GetBaseGameMode() const
	{
		AUTBaseGameMode* BaseGameMode = nullptr;
		if (MatchInfo.IsValid() && MatchInfo->CurrentRuleset.IsValid())
		{
			BaseGameMode = MatchInfo->CurrentRuleset->GetDefaultGameModeObject();		
		}
		else if ( MatchData.IsValid() && !MatchData->GameModeClass.IsEmpty() )
		{
			UClass* GameModeClass = LoadClass<AUTGameMode>(NULL, *MatchData->GameModeClass, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
			if (GameModeClass)
			{
				BaseGameMode = GameModeClass->GetDefaultObject<AUTBaseGameMode>();
			}
		}

		if (BaseGameMode == nullptr) BaseGameMode = AUTBaseGameMode::StaticClass()->GetDefaultObject<AUTBaseGameMode>();
		return BaseGameMode;
	}

};

class FServerData;

DECLARE_DELEGATE_TwoParams(FMatchPanelJoinMatchDelegate, const FString& , bool );
DECLARE_DELEGATE(FMatchPanelStartMatchDelegate);

class UNREALTOURNAMENT_API SUTMatchPanel : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUTMatchPanel)
	: _bExpectLiveData(true)
	{}
		SLATE_ARGUMENT( TWeakObjectPtr<UUTLocalPlayer>, PlayerOwner )
		SLATE_EVENT(FMatchPanelJoinMatchDelegate, OnJoinMatchDelegate )
		SLATE_EVENT(FMatchPanelStartMatchDelegate, OnStartMatchDelegate )
		SLATE_ARGUMENT( bool, bExpectLiveData)

	SLATE_END_ARGS()


public:	
	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	void SetServerData(TSharedPtr<FServerData> inServerData);

protected:

	TSharedPtr<FServerData> ServerData;

	bool bExpectLiveData;

	// Will return true if the player owner is connected to a hub.  It will tell the panel
	// to poll the live data during the tick instead of waiting on updates.

	bool ShouldUseLiveData();

	/**
	 *	Returns INDEX_NONDE if not.
	 **/
	int32 IsTrackingMatch(AUTLobbyMatchInfo* Match);
	// Holds a list of matches that need to be displayed.

	TArray<TSharedPtr<FTrackedMatch> > TrackedMatches;
	TSharedRef<ITableRow> OnGenerateWidgetForMatchList( TSharedPtr<FTrackedMatch> InItem, const TSharedRef<STableViewBase>& OwnerTable );
	TSharedPtr<SListView<TSharedPtr<FTrackedMatch>>> MatchList;
	TSharedPtr<SVerticalBox> NoMatchesBox;
	bool bShowingNoMatches;

	// The Player Owner that owns this panel
	TWeakObjectPtr<UUTLocalPlayer> PlayerOwner;

	// Every frame check the status of the match and update.
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	FText GetServerNameText() const;
	FText GetMatchButtonText() const;
	
	FReply StartNewMatch();

	virtual void OnListMouseButtonDoubleClick(TSharedPtr<FTrackedMatch> SelectedMatch);

	virtual TSharedRef<SWidget> OnGetPopup(TSharedPtr<SUTPopOverAnchor> Anchor, TSharedPtr<FTrackedMatch> TrackedMatch);
	virtual TSharedRef<SWidget> OnGetPopupContent(TSharedPtr<SUTPopOverAnchor> Anchor, TSharedPtr<FTrackedMatch> TrackedMatch);

	FReply JoinMatchButtonClicked(TSharedPtr<FTrackedMatch> InItem);
	FReply SpectateMatchButtonClicked(TSharedPtr<FTrackedMatch> InItem);

	FReply DownloadAllButtonClicked();

	FMatchPanelJoinMatchDelegate OnJoinMatchDelegate;
	FMatchPanelStartMatchDelegate OnStartMatchDelegate;

	TSharedPtr<SUTButton> DownloadContentButton;
	TSharedPtr<SUTPopOverAnchor> CurrentAnchor;
	bool bSuspendPopups;

	EVisibility GetMatchButtonVis() const;
	TSharedPtr<SButton> StartMatchButton;

	bool bShowPrivateHub;
	EVisibility GetPrivateHubVis() const;
	EVisibility GetHbInstanceListHubVis() const;

};

#endif
