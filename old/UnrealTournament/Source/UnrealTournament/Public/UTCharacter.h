// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTCarriedObject.h"
#include "UTCharacterMovement.h"
#include "UTRecastNavMesh.h"
#include "UTHat.h"
#include "UTHatLeader.h"
#include "UTEyewear.h"

#include "UTCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterDiedSignature, class AController*, Killer, const class UDamageType*, DamageType);

class AUTDroppedHealth;
class AUTDroppedArmor;

/** Replicated movement data of our RootComponent.
* More efficient than engine's FRepMovement
*/
USTRUCT()
struct FRepUTMovement
{
	GENERATED_USTRUCT_BODY()

	/** @TODO FIXMESTEVE version that just replicates XY components, plus could quantize to tens easily */
	UPROPERTY()
	FVector_NetQuantize LinearVelocity;

	UPROPERTY()
	FVector_NetQuantize Location;

	/* Compressed acceleration direction: lowest 2 bits are forward/back, next 2 bits are left/right (-1, 0, 1) */
	UPROPERTY()
	uint8 AccelDir;

	UPROPERTY()
	FRotator Rotation;

	FRepUTMovement()
		: LinearVelocity(ForceInit)
		, Location(ForceInit)
		//, Acceleration(ForceInit)
		, Rotation(ForceInit)
	{}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		bool bOutSuccessLocal = true;

		// update location, linear velocity
		Location.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		Rotation.SerializeCompressed(Ar);
		LinearVelocity.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		Ar.SerializeBits(&AccelDir, 8);

		return true;
	}

	bool operator==(const FRepUTMovement& Other) const
	{
		if (LinearVelocity != Other.LinearVelocity)
		{
			return false;
		}

		if (Location != Other.Location)
		{
			return false;
		}

		if (Rotation != Other.Rotation)
		{
			return false;
		}
		if (AccelDir != Other.AccelDir)
		{
			return false;
		}
		return true;
	}

	bool operator!=(const FRepUTMovement& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** the amount of damage */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	int32 Damage;
	/** the location of the hit (relative to Pawn center) */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	FVector_NetQuantize RelHitLocation;
	/** how much momentum was imparted */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	FVector_NetQuantize Momentum;
	/** the damage type we were hit with */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	TSubclassOf<UDamageType> DamageType;
	/** shot direction pitch, manually compressed */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	uint8 ShotDirPitch;
	/** shot direction yaw, manually compressed */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	uint8 ShotDirYaw;
	/** number of near-simultaneous shots of same damage type
	 * NOTE: on the server, on-hit functions (and effects, if applicable) will get called for every hit regardless of this value
	 *		therefore, this value should generally only be considered on clients
	 */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	uint8 Count;
	/** if damage was partially or completely absorbed by inventory, the item that did so
	 * used to play different effects
	 */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	TSubclassOf<class AUTInventory> HitArmor;
};

/** ammo counter */
USTRUCT(BlueprintType)
struct FStoredAmmo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Ammo)
	TSubclassOf<class AUTWeapon> Type;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Ammo)
	int32 Amount;
};

USTRUCT(BlueprintType)
struct FSavedPosition
{
	GENERATED_USTRUCT_BODY()

	FSavedPosition() : Position(FVector(0.f)), Rotation(FRotator(0.f)), Velocity(FVector(0.f)), bTeleported(false), bShotSpawned(false), Time(0.f), TimeStamp(0.f) {};

	FSavedPosition(FVector InPos, FRotator InRot, FVector InVel, bool InTeleported, bool InShotSpawned, float InTime, float InTimeStamp) : Position(InPos), Rotation(InRot), Velocity(InVel), bTeleported(InTeleported), bShotSpawned(InShotSpawned), Time(InTime), TimeStamp(InTimeStamp) {};

	/** Position of player at time Time. */
	UPROPERTY()
	FVector Position;

	/** Rotation of player at time Time. */
	UPROPERTY()
	FRotator Rotation;

	/** Keep velocity also for bots to use in realistic reaction time based aiming error model. */
	UPROPERTY()
	FVector Velocity;

	/** true if teleport occurred getting to current position (so don't interpolate) */
	UPROPERTY()
	bool bTeleported;

	/** true if shot was spawned at this position */
	UPROPERTY()
	bool bShotSpawned;

	/** Current server world time when this position was updated. */
	float Time;

	/** Client timestamp associated with this position. */
	float TimeStamp;
};

USTRUCT(BlueprintType)
struct FFlashLocRep
{
	GENERATED_USTRUCT_BODY()

	/** Position of player at time Time. */
	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantize Position;

	/** Rotation of player at time Time. */
	UPROPERTY(BlueprintReadOnly)
	uint8 Count;
};

UENUM(BlueprintType)
enum EAllowedSpecialMoveAnims
{
	EASM_Any,
	EASM_UpperBodyOnly, 
	EASM_None,
};

UENUM(BlueprintType)
enum EMovementEvent
{
	EME_Jump,
	EME_Dodge,
	EME_WallDodge,
	EME_Slide,
};

USTRUCT(BlueprintType)
struct FMovementEventInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	TEnumAsByte<EMovementEvent> EventType;

	UPROPERTY(BlueprintReadOnly)
		uint8 EventCount;

	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantize EventLocation;
};

USTRUCT(BlueprintType)
struct FOverlayEffect
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* Particles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParticleAttachPoint;

	explicit FOverlayEffect(UMaterialInterface* InMaterial = NULL, UParticleSystem* InParticles = NULL, FName InParticleAttachPoint = NAME_None)
		: Material(InMaterial), Particles(InParticles), ParticleAttachPoint(InParticleAttachPoint)
	{}

	inline bool operator== (const FOverlayEffect& B) const
	{
		return Material == B.Material && Particles == B.Particles; // attach point irrelevant
	}
	inline bool IsValid() const
	{
		return Material != NULL || Particles != NULL;
	}
	inline FString ToString() const
	{
		return FString::Printf(TEXT("(Material=%s,Particles=%s)"), *GetFullNameSafe(Material), *GetFullNameSafe(Particles));
	}
};

USTRUCT(BlueprintType)
struct FPhysicalSoundResponse
{
	GENERATED_USTRUCT_BODY()

	/** the generic name of the sound that will be used to look up the audio */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	TEnumAsByte<EPhysicalSurface> SurfaceType;
	/** reference to the actual sound object */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* Sound;
	/** reference to the actual sound object */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* SoundOwner;
};

