// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeaponStateFiringCharged.h"
#include "UTWeaponStateFiringChargedRocket.generated.h"

/**
 * 
 */
UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTWeaponStateFiringChargedRocket : public UUTWeaponStateFiringCharged
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	AUTWeap_RocketLauncher* RocketLauncher;

	UUTWeaponStateFiringChargedRocket(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{}

	FTimerHandle GraceTimerHandle;
	FTimerHandle LoadTimerHandle;
	FTimerHandle FireLoadedRocketHandle;

	virtual void BeginState(const UUTWeaponState* PrevState) override
	{
		GetOuterAUTWeapon()->OnStartedFiring();
		if (GetUTOwner() == NULL || GetOuterAUTWeapon()->GetCurrentState() != this)
		{
			return;
		}

		RocketLauncher = Cast<AUTWeap_RocketLauncher>(GetOuterAUTWeapon());
		bCharging = true;

		//Only used for the rocket launcher
		if (RocketLauncher == NULL)
		{
			UE_LOG(UT, Warning, TEXT("%s::BeginState(): Weapon is not AUTWeap_RocketLauncher"));
			GetOuterAUTWeapon()->GotoActiveState();
			return;
		}

		RocketLauncher->SetRocketFlashExtra(GetFireMode(), 1, RocketLauncher->CurrentRocketFireMode, RocketLauncher->bDrawRocketModeString);
		RocketLauncher->BeginLoadRocket();
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(LoadTimerHandle, this, &UUTWeaponStateFiringChargedRocket::LoadTimer, RocketLauncher->GetLoadTime(RocketLauncher->NumLoadedRockets), false);
	}

	virtual void EndState() override
	{
		Super::EndState();

		// the super will only fire if there are rockets charged up, so we need to make sure to clear everything in case the loading never got that far
		ChargeTime = 0.0f;
		bCharging = false;
		RocketLauncher->NumLoadedRockets = 0;
		RocketLauncher->NumLoadedBarrels = 0;
		GetOuterAUTWeapon()->GetWorldTimerManager().ClearTimer(RefireCheckHandle);
		GetOuterAUTWeapon()->GetWorldTimerManager().ClearTimer(GraceTimerHandle);
		GetOuterAUTWeapon()->GetWorldTimerManager().ClearTimer(LoadTimerHandle);
	}

	virtual void LoadTimer()
	{
		if (!RocketLauncher)
		{
			return;
		}
		RocketLauncher->EndLoadRocket();

		if (!bCharging)
		{
			//Fire delay for shooting one alternate rocket
			EndFiringSequence(GetFireMode());
		}
		else if (RocketLauncher->NumLoadedBarrels >= RocketLauncher->MaxLoadedRockets)
		{
			//If we are fully loaded or out of ammo start the grace timer
			// since ammo consumption is not locally simulated the client won't know if it needs to stop loading rockets early and thus we need to tell it
			if (GetUTOwner() && !GetUTOwner()->IsLocallyControlled() && GetWorld()->GetNetMode() != NM_Client)
			{
				RocketLauncher->ClientAbortLoad();
			}
			if (!GetOuterAUTWeapon()->GetWorldTimerManager().IsTimerActive(GraceTimerHandle))
			{
				GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(GraceTimerHandle, this, &UUTWeaponStateFiringChargedRocket::GraceTimer, RocketLauncher->GracePeriod, false);
			}
		}
		else
		{
			RocketLauncher->BeginLoadRocket();
			GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(LoadTimerHandle, this, &UUTWeaponStateFiringChargedRocket::LoadTimer, RocketLauncher->GetLoadTime(RocketLauncher->NumLoadedRockets), false);
		}
	}

	virtual void RefireCheckTimer() override
	{
		// query bot to consider whether to still fire, switch modes, etc
		AUTBot* B = Cast<AUTBot>(GetUTOwner()->Controller);
		if (B != NULL)
		{
			B->CheckWeaponFiring();
		}
		// make sure AI code didn't result in bot blowing self up, etc
		if (GetUTOwner() != NULL)
		{
			if (GetOuterAUTWeapon()->HandleContinuedFiring())
			{
				bCharging = true;
				BeginState(this);
			}
		}
	}

	virtual void GraceTimer()
	{
		EndFiringSequence(GetFireMode());
	}

	virtual void UpdateTiming() override
	{
		Super::UpdateTiming();
		if (GetOuterAUTWeapon()->GetWorldTimerManager().IsTimerActive(LoadTimerHandle))
		{
			float RemainingPct = GetOuterAUTWeapon()->GetWorldTimerManager().GetTimerRemaining(LoadTimerHandle) / GetOuterAUTWeapon()->GetWorldTimerManager().GetTimerRate(LoadTimerHandle);
			GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(LoadTimerHandle, this, &UUTWeaponStateFiringChargedRocket::LoadTimer, RocketLauncher->GetLoadTime(RocketLauncher->NumLoadedRockets) * RemainingPct, false);
		}
	}

	virtual void EndFiringSequence(uint8 FireModeNum) override
	{
		if (FireModeNum == GetFireMode())
		{
			bCharging = false;
			if (RocketLauncher->NumLoadedRockets > 0)
			{
				AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
				if (GameState && (GameState->HasMatchEnded() || GameState->IsMatchIntermission()))
				{
					RocketLauncher->NumLoadedRockets = 0;
				}
				else
				{
					FireLoadedRocket();
				}
			}
		}
	}

	virtual void FireLoadedRocket()
	{
		AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
		if (GameState && (GameState->HasMatchEnded() || GameState->IsMatchIntermission()))
		{
			RocketLauncher->NumLoadedRockets = 0;

		}
		if (RocketLauncher->NumLoadedRockets > 0)
		{
			FireShot();
			if ((RocketLauncher->NumLoadedRockets > 0) && (RocketLauncher->BurstInterval <= 0.f))
			{
				// fire all remaining rockets
				while (RocketLauncher->NumLoadedRockets > 0)
				{
					FireShot();
				}
			}
		}
		if (RocketLauncher->NumLoadedRockets > 0)
		{
			if (RocketLauncher->ShouldFireLoad())
			{
				// fire all remaining rockets
				while (RocketLauncher->NumLoadedRockets > 0)
				{
					FireShot();
				}
			}
			else
			{
				GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(FireLoadedRocketHandle, this, &UUTWeaponStateFiringChargedRocket::FireLoadedRocket, (RocketLauncher->CurrentRocketFireMode == 0) ? RocketLauncher->BurstInterval : RocketLauncher->GrenadeBurstInterval, false);
			}
		}
		else
		{
			ChargeTime = 0.0f;
			if (GetOuterAUTWeapon()->GetCurrentState() == this)
			{
				GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(RefireCheckHandle, this, &UUTWeaponStateFiring::RefireCheckTimer, GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode()), false);
			}

			GetOuterAUTWeapon()->GetWorldTimerManager().ClearTimer(GraceTimerHandle);
			GetOuterAUTWeapon()->GetWorldTimerManager().ClearTimer(LoadTimerHandle);
		}
	}

	virtual void PutDown()
	{
		// don't process putdown while in the middle of burst fire
		if (!bCharging && !GetOuterAUTWeapon()->GetWorldTimerManager().IsTimerActive(FireLoadedRocketHandle) && !GetOuterAUTWeapon()->GetWorldTimerManager().IsTimerActive(GraceTimerHandle))
		{
			if (RocketLauncher->NumLoadedRockets > 0)
			{
				FireLoadedRocket();
				if (GetOuterAUTWeapon()->GetCurrentState() == this)
				{
					GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(RefireCheckHandle, this, &UUTWeaponStateFiring::RefireCheckTimer, GetOuterAUTWeapon()->GetRefireTime(GetOuterAUTWeapon()->GetCurrentFireMode()), false);
				}
			}
			else
			{
				Super::PutDown();
			}
		}
	}
};
