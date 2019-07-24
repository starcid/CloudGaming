// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTProj_ShockBall.h"
#include "UTWeap_ShockRifle.h"
#include "UnrealNetwork.h"
#include "StatNames.h"
#include "UTRewardMessage.h"
#include "UTGameMode.h"
#include "UTProj_TransDisk.h"

AUTProj_ShockBall::AUTProj_ShockBall(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ComboDamageParams = FRadialDamageParams(220.0f, 500.0f);   
	ComboDamageParams.MinimumDamage = 50.f;
	ComboAmmoCost = 3;
	bComboExplosion = false;
	ComboMomentum = 330000.0f;
	bIsEnergyProjectile = true;
	PrimaryActorTick.bCanEverTick = true;
	bMoveFakeToReplicatedPos = false;
}

void AUTProj_ShockBall::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	if (InstigatorController && OwnBallEffect && (GetCachedScalabilityCVars().DetailMode > 0) && Cast<AUTPlayerController>(InstigatorController) && InstigatorController->IsLocalController())
	{
		OwnBallPSC = UGameplayStatics::SpawnEmitterAttached(OwnBallEffect, RootComponent);
	}
}

void AUTProj_ShockBall::InitFakeProjectile(AUTPlayerController* OwningPlayer)
{
	Super::InitFakeProjectile(OwningPlayer);
	TArray<USphereComponent*> Components;
	GetComponents<USphereComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		if (Components[i] != CollisionComp)
		{
			Components[i]->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
}

float AUTProj_ShockBall::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bFakeClientProjectile)
	{
		if (MasterProjectile && !MasterProjectile->IsPendingKillPending())
		{
			MasterProjectile->TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
		}
		return Damage;
	}
	if (ComboTriggerType != NULL && DamageEvent.DamageTypeClass != NULL && DamageEvent.DamageTypeClass->IsChildOf(ComboTriggerType))
	{
		if (Role != ROLE_Authority)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(EventInstigator);
			if (UTPC)
			{
				// verify that the beam that hit this was real
				AUTWeapon* FiringWeapon = Cast <AUTWeapon>(DamageCauser);
				if ((EventInstigator == InstigatorController) && UTPC->PlayerState)
				{
					float ImpliedSpawnTime = CreationTime - 0.001f * (UTPC->PlayerState->ExactPing + UTPC->PredictionFudgeFactor);
					float FireInterval = FiringWeapon ? FiringWeapon->GetRefireTime(1) : 0.6f;
					if (GetWorld()->GetTimeSeconds() - ImpliedSpawnTime < FireInterval - 0.1f)
					{
						// no combo - this shockball was spawned on the server de-synched from client firing beam
						SetActorEnableCollision(false);
						Destroy();

						// let weapon fire again, so this hit doesn't block shot
						if (FiringWeapon)
						{
							FiringWeapon->FireInstantHit();
						}
						return Damage;
					}
				}
				UTPC->ServerNotifyProjectileHit(this, GetActorLocation(), DamageCauser, GetWorld()->GetTimeSeconds());
				if (FiringWeapon != nullptr)
				{
					if (FiringWeapon->Ammo <= ComboAmmoCost)
					{
						FiringWeapon->Ammo = 0;
						FiringWeapon->HandleContinuedFiring();
						FiringWeapon->SwitchToBestWeaponIfNoAmmo();
					}
				}
			}
		}
		else if (GetNetMode() == NM_Standalone)
		{
			PerformCombo(EventInstigator, DamageCauser);
		}
	}

	return Damage;
}

void AUTProj_ShockBall::Destroyed()
{
	if (OwnBallPSC)
	{
		OwnBallPSC->DeactivateSystem();
		OwnBallPSC->bAutoDestroy = true;
		OwnBallPSC = nullptr;
	}
	Super::Destroyed();
}

void AUTProj_ShockBall::ShutDown()
{
	if (OwnBallPSC)
	{
		OwnBallPSC->DeactivateSystem();
		OwnBallPSC->bAutoDestroy = true;
		OwnBallPSC = nullptr;
	}
	Super::ShutDown();
}

bool AUTProj_ShockBall::ShouldIgnoreHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if (Super::ShouldIgnoreHit_Implementation(OtherActor, OtherComp))
	{
		return ((Cast<AUTProj_ShockBall>(OtherActor) == NULL) && (Cast<AUTProj_TransDisk>(OtherActor) == NULL));
	}
	return false;
}