UCLASS(config=Game, collapsecategories, hidecategories=(Clothing,Lighting,AutoExposure,LensFlares,AmbientOcclusion,DepthOfField,MotionBlur,Misc,ScreenSpaceReflections,Bloom,SceneColor,Film,AmbientCubemap,AgentPhysics,Attachment,Avoidance,PlanarMovement,AI,Replication,Input,Actor,Tags,GlobalIllumination,CharacterData))
class UNREALTOURNAMENT_API AUTCharacter : public ACharacter, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	friend class UUTGhostComponent;
	friend void UUTCharacterMovement::PerformMovement(float DeltaSeconds);

	virtual void SetBase(UPrimitiveComponent* NewBase, const FName BoneName = NAME_None, bool bNotifyActor = true) override;

	//====================================
	// Networking

	virtual void PawnClientRestart() override;

	/** Used for replication of our RootComponent's position and velocity */
	UPROPERTY(ReplicatedUsing = OnRep_UTReplicatedMovement)
	struct FRepUTMovement UTReplicatedMovement;

	/** @TODO FIXMESTEVE Temporary different name until engine team makes UpdateSimulatedPosition() virtual */
	virtual void UTUpdateSimulatedPosition(const FVector & NewLocation, const FRotator & NewRotation, const FVector& NewVelocity);

	virtual void PostNetReceiveLocationAndRotation();

	/** True if character is currently wall sliding. */
	UPROPERTY(Category = "Wall Slide", BlueprintReadOnly, Replicated)
		bool bApplyWallSlide;

	/** UTReplicatedMovement struct replication event */
	UFUNCTION()
	virtual void OnRep_UTReplicatedMovement();

	/** Return synchronized time stamp for a shot. */
	virtual float GetCurrentSynchTime(bool bNetDelayedShot);

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	/** UTCharacter version of GatherMovement(), gathers into UTReplicatedMovement.  Return true if using UTReplicatedMovement rather than ReplicatedMovement */
	virtual bool GatherUTMovement();

	virtual void OnRep_ReplicatedMovement() override;

	/** Replicated function sent by client to server - contains client movement and view info. */
	UFUNCTION(unreliable, server, WithValidation)
	virtual void UTServerMove(float TimeStamp, FVector_NetQuantize InAccel, FVector_NetQuantize ClientLoc, uint8 CompressedMoveFlags, float ViewYaw, float ViewPitch, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode);

	/* Resending an (important) old move. Process it if not already processed. */
	UFUNCTION(unreliable, server, WithValidation)
	virtual void UTServerMoveOld(float OldTimeStamp, FVector_NetQuantize OldAccel, float OldYaw, uint8 OldMoveFlags);

	/** Replicated function sent by client to server - contains client movement for a pending move, but no expectation of correction. */
	UFUNCTION(unreliable, server, WithValidation)
	virtual void UTServerMoveQuick(float TimeStamp, FVector_NetQuantize InAccel, uint8 PendingFlags);

	/** Replicated function sent by client to server - contains client movement for an important pending move where rotation is needed. */
	UFUNCTION(unreliable, server, WithValidation)
	virtual void UTServerMoveSaved(float TimeStamp, FVector_NetQuantize InAccel, uint8 PendingFlags, float ViewYaw, float ViewPitch);

	//====================================

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class USphereComponent* FirstPersonMeshBoundSphere;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Mesh)
	class USkeletalMeshComponent* FirstPersonMesh;
	
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TArray<UMaterialInstanceDynamic*> FirstPersonMeshMIDs;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	class UCameraComponent* CharacterCameraComponent;

	/** Cached UTCharacterMovement casted CharacterMovement */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	class UUTCharacterMovement* UTCharacterMovement;
		
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	AUTHat* Hat;

	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	AUTHatLeader* LeaderHat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Pawn)
	TSubclassOf<AUTHatLeader> DefaultLeaderHatClass;

	UFUNCTION()
	virtual void SetHatClass(TSubclassOf<AUTHat> HatClass);
		
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	AUTEyewear* Eyewear;

	UFUNCTION()
	virtual void SetEyewearClass(TSubclassOf<AUTEyewear> EyewearClass);

	UPROPERTY()
	int32 HatVariant;

	UFUNCTION()
	virtual void SetHatVariant(int32 NewHatVariant);

	UPROPERTY()
	int32 EyewearVariant;

	UFUNCTION()
	virtual void SetEyewearVariant(int32 NewEyewearVariant);

	void SetCosmeticsFromPlayerState();

	UPROPERTY(BlueprintReadOnly, Category = Pawn, Replicated, ReplicatedUsing = OnRepCosmeticFlashCount)
	int32 CosmeticFlashCount;

	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float LastCosmeticFlashTime;

	UFUNCTION()
	virtual bool IsWearingAnyCosmetic();

	UFUNCTION()
	virtual void OnRepCosmeticFlashCount();

	UPROPERTY(BlueprintReadOnly, Category = Pawn, Replicated, ReplicatedUsing = OnRepCosmeticSpreeCount)
	int32 CosmeticSpreeCount;

	UFUNCTION()
	virtual void OnRepCosmeticSpreeCount();

	UFUNCTION(BlueprintCallable, Category=Weapon)
	virtual float GetWeaponBobScaling();

	virtual void SetEmoteSpeed(float NewEmoteSpeed);
	
	UFUNCTION(BlueprintCallable, Category = Taunt)
	void PlayTauntByClass(TSubclassOf<AUTTaunt> TauntToPlay, float EmoteSpeed = 1.0f);

	void ResetTaunt();

	UFUNCTION()
	void OnEmoteEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void PlayGroupTaunt(TSubclassOf<AUTGroupTaunt> TauntToPlay);

	UFUNCTION()
	void OnGroupTauntEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void CascadeGroupTaunt();

	UPROPERTY()
	TSubclassOf<AUTGroupTaunt> CurrentGroupTauntClass;

	UPROPERTY()
	UAnimMontage* CurrentGroupTaunt;

	UPROPERTY()
	UAnimMontage* CurrentTaunt;

	// Taunts should only play one sound at a time and cease when taunt is complete
	UPROPERTY()
	UAudioComponent* CurrentTauntAudioComponent;

	UPROPERTY()
	UAnimMontage* CurrentFirstPersonTaunt; 

	// Keep track of emote count so we can clear CurrentEmote
	UPROPERTY(BlueprintReadOnly, Category=Taunt)
	int32 TauntCount;

	UFUNCTION()
	bool IsThirdPersonTaunting() const;

	UPROPERTY()
		TArray<AUTPlayerState*> HealthRemovalAssists;

	UPROPERTY()
		TArray<AUTPlayerState*> ArmorRemovalAssists;


	/** Stored past positions of this player.  Used for bot aim error model, and for server side hit resolution. */
	UPROPERTY()
	TArray<FSavedPosition> SavedPositions;	

	/** Maximum interval to hold saved positions for. */
	UPROPERTY()
	float MaxSavedPositionAge;
	
	/** Mark the last saved position as where a shot was spawned so can synch firing to client position. */
	virtual void NotifyPendingServerFire();

	/** Called by CharacterMovement after movement */
	virtual void PositionUpdated(bool bShotSpawned);

	/** Returns this character's position PredictionTime seconds ago. */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual FVector GetRewindLocation(float PredictionTime, AUTPlayerController* DebugViewer=nullptr);

	/** Max time server will look back to found client synchronized shot position. */
	UPROPERTY(EditAnyWhere, Category = "Weapon")
	float MaxShotSynchDelay;

	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		class AUTGameVolume* LastGameVolume;

	/** Returns most recent position with bShotSpawned. */
	virtual FVector GetDelayedShotPosition();
	virtual FRotator GetDelayedShotRotation();

	/** Return true if there's a recent delayed shot */
	virtual bool DelayedShotFound();

	/** returns a simplified set of SavedPositions containing only the latest position for a given frame (i.e. each element has a unique Time)
	 * @param bStopAtTeleport - removes any positions prior to and including the most recent teleportation
	 */
	void GetSimplifiedSavedPositions(TArray<FSavedPosition>& OutPositions, bool bStopAtTeleport) const;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Pawn")
	bool bCanRally;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	bool bWasInWarningZone;

	protected:
	UPROPERTY(BlueprintReadWrite, Category = Pawn, ReplicatedUsing=OnArmorUpdated)
	int32 ArmorAmount;

	UPROPERTY()
	int32 OldArmorAmount;

	public:
	UFUNCTION()
	virtual void OnArmorUpdated();

	/** Limit to armor stacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pawn")
	int32 MaxStackedArmor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn", ReplicatedUsing = UpdateArmorOverlay)
	class AUTArmor* ArmorType;

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void GiveArmor(class AUTArmor* InArmorType);

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void RemoveArmor(int32 Amount);

	/** Returns current total armor amount. */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual int32 GetArmorAmount() const;

	/** Updates armor overlay to reflect current armor value. */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void UpdateArmorOverlay();

	/** return total effective health of this Pawn as a percentage/multiplier of its starting value
	 * this is used by AI as part of evaluating enemy strength
	 * if bOnlyVisible, only return results of values that can be obviously detected by seeing this Pawn (e.g. shield belt)
	 */
	virtual float GetEffectiveHealthPct(bool bOnlyVisible) const;

	/** counters of ammo for which the pawn doesn't yet have the corresponding weapon in its inventory */
	UPROPERTY()
	TArray<FStoredAmmo> SavedAmmo;

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void AddAmmo(const FStoredAmmo& AmmoToAdd);

	/** returns whether the character (via SavedAmmo, active weapon, or both) has the maximum allowed ammo for the passed in weapon */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual bool HasMaxAmmo(TSubclassOf<AUTWeapon> Type) const;

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual int32 GetAmmoAmount(TSubclassOf<AUTWeapon> Type) const;

	/** grant percentage of ammo for all held weapons
	 * bPctOfMax : true = percent is relative to weapon's MaxAmmo, false = percent is relative to weapon's starting ammo
	 */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void RestoreAmmoPct(float Pct, bool bPctOfMax = false);

	// Cheat, only works if called server side
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual	void AllAmmo();

	void ClientCheatWalk_Implementation() override;
	void ClientCheatFly_Implementation() override;
	void ClientCheatGhost_Implementation() override;

	// use this to iterate inventory
	template<typename> friend class TInventoryIterator;

	/** returns first inventory item in the chain
	* NOTE: usually you should use TInventoryIterator
	*/
	inline AUTInventory* GetInventory()
	{
		return InventoryList;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pawn|Inventory", meta = (DisplayName = "CreateInventory", AdvancedDisplay = "bAutoActivate"))
	virtual AUTInventory* K2_CreateInventory(TSubclassOf<AUTInventory> NewInvClass, bool bAutoActivate = true);
	
	template<typename InvClass>
	inline InvClass* CreateInventory(TSubclassOf<InvClass> NewInvClass, bool bAutoActivate = true)
	{
		InvClass* Result = (InvClass*)K2_CreateInventory(NewInvClass, bAutoActivate);
		checkSlow(Result == NULL || Result->IsA(InvClass::StaticClass()));
		return Result;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pawn")
	virtual bool AddInventory(AUTInventory* InvToAdd, bool bAutoActivate);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pawn")
	virtual void RemoveInventory(AUTInventory* InvToRemove);

	/** find an inventory item of a specified type */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual AUTInventory* K2_FindInventoryType(TSubclassOf<AUTInventory> Type, bool bExactClass = false) const;

	template<typename InvClass>
	inline InvClass* FindInventoryType(TSubclassOf<InvClass> Type, bool bExactClass = false) const
	{
		InvClass* Result = (InvClass*)K2_FindInventoryType(Type, bExactClass);
		checkSlow(Result == NULL || Result->IsA(InvClass::StaticClass()));
		return Result;
	}

	/** True if this character was falling when last took damage. */
	UPROPERTY()
		bool bWasFallingWhenDamaged;

	/** True during a translocator teleport, which has different telefragging rules. */
	UPROPERTY()
		bool bIsTranslocating;

	/** Return true if this character can block translocator telefrags. */
	virtual bool CanBlockTelefrags();

	/** toss an inventory item in the direction the player is facing
	 * (the inventory must have a pickup defined)
	 * ExtraVelocity is in the reference frame of the character (X is forward)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pawn")
	virtual void TossInventory(AUTInventory* InvToToss, FVector ExtraVelocity = FVector::ZeroVector);

	/** discards (generally destroys) all inventory items */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void DiscardAllInventory();
	
	/** call to propagate a named character event (jumping, firing, etc) to all inventory items with bCallOwnerEvent = true */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void InventoryEvent(FName EventName);

	virtual void PrepareForIntermission();

	/** switches weapons; handles client/server sync, safe to call on either side.  Uses classic groups, temporary until we have full weapon switching configurability menu. FIXMESTEVE */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void SwitchWeapon(AUTWeapon* NewWeapon);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pawn")
	void OnFirstPersonWeaponEquipped(AUTWeapon* EquippedWeapon);

	inline bool IsPendingFire(uint8 InFireMode) const
	{
		return !IsFiringDisabled() && (InFireMode < PendingFire.Num() && PendingFire[InFireMode] != 0);
	}

	/** blueprint accessor to what firemodes the player currently has active */
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool IsTriggerDown(uint8 InFireMode);

	/** sets the pending fire flag; generally should be called by whatever weapon processes the firing command, unless it's an explicit single shot */
	inline void SetPendingFire(uint8 InFireMode, bool bNowFiring)
	{
		if (PendingFire.Num() < InFireMode + 1)
		{
			PendingFire.SetNumZeroed(InFireMode + 1);
		}
		PendingFire[InFireMode] = bNowFiring ? 1 : 0;
	}

	inline void ClearPendingFire()
	{
		for (int32 i = 0; i < PendingFire.Num(); i++)
		{
			PendingFire[i] = 0;
		}
	}

	inline AUTWeapon* GetWeapon() const
	{
		return Weapon;
	}
	inline TSubclassOf<AUTWeapon> GetWeaponClass() const
	{
		// debug check to make sure this matches as expected
		checkSlow(GetNetMode() == NM_Client || (Weapon == NULL ? WeaponClass == NULL : ((UObject*)Weapon)->GetClass() == WeaponClass));
		
		return WeaponClass;
	}
	inline AUTWeapon* GetPendingWeapon() const
	{
		return PendingWeapon;
	}
	virtual void SetPendingWeapon(AUTWeapon* NewPendingWeapon);

	bool IsInInventory(const AUTInventory* TestInv) const;

	/** Used for spectating fired projectiles. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	class AUTProjectile* LastFiredProjectile;

	/** USed to store delayed auto switch if run over weapon while firing. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
		class AUTWeapon* PendingAutoSwitchWeapon;

	/** called by weapon being put down when it has finished being unequipped. Transition PendingWeapon to Weapon and bring it up 
	 * @param OverflowTime - amount of time past end of timer that previous weapon PutDown() used (due to frame delta) - pass onto BringUp() to keep things in sync
	 */
	virtual void WeaponChanged(float OverflowTime = 0.0f);

	/** called when the client's current weapon has been invalidated (removed from inventory, etc) */
	UFUNCTION(Client, Reliable)
	void ClientWeaponLost(AUTWeapon* LostWeapon);

	/** replicated weapon switch status (for animation). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Weapon")
	bool bIsSwitchingWeapon;

	/** replicated weapon firing info */
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = FiringInfoReplicated, Category = "Weapon")
	uint8 FlashCount;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Weapon")
	uint8 FireMode;
	/** weapon/attachment type specific extra data (for example, can be used to replicate charging without complicated FlashCount/FlashLocation gymnastics)
	 * note: setting this value goes through a different call chain and does NOT imply a shot
	 * if the information is needed to display the effects for an actual shot, set this prior to calling IncrementFlashCount() or SetFlashLocation()
	 */
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = FiringExtraUpdated, Category = "Weapon")
	uint8 FlashExtra;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = FiringInfoReplicated, Category = "Weapon")
	FFlashLocRep FlashLocation;

	/** Updated on client when FlashCount is replicated. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
		float LastWeaponFireTime;

	/** Set on successful link pull for other clients. */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Weapon")
		AActor* PulseTarget;

	/** set when client is locally simulating FlashLocation so ignore any replicated value */
	bool bLocalFlashLoc;

	/** set info for one instance of firing and plays firing effects; assumed to be a valid shot - call ClearFiringInfo() if the weapon has stopped firing
	 * if a location is not needed (projectile) call IncrementFlashCount() instead
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetFlashLocation(const FVector& InFlashLoc, uint8 InFireMode);

	/** set info for one instance of firing and plays firing effects; assumed to be a valid shot - call ClearFiringInfo() if the weapon has stopped firing
	* if a location is needed (instant hit, beam, etc) call SetFlashLocation() instead
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void IncrementFlashCount(uint8 InFireMode);

	/** set FlashExtra for indicating additional state that doesn't necessarily correspond to firing
	 * this uses a different WeaponAttachment notify path via FiringExtraUpdated(); since it may be e.g. charging instead of a shot, it doesn't call FiringInfoUpdated()
	 * this only triggers third person effects; first person aspects are assumed to be handled locally in UTWeapon
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetFlashExtra(uint8 NewFlashExtra, uint8 InFireMode);

	/** clears firing variables; i.e. because the weapon has stopped firing */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void ClearFiringInfo();

	/** called when firing variables are updated to trigger/stop effects */
	virtual void FiringInfoUpdated();
	/** blueprint hook into FiringInfoUpdated() - called AFTER normal weapon effects */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon", meta = (DisplayName = "Firing Info Updated"))
	void K2_FiringInfoUpdated();
	/** called when FlashExtra is updated; routes call to weapon attachment */
	UFUNCTION()
	virtual void FiringExtraUpdated();
	/** repnotify handler for firing variables, generally just calls FiringInfoUpdated() */
	UFUNCTION()
	virtual void FiringInfoReplicated();

	UPROPERTY(BlueprintReadWrite, Category = Pawn, ReplicatedUsing=OnHealthUpdated)
	int32 Health;

	UPROPERTY()
		int32 OldHealth;

	UFUNCTION()
		virtual void OnHealthUpdated();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	int32 HealthMax;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	int32 SuperHealthMax;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	bool bDamageHurtsHealth;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	bool bUnlimitedHealth;

	/** head bone/socket for headshots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	FName HeadBone;
	/** head Z offset from head bone */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	float HeadHeight;
	/** radius around head location that counts as headshot at 1.0 head scaling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	float HeadRadius;
	/** head scale factor (generally for use at runtime) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = Pawn)
	float HeadScale;

	/** multiplier to damage caused by this Pawn */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Pawn)
	float DamageScaling;
	
	/** accessors to FireRateMultiplier */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	float GetFireRateMultiplier();
	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetFireRateMultiplier(float InMult);
	UFUNCTION()
	void FireRateChanged();

	UPROPERTY(BlueprintReadWrite, Category = Pawn, Replicated, ReplicatedUsing=PlayTakeHitEffects)
	FTakeHitInfo LastTakeHitInfo;

	/** time of last SetLastTakeHitInfo() - authority only */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float LastTakeHitTime;

	/** last time LastTakeHitInfo was checked for replication; used to combine multiple hits into one LastTakeHitInfo */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float LastTakeHitReplicatedTime;

	/** Last time this character targeted or hit  an enemy. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		float LastTargetingTime;

	/** Last time this character was targeted or hit by an enemy. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		float LastTargetedTime;

	/** Last time this character was seen by an enemy that just targeted me. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		float LastTargetSeenTime;

	/** Last enemy to target or hit this character. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		AUTPlayerState* LastTargeter;

	/** Last enemy targeted or hit by this character. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		AUTCharacter* LastTarget;

	/** set true when target or keep eyes on enemy FC. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		bool bHaveTargetVisual;

	virtual void TargetedBy(APawn* Targeter, AUTPlayerState* PS);

	virtual void FlagPingedBy(AUTPlayerState* PS);


	/** Last time this character targeted or hit  an enemy. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Pawn)
		bool bIsInCombat;

	UPROPERTY(replicated)
	int32 VisibilityMask;

	/** Use 1-32 to enable a visibility channel */
	UFUNCTION(BlueprintCallable, Category = Pawn, BlueprintAuthorityOnly)
	void AddVisibilityMask(int32 Channel);
	/** Use 1-32 to disable a visibility channel */
	UFUNCTION(BlueprintCallable, Category = Pawn, BlueprintAuthorityOnly)
	void RemoveVisibilityMask(int32 Channel);

