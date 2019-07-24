// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_Sniper.h"
#include "UTProj_Sniper.h"
#include "UTWeaponState.h"
#include "UTWeaponStateFiring.h"
#include "UTWeaponStateZooming.h"
#include "Particles/ParticleSystemComponent.h"
#include "StatNames.h"

AUTWeap_Sniper::AUTWeap_Sniper(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UUTWeaponStateZooming>(TEXT("FiringState1")) )
{
	DefaultGroup = 9;
	BringUpTime = 0.45f;
	PutDownTime = 0.4f;
	StoppedHeadshotScale = 1.4f;
	SlowHeadshotScale = 1.4f;
	AimedHeadshotScale = 1.f;
	RunningHeadshotScale = 1.0f;
	HeadshotDamage = 125.f;
	ProjClass.Insert(AUTProj_Sniper::StaticClass(), 0);
	FOVOffset = FVector(0.1f, 1.f, 1.7f);
	HUDIcon = MakeCanvasIcon(HUDIcon.Texture, 726, 532, 165, 51);
	bPrioritizeAccuracy = true;
	BaseAISelectRating = 0.7f;
	BasePickupDesireability = 0.63f;
	FiringViewKickback = -50.f;
	FiringViewKickbackY = 0.f;
	BlockedHeadshotDamage = 45;
	bSniping = true;
	HUDViewKickback = FVector2D(0.f, 0.2f);

	KillStatsName = NAME_SniperKills;
	AltKillStatsName = NAME_SniperHeadshotKills;
	DeathStatsName = NAME_SniperDeaths;
	AltDeathStatsName = NAME_SniperHeadshotDeaths;
	HitsStatsName = NAME_SniperHits;
	ShotsStatsName = NAME_SniperShots;
	bCheckHeadSphere = true;
	bCheckMovingHeadSphere = true;
	bIgnoreShockballs = true;
	bTrackHitScanReplication = true;

	WeaponCustomizationTag = EpicWeaponCustomizationTags::Sniper;
	WeaponSkinCustomizationTag = EpicWeaponSkinCustomizationTags::Sniper;
	TutorialAnnouncements.Add(TEXT("PriSniper"));
	TutorialAnnouncements.Add(TEXT("SecSniper"));
	HighlightText = NSLOCTEXT("Weapon", "SniperHighlightText", "One Man One Bullet");
	LowMeshOffset = FVector(0.f, 0.f, -5.f);
	VeryLowMeshOffset = FVector(0.f, 0.f, -11.f);
}

float AUTWeap_Sniper::GetHeadshotScale(AUTCharacter* HeadshotTarget) const
{
	if (HeadshotTarget && HeadshotTarget->UTCharacterMovement && HeadshotTarget->UTCharacterMovement->bIsFloorSliding)
	{
		return RunningHeadshotScale;
	}
	if ( (GetUTOwner()->GetVelocity().Size() <= GetUTOwner()->GetCharacterMovement()->MaxWalkSpeedCrouched + 1.0f) &&
		(GetUTOwner()->bIsCrouched || GetUTOwner()->GetCharacterMovement() == NULL || GetUTOwner()->GetCharacterMovement()->GetCurrentAcceleration().Size() < GetUTOwner()->GetCharacterMovement()->MaxWalkSpeedCrouched + 1.0f) )
	{
		return (GetUTOwner()->GetVelocity().Size() < 10.f) ? StoppedHeadshotScale : SlowHeadshotScale;
	}
	else if (GetUTOwner()->GetCharacterMovement()->GetCurrentAcceleration().IsZero())
	{
		return AimedHeadshotScale;
	}
	else
	{
		return RunningHeadshotScale;
	}
}

AUTProjectile* AUTWeap_Sniper::FireProjectile()
{
	AUTProj_Sniper* SniperProj = Cast<AUTProj_Sniper>(Super::FireProjectile());
	if (SniperProj != NULL)
	{
		SniperProj->HeadScaling *= GetHeadshotScale(nullptr);
	}
	return SniperProj;
}