void AUTProj_ShockBall::NotifyClientSideHit(AUTPlayerController* InstigatedBy, FVector HitLocation, AActor* DamageCauser, int32 Damage)
{
	TArray<USphereComponent*> Components;
	GetComponents<USphereComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		if (Components[i] != CollisionComp)
		{
			Components[i]->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
	// clamp movement to max prediction time
	// TODO: need to verify that projectile was really at that location - either here or in AUTPlayerController::ServerNotifyProjectileHit()
	const FVector Diff = HitLocation - GetActorLocation();
	const float MaxChange = ProjectileMovement->MaxSpeed * (InstigatedBy->MaxPredictionPing + InstigatedBy->PredictionFudgeFactor);
	if (Diff.SizeSquared() > FMath::Square<float>(MaxChange))
	{
		HitLocation = GetActorLocation() + Diff.GetSafeNormal() * MaxChange;
	}
	SetActorLocation(HitLocation);
	PerformCombo(InstigatedBy, DamageCauser);
}

void AUTProj_ShockBall::PerformCombo(class AController* InstigatedBy, class AActor* DamageCauser)
{
	//Consume extra ammo for the combo
	if (Role == ROLE_Authority)
	{
		AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
		AUTWeapon* Weapon = Cast<AUTWeapon>(DamageCauser);
		if (Weapon && (!GameMode || GameMode->bAmmoIsLimited || (Weapon->Ammo > 9)))
		{
			Weapon->AddAmmo(-ComboAmmoCost);
		}

		//This gets called before server startfire(). bPlayComboEffects = true will send the FireExtra when fired
		AUTCharacter* UTC = (InstigatedBy != nullptr) ? Cast<AUTCharacter>(InstigatedBy->GetPawn()) : nullptr;
		AUTWeap_ShockRifle* ShockRifle = (UTC != nullptr) ? Cast<AUTWeap_ShockRifle>(UTC->GetWeapon()) : nullptr;
		if (ShockRifle != nullptr)
		{
			ShockRifle->bPlayComboEffects = true;
		}
	}

	//The player who combos gets the credit
	InstigatorController = InstigatedBy;

	// Replicate combo and execute locally
	bComboExplosion = true;
	OnRep_ComboExplosion();
	Explode(GetActorLocation(), FVector(1.0f, 0.0f, 0.0f));
}

void AUTProj_ShockBall::OnRep_ComboExplosion()
{
	//Swap combo damage and effects
	DamageParams = ComboDamageParams;
	ExplosionEffects = NULL; // done via the vortex
	MyDamageType = ComboDamageType;
	Momentum = ComboMomentum;
}

void AUTProj_ShockBall::OnComboExplode_Implementation()
{
	if (GetNetMode() != NM_Client && ComboVortexType != NULL)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = Instigator;
		GetWorld()->SpawnActor<AUTPhysicsVortex>(ComboVortexType, GetActorLocation(), GetActorRotation(), Params);
	}
}

void AUTProj_ShockBall::Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp)
{
	if (!bExploded)
	{
		if (MyDamageType == ComboDamageType)
		{
			// special case for combo - get rid of fake projectile
			if (MyFakeProjectile != NULL)
			{
				// to make sure effects play consistently we need to copy over the LastRenderTime
				float FakeLastRenderTime = MyFakeProjectile->GetLastRenderTime();
				TArray<UPrimitiveComponent*> Components;
				GetComponents<UPrimitiveComponent>(Components);
				for (UPrimitiveComponent* Comp : Components)
				{
					Comp->LastRenderTime = FMath::Max<float>(Comp->LastRenderTime, FakeLastRenderTime);
				}

				MyFakeProjectile->Destroy();
				MyFakeProjectile = NULL;
			}
		}
		AUTPlayerController* PC = Cast<AUTPlayerController>(InstigatorController);
		AUTPlayerState* PS = PC ? PC->UTPlayerState : NULL;
		int32 ComboKillCount = PS ? PS->GetStatsValue(NAME_ShockComboKills) : 0;
		bPendingSpecialReward = bComboExplosion && PC && PS && ComboRewardMessageClass && (PC == InstigatorController);
		float ComboMovementScore = 0.f;
		if (bPendingSpecialReward)
		{
			AUTGameMode* GS = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if (GS)
			{
				ComboKillCount += GS->WarmupKills;
			}
			ComboMovementScore = RateComboMovement(PC);
			if (ComboMovementScore < 4.f)
			{
				bPendingSpecialReward = false;
			}
		}
		Super::Explode_Implementation(HitLocation, HitNormal, HitComp);
		if (bComboExplosion)
		{
			OnComboExplode();
			if (bPendingSpecialReward)
			{
				RateShockCombo(PC, PS, ComboKillCount, ComboMovementScore);
			}
		}

		// if bot is low skill, delay clearing bot monitoring so that it will occasionally fire for the combo slightly too late - a realistic player mistake
		AUTBot* B = Cast<AUTBot>(InstigatorController);
		if (IsPendingKillPending() || B == NULL || B->WeaponProficiencyCheck())
		{
			ClearBotCombo();
		}
		else
		{
			FTimerHandle TempHandle;
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTProj_ShockBall::ClearBotCombo, 0.2f, false);
		}
	}
}