protected:
	/** indicates character is (mostly) invisible so AI only sees at short range, homing effects can't target the character, etc */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Invisible, Category = Pawn)
	bool bInvisible;
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void OnRep_Invisible();

	virtual void HideCharacter(bool bHideCharacter);

	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetInvisible(bool bNowInvisible);

	inline bool IsInvisible() const
	{
		return bInvisible;
	}

	/** whether spawn protection is currently applied.
	 * set to false after firing weapon or any other action that is considered offensive, or spawn protection time runs out
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Pawn)
	bool bSpawnProtectionEligible;

	UPROPERTY(BlueprintReadWrite, Category = Pawn)
		float SpawnProtectionStartTime;

	/** returns whether spawn protection currently applies for this character (valid on client) */
	UFUNCTION(BlueprintCallable, Category = Damage)
	bool IsSpawnProtected();

	/** set temporarily during client reception of replicated properties because replicated position and switches to ragdoll may be processed out of the desired order 
	 * when set, OnRep_ReplicatedMovement() will be called after switching to ragdoll
	 */
	bool bDeferredReplicatedMovement;

	/** set to prevent firing (does not stop already started firing, call StopFiring() for that) */
protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=UpdateWeaponFiring, Category = Pawn)
	bool bDisallowWeaponFiring;

	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		bool bDisallowWeaponSwitching;

