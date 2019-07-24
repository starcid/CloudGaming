// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRconAdminInfo.h"
#include "Net/UnrealNetwork.h"
#include "UTGameSessionNonRanked.h"


AUTRconAdminInfo::AUTRconAdminInfo(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	bReplicateMovement = false;
	bNetLoadOnClient = false;
}

void AUTRconAdminInfo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTRconAdminInfo, PlayerData);
	DOREPLIFETIME(AUTRconAdminInfo, BanData);
}

void AUTRconAdminInfo::BeginPlay()
{
	if (Role == ROLE_Authority)
	{
		UpdateList();
		GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &AUTRconAdminInfo::UpdateList, 2.0, true);

		AUTBaseGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
		if (GameMode)
		{
			AUTGameSessionNonRanked* GameSession = Cast<AUTGameSessionNonRanked>(GameMode->GameSession);
			if (GameSession)
			{
				BanData = GameSession->BannedUsers;			
			}
		}

	}

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetOwner());
		if (PC)
		{
			PC->ShowAdminDialog(this);
		}
	}
}

void AUTRconAdminInfo::UpdateList()
{
	if (Role == ROLE_Authority)
	{
		AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
		if (GameState)
		{
			GameState->FillOutRconPlayerList(PlayerData);

			for (int32 i = PlayerData.Num() - 1; i >= 0; i--)
			{
				if (PlayerData[i].bPendingDelete)
				{
					PlayerData.RemoveAt(i,1);
				}
			}
		}
	}
}


void AUTRconAdminInfo::NoLongerNeeded()
{
	PlayerData.Empty();
	ServerNoLongerNeeded();
}

bool AUTRconAdminInfo::ServerNoLongerNeeded_Validate() { return true; }
void AUTRconAdminInfo::ServerNoLongerNeeded_Implementation()
{
	Destroy();
}
