// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystemUtils.h"
#include "UTMultiKillMessage.h"
#include "UTSpreeMessage.h"
#include "UTRemoteRedeemer.h"
#include "UTGameMessage.h"
#include "Net/UnrealNetwork.h"
#include "UTTimerMessage.h"
#include "UTMutator.h"
#include "UTReplicatedMapInfo.h"
#include "UTPickup.h"
#include "UTArmor.h"
#include "UTPainVolume.h"
#include "StatNames.h"
#include "UTGameEngine.h"
#include "UTBaseGameMode.h"
#include "UTGameInstance.h"
#include "UTWorldSettings.h"
#include "Engine/DemoNetDriver.h"
#include "UTKillcamPlayback.h"
#include "UTAnalytics.h"
#include "ContentStreaming.h"
#include "UTLineUpZone.h"
#include "UTLineUpHelper.h"
#include "Runtime/Analytics/Analytics/Public/AnalyticsEventAttribute.h"
#include "UTIntermissionBeginInterface.h"
#include "UTPlayerStart.h"
#include "UTTeamPlayerStart.h"
#include "UTDemoRecSpectator.h"
#include "UTGameVolume.h"
#include "UserActivityTracking.h"

AUTGameState::AUTGameState(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MultiKillMessageClass = UUTMultiKillMessage::StaticClass();
	SpreeMessageClass = UUTSpreeMessage::StaticClass();
	MultiKillDelay = 3.0f;
	SpawnProtectionTime = 2.f;
	bWeaponStay = true;
	bAllowTeamSwitches = true;
	KickThreshold=51.0f;
	TauntSelectionIndex = 0;
	bPersistentKillIconMessages = false;
	bTeamProjHits = false;
	bTeamCollision = false;
	NetUpdateFrequency = 60.0f;

	// We want to be ticked.
	PrimaryActorTick.bCanEverTick = true;

	ServerName = TEXT("My First Server");
	ServerMOTD = TEXT("Welcome!");
	SecondaryAttackerStat = NAME_None;
	GoalScoreText = NSLOCTEXT("UTScoreboard", "GoalScoreFormat", "{0} Frags");

	GameScoreStats.Add(NAME_AttackerScore);
	GameScoreStats.Add(NAME_DefenderScore);
	GameScoreStats.Add(NAME_SupporterScore);
	GameScoreStats.Add(NAME_Suicides);

	GameScoreStats.Add(NAME_UDamageTime);
	GameScoreStats.Add(NAME_BerserkTime);
	GameScoreStats.Add(NAME_InvisibilityTime);
	GameScoreStats.Add(NAME_UDamageCount);
	GameScoreStats.Add(NAME_BerserkCount);
	GameScoreStats.Add(NAME_InvisibilityCount);
	GameScoreStats.Add(NAME_KegCount);
	GameScoreStats.Add(NAME_BootJumps);
	GameScoreStats.Add(NAME_ShieldBeltCount);
	GameScoreStats.Add(NAME_ArmorVestCount);
	GameScoreStats.Add(NAME_ArmorPadsCount);
	GameScoreStats.Add(NAME_HelmetCount);

	TeamStats.Add(NAME_TeamKills);
	TeamStats.Add(NAME_UDamageTime);
	TeamStats.Add(NAME_BerserkTime);
	TeamStats.Add(NAME_InvisibilityTime);
	TeamStats.Add(NAME_UDamageCount);
	TeamStats.Add(NAME_BerserkCount);
	TeamStats.Add(NAME_InvisibilityCount);
	TeamStats.Add(NAME_KegCount);
	TeamStats.Add(NAME_BootJumps);
	TeamStats.Add(NAME_ShieldBeltCount);
	TeamStats.Add(NAME_ArmorVestCount);
	TeamStats.Add(NAME_ArmorPadsCount);
	TeamStats.Add(NAME_HelmetCount);

	WeaponStats.Add(NAME_ImpactHammerKills);
	WeaponStats.Add(NAME_EnforcerKills);
	WeaponStats.Add(NAME_BioRifleKills);
	WeaponStats.Add(NAME_BioLauncherKills);
	WeaponStats.Add(NAME_ShockBeamKills);
	WeaponStats.Add(NAME_ShockCoreKills);
	WeaponStats.Add(NAME_ShockComboKills);
	WeaponStats.Add(NAME_LinkKills);
	WeaponStats.Add(NAME_LinkBeamKills);
	WeaponStats.Add(NAME_MinigunKills);
	WeaponStats.Add(NAME_MinigunShardKills);
	WeaponStats.Add(NAME_FlakShardKills);
	WeaponStats.Add(NAME_FlakShellKills);
	WeaponStats.Add(NAME_RocketKills);
	WeaponStats.Add(NAME_SniperKills);
	WeaponStats.Add(NAME_SniperHeadshotKills);
	WeaponStats.Add(NAME_RedeemerKills);
	WeaponStats.Add(NAME_InstagibKills);
	WeaponStats.Add(NAME_TelefragKills);

	WeaponStats.Add(NAME_ImpactHammerDeaths);
	WeaponStats.Add(NAME_EnforcerDeaths);
	WeaponStats.Add(NAME_BioRifleDeaths);
	WeaponStats.Add(NAME_BioLauncherDeaths);
	WeaponStats.Add(NAME_ShockBeamDeaths);
	WeaponStats.Add(NAME_ShockCoreDeaths);
	WeaponStats.Add(NAME_ShockComboDeaths);
	WeaponStats.Add(NAME_LinkDeaths);
	WeaponStats.Add(NAME_LinkBeamDeaths);
	WeaponStats.Add(NAME_MinigunDeaths);
	WeaponStats.Add(NAME_MinigunShardDeaths);
	WeaponStats.Add(NAME_FlakShardDeaths);
	WeaponStats.Add(NAME_FlakShellDeaths);
	WeaponStats.Add(NAME_RocketDeaths);
	WeaponStats.Add(NAME_SniperDeaths);
	WeaponStats.Add(NAME_SniperHeadshotDeaths);
	WeaponStats.Add(NAME_RedeemerDeaths);
	WeaponStats.Add(NAME_InstagibDeaths);
	WeaponStats.Add(NAME_TelefragDeaths);

	WeaponStats.Add(NAME_BestShockCombo);
	WeaponStats.Add(NAME_AirRox);
	WeaponStats.Add(NAME_AmazingCombos);
	WeaponStats.Add(NAME_FlakShreds);
	WeaponStats.Add(NAME_AirSnot);

	RewardStats.Add(NAME_MultiKillLevel0);
	RewardStats.Add(NAME_MultiKillLevel1);
	RewardStats.Add(NAME_MultiKillLevel2);
	RewardStats.Add(NAME_MultiKillLevel3);

	RewardStats.Add(NAME_SpreeKillLevel0);
	RewardStats.Add(NAME_SpreeKillLevel1);
	RewardStats.Add(NAME_SpreeKillLevel2);
	RewardStats.Add(NAME_SpreeKillLevel3);
	RewardStats.Add(NAME_SpreeKillLevel4);

	MovementStats.Add(NAME_RunDist);
	MovementStats.Add(NAME_InAirDist);
	MovementStats.Add(NAME_SwimDist);
	MovementStats.Add(NAME_TranslocDist);
	MovementStats.Add(NAME_NumDodges);
	MovementStats.Add(NAME_NumWallDodges);
	MovementStats.Add(NAME_NumJumps);
	MovementStats.Add(NAME_NumLiftJumps);
	MovementStats.Add(NAME_NumFloorSlides);
	MovementStats.Add(NAME_NumWallRuns);
	MovementStats.Add(NAME_NumImpactJumps);
	MovementStats.Add(NAME_NumRocketJumps);
	MovementStats.Add(NAME_SlideDist);
	MovementStats.Add(NAME_WallRunDist);

	WeaponStats.Add(NAME_EnforcerShots);
	WeaponStats.Add(NAME_BioRifleShots);
	WeaponStats.Add(NAME_BioLauncherShots);
	WeaponStats.Add(NAME_ShockRifleShots);
	WeaponStats.Add(NAME_LinkShots);
	WeaponStats.Add(NAME_MinigunShots);
	WeaponStats.Add(NAME_FlakShots);
	WeaponStats.Add(NAME_RocketShots);
	WeaponStats.Add(NAME_SniperShots);
	WeaponStats.Add(NAME_RedeemerShots);
	WeaponStats.Add(NAME_InstagibShots);

	WeaponStats.Add(NAME_EnforcerHits);
	WeaponStats.Add(NAME_BioRifleHits);
	WeaponStats.Add(NAME_BioLauncherHits);
	WeaponStats.Add(NAME_ShockRifleHits);
	WeaponStats.Add(NAME_LinkHits);
	WeaponStats.Add(NAME_MinigunHits);
	WeaponStats.Add(NAME_FlakHits);
	WeaponStats.Add(NAME_RocketHits);
	WeaponStats.Add(NAME_SniperHits);
	WeaponStats.Add(NAME_RedeemerHits);
	WeaponStats.Add(NAME_InstagibHits);

	HighlightMap.Add(HighlightNames::TopScorer, NSLOCTEXT("AUTGameMode", "HighlightTopScore", "Top Score with {0} points"));
	HighlightMap.Add(HighlightNames::TopScorerRed, NSLOCTEXT("AUTGameMode", "HighlightTopScoreRed", "Top Score with {0} points"));
	HighlightMap.Add(HighlightNames::TopScorerBlue, NSLOCTEXT("AUTGameMode", "HighlightTopScoreBlue", "Top Score with {0} points"));
	HighlightMap.Add(HighlightNames::MostKills, NSLOCTEXT("AUTGameMode", "MostKills", "Most Kills ({0})"));
	HighlightMap.Add(HighlightNames::LeastDeaths, NSLOCTEXT("AUTGameMode", "LeastDeaths", "Least Deaths with {0}"));
	HighlightMap.Add(HighlightNames::BestKD, NSLOCTEXT("AUTGameMode", "BestKD", "Kill/Death ratio {0}"));
	HighlightMap.Add(HighlightNames::MostWeaponKills, NSLOCTEXT("AUTGameMode", "MostWeaponKills", "{0} kills with {1}"));
	HighlightMap.Add(HighlightNames::BestCombo, NSLOCTEXT("AUTGameMode", "BestCombo", "Most Impressive Shock Combo"));
	HighlightMap.Add(HighlightNames::MostHeadShots, NSLOCTEXT("AUTGameMode", "MostHeadShots", "Most Headshots ({0})"));
	HighlightMap.Add(HighlightNames::MostAirRockets, NSLOCTEXT("AUTGameMode", "MostAirRockets", "Most Air Rockets ({0})"));
	HighlightMap.Add(HighlightNames::KillsAward, NSLOCTEXT("AUTGameMode", "KillsAward", "{0} Kills"));
	HighlightMap.Add(HighlightNames::KillingBlowsAward, NSLOCTEXT("AUTGameMode", "KillingBlowsAward", "{0} Killing Blows"));
	HighlightMap.Add(HighlightNames::DamageAward, NSLOCTEXT("AUTGameMode", "DamageAward", "{0} Damage Done"));
	HighlightMap.Add(HighlightNames::ParticipationAward, NSLOCTEXT("AUTGameMode", "ParticipationAward", "Was there, more or less"));

	HighlightMap.Add(NAME_AmazingCombos, NSLOCTEXT("AUTGameMode", "AmazingCombosHL", "{0} Amazing Combos"));
	HighlightMap.Add(NAME_SniperHeadshotKills, NSLOCTEXT("AUTGameMode", "SniperHeadshotKillsHL", "{0} Headshot Kills"));
	HighlightMap.Add(NAME_AirRox, NSLOCTEXT("AUTGameMode", "AirRoxHL", "{0} Air Rocket Kills"));
	HighlightMap.Add(NAME_FlakShreds, NSLOCTEXT("AUTGameMode", "FlakShredsHL", "{0} Flak Shred Kills"));
	HighlightMap.Add(NAME_AirSnot, NSLOCTEXT("AUTGameMode", "AirSnotHL", "{0} Air Snot Kills"));
	HighlightMap.Add(NAME_MultiKillLevel0, NSLOCTEXT("AUTGameMode", "MultiKillLevel0", "Double Kill X{0}"));
	HighlightMap.Add(NAME_MultiKillLevel1, NSLOCTEXT("AUTGameMode", "MultiKillLevel1", "Multi Kill X{0}"));
	HighlightMap.Add(NAME_MultiKillLevel2, NSLOCTEXT("AUTGameMode", "MultiKillLevel2", "Ultra Kill X{0}"));
	HighlightMap.Add(NAME_MultiKillLevel3, NSLOCTEXT("AUTGameMode", "MultiKillLevel3", "Monster Kill X{0}"));
	HighlightMap.Add(NAME_SpreeKillLevel0, NSLOCTEXT("AUTGameMode", "SpreeKillLevel0", "Killing Spree X{0}"));
	HighlightMap.Add(NAME_SpreeKillLevel1, NSLOCTEXT("AUTGameMode", "SpreeKillLevel1", "Rampage X{0}"));
	HighlightMap.Add(NAME_SpreeKillLevel2, NSLOCTEXT("AUTGameMode", "SpreeKillLevel2", "Dominating X{0}"));
	HighlightMap.Add(NAME_SpreeKillLevel3, NSLOCTEXT("AUTGameMode", "SpreeKillLevel3", "Unstoppable X{0}"));
	HighlightMap.Add(NAME_SpreeKillLevel4, NSLOCTEXT("AUTGameMode", "SpreeKillLevel4", "Godlike X{0}"));

	ShortHighlightMap.Add(HighlightNames::TopScorer, NSLOCTEXT("AUTGameMode", "ShortHighlightTopScore", "Top Score overall"));
	ShortHighlightMap.Add(HighlightNames::TopScorerRed, NSLOCTEXT("AUTGameMode", "ShortHighlightTopScoreRed", "Red Team Top Score"));
	ShortHighlightMap.Add(HighlightNames::TopScorerBlue, NSLOCTEXT("AUTGameMode", "ShortHighlightTopScoreBlue", "Blue Team Top Score"));
	ShortHighlightMap.Add(HighlightNames::MostKills, NSLOCTEXT("AUTGameMode", "ShortMostKills", "Most Kills"));
	ShortHighlightMap.Add(HighlightNames::LeastDeaths, NSLOCTEXT("AUTGameMode", "ShortLeastDeaths", "Hard to Kill"));
	ShortHighlightMap.Add(HighlightNames::BestKD, NSLOCTEXT("AUTGameMode", "ShortBestKD", "Best Kill/Death ratio "));
	ShortHighlightMap.Add(HighlightNames::MostWeaponKills, NSLOCTEXT("AUTGameMode", "ShortMostWeaponKills", "Most Kills with {1}"));
	ShortHighlightMap.Add(HighlightNames::BestCombo, NSLOCTEXT("AUTGameMode", "ShortBestCombo", "Most Impressive Shock Combo"));
	ShortHighlightMap.Add(HighlightNames::MostHeadShots, NSLOCTEXT("AUTGameMode", "ShortMostHeadShots", "Most Headshots"));
	ShortHighlightMap.Add(HighlightNames::MostAirRockets, NSLOCTEXT("AUTGameMode", "ShortMostAirRockets", "Air to Air"));
	ShortHighlightMap.Add(HighlightNames::ParticipationAward, NSLOCTEXT("AUTGameMode", "ShortParticipationAward", "Participation Award"));

	ShortHighlightMap.Add(NAME_AmazingCombos, NSLOCTEXT("AUTGameMode", "ShortAmazingCombos", "Amazing Combos"));
	ShortHighlightMap.Add(NAME_SniperHeadshotKills, NSLOCTEXT("AUTGameMode", "ShortSniperHeadshotKills", "HeadHunter"));
	ShortHighlightMap.Add(NAME_AirRox, NSLOCTEXT("AUTGameMode", "ShortAirRox", "Rocketman"));
	ShortHighlightMap.Add(NAME_FlakShreds, NSLOCTEXT("AUTGameMode", "ShortFlakShreds", "Pulverizer"));
	ShortHighlightMap.Add(NAME_AirSnot, NSLOCTEXT("AUTGameMode", "ShortAirSnot", "Snot Funny"));
	ShortHighlightMap.Add(NAME_MultiKillLevel0, NSLOCTEXT("AUTGameMode", "ShortMultiKillLevel0", "Double Kill"));
	ShortHighlightMap.Add(NAME_MultiKillLevel1, NSLOCTEXT("AUTGameMode", "ShortMultiKillLevel1", "Multi Kill"));
	ShortHighlightMap.Add(NAME_MultiKillLevel2, NSLOCTEXT("AUTGameMode", "ShortMultiKillLevel2", "Ultra Kill"));
	ShortHighlightMap.Add(NAME_MultiKillLevel3, NSLOCTEXT("AUTGameMode", "ShortMultiKillLevel3", "Monster Kill"));
	ShortHighlightMap.Add(NAME_SpreeKillLevel0, NSLOCTEXT("AUTGameMode", "ShortSpreeKillLevel0", "Killing Spree"));
	ShortHighlightMap.Add(NAME_SpreeKillLevel1, NSLOCTEXT("AUTGameMode", "ShortSpreeKillLevel1", "Rampage"));
	ShortHighlightMap.Add(NAME_SpreeKillLevel2, NSLOCTEXT("AUTGameMode", "ShortSpreeKillLevel2", "Dominating"));
	ShortHighlightMap.Add(NAME_SpreeKillLevel3, NSLOCTEXT("AUTGameMode", "ShortSpreeKillLevel3", "Unstoppable"));
	ShortHighlightMap.Add(NAME_SpreeKillLevel4, NSLOCTEXT("AUTGameMode", "ShortSpreeKillLevel4", "Godlike"));
	ShortHighlightMap.Add(HighlightNames::KillsAward, NSLOCTEXT("AUTGameMode", "ShortKillsAward", "Kills"));
	ShortHighlightMap.Add(HighlightNames::KillingBlowsAward, NSLOCTEXT("AUTGameMode", "ShortKillingBlowsAward", "Killing Blows"));
	ShortHighlightMap.Add(HighlightNames::DamageAward, NSLOCTEXT("AUTGameMode", "ShortDamageAward", "I'd hit that"));

	HighlightPriority.Add(HighlightNames::TopScorer, 10.f);
	HighlightPriority.Add(HighlightNames::TopScorerRed, 5.f);
	HighlightPriority.Add(HighlightNames::TopScorerBlue, 5.f);
	HighlightPriority.Add(HighlightNames::MostKills, 3.3f);
	HighlightPriority.Add(HighlightNames::LeastDeaths, 1.f);
	HighlightPriority.Add(HighlightNames::BestKD, 2.f);
	HighlightPriority.Add(HighlightNames::MostWeaponKills, 2.f);
	HighlightPriority.Add(HighlightNames::BestCombo, 2.f);
	HighlightPriority.Add(HighlightNames::MostHeadShots, 2.f);
	HighlightPriority.Add(HighlightNames::MostAirRockets, 2.f);

	HighlightPriority.Add(NAME_AmazingCombos, 1.f);
	HighlightPriority.Add(NAME_SniperHeadshotKills, 1.f);
	HighlightPriority.Add(NAME_AirRox, 1.f);
	HighlightPriority.Add(NAME_FlakShreds, 1.f);
	HighlightPriority.Add(NAME_AirSnot, 1.f);
	HighlightPriority.Add(NAME_MultiKillLevel0, 0.5f);
	HighlightPriority.Add(NAME_MultiKillLevel1, 0.5f);
	HighlightPriority.Add(NAME_MultiKillLevel2, 1.5f);
	HighlightPriority.Add(NAME_MultiKillLevel3, 2.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel0, 2.f);
	HighlightPriority.Add(NAME_SpreeKillLevel1, 2.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel2, 3.f);
	HighlightPriority.Add(NAME_SpreeKillLevel3, 3.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel4, 4.f);
	HighlightPriority.Add(HighlightNames::KillsAward, 0.2f);
	HighlightPriority.Add(HighlightNames::KillingBlowsAward, 0.18f);
	HighlightPriority.Add(HighlightNames::DamageAward, 0.15f);
	HighlightPriority.Add(HighlightNames::ParticipationAward, 0.1f);

	GameOverStatus = NSLOCTEXT("UTGameState", "PostGame", "Game Over");
	MapVoteStatus = NSLOCTEXT("UTGameState", "Mapvote", "Map Vote");
	PreGameStatus = NSLOCTEXT("UTGameState", "PreGame", "Pre-Game");
	NeedPlayersStatus = NSLOCTEXT("UTGameState", "NeedPlayers", "Waiting for Players");
	OvertimeStatus = NSLOCTEXT("UTCTFGameState", "Overtime", "Overtime");
	HostStatus = NSLOCTEXT("UTCTFGameState", "HostStatus", "Waiting for Host");
	BoostRechargeMaxCharges = 1;
	BoostRechargeTime = 0.0f; // off by default
	MusicVolume = 1.f;

	bOnlyTeamCanVoteKick = false;
	bDisableVoteKick = false;

	UserInfoQueryRetryTime = 0.5f;

	// more stringent by default
	UnplayableHitchThresholdInMs = 300;
	MaxUnplayableHitchesToTolerate = 1;
	bPlayStatusAnnouncements = false;
	bDebugHitScanReplication = false;

	MapVoteListCount = -1;
	AIDifficulty = 0;

	ActiveLineUpHelper = nullptr;
}

void AUTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTGameState, ReplicatedRemainingTime);
	DOREPLIFETIME(AUTGameState, ScoringPlayerState);
	DOREPLIFETIME(AUTGameState, WinnerPlayerState);
	DOREPLIFETIME(AUTGameState, WinningTeam);
	DOREPLIFETIME(AUTGameState, bStopGameClock);
	DOREPLIFETIME(AUTGameState, TimeLimit);  
	DOREPLIFETIME(AUTGameState, RespawnWaitTime);  
	DOREPLIFETIME_CONDITION(AUTGameState, ForceRespawnTime, COND_InitialOnly);  
	DOREPLIFETIME_CONDITION(AUTGameState, bTeamGame, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, bRankedSession, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, bIsQuickMatch, COND_InitialOnly);
	DOREPLIFETIME(AUTGameState, TeamSwapSidesOffset);
	DOREPLIFETIME_CONDITION(AUTGameState, bIsInstanceServer, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, AIDifficulty, COND_InitialOnly);
	DOREPLIFETIME(AUTGameState, PlayersNeeded);
	DOREPLIFETIME(AUTGameState, HubGuid);

	DOREPLIFETIME_CONDITION(AUTGameState, bWeaponStay, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, GoalScore, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, OverlayEffects, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, OverlayEffects1P, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, SpawnProtectionTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, NumTeams, COND_InitialOnly);

	DOREPLIFETIME_CONDITION(AUTGameState, ServerName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, ServerDescription, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, ServerMOTD, COND_InitialOnly);
	DOREPLIFETIME(AUTGameState, bAllowTeamSwitches);

	DOREPLIFETIME(AUTGameState, ServerSessionId);

	DOREPLIFETIME(AUTGameState, MapVoteList);
	DOREPLIFETIME(AUTGameState, MapVoteListCount);
	DOREPLIFETIME(AUTGameState, VoteTimer);

	DOREPLIFETIME(AUTGameState, bHaveMatchHost);
	DOREPLIFETIME(AUTGameState, bRequireFull);

	DOREPLIFETIME_CONDITION(AUTGameState, BoostRechargeTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, BoostRechargeMaxCharges, COND_InitialOnly);

	DOREPLIFETIME(AUTGameState, bRestrictPartyJoin);
	DOREPLIFETIME(AUTGameState, bTeamProjHits);

	DOREPLIFETIME_CONDITION(AUTGameState, ServerInstanceGUID, COND_InitialOnly);

	DOREPLIFETIME(AUTGameState, ReplayID);
	DOREPLIFETIME(AUTGameState, MatchID);
	DOREPLIFETIME(AUTGameState, LeadLineUpPlayer);

	DOREPLIFETIME(AUTGameState, FCFriendlyLocCount);
	DOREPLIFETIME(AUTGameState, FCEnemyLocCount);
	DOREPLIFETIME(AUTGameState, bDebugHitScanReplication);

	DOREPLIFETIME(AUTGameState, HostIdString);

	DOREPLIFETIME(AUTGameState, ActiveLineUpHelper);
}

void AUTGameState::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	NumTeams = Teams.Num();
	Super::PreReplication(ChangedPropertyTracker);
}

