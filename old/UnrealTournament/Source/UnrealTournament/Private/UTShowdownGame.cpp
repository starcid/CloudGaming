// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTShowdownGame.h"
#include "UTTimerMessage.h"
#include "UTShowdownGameMessage.h"
#include "UTHUD_Showdown.h"
#include "UTShowdownGameState.h"
#include "UTTimedPowerup.h"
#include "SlateGameResources.h"
#include "SUWindowsStyle.h"
#include "SUTStyle.h"
#include "SNumericEntryBox.h"
#include "UTShowdownSquadAI.h"
#include "UTGenericObjectivePoint.h"
#include "UTShowdownRewardMessage.h"
#include "UTShowdownStatusMessage.h"
#include "UTFirstBloodMessage.h"
#include "UTMutator.h"
#include "StatNames.h"
#include "UTSpectatorCamera.h"
#include "UTPickupAmmo.h"
#include "UTPlayerStart.h"
#include "UTLineUpHelper.h"

AUTShowdownGame::AUTShowdownGame(const FObjectInitializer& OI)
: Super(OI)
{
	ExtraHealth = 100;
	bForceRespawn = false;
	DisplayName = NSLOCTEXT("UTGameMode", "Showdown", "Duel Showdown");
	TimeLimit = 2; // per round
	GoalScore = 5;
	SpawnSelectionTime = 9;
	PowerupDuration = 20.0f;
	XPMultiplier = 1.0f;
	bHasRespawnChoices = false; // unique system
	HUDClass = AUTHUD_Showdown::StaticClass();
	GameStateClass = AUTShowdownGameState::StaticClass();
	StartingArmorObject.Reset();

	PowerupBreakerPickupClass.SetPath(TEXT("/Game/RestrictedAssets/Pickups/Powerups/SuperchargeBase.SuperchargeBase_C"));
	PowerupBreakerItemClass.SetPath(TEXT("/Game/RestrictedAssets/Pickups/Powerups/BP_Supercharge.BP_Supercharge_C"));

	SuperweaponReplacementPickupClass.SetPath(TEXT("/Game/RestrictedAssets/Pickups/Powerups/PowerupBase.PowerupBase_C"));
	SuperweaponReplacementItemClass.SetPath(TEXT("/Game/RestrictedAssets/Pickups/Powerups/BP_Invis.BP_Invis_C"));

	bPowerupBreaker = true;
}

void AUTShowdownGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	bXRayBreaker = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("XRayBreaker")), bXRayBreaker);
	bPowerupBreaker = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("PowerupBreaker")), bPowerupBreaker);

	// this game mode requires a goal score
	if (GoalScore <= 0)
	{
		GoalScore = 5;
	}
}

void AUTShowdownGame::InitGameState()
{
	Super::InitGameState();

	if (bPowerupBreaker)
	{
		TSubclassOf<AUTTimedPowerup> PowerupClass = Cast<UClass>(PowerupBreakerItemClass.TryLoad());
		if (PowerupClass != NULL)
		{
			PowerupClass.GetDefaultObject()->AddOverlayMaterials(UTGameState);
		}
	}
}

void AUTShowdownGame::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AUTPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->AssociatedPickup != nullptr)
		{
			bAssociatedPickupsSet = true;
			break;
		}
	}
}

void AUTShowdownGame::HandleCountdownToBegin()
{
	if (bIsQuickMatch && UTGameState)
	{
		UTGameState->bAllowTeamSwitches = false;
		UTGameState->ForceNetUpdate();
	}
	BeginGame();
}

void AUTShowdownGame::ScoreDamage_Implementation(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker)
{
	Super::ScoreDamage_Implementation(DamageAmount, Victim, Attacker);
	if (Victim && Attacker && UTGameState && !UTGameState->OnSameTeam(Victim, Attacker))
	{
		Attacker->AdjustScore(DamageAmount);
	}
}

void AUTShowdownGame::StartMatch()
{
	if (HasMatchStarted())
	{
		// Already started
		return;
	}
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS && !PS->bIsInactive && !PS->bOnlySpectator)
		{
			PS->SetOutOfLives(false);
		}
	}

	Super::StartMatch();
}

void AUTShowdownGame::StartNewRound()
{
	RoundElapsedTime = 0;
	LastRoundWinner = NULL;
	// reset everything
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		// pickups spawn at start, don't respawn
		AUTPickup* Pickup = Cast<AUTPickup>(*It);
		if (Pickup != NULL && Pickup != BreakerPickup)
		{
			Pickup->bDelayedSpawn = false;
			if (!Pickup->IsA<AUTPickupAmmo>() && (!UTGameState->bWeaponStay || !Pickup->IsA<AUTPickupWeapon>()))
			{
				Pickup->RespawnTime = 0.0f;
			}
		}
		if (Cast<AUTCharacter>(*It) != NULL)
		{
			It->Destroy();
		}
		if (It->GetClass()->ImplementsInterface(UUTResetInterface::StaticClass()))
		{
			IUTResetInterface::Execute_Reset(*It);
		}
	}
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS && !PS->bIsInactive && !PS->bOnlySpectator)
		{
			PS->SetOutOfLives(false);
		}
	}

	// respawn players
	bAllowPlayerRespawns = true;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (C != NULL && C->GetPawn() == NULL && C->PlayerState != NULL && !C->PlayerState->bOnlySpectator)
		{
			// don't spawn players that joined during spawn selection and therefore didn't get to pick yet
			AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
			if (PS == NULL || PS->SelectionOrder != 255)
			{
				RestartPlayer(It->Get());
			}
		}
	}
	bAllowPlayerRespawns = false;
	UTGameState->SetTimeLimit(TimeLimit);
	AnnounceMatchStart();
}

