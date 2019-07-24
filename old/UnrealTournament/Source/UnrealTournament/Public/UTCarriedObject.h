// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTPlayerState.h"
#include "UTCarriedObjectMessage.h"
#include "UTTeamInterface.h"
#include "UTProjectileMovementComponent.h"
#include "UTCarriedObject.generated.h"

class AUTCarriedObject;
class AUTHUD;
class AUTGhostFlag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCarriedObjectStateChangedDelegate, class AUTCarriedObject*, Sender, FName, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedObjectHolderChangedDelegate, class AUTCarriedObject*, Sender);
const int32 NUM_MIDPOINTS = 3;

USTRUCT()
struct FAssistTracker
{
	GENERATED_USTRUCT_BODY()

public:

	// The PlayerState of the player who held it
	UPROPERTY()
	AUTPlayerState* Holder;

	// Total amount of time it's been held.
	UPROPERTY()
	float TotalHeldTime;
};

USTRUCT()
struct FFlagTrailPos
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FVector Location;

	UPROPERTY()
		FVector MidPoints[3];

	UPROPERTY()
		bool bIsInNoRallyZone; // FIXMESTEVE REMOVE ONCE DETERMINE DONT WANT

	UPROPERTY()
		bool bEnteringNoRallyZone;

	FFlagTrailPos()
		: Location(ForceInit)
	{
		for (int32 i = 0; i < 3; i++)
		{
			MidPoints[i] = FVector(0.f);
		}
		bIsInNoRallyZone = false;
		bEnteringNoRallyZone = false;
	}

	FFlagTrailPos(FVector inLocation)
		: Location(inLocation)
	{
		for (int32 i = 0; i < 3; i++)
		{
			MidPoints[i] = FVector(0.f);
		}
		bIsInNoRallyZone = false;
		bEnteringNoRallyZone = false;
	}

};

UCLASS()
class UNREALTOURNAMENT_API AUTCarriedObject : public AActor, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	// The Current state of this object.  See ObjectiveState in UTATypes.h
	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnObjectStateChanged, Category = GameObject)
	FName ObjectState;

	UFUNCTION(BlueprintCallable, Category = GameObject)
	virtual bool IsHome();

	// Holds the UTPlayerState of the person currently holding this object.  
	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnHolderChanged, Category = GameObject)
	AUTPlayerState* Holder;

	// Holds the UTPlayerState of the last person to hold this object.  
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	AUTPlayerState* LastHolder;

	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	float PickedUpTime;

	// Holds a array of information about people who have held this object
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	TArray<FAssistTracker> AssistTracking;

	/** list of players who have 'rescued' the flag carrier by killing an enemy that is targeting the carrier
	 * reset on return/score
	 */
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	TArray<AController*> HolderRescuers;

	// Server Side - Holds a reference to the pawn that is holding this object
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=UpdateOutline, Category = GameObject)
	AUTCharacter* HoldingPawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
		UParticleSystem* FirstPersonRedFlagEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
		UParticleSystem* FirstPersonBlueFlagEffect;

	// Holds the home base for this object.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameObject)
	class AUTGameObjective* HomeBase;

	// Holds the team that this object belongs to
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameObject, replicatedUsing = OnRep_Team)
	AUTTeamInfo* Team;

	// Last time this object was pinged (for enemy status icon relevance).
	UPROPERTY(BlueprintReadWrite, Category = GameObject)
	float LastPingedTime;

	// last player to ping this object
	UPROPERTY(BlueprintReadWrite, Category = GameObject)
		AUTPlayerState* LastPinger;

	// How long a non-enemy ping is valid
	UPROPERTY(EditDefaultsOnly, Category = GameObject)
		float PingedDuration;

	// How long an enemy targeting ping is valid
	UPROPERTY(EditDefaultsOnly, Category = GameObject)
		float TargetPingedDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
		bool bShouldPingFlag;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameObject)
	bool bCurrentlyPinged;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = GameObject)
		bool bSlowsMovement;

	UPROPERTY(BlueprintReadWrite, Category = GameObject)
		bool bWaitingForFirstPickup;

	UPROPERTY(EditDefaultsOnly, Category = GameObject)
		TSubclassOf<class AUTGhostFlag> GhostFlagClass;

	UPROPERTY(BlueprintReadOnly, Category = GameObject)
		bool bWasInEnemyBase;
	
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
		float EnteredEnemyBaseTime;

private:
	UPROPERTY()
	TArray<AUTGhostFlag*> MyGhostFlags;

