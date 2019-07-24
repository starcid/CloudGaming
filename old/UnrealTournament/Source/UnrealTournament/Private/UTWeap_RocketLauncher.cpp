// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeap_RocketLauncher.h"
#include "UTWeaponStateFiringChargedRocket.h"
#include "UTProj_Rocket.h"
#include "Particles/ParticleSystemComponent.h"
#include "UnrealNetwork.h"
#include "StatNames.h"
#include "Animation/AnimMontage.h"

AUTWeap_RocketLauncher::AUTWeap_RocketLauncher(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UUTWeaponStateFiringChargedRocket>(TEXT("FiringState1")))
{
	DefaultGroup = 8;
	BringUpTime = 0.41f;

	NumLoadedRockets = 0;
	NumLoadedBarrels = 0;
	MaxLoadedRockets = 3;
	RocketLoadTime = 0.9f;
	FirstRocketLoadTime = 0.4f;
	CurrentRocketFireMode = 0;
	bDrawRocketModeString = false;
	FOVOffset = FVector(0.5f, 1.f, 1.f);

	bLockedOnTarget = false;
	LockCheckTime = 0.1f;
	LockRange = 16000.0f;
	LockAcquireTime = 0.5f;
	LockTolerance = 0.2f;
	LockedTarget = NULL;
	PendingLockedTarget = NULL;
	LastLockedOnTime = 0.0f;
	PendingLockedTargetTime = 0.0f;
	LastValidTargetTime = 0.0f;
	LockAim = 0.997f;
	LockOffset = 800.f;
	bTargetLockingActive = true;
	LastTargetLockCheckTime = 0.0f;
	HUDViewKickback = FVector2D(0.f, 0.2f);

	UnderReticlePadding = 20.0f;
	CrosshairScale = 0.5f;

	CrosshairRotationTime = 0.3f;
	CurrentRotation = 0.0f;

	BarrelRadius = 9.0f;

	GracePeriod = 0.6f;
	BurstInterval = 0.f;
	GrenadeBurstInterval = 0.1f;
	FullLoadSpread = 8.f;
	SeekingLoadSpread = 16.f;
	bAllowGrenades = false;

	BasePickupDesireability = 0.78f;
	BaseAISelectRating = 0.78f;
	FiringViewKickback = -50.f;
	FiringViewKickbackY = 20.f;
	bRecommendSplashDamage = true;

	KillStatsName = NAME_RocketKills;
	DeathStatsName = NAME_RocketDeaths;
	HitsStatsName = NAME_RocketHits;
	ShotsStatsName = NAME_RocketShots;

	WeaponCustomizationTag = EpicWeaponCustomizationTags::RocketLauncher;
	WeaponSkinCustomizationTag = EpicWeaponSkinCustomizationTags::RocketLauncher;

	TutorialAnnouncements.Add(TEXT("PriRocketLauncher"));
	TutorialAnnouncements.Add(TEXT("SecRocketLauncher"));
	HighlightText = NSLOCTEXT("Weapon", "RockerHighlightText", "I am the Rocketman");
	LowMeshOffset = FVector(0.f, 0.f, -7.f);
	VeryLowMeshOffset = FVector(0.f, 0.f, -15.f);
}

void AUTWeap_RocketLauncher::Destroyed()
{
	Super::Destroyed();
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

void AUTWeap_RocketLauncher::BeginLoadRocket()
{
	//Play the load animation. Speed of anim based on GetLoadTime()
	if ((GetNetMode() != NM_DedicatedServer) && (GetMesh()->GetAnimInstance() != nullptr))
	{
		UAnimMontage* PickedAnimation = nullptr;
		UAnimMontage* PickedHandHanim = nullptr;
		if (Ammo > 0)
		{
			PickedAnimation = (LoadingAnimation.IsValidIndex(NumLoadedBarrels) && LoadingAnimation[NumLoadedBarrels] != NULL) ? LoadingAnimation[NumLoadedBarrels] : nullptr;
			PickedHandHanim = (LoadingAnimationHands.IsValidIndex(NumLoadedBarrels) && LoadingAnimationHands[NumLoadedBarrels] != NULL) ? LoadingAnimationHands[NumLoadedBarrels] : nullptr;

		}
		else
		{
			PickedAnimation = (EmptyLoadingAnimation.IsValidIndex(NumLoadedBarrels) && EmptyLoadingAnimation[NumLoadedBarrels] != NULL) ? EmptyLoadingAnimation[NumLoadedBarrels] : nullptr;
			PickedHandHanim = (EmptyLoadingAnimationHands.IsValidIndex(NumLoadedBarrels) && EmptyLoadingAnimationHands[NumLoadedBarrels] != NULL) ? EmptyLoadingAnimationHands[NumLoadedBarrels] : nullptr;
		}

		if (PickedAnimation != nullptr)
		{
			GetMesh()->GetAnimInstance()->Montage_Play(PickedAnimation, PickedAnimation->SequenceLength / GetLoadTime(NumLoadedBarrels));
		}
		if (GetUTOwner() != nullptr && GetUTOwner()->FirstPersonMesh != nullptr && GetUTOwner()->FirstPersonMesh->GetAnimInstance() != nullptr && PickedHandHanim != nullptr)
		{
			GetUTOwner()->FirstPersonMesh->GetAnimInstance()->Montage_Play(PickedHandHanim, PickedHandHanim->SequenceLength / GetLoadTime(NumLoadedBarrels));
		}
	}
}

void AUTWeap_RocketLauncher::EndLoadRocket()
{
	NumLoadedBarrels++;
	if (Ammo > 0)
	{
		NumLoadedRockets++;
		SetRocketFlashExtra(CurrentFireMode, NumLoadedRockets + 1, CurrentRocketFireMode, bDrawRocketModeString);
		ConsumeAmmo(CurrentFireMode);
		if ((Ammo <= LowAmmoThreshold) && (Ammo > 0) && (LowAmmoSound != nullptr))
		{
			AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if (!GameMode || GameMode->bAmmoIsLimited)
			{
				GetWorldTimerManager().SetTimer(PlayLowAmmoSoundHandle, this, &AUTWeapon::PlayLowAmmoSound, LowAmmoSoundDelay, false);
			}
		}
	}
	else
	{
		PlayLowAmmoSound();
	}
	LastLoadTime = GetWorld()->TimeSeconds;

	//Replicate the loading sound to other players 
	//Local players will use the sounds synced to the animation
	AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
	if ((PC == nullptr) || !PC->IsLocalPlayerController())
	{
		UUTGameplayStatics::UTPlaySound(GetWorld(), RocketLoadedSound, UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, NULL, NULL, true, SAT_WeaponFire);
	}

	// bot maybe shoots rockets from here
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B != NULL && B->GetTarget() != NULL && B->LineOfSightTo(B->GetTarget()) && !B->NeedToTurn(B->GetFocalPoint()))
	{
		if (NumLoadedRockets == MaxLoadedRockets)
		{
			UTOwner->StopFiring();
		}
		else if (NumLoadedRockets > 1)
		{
			if (B->GetTarget() != B->GetEnemy())
			{
				if (FMath::FRand() < 0.5f)
				{
					UTOwner->StopFiring();
				}
			}
			else if (FMath::FRand() < 0.3f)
			{
				UTOwner->StopFiring();
			}
			else if (ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL)
			{
				// if rockets would do more than 2x target's health, worth trying for the kill now
				AUTCharacter* P = Cast<AUTCharacter>(B->GetEnemy());
				if (P != NULL && P->HealthMax * B->GetEnemyInfo(B->GetEnemy(), true)->EffectiveHealthPct * 2.0f < ProjClass[CurrentFireMode].GetDefaultObject()->DamageParams.BaseDamage * NumLoadedRockets)
				{
					UTOwner->StopFiring();
				}
			}
		}
	}
}