public:
	/** allows disabling all weapon firing from this Pawn
	 * NOT replicated, must be called on both sides to work properly
	 */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void DisallowWeaponFiring(bool bDisallowed);

	UFUNCTION()
		virtual void UpdateWeaponFiring();

	/** Call after calling DissallowWeaponFiring(true) to re-enable weapon switching without enabling weapon firing.  NOT replicated. */
	UFUNCTION(BlueprintCallable, Category = Pawn)
		virtual void EnableWeaponSwitching();

	inline bool IsSwitchingDisabled() const
	{
		return bDisallowWeaponSwitching;
	}

	inline bool IsFiringDisabled() const
	{
		return bDisallowWeaponFiring;
	}

	/** Used to replicate bIsFloorSliding to non owning clients */
	UPROPERTY(ReplicatedUsing = OnRepFloorSliding)
	bool bRepFloorSliding;

	UFUNCTION()
	virtual void OnRepFloorSliding();

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual EAllowedSpecialMoveAnims AllowedSpecialMoveAnims();

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual float GetRemoteViewPitch();

	UPROPERTY(ReplicatedUsing = OnRepDrivenVehicle)
	APawn* DrivenVehicle;

	UFUNCTION()
	virtual void OnRepDrivenVehicle();

	virtual void StartDriving(APawn* Vehicle);
	virtual void StopDriving(APawn* Vehicle);

	virtual APhysicsVolume* GetPawnPhysicsVolume() const override
	{
		if (IsRagdoll() && RootComponent != NULL)
		{
			// prioritize root (ragdoll) volume over MovementComponent
			return RootComponent->GetPhysicsVolume();
		}
		else
		{
			return Super::GetPawnPhysicsVolume();
		}
	}

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void TurnOff() override;

	/** Return true if character is currently feigning death (Note: returns false within the recovery state) */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	virtual bool IsFeigningDeath() const;

	// AI hooks
	virtual void OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta) override;

protected:
	/** set when feigning death or other forms of non-fatal ragdoll (knockdowns, etc) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = PlayFeignDeath, Category = Pawn)
	bool bFeigningDeath;

	/** set during ragdoll recovery (still blending out physics, playing recover anim, etc) */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	bool bInRagdollRecovery;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* RagdollRecoveryMontage;

	/** top root speed during ragdoll, used for falling damage
	 * this is needed because the velocity will have changed by the time we get the physics notify so it's too late to work off that
	 */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float TopRagdollSpeed;

	/** Magnitude of impulse to push ragdoll around if fail to get up from feign. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	float FeignNudgeMag;

	/** Count failed attempts to unfeign, kill player if too many. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	int32 UnfeignCount;

	/* Used to limit how often impulses are applied to ragdolls, particularly when projectile is passing through it. */
	UPROPERTY(BlueprintReadWrite, Category = Pawn)
		float LastRagdollDamageTime;

