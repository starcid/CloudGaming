// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTTeamInfo.h"
#include "UnrealNetwork.h"
#include "UTBot.h"
#include "UTSquadAI.h"
#include "StatNames.h"

AUTTeamInfo::AUTTeamInfo(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetReplicates(true);
	bAlwaysRelevant = true;
	NetUpdateFrequency = 1.0f;
	TeamIndex = 255; // invalid so we will always get ReceivedTeamIndex() on clients
	TeamColor = FLinearColor::White;
	DefaultOrderIndex = -1;
	DefaultOrders.Add(NAME_Attack);
	DefaultOrders.Add(NAME_Defend);
	TopAttacker = NULL;
	TopDefender = NULL;
	TopSupporter = NULL;
}

void AUTTeamInfo::BeginPlay()
{
	Super::BeginPlay();

	if (!IsPendingKillPending())
	{
		FTimerHandle TempHandle;
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTTeamInfo::UpdateTeamLeaders, 3.f, true);
		}
	}
}

void AUTTeamInfo::Destroyed()
{
	Super::Destroyed();
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

int32 AUTTeamInfo::AverageEloFor(AUTGameMode* GameMode)
{
	if (GameMode)
	{
		int32 TeamElo = 0;
		int32 EloCount = 0;
		for (AController* C : TeamMembers)
		{
			AUTPlayerState* TeamPS = Cast<AUTPlayerState>(C->PlayerState);
			if (TeamPS)
			{
				int32 NewElo = GameMode->IsValidElo(TeamPS, GameMode->bRankedSession) ? GameMode->GetEloFor(TeamPS, GameMode->bRankedSession) : NEW_USER_ELO;
				EloCount++;
				TeamElo += NewElo;
			}
		}
		return (EloCount > 0) ? TeamElo / EloCount : NEW_USER_ELO;
	}

	return 0;
}

AController* AUTTeamInfo::MemberClosestToElo(class AUTGameMode* GameMode, int32 DesiredElo)
{
	AController* BestMatch = nullptr;
	int32 BestDiff = 0;
	for (AController* C : TeamMembers)
	{
		AUTPlayerState* TeamPS = Cast<AUTPlayerState>(C->PlayerState);
		if (TeamPS)
		{
			int32 NewElo = GameMode->IsValidElo(TeamPS, GameMode->bRankedSession) ? GameMode->GetEloFor(TeamPS, GameMode->bRankedSession) : NEW_USER_ELO;
			int32 NewEloDiff = FMath::Abs(DesiredElo - NewElo);
			if (!BestMatch || (NewEloDiff < BestDiff))
			{
				BestMatch = C;
				BestDiff = NewEloDiff;
			}
		}
	}
	return BestMatch ? BestMatch : TeamMembers[0];
}


void AUTTeamInfo::UpdateTeamLeaders()
{
	TArray<AUTPlayerState*> MemberPS;

	//Update scores for all player states. Including InactivePRIs
	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = (*It);
		if (PS->GetTeamNum() == GetTeamNum())
		{
			PS->AttackerScore = PS->GetStatsValue(NAME_AttackerScore);
			PS->DefenderScore = PS->GetStatsValue(NAME_DefenderScore);
			PS->SupporterScore = PS->GetStatsValue(NAME_SupporterScore);

			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			if ((GS != NULL) && (GS->SecondaryAttackerStat != NAME_None))
			{
				PS->SecondaryAttackerScore = PS->GetStatsValue(GS->SecondaryAttackerStat);
			}

			MemberPS.Add(PS);
		}
	}

	if (MemberPS.Num() == 0)
	{
		return;
	}

	MemberPS.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
	{
		return A.AttackerScore > B.AttackerScore;
	});
	TopAttacker = (MemberPS[0] && (MemberPS[0]->AttackerScore > 0)) ? MemberPS[0] : NULL;
	if (!TopAttacker)
	{
		// @TODO FIXMESTEVE this is CTF ONLY - award to most flag time
		MemberPS.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
		{
			return A.SecondaryAttackerScore > B.SecondaryAttackerScore;
		});
		TopAttacker = (MemberPS[0] && (MemberPS[0]->SecondaryAttackerScore > 0)) ? MemberPS[0] : NULL;
	}

	MemberPS.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
	{
		return A.DefenderScore > B.DefenderScore;
	});
	TopDefender = (MemberPS[0] && (MemberPS[0]->DefenderScore > 0)) ? MemberPS[0] : NULL;

	MemberPS.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
	{
		return A.SupporterScore > B.SupporterScore;
	});
	TopSupporter = (MemberPS[0] && (MemberPS[0]->SupporterScore > 0)) ? MemberPS[0] : NULL;
}