void AUTWeap_RocketLauncher::ClearLoadedRockets()
{
	CurrentRocketFireMode = 0;
	NumLoadedBarrels = 0;
	NumLoadedRockets = 0;
	if (Role == ROLE_Authority)
	{
		SetLockTarget(nullptr);
		PendingLockedTarget = nullptr;
		PendingLockedTargetTime = 0.f;
	}
	if (UTOwner != NULL)
	{
		UTOwner->SetFlashExtra(0, CurrentFireMode);
	}
	bDrawRocketModeString = false;
	LastLoadTime = 0.0f;
}

void AUTWeap_RocketLauncher::ClientAbortLoad_Implementation()
{
	UUTWeaponStateFiringChargedRocket* LoadState = Cast<UUTWeaponStateFiringChargedRocket>(CurrentState);
	if (LoadState != NULL)
	{
		// abort loading anim if it was playing
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL && LoadingAnimation.IsValidIndex(NumLoadedRockets) && LoadingAnimation[NumLoadedRockets] != NULL)
		{
			AnimInstance->Montage_SetPlayRate(LoadingAnimation[NumLoadedRockets], 0.0f);
		}

		UAnimInstance* AnimInstanceHands = (GetUTOwner() && GetUTOwner()->FirstPersonMesh) ? GetUTOwner()->FirstPersonMesh->GetAnimInstance() : nullptr;
		if (AnimInstanceHands != NULL && LoadingAnimationHands.IsValidIndex(NumLoadedRockets) && LoadingAnimationHands[NumLoadedRockets] != NULL)
		{
			AnimInstanceHands->Montage_SetPlayRate(LoadingAnimationHands[NumLoadedRockets], 0.0f);
		}
		// set grace timer
		float AdjustedGraceTime = GracePeriod;
		if (UTOwner != NULL && UTOwner->PlayerState != NULL)
		{
			AdjustedGraceTime = FMath::Max<float>(0.01f, AdjustedGraceTime - UTOwner->PlayerState->ExactPing * 0.0005f); // one way trip so half ping
		}
		GetWorldTimerManager().SetTimer(LoadState->GraceTimerHandle, LoadState, &UUTWeaponStateFiringChargedRocket::GraceTimer, AdjustedGraceTime, false);
	}
}

float AUTWeap_RocketLauncher::GetLoadTime(int32 InNumLoadedRockets)
{
	return ((InNumLoadedRockets > 0) ? RocketLoadTime : FirstRocketLoadTime) / ((UTOwner != NULL) ? UTOwner->GetFireRateMultiplier() : 1.0f);
}

void AUTWeap_RocketLauncher::OnMultiPress_Implementation(uint8 OtherFireMode)
{
	if (bAllowGrenades && (CurrentFireMode == 1))
	{
		UUTWeaponStateFiringChargedRocket* AltState = Cast<UUTWeaponStateFiringChargedRocket>(FiringState[1]);
		if (AltState != NULL && AltState->bCharging)
		{
			if ((GetWorldTimerManager().IsTimerActive(AltState->GraceTimerHandle)
				&& (GetWorldTimerManager().GetTimerRemaining(AltState->GraceTimerHandle) < 0.05f))
				|| (GetWorldTimerManager().IsTimerActive(SpawnDelayedFakeProjHandle)))
			{
				// too close to timer ending, so don't allow mode change to avoid de-synchronizing client and server
				return;
			}
			CurrentRocketFireMode++;
			bDrawRocketModeString = true;

			if (CurrentRocketFireMode >= RocketFireModes.Num())
			{
				CurrentRocketFireMode = 0;
			}
			UUTGameplayStatics::UTPlaySound(GetWorld(), AltFireModeChangeSound, UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, NULL, NULL, true, SAT_WeaponFoley);

			//Update Extraflash so spectators can see the hud text
			if (Role == ROLE_Authority)
			{
				SetRocketFlashExtra(CurrentFireMode, NumLoadedRockets + 1, CurrentRocketFireMode, bDrawRocketModeString);
			}
		}
	}
}

