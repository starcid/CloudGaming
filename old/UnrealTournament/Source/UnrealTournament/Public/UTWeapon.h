	// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTInventory.h"
#include "UTProjectile.h"
#include "UTATypes.h"
#include "UTGameplayStatics.h"
#include "UTCrosshair.h"
#include "UTWeapon.generated.h"

USTRUCT(BlueprintType)
struct FInstantHitDamageInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	int32 Damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	TSubclassOf<UDamageType> DamageType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	float Momentum;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	float TraceRange;
	/** size of trace (radius of sphere); if <= 0, line trace is used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	float TraceHalfSize;
	/** if > 0.0, hit all targets in a cone with this dot angle (needs to also pass hitscan trace against world geo using TraceHalfSize) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageInfo")
	float ConeDotAngle;

	FInstantHitDamageInfo()
		: Damage(10), Momentum(0.0f), TraceRange(25000.0f), TraceHalfSize(0.0f), ConeDotAngle(0.0f)
	{}
};

USTRUCT()
struct FPendingFireEvent
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		bool bIsStartFire;

	UPROPERTY()
		uint8 FireModeNum;
	
	UPROPERTY()
		uint8 FireEventIndex;
		
	UPROPERTY()
		uint8 ZOffset;
	
	UPROPERTY()
		bool bClientFired; 
	
	FPendingFireEvent()
		: bIsStartFire(false), FireModeNum(0), FireEventIndex(0), ZOffset(0), bClientFired(false)
	{}

	FPendingFireEvent(bool bInStartFire, uint8 InFireModeNum, uint8 InFireEventIndex, uint8 InZOffset, bool bInClientFired) 
		: bIsStartFire(bInStartFire), FireModeNum(InFireModeNum), FireEventIndex(InFireEventIndex), ZOffset(InZOffset), bClientFired(bInClientFired) 
	{}
};

USTRUCT()
struct FDelayedProjectileInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<AUTProjectile> ProjectileClass;

	UPROPERTY()
	FVector SpawnLocation;

	UPROPERTY()
	FRotator SpawnRotation;

	FDelayedProjectileInfo()
		: ProjectileClass(NULL), SpawnLocation(ForceInit), SpawnRotation(ForceInit)
	{}
};

USTRUCT()
struct FDelayedHitScanInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector ImpactLocation;

	UPROPERTY()
	uint8 FireMode;

	UPROPERTY()
	FVector SpawnLocation;

	UPROPERTY()
	FRotator SpawnRotation;

	FDelayedHitScanInfo()
		: ImpactLocation(ForceInit), FireMode(0), SpawnLocation(ForceInit), SpawnRotation(ForceInit)
	{}
};

UENUM(BlueprintType)
namespace EZoomState
{
	enum Type
	{
		EZS_NotZoomed,
		EZS_ZoomingIn,
		EZS_ZoomingOut,
		EZS_Zoomed,
	};
}

USTRUCT(BlueprintType)
struct FZoomInfo
{
	GENERATED_USTRUCT_BODY()

	/** FOV angle at start of zoom, or zero to start at the camera's default FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float StartFOV;
	/** FOV angle at the end of the zoom, or zero to end at the camera's default FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float EndFOV;
	/** time to reach EndFOV from StartFOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float Time;
};

UCLASS(Blueprintable, Abstract, NotPlaceable, Config = Game)
class UNREALTOURNAMENT_API AUTWeapon : public AUTInventory
{
	GENERATED_UCLASS_BODY()

	friend class UUTWeaponState;
	friend class UUTWeaponStateInactive;
	friend class UUTWeaponStateActive;
	friend class UUTWeaponStateEquipping;
	friend class UUTWeaponStateUnequipping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = OnRep_AttachmentType, Category = "Weapon")
	TSubclassOf<class AUTWeaponAttachment> AttachmentType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = OnRep_Ammo, Category = "Weapon")
	int32 Ammo;

	UPROPERTY(BlueprintReadOnly)
		uint8 FireEventIndex;

	UFUNCTION()
	virtual void OnRep_AttachmentType();

	/** handles weapon switch when out of ammo, etc
	 * NOTE: called on server if owner is locally controlled, on client only when owner is remote
	 */
	UFUNCTION()
	virtual void OnRep_Ammo();
	virtual void SwitchToBestWeaponIfNoAmmo();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	int32 MaxAmmo;

	/** ammo cost for one shot of each fire mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<int32> AmmoCost;

	/** whether this weapon has a condition by which it recharges ammo automatically
	 * note that setting this doesn't enable any such property; it's just informing the internal weapon code that a subclass has implemented it to e.g. don't switch when out of ammo
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bCanRegenerateAmmo;

	/** projectile class for fire mode (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray< TSubclassOf<AUTProjectile> > ProjClass;

	/** instant hit data for fire mode (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<FInstantHitDamageInfo> InstantHitInfo;

	/** firing state for mode, contains core firing sequence and directs to appropriate global firing functions */
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = "Weapon", NoClear)
	TArray<class UUTWeaponStateFiring*> FiringState;

	/** True for melee weapons affected by "stopping power" (momentum added for weapons that don't normally impart much momentum) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bAffectedByStoppingPower;

	/** Whether Hitscan hits should do extra check for whether hit head sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		bool bCheckHeadSphere;

	/** Whether Hitscan hits should do extra check for whether hit head sphere when target is moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		bool bCheckMovingHeadSphere;

	/** Whether Hitscan hits should do extra check to ignore shockballs. (only works for traces, not sweeps) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		bool bIgnoreShockballs;

	/** Whether Hitscan shots are blocked by teammates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		bool bTeammatesBlockHitscan;

	/** Custom Momentum scaling for friendly hitscanned pawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FriendlyMomentumScaling;

	virtual	float GetImpartedMomentumMag(AActor* HitActor);

	virtual void Serialize(FArchive& Ar) override
	{
		// prevent AutoSwitchPriority from being serialized using non-config paths
		// without this any local user setting will get pushed to blueprints and then override other users' configuration
		float SavedSwitchPriority = AutoSwitchPriority;
		Super::Serialize(Ar);
		AutoSwitchPriority = SavedSwitchPriority;
	}

	/** Synchronize random seed on server and firing client so projectiles stay synchronized */
	virtual void NetSynchRandomSeed();

	/** socket to attach weapon to hands; if None, then the hands are hidden */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName HandsAttachSocket;

	/** time between shots, trigger checks, etc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.1"))
	TArray<float> FireInterval;
	/** firing spread (random angle added to shots) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<float> Spread;

	/** Scale Vertical portion of spread by this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float VerticalSpreadScaling;

	/** Clamp randomized Vertical spread scaling to this value, used with VerticalSpreadScaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float MaxVerticalSpread;

	/** First person, non attenuated sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TArray<USoundBase*> FPFireSound;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<USoundBase*> FireSound;

	/** Sound to play on shooter when weapon is fired.  This sound starts at the same time as the FireSound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<USoundBase*> ReloadSound;

	/** looping (ambient) sound to set on owner while firing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<USoundBase*> FireLoopingSound;

	/** sound played during reload if low on ammo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		USoundBase* LowAmmoSound;

	/** delay time from firing for low ammo sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float LowAmmoSoundDelay;

	/** ammo threshold for low ammo sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		int32 LowAmmoThreshold;

	FTimerHandle PlayLowAmmoSoundHandle;

	virtual void PlayLowAmmoSound();

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> FireAnimation;
	/** AnimMontage to play on hands each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> FireAnimationHands;
	/** particle component for muzzle flash */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray<UParticleSystemComponent*> MuzzleFlash;
	/** saved transforms of MuzzleFlash components used for UpdateWeaponHand() to know original values from the blueprint */
	UPROPERTY(Transient)
	TArray<FTransform> MuzzleFlashDefaultTransforms;
	/** saved sockets for MuzzleFlash components used for UpdateWeaponHand() to know original values from the blueprint */
	UPROPERTY(Transient)
	TArray<FName> MuzzleFlashSocketNames;
	/** particle system for firing effects (instant hit beam and such)
	 * particles will be sourced at FireOffset and a parameter HitLocation will be set for the target, if applicable
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray<UParticleSystem*> FireEffect;

	/** Max Distance to stretch weapon tracer. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
		float MaxTracerDist;

	/** Fire Effect happens once every FireEffectInterval shots */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	int32 FireEffectInterval;
	/** shots since last fire effect. */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 FireEffectCount;
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
	/** materials on weapon mesh first time we change its skin, used to preserve any runtime blueprint changes */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TArray<UMaterialInterface*> SavedMeshMaterials;

	/** If true, weapon is visibly holstered when not active.  There can only be one holstered weapon in inventory at a time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bMustBeHolstered;

	/** If true , weapon can be thrown. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	bool bCanThrowWeapon;

	/** if true, user has no voluntary movement while IsFiring() returns true
	 * and cannot start firing while in the air
	 * does not affect movement caused by outside control (knockback, jumppad, etc)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	bool bRootWhileFiring;

	/** if true, don't display in menus like the weapon priority menu (generally because the weapon's use is outside the user's control, e.g. instagib) */
	UPROPERTY(EditDefaultsOnly, Category = UI)
	bool bHideInMenus;

	/** if true, don't display in custom crosshair menu*/
	UPROPERTY(EditDefaultsOnly, Category = UI)
	bool bHideInCrosshairMenu;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText HighlightText;

	/** Hack for adjusting first person weapon mesh at different FOVs (until we have separate render pass for first person weapon. */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FVector FOVOffset;
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponRenderScale;

	UPROPERTY()
	UUTWeaponSkin* WeaponSkin;

	UFUNCTION(BlueprintCallable, Category = Effects)
	virtual void SetupSpecialMaterials();

	UFUNCTION()
	virtual void AttachToHolster();

	UFUNCTION()
	virtual void DetachFromHolster();

	virtual void DropFrom(const FVector& StartLocation, const FVector& TossVelocity) override;
	virtual bool StackLockerPickup(AUTInventory* ContainedInv) override;

	virtual void InitializeDroppedPickup(class AUTDroppedPickup* Pickup);

	/** Return true if this weapon should be dropped if held on player death. */
	virtual bool ShouldDropOnDeath();

	/** first person mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* Mesh;

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** causes weapons fire to originate from the center of the player's view when in first person mode (and human controlled)
	 * in other cases the fire start point defaults to the weapon's world position
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bFPFireFromCenter;
	/** if set ignore FireOffset for instant hit fire modes when in first person mode */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bFPIgnoreInstantHitFireOffset;
	/** Firing offset from weapon for weapons fire. If bFPFireFromCenter is true and it's a player in first person mode, this is from the camera center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector FireOffset;

	/** If true (on server), use the last bSpawnedShot saved position as starting point for this shot to synch with client firing position. */
	UPROPERTY()
	bool bNetDelayedShot;

	/** indicates this weapon is most useful in melee range (used by AI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	bool bMeleeWeapon;
	/** indicates AI should prioritize accuracy over evasion (low skill bots will stop moving, higher skill bots prioritize strafing and avoid actions that move enemy across view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	bool bPrioritizeAccuracy;
	/** indicates AI should target for splash damage (e.g. shoot at feet or nearby walls) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	bool bRecommendSplashDamage;
	/** indicates AI should consider firing at enemies that aren't currently visible but are close to being so
	 * generally set for rapid fire weapons or weapons with spinup/spindown costs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	bool bRecommendSuppressiveFire;
	/** indicates this is a sniping weapon (for AI, will prioritize headshots and long range targeting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	bool bSniping;

	/** Delayed projectile information */
	UPROPERTY()
	FDelayedProjectileInfo DelayedProjectile;

	/** Delayed hitscan information */
	UPROPERTY()
	FDelayedHitScanInfo DelayedHitScan;

	/** return whether to play first person visual effects
	 * not as trivial as it sounds as we need to appropriately handle bots (never actually 1P), first person spectating (can be 1P even though it's a remote client), hidden weapons setting (should draw even though weapon mesh is hidden), etc
	 */
	UFUNCTION(BlueprintCallable, Category = Effects)
	bool ShouldPlay1PVisuals() const;

	/** Play impact effects client-side for predicted hitscan shot - decides whether to delay because of high client ping. */
	virtual void PlayPredictedImpactEffects(FVector ImpactLoc);

	FTimerHandle PlayDelayedImpactEffectsHandle;

	/** Trigger delayed hitscan effects, delayed because client ping above max forward prediction limit. */
	virtual void PlayDelayedImpactEffects();

	FTimerHandle SpawnDelayedFakeProjHandle;

	/** Spawn a delayed projectile, delayed because client ping above max forward prediction limit. */
	virtual void SpawnDelayedFakeProjectile();

	/** time to bring up the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BringUpTime;
	/** time to put down the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float PutDownTime;
	/** scales refire put down time for the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RefirePutDownTimePercent;

	/** Earliest time can fire again (failsafe for weapon swapping). */
	UPROPERTY()
	float EarliestFireTime;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual float GetBringUpTime();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual float GetPutDownTime();

	/** equip anims */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* BringUpAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* BringUpAnimHands;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* PutDownAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* PutDownAnimHands;


	/** sound played when raising the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	USoundBase* BringUpSound;

	/** Sound played when lowering the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	USoundBase* LowerSound;
	
	/** weapon group - NextWeapon() picks the next highest group, PrevWeapon() the next lowest, etc
	 * generally, the corresponding number key is bound to access the weapons in that group
	 */
	UPROPERTY(Config, Transient, BlueprintReadOnly, Category = "Selection")
	int32 Group;

	/** Group this weapon was assigned to in past UTs when each weapon was in its own slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Selection")
	int32 DefaultGroup;

	/** if the player acquires more than one weapon in a group, we assign a unique GroupSlot to keep a consistent order
	 * this value is only set on clients
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Selection")
	int32 GroupSlot;

	/** Returns true if weapon follows OtherWeapon in the weapon list (used for nextweapon/previousweapon) */
	virtual bool FollowsInList(AUTWeapon* OtherWeapon);

	/** user set priority for auto switching and switch to best weapon functionality
	 * this value only has meaning on clients
	 */
	UPROPERTY(EditAnywhere, Config, Transient, BlueprintReadOnly, Category = "Selection")
	float AutoSwitchPriority;

	/** Deactivate any owner spawn protection */
	virtual void DeactivateSpawnProtection();

	/** whether this weapon stays around by default when someone picks it up (i.e. multiple people can pick up from the same spot without waiting for respawn time) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bWeaponStay;

	/** Base offset of first person mesh, cached from offset set up in blueprint. */
	UPROPERTY()
	FVector FirstPMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		FVector LowMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
		FVector VeryLowMeshOffset;

	/** Base relative rotation of first person mesh, cached from offset set up in blueprint. */
	UPROPERTY()
	FRotator FirstPMeshRotation;

	/** Scaling for Procedural 1st person weapon bob.  Set to zero to disable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponBob")
	float WeaponBobScaling;

	/**1st person firing view kickback in Z */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponBob")
	float FiringViewKickback;

	/**1st person firing view kickback to the side*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponBob")
		float FiringViewKickbackY;

	/**HUD recoil when firing weapon*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponBob")
		FVector2D HUDViewKickback;

	virtual void UpdateViewBob(float DeltaTime);

	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;

	UPROPERTY()
	bool bAttachingToOwner;

	virtual void RegisterAllComponents() override;

	virtual UMeshComponent* GetPickupMeshTemplate_Implementation(FVector& OverrideScale) const override;

	virtual void GotoState(class UUTWeaponState* NewState);
	/** notification of state change (CurrentState is new state)
	 * if a state change triggers another state change (i.e. within BeginState()/EndState()) this function will only be called once, when CurrentState is the final state
	 */
	virtual void StateChanged()
	{}

	/** firing entry point */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual void StartFire(uint8 FireModeNum);
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual void StopFire(uint8 FireModeNum);

	UPROPERTY()
		TArray<FPendingFireEvent> ResendFireEvents;

	FTimerHandle ResendFireHandle;

	virtual void ResendNextFireEvent();

	virtual void ClearFireEvents();

	/** Make sure that passed in FireEventIndex has not yet been processed (called on server). */
	virtual bool ValidateFireEventIndex(uint8 FireModeNum, uint8 InFireEventIndex);

	/** Queue up repeat RPCs to make sure FireEvent gets through. (called on client) */
	virtual void QueueResendFire(bool bIsStartFire, uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired);

	/** Tell server fire button was pressed.  bClientFired is true if client actually fired weapon. */
	UFUNCTION(Server, unreliable, WithValidation)
		virtual void ResendServerStartFire(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired);

	/** ServerStartFire, also pass Z offset since it is interpolating. */
	UFUNCTION(Server, unreliable, WithValidation)
		virtual void ResendServerStartFireOffset(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired);

	/** Tell server fire button was pressed.  bClientFired is true if client actually fired weapon. */
	UFUNCTION(Server, unreliable, WithValidation)
		virtual void ServerStartFire(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired);

	/** ServerStartFire, also pass Z offset since it is interpolating. */
	UFUNCTION(Server, unreliable, WithValidation)
	virtual void ServerStartFireOffset(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired);

	/** Send current fire settings to server. */
	UFUNCTION(Server, unreliable, WithValidation)
		virtual void ServerUpdateFiringStates(uint8 FireSettings);

	/** Just replicated ZOffset for shot fire location. */
	UPROPERTY()
		float FireZOffset;

	/** When received FireZOffset - only valid for same time and next frame. */
	UPROPERTY()
		float FireZOffsetTime;

	UFUNCTION(Server, unreliable, WithValidation)
	virtual void ServerStopFire(uint8 FireModeNum, uint8 InFireEventIndex);

	/** Used when client just triggered a fire on a held trigger right before releasing.*/
	UFUNCTION(Server, unreliable, WithValidation)
		virtual void ServerStopFireRecent(uint8 FireModeNum, uint8 InFireEventIndex);

	virtual bool BeginFiringSequence(uint8 FireModeNum, bool bClientFired);
	virtual void EndFiringSequence(uint8 FireModeNum);

	/** Returns true if weapon will fire a shot this frame - used for network synchronization */
	virtual bool WillSpawnShot(float DeltaTime);

	/** Returns true if weapon can fire again (fire button is pressed, weapon is held, has ammo, etc.). */
	virtual bool CanFireAgain();

	/** hook when the firing state starts; called on both client and server */
	UFUNCTION(BlueprintNativeEvent)
	void OnStartedFiring();

	/** hook for the return to active state (was firing, refire timer expired, trigger released and/or out of ammo)  */
	UFUNCTION(BlueprintNativeEvent)
	void OnStoppedFiring();

	/* Last time weapon fired because of held trigger*/
	UPROPERTY(BlueprintReadOnly, Category = Weapon)
		float LastContinuedFiring;

	/** Return true and  trigger effects if should continue firing, otherwise sends weapon to its active state */
	virtual bool HandleContinuedFiring();

	/** hook for when the weapon has fired, the refire delay passes, and the user still wants to fire (trigger still down) so the firing loop will repeat */
	UFUNCTION(BlueprintNativeEvent)
	void OnContinuedFiring();

	/** blueprint hook for pressing one fire mode while another is currently firing (e.g. hold alt, press primary)
	 * CurrentFireMode == current, OtherFireMode == one just pressed
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnMultiPress(uint8 OtherFireMode);

	/** activates the weapon as part of a weapon switch
	 * (this weapon has already been set to Pawn->Weapon)
	 * @param OverflowTime - overflow from timer of previous weapon PutDown() due to tick delta
	 */
	virtual void BringUp(float OverflowTime = 0.0f);
	/** puts the weapon away as part of a weapon switch
	 * return false to prevent weapon switch (must keep this weapon equipped)
	 */
	virtual bool PutDown();

	/**Hook to do effects when the weapon is raised*/
	UFUNCTION(BlueprintImplementableEvent)
	void OnBringUp();

	/** allows blueprint to prevent the weapon from being switched away from */
	UFUNCTION(BlueprintImplementableEvent)
	bool eventPreventPutDown();

	/** attach the visuals to Owner's first person view */
	UFUNCTION(BlueprintNativeEvent)
	void AttachToOwner();

	/** detach the visuals from the Owner's first person view */
	UFUNCTION(BlueprintNativeEvent)
	void DetachFromOwner();

	/** return number of fire modes */
	virtual uint8 GetNumFireModes() const
	{
		return FMath::Min3(255, FiringState.Num(), FireInterval.Num());
	}

	/** returns if the specified fire mode is a charged mode - that is, if the trigger is held firing will be delayed and the effect will improve in some way */
	virtual bool IsChargedFireMode(uint8 TestMode) const;

	virtual void GivenTo(AUTCharacter* NewOwner, bool bAutoActivate) override;
	virtual void ClientGivenTo_Internal(bool bAutoActivate) override;

	virtual void Removed() override;
	virtual void ClientRemoved_Implementation() override;

	/** fires a shot and consumes ammo */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void FireShot();

	/** blueprint override for firing
	 * NOTE: do an authority check before spawning projectiles, etc as this function is called on both sides
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
		bool FireShotOverride();

	/** blueprint override for handling instant hit fire result (damage, etc.). 
	@param Hit contains the hit result for the actor being damaged.
	@param FireDir is the direction of the trace. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
		void OnHitScanDamage(const FHitResult& Hit, FVector FireDir);

	/** plays an anim on the weapon and optionally hands
	 * automatically handles fire rate modifiers by default, overridden if RateOverride is > 0.0
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void PlayWeaponAnim(UAnimMontage* WeaponAnim, UAnimMontage* HandsAnim = NULL, float RateOverride = 0.0f);
	/** returns montage to play on the weapon for the specified firing mode */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual UAnimMontage* GetFiringAnim(uint8 FireMode, bool bOnHands = false) const;


	UPROPERTY()
		TEnumAsByte<ESoundAmplificationType> FireSoundAmp;

	virtual void PlayFiringSound(uint8 EffectFiringMode);

	/** play firing effects not associated with the shot's results (e.g. muzzle flash but generally NOT emitter to target) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void PlayFiringEffects();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void StopFiringEffects();

	/** blueprint hook to modify spawned instance of FireEffect (e.g. tracer or beam) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void ModifyFireEffect(UParticleSystemComponent* Effect);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void GetImpactSpawnPosition(const FVector& TargetLoc, FVector& SpawnLocation, FRotator& SpawnRotation);

	/** If true, don't spawn impact effect.  Used for hitscan hits, skips by default for pawn and projectile hits.
	 * note: this is called on the default object for weapon attachments (so they can share the code)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool CancelImpactEffect(const FHitResult& ImpactHit) const;

	/** play effects associated with the shot's impact given the impact point
	 * called only if FlashLocation has been set (instant hit weapon)
	 * Call GetImpactSpawnPosition() to set SpawnLocation and SpawnRotation
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void PlayImpactEffects(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation);

	/** shared code between UTWeapon and UTWeaponAttachment to refine replicated FlashLocation into impact effect transform via trace */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	static FHitResult GetImpactEffectHit(APawn* Shooter, const FVector& StartLoc, const FVector& TargetLoc);

	/** return start point for weapons fire */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	virtual FVector GetFireStartLoc(uint8 FireMode = 255);
	/** return base fire direction for weapons fire (i.e. direction player's weapon is pointing) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	virtual FRotator GetBaseFireRotation();
	/** return adjusted fire rotation after accounting for spread, aim help, and any other secondary factors affecting aim direction (may include randomized components) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "Weapon")
	FRotator GetAdjustedAim(FVector StartFireLoc);

	/** Returns UTPC controlling current target of this weapon. */
	virtual AUTPlayerController* GetCurrentTargetPC();

	/** if owned by a human, set AUTPlayerController::LastShotTargetGuess to closest target to player's aim */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void GuessPlayerTarget(const FVector& StartFireLoc, const FVector& FireDir);

	/** add (or remove via negative number) the ammo held by the weapon */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon")
	virtual void AddAmmo(int32 Amount);

	/** use up AmmoCost units of ammo for the current fire mode
	 * also handles triggering auto weapon switch if out of ammo
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon")
	virtual void ConsumeAmmo(uint8 FireModeNum);

	virtual void FireInstantHit(bool bDealDamage = true, FHitResult* OutHit = NULL);

	UFUNCTION(BlueprintCallable, Category = Firing)
	void K2_FireInstantHit(bool bDealDamage, FHitResult& OutHit);

	/** return true if Actor should be ignored by weapon traces in HitScanTrace() and FireCone() */
	virtual bool ShouldTraceIgnore(AActor* TestActor);

	/** Handles rewind functionality for net games with ping prediction */
	virtual void HitScanTrace(const FVector& StartLocation, const FVector& EndTrace, float TraceRadius, FHitResult& Hit, float PredictionTime);

	UFUNCTION(BlueprintCallable, Category = Firing)
	virtual AUTProjectile* FireProjectile();

	/** Spawn a projectile on both server and owning client, and forward predict it by 1/2 ping on server. */
	virtual AUTProjectile* SpawnNetPredictedProjectile(TSubclassOf<AUTProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(BlueprintCallable, Category = Firing)
	virtual void FireCone();

	/** returns whether we can meet AmmoCost for the given fire mode */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool HasAmmo(uint8 FireModeNum);

	/** returns whether we have ammo for any fire mode */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool HasAnyAmmo();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool CanSwitchTo();

	/** get interval between shots, including any fire rate modifiers */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual float GetRefireTime(uint8 FireModeNum);

	inline uint8 GetCurrentFireMode()
	{
		return CurrentFireMode;
	}

	inline void GotoActiveState()
	{
		GotoState(ActiveState);
		if (!GetWorldTimerManager().IsTimerActive(ResendFireHandle) && UTOwner && UTOwner->IsLocallyControlled())
		{
			GetWorldTimerManager().SetTimer(ResendFireHandle, this, &AUTWeapon::ResendNextFireEvent, 0.2f, true);
		}
	}

	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual void GotoFireMode(uint8 NewFireMode);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsFiring() const;

	virtual bool StackPickup_Implementation(AUTInventory* ContainedInv) override;

	/** update any timers and such due to a weapon timing change; for example, a powerup that changes firing speed */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual void UpdateTiming();

	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	/** we added an editor tool to allow the user to set the MuzzleFlash entries to a component created in the blueprint components view,
	 * but the resulting instances won't be automatically set...
	 * so we need to manually hook it all up
	 * static so we can share with UTWeaponAttachment
	 */
	static void InstanceMuzzleFlashArray(AActor* Weap, TArray<UParticleSystemComponent*>& MFArray);

	inline UUTWeaponState* GetCurrentState()
	{
		return CurrentState;
	}

	/** called on clients only when the local owner got a kill while holding this weapon
	 * NOTE: this weapon didn't necessarily cause the kill (previously fired projectile, etc), if you care check the damagetype
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void NotifyKillWhileHolding(TSubclassOf<UDamageType> DmgType);

	/** returns scaling of head for headshot effects
	 * NOTE: not used by base weapon implementation; stub is here for subclasses and firemodes that use it to avoid needing to cast to a specific weapon type
	 */
	virtual float GetHeadshotScale(AUTCharacter* HeadshotTarget) const
	{
		return 0.0f;
	}

	/** Return true if needs HUD ammo display widget drawn. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = HUD)
	bool NeedsAmmoDisplay() const;

	/** returns crosshair color taking into account user settings, red flash on hit, etc */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	FLinearColor GetCrosshairColor(UUTHUDWidget* WeaponHudWidget) const;

	/** The player state of the player currently under the crosshair */
	UPROPERTY()
	AUTPlayerState* TargetPlayerState;

	/** The time this player was last seen under the crosshaiar */
	float TargetLastSeenTime;

	/** Last result of GuessPlayerTarget(). */
	UPROPERTY()
		AUTCharacter* TargetedCharacter;

	/** returns whether we should draw the friendly fire indicator on the crosshair */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual bool ShouldDrawFFIndicator(APlayerController* Viewer, AUTPlayerState *& HitPlayerState ) const;

	/** Returns desired crosshair scale (affected by recent pickups */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	virtual float GetCrosshairScale(class AUTHUD* HUD);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = HUD)
	void DrawWeaponCrosshair(UUTHUDWidget* WeaponHudWidget, float RenderDelta);

	UFUNCTION()
	void UpdateCrosshairTarget(AUTPlayerState* NewCrosshairTarget, UUTHUDWidget* WeaponHudWidget, float RenderDelta);

	/** default parameters set on overlay particle effect (if any)
	 * up to the effect to care about them
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TArray<struct FParticleSysParam> OverlayEffectParams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Effects)
		UParticleSystem* UDamageOverrideEffect1P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Effects)
		UParticleSystem* UDamageOverrideEffect3P;

	/** helper for shared overlay code between UTWeapon and UTWeaponAttachment
	 * NOTE: can called on default object!
	 */
	virtual void UpdateOverlaysShared(AActor* WeaponActor, AUTCharacter* InOwner, USkeletalMeshComponent* InMesh, const TArray<struct FParticleSysParam>& InOverlayEffectParams, USkeletalMeshComponent*& InOverlayMesh) const;
	/** read WeaponOverlayFlags from owner and apply the appropriate overlay material (if any) */
	virtual void UpdateOverlays();

	virtual void UpdateOutline();

	/** set main skin override for the weapon, NULL to restore to default */
	virtual void SetSkin(UMaterialInterface* NewSkin);

	//*********
	// Rotation Lag/Lead

	/** Previous frame's weapon rotation */
	UPROPERTY()
	FRotator LastRotation;

	/** Saved values used for lagging weapon rotation */
	UPROPERTY()
	float	OldRotDiff[2];
	UPROPERTY()
	float	OldLeadMag[2];
	UPROPERTY()
	float	OldMaxDiff[2];

	/** Animated weapon lag scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
		float	AnimLagMultiplier;

	/** Animated weapon lag return speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
		float	AnimLagSpeedReturn;

	/** How fast Procedural Weapon Rotation offsets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
	float	RotChgSpeed; 

	/** How fast Procedural Weapon Rotation returns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
	float	ReturnChgSpeed;

	/** Max Procedural Weapon Rotation Yaw offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
	float	MaxYawLag;

	/** Max Procedural Weapon Rotation Pitch offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
	float	MaxPitchLag;

	/** If true, the weapon's rotation will procedurally lag behind the holder's rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Rotation")
		bool bProceduralLagRotation;

	/** @return whether the weapon's rotation is allowed to lag behind the holder's rotation */
	virtual bool ShouldLagRot();

	/** Lag a component of weapon rotation behind player's rotation. */
	virtual float LagWeaponRotation(float NewValue, float LastValue, float DeltaTime, float MaxDiff, int32 Index);

	/** called when initially attaching the first person weapon and when a camera viewing this player changes the weapon hand setting */
	virtual void UpdateWeaponHand();

	/** get weapon hand from the owning playercontroller, or spectating playercontroller if it's a client and a nonlocal player */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	EWeaponHand GetWeaponHand() const;

	/** Begin unequipping this weapon */
	virtual void UnEquip();

	virtual void GotoEquippingState(float OverflowTime);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	virtual bool IsUnEquipping() { return GetCurrentState() == UnequippingState; };

	/** informational function that returns the damage radius that a given fire mode has (used by e.g. bots) */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	float GetDamageRadius(uint8 TestMode) const;

	virtual float BotDesireability_Implementation(APawn* Asker, AController* RequestOwner, AActor* Pickup, float PathDistance) const;
	virtual float DetourWeight_Implementation(APawn* Asker, AActor* Pickup, float PathDistance) const;
	/** base weapon selection rating for AI
	 * this is often used to determine if the AI has a good enough weapon to not pursue further pickups,
	 * since GetAIRating() will fluctuate wildly depending on the combat scenario
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	float BaseAISelectRating;
	/** AI switches to the weapon that returns the highest rating */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	float GetAISelectRating();

	/** return a value from -1 to 1 for suggested method of attack for AI when holding this weapon, where < 0 indicates back off and fire from afar while > 0 indicates AI should advance/charge */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	float SuggestAttackStyle();
	/** return a value from -1 to 1 for suggested method of defense for AI when fighting a player with this weapon, where < 0 indicates back off and fire from afar while > 0 indicates AI should advance/charge */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	float SuggestDefenseStyle();

	/** called by the AI to ask if a weapon attack is being prepared; for example loading rockets or waiting for a shock ball to combo
	 * the AI uses this to know it shouldn't move around too much and should focus on its current target to avoid messing up the shot
	 */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	bool IsPreparingAttack();
	/** called by the AI to ask if it should avoid firing even if the weapon can currently hit its target (e.g. setting up a combo) */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	bool ShouldAIDelayFiring();

	/** returns whether this weapon has a viable attack against Target
	 * this function should not consider Owner's view rotation
	 * @param Target - Target actor
	 * @param TargetLoc - Target location, not guaranteed to be Target's true location (AI may pass a predicted or guess location)
	 * @param bDirectOnly - if true, only return success if weapon can directly hit Target from its current location (i.e. no need to wait for owner or target to move, no bounce shots, etc)
	 * @param bPreferCurrentMode - if true, only change BestFireMode if current value can't attack target but another mode can
	 * @param BestFireMode (in/out) - (in) current fire mode bot is set to use; (out) the fire mode that would be best to use for the attack
	 * @param OptimalTargetLoc (out) - best position to shoot at to hit TargetLoc (generally TargetLoc unless weapon has an indirect or special attack that doesn't require pointing at the target)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = AI)
	bool CanAttack(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, UPARAM(ref) uint8& BestFireMode, UPARAM(ref) FVector& OptimalTargetLoc);

	/** convenience redirect if the out params are not needed (just checking if firing is a good idea)
	 * would prefer to use pointer params but Blueprints don't support that
	 */
	inline bool CanAttack(AActor* Target, const FVector& TargetLoc, bool bDirectOnly)
	{
		uint8 UnusedFireMode;
		FVector UnusedOptimalLoc;
		return CanAttack(Target, TargetLoc, bDirectOnly, false, UnusedFireMode, UnusedOptimalLoc);
	}

	/** called by AI to try an assisted jump (e.g. impact jump or rocket jump)
	 * return true if an action was performed and the bot can continue along its path
	 */
	virtual bool DoAssistedJump()
	{
		return false;
	}

	/** returns the meshes used to represent this weapon in first person, for example so they can be hidden when viewing in 3p
	 * weapons that have additional relevant meshes (hands, dual wield, etc) should override to return those additional components
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Mesh)
	TArray<UMeshComponent*> Get1PMeshes() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation Toggles")
	bool bSecondaryIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation Toggles")
	bool bWallRunFire;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation Toggles")
	bool bIdleOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation Toggles")
	bool bIdleEmpty;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation Toggles")
	bool bIdleAlt;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idle_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idleOffset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idleEmpty_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idleAlt_offset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* idle_pose_zero;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idle_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idle_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idle_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idleOffset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* secondary_idleAlt_offset_pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* runForward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* runForward_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* runForward_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* jump;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* fall;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* fall_long;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* land;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* land_soft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* land_medium;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* land_heavy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* slide;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeForward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeForward_right;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* dodgeForward_left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_L_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_L_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_R_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_R_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_L_dodge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* wallRun_R_dodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	class UAimOffsetBlendSpace* lagAO;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	class UBlendSpace* leanBS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* inspect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* accent_A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* accent_B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* fidget_A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* fidget_B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimSequence* fidget_C;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	UUTWeaponState* CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	uint8 CurrentFireMode;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	UUTWeaponState* ActiveState;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	class UUTWeaponStateEquipping* EquippingState;
	UPROPERTY(Instanced, BlueprintReadOnly,  Category = "States")
	UUTWeaponState* UnequippingStateDefault;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	UUTWeaponState* UnequippingState;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	UUTWeaponState* InactiveState;

	/** overlay mesh for overlay effects */
	UPROPERTY()
	USkeletalMeshComponent* OverlayMesh;
	/** customdepth mesh for PP outline */
	UPROPERTY()
	USkeletalMeshComponent* CustomDepthMesh;