void AUTGameState::UpdateFCFriendlyLocation(AUTPlayerState* AnnouncingPlayer, AUTGameVolume* LocationVolume)
{
	if (AnnouncingPlayer && LocationVolume)
	{
		AnnouncingPlayer->AnnounceLocation(LocationVolume, 1);
		LastFriendlyLocationReportTime = GetWorld()->GetTimeSeconds();
		LastFriendlyLocationName = LocationVolume->VoiceLinesSet;
		FCFriendlyLocCount++;
		ForceNetUpdate();
	}
}

void AUTGameState::UpdateFCEnemyLocation(AUTPlayerState* AnnouncingPlayer, AUTGameVolume* LocationVolume)
{
	if (AnnouncingPlayer && LocationVolume)
	{
		AnnouncingPlayer->AnnounceLocation(LocationVolume, 0);
		LastEnemyLocationReportTime = GetWorld()->GetTimeSeconds();
		LastEnemyLocationName = LocationVolume->VoiceLinesSet;
		FCEnemyLocCount++;
		ForceNetUpdate();
	}
}

void AUTGameState::OnUpdateFriendlyLocation()
{
	LastFriendlyLocationReportTime = GetWorld()->GetTimeSeconds();
}

void AUTGameState::OnUpdateEnemyLocation()
{
	LastEnemyLocationReportTime = GetWorld()->GetTimeSeconds();
}

void AUTGameState::OnRep_GameModeClass()
{
	Super::OnRep_GameModeClass();
	if (GameModeClass != nullptr)
	{
		const AUTGameMode* DefGame = Cast<AUTGameMode>(GameModeClass.GetDefaultObject());
		if (DefGame != nullptr)
		{
			DefGame->PreloadClientAssets(PreloadedAssets);
		}
	}
}

void AUTGameState::AsyncPackageLoaded(UObject* Package)
{
	AsyncLoadedAssets.Add(Package);
}

void AUTGameState::AddOverlayMaterial(UMaterialInterface* NewOverlay, UMaterialInterface* NewOverlay1P)
{
	AddOverlayEffect(FOverlayEffect(NewOverlay), FOverlayEffect(NewOverlay1P));
}

void AUTGameState::AddOverlayEffect(const FOverlayEffect& NewOverlay, const FOverlayEffect& NewOverlay1P)
{
	if (NewOverlay.IsValid() && Role == ROLE_Authority)
	{
		if (NetUpdateTime > 0.0f)
		{
			UE_LOG(UT, Warning, TEXT("UTGameState::AddOverlayMaterial() called after startup; may not take effect on clients"));
		}
		for (int32 i = 0; i < ARRAY_COUNT(OverlayEffects); i++)
		{
			if (OverlayEffects[i] == NewOverlay)
			{
				OverlayEffects1P[i] = NewOverlay1P;
				return;
			}
			else if (!OverlayEffects[i].IsValid())
			{
				OverlayEffects[i] = NewOverlay;
				OverlayEffects1P[i] = NewOverlay1P;
				return;
			}
		}
		UE_LOG(UT, Warning, TEXT("UTGameState::AddOverlayMaterial(): Ran out of slots, couldn't add %s"), *NewOverlay.ToString());
	}
}

void AUTGameState::OnRep_OverlayEffects()
{
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AUTCharacter* UTC = Cast<AUTCharacter>(It->Get());
		if (UTC != NULL)
		{
			UTC->UpdateCharOverlays();
			UTC->UpdateArmorOverlay();
			UTC->UpdateWeaponOverlays();
		}
	}
}

bool AUTGameState::CanSpectate(APlayerController* Viewer, APlayerState* ViewTarget)
{
	if (bTeamGame)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(Viewer);
		return (Viewer->PlayerState->bOnlySpectator || ViewTarget == NULL || ViewTarget->bOnlySpectator || (PC != NULL && PC->GetTeamNum() == 255) || GetWorld()->GetGameState<AUTGameState>()->OnSameTeam(Viewer, ViewTarget));
	}
	return true;
}

void AUTGameState::Tick(float DeltaTime)
{
	ManageMusicVolume(DeltaTime);

	if (bNeedToClearIntermission && !IsMatchIntermission())
	{
		bNeedToClearIntermission = false;
		for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
		{
			UParticleSystemComponent* PSC = *It;
			if (PSC && !PSC->IsTemplate() && PSC->GetOwner() && (PSC->GetOwner()->GetWorld() == GetWorld()) && !Cast<AUTWeapon>(PSC->GetOwner()) && (PSC->CustomTimeDilation == 0.001f))
			{
				PSC->CustomTimeDilation = 1.f;
			}
		}
	}
}

void AUTGameState::ManageMusicVolume(float DeltaTime)
{
	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(GetWorldSettings());
	if (Settings && Settings->MusicComp)
	{
		UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings()); 
		float DesiredVolume = IsMatchInProgress() && !IsMatchIntermission() ? UserSettings->GetSoundClassVolume(EUTSoundClass::GameMusic) : 1.f;
		if (bLocalMenusAreActive) DesiredVolume = 1.0f;
		MusicVolume = MusicVolume * (1.f - 0.5f*DeltaTime) + 0.5f*DeltaTime*DesiredVolume;
		Settings->MusicComp->SetVolumeMultiplier(MusicVolume);
	}
}

void AUTGameState::BeginPlay()
{
	Super::BeginPlay();

	// HACK: temporary hack around config property replication bug; force to be different from defaults and clamp their sizes so they won't break networking

	if (ServerMOTD.Len() > 512) ServerMOTD = ServerMOTD.Left(512);
	if (ServerName.Len() > 512) ServerName = ServerName.Left(512);

	ServerName += TEXT(" ");
	ServerMOTD += TEXT(" ");

	if (GetNetMode() == NM_Client)
	{
		// hook up any TeamInfos that were received prior
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AUTTeamInfo* Team = Cast<AUTTeamInfo>(*It);
			if (Team != NULL)
			{
				Team->ReceivedTeamIndex();
			}
		}
	}
	else
	{
		TArray<UObject*> AllInventory;
		GetObjectsOfClass(AUTInventory::StaticClass(), AllInventory, true, RF_NoFlags);
		for (int32 i = 0; i < AllInventory.Num(); i++)
		{
			if (AllInventory[i]->HasAnyFlags(RF_ClassDefaultObject))
			{
				checkSlow(AllInventory[i]->IsA(AUTInventory::StaticClass()));
				((AUTInventory*)AllInventory[i])->AddOverlayMaterials(this);
			}
		}

		TArray<UObject*> AllEffectVolumes;
		GetObjectsOfClass(AUTPainVolume::StaticClass(), AllEffectVolumes, true, RF_NoFlags);
		for (int32 i = 0; i < AllEffectVolumes.Num(); i++)
		{
			checkSlow(AllEffectVolumes[i]->IsA(AUTPainVolume::StaticClass()));
			((AUTPainVolume*)AllEffectVolumes[i])->AddOverlayMaterials(this);
		}

		//Register any overlays on the ActivatedPlaceholderClass
		AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (GameMode)
		{
			if (GameMode->GetActivatedPowerupPlaceholderClass())
			{
				GameMode->GetActivatedPowerupPlaceholderClass().GetDefaultObject()->AddOverlayMaterials(this);
			}
		}
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		OnlineUserInterface = OnlineSub->GetUserInterface();

		if (OnlineUserInterface.IsValid())
		{
			OnUserInfoCompleteDelegate = FOnQueryUserInfoCompleteDelegate::CreateUObject(this, &AUTGameState::OnQueryUserInfoComplete);
			OnlineUserInterface->AddOnQueryUserInfoCompleteDelegate_Handle(0, OnUserInfoCompleteDelegate);
		}
	}

	bIsAlreadyPendingUserQuery = false;
	AddAllUsersToInfoQuery();
	SpawnDefaultLineUpZones();
}

void AUTGameState::AddUserInfoQuery(TSharedRef<const FUniqueNetId> UserId)
{
	if (OnlineUserInterface.IsValid() && UserId->IsValid())
	{
		TSharedPtr<FOnlineUser> UserInfo = OnlineUserInterface->GetUserInfo(0, *UserId);
		if (!UserInfo.IsValid())
		{
			bIsUserQueryNeeded = true;
			CurrentUsersToQuery.AddUnique(UserId);
		}
	}

	RunAllUserInfoQuery();
}

void AUTGameState::AddAllUsersToInfoQuery()
{
	if (OnlineUserInterface.IsValid())
	{
		for (APlayerState* ConnectedPlayer : PlayerArray)
		{
			if (ConnectedPlayer && ConnectedPlayer->UniqueId.IsValid())
			{
				TSharedPtr<FOnlineUser> UserInfo = OnlineUserInterface->GetUserInfo(0, *ConnectedPlayer->UniqueId);
				if (!UserInfo.IsValid())
				{
					bIsUserQueryNeeded = true;
					CurrentUsersToQuery.AddUnique(ConnectedPlayer->UniqueId->AsShared());
				}
			}
		}
	}

	RunAllUserInfoQuery();
}

void AUTGameState::RunAllUserInfoQuery()
{
	if (bIsUserQueryNeeded && !bIsAlreadyPendingUserQuery )
	{
		bIsAlreadyPendingUserQuery = true;
		bIsUserQueryNeeded = false;

		OnlineUserInterface->QueryUserInfo(0, CurrentUsersToQuery);
	}
}

void AUTGameState::OnQueryUserInfoComplete(int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<const FUniqueNetId> >& UserIds, const FString& ErrorStr)
{
	bIsAlreadyPendingUserQuery = false;

	//remove all successfully processed names from the list
	if (bWasSuccessful)
	{
		for (TSharedRef<const FUniqueNetId> id : UserIds)
		{
			CurrentUsersToQuery.Remove(id);
		}
	}
	
	if (!bWasSuccessful || bIsUserQueryNeeded)
	{
		bIsUserQueryNeeded = true;

		//Schedule a retry of this query if this one failed or if new information has come in since this query was started
		if (!GetWorldTimerManager().IsTimerActive(UserInfoQueryRetryHandle))
		{
			GetWorldTimerManager().SetTimer(UserInfoQueryRetryHandle, this, &AUTGameState::RunAllUserInfoQuery, UserInfoQueryRetryTime, false);
		}
	}
}

FText AUTGameState::GetEpicAccountNameForAccount(TSharedRef<const FUniqueNetId> UserId)
{
	FText ReturnName = FText::GetEmpty();

	if (UserId->IsValid() && OnlineUserInterface.IsValid())
	{
		TSharedPtr<FOnlineUser> UserInfo = OnlineUserInterface->GetUserInfo(0, *UserId);
		if (UserInfo.IsValid())
		{
			ReturnName = FText::FromString(UserInfo->GetDisplayName());
		}
	}

	return ReturnName;
}

float AUTGameState::GetRespawnWaitTimeFor(AUTPlayerState* PS)
{
	return (PS != nullptr) ? FMath::Max(RespawnWaitTime, PS->RespawnWaitTime) : RespawnWaitTime;
}

void AUTGameState::SetRespawnWaitTime(float NewWaitTime)
{
	RespawnWaitTime = NewWaitTime;
}

//By default return nullptr, this should be overriden in other game modes.
TSubclassOf<class AUTInventory> AUTGameState::GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const
{
	return nullptr;
}

//By default return false, this should be overriden in other game modes.
bool AUTGameState::IsSelectedBoostValid(AUTPlayerState* PlayerState) const
{
	return false;
}

float AUTGameState::GetClockTime()
{
	if (IsMatchInOvertime() && bStopGameClock)
	{
			return ElapsedTime-TimeLimit;
	}
	return ((TimeLimit > 0.f) || !HasMatchStarted()) ? RemainingTime : ElapsedTime;
}

float AUTGameState::GetIntermissionTime()
{
	return RemainingTime;
}

void AUTGameState::SetRemainingTime(int32 NewRemainingTime)
{
	RemainingTime = NewRemainingTime;
	ReplicatedRemainingTime = RemainingTime;
	ForceNetUpdate();
}

void AUTGameState::OnRepRemainingTime()
{
	RemainingTime = ReplicatedRemainingTime;
}