bool AUTShowdownGame::CheckRelevance_Implementation(AActor* Other)
{
	if (BreakerPickup != NULL && Cast<AUTTimedPowerup>(Other) != NULL && Other->GetClass() == BreakerPickup->GetInventoryType())
	{
		// don't override breaker powerup duration
		return true;
	}
	else
	{
		AUTPickupWeapon* PickupWeapon = Cast<AUTPickupWeapon>(Other);
		if (PickupWeapon != NULL && PickupWeapon->WeaponType != NULL && PickupWeapon->WeaponType.GetDefaultObject()->bMustBeHolstered)
		{
			TSubclassOf<AUTPickupInventory> ReplacementPickupClass = SuperweaponReplacementPickupClass.TryLoadClass<AUTPickupInventory>();
			TSubclassOf<AUTInventory> ReplacementItemClass = SuperweaponReplacementItemClass.TryLoadClass<AUTInventory>();
			if (ReplacementPickupClass != NULL && ReplacementItemClass != NULL)
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AUTPickupInventory* Pickup = GetWorld()->SpawnActor<AUTPickupInventory>(ReplacementPickupClass, PickupWeapon->GetActorLocation(), PickupWeapon->GetActorRotation(), Params);
				if (Pickup != NULL)
				{
					Pickup->SetInventoryType(ReplacementItemClass);
				}
			}
			return false;
		}
		else
		{
			AUTPickupAmmo* AmmoPack = Cast<AUTPickupAmmo>(Other);
			if (AmmoPack != NULL)
			{
				AmmoPack->RespawnTime *= 1.5f;
			}
			AUTTimedPowerup* Powerup = Cast<AUTTimedPowerup>(Other);
			if (Powerup)
			{
				Powerup->TimeRemaining = PowerupDuration;
			}
			return AUTTeamDMGameMode::CheckRelevance_Implementation(Other);
		}
	}
}

void AUTShowdownGame::HandleMatchHasStarted()
{
	if (UTGameState != NULL)
	{
		UTGameState->CompactSpectatingIDs();
	}
	UTGameState->SetTimeLimit(TimeLimit);
	bFirstBloodOccurred = false;

	GameSession->HandleMatchHasStarted();

	// Make sure level streaming is up to date before triggering NotifyMatchStarted
	GEngine->BlockTillLevelStreamingCompleted(GetWorld());

	// First fire BeginPlay, if we haven't already in waiting to start match
	GetWorldSettings()->NotifyBeginPlay();

	// Then fire off match started
	GetWorldSettings()->NotifyMatchStarted();

	if (UTIsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StartRecordingReplay(TEXT(""), GetWorld()->GetMapName());
	}
	
	SetMatchState(MatchState::MatchIntermission);
	Cast<AUTShowdownGameState>(GameState)->IntermissionStageTime = 1;
}

void AUTShowdownGame::SetPlayerDefaults(APawn* PlayerPawn)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(PlayerPawn);
	if (UTC != NULL)
	{
		UTC->HealthMax = UTC->GetClass()->GetDefaultObject<AUTCharacter>()->HealthMax + ExtraHealth;
		UTC->SuperHealthMax = UTC->GetClass()->GetDefaultObject<AUTCharacter>()->SuperHealthMax + ExtraHealth;
	}
	Super::SetPlayerDefaults(PlayerPawn);
}