bool AUTWeap_RocketLauncher::ShouldFireLoad()
{
	return !UTOwner || UTOwner->IsPendingKillPending() || (UTOwner->Health <= 0) || UTOwner->IsRagdoll();
}

void AUTWeap_RocketLauncher::FireShot()
{
	if (UTOwner)
	{
		UTOwner->DeactivateSpawnProtection();
	}
	//Alternate fire already consumed ammo
	if (CurrentFireMode != 1)
	{
		ConsumeAmmo(CurrentFireMode);
	}

	if (!FireShotOverride())
	{
		AUTProj_Rocket* NewRocket = Cast<AUTProj_Rocket>(FireProjectile());
		PlayFiringEffects();
		if (NumLoadedRockets <= 0)
		{
			ClearLoadedRockets();
		}
	}

	if (GetUTOwner() != NULL)
	{
		GetUTOwner()->InventoryEvent(InventoryEventName::FiredWeapon);
	}
	FireZOffsetTime = 0.f;
}

AUTProjectile* AUTWeap_RocketLauncher::FireProjectile()
{
	if (GetUTOwner() == NULL)
	{
		UE_LOG(UT, Warning, TEXT("%s::FireProjectile(): Weapon is not owned (owner died during firing sequence)"), *GetName());
		return NULL;
	}

	UTOwner->SetFlashExtra(0, CurrentFireMode);
	AUTPlayerState* PS = UTOwner->Controller ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;

	//For the alternate fire, the number of flashes are replicated by the FireMode. 
	if (CurrentFireMode == 1)
	{
		// Bots choose mode now
		if (bAllowGrenades)
		{
			AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
			if (B != NULL)
			{
				if (B->GetTarget() == B->GetEnemy())
				{
					// when retreating, we want grenades
					if (B->GetEnemy() == NULL || B->LostContact(1.0f) /*|| B.IsRetreating() || B.IsInState('StakeOut')*/)
					{
						CurrentRocketFireMode = 1;
					}
					else
					{
						CurrentRocketFireMode = 0;
					}
				}
				else
				{
					CurrentRocketFireMode = 1;
				}
			}
		}

		//Only play Muzzle flashes if the Rocket mode permits ie: Grenades no flash
		if ((Role == ROLE_Authority)/* && RocketFireModes.IsValidIndex(CurrentRocketFireMode) && RocketFireModes[CurrentRocketFireMode].bCauseMuzzleFlash*/)
		{
			UTOwner->IncrementFlashCount(NumLoadedRockets);
			if (PS && (ShotsStatsName != NAME_None))
			{
				PS->ModifyStatsValue(ShotsStatsName, NumLoadedRockets);
			}
		}
			
		return FireRocketProjectile();
	}
	else
	{
		checkSlow(ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL);
		if (Role == ROLE_Authority)
		{
			UTOwner->IncrementFlashCount(CurrentFireMode);
			if (PS && (ShotsStatsName != NAME_None))
			{
				PS->ModifyStatsValue(ShotsStatsName, 1);
			}
		}

		FVector SpawnLocation = GetFireStartLoc();
		FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);

		//Adjust from the center of the gun to the barrel
		FVector AdjustedSpawnLoc = SpawnLocation + FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::Z) * BarrelRadius; //Adjust rocket based on barrel size
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, SpawnLocation, AdjustedSpawnLoc, COLLISION_TRACE_WEAPON, FCollisionQueryParams(NAME_None, true, UTOwner)))
		{
			SpawnLocation = Hit.Location - (AdjustedSpawnLoc - SpawnLocation).GetSafeNormal();
		}
		else
		{
			SpawnLocation = AdjustedSpawnLoc;
		}
		AUTProjectile* SpawnedProjectile = SpawnNetPredictedProjectile(RocketFireModes[CurrentRocketFireMode].ProjClass, SpawnLocation, SpawnRotation);
		AUTProj_Rocket* SpawnedRocket = Cast<AUTProj_Rocket>(SpawnedProjectile);
		NumLoadedRockets = 0;
		NumLoadedBarrels = 0;
		return SpawnedProjectile;
	}
}

void AUTWeap_RocketLauncher::PlayDelayedFireSound()
{
	if (UTOwner && RocketFireModes.IsValidIndex(0) && RocketFireModes[0].FireSound != NULL)
	{
		if (RocketFireModes[0].FPFireSound != NULL && Cast<APlayerController>(UTOwner->Controller) != NULL && UTOwner->IsLocallyControlled())
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), RocketFireModes[0].FPFireSound, UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, SAT_WeaponFire);
		}
		else
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), RocketFireModes[0].FireSound, UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, SAT_WeaponFire);
		}
	}
}