void AUTGameState::DefaultTimer()
{
	if (GetWorld()->IsPaused())
	{
		return;
	}
	Super::DefaultTimer();
	if (IsMatchIntermission())
	{
		// no elapsed time - it was incremented in super
		ElapsedTime--;
	}
	else if (IsMatchInProgress())
	{
		for (int32 i = 0; i < PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
			if (PS)
			{
				PS->ElapsedTime++;
			}
		}
	}

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		// might have been deferred while waiting for teams to replicate
		if (TeamSwapSidesOffset != PrevTeamSwapSidesOffset)
		{
			OnTeamSideSwap();
		}
	}

	if ((RemainingTime > 0) && !bStopGameClock && TimeLimit > 0)
	{
		if (IsMatchInProgress())
		{
			RemainingTime--;
			if (GetWorld()->GetNetMode() != NM_Client)
			{
				int32 RepTimeInterval = 10;
				if (RemainingTime % RepTimeInterval == RepTimeInterval - 1)
				{
					ReplicatedRemainingTime = RemainingTime;
					ForceNetUpdate();
				}
				AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
				if (Game && (RemainingTime < 0.8f * TimeLimit))
				{
					Game->bPastELOLimit = true;
				}
			}
		}
		CheckTimerMessage();
	}
	UpdateTimeMessage();
}

void AUTGameState::UpdateTimeMessage()
{
}

void AUTGameState::CheckTimerMessage()
{
	if (GetWorld()->GetNetMode() != NM_DedicatedServer && IsMatchInProgress())
	{
		int32 TimerMessageIndex = -1;
		switch (RemainingTime)
		{
			case 300: TimerMessageIndex = 13; break;		// 5 mins remain
			case 180: TimerMessageIndex = 12; break;		// 3 mins remain
			case 60: TimerMessageIndex = 11; break;		// 1 min remains
			case 30: TimerMessageIndex = 10; break;		// 30 seconds remain
			default:
				if (RemainingTime <= 10)
				{
					TimerMessageIndex = RemainingTime - 1;
				}
				break;
		}

		if (TimerMessageIndex >= 0)
		{
			if ((GetWorld()->GetTimeSeconds() - LastTimerMessageTime > 1.5f) || (LastTimerMessageIndex != TimerMessageIndex))
			{
				LastTimerMessageTime = GetWorld()->GetTimeSeconds();
				LastTimerMessageIndex = TimerMessageIndex;
				for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
				{
					AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
					if (PC != NULL)
					{
						PC->ClientReceiveLocalizedMessage(UUTTimerMessage::StaticClass(), TimerMessageIndex);
					}
				}
			}
		}
	}
}

bool AUTGameState::OnSameTeam(const AActor* Actor1, const AActor* Actor2)
{
	const IUTTeamInterface* TeamInterface1 = Cast<IUTTeamInterface>(Actor1);
	const IUTTeamInterface* TeamInterface2 = Cast<IUTTeamInterface>(Actor2);
	if (TeamInterface1 == NULL || TeamInterface2 == NULL)
	{
		return false;
	}
	else if (TeamInterface1->IsFriendlyToAll() || TeamInterface2->IsFriendlyToAll())
	{
		return true;
	}
	else
	{
		uint8 TeamNum1 = TeamInterface1->GetTeamNum();
		uint8 TeamNum2 = TeamInterface2->GetTeamNum();

		if (TeamNum1 == 255 || TeamNum2 == 255)
		{
			return false;
		}
		else
		{
			return TeamNum1 == TeamNum2;
		}
	}
}

void AUTGameState::ChangeTeamSides(uint8 Offset)
{
	TeamSwapSidesOffset += Offset;
	OnTeamSideSwap();
}

void AUTGameState::OnTeamSideSwap()
{
	if (TeamSwapSidesOffset != PrevTeamSwapSidesOffset && Teams.Num() > 0 && (Role == ROLE_Authority || Teams.Num() == NumTeams))
	{
		uint8 TotalOffset;
		if (TeamSwapSidesOffset < PrevTeamSwapSidesOffset)
		{
			// rollover
			TotalOffset = uint8(uint32(TeamSwapSidesOffset + 255) - uint32(PrevTeamSwapSidesOffset));
		}
		else
		{
			TotalOffset = TeamSwapSidesOffset - PrevTeamSwapSidesOffset;
		}
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			IUTTeamInterface* TeamObj = Cast<IUTTeamInterface>(*It);
			if (TeamObj != NULL)
			{
				uint8 Team = TeamObj->GetTeamNum();
				if (Team != 255)
				{
					TeamObj->Execute_SetTeamForSideSwap(*It, (Team + TotalOffset) % Teams.Num());
				}
			}
			// check for script interface
			else if (It->GetClass()->ImplementsInterface(UUTTeamInterface::StaticClass()))
			{
				// a little hackery to ignore if relevant functions haven't been implemented
				static FName NAME_ScriptGetTeamNum(TEXT("ScriptGetTeamNum"));
				UFunction* GetTeamNumFunc = It->GetClass()->FindFunctionByName(NAME_ScriptGetTeamNum);
				if (GetTeamNumFunc != NULL && GetTeamNumFunc->Script.Num() > 0)
				{
					uint8 Team = IUTTeamInterface::Execute_ScriptGetTeamNum(*It);
					if (Team != 255)
					{
						IUTTeamInterface::Execute_SetTeamForSideSwap(*It, (Team + TotalOffset) % Teams.Num());
					}
				}
				else
				{
					static FName NAME_SetTeamForSideSwap(TEXT("SetTeamForSideSwap"));
					UFunction* SwapFunc = It->GetClass()->FindFunctionByName(NAME_SetTeamForSideSwap);
					if (SwapFunc != NULL && SwapFunc->Script.Num() > 0)
					{
						UE_LOG(UT, Warning, TEXT("Unable to execute SetTeamForSideSwap() for %s because GetTeamNum() must also be implemented"), *It->GetName());
					}
				}
			}
		}
		if (Role == ROLE_Authority)
		{
			// re-initialize all AI squads, in case objectives have changed sides
			for (AUTTeamInfo* Team : Teams)
			{
				Team->ReinitSquads();
			}
		}

		TeamSideSwapDelegate.Broadcast(TotalOffset);
		PrevTeamSwapSidesOffset = TeamSwapSidesOffset;
	}
}

void AUTGameState::SetTimeLimit(int32 NewTimeLimit)
{
	TimeLimit = NewTimeLimit;
	RemainingTime = TimeLimit;
	ReplicatedRemainingTime = TimeLimit;

	ForceNetUpdate();
}

void AUTGameState::SetGoalScore(int32 NewGoalScore)
{
	GoalScore = NewGoalScore;
	ForceNetUpdate();
}

void AUTGameState::SetWinner(AUTPlayerState* NewWinner)
{
	WinnerPlayerState = NewWinner;
	WinningTeam	= NewWinner != NULL ?  NewWinner->Team : 0;
	ForceNetUpdate();
}

/** Returns true if P1 should be sorted before P2.  */
bool AUTGameState::InOrder( AUTPlayerState* P1, AUTPlayerState* P2 )
{
	// spectators are sorted last
    if( P1->bOnlySpectator )
    {
		return P2->bOnlySpectator;
    }
    else if ( P2->bOnlySpectator )
	{
		return true;
	}

	// sort by Score
    if( P1->Score < P2->Score )
	{
		return false;
	}
    if( P1->Score == P2->Score )
    {
		// if score tied, use deaths to sort
		if ( P1->Deaths > P2->Deaths )
			return false;

		// keep local player highest on list
		if ( (P1->Deaths == P2->Deaths) && (Cast<APlayerController>(P2->GetOwner()) != NULL) )
		{
			ULocalPlayer* LP2 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
			if ( LP2 != NULL )
			{
				// make sure ordering is consistent for splitscreen players
				ULocalPlayer* LP1 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
				return ( LP1 != NULL );
			}
		}
	}
    return true;
}

/** Sort the PRI Array based on InOrder() prioritization.  */
void AUTGameState::SortPRIArray()
{
	for (int32 i=0; i<PlayerArray.Num()-1; i++)
	{
		AUTPlayerState* P1 = Cast<AUTPlayerState>(PlayerArray[i]);
		for (int32 j=i+1; j<PlayerArray.Num(); j++)
		{
			AUTPlayerState* P2 = Cast<AUTPlayerState>(PlayerArray[j]);
			if( !InOrder( P1, P2 ) )
			{
				PlayerArray[i] = P2;
				PlayerArray[j] = P1;
				P1 = P2;
			}
		}
	}
}

void AUTGameState::HandleMatchHasStarted()
{
	//Log user activity so Crash Reporter knows if we crash during Match
	FUserActivityTracking::SetActivity(FUserActivity(TEXT("MatchStarted")));

	if (GetNetMode() == NM_Client || GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone)
	{
		bRunFPSChart = true;
	}

	StartFPSCharts();

	Super::HandleMatchHasStarted();

	AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	if (WS != nullptr)
	{
		IUTResetInterface::Execute_Reset(WS);
	}
}

void AUTGameState::HandleMatchHasEnded()
{
	//Log user activity so Crash Reporter knows if we crash during Match
	FUserActivityTracking::SetActivity(FUserActivity(TEXT("MatchEnded")));

	MatchEndTime = GetWorld()->TimeSeconds;

	StopFPSCharts();

	Super::HandleMatchHasEnded();
}

void AUTGameState::StartFPSCharts()
{
	for (auto It = GetGameInstance()->GetLocalPlayerIterator(); It; ++It)
	{
		UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(*It);
		if (LocalPlayer)
		{
			if (LocalPlayer->IsReplay())
			{
				return;
			}
		}
	}

	OnHitchDetectedHandle = GEngine->OnHitchDetectedDelegate.AddUObject(this, &AUTGameState::OnHitchDetected);

	if (bRunFPSChart)
	{
		FString FPSChartLabel;
		if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone)
		{
			FPSChartLabel = TEXT("UTAnalyticsFPSCharts");
		}
		else
		{
			FPSChartLabel = TEXT("UTServerFPSChart");
		}

		if (!GameplayFPSChart.IsValid())
		{
			GameplayFPSChart = MakeShareable(new FPerformanceTrackingChart(FDateTime::Now(),FPSChartLabel));
			GEngine->AddPerformanceDataConsumer(GameplayFPSChart);
		}
	}
}

void AUTGameState::StopFPSCharts()
{
	for (auto It = GetGameInstance()->GetLocalPlayerIterator(); It; ++It)
	{
		UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(*It);
		if (LocalPlayer)
		{
			if (LocalPlayer->IsReplay())
			{
				return;
			}
		}
	}

	if (bRunFPSChart && GameplayFPSChart.IsValid())
	{
		TArray<FAnalyticsEventAttribute> ParamArray;
		FString MapName;

		if (GetLevel() && GetLevel()->OwningWorld)
		{
			MapName = GetLevel()->OwningWorld->GetMapName();
		}

		const bool bIsClient = (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone);

		GameplayFPSChart->DumpChartToAnalyticsParams(MapName, ParamArray, bIsClient);

		if (bIsClient)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController());
			if (UTPC)
			{
				if (ParamArray.Num() > 0)
				{
					int64 MaxRequiredTextureSize = 0;
					if (FPlatformProperties::SupportsTextureStreaming() && IStreamingManager::Get().IsTextureStreamingEnabled())
					{
						MaxRequiredTextureSize = IStreamingManager::Get().GetTextureStreamingManager().GetMaxEverRequired();
					}
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("MaxRequiredTextureSize"), MaxRequiredTextureSize));

					// Fire off the analytics event
					FUTAnalytics::FireEvent_UTFPSCharts(UTPC, ParamArray);
				}
				else
				{
					UE_LOG(UT, Error, TEXT("UT FPS Charts ParamArray is empty"));
				}
			}
		}
		else
		{
			AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
			if (UTGM)
			{
				FUTAnalytics::FireEvent_UTServerFPSCharts(UTGM, ParamArray);
			}
		}

		GEngine->RemovePerformanceDataConsumer(GameplayFPSChart);
		GameplayFPSChart.Reset();
		GameplayFPSChart = nullptr;

		GEngine->OnHitchDetectedDelegate.Remove(OnHitchDetectedHandle);
		bRunFPSChart = false;
	}
}

void AUTGameState::OnHitchDetected(float DurationInSeconds)
{
	const float DurationInMs = DurationInSeconds * 1000.0f;
	if (IsRunningDedicatedServer())
	{
		if (DurationInMs >= UnplayableHitchThresholdInMs)
		{
			++UnplayableHitchesDetected;
			UnplayableTimeInMs += DurationInMs;

			if (UnplayableHitchesDetected > MaxUnplayableHitchesToTolerate)
			{
				// send an analytics event
				AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
				if (UTGM)
				{
					FUTAnalytics::FireEvent_ServerUnplayableCondition(UTGM, UnplayableHitchThresholdInMs, UnplayableHitchesDetected, UnplayableTimeInMs);
				}

				// reset the counter, in case unplayable condition is going to be resolved by outside tools
				UnplayableHitchesDetected = 0;
				UnplayableTimeInMs = 0.0;
			}
		}
	}
}

