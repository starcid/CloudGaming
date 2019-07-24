// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponState.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeaponStateUnequipping.h"
#include "ComponentRecreateRenderStateContext.h"
#include "Animation/AnimMontage.h"

void UUTWeaponStateUnequipping::BeginState(const UUTWeaponState* PrevState)
{
	const UUTWeaponStateEquipping* PrevEquip = Cast<UUTWeaponStateEquipping>(PrevState);

	// if was previously equipping, pay same amount of time to take back down
	UnequipTime = (PrevEquip != NULL) ? FMath::Min(PrevEquip->PartialEquipTime, GetOuterAUTWeapon()->GetPutDownTime()) : GetOuterAUTWeapon()->GetPutDownTime();
	UnequipTimeElapsed = 0.0f;
	if (UnequipTime <= 0.0f)
	{
		PutDownFinished();
	}
	else
	{
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(PutDownFinishedHandle, this, &UUTWeaponStateUnequipping::PutDownFinished, UnequipTime);
		GetOuterAUTWeapon()->PlayWeaponAnim(GetOuterAUTWeapon()->PutDownAnim, GetOuterAUTWeapon()->PutDownAnimHands, GetAnimLengthForScaling(GetOuterAUTWeapon()->PutDownAnim, GetOuterAUTWeapon()->PutDownAnimHands) / UnequipTime);
		
		if ((GetOuterAUTWeapon()->UTOwner != nullptr) && (GetOuterAUTWeapon()->LowerSound != nullptr))
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), GetOuterAUTWeapon()->LowerSound, GetOuterAUTWeapon()->UTOwner, SRT_None);
		}
	}
}

void UUTWeaponStateEquipping::BeginState(const UUTWeaponState* PrevState)
{
	const UUTWeaponStateUnequipping* PrevEquip = Cast<UUTWeaponStateUnequipping>(PrevState);
	// if was previously unequipping, pay same amount of time to bring back up
	EquipTime = (PrevEquip != NULL) ? FMath::Min(PrevEquip->PartialEquipTime, GetOuterAUTWeapon()->GetBringUpTime()) : GetOuterAUTWeapon()->GetBringUpTime();
	EquipTime = FMath::Max(EquipTime, GetOuterAUTWeapon()->EarliestFireTime - GetWorld()->GetTimeSeconds());

	PendingFireSequence = -1;
	if (EquipTime <= 0.0f)
	{
		BringUpFinished();
	}
	// else require StartEquip() to start timer/anim so overflow time (if any) can be passed in
}

void UUTWeaponStateEquipping::BringUpFinished()
{
	GetOuterAUTWeapon()->GotoActiveState();
	if (PendingFireSequence >= 0)
	{
		GetOuterAUTWeapon()->bNetDelayedShot = (GetOuterAUTWeapon()->GetNetMode() == NM_DedicatedServer);
		GetOuterAUTWeapon()->BeginFiringSequence(PendingFireSequence, true);
		PendingFireSequence = -1;
		GetOuterAUTWeapon()->bNetDelayedShot = false;
	}
}

void UUTWeaponStateEquipping::StartEquip(float OverflowTime)
{
	EquipTime -= OverflowTime;
	if (EquipTime <= 0.0f)
	{
		BringUpFinished();
	}
	else
	{
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(BringUpFinishedHandle, this, &UUTWeaponStateEquipping::BringUpFinished, EquipTime);
		GetOuterAUTWeapon()->PlayWeaponAnim(GetOuterAUTWeapon()->BringUpAnim, GetOuterAUTWeapon()->BringUpAnimHands, GetAnimLengthForScaling(GetOuterAUTWeapon()->BringUpAnim, GetOuterAUTWeapon()->BringUpAnimHands) / EquipTime);

		if ((GetOuterAUTWeapon()->UTOwner != nullptr) && (GetOuterAUTWeapon()->BringUpSound != nullptr))
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), GetOuterAUTWeapon()->BringUpSound, GetOuterAUTWeapon()->UTOwner, SRT_None);
		}

		if (GetOuterAUTWeapon()->GetNetMode() != NM_DedicatedServer && GetOuterAUTWeapon()->ShouldPlay1PVisuals() && GetOuterAUTWeapon()->GetUTOwner() && !GetOuterAUTWeapon()->GetUTOwner()->IsPendingKillPending())
		{
			// now that the anim is playing, force update first person meshes
			// this is necessary to avoid one frame artifacts since the meshes may have been set to not update while the weapon was down
			GetOuterAUTWeapon()->GetUTOwner()->FirstPersonMesh->TickAnimation(0.0f, false);
			GetOuterAUTWeapon()->GetUTOwner()->FirstPersonMesh->RefreshBoneTransforms();
			GetOuterAUTWeapon()->GetUTOwner()->FirstPersonMesh->UpdateComponentToWorld();
			GetOuterAUTWeapon()->Mesh->TickAnimation(0.0f, false);
			GetOuterAUTWeapon()->Mesh->RefreshBoneTransforms();
			GetOuterAUTWeapon()->Mesh->UpdateComponentToWorld();
			FComponentRecreateRenderStateContext ReregisterContext(GetOuterAUTWeapon()->GetUTOwner()->FirstPersonMesh);
			FComponentRecreateRenderStateContext ReregisterContext2(GetOuterAUTWeapon()->Mesh);
		}
	}
}

bool UUTWeaponStateEquipping::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	// on server, might not be quite done equipping yet when client done, so queue firing
	if (bClientFired)
	{
		PendingFireSequence = FireModeNum;
		GetUTOwner()->NotifyPendingServerFire();
	}
	return false;
}