void AUTTeamInfo::AddToTeam(AController* C)
{
	if (C != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
		if (PS != NULL)
		{
			if (PS->Team != NULL)
			{
				RemoveFromTeam(C);
			}
			// allocate team specific spectating ID
			TArray<AController*> MembersCopy = TeamMembers;
			MembersCopy.Sort([](const AController& A, const AController& B) -> bool
			{
				if (Cast<AUTPlayerState>(A.PlayerState) == NULL)
				{
					return false;
				}
				else if (Cast<AUTPlayerState>(B.PlayerState) == NULL)
				{
					return true;
				}
				else
				{
					return ((AUTPlayerState*)A.PlayerState)->SpectatingIDTeam < ((AUTPlayerState*)B.PlayerState)->SpectatingIDTeam;
				}
			});
			// find first gap in assignments from player leaving, give it to this player
			// if none found, assign PlayerArray.Num() + 1
			bool bFound = false;
			for (int32 i = 0; i < MembersCopy.Num(); i++)
			{
				AUTPlayerState* OtherPS = Cast<AUTPlayerState>(MembersCopy[i]->PlayerState);
				if (OtherPS == NULL || OtherPS->SpectatingIDTeam != uint8(i + 1))
				{
					PS->SpectatingIDTeam = uint8(i + 1);
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				PS->SpectatingIDTeam = uint8(MembersCopy.Num() + 1);
			}

			PS->Team = this;
			PS->NotifyTeamChanged();
			TeamMembers.Add(C);
		}
	}
}

void AUTTeamInfo::RemoveFromTeam(AController* C)
{
	if (C != NULL && TeamMembers.Contains(C))
	{
		TeamMembers.Remove(C);
		// remove from squad
		AUTBot* B = Cast<AUTBot>(C);
		if (B != NULL && B->GetSquad() != NULL)
		{
			B->GetSquad()->RemoveMember(B);
		}
		// TODO: human player squads
		AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
		if (PS != NULL)
		{
			PS->Team = NULL;
			PS->SpectatingIDTeam = 0;
			PS->NotifyTeamChanged();
		}
	}
}

void AUTTeamInfo::RemoveSquad(AUTSquadAI* DeadSquad)
{
	if (DeadSquad != NULL && Squads.Contains(DeadSquad))
	{
		Squads.Remove(DeadSquad);
		DeadSquad->Destroy();
	}
}

void AUTTeamInfo::ReceivedTeamIndex()
{
	if (!bFromPreviousLevel && TeamIndex != 255)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS != NULL)
		{
			if (GS->Teams.Num() <= TeamIndex)
			{
				GS->Teams.SetNum(TeamIndex + 1);
			}
			GS->Teams[TeamIndex] = this;
		}
	}
}

void AUTTeamInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AUTTeamInfo, TeamName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTTeamInfo, TeamIndex, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTTeamInfo, TeamColor, COND_InitialOnly);
	DOREPLIFETIME(AUTTeamInfo, bFromPreviousLevel);
	DOREPLIFETIME(AUTTeamInfo, Score);
	DOREPLIFETIME(AUTTeamInfo, TopAttacker);
	DOREPLIFETIME(AUTTeamInfo, TopDefender);
	DOREPLIFETIME(AUTTeamInfo, TopSupporter);
}