bool AUTGameState::IsMatchInCountdown() const
{
	return GetMatchState() == MatchState::CountdownToBegin;
}

bool AUTGameState::HasMatchStarted() const
{
	return Super::HasMatchStarted() && GetMatchState() != MatchState::CountdownToBegin && GetMatchState() != MatchState::PlayerIntro;
}

bool AUTGameState::IsMatchInProgress() const
{
	FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::InProgress || CurrentMatchState == MatchState::MatchIsInOvertime);
}

bool AUTGameState::IsMatchInOvertime() const
{
	FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::MatchEnteringOvertime || CurrentMatchState == MatchState::MatchIsInOvertime);
}

bool AUTGameState::IsMatchIntermission() const
{
	return GetMatchState() == MatchState::MatchIntermission;
}

void AUTGameState::OnWinnerReceived()
{
}

FName AUTGameState::OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle)
{
	if (IsLineUpActive())
	{
		return FName(TEXT("LineUpCam"));
	}
	else if (HasMatchEnded())
	{
		return FName(TEXT("FreeCam"));
	}
	// FIXME: shouldn't this come from the Pawn?
	else if (Cast<AUTRemoteRedeemer>(PCOwner->GetPawn()) != nullptr)
	{
		return FName(TEXT("FirstPerson"));
	}
	else
	{
		return CurrentCameraStyle;
	}
}

FText AUTGameState::ServerRules()
{
	if (GameModeClass != NULL && GameModeClass->IsChildOf(AUTGameMode::StaticClass()))
	{
		return GameModeClass->GetDefaultObject<AUTGameMode>()->BuildServerRules(this);
	}
	else
	{
		return FText();
	}
}

void AUTGameState::TrackGame()
{
	if (!GIsEditor && GameModeClass != nullptr)
	{
		AUTGameMode* DefaultGame = Cast<AUTGameMode>(GameModeClass.GetDefaultObject());
		if (DefaultGame != nullptr)
		{
			// If this is a blueprint class, get the parent class.

			if (DefaultGame->IsInBlueprint())
			{
				TSubclassOf<AUTGameMode> ParentClass = GameModeClass->GetSuperClass();
				if (ParentClass != nullptr)
				{
					AUTGameMode* ParentGame = Cast<AUTGameMode>(ParentClass.GetDefaultObject());
					if (ParentGame != nullptr)
					{
						DefaultGame = ParentGame;
					}
				}
			}

			for (auto It = GetGameInstance()->GetLocalPlayerIterator(); It; ++It)
			{
				UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(*It);
				if (UTLocalPlayer)
				{
					UTLocalPlayer->TrackGamePlayed(DefaultGame->GetClass()->GetPathName());
					break;
				}
			}
		}
	}
}


void AUTGameState::ReceivedGameModeClass()
{
	Super::ReceivedGameModeClass();

	TSubclassOf<AUTGameMode> UTGameClass(*GameModeClass);
	if (UTGameClass != NULL)
	{
		// precache announcements
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->PlayerController);
			if (UTPC != NULL && UTPC->Announcer != NULL)
			{
				UTGameClass.GetDefaultObject()->PrecacheAnnouncements(UTPC->Announcer);
			}
		}
	}

	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		TrackGame();	
	}

	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &AUTGameState::StartRecordingReplay, 0.5f, false);
}

void AUTGameState::StartRecordingReplay()
{
	TSubclassOf<AUTGameMode> UTGameClass(*GameModeClass);
	bool bGameModeSupportsInstantReplay = UTGameClass ? UTGameClass.GetDefaultObject()->SupportsInstantReplay() : false;
		
	UWorld* const World = GetWorld();
	UGameInstance* const GameInstance = GetGameInstance();

	// Don't record for killcam if this world is already playing back a replay.
	const UDemoNetDriver* const DemoDriver = World ? World->DemoNetDriver : nullptr;
	const bool bIsPlayingReplay = DemoDriver ? DemoDriver->IsPlaying() : false;
	if (!bIsPlayingReplay && GameInstance != nullptr && World->GetNetMode() == NM_Client && CVarUTEnableInstantReplay->GetInt() == 1 && bGameModeSupportsInstantReplay)
	{
		// Since the killcam world will also have ReceivedGameModeClass() called in it, detect that and
		// don't try to start recording again. Killcam world contexts will have a valid PIEInstance for now.
		// Revisit when killcam is supported in PIE.
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(World);
		if (Context == nullptr || Context->PIEInstance != INDEX_NONE || Context->WorldType == EWorldType::PIE)
		{
			return;
		}

		const TCHAR* KillcamReplayName = TEXT("_DeathCam");

		// Start recording the replay for killcam, always using the in memory streamer.
		TArray<FString> AdditionalOptions;
		AdditionalOptions.Add(TEXT("ReplayStreamerOverride=InMemoryNetworkReplayStreaming"));
		GameInstance->StartRecordingReplay(KillcamReplayName, KillcamReplayName, AdditionalOptions);

		// Start playback for each local player. Since we're using the in-memory streamer, the replay will
		// be available immediately.
		for (auto It = GameInstance->GetLocalPlayerIterator(); It; ++It)
		{
			UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(*It);
			if (LocalPlayer != nullptr && LocalPlayer->GetKillcamPlaybackManager() != nullptr)
			{
				if (LocalPlayer->GetKillcamPlaybackManager()->GetKillcamWorld() != World)
				{
					LocalPlayer->GetKillcamPlaybackManager()->CreateKillcamWorld(World->URL, *Context);
					LocalPlayer->GetKillcamPlaybackManager()->PlayKillcamReplay(KillcamReplayName);
				}
			}
		}
	}
}

FLinearColor AUTGameState::GetGameStatusColor()
{
	return FLinearColor::White;
}

FText AUTGameState::GetGameStatusText(bool bForScoreboard)
{
	if (!IsMatchInProgress())
	{
		if (HasMatchEnded())
		{
			return GameOverStatus;
		}
		else if (GetMatchState() == MatchState::MapVoteHappening)
		{
			return MapVoteStatus;
		}
		else if (GetMatchState() == MatchState::WaitingTravel)
		{
			return NSLOCTEXT("UTGameState","WaitingTravel","Waiting For Server"); 
		}
		else if ((GetNetMode() != NM_Standalone) && bHaveMatchHost)
		{
			return HostStatus;
		}
		else if ((PlayersNeeded > 0) && (GetNetMode() != NM_Standalone))
		{
			return NeedPlayersStatus;
		}
		else
		{
			return PreGameStatus;
		}
	}
	else if (IsMatchInOvertime())
	{
		return OvertimeStatus;
	}

	return FText::GetEmpty();
}

void AUTGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (It->PlayerController != NULL)
		{
			AUTHUD* Hud = Cast<AUTHUD>(It->PlayerController->MyHUD);
			if (Hud != NULL)
			{
				Hud->NotifyMatchStateChange();
			}
		}
	}
	if ((Role < ROLE_Authority) && (GetMatchState() == MatchState::PlayerIntro))
	{
		// destroy torn off pawns
		TArray<APawn*> PawnsToDestroy;
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			if (It->IsValid() && !Cast<ASpectatorPawn>((*It).Get()))
			{
				PawnsToDestroy.Add(It->Get());
			}
		}

		for (int32 i = 0; i<PawnsToDestroy.Num(); i++)
		{
			APawn* Pawn = PawnsToDestroy[i];
			if (Pawn != NULL && !Pawn->IsPendingKill() && Pawn->bTearOff)
			{
				Pawn->Destroy();
			}
		}
	}
}

// By default, do nothing.  
void AUTGameState::OnRep_ServerName()
{
}

// By default, do nothing.  
void AUTGameState::OnRep_ServerMOTD()
{
}

void AUTGameState::AddPlayerState(APlayerState* PlayerState)
{
	// assign spectating ID to this player
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	// NOTE: in the case of a rejoining player, this function gets called for both the original and new PLayerStates
	//		we will migrate the initially selected ID to avoid unnecessary ID shuffling
	//		and thus need to check here if that has happened and avoid assigning a new one
	if (PS != NULL && !PS->bOnlySpectator && PS->SpectatingID == 0)
	{
		TArray<APlayerState*> PlayerArrayCopy = PlayerArray;
		PlayerArrayCopy.Sort([](const APlayerState& A, const APlayerState& B) -> bool
		{
			if (Cast<AUTPlayerState>(&A) == NULL)
			{
				return false;
			}
			else if (Cast<AUTPlayerState>(&B) == NULL)
			{
				return true;
			}
			else
			{
				return ((AUTPlayerState*)&A)->SpectatingID < ((AUTPlayerState*)&B)->SpectatingID;
			}
		});
		// find first gap in assignments from player leaving, give it to this player
		// if none found, assign PlayerArray.Num() + 1
		bool bFound = false;
		for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
		{
			AUTPlayerState* OtherPS = Cast<AUTPlayerState>(PlayerArrayCopy[i]);
			if (OtherPS == NULL || OtherPS->bOnlySpectator || OtherPS->SpectatingID != uint8(i + 1))
			{
				PS->SpectatingID = uint8(i + 1);
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			PS->SpectatingID = uint8(PlayerArrayCopy.Num() + 1);
		}
	}

	Super::AddPlayerState(PlayerState);
}

void AUTGameState::CompactSpectatingIDs()
{
	if (Role == ROLE_Authority)
	{
		// get sorted list of UTPlayerStates that have been assigned an ID
		TArray<AUTPlayerState*> PlayerArrayCopy;
		for (APlayerState* PS : PlayerArray)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (UTPS != NULL && UTPS->SpectatingID > 0 && !UTPS->bOnlySpectator)
			{
				PlayerArrayCopy.Add(UTPS);
			}
		}
		PlayerArrayCopy.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
		{
			return A.SpectatingID < B.SpectatingID;
		});

		// fill in gaps from IDs at the end of the list
		for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
		{
			if (PlayerArrayCopy[i]->SpectatingID != uint8(i + 1))
			{
				AUTPlayerState* MovedPS = PlayerArrayCopy.Pop(false);
				MovedPS->SpectatingID = uint8(i + 1);
				PlayerArrayCopy.Insert(MovedPS, i);
			}
		}

		// now do the same for SpectatingIDTeam
		for (AUTTeamInfo* Team : Teams)
		{
			PlayerArrayCopy.Reset();
			const TArray<AController*> Members = Team->GetTeamMembers();
			for (AController* C : Members)
			{
				if (C)
				{
					AUTPlayerState* UTPS = Cast<AUTPlayerState>(C->PlayerState);
					if (UTPS != NULL && UTPS->SpectatingIDTeam)
					{
						PlayerArrayCopy.Add(UTPS);
					}
				}
			}
			PlayerArrayCopy.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
			{
				return A.SpectatingIDTeam < B.SpectatingIDTeam;
			});

			for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
			{
				if (PlayerArrayCopy[i]->SpectatingIDTeam != uint8(i + 1))
				{
					AUTPlayerState* MovedPS = PlayerArrayCopy.Pop(false);
					MovedPS->SpectatingIDTeam = uint8(i + 1);
					PlayerArrayCopy.Insert(MovedPS, i);
				}
			}
		}
	}
}

int32 AUTGameState::GetMaxSpectatingId()
{
	int32 MaxSpectatingID = 0;
	for (int32 i = 0; i<PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS && (PS->SpectatingID > MaxSpectatingID))
		{
			MaxSpectatingID = PS->SpectatingID;
		}
	}
	return MaxSpectatingID;
}

int32 AUTGameState::GetMaxTeamSpectatingId(int32 TeamNum)
{
	int32 MaxSpectatingID = 0;
	for (int32 i = 0; i<PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS && (PS->GetTeamNum() == TeamNum) && (PS->SpectatingIDTeam > MaxSpectatingID))
		{
			MaxSpectatingID = PS->SpectatingIDTeam;
		}
	}
	return MaxSpectatingID;

}

bool AUTGameState::IsTempBanned(const FUniqueNetIdRepl& UniqueId)
{
	for (int32 i=0; i< TempBans.Num(); i++)
	{
		if (*TempBans[i] == *UniqueId)
		{
			return true;
		}
	}
	return false;
}

