// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "TAttributeProperty.h"
#include "UTServerBeaconLobbyClient.h"
#include "UTAntiCheatModularFeature.h"
#include "UTBotPlayer.h"
#include "UTPlayerStart.h"

#include "UTGameMode.generated.h"

UNREALTOURNAMENT_API DECLARE_LOG_CATEGORY_EXTERN(LogUTGame, Log, All);

/** Defines the current state of the game. */

namespace MatchState
{
	extern UNREALTOURNAMENT_API const FName PlayerIntro;					// Playing the player intro in the match summary window
	extern UNREALTOURNAMENT_API const FName CountdownToBegin;				// We are entering this map, actors are not yet ticking
	extern UNREALTOURNAMENT_API const FName MatchEnteringOvertime;			// The game is entering overtime
	extern UNREALTOURNAMENT_API const FName MatchIsInOvertime;				// The game is in overtime
	extern UNREALTOURNAMENT_API const FName MapVoteHappening;				// The game is in mapvote stage
	extern UNREALTOURNAMENT_API const FName MatchIntermission;				// The game is in a round intermission
	extern UNREALTOURNAMENT_API const FName MatchExitingIntermission;		// Exiting Halftime
	extern UNREALTOURNAMENT_API const FName MatchRankedAbandon;				// Exiting Halftime
	extern UNREALTOURNAMENT_API const FName WaitingTravel;					// The client is awaying travel to the next map

}

class AUTPlayerController;

/** list of bots user asked to put into the game */
USTRUCT()
struct FSelectedBot
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FStringAssetReference BotAsset;
	/** team to place them on - note that 255 is valid even for team games (placed on any team) */
	UPROPERTY()
	uint8 Team;

	FSelectedBot()
		: Team(255)
	{}
	FSelectedBot(const FStringAssetReference& InAssetPath, uint8 InTeam)
		: BotAsset(InAssetPath), Team(InTeam)
	{}
};

#if !UE_SERVER
class SUTHUDWindow;
#endif

UCLASS(Config = Game, Abstract)
class UNREALTOURNAMENT_API AUTGameMode : public AUTBaseGameMode
{
	GENERATED_UCLASS_BODY()

public:
	/** Cached reference to our game state for quick access. */
	UPROPERTY()
	AUTGameState* UTGameState;		

	/** base difficulty of bots */
	UPROPERTY()
	float GameDifficulty;		

	/* How long to display the scoring summary */
	UPROPERTY(EditDefaultsOnly, Category = PostMatchTime)
		float MatchSummaryDelay;

	/* How long to display winning team match summary */
	UPROPERTY(EditDefaultsOnly, Category = PostMatchTime)
		float MatchSummaryTime;

	/** Return how long to wait after end of match before travelling to next map. */
	virtual float GetTravelDelay();

	UPROPERTY(EditDefaultsOnly, Category = Game)
	uint32 bAllowOvertime:1;

	/**If enabled, the server grants special control for casters*/
	UPROPERTY()
	uint32 bCasterControl:1;

	/**True when caster is ready to start match*/
	UPROPERTY()
		uint32 bCasterReady : 1;

	/** True if this match was started as a quickmatch. */
	UPROPERTY()
		uint32 bIsQuickMatch : 1;

	/** If true, all players are on blue team in team games. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TeamGame)
		uint32 bIsVSAI : 1; 

	/** If TRUE, force dead players to respawn immediately. Can be overridden with ForceRespawn=x on the url */
	UPROPERTY(EditDefaultsOnly, Category = Game)
	bool bForceRespawn;

	UPROPERTY(EditDefaultsOnly)
	bool bHasRespawnChoices;

	UPROPERTY()
		TArray<APlayerStart*> PlayerStarts;

	/** If true, when rating player starts also rate against potential starts (if bHasRespawnChoices is true).  Used before match to keep player start choices apart. */
	UPROPERTY(BlueprintReadWrite, Category = Game)
		bool bCheckAgainstPotentialStarts;

	/** If true, allow announcements for pickup spawn. */
	UPROPERTY(EditDefaultsOnly, Category=Game)
		bool bAllowPickupAnnouncements;

	/** If true, allow armor pickup even if player can't use it. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		bool bAllowAllArmorPickups;

	UPROPERTY(AssetRegistrySearchable, EditDefaultsOnly)
	bool bHideInUI;

	/** If true, players joining past this time will not count for Elo. */
	UPROPERTY(BlueprintReadOnly)
		bool bPastELOLimit;

	/** If true, require full set of players to be ready to start  (ready implied by warming up). */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		bool bRequireReady;