public:
	/** Return true if character is in a ragdoll state */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	virtual bool IsRagdoll() const
	{
		return bFeigningDeath || bInRagdollRecovery || (RootComponent == GetMesh() && GetMesh()->IsSimulatingPhysics());
	}

	/** returns offset from this Pawn's Location (i.e. GetActorLocation()) to the center of its collision
	 * generally zero, but in ragdoll GetActorLocation() may not return the center depending on the physics setup
	 */
	virtual FVector GetLocationCenterOffset() const;

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void Restart() override;

	/** Called when PlayerCard is updated */
	virtual void PlayerCardUpdated();

	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override
	{
		// we want to allow zero damage (momentum only) hits so never pass 0 to super call
		return bTearOff || Super::ShouldTakeDamage(FMath::Max<float>(1.0f, Damage), DamageEvent, EventInstigator, DamageCauser);
	}

	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** returns location of head (origin of headshot zone); will force a skeleton update if mesh hasn't been rendered (or dedicated server) so the provided position is accurate */
	virtual FVector GetHeadLocation(float PredictionTime=0.f);

	/** checks for a head shot - called by weapons with head shot bonuses. */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual bool IsHeadShot(FVector HitLocation, FVector ShotDirection, float WeaponHeadScaling, AUTCharacter* ShotInstigator, float PredictionTime = 0.f);

	/** Returns true if if some armor effect prevents head shots.
	* if bConsumeArmor is true, the first item that prevents an otherwise valid head shot will be consumed
	*/
	UFUNCTION(BlueprintCallable, Category = Pawn)
		virtual bool BlockedHeadShot(FVector HitLocation, FVector ShotDirection, float WeaponHeadScaling, bool bConsumeArmor, AUTCharacter* ShotInstigator);

	/** Called when a headshot by this character is blocked. */
	virtual void HeadShotBlocked();

	/** Sound played locally when your headshot is blocked by armor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* HeadShotBlockedSound;

	/** Replicated to cause client-side head armor block effect. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn, Replicated, ReplicatedUsing = OnRepHeadArmorFlashCount)
	int32 HeadArmorFlashCount;

	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float LastHeadArmorFlashTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	UParticleSystem* HeadArmorHitEffect;

	virtual void NotifyBlockedHeadShot(AUTCharacter* ShotInstigator);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
		UParticleSystem* FirstPersonHealthEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
		UParticleSystem* ThirdPersonHealthEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
		UParticleSystem* FirstPersonArmorEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
		UParticleSystem* ThirdPersonArmorEffect;

	UFUNCTION(exec)
		void OV(FName InName, float value);
	UFUNCTION(exec)
		void OVV(FName InName, FVector value);

	UFUNCTION(exec)
		void JumpVis();
	
	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetHeadScale(float NewHeadScale);

	/** sends notification to any other server-side Actors (controller, etc) that need to know about being hit 
	AppliedDamage is the damage inflicted, Damage is the net damage taken after armor, etc. reductions. */
	virtual void NotifyTakeHit(AController* InstigatedBy, int32 AppliedDamage, int32 Damage, FVector Momentum, AUTInventory* HitArmor, const FDamageEvent& DamageEvent);

	/** Set LastTakeHitInfo from a damage event and call PlayTakeHitEffects() */
	virtual void SetLastTakeHitInfo(int32 AttemptedDamage, int32 Damage, const FVector& Momentum, AUTInventory* HitArmor, const FDamageEvent& DamageEvent);

	/** blood effects (chosen at random when spawning blood)
	 * note that these are intentionally split instead of a UTImpactEffect because the sound, particles, and decals are all handled with separate logic
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Effects)
	TArray<UParticleSystem*> BloodEffects;
	/** blood decal materials placed on nearby world geometry */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Effects)
	TArray<FBloodDecalInfo> BloodDecals;

	/** trace to nearest world geometry and spawn a blood decal at the hit location (if any) */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Effects)
	virtual void SpawnBloodDecal(const FVector& TraceStart, const FVector& TraceDir);

	/** last time ragdolling corpse spawned a blood decal */
	UPROPERTY(BlueprintReadWrite, Category = Effects)
	float LastDeathDecalTime;

	UPROPERTY(BlueprintReadWrite, Category = Efftects)
	float RagdollCollisionBleedThreshold;

	/** plays clientside hit effects using the data previously stored in LastTakeHitInfo */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void PlayTakeHitEffects();

	/** plays clientside damage effects.  NOTE: This is only called if the player takes actual damage  */
	UFUNCTION(blueprintNativeEvent, BlueprintCosmetic)
	void PlayDamageEffects();

	/** Time character died */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		float TimeOfDeath;

	/** Hack to accumulate flak shards for close kill - can also use for other multi-proj weapons. */
	UPROPERTY()
		float FlakShredTime;

	UPROPERTY()
		FName FlakShredStatName;

	UPROPERTY()
	class AUTPlayerController* FlakShredInstigator;

	/** Playerstate this pawn had when last possessed. */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		AUTPlayerState* OldPlayerState;

	/** Reward announcement for close up flak kill. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Announcement)
		TSubclassOf<class UUTRewardMessage> CloseFlakRewardMessageClass;

	virtual void AnnounceShred(class AUTPlayerController *PC);

	/** called when we die (generally, by running out of health)
	 *  SERVER ONLY - do not do visual effects here!
	 * return true if we can die, false if immortal (gametype effect, powerup, mutator, etc)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Meta = (DisplayName = "Died"), Category = Pawn)
	bool K2_Died(AController* EventInstigator, TSubclassOf<UDamageType> DamageType);
	virtual bool Died(AController* EventInstigator, const FDamageEvent& DamageEvent, AActor* DamagerCauser=nullptr);

	/** blueprint override for FellOutOfWorld()
	 * if you return false, make sure to move the Pawn somewhere valid or you are likely to get spammed with this call
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Pawn)
	bool OverrideFellOutOfWorld(TSubclassOf<UDamageType> DamageType);

	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	/** time between StopRagdoll() call and when physics has been fully blended out of our mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	float RagdollBlendOutTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	float RagdollPhysicsBlendOutTime;

	UPROPERTY()
	float RagdollBlendOutTimeLeft;

	/** player in feign death can't recover until world time reaches this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	float FeignDeathRecoverStartTime;

	virtual void StartRagdoll();
	virtual void StopRagdoll();

	// Prevent re-entry into StartRagdoll()
	UPROPERTY()
	bool bStartingRagdoll;

	FTimerHandle DeathSoundHandle;

	virtual void PlayDeathSound();

protected:
	UPROPERTY(BlueprintReadOnly, Category = Ragdoll)
	float RagdollGravityScale;
public:
	inline float GetRagdollGravityScale() const
	{
		return RagdollGravityScale;
	}
	/** sets gravity for the ragdoll (used by some death effects) */
	UFUNCTION(BlueprintCallable, Category = Ragdoll)
	virtual void SetRagdollGravityScale(float NewScale);

	/** Return true to prevent feigning. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Pawn)
	bool BlockFeignDeath();

	UFUNCTION(Exec, BlueprintCallable, Category = Pawn)
	virtual void FeignDeath();
	UFUNCTION(Reliable, Server, WithValidation, Category = Pawn)
	void ServerFeignDeath();
	UFUNCTION()
	virtual void PlayFeignDeath();
	/** force feign death/ragdoll state for e.g. knockdown effects */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Pawn)
	virtual void ForceFeignDeath(float MinRecoveryDelay);

	/** Updates Pawn's rotation to the given rotation, assumed to be the Controller's ControlRotation. Respects the bUseControllerRotation* settings. */
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.f) override;

	// whether gibs are allowed; some death effects can turn it off
	UPROPERTY(BlueprintReadWrite, Category = Effects)
	bool bAllowGibs;

	/** gibs the entire Pawn and destroys it (only the blood/gibs remain) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void GibExplosion();

	/** spawns a gib at the specified bone */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Death)
	virtual void SpawnGib(const struct FGibSlotInfo& GibSlot, TSubclassOf<class UUTDamageType> DmgType = NULL);

	/** plays death effects; use LastTakeHitInfo to do damage-specific death effects */
	virtual void PlayDying();
	virtual void TornOff() override
	{
		PlayDying();
	}

	UFUNCTION()
	virtual void DeathAnimEnd(UAnimMontage* Montage, bool bInterrupted);

	virtual bool IsRecentlyDead();

	virtual void DeactivateSpawnProtection();

	virtual void AddDefaultInventory(const TArray<TSubclassOf<AUTInventory>>& DefaultInventoryToAdd);

	UFUNCTION(BlueprintCallable, Category = Pawn)
	bool IsDead() const;

	/** weapon firing */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StartFire(uint8 FireModeNum);

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StopFire(uint8 FireModeNum);

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StopFiring();

	// redirect engine version
	virtual void PawnStartFire(uint8 FireModeNum = 0) override
	{
		StartFire(FireModeNum);
	}

	/** Return true if character is currently able to dodge. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	bool CanDodge() const;

	/** Return true if character is currently able to slide. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	bool CanSlide() const;

	/** Dodge requested by controller, return whether dodge occurred. */
	virtual bool Dodge(FVector DodgeDir, FVector DodgeCross);

	/** Dodge just occurred in dodge dir, play any sounds/effects desired.
	 * called on all clients
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnDodge(const FVector& DodgeLocation, const FVector &DodgeDir);

	/** Wall Dodge just occurred in dodge dir, play any sounds/effects desired.
	* called on all clients
	*/
	UFUNCTION(BlueprintNativeEvent)
	void OnWallDodge(const FVector& DodgeLocation, const FVector &DodgeDir);

	/** Slide just occurred, play any sounds/effects desired.
	* called on server and owning client
	*/
	UFUNCTION(BlueprintNativeEvent)
	void OnSlide(const FVector& SlideLocation, const FVector &SlideDir);

	/** Landing just occurred, play any effects desired. Landing sound will be played server-side elsewhere.
	* called on server and owning client
	*/
	UFUNCTION(BlueprintNativeEvent)
		void PlayLandedEffect();

	/** Landing assist just occurred */
	UFUNCTION(BlueprintImplementableEvent)
	void OnLandingAssist();

	/** Blueprint override for dodge handling. Return true to skip default dodge in C++. */
	UFUNCTION(BlueprintImplementableEvent)
	bool DodgeOverride(const FVector &DodgeDir, const FVector &DodgeCross);

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void CheckJumpInput(float DeltaTime) override;

	virtual void NotifyJumpApex();

	/** Handles moving forward/backward */
	virtual void MoveForward(float Val);

	/** Handles strafing movement, left and right */
	virtual void MoveRight(float Val);

	/** Handles up and down when swimming or flying */
	virtual void MoveUp(float Val);

	/** Also call UTCharacterMovement ClearJumpInput() */
	virtual void ClearJumpInput() override;

	virtual void MoveBlockedBy(const FHitResult& Impact) override;

	UFUNCTION(BlueprintPure, Category = PlayerController)
	virtual APlayerCameraManager* GetPlayerCameraManager();

	/** particle component for dodge */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* DodgeEffect;

	/** particle component for slide */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* SlideEffect;

	/** min Z speed to spawn LandEffect. */
	UPROPERTY(EditAnywhere, Category = "Effects")
		float LandEffectSpeed;

	/** particle component for high velocity jump landing */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* LandEffect;

	/** particle component for teleport */
	UPROPERTY(EditAnywhere, Category = "Effects")
	TArray< TSubclassOf<class AUTReplicatedEmitter> > TeleportEffect;

	/** particle component for rally */
	UPROPERTY(EditAnywhere, Category = "Effects")
		TArray< TSubclassOf<class AUTReplicatedEmitter> > RallyEffect;

	/** particle component for rally destination */
	UPROPERTY(EditAnywhere, Category = "Effects")
		TArray< TSubclassOf<class AUTReplicatedEmitter> > RallyDestinationEffect;

	UPROPERTY(EditAnywhere, Category = "Effects")
		TSubclassOf<AUTTaunt>  RallyAnimation;

	UPROPERTY(ReplicatedUsing=OnTriggerRallyEffect, BlueprintReadWrite)
		bool bTriggerRallyEffect;

	UPROPERTY(Replicated, BlueprintReadOnly)
		bool bRallyInProgress;

	UPROPERTY()
		float RallyCompleteTime;

	UFUNCTION()
	virtual void OnTriggerRallyEffect();

	virtual void SpawnRallyEffectAt(FVector EffectLocation);

	virtual void SpawnRallyDestinationEffectAt(FVector EffectLocation);

	/** particle component for normal ground footstep */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* GroundFootstepEffect;

	/** particle component for water footstep */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* WaterFootstepEffect;

	/** particle component for water entry */
	UPROPERTY(EditAnywhere, Category = "Effects")
		UParticleSystem* WaterEntryEffect;

	UFUNCTION(BlueprintCallable, Category = Effects)
		virtual void PlayWaterEntryEffect(const FVector& InWaterLoc, const FVector& OutofWaterLoc);

	/** plays a footstep effect; called via animation when anims are active (in vis range and not server), otherwise on interval via Tick() */
	UFUNCTION(BlueprintCallable, Category = Effects)
	virtual void PlayFootstep(uint8 FootNum, bool bFirstPerson = false);

	/** play jumping sound/effects; should be called on server and owning client */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Effects)
		void PlayJump(const FVector& JumpLocation, const FVector& JumpDir);

	/** Pawns must be overlapping at least this much for a telefrag to occur. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
		float MinOverlapToTelefrag;

	/** Landing at faster than this velocity results in damage (note: use positive number) */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float MaxSafeFallSpeed;

	/** amount of falling damage dealt if the player's fall speed is double MaxSafeFallSpeed (scaled linearly from there) */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float FallingDamageFactor;

	/** amount of damage dealt to other characters we land on per 100 units of speed */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float CrushingDamageFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Falling Damage")
		TSubclassOf<UUTDamageType> CrushingDamageType;

	/** Blueprint override for take falling damage.  Return true to keep TakeFallingDamage() from causing damage.
		FallingSpeed is the Z velocity at landing, and Hit describes the impacted surface. */
	UFUNCTION(BlueprintImplementableEvent)
	bool HandleFallingDamage(float FallingSpeed, const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = Pawn)
		virtual void TakeFallingDamage(const FHitResult& Hit, float FallingSpeed);

	virtual void Landed(const FHitResult& Hit) override;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		virtual void OnRepHeadArmorFlashCount();
	
	UPROPERTY(EditAnywhere, Category = Sounds)
	TArray<FPhysicalSoundResponse> FootstepSounds;

	UPROPERTY()
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSoundsMap;
	UPROPERTY()
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> OwnFootstepSoundsMap;

	/** Footstep sound when none is specified by the surface you are standing on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* WaterFootstepSound;

	USoundBase* GetFootstepSoundForSurfaceType(EPhysicalSurface SurfaceType, bool bLocalPlayer);
	
	// @TODO FIXMESTEVE temp
	float DefaultMeshTranslationZ;

	//================================
	// Swimming

	/** Minimum time between playing water entry/exit sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UnderWater)
	float MinWaterSoundInterval;

	/** Last time a water sound was played (for limiting frequency). */
	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	float LastWaterSoundTime;

	/** Maximum time underwater without breathing before taking damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UnderWater)
	float MaxUnderWaterTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UnderWater)
	float DrowningDamagePerSecond;

	UPROPERTY(BlueprintReadWrite, Category = UnderWater)
	bool bHeadIsUnderwater;

	/** Compare to MaxUnderWaterTime to see if drowning */
	UPROPERTY(BlueprintReadWrite, Category = UnderWater)
	float LastBreathTime;

	/** Take drowning damage once per seconds */
	UPROPERTY(BlueprintReadWrite, Category = UnderWater)
	float LastDrownTime;

	UPROPERTY(BlueprintReadWrite, Category = Game)
		float EnteredSafeVolumeTime;

	UPROPERTY(BlueprintReadWrite, Category = Game)
		bool bHasLeftSafeVolume;

	/** returns true if sound was played. (doesn't allow spamming) */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual bool PlayWaterSound(USoundBase* WaterSound);

	/** Called when character enters water volume. */
	virtual void EnteredWater(class AUTWaterVolume* WaterVolume);

	/** Returns true if core is in water. */
	virtual bool IsInWater() const;

	/** Returns true if BaseEyeHeight position is underwater */
	virtual bool HeadIsUnderWater() const;

	/** Returns true if bottom of capsule is in water */
	virtual bool FeetAreInWater() const;

	/** Return the water volume character is in, if currently in water.  Return null if not in water. */
	virtual APhysicsVolume* PositionIsInWater(const FVector& Position) const;

	/** Take drowning damage, play drowning sound */
	virtual void TakeDrowningDamage();

	//===============================

	/** Ambient sound played while wall sliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
		USoundBase* WallSlideAmbientSound;
	
	/** Ambient sound played while falling fast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* FallingAmbientSound;

	/** Falling speed to engage falling sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	float FallingAmbientStartSpeed;

	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	float LastPainSoundTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	float MinPainSoundInterval;

	/** Last time we handled  wall hit for gameplay (damage,sound, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	float LastWallHitNotifyTime;

	UPROPERTY(EditAnywhere, Category = Sounds)
		USoundBase* SpawnSound;

	/** sets character overlay effect; effect must be added to the UTGameState's OverlayEffects at level startup to work correctly (for replication reasons)
	 * multiple overlays can be active at once, but only one will be displayed at a time
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetCharacterOverlayEffect(const FOverlayEffect& NewOverlay, bool bEnabled);
	UFUNCTION(Meta = (DeprecatedFunction, DeprecationMessage = "Use SetCharacterOverlayEffect"), BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetCharacterOverlay(UMaterialInterface* NewOverlay, bool bEnabled);

	UFUNCTION()
		virtual void UpdateCharOverlayFlags();

	/** uses CharOverlayFlags to apply the desired overlay material (if any) to OverlayMesh */
	UFUNCTION()
	virtual void UpdateCharOverlays();

	/** returns the material instance currently applied to the character's overlay mesh, if any
	 * if not NULL, it is safe to change parameters on this material
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Effects)
	virtual UMaterialInstanceDynamic* GetCharOverlayMI();
	
	/** sets weapon overlay effect; effect must be added to the UTGameState's OverlayEffects at level startup to work correctly (for replication reasons)
	 * multiple overlays can be active at once, but the default in the weapon code is to only display one at a time
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetWeaponOverlayEffect(const FOverlayEffect& NewOverlay, bool bEnabled);
	UFUNCTION(Meta = (DeprecatedFunction, DeprecationMessage = "Use SetWeaponOverlayEffect"), BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetWeaponOverlay(UMaterialInterface* NewOverlay, bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetWeaponAttachmentClass(TSubclassOf<class AUTWeaponAttachment> NewWeaponAttachmentClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetHolsteredWeaponAttachmentClass(TSubclassOf<class AUTWeaponAttachment> NewWeaponAttachmentClass);

	/** uses WeaponOverlayFlags to apply the desired overlay material (if any) to OverlayMesh */
	UFUNCTION()
	virtual void UpdateWeaponOverlays();

	inline int16 GetCharOverlayFlags()
	{
		return CharOverlayFlags;
	}
	inline int16 GetWeaponOverlayFlags()
	{
		return WeaponOverlayFlags;
	}

	/** sets full body material override
	 * only one at a time is allowed
	 * pass NULL to restore default skin
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetSkin(UMaterialInterface* NewSkin, UMaterialInterface* NewSkin1P = NULL);
	inline UMaterialInterface* GetSkin()
	{
		return ReplicatedBodyMaterial;
	}

	/** apply skin in ReplicatedBodyMaterial or restore to default if it's NULL */
	UFUNCTION()
	virtual void UpdateSkin();

	/** timed full body color flash implemented via material parameter */
	UPROPERTY(BlueprintReadOnly, Category = Effects)
	const UCurveLinearColor* BodyColorFlashCurve;
	/** time elapsed in BodyColorFlashCurve */
	UPROPERTY(BlueprintReadOnly, Category = Effects)
	float BodyColorFlashElapsedTime;
	/** set timed body color flash (generally for hit effects)
	 * NOT REPLICATED
	 */
	UFUNCTION(BlueprintCallable, Category = Effects)
	virtual void SetBodyColorFlash(const UCurveLinearColor* ColorCurve, bool bRimOnly);

	/** updates time and sets BodyColorFlash in the character material */
	virtual void UpdateBodyColorFlash(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = Team)
	virtual uint8 GetTeamNum() const;
	// not applicable
	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

	UFUNCTION(BlueprintCallable, Category = Team)
	virtual FLinearColor GetTeamColor() const;

	virtual void OnRep_PlayerState() override;
	virtual void NotifyTeamChanged();

	virtual void PlayerChangedTeam();
	virtual void PlayerSuicide();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CharacterData)
	TSubclassOf<class AUTCharacterContent> CharacterData;

	UPROPERTY(BlueprintReadWrite, Category = FFA)
		uint8 FFAColor;

	/**Whether character data can override character blueprint defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CharacterDataOverride)
		bool bAllowCharacterDataOverride;

	// Temp replacement for CharacterData->GetDefaultObject() until 4.9 enables that in blueprints
	UFUNCTION(BlueprintCallable, Category = Character)
	AUTCharacterContent* GetCharacterData() const;

	virtual void ApplyCharacterData(TSubclassOf<class AUTCharacterContent> Data);

	/** called when a PC viewing this character switches from behindview to first person or vice versa */
	virtual void BehindViewChange(APlayerController* PC, bool bNowBehindView);

	virtual void BecomeViewTarget(APlayerController* PC) override;
	virtual void EndViewTarget(APlayerController* PC) override;

	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;

	/** For replicating movement events to generate client side sounds and effects. */
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = MovementEventReplicated, Category = "Movement")
		FMovementEventInfo MovementEvent;

	/** Direction associated with movement event.  Only accurate on server and player creating event, otherwise, uses Velocity normal. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
		FVector MovementEventDir;

	/** called when movement event needing client side sound/effects occurs */
	UFUNCTION()
	virtual void MovementEventUpdated(EMovementEvent MovementEventType, FVector Dir);

	/** repnotify handler for MovementEvent. */
	UFUNCTION()
		virtual void MovementEventReplicated();

	/** Effective target height when sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
		float SlideTargetHeight;

	//--------------------------
	// Weapon bob and eye offset

	/** Current 1st person weapon deflection due to running bob. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CurrentWeaponBob;

	/** Max 1st person weapon bob deflection with axes based on player view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponBobMagnitude;

	/** Z deflection of first person weapon when player jumps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponJumpBob;

	/** deflection of first person weapon when player dodges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponDodgeBob;

	/** deflection of first person weapon when player slides */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponSlideBob;

	/** Z deflection of first person weapon when player lands */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponLandBob;

	/** Desired 1st person weapon deflection due to jumping. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector DesiredJumpBob;

	/* Current jump bob (interpolating to DesiredJumpBob)*/
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CurrentJumpBob;

	/** Time used for weapon bob sinusoids, reset on landing. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	float BobTime;

	/** Rate of weapon bob when standing still. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponBreathingBobRate;

	/** Rate of weapon bob when running. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponRunningBobRate;

	/** How fast to interpolate to Jump/Land bob offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponJumpBobInterpRate;

	/** Max horizontal weapon movement interpolation rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
		float WeaponHorizontalBobInterpRate;

	/** How fast to decay out Land bob offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponLandBobDecayRate;

	/** Get Max eye offset land bob deflection at landing velocity Z of FullEyeOffsetLandBobVelZ+EyeOffsetLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponDirChangeDeflection;

	/** Current Eye position offset from base view position - interpolates toward TargetEyeOffset. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector EyeOffset;

	/** Eyeoffset due to crouching transition (not scaled). */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CrouchEyeOffset;

	/** Default crouched eye height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
		float DefaultCrouchedEyeHeight;

	/** Default crouched eye height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
		float FloorSlideEyeHeight;

	/** Transition between regular and floor slide crouched eyeheight. */
	virtual void UpdateCrouchedEyeHeight();

	/** Target Eye position offset from base view position. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector TargetEyeOffset;

	/** How fast EyeOffset interpolates to TargetEyeOffset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector EyeOffsetInterpRate;

	/** Small linear eye offset change added to interpolated to speed up end of interp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
		FVector EyeOffsetDirectRate;

	/** How fast CrouchEyeOffset interpolates to 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float CrouchEyeOffsetInterpRate;

	/** How fast TargetEyeOffset decays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector EyeOffsetDecayRate;

	/** Jump target view bob magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetJumpBob;

	/** Jump Landing target view bob magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetLandBob;

	/** Jump Landing target view bob Velocity threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetLandBobThreshold;

	/** Jump Landing target weapon bob Velocity threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponLandBobThreshold;

	/** Get Max weapon land bob deflection at landing velocity Z of FullWeaponLandBobVelZ+WeaponLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float FullWeaponLandBobVelZ;

	/** Get Max eye offset land bob deflection at landing velocity Z of FullEyeOffsetLandBobVelZ+EyeOffsetLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float FullEyeOffsetLandBobVelZ;

	/** Get Max weapon land bob deflection at landing velocity Z of FullWeaponLandBobVelZ+WeaponLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float DefaultBaseEyeHeight;

	/** maximum amount of time Pawn stays around when dead even if visible (may be cleaned up earlier if not visible) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Death)
	float MaxDeathLifeSpan;

	/** Broadcast when the pawn has died [Server only] */
	UPROPERTY(BlueprintAssignable)
	FCharacterDiedSignature OnDied;

	/** whether this pawn can obtain pickup items (UTPickup, UTDroppedPickup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanPickupItems;

	/** Max distance for enemy player indicator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  HUD)
	float PlayerIndicatorMaxDistance;

	/** Max distance for same team player indicator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	float TeamPlayerIndicatorMaxDistance;

	/** Max distance for same team player indicator */
	UPROPERTY(BlueprintReadWrite, Category = HUD)
	float SpectatorIndicatorMaxDistance;

	/** Scale of text for player indicator (interpolates based on distance). */
	UPROPERTY(BlueprintReadWrite, Category = HUD)
		float BeaconTextScale;

	/** Mark this pawn as should wear leader hat */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
		bool bShouldWearLeaderHat;

	UFUNCTION()
		void OnRep_ShouldWearLeaderHat();

	UFUNCTION(BlueprintNativeEvent)
		void LeaderHatStatusChanged();

	UFUNCTION(BlueprintNativeEvent)
	void HasHighScoreChanged();
	
	virtual void RecalculateBaseEyeHeight() override;

	/** Returns offset to add to first person mesh for weapon bob. */
	virtual FVector GetWeaponBobOffset(float DeltaTime, AUTWeapon* MyWeapon);
	
	/** Returns eyeoffset transformed into current view */
	virtual FVector GetTransformedEyeOffset() const;

	/** Returns amount of eyeoffset scaling for this character's view. */
	virtual float GetEyeOffsetScaling() const;

	virtual FVector GetPawnViewLocation() const override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual void UnCrouch(bool bClientSimulation = false) override;

	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false) override;
	UFUNCTION()
	virtual void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);
	
	virtual void CheckRagdollFallingDamage(const FHitResult& Hit);

	UFUNCTION()
	virtual void OnRagdollCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual bool CanPickupObject(AUTCarriedObject* PendingObject);
	/** @return the current object carried by this pawn */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual AUTCarriedObject* GetCarriedObject();

	virtual float GetLastRenderTime() const override;

	virtual void PostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir) override;

	virtual void PostRenderForInGameIntro(APlayerController* PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir);

	/** returns true if any local PlayerController is viewing this Pawn */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = PlayerController)
	bool IsLocallyViewed() const
	{
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			if (It->PlayerController != NULL && It->PlayerController->GetViewTarget() == this)
			{
				return true;
			}
		}
		return false;
	}

	/** spawn/destroy/replace the current weapon attachment to represent the equipped weapon (through WeaponClass) */
	UFUNCTION()
	virtual void UpdateWeaponAttachment();

	UFUNCTION()
	virtual void UpdateWeaponSkinPrefFromProfile(AUTWeapon* Weapon);

	UPROPERTY(replicatedUsing = OnRepWeaponSkin)
	TArray<UUTWeaponSkin*> WeaponSkins;

	UFUNCTION()
	virtual void OnRepWeaponSkin();

	virtual void SetSkinForWeapon(UUTWeaponSkin* WeaponSkin);

	/** spawn/destroy/replace the current holstered weapon attachment to represent the equipped weapon (through WeaponClass) */
	UFUNCTION()
	virtual void UpdateHolsteredWeaponAttachment();

	virtual FVector GetNavAgentLocation() const override
	{
		// push down a little to make sure we intersect with the navmesh but not so much that we get stuff on a lower level that requires a jump
		return GetActorLocation() - FVector(0.f, 0.f, FMath::Max<float>(25.0f, GetCharacterMovement()->MaxStepHeight));
	}

