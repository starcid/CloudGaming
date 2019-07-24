// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTRallyPoint.generated.h"

UCLASS(Blueprintable)
class UNREALTOURNAMENT_API AUTRallyPoint : public AUTGameObjective, public IUTResetInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = RallyPoint)
		UCapsuleComponent* Capsule;

	UPROPERTY(BlueprintReadOnly)
		class AUTGameVolume* MyGameVolume;

	/** How far away from FC to display this RallyPoint as available (both beacon for FC and team colored effect for others) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = RallyPoint)
		float RallyAvailableDistance;

	/** how long FC has to be here for rally to start */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = RallyPoint)
		float RallyReadyDelay;

	/** When rallypoint was powered up */
	UPROPERTY(BlueprintReadOnly, Category = Objective)
		float RallyStartTime;

	FTimerHandle EndRallyHandle;

	FTimerHandle WarnNoFlagHandle;

	virtual void WarnNoFlag(AUTCharacter* Toucher);

	virtual void UpdateRallyReadyCountdown(float NewValue);

	/** Min remaining time if FC steps off */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = RallyPoint)
		float MinPersistentRemaining;

	/** Minimum powered up time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = RallyPoint)
		float MinimumRallyTime;

	UPROPERTY(BlueprintReadOnly, Category = Objective)
		float RallyReadyCountdown;

	/** Last time enemy combat associated with this Rally Point. */
	UPROPERTY(BlueprintReadWrite, Category = Objective)
		float LastRallyHot;

	/** Replicate 10 * RallyReadyCountdown */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnReplicatedCountdown, Category = RallyPoint)
		int32 ReplicatedCountdown;

	UPROPERTY(BlueprintReadOnly, Category = RallyPoint)
		float ClientCountdown;

	UPROPERTY()
		float OldClientCountdown;

	UPROPERTY(BlueprintReadOnly, Category = RallyPoint)
		float RallyTimeRemaining;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRallyTimeRemaining, Category = RallyPoint)
		float ReplicatedRallyTimeRemaining;

	UPROPERTY(ReplicatedUsing = OnAvailableEffectChanged, BlueprintReadOnly)
		bool bShowAvailableEffect;

	/** Flag carrier currently touching this point. */
	UPROPERTY(BlueprintReadOnly, Category = Objective)
		class AUTCharacter* TouchingFC;

	UPROPERTY()
		bool bHaveGameState;

	UPROPERTY(Replicated, BlueprintReadOnly)
		bool bIsEnabled;

	UPROPERTY(ReplicatedUsing = OnRallyChargingChanged, BlueprintReadOnly, Category = RallyPoint)
		FName RallyPointState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* AvailableEffect;

	UPROPERTY()
		UParticleSystemComponent* AvailableEffectPSC;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyChargingEffectRed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyPoweredEffectRed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyFinishedEffectRed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* LosingChargeEffectRed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyChargingEffectBlue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyPoweredEffectBlue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* RallyFinishedEffectBlue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RallyPoint)
		UParticleSystem* LosingChargeEffectBlue;

	UPROPERTY()
		UParticleSystemComponent* RallyChargingEffectPSC;

	UPROPERTY()
		UParticleSystemComponent* RallyPoweredEffectPSC;

	UPROPERTY()
		UParticleSystemComponent* LosingChargeEffectPSC;

	UPROPERTY()
		UDecalComponent* AvailableDecal;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = AmbientSoundUpdated, Category = "Audio")
		USoundBase* AmbientSound;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
		float AmbientSoundPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
		UAudioComponent* AmbientSoundComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* PoweringUpSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* FullyPoweredSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* ReadyToRallySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* FCTouchedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* EnabledSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* RallyEndedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
		USoundBase* RallyBrokenSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
		UMaterialInterface* GlowDecalMaterial;

	UPROPERTY(BlueprintReadOnly, Category = PickupDisplay)
		UMaterialInstanceDynamic* GlowDecalMaterialInstance;

	UPROPERTY()
		float LastEnemyRallyWarning;

	/** Status message to use as warning when this rally point is being powered up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RallyPoint)
		FName EnemyRallyWarning;

	/** What to display on the Rally Beacon for flag carriers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RallyPoint)
		FText RallyBeaconText;

	/** Location name for this RallyPoint.  If not set, use the game volume's VolumeName. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RallyPoint)
		FText LocationText;

	UFUNCTION()
		void OnReplicatedCountdown();

	FTimerHandle EnemyRallyWarningHandle;

	virtual void WarnEnemyRally();

	UFUNCTION()
		void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void BeginPlay() override;
	virtual void GenerateDefensePoints() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir) override;

	/** Draw rally charging thermometer. */
	virtual void DrawChargingThermometer(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, bool bFixedPosition);

	UPROPERTY()
		bool bBeaconWasLeft;

	virtual FVector GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow);

	virtual void Reset_Implementation() override;

	virtual void FlagNearbyChanged(bool bIsNearby);

	virtual void StartRallyCharging();

	virtual void EndRallyCharging();

	virtual void RallyPoweredComplete();

	virtual void RallyPoweredTurnOff();

	virtual void SetRallyPointState(FName NewState);

	virtual FVector GetRallyLocation(class AUTCharacter* TestChar);

	// increment to give different rally spots to each arriving player
	UPROPERTY()
		int32 RallyOffset;

	UFUNCTION()
		void OnRallyTimeRemaining();

	UFUNCTION()
	void OnAvailableEffectChanged();

	UFUNCTION()
		void OnRallyChargingChanged();

	UFUNCTION(BlueprintImplementableEvent, Category=RallyPoint)
		void OnRallyStateChanged();

	virtual void ChangeAmbientSoundPitch(USoundBase* InAmbientSound, float NewPitch);

	UFUNCTION()
		virtual void AmbientSoundPitchUpdated();

	virtual void SetAmbientSound(USoundBase* NewAmbientSound, bool bClear);

	UFUNCTION()
		void AmbientSoundUpdated();

	/** Arrow component to indicate forward direction of start */
#if WITH_EDITORONLY_DATA
	private_subobject :
						  UPROPERTY()
						  class UArrowComponent* ArrowComponent;
public:
	/** Returns ArrowComponent subobject **/
	class UArrowComponent* GetArrowComponent() const;
#endif
};