	/** If true, require full set of players to start. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		bool bRequireFull;

	/** If true, bots will not fill undermanned match. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		bool bForceNoBots;

	UPROPERTY(BlueprintReadWrite)
		bool bAutoAdjustBotSkill;

	UPROPERTY(BlueprintReadWrite)
		float RedTeamSkill;

	UPROPERTY(BlueprintReadWrite)
		float BlueTeamSkill;

	UPROPERTY(BlueprintReadWrite)
		int32 BlueTeamKills;

	UPROPERTY(BlueprintReadWrite)
		int32 RedTeamKills;

	UPROPERTY(BlueprintReadWrite)
		int32 BlueTeamDeaths;

	UPROPERTY(BlueprintReadWrite)
		int32 RedTeamDeaths;

	virtual void UpdateSkillAdjust(const AUTPlayerState* KillerPlayerState, const AUTPlayerState* KilledPlayerState);

	virtual void NotifyFirstPickup(class AUTCarriedObject* Flag) {};
	virtual void AddDeniedEventToReplay(APlayerState* KillerPlayerState, AUTPlayerState* Holder, AUTTeamInfo* Team) {};

	/** Delay to start match after start conditions are met. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		float StartDelay;

	/** Last time match was not ready to start. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		float LastMatchNotReady;

	/** Score needed to win the match.  Can be overridden with GOALSCORE=x on the url */
	UPROPERTY(EditDefaultsOnly, Category = Game)
	int32 GoalScore;    

	/** How long should the match be.  Can be overridden with TIMELIMIT=x on the url */
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	int32 TimeLimit;    

	/** multiplier to all XP awarded */
	UPROPERTY()
	float XPMultiplier;

	/** XP cap/minute */
	UPROPERTY()
	int32 XPCapPerMin;

	/** Will be TRUE if the game has ended */
	UPROPERTY()
	uint32 bGameEnded:1;    

	/** Will be TRUE if this is a team game */
	UPROPERTY()
	uint32 bTeamGame:1;

	/** Force warmup for players who enter before match has started. */
	UPROPERTY()
		uint32 bForceWarmup : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bFirstBloodOccurred;

	/** After this wait, add bots to min players level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MatchStart")
		int32 MaxWaitForPlayers;

	/** MaxWaitForPlayers for QuickMatch */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MatchStart")
		int32 QuickWaitForPlayers;

	/** ShortWaitForPlayers for LAN servers or servers that have already been through a map vote */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MatchStart")
		int32 ShortWaitForPlayers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MatchStart")
		int32 DefaultMaxPlayers;

	/** World time when match was first ready to start. */
	UPROPERTY()
	float StartPlayTime;

	UPROPERTY()
		float MatchIntroTime;

	/** add bots until NumPlayers + NumBots is this number */
	UPROPERTY(BlueprintReadWrite, Category = "MatchStart")
	int32 BotFillCount;

	/** for warm up, add bots until NumPlayers + NumBots is this number */
	UPROPERTY(BlueprintReadWrite, Category = "MatchStart")
		int32 WarmupFillCount;

	// How long a player must wait before respawning.  Set to 0 for no delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Rules)
	float RespawnWaitTime;

	// How long a player can wait before being forced respawned (added to RespawnWaitTime).  Set to 0 for no delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Rules)
	float ForceRespawnTime;

	/** # of seconds before the match begins */
	UPROPERTY()
	int32 CountDown;

	/** Holds the last place any player started from */
	UPROPERTY()
	class AActor* LastStartSpot;    // last place any player started from

	/** Timestamp of when this game ended */
	UPROPERTY()
	float EndTime;

	/** whether weapon stay is active */
	UPROPERTY(EditDefaultsOnly, Category = "Game")
	bool bWeaponStayActive;

	/** Which actor in the game should all other actors focus on after the game is over */
	UPROPERTY()
	class AActor* EndGameFocus;

	UPROPERTY(EditDefaultsOnly, Category = Game)
	TSubclassOf<class UUTLocalMessage> DeathMessageClass;

	UPROPERTY(EditDefaultsOnly, Category = Game)
	TSubclassOf<class UUTLocalMessage> GameMessageClass;

	UPROPERTY(EditDefaultsOnly, Category = Game)
	TSubclassOf<class UUTLocalMessage> VictoryMessageClass;

	/** Remove all items from character inventory list, before giving him game mode's default inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bClearPlayerInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<TSubclassOf<AUTInventory> > DefaultInventory;

	/** If true, characters taking damage lose health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bDamageHurtsHealth;

	/** If true, firing weapons costs ammo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bAmmoIsLimited;

	/** Offline challenge mode. */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bOfflineChallenge;