void AUTWeap_Sniper::FireInstantHit(bool bDealDamage, FHitResult* OutHit)
{
	checkSlow(InstantHitInfo.IsValidIndex(CurrentFireMode));

	const FVector SpawnLocation = GetFireStartLoc();
	const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);
	const FVector FireDir = SpawnRotation.Vector();
	const FVector EndTrace = SpawnLocation + FireDir * InstantHitInfo[CurrentFireMode].TraceRange;

	FHitResult Hit;
	AUTPlayerController* UTPC = UTOwner ? Cast<AUTPlayerController>(UTOwner->Controller) : NULL;
	AUTPlayerState* PS = (UTOwner && UTOwner->Controller) ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;
	float PredictionTime = UTPC ? UTPC->GetPredictionTime() : 0.f;
	HitScanTrace(SpawnLocation, EndTrace, InstantHitInfo[CurrentFireMode].TraceHalfSize, Hit, PredictionTime);

	if (UTOwner && Cast<AUTCharacter>(Hit.Actor.Get()) == NULL)
	{
		// in some cases the head sphere is partially outside the capsule
		// so do a second search just for that
		AUTCharacter* AltTarget = Cast<AUTCharacter>(UUTGameplayStatics::ChooseBestAimTarget(GetUTOwner()->Controller, SpawnLocation, FireDir, 0.7f, (Hit.Location - SpawnLocation).Size(), 150.0f, AUTCharacter::StaticClass()));
		if (AltTarget != NULL && AltTarget->IsHeadShot(SpawnLocation, FireDir, GetHeadshotScale(AltTarget), UTOwner, PredictionTime))
		{
			Hit = FHitResult(AltTarget, AltTarget->GetCapsuleComponent(), SpawnLocation + FireDir * ((AltTarget->GetHeadLocation() - SpawnLocation).Size() - AltTarget->GetCapsuleComponent()->GetUnscaledCapsuleRadius()), -FireDir);
		}
	}

	if (Role == ROLE_Authority)
	{
		if (PS && (ShotsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(ShotsStatsName, 1);
		}

		// warn bot target, if any
		if (UTPC != NULL)
		{
			APawn* PawnTarget = Cast<APawn>(Hit.Actor.Get());
			if (PawnTarget != NULL)
			{
				UTPC->LastShotTargetGuess = PawnTarget;
			}
			// if not dealing damage, it's the caller's responsibility to send warnings if desired
			if (bDealDamage && UTPC->LastShotTargetGuess != NULL)
			{
				AUTBot* EnemyBot = Cast<AUTBot>(UTPC->LastShotTargetGuess->Controller);
				if (EnemyBot != NULL)
				{
					EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
				}
			}
		}
		else if (bDealDamage)
		{
			AUTBot* B = UTOwner ? Cast<AUTBot>(UTOwner->Controller) : nullptr;
			if (B != NULL)
			{
				APawn* PawnTarget = Cast<APawn>(Hit.Actor.Get());
				if (PawnTarget == NULL)
				{
					PawnTarget = Cast<APawn>(B->GetTarget());
				}
				if (PawnTarget != NULL)
				{
					AUTBot* EnemyBot = Cast<AUTBot>(PawnTarget->Controller);
					if (EnemyBot != NULL)
					{
						EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
					}
				}
			}
		}
	}
	else if (PredictionTime > 0.f)
	{
		PlayPredictedImpactEffects(Hit.Location);
	}
	if (Hit.Actor != NULL && Hit.Actor->bCanBeDamaged && bDealDamage)
	{
		int32 Damage = GetHitScanDamage();
		TSubclassOf<UDamageType> DamageType = InstantHitInfo[CurrentFireMode].DamageType;

		bool bIsHeadShot = false;
		bool bBlockedHeadshot = false;
		AUTCharacter* C = Cast<AUTCharacter>(Hit.Actor.Get());
		if (C != NULL && CanHeadShot() && C->IsHeadShot(Hit.Location, FireDir, GetHeadshotScale(C), UTOwner, PredictionTime))
		{
			bIsHeadShot = true;
			if (C->BlockedHeadShot(Hit.Location, FireDir, GetHeadshotScale(C), true, UTOwner))
			{
				Damage = BlockedHeadshotDamage;
				bBlockedHeadshot = true;
			}
			else
			{
				AUTBot* B = UTOwner ? Cast<AUTBot>(UTOwner->Controller) : nullptr;
				if (!B || (B->Skill + B->Personality.Accuracy > 3.5f))
				{
					Damage = HeadshotDamage;
				}
			}
			if (HeadshotDamageType != NULL)
			{
				DamageType = HeadshotDamageType;
			}
		}
		OnHitScanDamage(Hit, FireDir);
		Hit.Actor->TakeDamage(Damage, FUTPointDamageEvent(Damage, Hit, FireDir, DamageType, FireDir * InstantHitInfo[CurrentFireMode].Momentum), (UTOwner ? UTOwner->Controller : nullptr), this);

		if ((Role == ROLE_Authority) && bIsHeadShot && C && (C->Health > 0) && (bBlockedHeadshot || (Damage >= 100)))
		{
			C->NotifyBlockedHeadShot(UTOwner);
		}
		if ((Role == ROLE_Authority) && PS && (HitsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(HitsStatsName, 1);
		}
	}
	if (UTOwner && (Role == ROLE_Authority))
	{
		SetFlashExtra(Hit.Actor.Get());
		UTOwner->SetFlashLocation(Hit.Location, CurrentFireMode);
	}
	if (OutHit != NULL)
	{
		*OutHit = Hit;
	}
}

void AUTWeap_Sniper::PlayPredictedImpactEffects(FVector ImpactLoc)
{
	SetFlashExtra(nullptr);
	Super::PlayPredictedImpactEffects(ImpactLoc);
}

int32 AUTWeap_Sniper::GetHitScanDamage()
{
	return InstantHitInfo[CurrentFireMode].Damage;;
}

bool AUTWeap_Sniper::CanHeadShot()
{
	return true;
}

float AUTWeap_Sniper::GetAISelectRating_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == NULL)
	{
		return BaseAISelectRating;
	}
	else if (Cast<APawn>(B->GetTarget()) == NULL)
	{
		return BaseAISelectRating - 0.15f;
	}
	else if (B->GetEnemy() == NULL)
	{
		return BaseAISelectRating;
	}
	else
	{
		float Result = B->IsStopped() ? (BaseAISelectRating + 0.1f) : (BaseAISelectRating - 0.1f);
		/*if (Vehicle(B.Enemy) != None)
			result -= 0.2;*/
		const FVector EnemyLoc = B->GetEnemyLocation(B->GetEnemy(), false);
		float ZDiff = UTOwner->GetActorLocation().Z - EnemyLoc.Z;
		if (ZDiff < -B->TacticalHeightAdvantage)
		{
			Result += 0.1;
		}
		float Dist = (EnemyLoc - UTOwner->GetActorLocation()).Size();
		if (Dist > 4500.0f)
		{
			if (!CanAttack(B->GetEnemy(), EnemyLoc, false))
			{
				Result -= 0.15f;
			}
			return FMath::Min<float>(2.0f, Result + (Dist - 4500.0f) * 0.0001);
		}
		else if (!CanAttack(B->GetEnemy(), EnemyLoc, false))
		{
			return BaseAISelectRating - 0.1f;
		}
		else
		{
			return Result;
		}
	}
}

