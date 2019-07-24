// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_BioRifle.h"
#include "UTProj_BioShot.h"
#include "UTWeaponStateFiringCharged.h"
#include "StatNames.h"
#include "UTDefensePoint.h"

AUTWeap_BioRifle::AUTWeap_BioRifle(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UUTWeaponStateFiringCharged>(TEXT("FiringState1")))
{
	DefaultGroup = 3;
	Ammo = 40;
	MaxAmmo = 100;
	AmmoCost[0] = 2;
	AmmoCost[1] = 2;
	FOVOffset = FVector(1.7f, 1.f, 1.f);

	FireInterval[0] = 0.4f;
	FireInterval[1] = 0.66f;

	GlobConsumeTime = 0.33f;

	MaxGlobStrength = 9;
	GlobStrength = 0;

	SqueezeFireInterval = 0.2f;
	SqueezeFireSpread = 0.3f;
	SqueezeAmmoCost = 1;

	if (FiringState.IsValidIndex(1) && Cast<UUTWeaponStateFiringCharged>(FiringState[1]) != NULL)
	{
		((UUTWeaponStateFiringCharged*)FiringState[1])->bChargeFlashCount = true;
	}
	KillStatsName = NAME_BioRifleKills;
	DeathStatsName = NAME_BioRifleDeaths;
	HitsStatsName = NAME_BioRifleHits;
	ShotsStatsName = NAME_BioRifleShots;

	WeaponCustomizationTag = EpicWeaponCustomizationTags::BioRifle;
	WeaponSkinCustomizationTag = EpicWeaponSkinCustomizationTags::BioRifle;

	TutorialAnnouncements.Add(TEXT("PriBioRifle"));
	TutorialAnnouncements.Add(TEXT("SecBioRifle"));

	HighlightText = NSLOCTEXT("Weapon", "BioHighlightText", "So Much Snot");
	VeryLowMeshOffset = FVector(0.f, 0.f, -3.f);
}

void AUTWeap_BioRifle::UpdateSqueeze()
{
	if (SqueezeProjClass != NULL)
	{
		if (GetUTOwner() != NULL && GetUTOwner()->IsPendingFire(1))
		{
			FireInterval[0] = SqueezeFireInterval;
			Spread[0] = SqueezeFireSpread;
			ProjClass[0] = SqueezeProjClass;
			AmmoCost[0] = SqueezeAmmoCost;
		}
		else
		{
			FireInterval[0] = GetClass()->GetDefaultObject<AUTWeapon>()->FireInterval[0];
			Spread[0] = GetClass()->GetDefaultObject<AUTWeapon>()->Spread[0];
			ProjClass[0] = GetClass()->GetDefaultObject<AUTWeapon>()->ProjClass[0];
			AmmoCost[0] = GetClass()->GetDefaultObject<AUTWeapon>()->AmmoCost[0];
		}
	}
}

void AUTWeap_BioRifle::GotoFireMode(uint8 NewFireMode)
{
	if (NewFireMode == 0)
	{
		UpdateSqueeze();
	}
	Super::GotoFireMode(NewFireMode);
}

bool AUTWeap_BioRifle::HandleContinuedFiring()
{
	if (GetUTOwner())
	{
		// possible switch to squirt mode
		if (GetCurrentFireMode() == 0)
		{
			float OldFireInterval = FireInterval[0];
			// if currently in firemode 0, and 1 pressed, just switch mode
			UpdateSqueeze();

			if (FireInterval[0] != OldFireInterval)
			{
				UpdateTiming();
			}
		}
	}

	return Super::HandleContinuedFiring();
}

void AUTWeap_BioRifle::OnStartedFiring_Implementation()
{
	if (CurrentFireMode == 1)
	{
		StartCharge();
	}
}

void AUTWeap_BioRifle::OnContinuedFiring_Implementation()
{
	if (CurrentFireMode == 1)
	{
		StartCharge();
	}
}

void AUTWeap_BioRifle::StartCharge()
{
	ClearGlobStrength();
	IncreaseGlobStrength();
	OnStartCharging();

	//Play the charge animation
	if (GetNetMode() != NM_DedicatedServer && UTOwner != NULL)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL && ChargeAnimation != NULL)
		{
			AnimInstance->Montage_Play(ChargeAnimation, 1.f);
		}
	}
}

void AUTWeap_BioRifle::OnStartCharging_Implementation()
{
}

void AUTWeap_BioRifle::OnChargeShot_Implementation()
{
}

void AUTWeap_BioRifle::OnRep_Ammo()
{
	// defer weapon switch on out of ammo until charge release
	if (Cast<UUTWeaponStateFiringCharged>(GetCurrentState()) == nullptr)
	{
		Super::OnRep_Ammo();
	}
}

