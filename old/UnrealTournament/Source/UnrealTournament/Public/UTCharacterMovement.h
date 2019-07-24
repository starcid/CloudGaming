// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTCharacterMovement.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

public:

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual bool ClientUpdatePositionAfterServerUpdate() override;
	virtual void ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration) override;
	virtual void ClientAckGoodMove_Implementation(float TimeStamp) override;

	/** Return true if it is OK to delay sending this player movement to the server to conserve bandwidth. */
	virtual bool CanDelaySendingMove(const FSavedMovePtr& NewMove) override;

	virtual void UTServerMoveHandleClientError(float TimeStamp, float DeltaTime, const FVector& Accel, const FVector& RelativeClientLoc, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode);

	virtual void UTCallServerMove();

	virtual void UTSimulateMovement(float DeltaTime) { SimulateMovement(DeltaTime); }

	UPROPERTY(Category = "Character Movement", BlueprintReadOnly)
		float MaxPositionErrorSquared;

	/** @return true if position error exceeds max allowable amount */
	virtual bool ExceedsAllowablePositionError(FVector LocDiff) const;

	/** Process servermove forwarded by character */
	virtual void ProcessServerMove(float TimeStamp, FVector Accel, FVector ClientLoc, uint8 CompressedMoveFlags, float ViewYaw, float ViewPitch, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode);

	/** Process servermove forwarded by character, without correction */
	virtual void ProcessQuickServerMove(float TimeStamp, FVector Accel, uint8 CompressedMoveFlags);

	/** Process servermove forwarded by character, without correction, but needs rotation */
	virtual void ProcessSavedServerMove(float TimeStamp, FVector Accel, uint8 CompressedMoveFlags, float ViewYaw, float ViewPitch);

	/** Process old servermove forwarded by character */
	virtual void ProcessOldServerMove(float OldTimeStamp, FVector OldAccel, float OldYaw, uint8 OldMoveFlags);

	/** Sets LastClientAdjustmentTime so there will be no delay in sending any needed adjustment. */
	virtual void NeedsClientAdjustment();

	/** Clear falling mode flags because left falling without calling processlanded, */
	virtual void ClearFallingStateFlags();

	virtual void ResetPredictionData_Client() override;

	/* Bandwidth saving version, when position is not relative to base */
	UFUNCTION(unreliable, client)
	virtual void ClientNoBaseAdjustPosition(float TimeStamp, FVector NewLoc, FVector NewVelocity, uint8 ServerMovementMode);

	/** Last time a client adjustment was sent.  Used to limit frequency (for when client hasn't had a chance to respond yet. */
	UPROPERTY()
	float LastClientAdjustmentTime;

	/** Last time a good move ack was sent.  Used to limit frequency (for when client hasn't had a chance to respond yet. */
	UPROPERTY()
		float LastGoodMoveAckTime;

	/** Set if the pending client adjustment is for a large position correction. */
	UPROPERTY()
		bool bLargeCorrection;

	/** Debugging jump visualization. */
	UPROPERTY()
		bool bDrawJumps;

	/** Position error considered a large correction for client adjustment. */
	UPROPERTY()
		float LargeCorrectionThreshold;

	/** Used to control minimum frequency of client adjustment and good move ack replication. */
	UPROPERTY()
	float MinTimeBetweenClientAdjustments;

	/** Set client-side acceleration based on replicated acceldir. */
	virtual void SetReplicatedAcceleration(FRotator MovementRotation, uint8 AccelDir);

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	virtual FVector GetImpartedMovementBaseVelocity() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual void PerformMovement(float DeltaSeconds) override;
	virtual float ComputeAnalogInputModifier() const override;

	/** Update traversal stats. */
	virtual void UpdateMovementStats(const FVector& StartLocation);

	/** Try to base on lift that just ran into me, return true if success */
	virtual bool CanBaseOnLift(UPrimitiveComponent* LiftPrim, const FVector& LiftMoveDelta);

	virtual void UpdateBasedMovement(float DeltaSeconds) override;

	virtual bool CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump) override;

	virtual FVector GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir) const override;

	/** If I'm on a lift, tell it to return */
	virtual void OnUnableToFollowBaseMove(const FVector& DeltaPosition, const FVector& OldLocation, const FHitResult& MoveOnBaseHit) override;
	
	/** Reset timers (called on pawn possessed) */
	virtual void ResetTimers();

	/** Return true if movement input should not be constrained to horizontal plane */
	virtual bool Is3DMovementMode() const;

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	/** Allows custom handling of timestamp and delta time updates, including resetting movement timers. */
	virtual float UpdateTimeStampAndDeltaTime(float DeltaTime, FNetworkPredictionData_Client_Character* ClientData);

	/** Adjust movement timers after timestamp reset */
	virtual void AdjustMovementTimers(float Adjustment);

	/** Allows custom handling of timestamp and delta time updates, including resetting movement timers. */
	virtual bool UTVerifyClientTimeStamp(float TimeStamp, FNetworkPredictionData_Server_Character & ServerData);

	/** for replaying moves set up */
	bool bIsSettingUpFirstReplayMove;

	/** Used to disable jump boots if they are setup to be disabled on flag carrier */
	bool bIsDoubleJumpAvailableForFlagCarrier;

	/** Smoothed speed */
	UPROPERTY()
	float AvgSpeed;

	/** Max Acceleration when falling (will be scaled by AirControl property). */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxFallingAcceleration;

	/** Max constant Acceleration when swimming. */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxSwimmingAcceleration;

	/** Fast initial acceleration when walking. */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float FastInitialAcceleration;

	/** Max speed to get fast initial acceleration when walking. */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float MaxFastAccelSpeed;

	/** Additional Acceleration when swimming, divided by Velocity magnitude.  Swimming acceleration is MaxSwimmingAcceleration + MaxRelativeSwimmingAccelNumerator/(Speed + MaxRelativeSwimmingAccelDenominator). */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxRelativeSwimmingAccelNumerator;

	/** Part of swimming acceleration formula.  Swimming acceleration is MaxSwimmingAcceleration + MaxRelativeSwimmingAccelNumerator/(Speed + MaxRelativeSwimmingAccelDenominator). */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxRelativeSwimmingAccelDenominator;

	/** Braking when sliding. */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float BrakingDecelerationSliding;

	/** Braking when walking - set to same value as BrakingDecelerationWalking. */
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float DefaultBrakingDecelerationWalking;

	/** Max speed player can travel in water (faster than powered swim speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Swimming)
	float MaxWaterSpeed;

	/** if set force team collision for this character even if game mode turns it off in general */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision)
	bool bForceTeamCollision;

	/** Impulse when pushing off wall underwater */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Swimming)
	float SwimmingWallPushImpulse;

	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;
	virtual void HandleSwimmingWallHit(const FHitResult& Hit, float DeltaTime) override;

	/** True if was swimming in current, and haven't landed yet. */
	UPROPERTY(BlueprintReadOnly, Category = Swimming)
		bool bFallingInWater;

	/** True if character is currently running along a wall. */
	UPROPERTY(Category = "Wall Slide", BlueprintReadOnly)
		bool bSlidingAlongWall;

	/** Apply water current to swimming or falling player in contact with water. */
	virtual void ApplyWaterCurrent(float DeltaTime);

	virtual bool ShouldJumpOutOfWater(FVector& JumpDir) override;

	/** Push off bottom while swimming. */
	virtual void PerformWaterJump();

	inline FVector GetPendingImpulse() const
	{
		return PendingImpulseToApply;
	}

	//Checks if we are a flag carrier at the moment
	virtual bool IsCarryingFlag() const;

