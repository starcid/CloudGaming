// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "Net/UnrealNetwork.h"
#include "UTCTFScoring.h"
#include "StatNames.h"
#include "UTGameVolume.h"
#include "UTCTFMajorMessage.h"
#include "UTLineUpHelper.h"

AUTCTFGameState::AUTCTFGameState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bSecondHalf = false;
	bIsAtIntermission = false;
	HalftimeScoreDelay = 3.5f;
	GoalScoreText = NSLOCTEXT("UTScoreboard", "CTFGoalScoreFormat", "{0} Caps");
	
	GameScoreStats.Add(NAME_RegularKillPoints);
	GameScoreStats.Add(NAME_FCKills);
	GameScoreStats.Add(NAME_FCKillPoints);
	GameScoreStats.Add(NAME_FlagSupportKills);
	GameScoreStats.Add(NAME_FlagSupportKillPoints);
	GameScoreStats.Add(NAME_EnemyFCDamage);
	GameScoreStats.Add(NAME_FlagHeldDeny);
	GameScoreStats.Add(NAME_FlagHeldDenyTime);
	GameScoreStats.Add(NAME_FlagHeldTime);
	GameScoreStats.Add(NAME_FlagReturnPoints);
	GameScoreStats.Add(NAME_CarryAssist);
	GameScoreStats.Add(NAME_CarryAssistPoints);
	GameScoreStats.Add(NAME_FlagCapPoints);
	GameScoreStats.Add(NAME_DefendAssist);
	GameScoreStats.Add(NAME_DefendAssistPoints);
	GameScoreStats.Add(NAME_ReturnAssist);
	GameScoreStats.Add(NAME_ReturnAssistPoints);
	GameScoreStats.Add(NAME_TeamCapPoints);
	GameScoreStats.Add(NAME_FlagGrabs);

	TeamStats.Add(NAME_TeamFlagGrabs);
	TeamStats.Add(NAME_TeamFlagHeldTime);

	SecondaryAttackerStat = NAME_FlagHeldTime;

	HighlightMap.Add(HighlightNames::TopFlagCapturesRed, NSLOCTEXT("AUTGameMode", "TopFlagCapturesRed", "Most Flag Caps for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagCapturesBlue, NSLOCTEXT("AUTGameMode", "TopFlagCapturesBlue", "Most Flag Caps for Blue with {0}."));
	HighlightMap.Add(HighlightNames::TopAssistsRed, NSLOCTEXT("AUTGameMode", "TopAssistsRed", "Most Assists for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopAssistsBlue , NSLOCTEXT("AUTGameMode", "TopAssistsBlue", "Most Assists for Blue with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagReturnsRed, NSLOCTEXT("AUTGameMode", "TopFlagReturnsRed", "Most Flag Returns for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagReturnsBlue, NSLOCTEXT("AUTGameMode", "TopFlagReturnsBlue", "Most Flag Returns for Blue with {0}."));

	HighlightMap.Add(NAME_FCKills, NSLOCTEXT("AUTGameMode", "FCKills", "Killed Enemy Flag Carrier ({0})."));
	HighlightMap.Add(NAME_FlagGrabs, NSLOCTEXT("AUTGameMode", "FlagGrabs", "Grabbed Enemy Flag {0} times."));
	HighlightMap.Add(NAME_FlagSupportKills, NSLOCTEXT("AUTGameMode", "FlagSupportKills", "Killed Enemy chasing Flag Carrier ({0})."));
	HighlightMap.Add(HighlightNames::FlagCaptures, NSLOCTEXT("AUTGameMode", "FlagCaptures", "Captured Flag ({0})."));
	HighlightMap.Add(HighlightNames::Assists, NSLOCTEXT("AUTGameMode", "Assists", "Assisted Flag Capture ({0})."));
	HighlightMap.Add(HighlightNames::FlagReturns, NSLOCTEXT("AUTGameMode", "FlagReturns", "Returned Flag ({0})."));

	ShortHighlightMap.Add(HighlightNames::TopFlagCapturesRed, NSLOCTEXT("AUTGameMode", "ShortTopFlagCapturesRed", "Most Flag Caps for Red"));
	ShortHighlightMap.Add(HighlightNames::TopFlagCapturesBlue, NSLOCTEXT("AUTGameMode", "ShortTopFlagCapturesBlue", "Most Flag Caps for Blue"));
	ShortHighlightMap.Add(HighlightNames::TopAssistsRed, NSLOCTEXT("AUTGameMode", "ShortTopAssistsRed", "Most Assists for Red"));
	ShortHighlightMap.Add(HighlightNames::TopAssistsBlue, NSLOCTEXT("AUTGameMode", "ShortTopAssistsBlue", "Most Assists for Blue"));
	ShortHighlightMap.Add(HighlightNames::TopFlagReturnsRed, NSLOCTEXT("AUTGameMode", "ShortTopFlagReturnsRed", "Most Flag Returns for Red"));
	ShortHighlightMap.Add(HighlightNames::TopFlagReturnsBlue, NSLOCTEXT("AUTGameMode", "ShortTopFlagReturnsBlue", "Most Flag Returns for Blue"));

	ShortHighlightMap.Add(NAME_FCKills, NSLOCTEXT("AUTGameMode", "ShortFCKills", "{0} Enemy Flag Carrier Kills"));
	ShortHighlightMap.Add(NAME_FlagGrabs, NSLOCTEXT("AUTGameMode", "ShortFlagGrabs", "{0} Flag Grabs"));
	ShortHighlightMap.Add(NAME_FlagSupportKills, NSLOCTEXT("AUTGameMode", "ShortFlagSupportKills", "Killed Enemy chasing Flag Carrier"));
	ShortHighlightMap.Add(HighlightNames::FlagCaptures, NSLOCTEXT("AUTGameMode", "ShortFlagCaptures", "{0} Flag Captures"));
	ShortHighlightMap.Add(HighlightNames::Assists, NSLOCTEXT("AUTGameMode", "ShortAssists", "{0} Assists"));
	ShortHighlightMap.Add(HighlightNames::FlagReturns, NSLOCTEXT("AUTGameMode", "ShortFlagReturns", "{0} Flag Returns"));

	HighlightPriority.Add(HighlightNames::TopFlagCapturesRed, 4.5f);
	HighlightPriority.Add(HighlightNames::TopFlagCapturesBlue, 4.5f);
	HighlightPriority.Add(HighlightNames::TopAssistsRed, 3.5f);
	HighlightPriority.Add(HighlightNames::TopAssistsRed, 3.5f);
	HighlightPriority.Add(HighlightNames::TopFlagReturnsRed, 3.3f);
	HighlightPriority.Add(HighlightNames::TopFlagReturnsBlue, 3.3f);
	HighlightPriority.Add(NAME_FCKills, 3.5f);
	HighlightPriority.Add(NAME_FlagGrabs, 1.5f);
	HighlightPriority.Add(NAME_FlagSupportKills, 2.5f);
	HighlightPriority.Add(HighlightNames::FlagCaptures, 3.5f);
	HighlightPriority.Add(HighlightNames::Assists, 3.f);
	HighlightPriority.Add(HighlightNames::FlagReturns, 1.9f);

	RedAdvantageStatus = NSLOCTEXT("UTCTFGameState", "RedAdvantage", "Red Advantage");
	BlueAdvantageStatus = NSLOCTEXT("UTCTFGameState", "BlueAdvantage", "Blue Advantage");
	RoundInProgressStatus = NSLOCTEXT("UTCharacter", "CTFRoundDisplay", "Round {RoundNum}");
	FullRoundInProgressStatus = NSLOCTEXT("UTCharacter", "CTFRoundDisplay", "Round {RoundNum} / {NumRounds}");
	IntermissionStatus = NSLOCTEXT("UTCTFGameState", "Intermission", "Intermission");
	HalftimeStatus = NSLOCTEXT("UTCTFGameState", "HalfTime", "HalfTime");
	ExtendedOvertimeStatus = NSLOCTEXT("UTCTFGameState", "ExtendedOvertime", "Extra Overtime");
	FirstHalfStatus = NSLOCTEXT("UTCTFGameState", "FirstHalf", "First Half");
	SecondHalfStatus = NSLOCTEXT("UTCTFGameState", "SecondHalf", "Second Half");
}

void AUTCTFGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTCTFGameState, bSecondHalf);
	DOREPLIFETIME(AUTCTFGameState, bIsAtIntermission);
	DOREPLIFETIME(AUTCTFGameState, FlagBases);
	DOREPLIFETIME(AUTCTFGameState, bPlayingAdvantage);
	DOREPLIFETIME(AUTCTFGameState, AdvantageTeamIndex);
	DOREPLIFETIME(AUTCTFGameState, ScoringPlays);
	DOREPLIFETIME(AUTCTFGameState, CTFRound); 
	DOREPLIFETIME(AUTCTFGameState, NumRounds);
	DOREPLIFETIME(AUTCTFGameState, IntermissionTime);
}

void AUTCTFGameState::SetMaxNumberOfTeams(int32 TeamCount)
{
	for (int32 TeamIdx = 0; TeamIdx < TeamCount; TeamIdx++)
	{
		FlagBases.Add(NULL);
	}
}

void AUTCTFGameState::CacheFlagBase(AUTCTFFlagBase* BaseToCache)
{
	uint8 TeamNum = BaseToCache->GetTeamNum();
	if (FlagBases.IsValidIndex(TeamNum))
	{
		FlagBases[TeamNum] = BaseToCache;
	}
}

float AUTCTFGameState::GetClockTime()
{
	if (IsMatchIntermission())
	{
		return IntermissionTime;
	}
	else if (IsMatchInOvertime())
	{
		return ElapsedTime - OvertimeStartTime;
	}
	return ((TimeLimit > 0.f) || !HasMatchStarted()) ? GetRemainingTime() : ElapsedTime;
}

void AUTCTFGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	//Make sure the timers are cleared since advantage-time may have been counting down
	if (IsMatchInOvertime())
	{
		OvertimeStartTime = ElapsedTime;
		SetRemainingTime(0);
	}
}

FName AUTCTFGameState::GetFlagState(uint8 TeamNum)
{
	if (TeamNum < FlagBases.Num() && FlagBases[TeamNum] != NULL)
	{
		return FlagBases[TeamNum]->GetFlagState();
	}
	return NAME_None;
}

AUTPlayerState* AUTCTFGameState::GetFlagHolder(uint8 TeamNum)
{
	if (TeamNum < FlagBases.Num() && FlagBases[TeamNum] != NULL)
	{
		return FlagBases[TeamNum]->GetCarriedObjectHolder();
	}
	return NULL;
}

AUTCTFFlagBase* AUTCTFGameState::GetFlagBase(uint8 TeamNum)
{
	if (TeamNum < FlagBases.Num())
	{
		return FlagBases[TeamNum];
	}
	return NULL;
}

void AUTCTFGameState::ResetFlags()
{
	for (int32 i=0; i < FlagBases.Num(); i++)
	{
		if (FlagBases[i] != NULL)
		{
			FlagBases[i]->RecallFlag();
		}
	}
}

bool AUTCTFGameState::IsMatchInProgress() const
{
	const FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::InProgress || CurrentMatchState == MatchState::MatchIsInOvertime || CurrentMatchState == MatchState::MatchIntermission || CurrentMatchState == MatchState::MatchExitingIntermission);
}