	/** If true, is a training mode with no cheats allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bBasicTrainingGame;


	/** If true, play inventory tutorial announcements on pickup/equip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
		bool bPlayInventoryTutorialAnnouncements;

	/** If true, don't go back to training menu at end of game. */
	UPROPERTY(BlueprintReadWrite, Category = "Game")
		bool bNoTrainingMenu;

	/** To make it easier to customize for instagib.  FIXMESTEVE should eventually all be mutator driven so other mutators can take advantage too. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
		bool bIsInstagib;

	/** Tag of the current challenge */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	FName ChallengeTag;

	/** How difficult is this challenge (0-2) */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 ChallengeDifficulty;

	/** Last time asnyone sent a taunt voice message. */
	UPROPERTY()
	float LastGlobalTauntTime;

	/** Toggle invulnerability */
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual void Demigod();

	/** mutators required for the game, added at startup just before command line mutators */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Game)
	TArray< TSubclassOf<class AUTMutator> > BuiltInMutators;
	
	/** mutators that will load from the config*/
	UPROPERTY(Config)
	TArray<FString> ConfigMutators;

	/** last starting map selected in the UI */
	UPROPERTY(Config)
	FString UILastStartingMap;

	UPROPERTY()
	FString RconNextMapName;

	/** How long should the server wait when there is no one on it before auto-restarting */
	UPROPERTY(Config)
	int32 AutoRestartTime;

	UPROPERTY(GlobalConfig)
		TArray<FString> ProtoRed;

	UPROPERTY(GlobalConfig)
		TArray<FString> ProtoBlue;

	UPROPERTY()
		bool bUseProtoTeams;

	UPROPERTY()
		int32 RedProtoIndex;

	UPROPERTY()
		int32 BlueProtoIndex;

	/** How long has the server been empty */
	int32 EmptyServerTime;

	/** How many bots to fill in if game needs more players. */
	virtual int32 AdjustedBotFillCount();

	/** Whether player is allowed to suicide. */
	virtual bool AllowSuicideBy(AUTPlayerController* PC);

	/** HUD class used for the caster's multiview */
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadWrite, Category = Classes)
	TSubclassOf<class AHUD> CastingGuideHUDClass;

	virtual void SwitchToCastingGuide(AUTPlayerController* NewCaster);

	/** first mutator; mutators are a linked list */
	UPROPERTY(BlueprintReadOnly, Category = Mutator)
	class AUTMutator* BaseMutator;

	virtual void PostInitProperties()
	{
		Super::PostInitProperties();
		if (DisplayName.IsEmpty() || (GetClass() != AUTGameMode::StaticClass() && DisplayName.EqualTo(GetClass()->GetSuperClass()->GetDefaultObject<AUTGameMode>()->DisplayName)))
		{
			DisplayName = FText::FromName(GetClass()->GetFName());
		}
	}

	/** class used for AI bots */
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadWrite, Category = Classes)
	TSubclassOf<class AUTBotPlayer> BotClass;

	/** Sorted array of remaining eligible bot characters to select from. */
	UPROPERTY()
	TArray<UUTBotCharacter*> EligibleBots;

	/** type of SquadAI that contains game specific AI logic for this gametype */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	TSubclassOf<class AUTSquadAI> SquadType;
	/** maximum number of players per squad (except human-led squad if human forces bots to follow) */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	int32 MaxSquadSize;


	// returns true if the player is idle
	UFUNCTION()
		bool IsPlayerIdle(AUTPlayerState* PS);

	/** cached list of mutator assets from the asset registry and native classes, used to allow shorthand names for mutators instead of full paths all the time */
	TArray<FAssetData> MutatorAssets;

	/** whether to record a demo (starts when the countdown starts) */
	UPROPERTY(GlobalConfig)
	bool bRecordDemo;

	/** filename for demos... should use one of the replacement strings or it'll overwrite every game */
	UPROPERTY(GlobalConfig)
	FString DemoFilename;

	UPROPERTY()
		bool bDevServer;

	UPROPERTY()
		float EndOfMatchMessageDelay;


	TAssetSubclassOf<AUTWeapon> ImpactHammerObject;

	UPROPERTY()
		TSubclassOf<AUTWeapon> ImpactHammerClass;

	/** Set true to make impact hammer part of default inventory. */
	UPROPERTY(EditDefaultsOnly, Category = Game)
		bool bGameHasImpactHammer;

	/** workaround for call chain from engine, SetPlayerDefaults() could be called while pawn is alive to reset its values but we don't want it to do new spawn stuff like spawning inventory unless it's called from RestartPlayer() */
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bSetPlayerDefaultsNewSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Game")
		bool bPlayersStartWithArmor;

	TAssetSubclassOf<class AUTArmor> StartingArmorObject;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game")
		TSubclassOf<class AUTArmor> StartingArmorClass;

	/** If true remove any pawns in the level when match starts, (normally true to clean up after warm up). 
	    Pawns will not be removed if in standalone or PIE. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game")
		bool bRemovePawnsAtStart;

	/** Count of total kills during warm up. */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
		int32 WarmupKills;

	/** assign squad to player - note that humans can have a squad for bots to follow their lead
	 * this method should always result in a valid squad being assigned
	 */
	virtual void AssignDefaultSquadFor(AController* C);

	virtual void EntitlementQueryComplete(bool bWasSuccessful, const class FUniqueNetId& UniqueId, const FString& Namespace, const FString& ErrorMessage);

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	UFUNCTION(BlueprintImplementableEvent)
	void PostInitGame(const FString& Options);
	/** add a mutator by string path name */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Mutator)
	virtual void AddMutator(const FString& MutatorPath);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Mutator)
	virtual void AddMutatorClass(TSubclassOf<AUTMutator> MutClass);
	virtual void InitGameState() override;
	virtual APlayerController* Login(class UPlayer* NewPlayer, ENetRole RemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void RemoveAllPawns();
	virtual void RestartGame();
	virtual void BeginGame();
	virtual bool AllowPausing(APlayerController* PC) override;
	virtual bool AllowCheats(APlayerController* P) override;
	virtual bool IsEnemy(class AController* First, class AController* Second);
	virtual void Killed(class AController* Killer, class AController* KilledPlayer, class APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);
	virtual void NotifyKilled(AController* Killer, AController* Killed, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void ScorePickup(AUTPickup* Pickup, AUTPlayerState* PickedUpBy, AUTPlayerState* LastPickedUpBy);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
		void ScoreDamage(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void ScoreKill(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
		bool bTrackKillAssists;

	virtual void TrackKillAssists(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType, AUTPlayerState* KillerPlayerState, AUTPlayerState* OtherPlayerState);

	TMap<TSubclassOf<UDamageType>, int32> EnemyKillsByDamageType;

	/** Score teammate killing another teammate. */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void ScoreTeamKill(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void ScoreObject(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	bool CheckScore(AUTPlayerState* Scorer);

	virtual void FindAndMarkHighScorer();
	virtual void SetEndGameFocus(AUTPlayerState* Winner);	
	virtual void PickMostCoolMoments(bool bClearCoolMoments = false, int32 CoolMomentsToShow = 1);

	UFUNCTION(BlueprintCallable, Category = UTGame)
	virtual void EndGame(AUTPlayerState* Winner, FName Reason);

	/** Return true if human player won offline challenge. */
	virtual bool PlayerWonChallenge();

	virtual void StartMatch();
	virtual void EndMatch();
	virtual void BroadcastDeathMessage(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType);
	virtual void PlayEndOfMatchMessage();

	UFUNCTION(BlueprintCallable, Category = UTGame)
	virtual void DiscardInventory(APawn* Other, AController* Killer = NULL);

	virtual void OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState) override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void PostLogin( APlayerController* NewPlayer );
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* aPlayer);
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	UFUNCTION(BlueprintCallable, Category = UTGame)
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
	virtual FString InitNewPlayer(class APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	virtual void GiveDefaultInventory(APawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = UTGame)
	void NotifyPlayerDefaultsSet(APawn* PlayerPawn);

	virtual float OverrideRespawnTime(AUTPickupInventory* Pickup, TSubclassOf<AUTInventory> InventoryType);

	/** Return true if playerstart P should be avoided for this game mode. */
	virtual bool AvoidPlayerStart(class AUTPlayerStart* P);

	virtual void InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual class AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual float RatePlayerStart(APlayerStart* P, AController* Player);
	virtual float AdjustNearbyPlayerStartScore(const AController* Player, const AController* OtherController, const ACharacter* OtherCharacter, const FVector& StartLoc, const APlayerStart* P);
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	virtual bool ReadyToStartMatch_Implementation() override;

	virtual bool HasMatchStarted() const override;
	virtual bool IsMatchInProgress() const override;
	virtual bool HasMatchEnded() const override;
	virtual void SetMatchState(FName NewState) override;
	virtual void CallMatchStateChangeNotify();

	virtual void HandleCountdownToBegin();
	virtual void CheckCountDown();

	virtual void HandlePlayerIntro();
	virtual void EndPlayerIntro();

	virtual void HandleMatchHasStarted();
	virtual void AnnounceMatchStart();
	virtual void HandleMatchHasEnded() override;
	virtual void HandleEnteringOvertime();
	virtual void HandleMatchInOvertime();

	UFUNCTION(BlueprintNativeEvent)
	void TravelToNextMap();

	virtual void StopReplayRecording();

	virtual void RecreateLobbyBeacon();
	virtual void DefaultTimer() override;
	virtual void CheckGameTime();

	virtual void SendEveryoneBackToLobbyGameAbandoned();

	/**  Used to check when time has run out if there is a winner.  If there is a tie, return NULL to enter overtime. **/	
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	AUTPlayerState* IsThereAWinner(bool& bTied);

	// Allows gametypes to build their rules settings for the mid game menu.
	virtual FText BuildServerRules(AUTGameState* GameState);

	/**
	 *	Builds a \t separated list of rules that will be sent out to clients when they request server info via the UTServerBeaconClient.  
	 **/
	virtual void BuildServerResponseRules(FString& OutRules);

	void AddKillEventToReplay(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType);
	void AddMultiKillEventToReplay(AController* Killer, int32 MultiKillLevel);
	void AddSpreeKillEventToReplay(AController* Killer, int32 SpreeLevel);
	
	UPROPERTY(config)
	bool bRecordReplays;

	/** The standard IsHandlingReplays() codepath is not flexible enough for UT, this is the compromise */
	virtual bool UTIsHandlingReplays();

	virtual float GetLineUpTime(LineUpTypes LineUpType);

protected:

	/** Returns random bot character skill matched to current GameDifficulty. */
	virtual UUTBotCharacter* ChooseRandomCharacter(uint8 TeamNum);

	/** Adds a bot to the game */
	virtual class AUTBotPlayer* AddBot(uint8 TeamNum = 255);

	virtual class AUTBotPlayer* AddNamedBot(const FString& BotName, uint8 TeamNum = 255);
	virtual class AUTBotPlayer* AddAssetBot(const FStringAssetReference& BotAssetPath, uint8 TeamNum = 255);
	/** check for adding/removing bots to satisfy BotFillCount */
	virtual void CheckBotCount();

	/** Immediately remove all extra bots. */
	virtual void RemoveExtraBots();

	/** returns whether we should allow removing the given bot to satisfy the desired player/bot count settings
	 * generally used to defer destruction of bots that currently are important to the current game state, like flag carriers
	 */
	virtual bool AllowRemovingBot(AUTBotPlayer* B);

public:
	/** adds a bot to the game, ignoring game settings */
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual class AUTBotPlayer* ForceAddBot(uint8 TeamNum = 255);
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual class AUTBotPlayer* ForceAddNamedBot(const FString& BotName, uint8 TeamNum = 255);
	/** sets bot count, ignoring startup settings */
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual void SetBotCount(uint8 NewCount);
	/** adds num bots to current total */
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual void AddBots(uint8 Num);
	/** Remove all bots */
	UFUNCTION(Exec, BlueprintCallable, Category = AI)
	virtual void KillBots();

	/** Starts a line-up of the specified type*/
	UFUNCTION(Exec, BlueprintCallable, Category = LineUp)
	virtual void BeginLineUp(const FString& LineUpTypeName);

	/** Ends any active line-up*/
	UFUNCTION(Exec, BlueprintCallable, Category = LineUp)
	virtual void EndLineUp();

	/** NOTE: return value is a workaround for blueprint bugs involving ref parameters and is not used */
	UFUNCTION(BlueprintNativeEvent)
	bool ModifyDamage(UPARAM(ref) int32& Damage, UPARAM(ref) FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType);

	/** return true to prevent the passed in pawn from dying (i.e. from Died()) */
	UFUNCTION(BlueprintNativeEvent)
	bool PreventDeath(APawn* KilledPawn, AController* Killer, TSubclassOf<UDamageType> DamageType, const FHitResult& HitInfo);

	/** used to modify, remove, and replace Actors. Return false to destroy the passed in Actor. Default implementation queries mutators.
	 * note that certain critical Actors such as PlayerControllers can't be destroyed, but we'll still call this code path to allow mutators
	 * to change properties on them
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	bool CheckRelevance(AActor* Other);

	/** changes world gravity to the specified value */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = World)
	void SetWorldGravity(float NewGravity);

	/** set or change a player's team
	 * NewTeam is a request, not a guarantee (game mode may force balanced teams, for example)
	 */
	UFUNCTION(BlueprintCallable, Category = TeamGame)
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam = 255, bool bBroadcast = true);
	/** pick the best team to place this player to keep the teams as balanced as possible
	 * passed in team number is used as tiebreaker if the teams would be just as balanced either way
	 */

	UPROPERTY()
	bool bRankedSession;
	UPROPERTY()
	bool bUseMatchmakingSession;

	virtual TSubclassOf<class AGameSession> GetGameSessionClass() const;
	
	virtual void PreInitializeComponents() override;

	virtual void GameObjectiveInitialized(AUTGameObjective* Obj);

	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList) override;

	// Creates the URL options for custom games
	virtual void CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps);

	virtual TSharedPtr<TAttributePropertyBase> FindGameURLOption(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, const FString& SearchKey);

#if !UE_SERVER
	/** called on the default object of this class by the UI to create widgets to manipulate this game type's settings
	 * you can use TAttributeProperty<> to easily implement get/set delegates that map directly to the config property address
	 * add any such to the ConfigProps array so the menu maintains the shared pointer
	 */
	virtual void CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers);
	virtual FString GetHUBPregameFormatString();
