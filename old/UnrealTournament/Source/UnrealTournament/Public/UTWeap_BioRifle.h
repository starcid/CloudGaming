// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeapon.h"
#include "UTWeap_BioRifle.generated.h"

/**
 * 
 */
UCLASS(abstract)
class UNREALTOURNAMENT_API AUTWeap_BioRifle : public AUTWeapon
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	UAnimMontage* ChargeAnimation;

	/** maximum number of globs we can charge for the alternate fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	uint8 MaxGlobStrength;

	/** Delay between consuming globs for the alternate fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	float GlobConsumeTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	float SqueezeFireInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	float SqueezeFireSpread;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
		float SqueezeAmmoCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bio)
	TSubclassOf<AUTProjectile> SqueezeProjClass;

	virtual void UpdateSqueeze();

	virtual void GotoFireMode(uint8 NewFireMode) override;

	virtual bool HandleContinuedFiring() override;

	virtual void OnStartedFiring_Implementation() override;
	virtual void OnContinuedFiring_Implementation() override;
	virtual void OnStoppedFiring_Implementation() override;

	virtual void OnRep_Ammo() override;
	virtual void StartCharge();

	virtual void FireShot() override;
	
	FTimerHandle IncreaseGlobStrengthHandle;

	virtual void IncreaseGlobStrength();
	virtual void ClearGlobStrength();

	virtual void UpdateTiming() override;

	UPROPERTY(BlueprintReadOnly, Category = Bio)
	uint8 GlobStrength;

	/** hook to to play glob anim while charging*/
	UFUNCTION(BlueprintNativeEvent, Category = Bio)
	void OnStartCharging();

	/** hook to to play glob anim while charging*/
	UFUNCTION(BlueprintNativeEvent, Category = Bio)
	void OnChargeShot();

	virtual void FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation) override;

	bool HasAmmo(uint8 FireModeNum) override
	{
		return (Ammo >= 1);
	}

	virtual bool CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc) override;
	virtual float SuggestAttackStyle_Implementation() override;
	virtual float SuggestDefenseStyle_Implementation() override;
};