void AUTWeap_Sniper::OnRep_ZoomState_Implementation()
{
	Super::OnRep_ZoomState_Implementation();

	if (GetNetMode() != NM_DedicatedServer)
	{
		UUTWeaponStateZooming* WeaponStateZooming = FiringState.IsValidIndex(1) ? Cast<UUTWeaponStateZooming>(FiringState[1]) : nullptr;
		if (WeaponStateZooming != nullptr)
		{
			if (ZoomState == EZoomState::EZS_NotZoomed)
			{
				SetActorHiddenInGame(false);
				WeaponStateZooming->ToggleZoomInSound(false);
				UUTGameplayStatics::UTPlaySound(GetWorld(), WeaponStateZooming->ZoomOutSound, GetUTOwner(), SRT_None, false, FVector::ZeroVector, NULL, NULL, false);
			}
			else
			{
				SetActorHiddenInGame(true);

				if (ZoomState == EZoomState::EZS_ZoomingIn)
				{
					WeaponStateZooming->ToggleZoomInSound(true);
					UUTGameplayStatics::UTPlaySound(GetWorld(), WeaponStateZooming->ZoomInSound, GetUTOwner(), SRT_None, false, FVector::ZeroVector, NULL, NULL, false);
				}
				else
				{
					WeaponStateZooming->ToggleZoomInSound(false);
				}
			}
		}
	}
}