#endif


	/** returns whether the given map name is appropriate for this game type
	 * this is just for UI and doesn't prevent the map from loading via e.g. the console
	 */
	virtual bool SupportsMap(const FString& MapName) const
	{
		// TEMP HACK: sublevel that shouldn't be shown
		if (MapName.EndsWith(TEXT("DM-Circuit_FX")))
		{
			return false;
		}

		return MapPrefix.Len() == 0 || MapName.StartsWith(MapPrefix + TEXT("-"));
	}

	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute = false);

	UFUNCTION(BlueprintCallable, Category = Messaging)
	virtual void BlueprintBroadcastLocalized( AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch = 0, APlayerState* RelatedPlayerState_1 = NULL, APlayerState* RelatedPlayerState_2 = NULL, UObject* OptionalObject = NULL );

	UFUNCTION(BlueprintCallable, Category = Messaging)
	virtual void BlueprintSendLocalized( AActor* Sender, AUTPlayerController* Receiver, TSubclassOf<ULocalMessage> Message, int32 Switch = 0, APlayerState* RelatedPlayerState_1 = NULL, APlayerState* RelatedPlayerState_2 = NULL, UObject* OptionalObject = NULL );

	/**Broadcasts a localized message only to spectators*/
	UFUNCTION(BlueprintCallable, Category = Messaging)
	virtual void BroadcastSpectator(AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject);

	/**Sends a pickup message to all spectators*/
	virtual void BroadcastSpectatorPickup(AUTPlayerState* PS, FName StatsName, UClass* PickupClass);

	virtual bool CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget) override
	{
		return UTGameState && UTGameState->CanSpectate(Viewer, ViewTarget);
	}

	/** called on the default object of the game class being played to precache announcer sounds
	 * needed because announcers are dynamically loaded for convenience of user announcer packs, so we need to load up the audio we think we'll use at game time
	 */
	virtual void PrecacheAnnouncements(class UUTAnnouncer* Announcer) const;

	/** OverridePickupQuery()
	* when pawn wants to pickup something, mutators are given a chance to modify it. If this function
	* returns true, bAllowPickup will determine if the object can be picked up.
	* Note that overriding bAllowPickup to false from this function without disabling the item in some way will have detrimental effects on bots,
	* since the pickup's AI interface will assume the default behavior and keep telling the bot to pick the item up.
	* @param Other the Pawn that wants the item
	* @param Pickup the Actor containing the item (this may be a UTPickup or it may be a UTDroppedPickup)
	* @param bAllowPickup (out) whether or not the Pickup actor should give its item to Other
	* @return whether or not to override the default behavior with the value of bAllowPickup
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	bool OverridePickupQuery(APawn* Other, TSubclassOf<AUTInventory> ItemClass, AActor* Pickup, bool& bAllowPickup);

	virtual TSubclassOf<class AUTInventory> GetActivatedPowerupPlaceholderClass() { return nullptr; };

protected:

	/** checks whether the mutator is allowed in this gametype and doesn't conflict with any existing mutators */
	virtual bool AllowMutator(TSubclassOf<AUTMutator> MutClass);

	// Updates the MCP with the current game state.  Happens once per minute.
	virtual void UpdateOnlineServer();
	virtual void RegisterServerWithSession();

	virtual void SendEndOfGameStats(FName Reason);
	virtual void UpdateSkillRating();
	virtual void SendLogoutAnalytics(class AUTPlayerState* PS);

	virtual void AwardXP(); 
	virtual float GetScoreForXP(class AUTPlayerState* PS);

	void ReportRankedMatchResults(const FString& MatchRatingType);
	void GetRankedTeamInfo(int32 TeamId, struct FRankedTeamInfo& RankedTeamInfoOut);
	// Base version handles 2 teams
	virtual void PrepareRankedMatchResultGameCustom(struct FRankedMatchResult& MatchResult);