void AUTShowdownGame::ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	if (GetMatchState() != MatchState::MatchIntermission && (TimeLimit <= 0 || UTGameState->GetRemainingTime() > 0))
	{
		if (Other != NULL)
		{
			AUTPlayerState* OtherPS = Cast<AUTPlayerState>(Other->PlayerState);
			if (OtherPS != NULL && OtherPS->Team != NULL)
			{
				OtherPS->SetOutOfLives(true);
				AUTPlayerState* KillerPS = (Killer != NULL && Killer != Other) ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
				AUTTeamInfo* KillerTeam = (KillerPS != NULL) ? KillerPS->Team : Teams[1 - FMath::Min<int32>(1, OtherPS->Team->TeamIndex)];
				KillerTeam->Score += 1;

				if (LastRoundWinner == NULL)
				{
					LastRoundWinner = KillerTeam;
				}
				else if (LastRoundWinner != KillerTeam)
				{
					LastRoundWinner = NULL; // both teams got a point so nobody won
				}

				// this is delayed so mutual kills can happen
				SetTimerUFunc(this, FName(TEXT("StartIntermission")), 1.0f, false);
			}
		}
		if (Killer != Other && Killer != NULL && UTGameState->OnSameTeam(Killer, Other))
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(Killer);
			if (PC)
			{
				PC->ClientReceiveLocalizedMessage(UUTShowdownRewardMessage::StaticClass(), 3, PC->PlayerState, NULL, NULL);
			}
			// AUTGameMode doesn't handle team kills and AUTTeamDMGameMode would change the team score so we need to do it ourselves
			AUTPlayerState* KillerPS = Cast<AUTPlayerState>(Killer->PlayerState);
			if (KillerPS != NULL)
			{
				KillerPS->AdjustScore(-100);
			}
		}
		else
		{
			AUTPlayerState* OtherPlayerState = Other ? Cast<AUTPlayerState>(Other->PlayerState) : NULL;
			if ((Killer == Other) || (Killer == NULL))
			{
				// If it's a suicide, subtract a kill from the player...
				if (OtherPlayerState)
				{
					OtherPlayerState->AdjustScore(-100);
				}
			}
			else
			{
				AUTPlayerState * KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
				if (KillerPlayerState != NULL)
				{
					KillerPlayerState->AdjustScore(+100);
					KillerPlayerState->IncrementKills(DamageType, true, OtherPlayerState);
					CheckScore(KillerPlayerState);

					if (!bFirstBloodOccurred)
					{
						BroadcastLocalized(this, UUTFirstBloodMessage::StaticClass(), 0, KillerPlayerState, NULL, NULL);
						bFirstBloodOccurred = true;
					}
					TrackKillAssists(Killer, Other, KilledPawn, DamageType, KillerPlayerState, OtherPlayerState);
				}
			}
		}
		AddKillEventToReplay(Killer, Other, DamageType);
		if (BaseMutator != NULL)
		{
			BaseMutator->ScoreKill(Killer, Other, DamageType);
		}
		FindAndMarkHighScorer();
	}
}

AInfo* AUTShowdownGame::GetTiebreakWinner(FName* WinReason) const
{
	// end round; player with highest health
	TArray< AUTPlayerState*, TInlineAllocator<2> > AlivePlayers;
	AUTPlayerState* RoundWinner = NULL;
	int32 BestTotalHealth = 0;
	bool bTied = false;
	bool bStartingHealth = false;
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AUTCharacter* UTC = Cast<AUTCharacter>(It->Get());
		if (UTC != NULL && !UTC->IsDead())
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(UTC->PlayerState);
			if (PS != NULL)
			{
				AlivePlayers.Add(PS);
				if (UTC->Health > BestTotalHealth)
				{
					RoundWinner = PS;
					BestTotalHealth = UTC->Health;
				}
				else if (UTC->Health == BestTotalHealth)
				{
					bTied = true;
					bStartingHealth = UTC->Health == UTC->GetClass()->GetDefaultObject<AUTCharacter>()->HealthMax + ExtraHealth;
				}
			}
		}
	}

	if (WinReason != NULL)
	{
		*WinReason = (bTied && bStartingHealth) ? FName(TEXT("NoAction")) : FName(TEXT("Health"));
	}
	return (bTied ? NULL : RoundWinner);
}

void AUTShowdownGame::ScoreExpiredRoundTime()
{
	FName WinReason;
	AUTPlayerState* RoundWinner = Cast<AUTPlayerState>(GetTiebreakWinner(&WinReason));
	if (RoundWinner == NULL)
	{
		if (WinReason != FName(TEXT("NoAction")))
		{
			// both players score a point
			for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
			{
				AUTCharacter* UTC = Cast<AUTCharacter>(It->Get());
				if (UTC != NULL && !UTC->IsDead())
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(UTC->PlayerState);
					if (PS != NULL)
					{
						PS->Score += 1.0f;
						if (PS->Team != NULL)
						{
							PS->Team->Score += 1.0f;
						}
					}
				}
			}
			BroadcastLocalized(NULL, UUTShowdownGameMessage::StaticClass(), 1);
		}
		LastRoundWinner = NULL;
	}
	else
	{
		RoundWinner->Score += 1.0f;
		if (RoundWinner->Team != NULL)
		{
			RoundWinner->Team->Score += 1.0f;
		}
		LastRoundWinner = RoundWinner->Team;
		BroadcastLocalized(NULL, UUTShowdownGameMessage::StaticClass(), 0, RoundWinner);
	}
}

void AUTShowdownGame::CheckGameTime()
{
	static FName NAME_StartIntermission(TEXT("StartIntermission"));
	if (IsMatchInProgress() && !HasMatchEnded() && TimeLimit > 0 && UTGameState->GetRemainingTime() <= 0 && !IsTimerActiveUFunc(this, NAME_StartIntermission))
	{
		ScoreExpiredRoundTime();
		SetTimerUFunc(this, NAME_StartIntermission, 2.0f, false);
	}
}