void AUTWeap_RocketLauncher::PlayFiringEffects()
{
	if (CurrentFireMode == 1 && UTOwner != NULL)
	{
		UTOwner->TargetEyeOffset.X = FiringViewKickback;
		UTOwner->TargetEyeOffset.Y = FiringViewKickbackY;
		AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
		if (PC != NULL)
		{
			PC->AddHUDImpulse(HUDViewKickback);
		}

		// try and play the sound if specified
		if (NumLoadedRockets > 0)
		{
			FTimerHandle TempHandle;
			GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &AUTWeap_RocketLauncher::PlayDelayedFireSound, 0.1f * NumLoadedRockets);
		}
		else
		{
			PlayDelayedFireSound();
		}

		if (ShouldPlay1PVisuals())
		{
			// try and play a firing animation if specified
			if (FiringAnimation.IsValidIndex(NumLoadedRockets) && FiringAnimation[NumLoadedRockets] != NULL)
			{
				UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
				if (AnimInstance != NULL)
				{
					AnimInstance->Montage_Play(FiringAnimation[NumLoadedRockets], 1.f);
				}
			}

			// try and play a firing animation for hands if specified
			if (FiringAnimationHands.IsValidIndex(NumLoadedRockets) && FiringAnimationHands[NumLoadedRockets] != NULL)
			{
				UAnimInstance* AnimInstanceHands = (GetUTOwner() && GetUTOwner()->FirstPersonMesh) ? GetUTOwner()->FirstPersonMesh->GetAnimInstance() : nullptr;
				if (AnimInstanceHands != NULL)
				{
					AnimInstanceHands->Montage_Play(FiringAnimationHands[NumLoadedRockets], 1.f);
				}
			}

			//muzzle flash for each loaded rocket
			if (RocketFireModes.IsValidIndex(CurrentRocketFireMode) && RocketFireModes[CurrentRocketFireMode].bCauseMuzzleFlash)
			{
				for (int32 i = 0; i < NumLoadedRockets; i++)
				{
					if (MuzzleFlash.IsValidIndex(i) && MuzzleFlash[i] != NULL && MuzzleFlash[i]->Template != NULL)
					{
						if (!MuzzleFlash[i]->bIsActive || MuzzleFlash[i]->Template->Emitters[0] == NULL ||
							IsLoopingParticleSystem(MuzzleFlash[i]->Template))
						{
							MuzzleFlash[i]->ActivateSystem();
						}
					}
				}
			}
		}
	}
	else
	{
		Super::PlayFiringEffects();
	}
}

AUTProjectile* AUTWeap_RocketLauncher::FireRocketProjectile()
{
	checkSlow(RocketFireModes.IsValidIndex(CurrentRocketFireMode) && RocketFireModes[CurrentRocketFireMode].ProjClass != NULL);

	TSubclassOf<AUTProjectile> RocketProjClass = nullptr;
	if (HasLockedTarget() && SeekingRocketClass)
	{
		RocketProjClass = SeekingRocketClass;
	}
	else if (bAllowGrenades)
	{
		RocketProjClass = RocketFireModes.IsValidIndex(CurrentRocketFireMode) ? RocketFireModes[CurrentRocketFireMode].ProjClass : nullptr;
	}
	else
	{
		RocketProjClass = ProjClass.IsValidIndex(CurrentRocketFireMode) ? ProjClass[CurrentRocketFireMode] : nullptr;
	}

	if (RocketProjClass == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Rocket fire mode %d No valid projectile class found"), CurrentRocketFireMode);
		return nullptr;
	}
	const FVector SpawnLocation = GetFireStartLoc();
	FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);

	FActorSpawnParameters Params;
	Params.Instigator = UTOwner;
	Params.Owner = UTOwner;

	AUTProjectile* ResultProj = NULL;

	switch (CurrentRocketFireMode)
	{
		case 0://rockets
		{
			if (NumLoadedRockets > 1)
			{
				FVector FireDir = SpawnRotation.Vector();
				FVector SideDir = (FireDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
				FireDir = FireDir + 0.01f*SideDir * float((NumLoadedRockets % 3) - 1.f) * (HasLockedTarget() ? SeekingLoadSpread : FullLoadSpread);
				SpawnRotation = FireDir.Rotation();
			}
			NetSynchRandomSeed(); 
			
			FVector Offset = (FMath::Sin(NumLoadedRockets*PI*0.667f)*FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::Z) + FMath::Cos(NumLoadedRockets*PI*0.667f)*FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::X)) * BarrelRadius * 2.f;
			ResultProj = SpawnNetPredictedProjectile(RocketProjClass, SpawnLocation + Offset, SpawnRotation);

			//Setup the seeking target
			AUTProj_Rocket* SpawnedRocket = Cast<AUTProj_Rocket>(ResultProj);
			if (HasLockedTarget() && SpawnedRocket)
			{
				SpawnedRocket->TargetActor = LockedTarget;
				TrackingRockets.AddUnique(SpawnedRocket);
			}

			break;
		}
		case 1://Grenades
		{
			float GrenadeSpread = GetSpread(0);
			float RotDegree = 360.0f / MaxLoadedRockets;
			SpawnRotation.Roll = RotDegree * MaxLoadedRockets;
			FRotator SpreadRot = SpawnRotation;
			SpreadRot.Yaw += GrenadeSpread*float(MaxLoadedRockets) - GrenadeSpread;
				
			AUTProjectile* SpawnedProjectile = SpawnNetPredictedProjectile(RocketProjClass, SpawnLocation, SpreadRot);
				
			if (SpawnedProjectile != nullptr)
			{
				//Spread the TossZ
				SpawnedProjectile->ProjectileMovement->Velocity.Z += (MaxLoadedRockets % 2) * GetSpread(2);
			}

			ResultProj = SpawnedProjectile;
			break;
		}
		default:
			UE_LOG(UT, Warning, TEXT("%s::FireRocketProjectile(): Invalid CurrentRocketFireMode"), *GetName());
			break;
		}

	NumLoadedRockets--;
	return ResultProj;
}

float AUTWeap_RocketLauncher::GetSpread(int32 ModeIndex)
{
	if (RocketFireModes.IsValidIndex(ModeIndex))
	{
		return RocketFireModes[ModeIndex].Spread;
	}
	return 0.0f;
}

// Target Locking Code
void AUTWeap_RocketLauncher::StateChanged()
{
	if (Role == ROLE_Authority && CurrentState != InactiveState && CurrentState != EquippingState && CurrentState != UnequippingState)
	{
		GetWorldTimerManager().SetTimer(UpdateLockHandle, this, &AUTWeap_RocketLauncher::UpdateLock, LockCheckTime, true);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(UpdateLockHandle);
	}

	//Clear loaded rockets and hide the HUD text when inactive
	if (CurrentState == InactiveState)
	{
		ClearLoadedRockets();
	}
}

