// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponState.h"
#include "UTWeaponStateFiring.h"

void UUTWeaponStateFiring::BeginState(const UUTWeaponState* PrevState)
{
	GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(RefireCheckHandle, this, &UUTWeaponStateFiring::RefireCheckTimer, GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode()), true);
	ToggleLoopingEffects(true);
	PendingFireSequence = -1;
	bDelayShot = false;
	GetOuterAUTWeapon()->OnStartedFiring();
	FireShot();
	GetOuterAUTWeapon()->bNetDelayedShot = false;
}

void UUTWeaponStateFiring::EndState()
{
	bDelayShot = false;
	ToggleLoopingEffects(false);
	GetOuterAUTWeapon()->OnStoppedFiring();
	GetOuterAUTWeapon()->StopFiringEffects();
	if (GetOuterAUTWeapon()->GetUTOwner())
	{
		GetOuterAUTWeapon()->GetUTOwner()->ClearFiringInfo();
	}
	GetOuterAUTWeapon()->GetWorldTimerManager().ClearAllTimersForObject(this);
}

void UUTWeaponStateFiring::ToggleLoopingEffects(bool bNowOn)
{
	if (GetUTOwner() && GetOuterAUTWeapon()->FireLoopingSound.IsValidIndex(GetFireMode()) && GetOuterAUTWeapon()->FireLoopingSound[GetFireMode()] != NULL)
	{
		GetUTOwner()->SetAmbientSound(GetOuterAUTWeapon()->FireLoopingSound[GetFireMode()], !bNowOn);
	}
}

void UUTWeaponStateFiring::UpdateTiming()
{
	float FirstDelay = -1.0f;
	FTimerManager& TimerMgr = GetOuterAUTWeapon()->GetWorldTimerManager();
	if (TimerMgr.IsTimerActive(RefireCheckHandle))
	{
		FirstDelay = TimerMgr.GetTimerRemaining(RefireCheckHandle);
		// FirstDelay will be 0 if timer hit exactly
		if (FirstDelay > 0)
		{
			FirstDelay = FMath::Max(FirstDelay, GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode()));
		}
		else
		{
			FirstDelay = GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode());
		}
	}
	TimerMgr.SetTimer(RefireCheckHandle, this, &UUTWeaponStateFiring::RefireCheckTimer, GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode()), true, FirstDelay);
}

bool UUTWeaponStateFiring::WillSpawnShot(float DeltaTime)
{
	return (GetOuterAUTWeapon()->GetUTOwner()->IsPendingFire(GetOuterAUTWeapon()->GetCurrentFireMode())) && (GetOuterAUTWeapon()->GetWorldTimerManager().GetTimerRemaining(RefireCheckHandle) < DeltaTime);
}

void UUTWeaponStateFiring::RefireCheckTimer()
{
	// query bot to consider whether to still fire, switch modes, etc
	AUTBot* B = Cast<AUTBot>(GetUTOwner()->Controller);
	if (B != NULL)
	{
		B->CheckWeaponFiring();
	}

	if (GetUTOwner())
	{
		GetOuterAUTWeapon()->bNetDelayedShot = (GetUTOwner()->GetNetMode() == NM_DedicatedServer);
		if (PendingFireSequence >= 0)
		{
			bool bClearPendingFire = !GetUTOwner()->IsPendingFire(PendingFireSequence);
			GetUTOwner()->SetPendingFire(PendingFireSequence, true);
			if (GetOuterAUTWeapon()->HandleContinuedFiring())
			{
				FireShot();
			}
			if (bClearPendingFire && GetUTOwner() != NULL) // FireShot() could result in suicide!
			{
				GetUTOwner()->SetPendingFire(PendingFireSequence, false);
			}
			PendingFireSequence = -1;
		}
		else if (GetOuterAUTWeapon()->HandleContinuedFiring())
		{
			bDelayShot = GetOuterAUTWeapon()->bNetDelayedShot && !GetUTOwner()->DelayedShotFound() && Cast<APlayerController>(GetUTOwner()->GetController());
			if (!bDelayShot)
			{
				FireShot();
			}
		}
	}
	GetOuterAUTWeapon()->bNetDelayedShot = false;
}

void UUTWeaponStateFiring::HandleDelayedShot()
{
	if (bDelayShot)
	{
		GetOuterAUTWeapon()->bNetDelayedShot = true;
		bDelayShot = false;
		FireShot();
		GetOuterAUTWeapon()->bNetDelayedShot = false;
	}
}

void UUTWeaponStateFiring::Tick(float DeltaTime)
{
	HandleDelayedShot();
}

void UUTWeaponStateFiring::FireShot()
{
	//float CurrentMoveTime = (GetUTOwner() && GetUTOwner()->UTCharacterMovement) ? GetUTOwner()->UTCharacterMovement->GetCurrentSynchTime() : GetWorld()->GetTimeSeconds();
	//UE_LOG(UT, Warning, TEXT("Fire SHOT at %f (world time %f)"), CurrentMoveTime, GetWorld()->GetTimeSeconds());
	GetOuterAUTWeapon()->FireShot();
}

void UUTWeaponStateFiring::PutDown()
{
	HandleDelayedShot();

	// by default, firing states delay put down until the weapon returns to active via player letting go of the trigger, out of ammo, etc
	// However, allow putdown time to overlap with reload time - start a timer to do an early check
	float TimeTillPutDown = GetOuterAUTWeapon()->GetWorldTimerManager().GetTimerRemaining(RefireCheckHandle) * GetOuterAUTWeapon()->RefirePutDownTimePercent;
	if ( TimeTillPutDown <= GetOuterAUTWeapon()->GetPutDownTime() )
	{
		GetOuterAUTWeapon()->EarliestFireTime = GetWorld()->GetTimeSeconds() + TimeTillPutDown;
		Super::PutDown();
	}
	else
	{
		TimeTillPutDown -= GetOuterAUTWeapon()->GetPutDownTime();
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(PutDownHandle, this, &UUTWeaponStateFiring::PutDown, TimeTillPutDown, false);
	}
}

bool UUTWeaponStateFiring::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	// on server, might not be quite done reloading yet when client done, so queue firing
	if (bClientFired)
	{
		PendingFireSequence = FireModeNum;
		GetUTOwner()->NotifyPendingServerFire();
	}
	return false;
}