bool AUTGameState::VoteForTempBan(AUTPlayerState* BadGuy, AUTPlayerState* Voter)
{
	bool bResult = false;
	AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();

	if (Game && Game->NumPlayers > 0)
	{
		// Quick out.
		if (bDisableVoteKick || (bOnlyTeamCanVoteKick && !OnSameTeam(BadGuy, Voter)))
		{
			return false;
		}
		
		bResult = BadGuy->LogBanRequest(Voter);
		Game->BroadcastLocalized(Voter, UUTGameMessage::StaticClass(), 13, Voter, BadGuy);

		int32 NumPlayers = 0;
		for (int32 i = 0; i <PlayerArray.Num(); i++)
		{
			if (!PlayerArray[i]->bIsSpectator && !PlayerArray[i]->bOnlySpectator && !PlayerArray[i]->bIsABot)
			{
				if (!bOnlyTeamCanVoteKick || OnSameTeam(BadGuy, PlayerArray[i]))
				{
					NumPlayers += 1.0f;
				}
			}
		}
		float Perc = (float(BadGuy->CountBanVotes()) / float(NumPlayers)) * 100.0f;
		BadGuy->KickCount = BadGuy->CountBanVotes();
		UE_LOG(UT,Log,TEXT("[KICK VOTE] Target = %s - # of Votes = %i (%i players), % = %f"), * BadGuy->PlayerName, BadGuy->KickCount, NumPlayers, Perc);

		if ( Perc >=  KickThreshold )
		{
			UE_LOG(UT,Log,TEXT("[KICK VOTE]     KICKED!!!!"));
			AUTPlayerController* PC = Cast<AUTPlayerController>(BadGuy->GetOwner());
			if (PC)
			{
				AUTGameSession* GS = Cast<AUTGameSession>(Game->GameSession);
				if (GS)
				{
					GS->KickPlayer(PC,NSLOCTEXT("UTGameState","TempKickBan","The players on this server have decided you need to leave"));
					TempBans.Add(BadGuy->UniqueId.GetUniqueNetId());				
				}
			}
		}
	}

	return bResult;
}

void AUTGameState::GetAvailableGameData(TArray<UClass*>& GameModes, TArray<UClass*>& MutatorList)
{
	UE_LOG(UTLoading, Log, TEXT("Iterate through UClasses"));
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* CurrentClass = (*It);
		// non-native classes are detected by asset search even if they're loaded for consistency
		if (!CurrentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_HideDropDown) && CurrentClass->HasAnyClassFlags(CLASS_Native))
		{
			if (CurrentClass->IsChildOf(AUTGameMode::StaticClass()))
			{
				if (!CurrentClass->GetDefaultObject<AUTGameMode>()->bHideInUI)
				{
					GameModes.Add(CurrentClass);
				}
			}
			else if (CurrentClass->IsChildOf(AUTMutator::StaticClass()) && !CurrentClass->GetDefaultObject<AUTMutator>()->DisplayName.IsEmpty())
			{
				MutatorList.Add(CurrentClass);
			}
		}
	}

	{
		UE_LOG(UTLoading, Log, TEXT("Load Gamemode blueprints"));
		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTGameMode::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if ((ClassPath != NULL) && !ClassPath->IsEmpty())
			{
				UE_LOG(UTLoading, Log, TEXT("load gamemode object classpath %s"), **ClassPath);
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTGameMode::StaticClass()) && !TestClass->GetDefaultObject<AUTGameMode>()->bHideInUI)
				{
					GameModes.AddUnique(TestClass);
				}
			}
		}
	}

	{
		UE_LOG(UTLoading, Log, TEXT("Load Mutator blueprints"));
		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTMutator::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if ((ClassPath != NULL) && !ClassPath->IsEmpty())
			{
				UE_LOG(UTLoading, Log, TEXT("load mutator object classpath %s"), **ClassPath);
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTMutator::StaticClass()) && !TestClass->GetDefaultObject<AUTMutator>()->DisplayName.IsEmpty())
				{
					MutatorList.AddUnique(TestClass);
				}
			}
		}
	}
}

/**
 *	returns a list of MapAssets give a constrained set of map prefixes.
 **/
void AUTGameState::ScanForMaps(const TArray<FString>& AllowedMapPrefixes, TArray<FAssetData>& MapList)
{
	TArray<FAssetData> MapAssets;
	GetAllAssetData(UWorld::StaticClass(), MapAssets, false);
	for (const FAssetData& Asset : MapAssets)
	{
		FString MapPackageName = Asset.PackageName.ToString();
		// ignore /Engine/ as those aren't real gameplay maps and make sure expected file is really there
		if ( !MapPackageName.StartsWith(TEXT("/Engine/")) && IFileManager::Get().FileSize(*FPackageName::LongPackageNameToFilename(MapPackageName, FPackageName::GetMapPackageExtension())) > 0 )
		{
			// Look to see if this is allowed.
			bool bMapIsAllowed = AllowedMapPrefixes.Num() == 0;
			for (int32 i=0; i<AllowedMapPrefixes.Num();i++)
			{
				if ( Asset.AssetName.ToString().StartsWith(AllowedMapPrefixes[i] + TEXT("-")) )
				{
					bMapIsAllowed = true;
					break;
				}
			}

			if (bMapIsAllowed)
			{
				MapList.Add(Asset);
			}
		}
	}
}

/**
 *	Creates a replicated map info that represents the data regarding a map.
 **/
AUTReplicatedMapInfo* AUTGameState::CreateMapInfo(const FAssetData& MapAsset)
{
	const FString* Title = MapAsset.TagsAndValues.Find(NAME_MapInfo_Title); 
	const FString* Author = MapAsset.TagsAndValues.Find(NAME_MapInfo_Author);
	const FString* Description = MapAsset.TagsAndValues.Find(NAME_MapInfo_Description);
	const FString* Screenshot = MapAsset.TagsAndValues.Find(NAME_MapInfo_ScreenshotReference);

	const FString* OptimalPlayerCountStr = MapAsset.TagsAndValues.Find(NAME_MapInfo_OptimalPlayerCount);
	int32 OptimalPlayerCount = 6;
	if (OptimalPlayerCountStr != NULL)
	{
		OptimalPlayerCount = FCString::Atoi(**OptimalPlayerCountStr);
	}

	const FString* OptimalTeamPlayerCountStr = MapAsset.TagsAndValues.Find(NAME_MapInfo_OptimalTeamPlayerCount);
	int32 OptimalTeamPlayerCount = 10;
	if (OptimalTeamPlayerCountStr != NULL)
	{
		OptimalTeamPlayerCount = FCString::Atoi(**OptimalTeamPlayerCountStr);
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	AUTReplicatedMapInfo* MapInfo = GetWorld()->SpawnActor<AUTReplicatedMapInfo>(Params);
	if (MapInfo)
	{

		FText LocDesc = FText::GetEmpty();
		if (Description != nullptr)
		{
			FTextStringHelper::ReadFromString(**Description, LocDesc);
		}

		MapInfo->MapPackageName = MapAsset.PackageName.ToString();
		MapInfo->MapAssetName = MapAsset.AssetName.ToString();
		MapInfo->Title = (Title != NULL && !Title->IsEmpty()) ? *Title : *MapAsset.AssetName.ToString();
		MapInfo->Author = (Author != NULL) ? *Author : FString();
		MapInfo->Description = (!LocDesc.IsEmpty()) ? LocDesc.ToString() : FString();
		MapInfo->MapScreenshotReference = (Screenshot != NULL) ? *Screenshot : FString();
		MapInfo->OptimalPlayerCount = OptimalPlayerCount;
		MapInfo->OptimalTeamPlayerCount = OptimalTeamPlayerCount;

		if (Role == ROLE_Authority)
		{
			// Look up it's redirect information if it has any.
			AUTBaseGameMode* BaseGameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
			if (BaseGameMode)
			{
				BaseGameMode->CheckMapStatus(MapInfo->MapPackageName, MapInfo->bIsEpicMap, MapInfo->bIsMeshedMap, MapInfo->bHasRights);
				BaseGameMode->FindRedirect(MapInfo->MapPackageName, MapInfo->Redirect);
			}
		}
	}

	return MapInfo;

}

void AUTGameState::CreateMapVoteInfo(const FString& MapPackage,const FString& MapTitle, const FString& MapScreenshotReference)
{

	//Make sure we don't already have a map info for this package

	for (int32 i=0; i < MapVoteList.Num(); i++)
	{
		if (MapVoteList[i]->MapPackageName.Equals(MapPackage, ESearchCase::IgnoreCase) && !MapVoteList[i]->IsPendingKillPending())
		{
			// Already exists, just quick out.
			return;
		}
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	AUTReplicatedMapInfo* MapVoteInfo = GetWorld()->SpawnActor<AUTReplicatedMapInfo>(Params);
	if (MapVoteInfo)
	{
		MapVoteInfo->MapPackageName= MapPackage;
		MapVoteInfo->Title = MapTitle;
		MapVoteInfo->MapScreenshotReference = MapScreenshotReference;

		if (Role == ROLE_Authority)
		{
			// Look up it's redirect information if it has any.

			AUTBaseGameMode* BaseGameMode = Cast<AUTBaseGameMode>(GetWorld()->GetAuthGameMode());
			if (BaseGameMode)
			{
				BaseGameMode->CheckMapStatus(MapVoteInfo->MapPackageName, MapVoteInfo->bIsEpicMap, MapVoteInfo->bIsMeshedMap, MapVoteInfo->bHasRights);
			}
		}

		MapVoteList.Add(MapVoteInfo);
	}
}

bool AUTGameState::GetImportantPickups_Implementation(TArray<AUTPickup*>& PickupList)
{
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AUTPickup* Pickup = Cast<AUTPickup>(*It);
		AUTPickupInventory* PickupInventory = Cast<AUTPickupInventory>(*It);

		if ((PickupInventory && PickupInventory->GetInventoryType() && PickupInventory->GetInventoryType()->GetDefaultObject<AUTInventory>()->bShowPowerupTimer
			&& ((PickupInventory->GetInventoryType()->GetDefaultObject<AUTInventory>()->HUDIcon.Texture != NULL) || PickupInventory->GetInventoryType()->IsChildOf(AUTArmor::StaticClass())))
			|| (Pickup && Pickup->bDelayedSpawn && Pickup->RespawnTime >= 0.0f && Pickup->HUDIcon.Texture != nullptr))
		{
			if (!Pickup->bOverride_TeamSide)
			{
				Pickup->TeamSide = NearestTeamSide(Pickup);
			}
			PickupList.Add(Pickup);
		}
	}

	//Sort the list by by respawn time 
	//TODO: powerup priority so different armors sort properly
	PickupList.Sort([](const AUTPickup& A, const AUTPickup& B) -> bool
	{
		return A.RespawnTime > B.RespawnTime;
	});

	return true;
}

void AUTGameState::OnRep_ServerSessionId()
{
	UUTGameEngine* GameEngine = Cast<UUTGameEngine>(GEngine);
	if (GameEngine && ServerSessionId != TEXT(""))
	{
		for(auto It = GameEngine->GetLocalPlayerIterator(GetWorld()); It; ++It)
		{
			UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(*It);
			if (LocalPlayer)
			{
				LocalPlayer->VerifyGameSession(ServerSessionId);
			}
		}
	}
}

float AUTGameState::GetStatsValue(FName StatsName)
{
	return StatsData.FindRef(StatsName);
}

void AUTGameState::SetStatsValue(FName StatsName, float NewValue)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	StatsData.Add(StatsName, NewValue);
}

void AUTGameState::ModifyStatsValue(FName StatsName, float Change)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	float CurrentValue = StatsData.FindRef(StatsName);
	StatsData.Add(StatsName, CurrentValue + Change);
}

bool AUTGameState::AreAllPlayersReady()
{
	if (!HasMatchStarted())
	{
		for (int32 i = 0; i < PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
			if (PS != NULL && !PS->bOnlySpectator && !PS->bIsWarmingUp && !PS->bIsInactive && !PS->bIsABot)
			{
				return false;
			}
		}
	}
	return true;
}

bool AUTGameState::IsAllowedSpawnPoint_Implementation(AUTPlayerState* Chooser, APlayerStart* DesiredStart) const
{
	return true;
}

bool AUTGameState::PreventWeaponFire()
{
	return (IsMatchIntermission() || HasMatchEnded() || IsLineUpActive());
}

