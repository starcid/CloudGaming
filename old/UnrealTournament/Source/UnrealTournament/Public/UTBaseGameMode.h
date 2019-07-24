// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "UTPlayerState.h"
#if WITH_PROFILE
#include "UtMcpProfileManager.h"
#endif
#include "UTBaseGameMode.generated.h"

#if !UE_SERVER
	class SUTMenuBase;
#endif

class UUTLocalPlayer;
class AUTReplicatedGameRuleset;
	
USTRUCT()
struct FEpicMapData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString MapPackageName;

	UPROPERTY()
	bool bIsEpicMap;

	UPROPERTY()
	bool bIsMeshedMap;

	FEpicMapData()
	{
		MapPackageName = TEXT("");
		bIsEpicMap = false;
		bIsMeshedMap = false;
	}
};

UCLASS()
class UNREALTOURNAMENT_API AUTBaseGameMode : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PreInitializeComponents() override;
	virtual void DefaultTimer() { };

	virtual void OnLoadingMovieBegin() { };
	virtual void OnLoadingMovieEnd() { };

	//Password required to join as a player
	UPROPERTY(GlobalConfig)
	FString ServerPassword;

	//Password required to join as a spectator
	UPROPERTY(GlobalConfig)
	FString SpectatePassword;

	uint32 bRequirePassword:1;

	// note that this is not visible because in the editor we will still have the user set DefaultPawnClass, see PostEditChangeProperty()
	// in C++ constructors, you should set this as it takes priority in InitGame()
	UPROPERTY()
	TAssetSubclassOf<APawn> PlayerPawnObject;

#if !UE_SERVER

	/**
	 *	Returns the Menu to popup when the user requests a menu
	 **/
	virtual TSharedRef<SUTMenuBase> GetGameMenu(UUTLocalPlayer* PlayerOwner) const;

#endif

protected:

	/** Handle for efficient management of DefaultTimer timer */
	FTimerHandle TimerHandle_DefaultTimer;

	// Will be > 0 if this is an instance created by lobby
	uint32 LobbyInstanceID;

	// Creates and stores a new server ID
	void CreateServerID();

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		if (DefaultPawnClass != NULL && PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("DefaultPawnClass")))
		{
			PlayerPawnObject = DefaultPawnClass;
		}
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
#endif

	/** human readable localized name for the game mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable, Category = Game)
	FText DisplayName;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual APlayerController* Login(class UPlayer* NewPlayer, ENetRole RemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void GenericPlayerInitialization(AController* C);
	virtual void ChangeName(AController* Other, const FString& S, bool bNameChange) override;

	virtual void PostInitProperties();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState();
	// Holds the server instance guid.  This is created when 
	UPROPERTY(GlobalConfig)
	FString ServerInstanceID;

	// The Unique ID for this game instance.
	FGuid ServerInstanceGUID;

	// The Unique ID for this 
	FGuid ContextGUID;

	virtual bool IsGameInstanceServer() { return LobbyInstanceID > 0; }
	virtual bool IsLobbyServer() { return false; }

	virtual FName GetNextChatDestination(AUTPlayerState* PlayerState, FName CurrentChatDestination);

	// Returns the # of instances controlled by this game mode and fills out the HostNames and Descriptions arrays.  
	virtual void GetInstanceData(TArray<TSharedPtr<FServerInstanceData>>& InstanceData);

	// Returns the # of players in this game.  By Default returns NumPlayers but can be overrride in children (like the HUBs)
	virtual int32 GetNumPlayers();

	// Returns the # of matches assoicated with this game type.  Typical returns 1 (this match) but HUBs will return all of their active matches
	virtual int32 GetNumMatches();

	// The Minimum ELO rank allowed on this server.
	UPROPERTY(Config)
	int32 MinAllowedRank;

	UPROPERTY(Config)
	int32 MaxAllowedRank;

	UPROPERTY(Config)
	bool bTrainingGround;

	UPROPERTY(GlobalConfig)
	FString TestString;

	UPROPERTY(GlobalConfig)
		FString PawnClassOverride;

	/**
	 * Converts a string to a bool.  If the string is empty, it will return the default.
	 **/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Options")
	static bool EvalBoolOptions(const FString& InOpt, bool Default)
	{
		if (!InOpt.IsEmpty())
		{
			if (FCString::Stricmp(*InOpt, TEXT("True")) == 0
				|| FCString::Stricmp(*InOpt, *GTrue.ToString()) == 0
				|| FCString::Stricmp(*InOpt, *GYes.ToString()) == 0)
			{
				return true;
			}
			else if (FCString::Stricmp(*InOpt, TEXT("False")) == 0
				|| FCString::Stricmp(*InOpt, *GFalse.ToString()) == 0
				|| FCString::Stricmp(*InOpt, TEXT("No")) == 0
				|| FCString::Stricmp(*InOpt, *GNo.ToString()) == 0)
			{
				return false;
			}
			else
			{
				return FCString::Atoi(*InOpt) != 0;
			}
		}
		else
		{
			return Default;
		}
	}

	UPROPERTY(Config)
	TArray<FPackageRedirectReference> RedirectReferences;

	virtual void GatherRequiredRedirects(TArray<FPackageRedirectReference>& Redirects);
	virtual bool FindRedirect(const FString& PackageName, FPackageRedirectReference& Redirect);
	virtual void GameWelcomePlayer(UNetConnection* Connection, FString& RedirectURL) override;

	UPROPERTY()
	int32 CurrentPlaylistId;

