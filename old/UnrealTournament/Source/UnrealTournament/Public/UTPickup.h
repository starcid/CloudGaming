// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTResetInterface.h"
#include "UTPathBuilderInterface.h"
#include "UTPlayerState.h"
#include "UTPickup.generated.h"

extern FName NAME_Progress;
extern FName NAME_RespawnTime;

USTRUCT()
struct FPickupReplicatedState
{
	GENERATED_USTRUCT_BODY()

	/** whether the pickup is currently active */
	UPROPERTY(BlueprintReadOnly, Category = Pickup)
	uint32 bActive : 1;
	/** plays taken effects when received on client as true
	 * only has meaning when !bActive
	 */
	UPROPERTY()
	uint32 bRepTakenEffects : 1;
	/** counter used to make sure same-frame toggles replicate correctly (activated while player is standing on it so immediate pickup) */
	UPROPERTY()
	uint8 ChangeCounter;
};

UCLASS(abstract, Blueprintable, meta = (ChildCanTick))
class UNREALTOURNAMENT_API AUTPickup : public AActor, public IUTResetInterface, public IUTPathBuilderInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	UCapsuleComponent* Collision;
	/** particles for the pickup timer, if enabled
	 * the parameter "Progress" should be used for the indicator
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	UParticleSystemComponent* TimerEffect;
	/** particles emitted from the base/bottom of the pickup */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	UParticleSystemComponent* BaseEffect;
	/** template for BaseEffect when pickup is available */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Effects)
	UParticleSystem* BaseTemplateAvailable;
	/** template for BaseEffect when pickup has been taken */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Effects)
	UParticleSystem* BaseTemplateTaken;

	/** if set, play this effect PreSpawnTime seconds before pickup spawns */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = SpawnEffects)
		UParticleSystem* PreSpawnEffect;

	/** How long before spawn to play PreSpawnEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = SpawnEffects)
		float PreSpawnTime;

	FTimerHandle PreSpawnTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = SpawnEffects)
		FTransform PreSpawnEffectTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = SpawnEffects)
		FVector PreSpawnColorVectorParam;

	virtual void PlayPreSpawnEffect();

	/** respawn time for the pickup; if it's <= 0 then the pickup doesn't respawn until the round resets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Pickup)
	float RespawnTime;
	/** if set, pickup begins play with its respawn time active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
	uint32 bDelayedSpawn : 1;
	/** if set, pickup respawns every RespawnTime seconds regardless of when it was picked up last */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Pickup)
	uint32 bFixedRespawnInterval : 1;

	/** if set, pickup only spawns once until reset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Pickup)
		uint32 bSpawnOncePerRound : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Pickup)
		uint32 bHasSpawnedThisRound : 1;

	/** one-shot particle effect played when the pickup is taken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	UParticleSystem* TakenParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	FTransform TakenEffectTransform;
	/** one-shot sound played when the pickup is taken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	USoundBase* TakenSound;
	/** one-shot particle effect played when the pickup respawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	UParticleSystem* RespawnParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	FTransform RespawnEffectTransform;
	/** one-shot sound played when the pickup respawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	USoundBase* RespawnSound;
	/** all components with any of these tags will be hidden when the pickup is taken
	 * if the array is empty, the entire pickup Actor is hidden
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	TArray<FName> TakenHideTags;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Pickup)
	FPickupReplicatedState State;

	UPROPERTY(BlueprintReadOnly, Category = Effects)
	UMaterialInstanceDynamic* TimerMI;

	// Holds the PRI of the last player to pick this item up.  Used to give a controlling bonus to score
	UPROPERTY(BlueprintReadOnly, Category = Game)
	AUTPlayerState* LastPickedUpBy;

	/** Spectator camera associated with this pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
		class AUTSpectatorCamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	FLinearColor IconColor;
	/** icon for drawing time remaining on the HUD. AUTPickupInventory use their InventoryClasses HUDIcon*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	FCanvasIcon HUDIcon;
	/** icon for minimap (if not specified, use HUDIcon) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	FCanvasIcon MinimapIcon;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = HUD)
	FCanvasIcon GetMinimapIcon() const;

	/** if set draw beacon on HUD when closer than this dist, optionally also through walls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	float BeaconDist;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	bool bBeaconThroughWalls;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	FCanvasIcon BeaconArrow;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	bool bHasTacComView;

	virtual void SetTacCom(bool bTacComEnabled);

	virtual float GetNextPickupTime();

	virtual void PreNetReceive();
	virtual void PostNetReceive();
	virtual void PostEditImport() override;
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	/** return whether Other can pick up this item (checks for stacking limits, etc)
	 * the default implementation checks for GameMode/Mutator overrides and returns bDefaultAllowPickup if no overrides are found
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Pickup)
	bool AllowPickupBy(APawn* Other, bool bDefaultAllowPickup);

	UFUNCTION(BlueprintNativeEvent)
	void ProcessTouch(APawn* TouchedBy);

	UFUNCTION(BlueprintNativeEvent)
		bool FlashOnMinimap();

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void GiveTo(APawn* Target);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Pickup)
	void StartSleeping();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Pickup)
	void WakeUp();

	FTimerHandle WakeUpTimerHandle;

	/** used for the timer-based call to WakeUp() so clients can perform different behavior to handle possible sync issues */
	UFUNCTION()
	void WakeUpTimer();
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void PlayTakenEffects(bool bReplicate);
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void PlayRespawnEffects();
	/** sets the hidden state of the pickup - note that this doesn't necessarily mean the whole object (e.g. item mesh but not holder) */
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void SetPickupHidden(bool bNowHidden);

	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual bool IsTaken(APawn* TestPawn)
	{
		return !State.bActive;
	}

	// add components that should be hidden for passed-in taken state
	virtual void AddHiddenComponents(bool bTaken, TSet<FPrimitiveComponentId>& HiddenComponents)
	{
		if (!bTaken)
		{
			if (TimerEffect != NULL)
			{
				HiddenComponents.Add(TimerEffect->ComponentId);
			}
		}
	}

	/** Pickup message to display on player HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	FText PickupMessageString;

	/** returns name of item for HUD displays, etc */
	virtual FText GetDisplayName() const
	{
		return PickupMessageString;
	}
	virtual FString GetHumanReadableName() const
	{
		return GetDisplayName().ToString();
	}

	virtual void Reset_Implementation() override;

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker);

	/** base desireability for AI acquisition/inventory searches (i.e. BotDesireability()) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	float BaseDesireability;

	/** returns how much the given Pawn (generally AI controlled) wants this item, where anything >= 1.0 is considered very important
	 * called only when either the pickup is active now, or if timing is enabled and the pickup will become active by the time Asker can cross the distance
	 * note that it isn't necessary for this function to modify the weighting based on the path distance as that is handled internally;
	 * the distance is supplied so that the code can make timing decisions and cost/benefit analysis
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AI)
	float BotDesireability(APawn* Asker, AController* RequestOwner, float PathDistance);

	/** similar to BotDesireability but this method is queried for items along the bot's path during most pathing queries, even when it isn't explicitly looking for items
	 * (e.g. checking to pick up health on the way to an enemy or game objective)
	 * in general this method should be more strict and return 0 in cases where the bot's objective should be higher priority than the item
	 * as with BotDesireability(), the PathDistance is weighted internally already and should primarily be used to reject things that are too far out of the bot's way
	 * note: this function is only called when Asker->Controller is the requestor, so it is OK to use that for querying bot skill/personality
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AI)
	float DetourWeight(APawn* Asker, float PathDistance);

	/** return whether the AI should consider this pickup 'super', which enables special timing, tracking, and pursuit logic
	 * the default is to consider any pickup with base or current desireability >= 1.0 to be super, but there are some cases
	 * where normal desireability and 'super' status don't intersect - for example, small health items will rate extremely highly
	 * when the AI is near death, but that doesn't mean AI should track and pursue one like it's a shield belt
	 * @param RequestOwner - Controller doing the inventory search; note that RequestOwner->Pawn is not necessarily the Pawn that is searching;
	 *						the Controller is provided in case it is relevant to check preferences
	 * @param CalculatedDesire - precalculated desire for the bot to get this item generally
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AI)
	bool IsSuperDesireable(AController* RequestOwner, float CalculatedDesire);

	/** if pickup is available, returns a negative value indicating the time since the pickup last respawned
	 * if not available, returns a positive value indicating the time until the pickup respawns (FLT_MAX if it will never respawn)
	 * this is used for bot pickup timing
	 * only valid on server
	 */
	UFUNCTION(BlueprintCallable, Category = AI)
	virtual float GetRespawnTimeOffset(APawn* Asker) const;

	/**Enables overriding the auto teamside for this pickup*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup, meta = (PinHiddenByDefault))
	bool bOverride_TeamSide;

	/** For spectator slide out - show which side this pickup is on when there are multiple. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup, meta = (editcondition = bOverride_TeamSide))
	uint8 TeamSide;

	virtual void PrecacheTutorialAnnouncements(class UUTAnnouncer* Announcer) const;

	virtual FName GetTutorialAnnouncement(int32 Switch) const;

	UPROPERTY(EditAnyWhere, Category = "Tutorial")
		TArray<FName> TutorialAnnouncements;

protected:
	/** last time pickup respawned, used by GetRespawnTimeOffset() */
	float LastRespawnTime;

	/** used to replicate remaining respawn time to newly joining clients */
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_RespawnTimeRemaining)
	float RespawnTimeRemaining;
	/** used to replicate effects of a call to Reset() (flag is flipped to trigger the repnotify, value is meaningless) */
	UPROPERTY(ReplicatedUsing = OnRep_Reset)
	bool bReplicateReset;

	UFUNCTION()
	virtual void OnRep_RespawnTimeRemaining();
	UFUNCTION()
	virtual void OnRep_Reset();
};