bool AUTCTFGameState::IsMatchInOvertime() const
{
	const FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::MatchIsInOvertime);
}

bool AUTCTFGameState::IsMatchIntermission() const
{
	const FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::MatchIntermission) || (CurrentMatchState == MatchState::MatchIntermission || CurrentMatchState == MatchState::MatchExitingIntermission);
}

FName AUTCTFGameState::OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle)
{
	if (IsLineUpActive())
	{
		return FName(TEXT("LineUpCam"));
	}
	else
	{
		return (IsMatchIntermission() || HasMatchEnded()) ? FName(TEXT("FreeCam")) : Super::OverrideCameraStyle(PCOwner, CurrentCameraStyle);
	}
}

AUTLineUpZone* AUTCTFGameState::GetAppropriateSpawnList(LineUpTypes ZoneType)
{
	AUTLineUpZone* FoundPotentialMatch = nullptr;
	AUTCTFFlagBase* ScoringBase = GetLeadTeamFlagBase();
	
	if (GetWorld())
	{
		for (TActorIterator<AUTLineUpZone> It(GetWorld()); It; ++It)
		{
			if (It->ZoneType == ZoneType)
			{
				//Found perfect match, lets return it!
				if (It->GameObjectiveReference == ScoringBase)
				{
					return *It;
				}

				//imperfect match, but it might be all we have
				else if (It->GameObjectiveReference == nullptr)
				{
					FoundPotentialMatch = *It;
				}
			}
		}
	}

	return (FoundPotentialMatch == nullptr) ? Super::GetAppropriateSpawnList(ZoneType) : FoundPotentialMatch;
}