public:
	float WeaponBarScale;

	// The UV coords for this weapon when displaying it on the weapon bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FTextureUVs WeaponBarSelectedUVs;

	// The UV coords for this weapon when displaying it on the weapon bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FTextureUVs WeaponBarInactiveUVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName KillStatsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName AltKillStatsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName DeathStatsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName AltDeathStatsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName HitsStatsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		FName ShotsStatsName;

	virtual int32 GetWeaponKillStats(AUTPlayerState * PS) const;
	virtual int32 GetWeaponKillStatsForRound(AUTPlayerState * PS) const;
	virtual int32 GetWeaponDeathStats(AUTPlayerState * PS) const;
	virtual float GetWeaponShotsStats(AUTPlayerState * PS) const;
	virtual float GetWeaponHitsStats(AUTPlayerState * PS) const;

	// TEMP for testing 1p offsets
	UFUNCTION(exec)
	void TestWeaponLoc(float X, float Y, float Z);
	UFUNCTION(exec)
	void TestWeaponRot(float Pitch, float Yaw, float Roll = 0.0f);
	UFUNCTION(exec)
	void TestWeaponScale(float X, float Y, float Z);

	/** blueprint hook to modify team color materials */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void NotifyTeamChanged();

	UFUNCTION(BlueprintNativeEvent, Category = Weapon)
	void FiringInfoUpdated(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation);
	UFUNCTION(BlueprintNativeEvent, Category = Weapon)
	void FiringExtraUpdated(uint8 NewFlashExtra, uint8 InFireMode);
	UFUNCTION(BlueprintNativeEvent, Category = Weapon)
	void FiringEffectsUpdated(uint8 InFireMode, FVector InFlashLocation);

	//Zoom Stuff

	/**Used to reset the ZoomTime*/
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ZoomCount)
	uint8 ZoomCount;
	UFUNCTION()
	virtual void OnRep_ZoomCount();

	/**The state of the weapons zoom. Override OnRep_ZoomState to handle any state changes*/
	UPROPERTY(BlueprintReadOnly, Category = Zoom, Replicated, ReplicatedUsing = OnRep_ZoomState)
	TEnumAsByte<EZoomState::Type> ZoomState;
	UFUNCTION(BlueprintNativeEvent)
	void OnRep_ZoomState();

	/**Current zoom mode. An index into ZoomModes array*/
	UPROPERTY(BlueprintReadOnly, Category = Zoom, Replicated)
	uint8 CurrentZoomMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	TArray<FZoomInfo> ZoomModes;

	/**How long the weapon been zooming for*/
	UPROPERTY(BlueprintReadOnly, Category = Zoom, Replicated)
	float ZoomTime;

	/**Sets a new zoom mode. Index into ZoomModes*/
	UFUNCTION(BlueprintCallable, Category = Zoom)
	virtual void SetZoomMode(uint8 NewZoomMode);
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetZoomMode(uint8 NewZoomMode);
	virtual void LocalSetZoomMode(uint8 NewZoomMode);

	/**Sets the zoom state*/
	UFUNCTION(BlueprintCallable, Category = Zoom)
	virtual void SetZoomState(TEnumAsByte<EZoomState::Type> NewZoomState);
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetZoomState(uint8 NewZoomState);
	virtual void LocalSetZoomState(uint8 NewZoomState);

	/**Called when the weapon has zoomed in as far as it can go. Default is EZoomState::EZS_Zoomed*/
	UFUNCTION(BlueprintNativeEvent)
	void OnZoomedIn();

	/**Called when the weapon has zoomed all the way out. Default is EZoomState::EZS_NotZoomed*/
	UFUNCTION(BlueprintNativeEvent)
	void OnZoomedOut();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Zoom)
	UMaterialInstanceDynamic* GetZoomMaterial(uint8 FireModeNum) const;

	virtual void TickZoom(float DeltaTime);

	// The Speed modifier for this weapon if we are using weighted weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float WeightSpeedPctModifier;

	// At what ammo tick do we start warning the user we are getting low.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 AmmoWarningAmount;

	// At what ammo tick do we start warning the user we are in danger of running out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 AmmoDangerAmount;

	/** defines which weapon config info to use for this weapon.  Children can use their own custom tag or just leave it be to inheirt the parents */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponCustomizationTag;

	/** defines which type of weapon skins this weapon uses. Children can use their own custom tag or just leave it be to inheirt the parents */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSkinCustomizationTag;

	/** Holds a pointer to the current crosshair */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	UUTCrosshair* ActiveCrosshair;

	// The customization for this crosshair based on the customization info
	UPROPERTY(BlueprintReadOnly, Category = Crosshair)
	FWeaponCustomizationInfo ActiveCrosshairCustomizationInfo;

	/** Hitscan replication debugging. */
	UPROPERTY()
		bool bTrackHitScanReplication;

	UPROPERTY()
		AUTCharacter* HitScanHitChar;

	UPROPERTY()
		FVector_NetQuantize HitScanCharLoc;

	UPROPERTY()
		FVector_NetQuantize HitScanStart;

	UPROPERTY()
		FVector_NetQuantize HitScanEnd;

	UPROPERTY()
		float HitScanHeight;


	UPROPERTY()
		uint8 HitScanIndex;

	UPROPERTY()
		float HitScanTime;

	UPROPERTY()
		AUTCharacter* ReceivedHitScanHitChar;

	UPROPERTY()
		uint8 ReceivedHitScanIndex;

	UFUNCTION(Server, Unreliable, WithValidation)
		void ServerHitScanHit(AUTCharacter* HitScanChar, uint8 HitScanEventIndex);

	UFUNCTION (Client, Unreliable)
		void ClientMissedHitScan(FVector_NetQuantize MissedHitScanStart, FVector_NetQuantize MissedHitScanEnd, FVector_NetQuantize MissedHitScanLoc, float MissedHitScanTime, uint8 MissedHitScanIndex);
};