bool AUTWeap_RocketLauncher::CanLockTarget(AActor *Target)
{
	//Make sure its not dead
	if (Target != NULL && !Target->bTearOff && !IsPendingKillPending())
	{
		AUTCharacter* UTP = Cast<AUTCharacter>(Target);

		//not same team
		return (UTP != NULL && (UTP->GetTeamNum() == 255 || UTP->GetTeamNum() != UTOwner->GetTeamNum()));
	}
	else
	{
		return false;
	}
}

bool AUTWeap_RocketLauncher::WithinLockAim(AActor *Target)
{
	if (CanLockTarget(Target))
	{
		const FVector FireLoc = UTOwner->GetPawnViewLocation();
		const FVector Dir = GetBaseFireRotation().Vector();
		const FVector TargetDir = (Target->GetActorLocation() - UTOwner->GetActorLocation()).GetSafeNormal();
		// note that we're not tracing to retain existing target; allows locking through walls to a limited extent
		return (FVector::DotProduct(Dir, TargetDir) > LockAim || UUTGameplayStatics::ChooseBestAimTarget(UTOwner->Controller, FireLoc, Dir, LockAim, LockRange, LockOffset, AUTCharacter::StaticClass()) == Target);
	}
	else
	{
		return false;
	}
}

void AUTWeap_RocketLauncher::OnRep_LockedTarget()
{
	SetLockTarget(LockedTarget);
}

void AUTWeap_RocketLauncher::OnRep_PendingLockedTarget()
{
	if (PendingLockedTarget != nullptr)
	{
		PendingLockedTargetTime = GetWorld()->GetTimeSeconds();
	}
	else
	{
		PendingLockedTargetTime = 0.f;
	}
}

void AUTWeap_RocketLauncher::SetLockTarget(AActor* NewTarget)
{
	LockedTarget = NewTarget;
	if (LockedTarget != NULL)
	{
		if (!bLockedOnTarget)
		{
			bLockedOnTarget = true;
			LastLockedOnTime = GetWorld()->TimeSeconds;
			if (GetNetMode() != NM_DedicatedServer && UTOwner != NULL && UTOwner->IsLocallyControlled() && Cast<AUTPlayerController>(UTOwner->GetController()))
			{
				Cast<AUTPlayerController>(UTOwner->GetController())->UTClientPlaySound(LockAcquiredSound);
			}
		}
	}
	else
	{
		if (bLockedOnTarget)
		{
			bLockedOnTarget = false;
			if (GetNetMode() != NM_DedicatedServer && UTOwner != NULL && UTOwner->IsLocallyControlled() && Cast<AUTPlayerController>(UTOwner->GetController()))
			{
				Cast<AUTPlayerController>(UTOwner->GetController())->UTClientPlaySound(LockLostSound);
			}
		}
	}
}

void AUTWeap_RocketLauncher::UpdateLock()
{
	if (Role != ROLE_Authority)
	{
		return;
	}
	if (UTOwner == NULL || UTOwner->Controller == NULL || UTOwner->IsFiringDisabled() || (CurrentFireMode != 1) || !IsFiring() || (NumLoadedRockets == 0))
	{
		SetLockTarget(NULL);
		return;
	}

	UUTWeaponStateFiringCharged* ChargeState = Cast<UUTWeaponStateFiringCharged>(CurrentState);
	if (ChargeState != NULL && !ChargeState->bCharging)
	{
		// rockets are being released, don't change lock at this time
		return;
	}

	const FVector FireLoc = UTOwner->GetPawnViewLocation();
	const FVector FireDir = GetBaseFireRotation().Vector();
	AActor* NewTarget = nullptr;
	
	AUTCharacter* LockedPawn = Cast<AUTCharacter>(LockedTarget);
	if (LockedPawn)
	{
		bool bKeepLock = false;
		if (!LockedPawn->IsDead())
		{
			// prioritize keeping same lock
			FVector AimDir = LockedPawn->GetActorLocation() - FireLoc;
			float TestAim = FireDir | AimDir;
			if (TestAim > 0.0f)
			{
				float FireDist = AimDir.SizeSquared();
				if (FireDist < FMath::Square(LockRange))
				{
					FireDist = FMath::Sqrt(FireDist);
					TestAim /= FireDist;
					if ((TestAim < LockAim) && (FireDist < 2.f*LockOffset))
					{
						AimDir.Z += LockedPawn->BaseEyeHeight;
						AimDir = AimDir.GetSafeNormal();
						TestAim = (FireDir | AimDir);
					}
					if (TestAim >= LockAim)
					{
						float OffsetDist = FMath::PointDistToLine(LockedPawn->GetActorLocation(), FireDir, FireLoc);
						if (OffsetDist < LockOffset)
						{
							// check visibility: try head, center, and actual fire line
							FCollisionQueryParams TraceParams(FName(TEXT("ChooseBestAimTarget")), false);
							bool bHit = GetWorld()->LineTraceTestByChannel(FireLoc, LockedPawn->GetActorLocation() + FVector(0.0f, 0.0f, LockedPawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()), COLLISION_TRACE_WEAPONNOCHARACTER, TraceParams);
							if (bHit)
							{
								bHit = GetWorld()->LineTraceTestByChannel(FireLoc, LockedPawn->GetActorLocation(), COLLISION_TRACE_WEAPONNOCHARACTER, TraceParams);
								if (bHit)
								{
									// try spot on capsule nearest to where shot is firing
									FVector ClosestPoint = FMath::ClosestPointOnSegment(LockedPawn->GetActorLocation(), FireLoc, FireLoc + FireDir*(FireDist + 500.f));
									FVector TestPoint = LockedPawn->GetActorLocation() + LockedPawn->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * (ClosestPoint - LockedPawn->GetActorLocation()).GetSafeNormal();
									float CharZ = LockedPawn->GetActorLocation().Z;
									float CapsuleHeight = LockedPawn->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
									TestPoint.Z = FMath::Clamp(ClosestPoint.Z, CharZ - CapsuleHeight, CharZ + CapsuleHeight);
									bHit = GetWorld()->LineTraceTestByChannel(FireLoc, TestPoint, COLLISION_TRACE_WEAPONNOCHARACTER, TraceParams);
								}
							}
							if (!bHit)
							{
								bKeepLock = true;
							}
						}
					}
				}
			}
		}
		if (!bKeepLock)
		{
			LockedPawn = nullptr;
		}
	}
	
	if (LockedPawn)
	{
		NewTarget = LockedPawn;
	}
	else
	{
		NewTarget = UUTGameplayStatics::ChooseBestAimTarget(UTOwner->Controller, FireLoc, FireDir, LockAim, LockRange, LockOffset, AUTCharacter::StaticClass());
	}

	//Have a target. Update the target lock
	if (LockedTarget != NULL)
	{
		//Still Valid LockTarget
		if (LockedTarget == NewTarget)
		{
			LastLockedOnTime = GetWorld()->TimeSeconds;
		}
		//Lost the Target
		else if ((LockTolerance + LastLockedOnTime) < GetWorld()->TimeSeconds)
		{
			SetLockTarget(NULL);
		}
	}
	//Have a pending target
	if (PendingLockedTarget != NULL && PendingLockedTarget == NewTarget)
	{
		//If we are looking at the target and its time to lock on
		if ((PendingLockedTargetTime + LockAcquireTime) < GetWorld()->TimeSeconds)
		{
			SetLockTarget(PendingLockedTarget);
			PendingLockedTarget = NULL;
		}
	}
	//Trace to see if we are looking at a new target
	else
	{
		PendingLockedTarget = NewTarget;
		PendingLockedTargetTime = (NewTarget != NULL) ? GetWorld()->TimeSeconds : 0.0f;
	}
}