void AUTCTFGameState::DefaultTimer()
{
	Super::DefaultTimer();
	if (bIsAtIntermission)
	{
		IntermissionTime--;
	}
}

float AUTCTFGameState::GetIntermissionTime()
{
	return IntermissionTime;
}

void AUTCTFGameState::SpawnDefaultLineUpZones()
{
	if (GetAppropriateSpawnList(LineUpTypes::Intro) == nullptr)
	{
		if (FlagBases.Num() > 0)
		{
			SpawnLineUpZoneOnFlagBase(FlagBases[0], LineUpTypes::Intro);
		}
	}

	if (GetAppropriateSpawnList(LineUpTypes::Intermission) == nullptr)
	{
		for (AUTCTFFlagBase* FlagBase : FlagBases)
		{
			SpawnLineUpZoneOnFlagBase(FlagBase, LineUpTypes::Intermission);
		}
	}

	if (GetAppropriateSpawnList(LineUpTypes::PostMatch) == nullptr)
	{
		for (AUTCTFFlagBase* FlagBase : FlagBases)
		{
			SpawnLineUpZoneOnFlagBase(FlagBase, LineUpTypes::PostMatch);
		}
	}

	Super::SpawnDefaultLineUpZones();
}

void AUTCTFGameState::SpawnLineUpZoneOnFlagBase(AUTCTFFlagBase* BaseToSpawnOn, LineUpTypes TypeToSpawn)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));

	if (BaseToSpawnOn)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = BaseToSpawnOn;

		AUTLineUpZone* NewZone = GetWorld()->SpawnActor<AUTLineUpZone>(AUTLineUpZone::StaticClass(), SpawnParams);
		NewZone->ZoneType = TypeToSpawn;
		NewZone->bIsTeamSpawnList = true;
		NewZone->bSnapToFloor = false;

		NewZone->AttachToActor(BaseToSpawnOn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		NewZone->SetActorRelativeLocation(FVector(0.0f, 0.0f, NewZone->SnapFloorOffset));

		if (TypeToSpawn == LineUpTypes::Intro)
		{
			NewZone->DefaultCreateForIntro();
		}
		else if (TypeToSpawn == LineUpTypes::Intermission)
		{
			NewZone->DefaultCreateForIntermission();
		}
		else if (TypeToSpawn == LineUpTypes::PostMatch)
		{
			NewZone->DefaultCreateForEndMatch();
		}
		
		//See if the new zone's camera is stuck inside of a wall
		if (GetWorld())
		{
			FHitResult CameraCollision;
			FCollisionQueryParams Params(NAME_FreeCam, false, this);
			Params.AddIgnoredActor(NewZone);
			Params.AddIgnoredActor(BaseToSpawnOn);
			
			GetWorld()->SweepSingleByChannel(CameraCollision, NewZone->GetActorLocation(), NewZone->Camera->GetComponentLocation(), FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeBox(FVector(12.f)),Params);

			if (CameraCollision.bBlockingHit)
			{
				NewZone->Camera->SetWorldLocation(CameraCollision.ImpactPoint);
			}
		}

		NewZone->GameObjectiveReference = BaseToSpawnOn;

		SpawnedLineUps.Add(NewZone);
	}
}