void AUTShowdownGame::StartIntermission()
{
	ClearTimerUFunc(this, FName(TEXT("StartIntermission")));
	// survival bonus
	if (LastRoundWinner != NULL)
	{
		for (AController* C : LastRoundWinner->GetTeamMembers())
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
			if (PS != NULL)
			{
				PS->AdjustScore(50);
			}
		}
	}
	bPastELOLimit = true;
	if (!HasMatchEnded())
	{
		// if there's not enough time for a new round to work, then award victory now
		bool bTied = false;
		AUTPlayerState* Winner = NULL;
		Winner = IsThereAWinner(bTied);
		if (Winner != NULL && !bTied && ((Winner->Team == NULL) ? (Winner->Score >= GoalScore) : (Winner->Team->Score >= GoalScore)))
		{
			EndGame(Winner, TEXT("scorelimit"));
		}
		else
		{
			if (LastRoundWinner != nullptr)
			{
				BroadcastLocalized(NULL, UUTShowdownGameMessage::StaticClass(), 3 + LastRoundWinner->TeamIndex);
			}
			SetMatchState(MatchState::MatchIntermission);
			GameState->ForceNetUpdate();
		}
	}
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC && PC->UTPlayerState)
		{
			PC->ClientUpdateDamageDone(PC->UTPlayerState->DamageDone, PC->UTPlayerState->RoundDamageDone);
		}
	}
}

void AUTShowdownGame::RestartPlayer(AController* aPlayer)
{
	if (GetMatchState() == MatchState::WaitingToStart || (UTGameState && UTGameState->IsLineUpActive()))
	{
		// warmup or line-up
		Super::RestartPlayer(aPlayer);
	}
	else
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(aPlayer->PlayerState);
		if (PS)
		{
			PS->SetOutOfLives(!bAllowPlayerRespawns);
		}
		if (bAllowPlayerRespawns)
		{
			Super::RestartPlayer(aPlayer);
		}
	}
}

AActor* AUTShowdownGame::ChoosePlayerStart_Implementation(AController* Player)
{
	// if they pre-selected, apply it
	AUTPlayerState* UTPS = Cast<AUTPlayerState>((Player != NULL) ? Player->PlayerState : NULL);
	if (UTPS != NULL && UTPS->RespawnChoiceA != nullptr)
	{
		return UTPS->RespawnChoiceA;
	}
	else
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
}

float AUTShowdownGame::RatePlayerStart(APlayerStart* P, AController* Player)
{
	AUTShowdownGameState* GS = Cast<AUTShowdownGameState>(GameState);
	if (Player != NULL && GS->SpawnSelector == Player->PlayerState && !GS->IsAllowedSpawnPoint(Cast<AUTPlayerState>(Player->PlayerState), P))
	{
		return -1000.0f;
	}
	else
	{
		return Super::RatePlayerStart(P, Player);
	}
}