void AUTWeap_RocketLauncher::DrawWeaponCrosshair_Implementation(UUTHUDWidget* WeaponHudWidget, float RenderDelta)
{
	float ScaledPadding = 50.f * WeaponHudWidget->GetRenderScale();
	//Draw the Rocket Firemode Text
	if (bDrawRocketModeString && RocketModeFont != NULL)
	{
		FText RocketModeText = RocketFireModes[CurrentRocketFireMode].DisplayString;
		float PosY = 0.3f * ScaledPadding;
		WeaponHudWidget->DrawText(RocketModeText, 0.0f, PosY, RocketModeFont, FLinearColor::Black, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Center, ETextVertPos::Top);
	}
	
	//Draw the crosshair
	Super::DrawWeaponCrosshair_Implementation(WeaponHudWidget, RenderDelta);

	// draw loaded rocket indicator
	float Scale = GetCrosshairScale(WeaponHudWidget->UTHUDOwner);
	if ((CurrentFireMode == 1) && (NumLoadedRockets > 0))
	{
		float DotSize = 12.f * Scale;
		WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 0.f, ScaledPadding, DotSize, DotSize, 894.f, 38.f, 26.f, 26.f, 1.f, FLinearColor::White, FVector2D(0.5f, 0.5f));
		if (NumLoadedRockets > 1)
		{
			WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 50.f*Scale, ScaledPadding, DotSize, DotSize, 894.f, 38.f, 26.f, 26.f, 1.f, FLinearColor::White, FVector2D(0.5f, 0.5f));
			if (NumLoadedRockets > 2)
			{
				WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, -50.f*Scale, ScaledPadding, DotSize, DotSize, 894.f, 38.f, 26.f, 26.f, 1.f, FLinearColor::White, FVector2D(0.5f, 0.5f));
			}
		}
	}

	//Draw the locked on crosshair
	if (LockCrosshairTexture)
	{
		float ResScaling = WeaponHudWidget->GetCanvas() ? WeaponHudWidget->GetCanvas()->ClipX / 1920.f : 1.f;
		float W = LockCrosshairTexture->GetSurfaceWidth();
		float H = LockCrosshairTexture->GetSurfaceHeight();
		float CrosshairRot = GetWorld()->TimeSeconds * 90.0f;
		const float AcquireDisplayTime = 0.6f;
		if (HasLockedTarget())
		{
			FVector ScreenTarget = WeaponHudWidget->GetCanvas()->Project(LockedTarget->GetActorLocation());
			ScreenTarget.X -= WeaponHudWidget->GetCanvas()->SizeX*0.5f;
			ScreenTarget.Y -= WeaponHudWidget->GetCanvas()->SizeY*0.5f;
			WeaponHudWidget->DrawTexture(LockCrosshairTexture, ScreenTarget.X, ScreenTarget.Y, W*ResScaling, H*ResScaling, 0.f, 0.f, W, H, 1.f, FLinearColor::Red, FVector2D(0.5f, 0.5f), CrosshairRot);
		}
		else if (PendingLockedTarget && (GetWorld()->GetTimeSeconds() - PendingLockedTargetTime > LockAcquireTime- AcquireDisplayTime))
		{
			FVector ScreenTarget = WeaponHudWidget->GetCanvas()->Project(PendingLockedTarget->GetActorLocation());
			ScreenTarget.X -= WeaponHudWidget->GetCanvas()->SizeX*0.5f;
			ScreenTarget.Y -= WeaponHudWidget->GetCanvas()->SizeY*0.5f;
			float Opacity = (GetWorld()->GetTimeSeconds() - PendingLockedTargetTime - LockAcquireTime + AcquireDisplayTime) / AcquireDisplayTime;
			float PendingScale = 1.f + 5.f * (1.f - Opacity);
			WeaponHudWidget->DrawTexture(LockCrosshairTexture, ScreenTarget.X, ScreenTarget.Y, W * PendingScale*ResScaling, H * PendingScale*ResScaling, 0.f, 0.f, W, H, 0.2f + 0.5f*Opacity, FLinearColor::White, FVector2D(0.5f, 0.5f), 2.5f*CrosshairRot);
		}

		for (int32 i = 0; i < TrackingRockets.Num(); i++)
		{
			if (TrackingRockets[i] && TrackingRockets[i]->MasterProjectile)
			{
				TrackingRockets[i] = Cast<AUTProj_Rocket>(TrackingRockets[i]->MasterProjectile);
			}
			if ((TrackingRockets[i] == nullptr) || TrackingRockets[i]->bExploded || TrackingRockets[i]->IsPendingKillPending() || (TrackingRockets[i]->TargetActor == nullptr) || TrackingRockets[i]->TargetActor->IsPendingKillPending())
			{
				TrackingRockets.RemoveAt(i, 1);
				i--;
			}
			else
			{
				FVector ScreenTarget = WeaponHudWidget->GetCanvas()->Project(TrackingRockets[i]->TargetActor->GetActorLocation());
				ScreenTarget.X -= WeaponHudWidget->GetCanvas()->SizeX*0.5f;
				ScreenTarget.Y -= WeaponHudWidget->GetCanvas()->SizeY*0.5f;
				WeaponHudWidget->DrawTexture(LockCrosshairTexture, ScreenTarget.X, ScreenTarget.Y, 2.f * W * ResScaling, 2.f * H * ResScaling, 0.f, 0.f, W, H, 1.f, FLinearColor::Red, FVector2D(0.5f, 0.5f), CrosshairRot);
			}
		}
	}
}