private:
	// hacked into ReceiveBeginPlay() so we can do mutator replacement of Actors and such
	void BeginPlayMutatorHack(FFrame& Stack, RESULT_DECL);

public:
	/**
	 *	Tells an associated lobby that this game is ready for joins.
	 **/
	void NotifyLobbyGameIsReady();

	// How long before a lobby instance times out waiting for players to join and the match to begin.  This is to keep lobby instance servers from sitting around forever.
	UPROPERTY(Config)
	float LobbyInitialTimeoutTime;

	UPROPERTY(Config)
	bool bDisableCloudStats;

	// These options will be forced on the game when played on an instance server using this game mode
	UPROPERTY(Config)
	FString ForcedInstanceGameOptions;

	bool bDedicatedInstance;

	void SendLobbyMessage(const FString& Message, AUTPlayerState* Sender);

	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);

protected:
	UPROPERTY(Config)
	bool bSkipReportingMatchResults;

	// The Address of the Hub this game wants to connect to.
	UPROPERTY(Config)
	FString HubAddress;

	UPROPERTY(Config)
	FString HubKey;

	// A Beacon for communicating back to the lobby
	UPROPERTY(transient)
	AUTServerBeaconLobbyClient* LobbyBeacon;

	FTimerHandle ServerRestartTimerHandle;

	float LastLobbyUpdateTime;
	virtual void ForceLobbyUpdate();

	uint32 HostLobbyListenPort;

	// Update the Lobby with the current stats of the game
	virtual void UpdateLobbyMatchStats();

	// Updates the lobby with the current player list
	virtual void UpdateLobbyPlayerList();

	// Gets the updated score information
	virtual void UpdateLobbyScore(FMatchUpdate& MatchUpdate);

	
	// When players leave/join or during the end of game state
	virtual void UpdatePlayersPresence();
	
	/**
	 * Lock down the session and prevent current/future players from unregistering with this session until the match is complete
	 */
	void LockSession();

	/**
	 * Unlock the session and remove currently inactive players from it
	 */
	void UnlockSession();

	UPROPERTY()
	int32 ExpectedPlayerCount;