void AUTShowdownGame::HandleMatchIntermission()
{
	// respawn breaker pickup
	if (bPowerupBreaker)
	{
		if (BreakerPickup != NULL)
		{
			BreakerPickup->Destroy();
		}
		TSubclassOf<AUTPickupInventory> PickupClass = Cast<UClass>(PowerupBreakerPickupClass.TryLoad());
		TSubclassOf<AUTTimedPowerup> PowerupClass = Cast<UClass>(PowerupBreakerItemClass.TryLoad());
		if (PickupClass != NULL && PowerupClass != NULL)
		{
			FVector SpawnLoc;
			TArray<AUTGenericObjectivePoint*> PlacedSpawnPoints;
			for (TActorIterator<AUTGenericObjectivePoint> It(GetWorld()); It; ++It)
			{
				PlacedSpawnPoints.Add(*It);
			}
			if (PlacedSpawnPoints.Num() > 0)
			{
				SpawnLoc = PlacedSpawnPoints[FMath::RandHelper(PlacedSpawnPoints.Num())]->GetActorLocation();
			}
			else
			{
				AActor* StartSpot = FindPlayerStart(NULL);
				SpawnLoc = StartSpot->GetActorLocation();
				AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
				if (NavData != NULL)
				{
					FVector Extent = NavData->GetPOIExtent(StartSpot);
					FRandomDestEval NodeEval;
					float Weight = 0.0f;
					TArray<FRouteCacheItem> Route;					
					if (NavData->FindBestPath(NULL, FNavAgentProperties(Extent.X, Extent.Z * 2.0f), NULL, NodeEval, StartSpot->GetActorLocation(), Weight, false, Route) && Route.Num() > 0)
					{
						SpawnLoc = Route.Last().GetLocation(NULL);
						// try to pick a better poly for spawning (we'd like to stay away from walls)
						for (int32 i = Route.Num() - 1; i >= 0; i--)
						{
							if (NavData->GetPolySurfaceArea2D(Route[i].TargetPoly) > 10000.0f || NavData->GetPolyWalls(Route[i].TargetPoly).Num() == 0)
							{
								SpawnLoc = Route[i].GetLocation(NULL) + FVector(0.0f, 0.0f, Extent.Z);
								break;
							}
						}
					}
				}
			}
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			BreakerPickup = GetWorld()->SpawnActor<AUTPickupInventory>(PickupClass, SpawnLoc, FRotator(0.0f, 360.0f * FMath::FRand(), 0.0f), Params);
			if (BreakerPickup != NULL)
			{
				BreakerPickup->SetInventoryType(PowerupClass);
				BreakerPickup->bDelayedSpawn = true;
				BreakerPickup->RespawnTime = 70.0f;
			}
		}
	}
	if ((Teams.Num() >= 2) && Teams[0] && Teams[1] && (Teams[0]->GetSize() > 1) && (Teams[1]->GetSize() > 1))
	{
		bFirstBloodOccurred = false;
	}

	AUTShowdownGameState* GS = Cast<AUTShowdownGameState>(GameState);

	GS->bActivateXRayVision = false;
	GS->IntermissionStageTime = 5;
	GS->bStartedSpawnSelection = false;
	GS->bFinalIntermissionDelay = false;
	RemainingPicks.Empty();
	GS->SpawnSelector = NULL;
	GS->SetRemainingTime(0);
	// reset timer for consistency
	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AUTGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation() / GetWorldSettings()->DemoPlayTimeDilation, true);

	// reset pickups in advance so they show up on the spawn previews
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AUTPickup* Pickup = Cast<AUTPickup>(*It);
		if (Pickup != NULL)
		{
			checkSlow(Pickup->GetClass()->ImplementsInterface(UUTResetInterface::StaticClass()));
			IUTResetInterface::Execute_Reset(Pickup);
		}
	}

	// give players spawn point selection
	TMultiMap<AUTTeamInfo*, AUTPlayerState*> TeamPlayers;
	for (FConstControllerIterator ControllerIt = GetWorld()->GetControllerIterator(); ControllerIt; ++ControllerIt)
	{
		AController* C = ControllerIt->Get();
		if (C != NULL)
		{
			APawn* P = C->GetPawn();
			if (P != NULL)
			{
				APlayerState* SavedPlayerState = P->PlayerState; // keep the PlayerState reference for end of round HUD stuff
				P->TurnOff();
				C->UnPossess();
				P->PlayerState = SavedPlayerState;
				// we want the character around for the HUD displays of status but we don't need to actually see it and this prevents potential camera clipping
				P->GetRootComponent()->SetHiddenInGame(true, true);
				AUTCharacter* UTC = Cast<AUTCharacter>(P);
				if (UTC != NULL)
				{
					for (TInventoryIterator<AUTInventory> It((AUTCharacter*)P); It; ++It)
					{
						// weapon doesn't want to seem to stop firing consistently on clients, just destroy it since the round is over
						if (It->IsA(AUTWeapon::StaticClass()))
						{
							It->Destroy();
						}
						else
						{
							// prevent tick so powerups don't count down and so forth
							// don't want to destroy all of these because they might affect the status display (armor, etc)
							It->SetActorTickEnabled(false);
							GetWorldTimerManager().ClearAllTimersForObject(*It);
						}
					}
				}
			}
			AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
			if (PS != NULL && !PS->bOnlySpectator && PS->Team != NULL)
			{
				PS->RespawnChoiceA = NULL;
				PS->RespawnChoiceB = NULL;
				TeamPlayers.Add(PS->Team, PS);
				// use spectating state so camera can be placed on spawn selection
				AUTPlayerController* PC = Cast<AUTPlayerController>(C);
				if (PC != NULL && !PC->IsInState(NAME_Spectating))
				{
					PC->ChangeState(NAME_Spectating);
					PC->ClientGotoState(NAME_Spectating);
					PC->SetViewTarget(P);
				}
			}
		}
	}
	// sort players by previous selection
	TeamPlayers.ValueSort([&](const AUTPlayerState& A, const AUTPlayerState& B){ return A.SelectionOrder > B.SelectionOrder; });
	// rotate players
	for (AUTTeamInfo* Team : Teams)
	{
		TArray<AUTPlayerState*> Players;
		TeamPlayers.MultiFind(Team, Players);
		if (Players.Num() > 1)
		{
			// remove and re-add the last player, which puts it at the front of the list
			AUTPlayerState* LastPlayer = Players.Last();
			TeamPlayers.Remove(Team, LastPlayer);
			TeamPlayers.Add(Team, LastPlayer);
		}
	}
	// sort team pick order by: winner of previous round last, others sorted by lowest score to highest score
	TArray<AUTTeamInfo*> SortedTeams = Teams;
	SortedTeams.Sort([&](const AUTTeamInfo& A, const AUTTeamInfo& B)
	{
		if (&A == LastRoundWinner)
		{
			return false;
		}
		else if (&B == LastRoundWinner)
		{
			return true;
		}
		else
		{
			return A.Score < B.Score;
		}
	});

	while (TeamPlayers.Num() > 0)
	{
		bool bFoundAny = false;
		for (int32 i = 0; i < SortedTeams.Num(); i++)
		{
			AUTPlayerState* NextPlayer = TeamPlayers.FindRef(SortedTeams[i]);
			if (NextPlayer != NULL)
			{
				RemainingPicks.Add(NextPlayer);
				TeamPlayers.Remove(SortedTeams[i], NextPlayer);
				bFoundAny = true;
			}
		}
		if (!bFoundAny)
		{
			// sanity check so we don't infinitely recurse
			for (TMultiMap<AUTTeamInfo*, AUTPlayerState*>::TIterator It(TeamPlayers); It; ++It)
			{
				UE_LOG(UT, Warning, TEXT("Showdown spawn selection sorting error with %s on team %s"), *It.Value()->PlayerName, *It.Value()->Team->TeamName.ToString());
				RemainingPicks.Add(It.Value());
				It.RemoveCurrent();
				break;
			}
		}
	}
	for (int32 i = 0; i < RemainingPicks.Num(); i++)
	{
		RemainingPicks[i]->SelectionOrder = uint8(i);
	}
}