void AUTWeap_RocketLauncher::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTWeap_RocketLauncher, LockedTarget, COND_None);
	DOREPLIFETIME_CONDITION(AUTWeap_RocketLauncher, PendingLockedTarget, COND_None);
}

float AUTWeap_RocketLauncher::GetAISelectRating_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == NULL || B->GetEnemy() == NULL)
	{
		return BaseAISelectRating;
	}
	// if standing on a lift, make sure not about to go around a corner and lose sight of target
	// (don't want to blow up a rocket in bot's face)
	else if (UTOwner->GetMovementBase() != NULL && !UTOwner->GetMovementBase()->GetComponentVelocity().IsZero() && !B->CheckFutureSight(0.1f))
	{
		return 0.1f;
	}
	else
	{
		const FVector EnemyDir = B->GetEnemyLocation(B->GetEnemy(), false) - UTOwner->GetActorLocation();
		float EnemyDist = EnemyDir.Size();
		float Rating = BaseAISelectRating;

		// don't pick rocket launcher if enemy is too close
		if (EnemyDist < 900.0f)
		{
			// don't switch away from rocket launcher unless really bad tactical situation
			// TODO: also don't if OK with mutual death (high aggressiveness, high target priority, or grudge against target?)
			if (UTOwner->GetWeapon() == this && (EnemyDist > 550.0f || (UTOwner->Health < 50 && UTOwner->GetEffectiveHealthPct(false) < B->GetEnemyInfo(B->GetEnemy(), true)->EffectiveHealthPct)))
			{
				return Rating;
			}
			else
			{
				return 0.05f + EnemyDist * 0.00045;
			}
		}

		// rockets are good if higher than target, bad if lower than target
		float ZDiff = EnemyDir.Z;
		if (ZDiff < -250.0f)
		{
			Rating += 0.25;
		}
		else if (ZDiff > 350.0f)
		{
			Rating -= 0.35;
		}
		else if (ZDiff > 175.0f)
		{
			Rating -= 0.1;
		}

		// slightly higher chance to use against melee because high rocket momentum will keep enemy away
		AUTCharacter* EnemyChar = Cast<AUTCharacter>(B->GetEnemy());
		if (EnemyChar != NULL && EnemyChar->GetWeapon() != NULL && EnemyChar->GetWeapon()->bMeleeWeapon && EnemyDist < 5500.0f)
		{
			Rating += 0.1f;
		}

		return Rating;
	}
}

float AUTWeap_RocketLauncher::SuggestAttackStyle_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B != NULL && B->GetEnemy() != NULL)
	{
		// recommend backing off if target is too close
		float EnemyDist = (B->GetEnemyLocation(B->GetEnemy(), false) - UTOwner->GetActorLocation()).Size();
		if (EnemyDist < 1600.0f)
		{
			return (EnemyDist < 1100.0f) ? -1.5f : -0.7f;
		}
		else if (EnemyDist > 3500.0f)
		{
			return 0.5f;
		}
		else
		{
			return -0.1f;
		}
	}
	else
	{
		return -0.1f;
	}
}