protected:
	/** reduces acceleration and friction while walking; generally applied temporarily by some weapons
	* (1.0 == no acceleration or friction, 0.0 == unrestricted movement)
	*/
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Movement)
	float WalkMovementReductionPct;
	/** time remaining until WalkMovementReductionPct is reset to zero */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Movement)
	float WalkMovementReductionTime;

public:
	inline float GetWalkMovementReductionPct()
	{
		return WalkMovementReductionPct;
	}
	/** sets walking movement reduction */
	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void SetWalkMovementReduction(float InPct, float InDuration);

	virtual void UpdateWeaponSkin();

protected:

	/** destroys dead character if no longer onscreen */
	UFUNCTION()
	void DeathCleanupTimer();

	UFUNCTION(BlueprintNativeEvent, Category = "Pawn|Character|InternalEvents", meta = (DisplayName = "CanDodge"))
	bool CanDodgeInternal() const;

	/** multiplier to firing speed */
	UPROPERTY(Replicated, ReplicatedUsing=FireRateChanged)
	float FireRateMultiplier;

	/** hook to modify damage taken by this Pawn
	 * NOTE: return value is a workaround for blueprint bugs involving ref parameters and is not used
	 */
	UFUNCTION(BlueprintNativeEvent)
		bool ModifyDamageTaken(UPARAM(ref) int32& AppliedDamage, UPARAM(ref) int32& Damage, UPARAM(ref) FVector& Momentum, UPARAM(ref) AUTInventory*& HitArmor, const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType);
	/** hook to modify damage CAUSED by this Pawn - note that EventInstigator may not be equal to Controller if we're in a vehicle, etc
	 * NOTE: return value is a workaround for blueprint bugs involving ref parameters and is not used
	 */
	UFUNCTION(BlueprintNativeEvent)
		bool ModifyDamageCaused(UPARAM(ref) int32& AppliedDamage, UPARAM(ref) int32& Damage, UPARAM(ref) FVector& Momentum, const FHitResult& HitInfo, AActor* Victim, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType);

	/** switches weapon locally, must execute independently on both server and client */
	virtual void LocalSwitchWeapon(AUTWeapon* NewWeapon);

	/** RPC to do weapon switch */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSwitchWeapon(AUTWeapon* NewWeapon);

	/** error checking for weapon switch to handle edge cases */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerVerifyWeapon(AUTWeapon* NewWeapon);