float AUTProj_ShockBall::RateComboMovement(AUTPlayerController *PC)
{
	float ComboScore = 0.f;
	AUTCharacter* Shooter = Cast<AUTCharacter>(PC->GetPawn());
	if (Shooter && Shooter->GetWeapon())
	{
		// difference in angle between shots
		FVector ShootPos = Shooter->GetWeapon()->GetFireStartLoc();
		ComboScore += 100.f * (1.f - (GetVelocity().GetSafeNormal() | (GetActorLocation() - ShootPos).GetSafeNormal()));
		// current movement speed relative to direction, with bonus if falling
		float MovementBonus = (Shooter->GetCharacterMovement()->MovementMode == MOVE_Falling) ? 5.f : 3.f;
		ComboScore += MovementBonus * Shooter->GetVelocity().Size() / 1000.f;
		// @TODO FIXMESTEVE also score target movement?
	}
	return ComboScore;
}

void AUTProj_ShockBall::RateShockCombo(AUTPlayerController *PC, AUTPlayerState* PS, int32 OldComboKillCount, float ComboScore)
{
	int32 KillCount = (PS->GetStatsValue(NAME_ShockComboKills) - OldComboKillCount);
	AUTGameMode* GS = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (GS)
	{
		KillCount += GS->WarmupKills;
	}
	ComboScore += 4.f * FMath::Min(KillCount, 3);

	if ((ComboScore >= 8.f) && (KillCount > 0))
	{
		PS->ModifyStatsValue(NAME_AmazingCombos, 1);
		PS->AddCoolFactorMinorEvent();
		if (ComboScore > 11.f)
		{
			PC->SendPersonalMessage(ComboRewardMessageClass, 100);
		}
		else
		{
			PC->SendPersonalMessage(ComboRewardMessageClass, PS->GetStatsValue(NAME_AmazingCombos));
		}
	}

	float CurrentComboRating = PS->GetStatsValue(NAME_BestShockCombo);
	if (ComboScore > CurrentComboRating)
	{
		PS->SetStatsValue(NAME_BestShockCombo, ComboScore);
	}
}

void AUTProj_ShockBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearBotCombo();
}

void AUTProj_ShockBall::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTProj_ShockBall, bComboExplosion, COND_None);
}

void AUTProj_ShockBall::StartBotComboMonitoring()
{
	bMonitorBotCombo = true;
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AUTProj_ShockBall::ClearBotCombo()
{
	AUTBot* B = Cast<AUTBot>(InstigatorController);
	if (B != NULL)
	{
		if (B->GetTarget() == this)
		{
			B->SetTarget(NULL);
		}
		if (B->GetFocusActor() == this)
		{
			B->SetFocus(B->GetTarget());
		}
	}
	bMonitorBotCombo = false;
}

void AUTProj_ShockBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bMonitorBotCombo)
	{
		AUTBot* B = Cast<AUTBot>(InstigatorController);
		if (B != NULL)
		{
			AUTWeap_ShockRifle* Rifle = (B->GetUTChar() != NULL) ? Cast<AUTWeap_ShockRifle>(B->GetUTChar()->GetWeapon()) : NULL;
			if (Rifle != NULL && !Rifle->IsFiring())
			{
				switch (B->ShouldTriggerCombo(GetActorLocation(), GetVelocity(), ComboDamageParams))
				{
					case BMS_Abort:
						if (Rifle->ComboTarget == this)
						{
							Rifle->ComboTarget = NULL;
						}
						// if high skill, still monitor just in case
						bMonitorBotCombo = B->WeaponProficiencyCheck() && B->LineOfSightTo(this);
						break;
					case BMS_PrepareActivation:
						if (B->GetTarget() != this)
						{
							B->SetTarget(this);
							B->SetFocus(this);
						}
						break;
					case BMS_Activate:
						if (Rifle->ComboTarget == this)
						{
							Rifle->ComboTarget = NULL;
						}
						if (B->GetFocusActor() == this && !B->NeedToTurn(GetTargetLocation()))
						{
							Rifle->DoCombo();
						}
						else if (B->GetTarget() != this)
						{
							B->SetTarget(this);
							B->SetFocus(this);
						}
						break;
					default:
						break;
				}
			}
		}
	}
}