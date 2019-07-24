// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTTimedPowerup.h"
#include "UTDroppedPickup.h"
#include "UTHUDWidget_Powerups.h"
#include "UnrealNetwork.h"

AUTTimedPowerup::AUTTimedPowerup(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	TimeRemaining = 30.0f;
	TriggeredTime = 15.f;
	DroppedTickRate = 1.f;
	RespawnTime = 90.0f;
	bAlwaysDropOnDeath = true;
	BasePickupDesireability = 2.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;

	bTimerPaused = false;
}

void AUTTimedPowerup::GivenTo(AUTCharacter* NewOwner, bool bAutoActivate)
{
	Super::GivenTo(NewOwner, bAutoActivate);

	TimeRemaining = FMath::Max(3.f, TimeRemaining);
	ClientSetTimeRemaining(TimeRemaining);
	NewOwner->SetWeaponOverlayEffect(OverlayEffect.IsValid() ? OverlayEffect : FOverlayEffect(OverlayMaterial), true);
	GetWorld()->GetTimerManager().SetTimer(PlayFadingSoundHandle, this, &AUTTimedPowerup::PlayFadingSound, FMath::Max<float>(0.1f, TimeRemaining - 3.0f), false);
	StatCountTime = GetWorld()->GetTimeSeconds();
}

void AUTTimedPowerup::InitAsTriggeredBoost(class AUTCharacter* TriggeringCharacter)
{
	TimeRemaining = TriggeredTime;
	Super::InitAsTriggeredBoost(TriggeringCharacter);
}

void AUTTimedPowerup::UpdateStatsCounter(float Amount)
{
	if (GetUTOwner() && (StatsNameTime != NAME_None))
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(GetUTOwner()->PlayerState);
		if (PS)
		{
			PS->ModifyStatsValue(StatsNameTime, Amount);
			if (PS->Team)
			{
				PS->Team->ModifyStatsValue(StatsNameTime, Amount);
			}
		}
	}
}

void AUTTimedPowerup::Removed()
{
	UpdateStatsCounter(GetWorld()->GetTimeSeconds() - StatCountTime);
	GetUTOwner()->SetWeaponOverlayEffect(OverlayEffect.IsValid() ? OverlayEffect : FOverlayEffect(OverlayMaterial), false);
	GetWorld()->GetTimerManager().ClearTimer(PlayFadingSoundHandle);
	Super::Removed();
}

void AUTTimedPowerup::PlayFadingSound()
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS && (GS->IsMatchInProgress() || (GS->GetMatchState() == MatchState::WaitingToStart)) && !GS->IsMatchIntermission())
	{
		// reset timer if time got added
		if (TimeRemaining > 3.f)
		{
			GetWorld()->GetTimerManager().SetTimer(PlayFadingSoundHandle, this, &AUTTimedPowerup::PlayFadingSound, TimeRemaining - 3.0f, false);
		}
		else
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), PowerupFadingSound, GetUTOwner());
			GetWorld()->GetTimerManager().SetTimer(PlayFadingSoundHandle, this, &AUTTimedPowerup::PlayFadingSound, 0.75f, false);
		}
	}
}

void AUTTimedPowerup::TimeExpired_Implementation()
{
	if (Role == ROLE_Authority)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GetUTOwner() && GS && (GS->IsMatchInProgress() || (GS->GetMatchState() == MatchState::WaitingToStart)) && !GS->IsMatchIntermission())
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), PowerupOverSound, GetUTOwner());
		}
		Destroy();
	}
}

void AUTTimedPowerup::Destroyed()
{
	if (MyPickup && !MyPickup->IsPendingKillPending())
	{
		MyPickup->Destroy();
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::Destroyed();
}

void AUTTimedPowerup::InitializeDroppedPickup(AUTDroppedPickup* Pickup)
{
	Super::InitializeDroppedPickup(Pickup);
	Pickup->SetLifeSpan(1.f + TimeRemaining/FMath::Max(DroppedTickRate, 0.001f));
	MyPickup = Pickup;
}

bool AUTTimedPowerup::StackPickup_Implementation(AUTInventory* ContainedInv)
{
	if (ContainedInv != NULL)
	{
		TimeRemaining += Cast<AUTTimedPowerup>(ContainedInv)->TimeRemaining;
	}
	else
	{
		TimeRemaining += GetClass()->GetDefaultObject<AUTTimedPowerup>()->TimeRemaining;
	}
	ClientSetTimeRemaining(TimeRemaining);
	return true;
}

void AUTTimedPowerup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TimeRemaining > 0.0f && !bTimerPaused)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS && (GS->IsMatchInProgress() || (GS->GetMatchState() == MatchState::WaitingToStart)) && !GS->IsMatchIntermission())
		{
			float TickMultiplier = (GetUTOwner() != NULL) ? 1.f : DroppedTickRate;
			TimeRemaining -= (DeltaTime * TickMultiplier);
			if ((TimeRemaining <= 0.0f) || (TimeRemaining <= 2.0f && GetUTOwner() == NULL))
			{
				TimeExpired();
			}
			float ElapsedTime = GetWorld()->GetTimeSeconds() - StatCountTime;
			if (ElapsedTime > 1.f)
			{
				UpdateStatsCounter(1.f);
				StatCountTime += 1.f;
			}
		}
	}
}

void AUTTimedPowerup::ClientSetTimeRemaining_Implementation(float InTimeRemaining)
{
	TimeRemaining = InTimeRemaining;
}

void AUTTimedPowerup::DrawInventoryHUD_Implementation(UUTHUDWidget* Widget, FVector2D Pos, FVector2D Size)
{
	if (HUDIcon.Texture != NULL)
	{

		FText Text = FText::AsNumber(FMath::CeilToInt(TimeRemaining));

		UFont* MediumFont = Widget->UTHUDOwner->MediumFont;
		float Xl, YL;
		Widget->GetCanvas()->StrLen(MediumFont, FString(TEXT("000")), Xl, YL);

		// Draws the Timer first 
		Widget->DrawText(Text, 0, Pos.Y + (Size.Y * 0.5f) , Widget->UTHUDOwner->MediumFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Right, ETextVertPos::Center);

		// icon left aligned, time remaining right aligned
		Widget->DrawTexture(HUDIcon.Texture, Xl * -1, Pos.Y, Size.X, Size.Y, HUDIcon.U, HUDIcon.V, HUDIcon.UL, HUDIcon.VL,1.0, FLinearColor::White, FVector2D(1.0,0.0));

	}
}

void AUTTimedPowerup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// this is for spectators, owner gets this via RPC for better accuracy
	DOREPLIFETIME_CONDITION(AUTTimedPowerup, TimeRemaining, COND_SkipOwner);
	DOREPLIFETIME(AUTTimedPowerup, bTimerPaused);
}

// Allows inventory items to decide if a widget should be allowed to render them.
bool AUTTimedPowerup::HUDShouldRender_Implementation(UUTHUDWidget* TargetWidget)
{
	return (Cast<UUTHUDWidget_Powerups>(TargetWidget) != NULL);
}