void AUTGameState::ClearHighlights()
{
	for (int32 i = 0; i < PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS)
		{
			for (int32 j = 0; j < 5; j++)
			{
				PS->MatchHighlights[j] = NAME_None;
			}
		}
	}
}

void AUTGameState::UpdateMatchHighlights()
{
	ClearHighlights();
	UpdateHighlights();
}

void AUTGameState::UpdateRoundHighlights()
{
	ClearHighlights();
	UpdateHighlights();
}

void AUTGameState::UpdateHighlights_Implementation()
{
	// add highlights to each player in order of highlight priority, filling to 5 if possible
	AUTPlayerState* TopScorer[2] = { NULL, NULL };
	AUTPlayerState* MostKills = NULL;
	AUTPlayerState* LeastDeaths = NULL;
	AUTPlayerState* BestKDPS = NULL;
	AUTPlayerState* BestComboPS = NULL;
	AUTPlayerState* MostHeadShotsPS = NULL;
	AUTPlayerState* MostAirRoxPS = NULL;

	//Collect all the weapons
	TArray<AUTWeapon *> StatsWeapons;
	if (StatsWeapons.Num() == 0)
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && Pickup->GetInventoryType())
			{
				StatsWeapons.AddUnique(Pickup->GetInventoryType()->GetDefaultObject<AUTWeapon>());
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;

			if (PS->Score > (TopScorer[TeamIndex] ? TopScorer[TeamIndex]->Score : 0))
			{
				TopScorer[TeamIndex] = PS;
			}
			if (PS->Kills > (MostKills ? MostKills->Kills : 0))
			{
				MostKills = PS;
			}
			if (!LeastDeaths || (PS->Deaths < LeastDeaths->Deaths))
			{
				LeastDeaths = PS;
			}
			if (PS->Kills > 0)
			{
				if (!BestKDPS)
				{
					BestKDPS = PS;
				}
				else if (PS->Deaths == 0)
				{
					if ((BestKDPS->Deaths > 0) || (PS->Kills > BestKDPS->Kills))
					{
						BestKDPS = PS;
					}
				}
				else if ((BestKDPS->Deaths > 0) && (PS->Kills / PS->Deaths > BestKDPS->Kills / BestKDPS->Deaths))
				{
					BestKDPS = PS;
				}
			}

			//Figure out what weapon killed the most
			PS->FavoriteWeapon = nullptr;
			int32 BestKills = 0;
			for (AUTWeapon* Weapon : StatsWeapons)
			{
				int32 Kills = Weapon->GetWeaponKillStats(PS);
				if (Kills > BestKills)
				{
					BestKills = Kills;
					PS->FavoriteWeapon = Weapon->GetClass();
				}
			}

			if (PS->GetStatsValue(NAME_BestShockCombo) > (BestComboPS ? BestComboPS->GetStatsValue(NAME_BestShockCombo) : 0.f))
			{
				BestComboPS = PS;
			}
			if (PS->GetStatsValue(NAME_SniperHeadshotKills) > (MostHeadShotsPS ? MostHeadShotsPS->GetStatsValue(NAME_SniperHeadshotKills) : 2.f))
			{
				MostHeadShotsPS = PS;
			}
			if (PS->GetStatsValue(NAME_AirRox) > (MostAirRoxPS ? MostAirRoxPS->GetStatsValue(NAME_AirRox) : 2.f))
			{
				MostAirRoxPS = PS;
			}
		}
	}

	if (BestComboPS)
	{
		BestComboPS->AddMatchHighlight(HighlightNames::BestCombo, BestComboPS->GetStatsValue(NAME_BestShockCombo));
	}
	if (BestKDPS)
	{
		BestKDPS->AddMatchHighlight(HighlightNames::BestKD, (BestKDPS->Deaths > 0) ? float(BestKDPS->Kills) / float(BestKDPS->Deaths) : BestKDPS->Kills);
	}

	float TopScoreRed = TopScorer[0] ? TopScorer[0]->Score : 1;
	float TopScoreBlue = TopScorer[1] ? TopScorer[1]->Score : 1;
	float TopScoreOverall = FMath::Max(TopScoreRed, TopScoreBlue);
	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if (Game && (Game->NumPlayers + Game->NumBots) >= 3)
			{
				// don't show top scorer highlight if 2 or fewer players
				if (PS->Score >= TopScoreOverall)
				{
					PS->AddMatchHighlight(HighlightNames::TopScorer, int32(PS->Score));
				}
				else if (PS->Team)
				{
					int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
					if (PS->Score >= ((PS->Team->TeamIndex == 0) ? TopScoreRed : TopScoreBlue))
					{
						FName TopScoreHighlightName = (TeamIndex == 0) ? HighlightNames::TopScorerRed : HighlightNames::TopScorerBlue;
						PS->AddMatchHighlight(TopScoreHighlightName, int32(PS->Score));
					}
				}
			}
			if (MostKills && (PS->Kills == MostKills->Kills))
			{
				PS->AddMatchHighlight(HighlightNames::MostKills, MostKills->Kills);
			}
			if (MostHeadShotsPS && (PS->GetStatsValue(NAME_SniperHeadshotKills) == MostHeadShotsPS->GetStatsValue(NAME_SniperHeadshotKills)))
			{
				PS->AddMatchHighlight(HighlightNames::MostHeadShots, MostHeadShotsPS->GetStatsValue(NAME_SniperHeadshotKills));
			}
			if (MostAirRoxPS && (PS->GetStatsValue(NAME_AirRox) == MostAirRoxPS->GetStatsValue(NAME_AirRox)))
			{
				PS->AddMatchHighlight(HighlightNames::MostAirRockets, MostAirRoxPS->GetStatsValue(NAME_AirRox));
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS  && !PS->bOnlySpectator)
		{
			// only add low priority highlights if not enough high priority highlights
			AddMinorHighlights(PS);

			if (LeastDeaths && (PS->Deaths == LeastDeaths->Deaths))
			{
				PS->AddMatchHighlight(HighlightNames::LeastDeaths, LeastDeaths->Deaths);
			}

			int32 FirstIndex = 5;
			for (int32 i = 0; i < 5; i++)
			{
				if (PS->MatchHighlights[i] == NAME_None)
				{
					FirstIndex = i;
					break;
				}
			}
			if (FirstIndex < 5)
			{
				if (PS->Kills > 0)
				{
					PS->MatchHighlights[FirstIndex] = HighlightNames::KillingBlowsAward;
					PS->MatchHighlightData[FirstIndex] = PS->Kills;
					FirstIndex++;
				}
			}
			if (FirstIndex < 5)
			{
				if (PS->KillAssists > 0)
				{
					PS->MatchHighlights[FirstIndex] = HighlightNames::KillsAward;
					PS->MatchHighlightData[FirstIndex] = PS->Kills + PS->KillAssists;
					FirstIndex++;
				}
			}
			if (FirstIndex < 5)
			{
				if (PS->DamageDone > 0)
				{
					PS->MatchHighlights[FirstIndex + 1] = HighlightNames::DamageAward;
					PS->MatchHighlightData[FirstIndex + 1] = PS->DamageDone;
					FirstIndex++;
				}
			}
			if (FirstIndex < 2)
			{
				PS->MatchHighlights[FirstIndex] = HighlightNames::ParticipationAward;
			}
		}
	}
}

int32 AUTGameState::NumHighlightsNeeded()
{
	return 4;
}

void AUTGameState::AddMinorHighlights_Implementation(AUTPlayerState* PS)
{
	// skip if already filled with major highlights
	if (PS->MatchHighlights[NumHighlightsNeeded()] != NAME_None)
	{
		return;
	}

	// sprees and multikills
	FName SpreeStatsNames[5] = { NAME_SpreeKillLevel4, NAME_SpreeKillLevel3, NAME_SpreeKillLevel2, NAME_SpreeKillLevel1, NAME_SpreeKillLevel0 };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetStatsValue(SpreeStatsNames[i]) > 0)
		{
			PS->AddMatchHighlight(SpreeStatsNames[i], PS->GetStatsValue(SpreeStatsNames[i]));
			break;
		}
	}

	// Most kills with favorite weapon, if needed
	if (PS->FavoriteWeapon)
	{
		AUTWeapon* DefaultWeapon = PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>();
		int32 WeaponKills = DefaultWeapon->GetWeaponKillStats(PS);
		if (WeaponKills > 2)
		{
			bool bIsBestOverall = true;
			for (int32 i = 0; i < PlayerArray.Num(); i++)
			{
				AUTPlayerState* OtherPS = Cast<AUTPlayerState>(PlayerArray[i]);
				if (OtherPS && (PS != OtherPS) && (DefaultWeapon->GetWeaponKillStats(OtherPS) > WeaponKills))
				{
					bIsBestOverall = false;
					break;
				}
			}
			if (bIsBestOverall)
			{
				PS->AddMatchHighlight(HighlightNames::MostWeaponKills, WeaponKills);
			}
		}
	}

	FName MultiKillsNames[4] = { NAME_MultiKillLevel3, NAME_MultiKillLevel2, NAME_MultiKillLevel1, NAME_MultiKillLevel0 };
	for (int32 i = 0; i < 4; i++)
	{
		if (PS->GetStatsValue(MultiKillsNames[i]) > 0)
		{
			PS->AddMatchHighlight(MultiKillsNames[i], PS->GetStatsValue(MultiKillsNames[i]));
			break;
		}
	}

	// announced kills
	FName AnnouncedKills[5] = { NAME_AmazingCombos, NAME_AirRox, NAME_AirSnot, NAME_SniperHeadshotKills, NAME_FlakShreds };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetStatsValue(AnnouncedKills[i]) > 0)
		{
			PS->AddMatchHighlight(AnnouncedKills[i], PS->GetStatsValue(AnnouncedKills[i]));
			if (PS->MatchHighlights[3] != NAME_None)
			{
				return;
			}
		}
	}
}

FText AUTGameState::ShortPlayerHighlightText(AUTPlayerState* PS, int32 Index)
{
	// return first highlight short version
	if (PS->MatchHighlights[0] == NAME_None)
	{
		return FText::GetEmpty();
	}
	if (PS->FavoriteWeapon && ((PS->MatchHighlights[Index] == HighlightNames::WeaponKills) || (PS->MatchHighlights[Index] == HighlightNames::MostWeaponKills)))
	{
		return PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->HighlightText;
	}
	FText BestWeaponText = PS->FavoriteWeapon ? PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->DisplayName : FText::GetEmpty();
	FText HighlightText = !ShortHighlightMap.FindRef(PS->MatchHighlights[Index]).IsEmpty() ? ShortHighlightMap.FindRef(PS->MatchHighlights[Index]) : HighlightMap.FindRef(PS->MatchHighlights[Index]);
	return FText::Format(HighlightText, FText::AsNumber(PS->MatchHighlightData[Index]), BestWeaponText);
}

FText AUTGameState::FormatPlayerHighlightText(AUTPlayerState* PS, int32 Index)
{
	if (PS->MatchHighlights[Index] == NAME_None)
	{
		return FText::GetEmpty();
	}
	FText BestWeaponText = PS->FavoriteWeapon ? PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->DisplayName : FText::GetEmpty();

	// special case for float KDR
	if (PS->MatchHighlights[Index] == HighlightNames::BestKD)
	{
		static const FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
			.SetMinimumFractionalDigits(2)
			.SetMaximumFractionalDigits(2);
		return FText::Format(HighlightMap.FindRef(PS->MatchHighlights[Index]), FText::AsNumber(PS->MatchHighlightData[Index], &FormatOptions), BestWeaponText);
	}

	return FText::Format(HighlightMap.FindRef(PS->MatchHighlights[Index]), FText::AsNumber(PS->MatchHighlightData[Index]), BestWeaponText);
}

TArray<FText> AUTGameState::GetPlayerHighlights_Implementation(AUTPlayerState* PS)
{
	TArray<FText> Highlights;
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->MatchHighlights[i] != NAME_None)
		{
			Highlights.Add(FormatPlayerHighlightText(PS, i));
		}
	}
	return Highlights;
}

float AUTGameState::MatchHighlightScore(AUTPlayerState* PS)
{
	float BestHighlightScore = 0.f;
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->MatchHighlights[i] != NAME_None)
		{
			BestHighlightScore = FMath::Max(BestHighlightScore, HighlightPriority.FindRef(PS->MatchHighlights[i]));
		}
	}
	return BestHighlightScore;
}

bool AUTGameState::AllowMinimapFor(AUTPlayerState* PS)
{
	return PS && (PS->bOnlySpectator || PS->bOutOfLives);
}