private:
	FString GetCloudID() const;

	// Never allow the engine to start replays, we'll do it manually
	bool IsHandlingReplays() override { return false; }

public:
	virtual void BuildPlayerInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList){};
	virtual void BuildScoreInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList){};
	virtual void BuildRewardInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList){};
	virtual void BuildWeaponInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList){};
	virtual void BuildMovementInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList){};

	virtual void SendRconMessage(const FString& DestinationId, const FString &Message);

	// Kicks a player
	virtual void RconKick(const FString& NameOrUIDStr, bool bBan, const FString& Reason);
	virtual void RconUnban(const FString& UIDStr);
	virtual void RconAuth(AUTBasePlayerController* Admin, const FString& Password);
	virtual void RconNormal(AUTBasePlayerController* Admin);

private:
	UPROPERTY()
	UObject* McpProfileManager;
public:
#if WITH_PROFILE
	inline UUtMcpProfileManager* GetMcpProfileManager() const
	{
		checkSlow(Cast<UUtMcpProfileManager>(McpProfileManager) != NULL);
		return Cast<UUtMcpProfileManager>(McpProfileManager);
	}
#endif

	// This will be set to true if this is a private match.  
	UPROPERTY()
	bool bPrivateMatch;

	/** How many matches (up to 255) have been played in this game mode. */
	virtual uint8 GetNumMatchesFor(AUTPlayerState* PS, bool bRankedSession) const;

	/** Returns whether enough matches have been played for this gamemode's Elo to be valid. */
	virtual bool IsValidElo(AUTPlayerState* PS, bool bRankedSession) const;

	/** Get the Elo rating for PS for this game mode. */
	virtual int32 GetEloFor(AUTPlayerState* PS, bool bRankedSession) const;

	/** Locally set Elo rating for this game mode (updated from server). */
	virtual void SetEloFor(AUTPlayerState* PS, bool bRankedSession, int32 NewELoValue, bool bIncrementMatchCount);

	/** Event that is called on servers when the initial Client->Server replication of the ELO/Rank/Progression occurs. **/
	virtual void ReceivedRankForPlayer(AUTPlayerState* UTPlayerState);

	/** Handle console exec commands */
	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor);

	/** Build a JSON object that contains information about this game mode. */
	virtual void MakeJsonReport(TSharedPtr<FJsonObject> JsonObject);

	virtual void CheckMapStatus(FString MapPackageName, bool& bIsEpicMap, bool& bIsMeshedMap, bool& bHasRights);

	virtual FString GetRankedLeagueName() { return TEXT(""); }

	virtual bool SupportsInstantReplay() const;

private:
	/** Holds a list of all Epic maps and their meshed stats */
	UPROPERTY(GlobalConfig)
	TArray<FEpicMapData> EpicMapList;

protected:
	/** map prefix for valid maps (not including the dash); you can create more broad handling by overriding SupportsMap() */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Game)
	FString MapPrefix;

	FString ServerNameOverride;

	UPROPERTY()
	bool bIgnoreIdlePlayers;

	FString HostIdString;

public:
	
	/**
	 *	Creates a Replicated ruleset for a custom game based on params passed in
	 **/
	static AUTReplicatedGameRuleset* CreateCustomReplicateGameRuleset(UWorld* World, AActor* Owner, const FString& GameMode, const FString& StartingMap, const FString& Description, const TArray<FString>& GameOptions, int32 DesiredPlayerCount, bool bTeamGame);

	UPROPERTY(BlueprintReadonly, Category=Game)
	bool bIsLANGame;

	void ForceClearUnpauseDelegates(AActor* PauseActor);

public:
	UFUNCTION(BlueprintNativeEvent, Category = Chat)
	bool AllowTextMessage(FString& Msg, bool bIsTeamMessage, AUTBasePlayerController* Sender);

	UPROPERTY(BlueprintReadOnly, Category=Game)
	FString HubGUIDString;

	UFUNCTION(BlueprintCallable, Category = Game)
	FString GetHostId()
	{
		return HostIdString;
	}

};