AUTCTFFlagBase* AUTCTFGameState::GetLeadTeamFlagBase()
{
	AUTTeamInfo* LeadTeamInfo = FindLeadingTeam();
	if (LeadTeamInfo && LeadTeamInfo->GetTeamNum() < FlagBases.Num())
	{
		return FlagBases[LeadTeamInfo->GetTeamNum()];
	}
	else if (WinningTeam && (FlagBases.Num() > WinningTeam->GetTeamNum()))
	{
		return FlagBases[WinningTeam->GetTeamNum()];
	}
	else if (ScoringPlayerState && (FlagBases.Num() > ScoringPlayerState->GetTeamNum()))
	{
		return FlagBases[ScoringPlayerState->GetTeamNum()];
	}
	else if (ScoringPlays.Num() > 0)
	{
		const FCTFScoringPlay& WinningPlay = ScoringPlays.Last();

		if (WinningPlay.Team && (FlagBases.Num() > WinningPlay.Team->GetTeamNum()))
		{
			return FlagBases[WinningPlay.Team->GetTeamNum()];
		}
	}

	return nullptr;
}

void AUTCTFGameState::AddScoringPlay(const FCTFScoringPlay& NewScoringPlay)
{
	if (Role == ROLE_Authority)
	{
		ScoringPlays.AddUnique(NewScoringPlay);
	}
}