void AUTShowdownGame::CallMatchStateChangeNotify()
{
	if (GetMatchState() == MatchState::MatchIntermission)
	{
		HandleMatchIntermission();
	}
	else if (GetMatchState() == MatchState::InProgress && GetWorld()->bMatchStarted)
	{
		StartNewRound();
	}
	else
	{
		Super::CallMatchStateChangeNotify();
	}
}

AUTPickupInventory* AUTShowdownGame::GetNearestItemToStart(APlayerStart* Start) const
{
	if (bAssociatedPickupsSet)
	{
		// prefer LD specified item if possible
		AUTPlayerStart* UTStart = Cast<AUTPlayerStart>(Start);
		return (UTStart != nullptr) ? Cast<AUTPickupInventory>(UTStart->AssociatedPickup) : nullptr;
	}
	else
	{
		// fallback, find shortest line distance
		AUTPickupInventory* Best = nullptr;
		float BestDistSq = FLT_MAX;
		for (TActorIterator<AUTPickupInventory> It(GetWorld()); It; ++It)
		{
			float DistSq = (It->GetActorLocation() - Start->GetActorLocation()).SizeSquared();
			if (DistSq < BestDistSq)
			{
				Best = *It;
				BestDistSq = DistSq;
			}
		}
		
		if (Best == nullptr)
		{
			return nullptr;
		}
		else
		{
			// make sure there are no playerstarts that are closer
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				if ((It->GetActorLocation() - Best->GetActorLocation()).SizeSquared() < BestDistSq)
				{
					return nullptr;
				}
			}

			return Best;
		}
	}
}

APlayerStart* AUTShowdownGame::AutoSelectPlayerStart(AController* Requestor)
{
	TArray<AUTPickupInventory*> WeaponPickups;
	TArray<AUTPickupInventory*> ItemPickups;

	for (TActorIterator<AUTPickupInventory> It(GetWorld()); It; ++It)
	{
		if (It->GetInventoryType() != nullptr)
		{
			if (It->GetInventoryType()->IsChildOf(AUTWeapon::StaticClass()))
			{
				WeaponPickups.Add(*It);
			}
			ItemPickups.Add(*It);
		}
	}

	for (APlayerState* PS : UTGameState->PlayerArray)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
		if (UTPS != nullptr && UTPS->RespawnChoiceA != nullptr)
		{
			AUTPickupInventory* PickedItem = GetNearestItemToStart(UTPS->RespawnChoiceA);
			if (PickedItem != nullptr)
			{
				ItemPickups.Remove(PickedItem);
				WeaponPickups.Remove(PickedItem);
			}
		}
	}

	if (WeaponPickups.Num() > 0)
	{
		// pick playerstart near best weapon
		// TODO: evaluate player's weapon preference from stats?
		WeaponPickups.Sort([](const AUTPickupInventory& A, const AUTPickupInventory& B) { return A.GetInventoryType().GetDefaultObject()->BasePickupDesireability > B.GetInventoryType().GetDefaultObject()->BasePickupDesireability; });
		for (AUTPickupInventory* Item : WeaponPickups)
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				if (GetNearestItemToStart(*It) == Item && UTGameState->IsAllowedSpawnPoint(Cast<AUTPlayerState>(Requestor->PlayerState), *It))
				{
					return *It;
				}
			}
		}
	}
	if (ItemPickups.Num() > 0)
	{
		// no weapons, so at least pick powerup or armor
		ItemPickups.Sort([](const AUTPickupInventory& A, const AUTPickupInventory& B) { return A.GetInventoryType().GetDefaultObject()->BasePickupDesireability > B.GetInventoryType().GetDefaultObject()->BasePickupDesireability; });
		for (AUTPickupInventory* Item : ItemPickups)
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				if (GetNearestItemToStart(*It) == Item && UTGameState->IsAllowedSpawnPoint(Cast<AUTPlayerState>(Requestor->PlayerState), *It))
				{
					return *It;
				}
			}
		}
	}

	// fallback, just pick anything
	return Cast<APlayerStart>(FindPlayerStart(Requestor));
}

