// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProjectileMovementComponent.h"
#include "UTProj_FlakShardMain.h"
#include "StatNames.h"
#include "UTRewardMessage.h"

AUTProj_FlakShardMain::AUTProj_FlakShardMain(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CenteredMomentumBonus = 0.f;
	CenteredDamageBonus = 0.0f;
	MaxBonusTime = 0.0f;
	MaxShreddedTime = 0.15f;
}

void AUTProj_FlakShardMain::DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(OtherActor);
	int32 OldHealth = UTC ? UTC->Health : 0;
	bPendingSpecialReward = (UTC && (UTC != Instigator) && Role == ROLE_Authority && Instigator != NULL && (InitialLifeSpan - GetLifeSpan() < 0.5f*MaxShreddedTime)); 
	Super::DamageImpactedActor_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);

	// check for camera shake
	if (bPendingSpecialReward)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(Instigator->Controller);
		if (PC && UTC->IsDead() && (UTC->TimeOfDeath == GetWorld()->GetTimeSeconds()))
		{
			if (ShortRangeKillShake)
			{
				PC->ClientPlayCameraShake(ShortRangeKillShake);
			}
			UTC->CloseFlakRewardMessageClass = CloseFlakRewardMessageClass;
			UTC->FlakShredStatName = NAME_FlakShreds;
			UTC->AnnounceShred(PC);
		}
		else if (PC)
		{
			UTC->FlakShredTime = GetWorld()->GetTimeSeconds();
			UTC->FlakShredInstigator = PC;
			UTC->CloseFlakRewardMessageClass = CloseFlakRewardMessageClass;
			UTC->FlakShredStatName = NAME_FlakShreds;
		}
	}
}

void AUTProj_FlakShardMain::OnBounce(const struct FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnBounce(ImpactResult, ImpactVelocity);

	// no damage/momentum bonus after bounce
	MaxBonusTime = 0.f;
}

/**
* Increase damage to UTPawns based on how centered this shard is on target.  If it is within the time MaxBonusTime time period.
* e.g. point blank shot with the flak cannon you will do mega damage.  Once MaxBonusTime passes then this shard becomes a normal shard.
*/
FRadialDamageParams AUTProj_FlakShardMain::GetDamageParams_Implementation(AActor* OtherActor, const FVector& HitLocation, float& OutMomentum) const
{
	FRadialDamageParams CalculatedParams = Super::GetDamageParams_Implementation(OtherActor, HitLocation, OutMomentum);

	// When hitting a pawn within bonus point blank time without a bounce
	AUTCharacter* OtherCharacter = Cast<AUTCharacter>(OtherActor);
	if (OtherCharacter && (MaxBonusTime > 0.f))
	{
		const float BonusPct = FMath::Min(1.f, 3.f * (CreationTime - GetWorld()->GetTimeSeconds() + MaxBonusTime) / MaxBonusTime);
		if (BonusPct > 0.0f)
		{
			// Apply bonus damage
			CalculatedParams.BaseDamage += CenteredDamageBonus * BonusPct;
			OutMomentum += CenteredMomentumBonus * BonusPct;
		}
	}

	return CalculatedParams;
}