public:
	virtual void SendEveryoneBackToLobby();

#if !UE_SERVER
	void BuildPaneHelper(TSharedPtr<SHorizontalBox>& HBox, TSharedPtr<SVerticalBox>& LeftPane, TSharedPtr<SVerticalBox>& RightPane);
	void NewPlayerInfoLine(TSharedPtr<SVerticalBox> VBox, FText DisplayName, TSharedPtr<TAttributeStat> Stat, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
	void NewWeaponInfoLine(TSharedPtr<SVerticalBox> VBox, FText DisplayName, TSharedPtr<TAttributeStat> KillStat, TSharedPtr<struct TAttributeStat> DeathStat, TSharedPtr<struct TAttributeStat> AccuracyStat, TArray<TSharedPtr<struct TAttributeStat> >& StatList);

	virtual void BuildPlayerInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
	virtual void BuildScoreInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
	virtual void BuildRewardInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
	virtual void BuildWeaponInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
	virtual void BuildMovementInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<struct TAttributeStat> >& StatList);
#endif
	// Allow game modes to restrict some content.
	virtual bool ValidateHat(AUTPlayerState* HatOwner, const FString& HatClass);
	
	UPROPERTY(EditDefaultsOnly, Category="Game")
	bool bNoDefaultLeaderHat;

	// Called when the player attempts to restart using AltFire
	UFUNCTION(BlueprintNativeEvent, Category="Game")
	bool PlayerCanAltRestart( APlayerController* Player );

	virtual bool HandleRallyRequest(AController* C) { return false; };
	virtual bool CompleteRallyRequest(AController* C) { return false; };
	virtual void FinishRallyRequest(AController *C) {};

	virtual void GetGameURLOptions(const TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, TArray<FString>& OptionsList, int32& DesiredPlayerCount);

	// Called from the Beacon, it makes this server become a dedicated instance
	virtual void BecomeDedicatedInstance(FGuid HubGuid, int32 InstanceID);

	FString GetMapPrefix()
	{
		return MapPrefix;
	}

	UPROPERTY(Config)
	int32 MapVoteTime;

	virtual void HandleMapVote();
	virtual void CullMapVotes();
	virtual void TallyMapVotes();

	/** Maximum time client can be ahead, without resetting. */
	UPROPERTY(GlobalConfig)
	float MaxTimeMargin;

	/** Maximum time client can be behind. */
	UPROPERTY(GlobalConfig)
	float MinTimeMargin;

	/** Accepted drift in clocks. */
	UPROPERTY(GlobalConfig)
	float TimeMarginSlack;

	/** Whether speedhack detection is enabled. */
	UPROPERTY(GlobalConfig)
	bool bSpeedHackDetection;

	virtual void NotifySpeedHack(ACharacter* Character);

	/** Overriden so we dont go into MatchState::LeavingMap state, which happens regardless if the travel fails
	* On failed map changes, the game will be stuck in a LeavingMap state
	*/
	virtual void StartToLeaveMap() {}

	/** Change playerstate properties as needed when becoming inactive. */
	virtual void SetPlayerStateInactive(APlayerState* NewPlayerState);

	/**Overridden to replicate Inactive Player States  */
	virtual void AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC) override;
	virtual bool FindInactivePlayer(APlayerController* PC) override;

	virtual void GatherRequiredRedirects(TArray<FPackageRedirectReference>& Redirects) override;