protected:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact) override;

public:
	/** Impulse imparted by "easy" impact jump. Not charge or jump dependent (although get a small bonus with timed jump). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
	float EasyImpactImpulse;

	/** Impulse imparted by "easy" impact jump. Not charge or jump dependent (although get a small bonus with timed jump). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
	float EasyImpactDamage;

	/** Impulse imparted by "easy" impact jump. Not charge or jump dependent (although get a small bonus with timed jump). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
		float FullImpactImpulse;

	/** Impulse imparted by "easy" impact jump. Not charge or jump dependent (although get a small bonus with timed jump). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
		float FullImpactDamage;

	/** Max total horizontal velocity after impact jump. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
		float ImpactMaxHorizontalVelocity;

	/** Scales impact impulse to determine max total vertical velocity after impact jump. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
		float ImpactMaxVerticalFactor;

	/** Add an impulse, damped if player would end up going too fast */
	UFUNCTION(BlueprintCallable, Category = "Impulse")
		virtual void ApplyImpactVelocity(FVector JumpDir, bool bIsFullImpactImpulse);

	/** Max undamped Z Impulse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactJumping)
	float MaxUndampedImpulse;

	/** Add an impulse, damped if player would end up going too fast */
	UFUNCTION(BlueprintCallable, Category = "Impulse")
	virtual void AddDampedImpulse(FVector Impulse, bool bSelfInflicted);

	/** Clear pending impulse. */
	virtual void ClearPendingImpulse();

	/** true if projectile/hitscan spawned this frame (replicated to synchronize held firing) */
	UPROPERTY()
	bool bShotSpawned;

	//=========================================
	// DODGING
	/** whether dodging is allowed at all */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
	bool bCanDodge;
	/** Dodge impulse in XY plane */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Dodge Impulse - Horizontal"))
	float DodgeImpulseHorizontal;

	/** Dodge impulse added in Z direction */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Dodge Impulse - Vertical"))
	float DodgeImpulseVertical;

	/** How far to trace looking for a wall to dodge from */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Wall Dodge Trace Distance"))
	float WallDodgeTraceDist;

	/** Wall Dodge impulse in XY plane */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Wall Dodge Impulse - Horizontal"))
		float WallDodgeImpulseHorizontal;

	/** Vertical impulse for first wall dodge. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Wall Dodge Impulse - Vertical"))
	float WallDodgeImpulseVertical;

	/** Vertical impulse for subsequent consecutive wall dodges. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float WallDodgeSecondImpulseVertical;

	/** Grace negative velocity which is zeroed before adding walldodgeimpulse */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float WallDodgeGraceVelocityZ;

	/** Minimum Normal of Wall Dodge from wall (1.0 is 90 degrees, 0.0 is along wall, 0.7 is 45 degrees). */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float WallDodgeMinNormal;

	/** Wall normal of most recent wall dodge */
	UPROPERTY(Category = "Dodging", BlueprintReadOnly)
	FVector LastWallDodgeNormal;

	/** Max dot product of previous wall normal and new wall normal to perform a dodge (to prevent dodging along a wall) */
	UPROPERTY(Category = "Dodging", BlueprintReadOnly)
	float MaxConsecutiveWallDodgeDP;

	/** Max number of consecutive wall dodges without landing. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		int32 MaxWallDodges;

	/** Current count of wall dodges. */
	UPROPERTY(Category = "Dodging", BlueprintReadWrite)
		int32 CurrentWallDodgeCount;

	/** Time after starting wall dodge before another can be attempted. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float WallDodgeResetInterval;

	/** If falling faster than this speed, then don't add wall dodge impulse. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
	float MinAdditiveDodgeFallSpeed;

	/** Max positive Z speed with additive Wall Dodge Vertical Impulse.  Wall Dodge will not add impulse making vertical speed higher than this amount. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
	float MaxAdditiveDodgeJumpSpeed;

	/** Horizontal speed reduction on dodge landing (multiplied). */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeLandingSpeedFactor;

	/** Horizontal speed reduction on dodge jump landing (multiplied). */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeJumpLandingSpeedFactor;

	/** Time after landing dodge before another can be attempted. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeResetInterval;

	/** Adjustment from DodgeResetInterval for when bIsDodgeLanding acceleration adjust ends. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeLandingTimeAdjust;

	/** Time after landing dodge-jump before another can be attempted. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeJumpResetInterval;

	/** Maximum XY velocity of dodge (dodge impulse + current movement combined). */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadWrite)
		float DodgeMaxHorizontalVelocity;

	/** World time when another dodge can be attempted. */
	UPROPERTY(Category = "Dodging", BlueprintReadWrite)
		float DodgeResetTime;

	/** Acceleration during a dodge landing. */
	UPROPERTY(Category = "Dodging", BlueprintReadOnly)
		float DodgeLandingAcceleration;

	/** If falling, verify can wall dodge.  This causes character to dodge. */
	UFUNCTION()
	bool PerformDodge(FVector &DodgeDir, FVector &DodgeCross);

	virtual bool CanWallDodge(const FVector &DodgeDir, const FVector &DodgeCross, FHitResult& Result, bool bIsLowGrav);

	/** True during a dodge. */
	UPROPERTY(Category = "Dodging", BlueprintReadOnly)
	bool bIsDodging;

	/** True during a floor slide. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	bool bIsFloorSliding;

	/** True if was floor sliding last movement update. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	bool bWasFloorSliding;

	UPROPERTY(Category = "Taunt", BlueprintReadOnly)
	bool bIsTaunting;
	
protected:
	/** True if player is holding modifier to floor slide.  Change with UpdateFloorSlide(). */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	bool bWantsFloorSlide;

	/** True if player is holding modifier to wall slide.  Change with UpdateWallSlide(). */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	bool bWantsWallSlide;

	/** true if wall slide stat should be updated.  Needed so we don't double count wallslides. */
	bool bCountWallSlides;

	/** true if wall hit has played. */
	bool bHasPlayedWallHitSound;

	/** set during ClientAdjustPosition() */
	bool bProcessingClientAdjustment;
