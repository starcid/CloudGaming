// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTWeaponAttachment.generated.h"

/** the third person representation of a weapon
 * not spawned on dedicated servers
 */
UCLASS(Blueprintable, NotPlaceable, Abstract)
class UNREALTOURNAMENT_API AUTWeaponAttachment : public AActor
{
	GENERATED_UCLASS_BODY()

protected:
	/** weapon class that resulted in this attachment; set at spawn time so no need to be reflexive here, just set AttachmentType in UTWeapon */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AUTWeapon> WeaponType;
	/** precast of Instigator to UTCharacter */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AUTCharacter* UTOwner;
	/** overlay mesh for overlay effects */
	UPROPERTY()
	USkeletalMeshComponent* OverlayMesh;
	/** custom depth mesh for outline rendering */
	UPROPERTY()
	USkeletalMeshComponent* CustomDepthMesh;
public:
	inline const USkeletalMeshComponent* GetCustomDepthMesh() const
	{
		return CustomDepthMesh;
	}

	/** Tells the aniamtion system what movement blends to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 WeaponStance;

	/** particle component for muzzle flash */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray<UParticleSystemComponent*> MuzzleFlash;

	/** third person mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* Mesh;
	/** third person mesh attach point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector AttachOffset;
	/** the attachment Mesh is also used for the pickup; set this override to non-zero to override the scale factor when used that way */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	FVector PickupScaleOverride;

	/** third person mesh attach point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName HolsterSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector HolsterOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FRotator HolsterRotation;

	/** If true, don't spawn impact effect.  Used for hitscan hits, skips by default for pawn and projectile hits. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CancelImpactEffect(const FHitResult& ImpactHit) const;

	/** particle system for firing effects (instant hit beam and such)
	* particles will be sourced at FireOffset and a parameter HitLocation will be set for the target, if applicable
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray<UParticleSystem*> FireEffect;
	/** optional effect for instant hit endpoint */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray< TSubclassOf<class AUTImpactEffect> > ImpactEffect;
	/** throttling for impact effects - don't spawn another unless last effect is farther than this away or longer ago than MaxImpactEffectSkipTime */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float ImpactEffectSkipDistance;
	/** throttling for impact effects - don't spawn another unless last effect is farther than ImpactEffectSkipDistance away or longer ago than this */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float MaxImpactEffectSkipTime;
	/** FlashLocation for last played impact effect */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	FVector LastImpactEffectLocation;
	/** last time an impact effect was played */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	float LastImpactEffectTime;
	/** if set, get impact effect from weapon class (most weapons use same instant hit impact for 1p and 3p) */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bCopyWeaponImpactEffect;

	/** materials on weapon mesh first time we change its skin, used to preserve any runtime blueprint changes */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TArray<UMaterialInterface*> SavedMeshMaterials;

	/** optional bullet whip sound when instant hit shots pass close by a local player without hitting */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	USoundBase* BulletWhip;

	/** maximum distance from fire line player can be and still get the bullet whip sound */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MaxBulletWhipDist;

	/** Time to delay bulletwhip to separate from firing sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float BulletWhipDelay;

	/** Save bullet path start and end for delayed bullet whip. */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
		FVector BulletWhipStart;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
		FVector BulletWhipEnd;

	UPROPERTY()
		TArray<AUTPlayerController*>  BulletWhipHearers;

	virtual void BeginPlay() override;
	virtual void RegisterAllComponents() override;
	virtual void Destroyed() override;

	UFUNCTION(BlueprintNativeEvent)
	void AttachToOwner();
	UFUNCTION(BlueprintNativeEvent)
	void DetachFromOwner();

	UFUNCTION(BlueprintNativeEvent)
	void HolsterToOwner();

	/** blueprint hook to modify spawned instance of FireEffect (e.g. tracer or beam) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void ModifyFireEffect(UParticleSystemComponent* Effect);

	/** play firing effects (both muzzle flash and any tracers/beams/impact effects)
	 * use UTOwner's FlashLocation and FireMode to determine firing data
	 * don't play sounds as those are played/replicated from UTWeapon by the server as the Pawn/WeaponAttachment may not be relevant
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void PlayFiringEffects();
	/** called when FlashExtra on the owner is changed
	 * some weapons may use this to display intermediate firing states (e.g. charging)
	 * or to provide extra data to a normal firing sequence (e.g. rocket type used for rocket launcher fire)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void FiringExtraUpdated();
	/** stops any looping fire effects */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void StopFiringEffects(bool bIgnoreCurrentMode = false);

	/** BP hook to firing effects */
	UFUNCTION(BlueprintImplementableEvent)
	bool OverrideFiringEffects();

	virtual void AttachToOwnerNative();

	virtual void HolsterToOwnerNative();

	/** default parameters set on overlay particle effect (if any)
	* up to the effect to care about them
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TArray<struct FParticleSysParam> OverlayEffectParams;

	/** read WeaponOverlayFlags from owner and apply the appropriate overlay material (if any) */
	virtual void UpdateOverlays();

	virtual void UpdateOutline(bool bOn, uint8 StencilValue);

	/** set main skin override for the weapon, NULL to restore to default */
	virtual void SetSkin(UMaterialInterface* NewSkin);

	/** Play bulletwhip (calls DelayedBulletWhip(). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void PlayBulletWhip();

	FTimerHandle BulletWhipHandle;

	/**  Actually plays bulletwhip sound, delayed by BulletWhipDelay*/
	virtual void DelayedBulletWhip();

	virtual void MarkComponentsAsPendingKill() override
	{
		// work around engine bug where components being destroyed try to calc bounding box and end up querying MasterPoseComponent in an invalid state
		if (OverlayMesh != NULL)
		{
			OverlayMesh->SetMasterPoseComponent(NULL);
		}
		Super::MarkComponentsAsPendingKill();
	}

	/** blueprint hook to modify team color materials */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void NotifyTeamChanged();
};