private:
	UTAntiCheatModularFeature* AntiCheatEngine;

public:
	UPROPERTY(Config)
	bool bDisableMapVote;

	FString GetGameRulesDescription();

	UFUNCTION(exec)
		virtual void GetGood();

	// Will be true if this instance is rank locked
	bool bRankLocked;

	// This is the match's combined ELO rank.  It incorporates the both the level and the sublevel and is set with the url option
	// ?RankCheck=xxxxx
	int32 RankCheck;

#if !UE_SERVER
	// The hud will create a spawn window that is displayed when the player has died.  
	virtual TSharedPtr<SUTHUDWindow> CreateSpawnWindow(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner);
#endif

	// Returns true if a player can activate a boost
	virtual bool CanBoost(AUTPlayerState* Who);

	/** PlayerController wants to trigger boost, return true if he can handle it. Mutator hooks as well. */
	virtual bool TriggerBoost(AUTPlayerState* Who);

	// Look to see if we can attempt a boost.  It will use up the charge if we can and return true, otherwise it returns false.
	virtual bool AttemptBoost(AUTPlayerState* Who);

	/**
	 *	Calculates the Com menu switch for a command command tag.
	 *
	 *  CommandTag - The tag we are looking for
	 *  ContextActor - The actor that is the current focus of the crosshair
	 *  Instigator - The Player Controller that owns the menu
	 **/
	virtual int32 GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* Instigator, UWorld* World);

	/**
	 *	Sends a voice communication message.  If Target is not null, then it will only be sent to the target and the sender, otherwise all of the players (or just
	 *  their teammates in a team game) will get the message.
	 * 
	 *  Sender is the controller sending this message.
	 *  Target is the player state of player who was the context target of this message.
	 *  Switch is the UTCharacterVoice switch to play
	 *  ContextObject is the object that 
	 **/
	virtual void SendComsMessage( AUTPlayerController* Sender, AUTPlayerState* Target, int32 Switch = 0);

	virtual void SendBotVoiceOrder(AUTPlayerController* Sender, AUTBot* Target, int32 Switch = 0);

	/** Holds the tutorial mask to set when this game completes.  */
	UPROPERTY(BlueprintReadOnly, Category=Onboarding)
	int32 TutorialMask;

	/** preload assets on client that are loaded on server but aren't immediately used by/replicated to client so they may not be loaded
	 * example: pawn classes for class-based modes
	 * prevents client hitches when the objects are replicated the first time
	 */
	virtual void PreloadClientAssets(TArray<UObject*>& ObjList) const;

	UPROPERTY(EditDefaultsOnly, Meta = (MetaClass = "UObject"))
	TArray<FStringClassReference> AssetsToPreloadOnClients;

	UPROPERTY()
	uint32 bDebugHitScanReplication : 1;

	// Holds the game time at which returning players are no longer guarenteed a spot.
	UPROPERTY()
	float ReturningPlayerGraceCutoff;

	/**
	 *	Kick any players who have been idle from the match
	 **/
	UFUNCTION(BlueprintCallable, Category = Game)
	void KickIdlePlayers();

	/**
	 *	Creates all of the replicated data needed for map voting
	 **/
	virtual bool PrepareMapVote();

	/**
	 *	Shuts down a game instance
	 **/
	virtual void ShutdownGameInstance();

	virtual void ForceEndServer();
	
	virtual void SendVoiceChatLoginToken(AUTPlayerController* PC);

public:
	// If this is a single player game, or an instance server, this will hold the unique tag of the ruleset being used if relevant
	UPROPERTY(BlueprintReadOnly,Category = Game)
	FString ActiveRuleTag;

	virtual bool AllowTextMessage_Implementation(FString& Msg, bool bIsTeamMessage, AUTBasePlayerController* Sender);

};