void AUTShowdownGame::DefaultTimer()
{
	AUTShowdownGameState* GS = Cast<AUTShowdownGameState>(GameState);
	if (GetMatchState() == MatchState::MatchIntermission)
	{
		// process bot spawn selection
		if (GS->SpawnSelector != NULL && ((GS->IntermissionStageTime < FMath::Max(2,SpawnSelectionTime - 2)) || (FMath::FRand() < 0.3f)))
		{
			AUTBot* B = Cast<AUTBot>(GS->SpawnSelector->GetOwner());
			if (B != NULL)
			{
				TArray<APlayerStart*> Choices;
				for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
				{
					if (GS->IsAllowedSpawnPoint(GS->SpawnSelector, *It))
					{
						Choices.Add(*It);
					}
				}
				GS->SpawnSelector->RespawnChoiceA = B->PickSpawnPoint(Choices);
			}
		}

		if (GS->SpawnSelector != NULL && (GS->SpawnSelector->IsPendingKillPending() || GS->SpawnSelector->RespawnChoiceA != NULL))
		{
			GS->IntermissionStageTime = 0;
		}
		else if (GS->IntermissionStageTime > 0)
		{
			GS->IntermissionStageTime--;
		}
		if (GS->IntermissionStageTime == 0)
		{
			if (!GS->bStartedSpawnSelection)
			{
				GS->bStartedSpawnSelection = true;
				GS->bStartingSpawnSelection = true;
				GS->IntermissionStageTime = 3;
			}
			else
			{
				if (GS->SpawnSelector != NULL)
				{
					if (GS->SpawnSelector->RespawnChoiceA == NULL && !GS->SpawnSelector->IsPendingKillPending())
					{
						GS->SpawnSelector->RespawnChoiceA = AutoSelectPlayerStart(Cast<AController>(GS->SpawnSelector->GetOwner()));
						GS->SpawnSelector->ForceNetUpdate();
					}
					RemainingPicks.Remove(GS->SpawnSelector);
					GS->SpawnSelector = NULL;
				}
				// make sure we don't have any stale entries from quitters
				for (int32 i = RemainingPicks.Num() - 1; i >= 0; i--)
				{
					if (RemainingPicks[i] == NULL || RemainingPicks[i]->IsPendingKillPending())
					{
						RemainingPicks.RemoveAt(i);
					}
				}
				if (RemainingPicks.Num() > 0)
				{
					GS->SpawnSelector = RemainingPicks[0];
					GS->IntermissionStageTime = FMath::Max<uint8>(1, SpawnSelectionTime);
				}
				else if (!GS->bFinalIntermissionDelay)
				{
					int32 MessageIndex = ((Teams.Num() >= 2) && Teams[0] && Teams[1] && (Teams[0]->Score == GoalScore - 1) && (Teams[1]->Score == GoalScore - 1)) ? 6 : 5;
					GS->bFinalIntermissionDelay = true;
					GS->IntermissionStageTime = 5;
					BroadcastLocalized(NULL, UUTShowdownGameMessage::StaticClass(), MessageIndex);
				}
				else
				{
					GS->bMatchHasStarted = true;
					GS->bFinalIntermissionDelay = false;
					SetMatchState(MatchState::InProgress);
				}
			}
		}
		if (GS->bFinalIntermissionDelay && (GS->IntermissionStageTime < 4))
		{
			BroadcastLocalized(NULL, UUTTimerMessage::StaticClass(), int32(GS->IntermissionStageTime) - 1);
		}
	}
	else
	{
		RoundElapsedTime++;

		if (!GS->bActivateXRayVision && bXRayBreaker && RoundElapsedTime >= 70)
		{
			GS->bActivateXRayVision = true;
			GS->OnRep_XRayVision();
		}

		Super::DefaultTimer();
	}
}

void AUTShowdownGame::AddDamagePing(AUTCharacter* Char, uint8 VisibleToTeam, float Lifetime)
{
	if (VisibleToTeam < 8)
	{
		bool bFoundExisting = false;
		for (FTimedPlayerPing& Existing : DamagePings)
		{
			if (Existing.Char == Char && Existing.VisibleTeamNum == VisibleToTeam)
			{
				Existing.TimeRemaining = FMath::Max<float>(Existing.TimeRemaining, Lifetime);
				bFoundExisting = true;
				break;
			}
		}
		if (!bFoundExisting)
		{
			new(DamagePings) FTimedPlayerPing(Char, VisibleToTeam, Lifetime);
			Char->SetOutlineServer(true, false, 1 << VisibleToTeam);
		}
	}
}

void AUTShowdownGame::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = DamagePings.Num() - 1; i >= 0; i--)
	{
		DamagePings[i].TimeRemaining -= DeltaTime;
		if (DamagePings[i].Char == nullptr || DamagePings[i].Char->IsDead() || DamagePings[i].Char->IsActorBeingDestroyed() || DamagePings[i].TimeRemaining <= 0.0f)
		{
			if (DamagePings[i].Char != nullptr)
			{
				DamagePings[i].Char->SetOutlineServer(false, false, 1 << DamagePings[i].VisibleTeamNum);
			}
			DamagePings.RemoveAt(i);
		}
	}
}

void AUTShowdownGame::CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps)
{
	MenuProps.Empty();
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &TimeLimit, TEXT("TimeLimit"))));
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &GoalScore, TEXT("GoalScore"))));
	MenuProps.Add(MakeShareable(new TAttributePropertyBool(this, &bPowerupBreaker, TEXT("PowerupBreaker"))));
	MenuProps.Add(MakeShareable(new TAttributePropertyBool(this, &bXRayBreaker, TEXT("XRayBreaker"))));
}

void AUTShowdownGame::UpdateSkillRating()
{
	if (bRankedSession)
	{
		ReportRankedMatchResults(GetRankedLeagueName());
	}
	else
	{
		ReportRankedMatchResults(NAME_ShowdownSkillRating.ToString());
	}
}

FString AUTShowdownGame::GetRankedLeagueName()
{
	return NAME_RankedShowdownSkillRating.ToString();
}

uint8 AUTShowdownGame::GetNumMatchesFor(AUTPlayerState* PS, bool InbRankedSession) const
{
	if (PS && InbRankedSession)
	{
		return PS->RankedShowdownMatchesPlayed;
	}

	return PS ? PS->ShowdownMatchesPlayed : 0;
}