public:
	inline bool IsProcessingClientAdjustment() const
	{
		return bProcessingClientAdjustment;
	}

	/** accessor to commonly used elements of CurrentFloor to avoid cumbersome/expensive "break" nodes */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Movement: Walking")
	void GetSimpleFloorInfo(FVector& ImpactPoint, FVector& Normal) const;

	/** Horizontal speed reduction on slide ending (multiplied). */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
	float FloorSlideEndingSpeedFactor;

	/** Acceleration during a floor slide. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	float FloorSlideAcceleration;

	/** Max speed during a floor slide (decelerates to this if too fast). */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	float MaxFloorSlideSpeed;

	/** Max initial speed for a floor slide. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
		float MaxInitialFloorSlideSpeed;

	/** How long floor slide lasts. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	float FloorSlideDuration;

	/** When floor slide ends. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	float FloorSlideEndTime;

	/** When floor slide button was last tapped. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
	float FloorSlideTapTime;

	/** Maximum interval floor slide tap can be performed before landing dodge to get bonus. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
	float FloorSlideBonusTapInterval;

	/** JumpZ velocity for pre-slide jump when press slide button without first being in the air. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
		float FloorSlideJumpZ;

	/** true when press slide button without first being in the air. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
		bool bSlideFromGround;

	virtual void ClearFloorSlideTap();

	FTimerHandle FloorSlideTapHandle;

	/** Amount of falling damage reduction */
	UFUNCTION(BlueprintCallable, Category = "FloorSlide")
	virtual	float FallingDamageReduction(float FallingDamage, const FHitResult& Hit);

	/** Scaling for how much upward slope affects max floor slide initial speed. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
		float FloorSlideSlopeBraking;
	
	/** Enables slope dodge boost. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadOnly)
	bool bAllowSlopeDodgeBoost;

	/** Affects amount of slope dodge possible. */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadOnly)
	float SlopeDodgeScaling;

	/** Update bWantsFloorSlide and FloorSlideTapTime */
	UFUNCTION(BlueprintCallable, Category = "FloorSlide")
	virtual void UpdateFloorSlide(bool bNewWantsFloorSlide);

	/** Update bWantsWallSlide */
	UFUNCTION(BlueprintCallable, Category = "WallSlide")
	virtual void UpdateWallSlide(bool bNewWantsWallSlide);

	/** returns current bWantsFloorSlide */
	UFUNCTION(BlueprintCallable, Category = "FloorSlide")
	virtual bool WantsFloorSlide();

	/** returns current bWantsWallSlide */
	UFUNCTION(BlueprintCallable, Category = "FloorSlide")
		virtual bool WantsWallSlide();

	virtual void HandleSlideRequest();
	virtual void HandlePressedSlide();

	virtual void HandleCrouchRequest();

	virtual void HandleUnCrouchRequest();

	virtual void Crouch(bool bClientSimulation = false) override;

	/** floor slide out (holding floor slide while dodging on ground) */
	virtual void PerformFloorSlide(const FVector& DodgeDir, const FVector& FloorNormal);

	virtual bool IsCrouching() const override;

	// Flags used to synchronize dodging in networking (analoguous to bPressedJump)
	bool bPressedDodgeForward;
	bool bPressedDodgeBack;
	bool bPressedDodgeLeft;
	bool bPressedDodgeRight;

	/** Flag to synchronize tap slide */
	bool bPressedSlide;

	/** return true if weapon is preventing controlled movement */
	bool IsRootedByWeapon() const;

	/** Return true if character can dodge. */
	virtual bool CanDodge();

	/** Return true if character can jump. */
	virtual bool CanJump();

	/** Clear dodging input related flags */
	virtual void ClearDodgeInput();

	/** Handle jump inputs */
	virtual void CheckJumpInput(float DeltaTime);

	virtual void GetDodgeDirection(FVector& OutDodgeDir, FVector & OutDodgeCross) const;

	/** Optionally allow slope dodge */
	virtual FVector ComputeSlideVectorUT(const float DeltaTime, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit);

	//=========================================
	// MULTIJUMP

	/** Max number of multijumps. 2= double jump. */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Multijump Count"))
	int32 MaxMultiJumpCount;

	/** Current count of multijumps. */
	UPROPERTY(Category = "Multijump", BlueprintReadWrite, meta = (DisplayName = "Current Multijump Count"))
	int32 CurrentMultiJumpCount;

	/** True if player is falling because of a jump or dodge. */
	UPROPERTY(Category = "Multijump", BlueprintReadWrite)
	bool bExplicitJump;

	/** Whether to allow multijumps during a dodge. */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite)
	bool bAllowDodgeMultijumps;

	/** Whether to allow multijumps during a jump. */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite)
	bool bAllowJumpMultijumps;

	/** Max absolute Z velocity allowed for multijump (low values mean only near jump apex). */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Multijump Z Speed"))
	float MaxMultiJumpZSpeed;

	/** if set, always allow multijump while Velocity.Z < 0 */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite)
	bool bAlwaysAllowFallingMultiJump;

	/** Vertical impulse on multijump. */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Multijump impulse (vertical)"))
	float MultiJumpImpulse;

	/** Vertical impulse on dodge jump. */
	UPROPERTY(Category = "Multijump", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Dodge jump impulse (vertical)"))
	float DodgeJumpImpulse;

	/** Air control during multijump . */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Multijump)
	float MultiJumpAirControl;

	/** Air control during dodge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Multijump)
		float DodgeAirControl;

	/** if set air control is forced to zero during the current jump/fall */
	UPROPERTY(BlueprintReadWrite, Category = AirControl)
	bool bRestrictedJump;

	virtual float GetCurrentAirControl();

	FTimerHandle ClearRestrictedJumpHandle;

	/** Call to restrict air control for part of a jump/fall. */
	virtual void RestrictJump(float RestrictedJumpTime);

	/** allows timed clearing of bRestrictedJump during the jump/fall (e.g. jump pads that only want to restrict for part of the jump) */
	UFUNCTION()
	virtual void ClearRestrictedJump();

	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;

	virtual void OnTeleported() override;

	virtual void OnLineUp();

	virtual bool DoJump(bool bReplayingMoves) override;

	/** Perform a multijump */
	virtual bool DoMultiJump();

	/** Return true if can multijump now. */
	virtual bool CanMultiJump();

	/** True when just landed a dodge and acceleration is still limited. */
	UPROPERTY(Category = "Dodging", EditAnywhere, BlueprintReadOnly)
		bool bIsDodgeLanding;

	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;

	//=========================================
	// Landing Assist

	/** Defines what jumps just barely missed */
	UPROPERTY(Category = "LandingAssist", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Landing Step Up Distance"))
	float LandingStepUp;

	/** Boost to help successfully land jumps that just barely missed */
	UPROPERTY(Category = "LandingAssist", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Landing Assist Boost"))
	float LandingAssistBoost;

	/** True if already assisted this jump */
	UPROPERTY(Category = "LandingAssist", BlueprintReadOnly, meta = (DisplayName = "Jump Assisted"))
		bool bJumpAssisted;

	/** True if already assisted this jump */
	UPROPERTY(Category = "Jump", BlueprintReadOnly)
		float JumpTime;

	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	/** Return true if found a landing assist spot, and add LandingAssistBoost */
	virtual void FindValidLandingSpot(const FVector& CapsuleLocation);

	/** Check for landing assist */
	virtual void NotifyJumpApex() override;

	//=========================================
	// Wall Run

	/**  Max Falling velocity Z to get wall run */
	UPROPERTY(Category = "Wall Run", EditAnywhere, BlueprintReadWrite)
	float MaxWallRunFallZ;

	/**  Max jump positive velocity Z to be considered for wall running */
	UPROPERTY(Category = "Wall Run", EditAnywhere, BlueprintReadWrite)
	float MaxWallRunRiseZ;

	/** Gravity reduction during wall run */
	UPROPERTY(Category = "Wall Run", EditAnywhere, BlueprintReadWrite)
	float WallRunGravityScaling;

	/** Minimum horizontal velocity to wall run */
	UPROPERTY(Category = "Wall Run", EditAnywhere, BlueprintReadWrite)
	float MinWallSlideSpeed;

	/** Maximum dist to wall for wallslide to continue */
	UPROPERTY(Category = "Wall Run", EditAnywhere, BlueprintReadWrite)
		float MaxSlideWallDist;

	/** If true, the player is against the wall and WallSlideNormal will describe the touch. */
	UPROPERTY(Category = "Wall Run", BlueprintReadOnly)
	bool bIsAgainstWall;

	/** Used to gate client=side checking whether other characters are falling against a wall. */
	UPROPERTY()
	float LastCheckedAgainstWall;

	/** Normal of the wall we are wall running against. */
	UPROPERTY(Category = "Wall Run", BlueprintReadOnly)
	FVector WallSlideNormal;

	/** Wall material we are wall running against. */
	UPROPERTY(Category = "Wall Run", BlueprintReadOnly)
		UPhysicalMaterial* WallRunMaterial;

	virtual void HandleImpact(FHitResult const& Impact, float TimeSlice, const FVector& MoveDelta) override;

	virtual float GetGravityZ() const override;
	
	/** Set bApplyWallSlide if should slide against wall we are currently touching */
	virtual void CheckWallSlide(FHitResult const& Impact);

	//=========================================
	// Networking

	virtual void SendClientAdjustment() override;

	virtual void SmoothClientPosition(float DeltaTime) override;

	virtual void SimulateMovement(float DeltaTime) override;
	virtual void SimulateMovement_Internal(float DeltaTime);

	/** Used for remote client simulation */
	UPROPERTY()
	FVector SimulatedVelocity;

	/** Time server is using for this move, from timestamp passed by client */
	UPROPERTY()
	float CurrentServerMoveTime;

	/** Return world time on client, CurrentClientTimeStamp on server */
	virtual float GetCurrentMovementTime() const;

	/** Return synchronized time (timestamp currently being used on server, timestamp being sent on client) */
	virtual float GetCurrentSynchTime() const;

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) override;

	/** Accumulated timestamp error. */
	float TotalTimeStampError;

	/** true if currently clearing potential speed hack. */
	bool bClearingSpeedHack;

	virtual void StopActiveMovement() override;
};