FText AUTCTFGameState::GetGameStatusText(bool bForScoreboard)
{
	if (HasMatchEnded())
	{
		return GameOverStatus;
	}
	else if (GetMatchState() == MatchState::MapVoteHappening)
	{
		return MapVoteStatus;
	}
	else if (bPlayingAdvantage)
	{
		if (AdvantageTeamIndex == 0)
		{
			return RedAdvantageStatus;
		}
		else
		{
			return BlueAdvantageStatus;
		}
	}
	else if (IsMatchIntermission())
	{
		return bSecondHalf ? IntermissionStatus : HalftimeStatus;
	}
	else if (IsMatchInProgress())
	{
		if (IsMatchInOvertime())
		{
			return (ElapsedTime - OvertimeStartTime < TimeLimit) ? OvertimeStatus : ExtendedOvertimeStatus;
		}
		if (TimeLimit == 0)
		{
			return FText::GetEmpty();
		}

		return bSecondHalf ? SecondHalfStatus : FirstHalfStatus;
	}

	return Super::GetGameStatusText(bForScoreboard);
}

float AUTCTFGameState::ScoreCameraView(AUTPlayerState* InPS, AUTCharacter *Character)
{
	// bonus score to player near but not holding enemy flag
	if (InPS && Character && !InPS->CarriedObject && InPS->Team && (InPS->Team->GetTeamNum() < 2))
	{
		uint8 EnemyTeamNum = 1 - InPS->Team->GetTeamNum();
		AUTFlag* EnemyFlag = FlagBases[EnemyTeamNum] ? FlagBases[EnemyTeamNum]->MyFlag : NULL;
		if (EnemyFlag && ((EnemyFlag->GetActorLocation() - Character->GetActorLocation()).Size() < FlagBases[EnemyTeamNum]->LastSecondSaveDistance))
		{
			float MaxScoreDist = FlagBases[EnemyTeamNum]->LastSecondSaveDistance;
			return FMath::Clamp(10.f * (MaxScoreDist - (EnemyFlag->GetActorLocation() - Character->GetActorLocation()).Size()) / MaxScoreDist, 0.f, 10.f);
		}
	}
	return 0.f;
}

uint8 AUTCTFGameState::NearestTeamSide(AActor* InActor)
{
	if (FlagBases.Num() > 1)
	{
		// if there is only one of this pickup, return 255
		bool bFoundAnother = false;
		AUTPickupInventory* InPickupInventory = Cast<AUTPickupInventory>(InActor);
		if (InPickupInventory)
		{
			for (FActorIterator It(GetWorld()); It; ++It)
			{
				AUTPickupInventory* PickupInventory = Cast<AUTPickupInventory>(*It);
				if (PickupInventory && (PickupInventory != InPickupInventory) && (PickupInventory->GetInventoryType() == InPickupInventory->GetInventoryType()))
				{
					bFoundAnother = true;
					break;
				}
			}
		}
		if (bFoundAnother)
		{
			float DistDiff = (InActor->GetActorLocation() - FlagBases[0]->GetActorLocation()).Size() - (InActor->GetActorLocation() - FlagBases[1]->GetActorLocation()).Size();
			return (DistDiff < 0) ? 0 : 1;
		}
	}
	return 255;
}

bool AUTCTFGameState::GetImportantPickups_Implementation(TArray<AUTPickup*>& PickupList)
{
	Super::GetImportantPickups_Implementation(PickupList);
	TMap<UClass*, TArray<AUTPickup*> > PickupGroups;

	//Collect the Powerups without bOverride_TeamSide and group by class
	for (AUTPickup* Pickup : PickupList)
	{
		if (!Pickup->bOverride_TeamSide)
		{
			UClass* PickupClass = (Cast<AUTPickupInventory>(Pickup) != nullptr) ? *Cast<AUTPickupInventory>(Pickup)->GetInventoryType() : Pickup->GetClass();
			TArray<AUTPickup*>& PickupGroup = PickupGroups.FindOrAdd(PickupClass);
			PickupGroup.Add(Pickup);
		}
	}

	//Auto get the TeamSide
	if (FlagBases.Num() > 1)
	{
		for (auto& Pair : PickupGroups)
		{
			TArray<AUTPickup*>& PickupGroup = Pair.Value;

			//Find the midfield pickup for an odd number of pickups per group
			if (PickupGroup.Num() % 2 != 0 && PickupGroup.Num() > 2)
			{
				AUTPickup* MidfieldPickup = nullptr;
				float FarthestDist = 0.0;

				//Find the furthest pickup that would've been returned by NearestTeamSide()
				for (AUTPickup* Pickup : PickupGroup)
				{
					float ClosestFlagDist = MAX_FLT;
					for (AUTCTFFlagBase* Flag : FlagBases)
					{
						if (Flag != nullptr)
						{
							ClosestFlagDist = FMath::Min(ClosestFlagDist, FVector::Dist(Pickup->GetActorLocation(), Flag->GetActorLocation()));
						}
					}

					if (FarthestDist < ClosestFlagDist)
					{
						MidfieldPickup = Pickup;
						FarthestDist = ClosestFlagDist;
					}
				}

				if (MidfieldPickup != nullptr)
				{
					MidfieldPickup->TeamSide = 255;
				}
			}
		}
	}

	//Sort the list by team and by respawn time 
	//TODO: powerup priority so different armors sort properly
	PickupList.Sort([](const AUTPickup& A, const AUTPickup& B) -> bool
	{
		return A.TeamSide > B.TeamSide || (A.TeamSide == B.TeamSide && A.RespawnTime > B.RespawnTime);
	});

	return true;
}