int32 AUTShowdownGame::GetEloFor(AUTPlayerState* PS, bool InbRankedSession) const
{
	if (PS && InbRankedSession)
	{
		return PS->RankedShowdownRank;
	}

	return PS ? PS->ShowdownRank : Super::GetEloFor(PS, InbRankedSession);
}

void AUTShowdownGame::SetEloFor(AUTPlayerState* PS, bool InbRankedSession, int32 NewEloValue, bool bIncrementMatchCount)
{
	if (PS)
	{
		if (InbRankedSession)
		{
			PS->RankedShowdownRank = NewEloValue;
			if (bIncrementMatchCount && (PS->ShowdownMatchesPlayed < 255))
			{
				PS->RankedShowdownMatchesPlayed++;
			}
			
		}
		else
		{
			PS->ShowdownRank = NewEloValue;
			if (bIncrementMatchCount && (PS->ShowdownMatchesPlayed < 255))
			{
				PS->ShowdownMatchesPlayed++;
			}
		}
	}
}

#if !UE_SERVER
void AUTShowdownGame::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers)
{
	// warning: might have been called by subclass!
	if (ConfigProps.Num() == 0)
	{
		CreateGameURLOptions(ConfigProps);
	}

	TSharedPtr< TAttributeProperty<int32> > TimeLimitAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps, TEXT("TimeLimit")));
	TSharedPtr< TAttributeProperty<int32> > GoalScoreAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps, TEXT("GoalScore")));
	TSharedPtr< TAttributePropertyBool > PowerupBreakerAttr = StaticCastSharedPtr<TAttributePropertyBool>(FindGameURLOption(ConfigProps, TEXT("PowerupBreaker")));
	TSharedPtr< TAttributePropertyBool > XRayBreakerAttr = StaticCastSharedPtr<TAttributePropertyBool>(FindGameURLOption(ConfigProps, TEXT("XRayBreaker")));

	if (GoalScoreAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.Padding(0.0f, 0.0f, 0.0f, 5.0f)
		.AutoHeight()
		.VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("UTGameMode", "GoalScore", "Goal Score"))
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f, 0.0f, 0.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween.Bold")
					.Text(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
					SNew(SNumericEntryBox<int32>)
					.Value(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
					.OnValueChanged(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
					.AllowSpin(true)
					.Delta(1)
					.MinValue(0)
					.MaxValue(999)
					.MinSliderValue(0)
					.MaxSliderValue(99)
					.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
					)
				]
			]
		];
	}
	if (TimeLimitAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.Padding(0.0f, 0.0f, 0.0f, 5.0f)
		.AutoHeight()
		.VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
					.Text(NSLOCTEXT("UTGameMode", "RoundTimeLimit", "Round Time Limit"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f, 0.0f, 0.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween.Bold")
					.Text(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
					SNew(SNumericEntryBox<int32>)
					.Value(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
					.OnValueChanged(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
					.AllowSpin(true)
					.Delta(1)
					.MinValue(0)
					.MaxValue(999)
					.MinSliderValue(0)
					.MaxSliderValue(60)
					.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
					)
				]
			]
		];
	}
	MenuSpace->AddSlot()
	.Padding(0.0f, 0.0f, 0.0f, 5.0f)
	.AutoHeight()
	.VAlign(VAlign_Top)
	[
		SNew(STextBlock)
		.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
		.Text(NSLOCTEXT("UTGameMode", "EXPERIMENTAL", "--- Experimental Options ---"))
	];
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f, 0.0f, 0.0f, 5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
				.Text(NSLOCTEXT("UTGameMode", "PowerupBreaker", "Spawn Overcharge Powerup after 70s"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(20.0f, 0.0f, 0.0f, 10.0f)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
				SNew(SCheckBox)
				.IsChecked(PowerupBreakerAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
				.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
				.ForegroundColor(FLinearColor::White)
				.Type(ESlateCheckBoxType::CheckBox)
				) :
				StaticCastSharedRef<SWidget>(
				SNew(SCheckBox)
				.IsChecked(PowerupBreakerAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
				.OnCheckStateChanged(PowerupBreakerAttr.ToSharedRef(), &TAttributePropertyBool::SetFromCheckBox)
				.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
				.ForegroundColor(FLinearColor::White)
				.Type(ESlateCheckBoxType::CheckBox)
				)
			]
		]
	];
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f, 0.0f, 0.0f, 5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
				.Text(NSLOCTEXT("UTGameMode", "XRayBreaker", "Get XRay Vision at 70s"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(20.0f, 0.0f, 0.0f, 10.0f)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
				SNew(SCheckBox)
				.IsChecked(XRayBreakerAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
				.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
				.ForegroundColor(FLinearColor::White)
				.Type(ESlateCheckBoxType::CheckBox)
				) :
				StaticCastSharedRef<SWidget>(
				SNew(SCheckBox)
				.IsChecked(XRayBreakerAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
				.OnCheckStateChanged(XRayBreakerAttr.ToSharedRef(), &TAttributePropertyBool::SetFromCheckBox)
				.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
				.ForegroundColor(FLinearColor::White)
				.Type(ESlateCheckBoxType::CheckBox)
				)
			]
		]
	];
}
#endif