public:
	UPROPERTY(BlueprintReadWrite, Category = Flag)
	bool bSingleGhostFlag;

	virtual AUTGhostFlag* PutGhostFlagAt(FFlagTrailPos NewPosition, bool bShowTimer = true, bool bSuppressTrails = false, uint8 TeamNum = 255);

	virtual void ClearGhostFlags();
	virtual void PlayReturnedEffects() {};

	// Allow children to know when the team changes
	UFUNCTION()
		virtual void OnRep_Team();

	// Where to display this object relative to the home base
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
	FVector HomeBaseOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
	FRotator HomeBaseRotOffset;

	// What bone or socket on holder should this object be attached to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	FName Holder3PSocketName;

	// Transform to apply when attaching to a Pawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	FVector Holder3PTransform;

	// Rotation to apply when attaching to a Pawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	FRotator Holder3PRotation;

	// if true, then anyone can pick this object up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	uint32 bAnyoneCanPickup:1;

	// if true, then enemy of this object's team can pick this object up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
		uint32 bEnemyCanPickup : 1;

	// if true, then player on this object's team can pick this object up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GameObject)
		uint32 bFriendlyCanPickup : 1;

	// If true, when a player on the team matching this object's team picks it up, it will be sent home instead of being picked up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	uint32 bTeamPickupSendsHome:1;

	// If true, when a player on the enemy team picks it up, it will be sent home instead of being picked up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
		uint32 bEnemyPickupSendsHome : 1;

	// If true, scoring plays send the flag home.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
		uint32 bSendHomeOnScore : 1;

	// How long before this object is automatically returned to it's base
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = GameObject)
	float AutoReturnTime;

	/** Replicated time till flag auto returns. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Flag)
		uint8 FlagReturnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
	TSubclassOf<UUTCarriedObjectMessage> MessageClass;

	UPROPERTY(BlueprintAssignable)
	FOnCarriedObjectStateChangedDelegate OnCarriedObjectStateChangedDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnCarriedObjectHolderChangedDelegate OnCarriedObjectHolderChangedDelegate;

	/** Last time a game announcement message was sent */
	UPROPERTY()
	float LastGameMessageTime;

	/** sound played for holder when the object is picked up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
		USoundBase* HolderPickupSound;

	/** sound played for other playerswhen the object is picked up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* PickupSound;

	/** sound played when the object is dropped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* DropSound;

	/** Ambient sound played while holding flag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
		USoundBase* HeldFlagAmbientSound;

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	UPROPERTY()
		FVector LastTeleportedLoc;

	UPROPERTY()
		float LastTeleportedTime;

	UPROPERTY()
		FText LastLocationName;

	virtual void Init(AUTGameObjective* NewBase);

	// Returns the team number of the team that owns this object
	UFUNCTION()
	virtual uint8 GetTeamNum() const;
	// not applicable
	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

	/**	Changes the current state of the carried object.  NOTE: this should only be called on the server*/
	UFUNCTION()
	virtual void ChangeState(FName NewCarriedObjectState);

	/**
	 *	Called when a player picks up this object or called
	 *  @NewHolder	The UTCharacter that picked it up
	 **/
	UFUNCTION()
	virtual void SetHolder(AUTCharacter* NewHolder);

	/**
	 *	Drops the object in to the world and allows it to become a pickup.
	 *  @Killer The controller that cause this object to be dropped
	 **/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameObject)
	virtual void Drop(AController* Killer = NULL);

	/**	Sends this object back to its base */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameObject)
	virtual void SendHome();
	virtual void SendHomeWithNotify();

	/**	Uses this carried object*/
	UFUNCTION()
	virtual void Use();

	/**  Call this to tell the object to score.*/
	UFUNCTION(BlueprintNativeEvent)
	void Score(FName Reason, AUTCharacter* ScoringPawn, AUTPlayerState* ScoringPS);

	UFUNCTION(BlueprintNativeEvent)
	void TryPickup(AUTCharacter* Character);

	virtual void SetTeam(AUTTeamInfo* NewTeam);

	virtual void EnteredPainVolume(class AUTPainVolume* PainVolume);

	UFUNCTION()
	virtual void AttachTo(USkeletalMeshComponent* AttachToMesh);

	UFUNCTION()
	virtual void DetachFrom(USkeletalMeshComponent* AttachToMesh);

	/** Called client-side when attachment state changes. */
	virtual void ClientUpdateAttachment(bool bNowAttached);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = GameObject)
	UCapsuleComponent* Collision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = GameObject)
	class UUTProjectileMovementComponent* MovementComponent;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

	void FellOutOfWorld(const UDamageType& dmgType);

	// workaround for bug in AActor implementation
	virtual void OnRep_AttachmentReplication() override;
	// HACK: workaround for engine bug with transform replication when attaching/detaching things
	virtual void OnRep_ReplicatedMovement() override;
	virtual void GatherCurrentMovement() override;

	virtual float GetHeldTime(AUTPlayerState* TestHolder);

	/**	@Returns the index of a player in the assist array */
	virtual int32 FindAssist(AUTPlayerState* InHolder)
	{
		for (int32 i=0; i<AssistTracking.Num(); i++)
		{
			if (AssistTracking[i].Holder == InHolder) return i;
		}

		return -1;
	}

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override
	{
		bMovementEnabled = MovementComponent != NULL && MovementComponent->UpdatedComponent != NULL;
		Super::PreReplication(ChangedPropertyTracker);
	}

	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override
	{
		MovementComponent->Velocity = NewVelocity;
	}

	/** Attach flag to lift if it landed on one. */
	UFUNCTION()
	virtual void OnStop(const FHitResult& Hit);

	UFUNCTION()
	virtual void CheckTouching();

	UPROPERTY()
		TArray<FFlagTrailPos> PastPositions;

	UPROPERTY()
		FVector RecentPosition[2];

	UPROPERTY()
		FVector MidPoints[NUM_MIDPOINTS];

	UPROPERTY()
		int32 MidPointPos;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameObject)
		bool bGradualAutoReturn;

	/** If bGradualAutoReturn, don't move if player that can pick flag up is nearby. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = GameObject)
		bool bWaitForNearbyPlayer;

	/** Minimum distance between adjacent gradual return points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameObject)
		float MinGradualReturnDist;

	/** If true, attach holder trail to character carrying this object. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameObject)
		bool bDisplayHolderTrail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameObject)
		UParticleSystem* HolderTrailEffect;

	UPROPERTY()
		UParticleSystemComponent* HolderTrail;

	UFUNCTION()
		virtual void UpdateOutline();

	FTimerHandle NeedFlagAnnouncementTimer;

	virtual void SendNeedFlagAnnouncement();

	virtual void RemoveInvalidPastPositions();

protected:
	// Server Side - Holds a reference to the pawn that is holding this object
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	AUTCharacter* LastHoldingPawn;

	/** whether the movement component is enabled, to make sure clients stay in sync */
	UPROPERTY(ReplicatedUsing = OnRep_Moving)
	bool bMovementEnabled;

	UFUNCTION()
	virtual void OnRep_Moving();

	// The timestamp of when this object was last taken.
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
	float TakenTime;

	// keep from playing flag dropped messages too often
	UPROPERTY(BlueprintReadOnly, Category = GameObject)
		float LastDroppedMessageTime;

	UPROPERTY(BlueprintReadOnly, Category = GameObject)
		float LastNeedFlagMessageTime;

	UFUNCTION()
	virtual void OnObjectStateChanged();

	UFUNCTION()
	virtual void OnHolderChanged();

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 *	By default, only people on the same team as the object can pick it up.  You can quickly override this by setting bTeamPickupSendsHome to true
	 *  or by override this function.  
	 **/
	virtual bool CanBePickedUpBy(AUTCharacter* Character);

	/**	This function will be called when a pick up is denied.*/
	virtual void PickupDenied(AUTCharacter* Character);

	/**	Called from both Drop and SendHome - cleans up the current holder.*/
	UFUNCTION()
	virtual void NoLongerHeld(AController* InstigatedBy = NULL);

	/**	Move the flag to it's home base*/
	UFUNCTION()
	virtual void MoveToHome();

	virtual void SendGameMessage(uint32 Switch, APlayerState* PS1, APlayerState* PS2, UObject* OptionalObject = NULL);

	virtual void TossObject(AUTCharacter* ObjectHolder);

	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false) override;

	virtual void UpdateHolderTrailTeam();

	/** used to prevent overlaps from triggering from within the drop code where it could cause inconvenient side effects */
	bool bIsDropping;

	FTimerHandle CheckTouchingHandle;

	// Will be true if this object has been initialized and is safe for gameplay
	bool bInitialized;


public:
	// The Speed modifier for this weapon if we are using weighted weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float WeightSpeedPctModifier;

	// Returns a status message for this object on the hud.
	virtual FText GetHUDStatusMessage(AUTHUD* HUD);

	/** return location for object when returning home */
	virtual FVector GetHomeLocation() const;
	virtual FRotator GetHomeRotation() const;

	float GetGhostFlagTimerTime(AUTGhostFlag* Ghost);

	UFUNCTION()
		void PlayAlarm();

	FTimerHandle AlarmHandle;
};