public:
	UFUNCTION(Client, Reliable)
	virtual void ClientVerifyWeapon();

protected:
	UFUNCTION(Client, Reliable)
	virtual void ClientSwitchWeapon(AUTWeapon* NewWeapon);
	/** utility to redirect to SwitchToBestWeapon() to the character's Controller (human or AI) */
	void SwitchToBestWeapon();

	// firemodes with input currently being held down (pending or actually firing)
	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	TArray<uint8> PendingFire;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pawn")
	AUTInventory* InventoryList;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	AUTWeapon* PendingWeapon;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pawn")
	class AUTWeapon* Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	class AUTWeaponAttachment* WeaponAttachment;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = UpdateWeaponAttachment, Category = "Pawn")
	TSubclassOf<AUTWeaponAttachment> WeaponAttachmentClass;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = UpdateWeaponAttachment, Category = "Pawn")
	TSubclassOf<AUTWeapon> WeaponClass;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	class AUTWeaponAttachment* HolsteredWeaponAttachment;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing=UpdateHolsteredWeaponAttachment, Category = "Pawn")
		TSubclassOf<AUTWeaponAttachment> HolsteredWeaponAttachmentClass;

public:
	inline AUTWeaponAttachment* GetWeaponAttachment() const
	{
		return WeaponAttachment;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn")
	TArray< TSubclassOf<AUTInventory> > DefaultCharacterInventory;

	/** If true, pickup will not execute its giveto().  Gives inventory a chance to override. */
	virtual bool OverrideGiveTo(class AUTPickup* Pickup);

protected:

	//================================
	// Ambient sounds

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=AmbientSoundUpdated, Category = "Audio")
	USoundBase* AmbientSound;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = AmbientSoundPitchUpdated, Category = "Audio")
		float AmbientSoundPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	UAudioComponent* AmbientSoundComp;

	/** Ambient sound played only on owning client */
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	USoundBase* LocalAmbientSound;

	/** Volume of Ambient sound played only on owning client */
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float LocalAmbientVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	UAudioComponent* LocalAmbientSoundComp;

	/** Status ambient sound played only on owning client */
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
		USoundBase* StatusAmbientSound;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	UAudioComponent* StatusAmbientSoundComp;