void AUTCTFGameState::UpdateHighlights_Implementation()
{
	AUTPlayerState* TopFlagCaps[2] = { NULL, NULL };
	AUTPlayerState* TopAssists[2] = { NULL, NULL };
	AUTPlayerState* TopFlagReturns[2] = { NULL, NULL };

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
			if (PS->FlagCaptures > (TopFlagCaps[TeamIndex] ? TopFlagCaps[TeamIndex]->FlagCaptures : 0))
			{
				TopFlagCaps[TeamIndex] = PS;
			}
			if (PS->Assists > (TopAssists[TeamIndex] ? TopAssists[TeamIndex]->Assists : 0))
			{
				TopAssists[TeamIndex] = PS;
			}
			if (PS->FlagReturns > (TopFlagReturns[TeamIndex] ? TopFlagReturns[TeamIndex]->FlagReturns : 0))
			{
				TopFlagReturns[TeamIndex] = PS;
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
			if ((TopFlagCaps[TeamIndex] != NULL) && (PS->FlagCaptures == TopFlagCaps[TeamIndex]->FlagCaptures))
			{
				PS->AddMatchHighlight((TeamIndex == 0) ? HighlightNames::TopFlagCapturesRed : HighlightNames::TopFlagCapturesBlue, PS->FlagCaptures);
			}
			else if (PS->FlagCaptures > 0)
			{
				PS->AddMatchHighlight(HighlightNames::FlagCaptures, PS->FlagCaptures);
			}
			if ((TopAssists[TeamIndex] != NULL) && (PS->Assists == TopAssists[TeamIndex]->Assists))
			{
				PS->AddMatchHighlight((TeamIndex == 0) ? HighlightNames::TopAssistsRed : HighlightNames::TopAssistsBlue, PS->Assists);
			}
			else if (PS->Assists > 0)
			{
				PS->AddMatchHighlight(HighlightNames::Assists, PS->Assists);
			}
			if ((TopFlagReturns[TeamIndex] != NULL) && (PS->FlagReturns == TopFlagReturns[TeamIndex]->FlagReturns))
			{
				PS->AddMatchHighlight((TeamIndex == 0) ? HighlightNames::TopFlagReturnsRed : HighlightNames::TopFlagReturnsBlue, PS->FlagReturns);
			}
			else if (PS->FlagReturns > 0)
			{
				PS->AddMatchHighlight(HighlightNames::FlagReturns, PS->FlagReturns);
			}
		}
	}

	Super::UpdateHighlights_Implementation();
}

void AUTCTFGameState::AddMinorHighlights_Implementation(AUTPlayerState* PS)
{
	// skip if already filled with major highlights
	if (PS->MatchHighlights[3] != NAME_None)
	{
		return;
	}

	if (PS->GetStatsValue(NAME_FCKills) > 0)
	{
		PS->AddMatchHighlight(NAME_FCKills, PS->GetStatsValue(NAME_FCKills));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
	Super::AddMinorHighlights_Implementation(PS);
	if (PS->MatchHighlights[3] != NAME_None)
	{
		return;
	}

	if (PS->GetStatsValue(NAME_FlagGrabs) > 0)
	{
		PS->AddMatchHighlight(NAME_FlagGrabs, PS->GetStatsValue(NAME_FlagGrabs));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
	if (PS->GetStatsValue(NAME_FlagSupportKills) > 0)
	{
		PS->AddMatchHighlight(NAME_FlagSupportKills, PS->GetStatsValue(NAME_FlagSupportKills));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
}