// Networking support
class UNREALTOURNAMENT_API FSavedMove_UTCharacter : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	FSavedMove_UTCharacter()
	{
		AccelMagThreshold = 2000.f;
		AccelDotThreshold = 0.8f;
		bShotSpawned = false;
	}

	/** true if projectile/hitscan spawned this frame, not from firing press/release. */
	bool bShotSpawned;

	// Flags used to synchronize dodging in networking (analoguous to bPressedJump)
	bool bPressedDodgeForward;
	bool bPressedDodgeBack;
	bool bPressedDodgeLeft;
	bool bPressedDodgeRight;
	bool bSavedIsDodgeLanding;
	bool bSavedIsRolling;
	bool bSavedWantsWallSlide;
	bool bSavedWantsSlide;
	bool bPressedSlide;

	// local only properties (not replicated) used when replaying moves
	int32 SavedMultiJumpCount;
	int32 SavedWallDodgeCount;
	float SavedDodgeResetTime;
	float SavedFloorSlideEndTime;
	bool bSavedJumpAssisted;
	bool bSavedIsDodging;

	// return true if rotation affects this moves implementation
	virtual bool NeedsRotationSent() const;

	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMove) const override;
	virtual bool IsCriticalMove(const FSavedMovePtr& LastAckedMove) const;
	virtual void PostUpdate(class ACharacter* C, EPostUpdateMode PostUpdateMode) override;
	virtual void PrepMoveFor(class ACharacter* C) override;
};


class UNREALTOURNAMENT_API FNetworkPredictionData_Client_UTChar : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_UTChar( const UCharacterMovementComponent& ClientMovement ) : FNetworkPredictionData_Client_Character( ClientMovement ) {}

	/** Allocate a new saved move. Subclasses should override this if they want to use a custom move class. */
	virtual FSavedMovePtr AllocateNewMove() override;
};