void AUTGameState::FillOutRconPlayerList(TArray<FRconPlayerData>& PlayerList)
{
	for (int32 i = 0; i < PlayerList.Num(); i++)
	{
		PlayerList[i].bPendingDelete = true;
	}

	for (int32 i = 0; i < PlayerArray.Num(); i++)
	{
		if (PlayerArray[i] && !PlayerArray[i]->IsPendingKillPending())
		{
			APlayerController* PlayerController = Cast<APlayerController>( PlayerArray[i]->GetOwner() );
			if (PlayerController && Cast<AUTDemoRecSpectator>(PlayerController) == nullptr)
			{
				FString PlayerID = PlayerArray[i]->UniqueId.ToString();

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
					AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(PlayerArray[i]);
					AUTBaseGameMode* DefaultGame = GameModeClass ? GameModeClass->GetDefaultObject<AUTBaseGameMode>() : NULL;
					int32 Rank = UTPlayerState && DefaultGame ? DefaultGame->GetEloFor(UTPlayerState, bRankedSession) : 0;
					FString PlayerIP = PlayerController->GetPlayerNetworkAddress();
					FRconPlayerData PlayerInfo(PlayerArray[i]->PlayerName, PlayerID, PlayerIP, Rank, UTPlayerState->bOnlySpectator);
					PlayerList.Add( PlayerInfo );
				}
			}
		}
	}
}

void AUTGameState::MakeJsonReport(TSharedPtr<FJsonObject> JsonObject)
{
	JsonObject->SetStringField(TEXT("ServerName"), ServerName);

	JsonObject->SetBoolField(TEXT("bWeaponStay"), bWeaponStay);
	JsonObject->SetBoolField(TEXT("bTeamGame"), bTeamGame);
	JsonObject->SetBoolField(TEXT("bAllowTeamSwitches"), bAllowTeamSwitches);
	JsonObject->SetBoolField(TEXT("bStopGameClock"), bStopGameClock);

	JsonObject->SetNumberField(TEXT("GoalScore"), GoalScore);
	JsonObject->SetNumberField(TEXT("TimeLimit"), TimeLimit);
	JsonObject->SetNumberField(TEXT("SpawnProtectionTime"), SpawnProtectionTime);
	JsonObject->SetNumberField(TEXT("RemainingTime"), RemainingTime);
	JsonObject->SetNumberField(TEXT("ElapsedTime"), ElapsedTime);
	JsonObject->SetNumberField(TEXT("RespawnWaitTime"), RespawnWaitTime);
	JsonObject->SetNumberField(TEXT("ForceRespawnTime"), ForceRespawnTime);

	TArray<TSharedPtr<FJsonValue>> PlayersJson;
	for (int32 i=0; i < PlayerArray.Num(); i++)
	{
		if (PlayerArray[i])
		{
			AUTPlayerState* PlayerState = Cast<AUTPlayerState>(PlayerArray[i]);
			if (PlayerState)
			{
				TSharedPtr<FJsonObject> PJson = MakeShareable(new FJsonObject);
				PlayerState->MakeJsonReport(PJson);
				PlayersJson.Add( MakeShareable( new FJsonValueObject( PJson )) );			
			}
		}
	}

	JsonObject->SetArrayField(TEXT("Players"),  PlayersJson);
}

bool AUTGameState::CanShowBoostMenu(AUTPlayerController* Target)
{
	return IsMatchIntermission() || !HasMatchStarted();
}

AUTLineUpZone* AUTGameState::GetAppropriateSpawnList(LineUpTypes ZoneType)
{
	AUTLineUpZone* FoundPotentialMatch = nullptr;

	if (GetWorld() && ZoneType != LineUpTypes::Invalid)
	{
		for (TActorIterator<AUTLineUpZone> It(GetWorld()); It; ++It)
		{
			if (It->ZoneType == ZoneType)
			{
				//Found a perfect match, so return it right away. Perfect match because this is a team spawn point in a team game
				if (It->bIsTeamSpawnList && bTeamGame)
				{
					return *It;
				}
				//Found a perfect match, so return it right away. Perfect match because this is not a team spawn point in a non-team game
				else if (!It->bIsTeamSpawnList && (!bTeamGame))
				{
					return *It;
				}

				//Imperfect match, try and find a perfect match before returning it
				FoundPotentialMatch = *It;
			}
		}
	}

	return FoundPotentialMatch;
}

void AUTGameState::CreateLineUp(LineUpTypes LineUpType)
{
	if (ActiveLineUpHelper)
	{
		ClearLineUp();
	}

	if ((GetNetMode() != NM_Client) && (ActiveLineUpHelper == nullptr))
	{
		ActiveLineUpHelper = GetWorld()->SpawnActor<AUTLineUpHelper>();
		ActiveLineUpHelper->InitializeLineUp(LineUpType);

		ActiveLineUpHelper->SetReplicates(true);
	}
}

void AUTGameState::ClearLineUp()
{
	if (ActiveLineUpHelper)
	{
		ActiveLineUpHelper->CleanUp();
		ActiveLineUpHelper->Destroy();

		ActiveLineUpHelper = nullptr;
	}
}

AUTLineUpZone* AUTGameState::CreateLineUpAtPlayerStart(LineUpTypes LineUpType, APlayerStart* PlayerSpawn)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	AUTLineUpZone* NewZone = nullptr;

	TSubclassOf<AUTLineUpZone> LineUpClass = LoadClass<AUTLineUpZone>(NULL, TEXT("/Game/RestrictedAssets/Blueprints/LineUpZone.LineUpZone_C"), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (LineUpClass == NULL)
	{
		LineUpClass = AUTLineUpZone::StaticClass();
	}

	if (PlayerSpawn)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = PlayerSpawn;
		NewZone = GetWorld()->SpawnActor<AUTLineUpZone>(LineUpClass, SpawnParams);

		if (NewZone)
		{
			NewZone->ZoneType = LineUpType;
			NewZone->bIsTeamSpawnList = (Teams.Num() > 0) ? true : false;
			NewZone->bSnapToFloor = false;

			NewZone->SetActorLocationAndRotation(PlayerSpawn->GetActorLocation() + FVector(0.0f, 0.0f, NewZone->SnapFloorOffset), PlayerSpawn->GetActorRotation());
			NewZone->DefaultCreateForOnly1Character();

			NewZone->AttachToActor(PlayerSpawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			SpawnedLineUps.Add(NewZone);

			//See if the new zone's camera is stuck inside of a wall
			if (GetWorld())
			{
				FHitResult CameraCollision;
				GetWorld()->SweepSingleByChannel(CameraCollision, NewZone->GetActorLocation(), NewZone->Camera->GetComponentLocation(), FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeBox(FVector(12.f)), FCollisionQueryParams(NAME_FreeCam, false, this));

				if (CameraCollision.bBlockingHit)
				{
					NewZone->Camera->SetWorldLocation(CameraCollision.ImpactPoint);
				}
			}
		}
	}
	return NewZone;
}

void AUTGameState::SpawnDefaultLineUpZones()
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	const bool bIsTeamGame = (Teams.Num() > 0) ? true : false;
	bool bIsTutorialGame = false;
	
	if ((GetNetMode() == NM_Standalone) && GetWorld())
	{
		AUTGameMode* UTGM = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (UTGM)
		{
			bIsTutorialGame = UTGM->bBasicTrainingGame;
		}
	}

	if (!bIsTutorialGame)
	{
		APlayerStart* PlayerStartToSpawnOn = nullptr;

		for (TActorIterator<AUTTeamPlayerStart> It(GetWorld()); It; ++It)
		{
			PlayerStartToSpawnOn = *It;
			break;
		}

		if (!bIsTeamGame || (PlayerStartToSpawnOn == nullptr))
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				PlayerStartToSpawnOn = *It;
				break;
			}
		}

		if (PlayerStartToSpawnOn)
		{
			if (GetAppropriateSpawnList(LineUpTypes::Intro) == nullptr)
			{
				AUTLineUpZone* NewZone = CreateLineUpAtPlayerStart(LineUpTypes::Intro, PlayerStartToSpawnOn);
			}

			if (GetAppropriateSpawnList(LineUpTypes::Intermission) == nullptr)
			{
				AUTLineUpZone* NewZone = CreateLineUpAtPlayerStart(LineUpTypes::Intermission, PlayerStartToSpawnOn);
			}

			if (GetAppropriateSpawnList(LineUpTypes::PostMatch) == nullptr)
			{
				AUTLineUpZone* NewZone = CreateLineUpAtPlayerStart(LineUpTypes::PostMatch, PlayerStartToSpawnOn);
			}
		}
	}
}

UCameraComponent* AUTGameState::GetCameraComponentForLineUp(LineUpTypes ZoneType)
{
	UCameraComponent* FoundCamera = nullptr;

	AUTLineUpZone* SpawnPointList = GetAppropriateSpawnList(ZoneType);
	if (SpawnPointList)
	{
		FoundCamera = SpawnPointList->Camera;
	}

	return FoundCamera;
}

bool AUTGameState::IsMapVoteListReplicationCompleted()
{
	bool bMapVoteListReplicationComplete = true;
	if (MapVoteListCount > 0 && MapVoteList.Num() == MapVoteListCount)
	{
		for (int32 i=0; i < MapVoteList.Num(); i++)
		{
			if (MapVoteList[i] == nullptr)
			{
				bMapVoteListReplicationComplete = false;
				break;
			}
		}
	}

	return bMapVoteListReplicationComplete;
}

void AUTGameState::PrepareForIntermission()
{
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (Cast<AUTRemoteRedeemer>(It->Get()))
		{
			TArray<USceneComponent*> Components;
			(*It)->GetComponents<USceneComponent>(Components);
			for (int32 i = 0; i < Components.Num(); i++)
			{
				UAudioComponent* Audio = Cast<UAudioComponent>(Components[i]);
				if (Audio != NULL)
				{
					Audio->Stop();
				}
			}
		}
		else
		{
			AUTCharacter* Char = Cast<AUTCharacter>(*It);
			if (Char)
			{
				Char->PrepareForIntermission();
			}
		}
	}

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AUTProjectile* TestProj = Cast<AUTProjectile>(*It);
		if (TestProj && !TestProj->IsPendingKill())
		{
			TestProj->PrepareForIntermission();
		}
	}

	// inform actors of intermission start
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(UUTIntermissionBeginInterface::StaticClass()))
		{
			IUTIntermissionBeginInterface::Execute_IntermissionBegin(*It);
		}
	}

	for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
	{
		UParticleSystemComponent* PSC = *It;
		if (PSC && !PSC->IsTemplate() && PSC->GetOwner() && (PSC->GetOwner()->GetWorld() == GetWorld()) && !Cast<AUTWeapon>(PSC->GetOwner()))
		{
			PSC->CustomTimeDilation = 0.001f;
		}
	}
	bNeedToClearIntermission = true;
}

bool AUTGameState::HasMatchEnded() const
{
	if (GetMatchState() == MatchState::MapVoteHappening || GetMatchState() == MatchState::WaitingTravel)
	{
		return true;
	}

	return Super::HasMatchEnded();
}

bool AUTGameState::IsLineUpActive()
{
	return ((ActiveLineUpHelper != nullptr) && !ActiveLineUpHelper->IsPendingKillPending() && ActiveLineUpHelper->IsActive());
}

void AUTGameState::OnReceiveHubGuid()
{
	// Pass the HubGuid replicated from the server on to all local players so they know where to return to.
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(*It);
		if (UTLocalPlayer)
		{
			UTLocalPlayer->ReturnDestinationGuidString = HubGuid.IsValid() ? HubGuid.ToString() : TEXT("");
		}
	}
}

AUTTeamInfo* AUTGameState::FindLeadingTeam()
{
	AUTTeamInfo* LeadingTeam = NULL;
	bool bTied = false;

	if (Teams.Num() > 0)
	{
		LeadingTeam = Teams[0];
		if (LeadingTeam)
		{
			for (int32 i = 1; i<Teams.Num(); i++)
			{
				if (Teams[i] != nullptr)
				{
					if (Teams[i]->Score == LeadingTeam->Score)
					{
						bTied = true;
					}
					else if (Teams[i]->Score > LeadingTeam->Score)
					{
						LeadingTeam = Teams[i];
						bTied = false;
					}
				}
			}
		}

		if (bTied) LeadingTeam = NULL;
	}
	return LeadingTeam;
}
