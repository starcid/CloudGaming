// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTTeamDMGameMode.h"

#include "UTDuelGame.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTDuelGame : public AUTTeamDMGameMode
{
	GENERATED_UCLASS_BODY()

	virtual bool CheckRelevance_Implementation(AActor* Other) override;
	virtual void InitGameState() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PlayEndOfMatchMessage() override;
	virtual void UpdateSkillRating() override;
	virtual FString GetRankedLeagueName() override;
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast) override;
	virtual bool ShouldBalanceTeams(bool bInitialTeam) const
	{
		// always, since 1v1 is the only option
		return true;
	}
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
	virtual void CheckBotCount() override;
	virtual void DefaultTimer() override;

	virtual void FindAndMarkHighScorer() override;

	// Creates the URL options for custom games
	virtual void CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps);

	virtual void BroadcastSpectatorPickup(AUTPlayerState* PS, FName StatsName, UClass* PickupClass);

	virtual uint8 GetNumMatchesFor(AUTPlayerState* PS, bool bRankedSession) const override;
	virtual int32 GetEloFor(AUTPlayerState* PS, bool bRankedSession) const override;
	virtual void SetEloFor(AUTPlayerState* PS, bool bRankedSession, int32 NewELoValue, bool bIncrementMatchCount) override;

#if !UE_SERVER
	virtual void CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers) override;
#endif

public:
	virtual void GetGameURLOptions(const TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, TArray<FString>& OptionsList, int32& DesiredPlayerCount) override;
};