void AUTWeap_BioRifle::IncreaseGlobStrength()
{
	if (GlobStrength < MaxGlobStrength && HasAmmo(CurrentFireMode))
	{
		GlobStrength++;
		ConsumeAmmo(CurrentFireMode);
	}
	GetWorldTimerManager().SetTimer(IncreaseGlobStrengthHandle, this, &AUTWeap_BioRifle::IncreaseGlobStrength, GlobConsumeTime / ((UTOwner != NULL) ? UTOwner->GetFireRateMultiplier() : 1.0f), false);

	// AI decision to release fire
	if (UTOwner != NULL && GlobStrength > 1)
	{
		AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
		if (B != NULL && B->GetFocusActor() == B->GetTarget() && CanAttack(B->GetTarget(), B->GetFocalPoint(), true))
		{
			if (GlobStrength >= MaxGlobStrength || !HasAmmo(CurrentFireMode) || !B->CheckFutureSight(0.25f))
			{
				UTOwner->StopFiring();
			}
			// stop if bot thinks its charge amount is enough to kill target
			else if (B->GetTarget() == B->GetEnemy() && ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL)
			{
				AUTProj_BioShot* DefaultGlob = Cast<AUTProj_BioShot>(ProjClass[CurrentFireMode]->GetDefaultObject());
				if (DefaultGlob != NULL)
				{
					AUTCharacter* P = Cast<AUTCharacter>(B->GetEnemy());
					if (P != NULL && P->HealthMax * B->GetEnemyInfo(B->GetEnemy(), true)->EffectiveHealthPct < DefaultGlob->DamageParams.BaseDamage * GlobStrength)
					{
						UTOwner->StopFiring();
					}
				}
			}
		}
	}
}

void AUTWeap_BioRifle::ClearGlobStrength()
{
	GlobStrength = 0;
	GetWorldTimerManager().ClearTimer(IncreaseGlobStrengthHandle);
}

void AUTWeap_BioRifle::UpdateTiming()
{
	Super::UpdateTiming();
	if (GetWorldTimerManager().IsTimerActive(IncreaseGlobStrengthHandle))
	{
		GetWorldTimerManager().SetTimer(IncreaseGlobStrengthHandle, this, &AUTWeap_BioRifle::IncreaseGlobStrength, GlobConsumeTime / ((UTOwner != NULL) ? UTOwner->GetFireRateMultiplier() : 1.0f), false, GetWorldTimerManager().GetTimerRemaining(IncreaseGlobStrengthHandle));
	}
}

void AUTWeap_BioRifle::OnStoppedFiring_Implementation()
{
	if (CurrentFireMode == 1)
	{
		ClearGlobStrength();
	}
}

void AUTWeap_BioRifle::FireShot()
{
	if (!FireShotOverride() && CurrentFireMode == 1)
	{
		if (ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL)
		{
			AUTProj_BioShot* Glob = Cast<AUTProj_BioShot>(FireProjectile());

			if (Glob != NULL)
			{
				Glob->SetGlobStrength(GlobStrength);
			}
		}
		OnChargeShot();
		PlayFiringEffects();
		ClearGlobStrength();
		if (GetUTOwner() != NULL)
		{
			GetUTOwner()->InventoryEvent(InventoryEventName::FiredWeapon);
		}
	}
	else
	{
		Super::FireShot();
	}
}

void AUTWeap_BioRifle::FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation)
{
	UUTWeaponStateFiringCharged* Charged = Cast<UUTWeaponStateFiringCharged>(FiringState[1]);
	if (Charged != nullptr)
	{
		// odd FlashCount is charging, even is firing
		if (UTOwner->FireMode != 1 || (UTOwner->FlashCount % 2) == 0)
		{
			OnChargeShot();
			Super::FiringInfoUpdated_Implementation(InFireMode, FlashCount, InFlashLocation);
		}
		else
		{
			StopFiringEffects();
			StartCharge();
		}
	}
}

float AUTWeap_BioRifle::SuggestAttackStyle_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == nullptr || B->GetEnemy() == nullptr)
	{
		return 0.4f;
	}
	else
	{
		float EnemyDist = (B->GetEnemy()->GetActorLocation() - UTOwner->GetActorLocation()).Size();
		if (EnemyDist > 3300.0f)
		{
			return 1.0f;
		}
		else if (EnemyDist >= 2200.0f)
		{
			return 0.4f;
		}
		else
		{
			return -0.4;
		}
	}
}
float AUTWeap_BioRifle::SuggestDefenseStyle_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == nullptr || B->GetEnemy() == nullptr)
	{
		return 0.0f;
	}
	else if ((B->GetEnemy()->GetActorLocation() - UTOwner->GetActorLocation()).Size() < 3600.0f)
	{
		return -0.6f;
	}
	else
	{
		return 0.0f;
	}
}
bool AUTWeap_BioRifle::CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc)
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (Super::CanAttack_Implementation(Target, TargetLoc, bDirectOnly, bPreferCurrentMode, BestFireMode, OptimalTargetLoc))
	{
		if (!bPreferCurrentMode)
		{
			// increasing chance of primary fire at shorter ranges
			BestFireMode = (FMath::FRand() < ((TargetLoc - UTOwner->GetActorLocation()).Size() - 1000.0f) / 3000.0f) ? 1 : 0;
		}
		return true;
	}
	else if (B == nullptr || !B->WeaponProficiencyCheck())
	{
		return false;
	}
	else if (B->IsCamping() && (B->GetDefensePoint() == nullptr || !B->GetDefensePoint()->bSniperSpot) && (TargetLoc - UTOwner->GetActorLocation()).Size() <= 4000.0f)
	{
		// charge up while waiting for an enemy to appear
		BestFireMode = 1;
		return true;
	}
	else if (Target != B->GetEnemy() || (B->GetEnemyLocation(B->GetEnemy(), true) - UTOwner->GetActorLocation()).Size() > 4000.0f || B->LostContact(3.0f))
	{
		return false;
	}
	else
	{
		// charge up for when we see enemy again
		BestFireMode = 1;
		return true;
	}
}