public:

	/** Ambient sound played while low in health*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* LowHealthAmbientSound;

	/** Health threshold for low health ambient sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	int32 LowHealthAmbientThreshold;

	/** sets replicated ambient (looping) sound on this Pawn
	* only one ambient sound can be set at a time
	* pass bClear with a valid NewAmbientSound to remove only if NewAmbientSound == CurrentAmbientSound
	*/
	UFUNCTION(BlueprintCallable, Category = Audio)
	virtual void SetAmbientSound(USoundBase* NewAmbientSound, bool bClear = false);

	UFUNCTION()
	void AmbientSoundUpdated();

	UFUNCTION()
	void AmbientSoundPitchUpdated();

	/** sets local (not replicated) ambient (looping) sound on this Pawn
	* only one ambient sound can be set at a time
	* pass bClear with a valid NewAmbientSound to remove only if NewAmbientSound == CurrentAmbientSound
	*/
	UFUNCTION(BlueprintCallable, Category = Audio)
	virtual void SetLocalAmbientSound(USoundBase* NewAmbientSound, float SoundVolume = 0.f, bool bClear = false);

	UFUNCTION()
	void LocalAmbientSoundUpdated();

	/** Adjust pitch of currently playing ambient sound if it is InAmbientSound. */
	UFUNCTION(BlueprintCallable, Category = Audio)
		void ChangeAmbientSoundPitch(USoundBase* InAmbientSound, float NewPitch);


	/** sets local (not replicated) status ambient (looping) sound on this Pawn
	* only one status ambient sound can be set at a time
	* pass bClear with a valid NewAmbientSound to remove only if NewAmbientSound == CurrentAmbientSound
	*/
	UFUNCTION(BlueprintCallable, Category = Audio)
	virtual void SetStatusAmbientSound(USoundBase* NewAmbientSound, float SoundVolume = 0.f, float PitchMultipier = 1.f, bool bClear = false);

	UFUNCTION()
	void StatusAmbientSoundUpdated();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetOutlineServer(bool bNowOutlined, bool bWhenUnoccluded = false, uint8 TeamMask = 0);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Effects)
	virtual void SetOutlineLocal(bool bNowOutlined, bool bWhenUnoccluded = false);

protected:
	/** whether the server has enabled the outline (because of gameplay effects like player pings) */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = UpdateOutline)
	bool bServerOutline;
	/** mask of teams that see the outline (0 and 255 == all teams) */
	UPROPERTY(VisibleInstanceOnly, Replicated)
	uint8 ServerOutlineTeamMask;
	/** whether to draw the outline when unoccluded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = UpdateOutline)
	bool bOutlineWhenUnoccluded;
	/** whether the local client has enabled the outline (because of e.g. vision modes) */
	UPROPERTY(VisibleInstanceOnly)
	bool bLocalOutline;

	UFUNCTION()
	virtual void UpdateOutline();

public:
	UPROPERTY(BlueprintReadWrite)
		bool bForceNoOutline;

	UFUNCTION(BlueprintCallable, Category = UTCharacter, BlueprintPure)
		bool IsOutlined(uint8 TestTeam = 255) const;
	inline bool GetOutlineWhenUnoccluded() const
	{
		return bOutlineWhenUnoccluded;
	}
	inline const USkeletalMeshComponent* GetCustomDepthMesh() const
	{
		return CustomDepthMesh;
	}


	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Effects)
		UMaterialInstance* GhostMaterial;

protected:
	/** copy of our mesh rendered to CustomDepth for the outline (which is done in postprocess using the resulting data) */
	UPROPERTY()
	USkeletalMeshComponent* CustomDepthMesh;

	/** last time PlayFootstep() was called, for timing footsteps when animations are disabled */
	float LastFootstepTime;

	/** last FootNum for PlayFootstep(), for alternating when animations are disabled */
	uint8 LastFoot;
	
	/** replicated overlays, bits match entries in UTGameState's OverlayMaterials array */
	UPROPERTY(Replicated, ReplicatedUsing = UpdateCharOverlayFlags)
	uint16 CharOverlayFlags;
	UPROPERTY(Replicated, ReplicatedUsing = UpdateWeaponOverlays)
	uint16 WeaponOverlayFlags;
public:
	/** mesh with current active overlay material on it (created dynamically when needed) */
	UPROPERTY(BlueprintReadOnly, Category = Effects)
	USkeletalMeshComponent* OverlayMesh;
protected:
	/** replicated character material override */
	UPROPERTY(Replicated, ReplicatedUsing = UpdateSkin)
	UMaterialInterface* ReplicatedBodyMaterial;
	UPROPERTY(Replicated, ReplicatedUsing = UpdateSkin)
	UMaterialInterface* ReplicatedBodyMaterial1P;
	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* OverrideBodyMaterialMID;
	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* OverrideBodyMaterial1PMID;

	/** runtime material instance for setting body material parameters (team color, etc) */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	TArray<UMaterialInstanceDynamic*> BodyMIs;
public:
	inline const TArray<UMaterialInstanceDynamic*>& GetBodyMIs() const
	{
		return BodyMIs;
	}
	/** legacy command for dropping the flag.  Just redirects to UseCarriedObject */
	UFUNCTION(Exec)
	virtual void DropFlag();

	/** uses the current carried object */
	UFUNCTION(exec)
		virtual void DropCarriedObject();

protected:

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropCarriedObject();

	/** uses the current carried object */
	UFUNCTION(exec)
	virtual void UseCarriedObject();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseCarriedObject();

private:
	void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);

public:
	/** set by path following to last MoveTarget that was successfully reached; used to avoid pathing getting confused about its current position on the navigation graph when it is on two nodes/polys simultaneously */
	UPROPERTY()
	FRouteCacheItem LastReachedMoveTarget;

	/** set by objects that set up ragdoll/death effects that involve a physics constraint on the ragdoll so we don't attach a second without destroying it, as multiple opposing constraints will break the physics */
	UPROPERTY(Transient, BlueprintReadWrite, Category = DeathEffects)
	class UPhysicsConstraintComponent* RagdollConstraint;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float FallingStartTime;

	virtual void Falling();

	/** Local player currently viewing this character. */
	UPROPERTY()
	class AUTPlayerController* CurrentViewerPC;

	virtual	class AUTPlayerController* GetLocalViewer();

	/** Previous actor location Z when last updated eye offset. */
	UPROPERTY()
	float OldZ;

	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ghost)
	class UUTGhostComponent* GhostComponent;


	// The character's max speed will be multipled by this value.  It allows for inventory and or game modes to override the max speed
	// for a character in a simple way.
	UPROPERTY(Replicated, Transient, BlueprintReadWrite, Category = Movement)
	float MaxSpeedPctModifier;

	// Used to reset the max speed modifier.  By default, this will return the value to 1.0f, but if the game is using weighted weapons
	// then calling this function will cause the pawn to pickup it's value from the weapon if possible.
	UFUNCTION(BlueprintCallable, Category=Weapon)
	virtual void ResetMaxSpeedPctModifier();

	// Sets the initial health for this character. 
	UFUNCTION(BlueprintNativeEvent, Category = Pawn)
	void SetInitialHealth();

public:
	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void BoostSpeedForTime(float SpeedBoostPct, float TimeToBoost);
	
	UFUNCTION(BlueprintCallable, Category = Aim)
	AActor* GetCurrentAimContext();

	FTimerHandle TriggerBoostPowerTimerHandle;

	/** Trigger boost. */
	virtual void TriggerBoostPower();
	virtual void TeamNotifyBoostPowerUse();


	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDropHealth();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDropArmor();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDropPowerup(AUTTimedPowerup* Powerup);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerDropBoots();

	virtual void SetArmorAmount(class AUTArmor* inArmorType, int32 Amount);


private:
	FTimerHandle SpeedBoostTimerHandle;

protected:
	TSubclassOf<AUTDroppedHealth> HealthDropType;
		
};

inline bool AUTCharacter::IsDead() const
{
	return bTearOff || IsPendingKillPending();
}