void AUTTeamInfo::UpdateEnemyInfo(APawn* NewEnemy, EAIEnemyUpdateType UpdateType)
{
	if (NewEnemy != NULL)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS == NULL || !GS->OnSameTeam(NewEnemy, this))
		{
			bool bFound = false;
			for (int32 i = 0; i < EnemyList.Num(); i++)
			{
				if (!EnemyList[i].IsValid(this))
				{
					EnemyList.RemoveAt(i--, 1);
				}
				else if (EnemyList[i].GetPawn() == NewEnemy)
				{
					EnemyList[i].Update(UpdateType);
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				new(EnemyList) FBotEnemyInfo(NewEnemy, UpdateType);
				// tell bots on team to consider new enemy
				/* TODO: notify squads, let it decide if this enemy is worth disrupting bots for
				 TODO: enemies aren't really 'lost' from this list, so requiring enemy to be 'new' in this context isn't good enough
				for (AController* Member : TeamMembers)
				{
					AUTBot* B = Cast<AUTBot>(Member);
					if (B != NULL)
					{
						B->PickNewEnemy();
					}
				}*/
			}
		}
	}
}

bool AUTTeamInfo::AssignToSquad(AController* C, FName Orders, AController* Leader)
{
	AUTSquadAI* NewSquad = NULL;
	for (int32 i = 0; i < Squads.Num(); i++)
	{
		if (Squads[i] == NULL || Squads[i]->IsPendingKillPending())
		{
			Squads.RemoveAt(i--);
		}
		else if (Squads[i]->Orders == Orders && (Leader == NULL || Squads[i]->GetLeader() == Leader) && (Leader != NULL || Squads[i]->GetSize() < GetWorld()->GetAuthGameMode<AUTGameMode>()->MaxSquadSize))
		{
			NewSquad = Squads[i];
			break;
		}
	}
	if (NewSquad == NULL && (Leader == NULL || Leader == C))
	{
		NewSquad = GetWorld()->SpawnActor<AUTSquadAI>(GetWorld()->GetAuthGameMode<AUTGameMode>()->SquadType);
		Squads.Add(NewSquad);
		NewSquad->Initialize(this, Orders);
	}
	if (NewSquad == NULL)
	{
		return false;
	}
	else
	{
		// assign squad
		AUTBot* B = Cast<AUTBot>(C);
		if (B != NULL)
		{
			B->SetSquad(NewSquad);
		}
		else
		{
			// TODO: playercontroller
		}
		return true;
	}
}

void AUTTeamInfo::AssignDefaultSquadFor(AController* C)
{
	if (Cast<AUTBot>(C) != NULL)
	{
		if (DefaultOrders.Num() > 0)
		{
			DefaultOrderIndex = (DefaultOrderIndex + 1) % DefaultOrders.Num();
			AssignToSquad(C, DefaultOrders[DefaultOrderIndex]);
		}
		else
		{
			AssignToSquad(C, NAME_None);
		}
	}
	else
	{
		// TODO: playercontroller
	}
}

void AUTTeamInfo::NotifyObjectiveEvent(AActor* InObjective, AController* InstigatedBy, FName EventName)
{
	for (AUTSquadAI* Squad : Squads)
	{
		// by default just notify all squads and let them filter
		if (Squad != NULL && !Squad->IsPendingKillPending())
		{
			Squad->NotifyObjectiveEvent(InObjective, InstigatedBy, EventName);
		}
	}
}

void AUTTeamInfo::ReinitSquads()
{
	for (AUTSquadAI* Squad : Squads)
	{
		Squad->Initialize(this, Squad->Orders);
	}
}

float AUTTeamInfo::GetStatsValue(FName StatsName)
{
	return StatsData.FindRef(StatsName);
}

void AUTTeamInfo::SetStatsValue(FName StatsName, float NewValue)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	StatsData.Add(StatsName, NewValue);
}

void AUTTeamInfo::ModifyStatsValue(FName StatsName, float Change)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	float CurrentValue = StatsData.FindRef(StatsName);
	StatsData.Add(StatsName, CurrentValue + Change);
}