bool AUTWeap_RocketLauncher::CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc)
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (Super::CanAttack_Implementation(Target, TargetLoc, bDirectOnly, bPreferCurrentMode, BestFireMode, OptimalTargetLoc))
	{
		if (!bPreferCurrentMode && B != NULL)
		{
			// prefer single rocket for visible enemy unless enemy is much stronger and want to try bursting
			BestFireMode = (FMath::FRand() < 0.3f || B->GetTarget() != B->GetEnemy() || (B->IsStopped() && Target == B->GetEnemy() && B->IsEnemyVisible(B->GetEnemy())) || B->RelativeStrength(B->GetEnemy()) <= 0.0f) ? 0 : 1;
		}
		return true;
	}
	else if (B == NULL)
	{
		return false;
	}
	else
	{
		if (GetWorld()->TimeSeconds - LastAttackSkillCheckTime < 1.0f)
		{
			LastAttackSkillCheckTime = GetWorld()->TimeSeconds;
			bAttackSkillCheckResult = B->WeaponProficiencyCheck() && FMath::FRand() > 0.5f;
		}
		if (!bAttackSkillCheckResult || B->GetTarget() != B->GetEnemy() || B->IsCharging() || (!IsPreparingAttack() && B->LostContact(3.0f))/* || !B->IsHunting()*/)
		{
			if (IsPreparingAttack() && bAttackSkillCheckResult)
			{
				BestFireMode = 1;
				if (NumLoadedRockets < MaxLoadedRockets - 1)
				{
					// don't modify aim yet, not close enough to firing
					return true;
				}
				else
				{
					// TODO: if high skill, look around for someplace to shoot rockets that won't blow self up
					return true;
				}
			}
			return false;
		}
		else
		{
			// consider firing standard rocket in case enemy is coming around corner soon
			// note: repeat of skill check is intentional
			if (!bPreferCurrentMode)
			{
				BestFireMode = (!B->LostContact(1.5f) && B->WeaponProficiencyCheck() && FMath::FRand() < 0.5f) ? 0 : 1;
			}
			const float MinDistance = 750.0f;

			if ( !PredicitiveTargetLoc.IsZero() && (PredicitiveTargetLoc - UTOwner->GetActorLocation()).Size() >= MinDistance &&
				!GetWorld()->LineTraceTestByChannel(UTOwner->GetActorLocation(), PredicitiveTargetLoc, ECC_Visibility, FCollisionQueryParams(FName(TEXT("PredictiveRocket")), false, UTOwner), WorldResponseParams) )
			{
				OptimalTargetLoc = PredicitiveTargetLoc;
				return true;
			}
			else
			{
				TArray<FVector> FoundPoints;
				B->GuessAppearancePoints(Target, TargetLoc, true, FoundPoints);
				if (FoundPoints.Num() == 0)
				{
					return false;
				}
				else
				{
					// find point that's far enough way to not blow self up
					int32 i = FMath::RandHelper(FoundPoints.Num());
					int32 StartIndex = i;
					do
					{
						i = (i + 1) % FoundPoints.Num();
						if ((FoundPoints[i] - UTOwner->GetActorLocation()).Size() >= MinDistance)
						{
							PredicitiveTargetLoc = FoundPoints[i];
							break;
						}
					} while (i != StartIndex);
					OptimalTargetLoc = PredicitiveTargetLoc;
					return true;
				}
			}
		}
	}
}

bool AUTWeap_RocketLauncher::IsPreparingAttack_Implementation()
{
	if (GracePeriod <= 0.0f)
	{
		return false;
	}
	else
	{
		// rocket launcher charge doesn't hold forever, so AI should make sure to fire before doing something else
		UUTWeaponStateFiringCharged* ChargeState = Cast<UUTWeaponStateFiringCharged>(CurrentState);
		return (ChargeState != NULL && ChargeState->bCharging);
	}
}

void AUTWeap_RocketLauncher::FiringExtraUpdated_Implementation(uint8 NewFlashExtra, uint8 InFireMode)
{
	if (InFireMode == 1)
	{
		int32 NewNumLoadedRockets;
		GetRocketFlashExtra(NewFlashExtra, InFireMode, NewNumLoadedRockets, CurrentRocketFireMode, bDrawRocketModeString);
		
		//Play the load animation
		if (NewNumLoadedRockets != NumLoadedRockets && GetNetMode() != NM_DedicatedServer)
		{
			NumLoadedRockets = NewNumLoadedRockets;

			if (NumLoadedRockets > 0)
			{
				UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
				if (AnimInstance != NULL && LoadingAnimation.IsValidIndex(NumLoadedRockets - 1) && LoadingAnimation[NumLoadedRockets - 1] != NULL)
				{
					AnimInstance->Montage_Play(LoadingAnimation[NumLoadedRockets - 1], LoadingAnimation[NumLoadedRockets - 1]->SequenceLength / GetLoadTime(NumLoadedRockets - 1));
				}

				UAnimInstance* AnimInstanceHands = (GetUTOwner() && GetUTOwner()->FirstPersonMesh) ? GetUTOwner()->FirstPersonMesh->GetAnimInstance() : nullptr;
				if (AnimInstanceHands != NULL && LoadingAnimationHands.IsValidIndex(NumLoadedRockets - 1) && LoadingAnimationHands[NumLoadedRockets - 1] != NULL)
				{
					AnimInstanceHands->Montage_Play(LoadingAnimationHands[NumLoadedRockets - 1], LoadingAnimationHands[NumLoadedRockets - 1]->SequenceLength / GetLoadTime(NumLoadedRockets - 1));
				}
			}
		}
	}
}

void AUTWeap_RocketLauncher::SetRocketFlashExtra(uint8 InFireMode, int32 InNumLoadedRockets, int32 InCurrentRocketFireMode, bool bInDrawRocketModeString)
{
	if (UTOwner != nullptr && Role == ROLE_Authority)
	{
		if (InFireMode == 0)
		{
			GetUTOwner()->SetFlashExtra(0, InFireMode);
		}
		else
		{
			uint8 NewFlashExtra = InNumLoadedRockets;
			if (bInDrawRocketModeString)
			{
				NewFlashExtra |= 1 << 7;
				NewFlashExtra |= InCurrentRocketFireMode << 4;
			}
			GetUTOwner()->SetFlashExtra(NewFlashExtra, InFireMode);
		}
	}
}

void AUTWeap_RocketLauncher::GetRocketFlashExtra(uint8 InFlashExtra, uint8 InFireMode, int32& OutNumLoadedRockets, int32& OutCurrentRocketFireMode, bool& bOutDrawRocketModeString)
{
	if (InFireMode == 1)
	{
		//High bit is whether or not we should display the the Rocket mode string
		if (InFlashExtra >> 7 > 0) 
		{
			bOutDrawRocketModeString = true;
			OutCurrentRocketFireMode = (InFlashExtra >> 4) & 0x07; //The next 3 bits is the rocket mode
		}
		//Low 4 bits is the number of rockets
		OutNumLoadedRockets = FMath::Min(InFlashExtra & 0x0F, MaxLoadedRockets);
	}
}

void AUTWeap_RocketLauncher::FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation)
{
	Super::FiringInfoUpdated_Implementation(InFireMode, FlashCount, InFlashLocation);
	CurrentRocketFireMode = 0;
	bDrawRocketModeString = false;
	NumLoadedRockets = 0;
	NumLoadedBarrels = 0;
}