// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTHUD_Duel.h"
#include "UTTimedPowerup.h"
#include "UTPickupWeapon.h"
#include "Slate/SlateGameResources.h"
#include "SUWindowsStyle.h"
#include "SUTStyle.h"
#include "SNumericEntryBox.h"
#include "UTDuelGame.h"
#include "StatNames.h"
#include "UTSpectatorPickupMessage.h"
#include "UTDuelSquadAI.h"
#include "UTDuelScoreboard.h"

AUTDuelGame::AUTDuelGame(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = AUTHUD_Duel::StaticClass();
	DisplayName = NSLOCTEXT("UTGameMode", "Duel", "Duel");
	GoalScore = 0;
	TimeLimit = 10;
	bForceRespawn = true;
	bAnnounceTeam = false;
	bHighScorerPerTeamBasis = false;
	bHasRespawnChoices = true;
	bWeaponStayActive = false;
	bNoDefaultLeaderHat = true;
	XPMultiplier = 7.0f;
	SquadType = AUTDuelSquadAI::StaticClass();
	bAllowAllArmorPickups = true;
	DefaultMaxPlayers = 2;
}

void AUTDuelGame::InitGameState()
{
	Super::InitGameState();

	UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState != NULL)
	{
		UTGameState->bWeaponStay = false;
		UTGameState->bAllowTeamSwitches = false;
		UTGameState->SpawnProtectionTime = 0.f;
	}
}

bool AUTDuelGame::ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast)
{
	// don't allow team changes in Duel once have initial team
	if (Player == NULL)
	{
		return false;
	}
	else
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(Player->PlayerState);
		if (PS == NULL || PS->bOnlySpectator || PS->Team)
		{
			return false;
		}
	}
	return Super::ChangeTeam(Player, NewTeam, bBroadcast);
}

bool AUTDuelGame::CheckRelevance_Implementation(AActor* Other)
{
	AUTPickupInventory* PickupInventory = Cast<AUTPickupInventory>(Other);
	if (PickupInventory)
	{
		if (Cast<AUTTimedPowerup>(PickupInventory->GetInventoryType().GetDefaultObject()) != nullptr)
		{
			PickupInventory->SetInventoryType(nullptr);
		}
		else
		{
			AUTPickupWeapon* PickupWeapon = Cast<AUTPickupWeapon>(Other);
			if (PickupWeapon != NULL && PickupWeapon->WeaponType != NULL && PickupWeapon->WeaponType.GetDefaultObject()->bMustBeHolstered)
			{
				PickupWeapon->SetInventoryType(nullptr);
			}
		}
	}

	return Super::CheckRelevance_Implementation(Other);
}

void AUTDuelGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	GameSession->MaxPlayers = DefaultMaxPlayers;
	BotFillCount = FMath::Min(BotFillCount, DefaultMaxPlayers);
	bForceRespawn = true;
	bBalanceTeams = true;
}

void AUTDuelGame::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
}

void AUTDuelGame::PlayEndOfMatchMessage()
{
	// individual winner, not team
	AUTGameMode::PlayEndOfMatchMessage();
}

void AUTDuelGame::GetGameURLOptions(const TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, TArray<FString>& OptionsList, int32& DesiredPlayerCount)
{
	Super::GetGameURLOptions(MenuProps, OptionsList, DesiredPlayerCount);
	DesiredPlayerCount = 2;
}

void AUTDuelGame::CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps)
{
	MenuProps.Empty();
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &TimeLimit, TEXT("TimeLimit"))));
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &GoalScore, TEXT("GoalScore"))));
}

#if !UE_SERVER
void AUTDuelGame::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers)
{
	CreateGameURLOptions(ConfigProps);

	TSharedPtr< TAttributeProperty<int32> > TimeLimitAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps,TEXT("TimeLimit")));
	TSharedPtr< TAttributeProperty<int32> > GoalScoreAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps,TEXT("GoalScore")));

	if (GoalScoreAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.Padding(0.0f,0.0f,0.0f,5.0f)
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
					.Text(NSLOCTEXT("UTGameMode", "GoalScore", "Goal Score"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f,0.0f,0.0f,0.0f)	
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
		.Padding(0.0f,0.0f,0.0f,5.0f)
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
					.Text(NSLOCTEXT("UTGameMode", "TimeLimit", "Time Limit"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f,0.0f,0.0f,0.0f)
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
}

#endif

void AUTDuelGame::UpdateSkillRating()
{
	if (bRankedSession)
	{
		ReportRankedMatchResults(GetRankedLeagueName());
	}
	else
	{
		ReportRankedMatchResults(NAME_SkillRating.ToString());
	}
}

FString AUTDuelGame::GetRankedLeagueName()
{
	return NAME_RankedDuelSkillRating.ToString();
}

void AUTDuelGame::FindAndMarkHighScorer()
{
	AUTGameMode::FindAndMarkHighScorer();
}

void AUTDuelGame::BroadcastSpectatorPickup(AUTPlayerState* PS, FName StatsName, UClass* PickupClass)
{
	if (PS != nullptr && PickupClass != nullptr && StatsName != NAME_None)
	{
		int32 PlayerNumPickups = (int32)PS->GetStatsValue(StatsName);
		int32 TotalPickups = (int32)UTGameState->GetStatsValue(StatsName);

		//Stats may not have been replicated to the client so pack them in the switch
		int32 Switch = TotalPickups << 16 | PlayerNumPickups;

		BroadcastSpectator(nullptr, UUTSpectatorPickupMessage::StaticClass(), Switch, PS, nullptr, PickupClass);
	}
}

uint8 AUTDuelGame::GetNumMatchesFor(AUTPlayerState* PS, bool InbRankedSession) const
{
	return PS ? PS->DuelMatchesPlayed : 0;
}

int32 AUTDuelGame::GetEloFor(AUTPlayerState* PS, bool InbRankedSession) const
{
	return PS ? PS->DuelRank : Super::GetEloFor(PS, InbRankedSession);
}

void AUTDuelGame::SetEloFor(AUTPlayerState* PS, bool InbRankedSession, int32 NewEloValue, bool bIncrementMatchCount)
{
	if (PS)
	{
		PS->DuelRank = NewEloValue;
		if (bIncrementMatchCount && (PS->DuelMatchesPlayed < 255))
		{
			PS->DuelMatchesPlayed++;
		}
	}
}

void AUTDuelGame::CheckBotCount()
{
	if (bRankedSession)
	{
		return;
	}

	Super::CheckBotCount();
}

void AUTDuelGame::DefaultTimer()
{
	if (bRankedSession && HasMatchStarted() && !HasMatchEnded())
	{
		if (NumPlayers == 1)
		{
			EndGame(Cast<AUTPlayerState>(GameState->PlayerArray[0]), FName(TEXT("OpponentDrop")));			
		}
	}

	Super::DefaultTimer();
}