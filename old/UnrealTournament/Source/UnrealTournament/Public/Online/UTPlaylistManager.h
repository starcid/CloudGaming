// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTATypes.h"
#include "UTPlaylistManager.generated.h"


USTRUCT()
struct UNREALTOURNAMENT_API FPlaylistItemStorage
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY()
	TArray<FPlaylistItem> NewItems;

	FPlaylistItemStorage()
	{
	}
};


UCLASS(config = Game, notplaceable)
class UNREALTOURNAMENT_API UUTPlaylistManager : public UObject
{
	GENERATED_BODY()

	TArray<FPlaylistItem> Playlist;

public:
	FUTGameRuleset* GetRuleset(int32 PlaylistId);

	/**
	 * Get the largest team count and team size for any zoneids on the specified playlist
	 *
	 * @param PlaylistId playlist to query
	 * @param MaxTeamCount largest number of possible teams in the game
	 * @param MaxTeamSize largest number of possible players on a team
	 * @param MaxPartySize maximum number of players that can join as a single party
	 */
	bool GetMaxTeamInfoForPlaylist(int32 PlaylistId, int32& MaxTeamCount, int32& MaxTeamSize, int32& MaxPartySize);

	bool GetURLForPlaylist(int32 PlaylistId, FString& URL);

	bool GetTeamEloRatingForPlaylist(int32 PlaylistId, FString& TeamEloRating);

	void UpdatePlaylistFromMCP(const FPlaylistItemStorage& MCPPlaylist);

	int32 GetNumPlaylists() { return Playlist.Num(); }

	bool GetPlaylistId(int32 PlaylistIndex, int32& PlaylistId);

	FName GetPlaylistSlateBadge(int32 PlaylistId);

	bool GetGameModeForPlaylist(int32 PlaylistId, FString& GameMode);

	int32 GetBotDifficulty(int32 PlaylistId);
	bool AreBotsAllowed(int32 PlaylistId);

	bool IsPlaylistRanked(int32 PlaylistId);
	bool ShouldPlaylistSkipElo(int32 PlaylistId);

	int32 HowManyRanked();
	int32 HowManyQuickPlay();

	bool IsValidPlaylist(int32 PlaylistId);

	void GetPlaylist(bool bRanked, TArray<FPlaylistItem*>& outList);

};