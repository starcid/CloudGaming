// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTHUDWidget_GameClock.h"

UUTHUDWidget_GameClock::UUTHUDWidget_GameClock(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Position=FVector2D(0.0f, 0.0f);
	Size=FVector2D(430.0f,83.0f);
	ScreenPosition=FVector2D(0.5f, 0.0f);
	Origin=FVector2D(0.5f,0.0f);
	NumPlayersDisplay = NSLOCTEXT("UTHUD", "NumPlayersDisplay", "/{PlayerCount}");
}

void UUTHUDWidget_GameClock::InitializeWidget(AUTHUD* Hud)
{
	Super::InitializeWidget(Hud);

	PlayerScoreText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_GameClock::GetPlayerScoreText);
	ClockText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_GameClock::GetClockText);
	PlayerRankText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_GameClock::GetPlayerRankText);
	PlayerRankThText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_GameClock::GetPlayerRankThText);
	NumPlayersText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_GameClock::GetNumPlayersText);
}

void UUTHUDWidget_GameClock::Draw_Implementation(float DeltaTime)
{
	if (UTGameState)
	{
		GameStateText.Text = UTGameState->GetGameStatusText(false);
		GameStateText.RenderColor = UTGameState->GetGameStatusColor();
	}
	float SkullX = (UTHUDOwner->CurrentPlayerScore < 10) ? 110.f : 125.f;
	UTHUDOwner->CalcStanding();
	if (UTHUDOwner->CurrentPlayerScore > 99)
	{
		SkullX = 140.f;
	}
	bool bEmptyText = GameStateText.Text.IsEmpty();
	GameStateBackground.bHidden = true; // bEmptyText; FIXMESTEVE remove widget entirely
	GameStateText.bHidden = bEmptyText;
	Skull.Position = FVector2D(SkullX, 10.f); // position 140, 10

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	if (GenericApplication.IsValid() && !GenericApplication->IsUsingHighPrecisionMouseMode() && UTHUDOwner->PlayerOwner && UTHUDOwner->PlayerOwner->PlayerState && !UTHUDOwner->PlayerOwner->PlayerState->bOnlySpectator)
	{
		Canvas->SetDrawColor(FColor::Red);
		Canvas->DrawTile(Canvas->DefaultTexture, Canvas->ClipX - 5, Canvas->ClipY - 5, 5, 5, 0.0f, 0.0f, 1.0f, 1.0f);
	}

	Super::Draw_Implementation(DeltaTime);
}

FText UUTHUDWidget_GameClock::GetPlayerScoreText_Implementation()
{
	return FText::AsNumber(UTHUDOwner->CurrentPlayerScore);
}

FText UUTHUDWidget_GameClock::GetClockText_Implementation()
{
	float RemainingTime = UTGameState ? UTGameState->GetClockTime() : 0.f;
	FText ClockString = UTHUDOwner->ConvertTime(FText::GetEmpty(),FText::GetEmpty(), RemainingTime,false);
	ClockText.TextScale = (RemainingTime >= 3600) ? AltClockScale : GetClass()->GetDefaultObject<UUTHUDWidget_GameClock>()->ClockText.TextScale;
	return ClockString;
}

FText UUTHUDWidget_GameClock::GetPlayerRankText_Implementation()
{
	return FText::AsNumber(UTHUDOwner->CurrentPlayerStanding);
}

FText UUTHUDWidget_GameClock::GetPlayerRankThText_Implementation()
{
	return UTHUDOwner->GetPlaceSuffix(UTHUDOwner->CurrentPlayerStanding);
}

FText UUTHUDWidget_GameClock::GetNumPlayersText_Implementation()
{
	FFormatNamedArguments Args;
	Args.Add("PlayerCount", FText::AsNumber(UTHUDOwner->NumActualPlayers));
	return FText::Format(NumPlayersDisplay, Args);
}


