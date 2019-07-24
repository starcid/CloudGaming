// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTHUDWidget_TeamGameClock.h"

UUTHUDWidget_TeamGameClock::UUTHUDWidget_TeamGameClock(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Position=FVector2D(0.0f, 0.0f);
	Size=FVector2D(430.0f,83.0f);
	ScreenPosition=FVector2D(0.5f, 0.0f);
	Origin=FVector2D(0.5f,0.0f);
}

void UUTHUDWidget_TeamGameClock::InitializeWidget(AUTHUD* Hud)
{
	Super::InitializeWidget(Hud);

	RedScoreText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_TeamGameClock::GetRedScoreText);
	BlueScoreText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_TeamGameClock::GetBlueScoreText);
	ClockText.GetTextDelegate.BindUObject(this, &UUTHUDWidget_TeamGameClock::GetClockText);
}

void UUTHUDWidget_TeamGameClock::Draw_Implementation(float DeltaTime)
{
	FText StatusText = FText::GetEmpty();
	if (UTGameState != NULL)
	{
		StatusText = UTGameState->GetGameStatusText(false);
	}

	if (!StatusText.IsEmpty())
	{
		GameStateText.bHidden = false;
		GameStateText.Text = StatusText;
		GameStateText.RenderColor = UTGameState->GetGameStatusColor();
	}
	else
	{
		GameStateText.bHidden = true;
		GameStateText.Text = StatusText;
	}

	AUTCharacter* UTC = Cast<AUTCharacter>(UTHUDOwner->UTPlayerOwner->GetViewTarget());
	AUTPlayerState* PS = UTC ? Cast<AUTPlayerState>(UTC->PlayerState) : NULL;
	if (UTGameState && UTGameState->bTeamGame && PS && PS->Team && UTGameState->HasMatchStarted())
	{
		RoleText.bHidden = false;
		FText RoleTextOverride = UTGameState->OverrideRoleText(PS);
		if (!RoleTextOverride.IsEmpty())
		{
			RoleText.Text = RoleTextOverride;
			RoleText.Position.X = 130.f;
			TeamNameText.Position.X = 285.f;
		}

		TeamNameText.bHidden = false;
		TeamNameText.Text = PS->Team->TeamName;
		TeamNameText.RenderColor = PS->Team->TeamColor;
	}
	else
	{
		RoleText.bHidden = true;
		TeamNameText.bHidden = true;
	}

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	if (GenericApplication.IsValid() && !GenericApplication->IsUsingHighPrecisionMouseMode() && UTHUDOwner->PlayerOwner && UTHUDOwner->PlayerOwner->PlayerState && !UTHUDOwner->PlayerOwner->PlayerState->bOnlySpectator)
	{
		Canvas->SetDrawColor(FColor::Red);
		Canvas->DrawTile(Canvas->DefaultTexture, Canvas->ClipX - 5, Canvas->ClipY - 5, 5, 5, 0.0f, 0.0f, 1.0f, 1.0f);
	}

	Super::Draw_Implementation(DeltaTime);
}

FText UUTHUDWidget_TeamGameClock::GetRedScoreText_Implementation()
{
	if (UTGameState && UTGameState->bTeamGame && UTGameState->Teams.Num() > 0 && UTGameState->Teams[0])
	{
		return FText::AsNumber(UTGameState->Teams[0]->Score);
	}
	return FText::AsNumber(0);
}

FText UUTHUDWidget_TeamGameClock::GetBlueScoreText_Implementation()
{
	if (UTGameState && UTGameState->bTeamGame && UTGameState->Teams.Num() > 1 && UTGameState->Teams[1])
	{
		return FText::AsNumber(UTGameState->Teams[1]->Score);
	}
	return FText::AsNumber(0);
}

FText UUTHUDWidget_TeamGameClock::GetClockText_Implementation()
{
	float RemainingTime = UTGameState ? UTGameState->GetClockTime() : 0.f;
	FText ClockString = UTHUDOwner->ConvertTime(FText::GetEmpty(),FText::GetEmpty(),RemainingTime,false);
	ClockText.TextScale = (RemainingTime >= 3600) ? AltClockScale : GetClass()->GetDefaultObject<UUTHUDWidget_TeamGameClock>()->ClockText.TextScale;
	return ClockString;
}
