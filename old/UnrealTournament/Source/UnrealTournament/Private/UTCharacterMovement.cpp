// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTCharacterMovement.h"
#include "GameFramework/GameNetworkManager.h"
#include "UTLift.h"
#include "UTReachSpec_Lift.h"
#include "UTWaterVolume.h"
#include "UTBot.h"
#include "StatNames.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "UTCustomMovementTypes.h"

const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps

UUTCharacterMovement::UUTCharacterMovement(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Reduce for testing of transitions
	// MinTimeBetweenTimeStampResets = 5.f;
	MinTimeBetweenTimeStampResets = 10000.f;

	MaxWalkSpeed = 940.f;
	MaxCustomMovementSpeed = MaxWalkSpeed;

	WallDodgeTraceDist = 50.f;
	MinAdditiveDodgeFallSpeed = -5000.f;  
	MaxAdditiveDodgeJumpSpeed = 700.f;  
	MaxMultiJumpCount = 0;
	bAllowDodgeMultijumps = true;
	bAllowJumpMultijumps = true;
	bIsDoubleJumpAvailableForFlagCarrier = true;
	MultiJumpImpulse = 600.f;
	bCanDodge = true;
	DodgeJumpImpulse = 600.f;
	DodgeLandingSpeedFactor = 1.f;
	DodgeJumpLandingSpeedFactor = 1.f;
	DodgeResetInterval = 0.35f;
	DodgeJumpResetInterval = 0.35f;
	WallDodgeResetInterval = 0.2f;
	LandingStepUp = 40.f;
	LandingAssistBoost = 430.f;
	CrouchedSpeedMultiplier_DEPRECATED = 0.31f;
	MaxWalkSpeedCrouched = 315.f;
	MaxWallDodges = 99;
	WallDodgeMinNormal = 0.5f; 
	MaxConsecutiveWallDodgeDP = 0.97f;
	WallDodgeGraceVelocityZ = -2400.f;
	AirControl = 0.55f;
	MultiJumpAirControl = 0.55f;
	DodgeAirControl = 0.41f;
	bAllowSlopeDodgeBoost = true;
	SetWalkableFloorZ(0.695f); 
	MaxAcceleration = 3200.f; 
	MaxFallingAcceleration = 3200.f;
	MaxSwimmingAcceleration = 3200.f;
	MaxRelativeSwimmingAccelNumerator = 0.f;
	MaxRelativeSwimmingAccelDenominator = 1000.f;
	BrakingDecelerationWalking = 520.f;
	DefaultBrakingDecelerationWalking = BrakingDecelerationWalking;
	BrakingDecelerationFalling = 0.f;
	BrakingDecelerationSwimming = 300.f;
	BrakingDecelerationSliding = 300.f;
	GroundFriction = 10.5f;
	BrakingFriction = 5.f;
	GravityScale = 1.f;
	MaxStepHeight = 51.0f;
	NavAgentProps.AgentStepHeight = MaxStepHeight; // warning: must be manually mirrored, won't be set automatically
	CrouchedHalfHeight = 69.0f;
	SlopeDodgeScaling = 0.93f;
	bSlideFromGround = false;
	bHasPlayedWallHitSound = false;
	DodgeLandingTimeAdjust = -0.25f;
	DodgeLandingAcceleration = 1000.f;

	FastInitialAcceleration = 12000.f;
	MaxFastAccelSpeed = 200.f;

	FloorSlideAcceleration = 400.f;
	MaxFloorSlideSpeed = 900.f;
	MaxInitialFloorSlideSpeed = 1350.f;
	FloorSlideDuration = 0.7f;
	FloorSlideBonusTapInterval = 0.17f;
	FloorSlideEndingSpeedFactor = 0.4f;
	FloorSlideSlopeBraking = 2.7f;

	MaxSwimSpeed = 1000.f;
	MaxWaterSpeed = 450.f; 
	Buoyancy = 0.95f;
	SwimmingWallPushImpulse = 730.f;

	MaxMultiJumpZSpeed = 280.f;
	JumpZVelocity = 730.f;
	DodgeImpulseHorizontal = 1500.f;
	DodgeMaxHorizontalVelocity = 1700.f; 
	WallDodgeSecondImpulseVertical = 320.f;
	DodgeImpulseVertical = 500.f;
	WallDodgeImpulseHorizontal = 1300.f; 
	WallDodgeImpulseVertical = 470.f; 

	MaxWallRunRiseZ = 650.f; 
	MaxWallRunFallZ = -120.f;
	WallRunGravityScaling = 0.08f;
	MinWallSlideSpeed = 500.f;
	MaxSlideWallDist = 20.f;
	FloorSlideJumpZ = 50.f;

	NavAgentProps.bCanCrouch = true;

	// initialization of transient properties
	bIsDodgeLanding = false;
	bJumpAssisted = false;					
	DodgeResetTime = 0.f;					
	bIsDodging = false;					
	bIsFloorSliding = false;				
	FloorSlideTapTime = 0.f;					
	FloorSlideEndTime = 0.f;					
	CurrentMultiJumpCount = 0;	
	bExplicitJump = false;
	CurrentWallDodgeCount = 0;				
	bWantsFloorSlide = false;	
	bWantsWallSlide = false;
	bCountWallSlides = true;
	LastCheckedAgainstWall = 0.f;
	bIsSettingUpFirstReplayMove = false;
	bUseFlatBaseForFloorChecks = true;

	EasyImpactImpulse = 1100.f;
	EasyImpactDamage = 25;
	FullImpactImpulse = 1600.f;
	FullImpactDamage = 40;
	ImpactMaxHorizontalVelocity = 1500.f;
	ImpactMaxVerticalFactor = 1.f;
	MaxUndampedImpulse = 2000.f;

	OutofWaterZ = 700.f;
	JumpOutOfWaterPitch = -90.f;
	bFallingInWater = false;

	MaxPositionErrorSquared = 5.f;
	LastClientAdjustmentTime = -1.f;
	LastGoodMoveAckTime = -1.f;
	MinTimeBetweenClientAdjustments = 0.1f;
	bLargeCorrection = false;
	LargeCorrectionThreshold = 15.f;

	TotalTimeStampError = -0.15f;  // allow one initial slow frame
	bClearingSpeedHack = false;
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
}

// @todo UE4 - handle lift moving up and down through encroachment
void UUTCharacterMovement::UpdateBasedMovement(float DeltaSeconds)
{
	Super::UpdateBasedMovement(DeltaSeconds);

	if (HasValidData())
	{
		// check for bot lift jump
		AUTBot* B = Cast<AUTBot>(CharacterOwner->Controller);
		if (B != NULL && CharacterOwner->CanJump())
		{
			FVector LiftVelocity = (CharacterOwner->GetMovementBase() != NULL) ? CharacterOwner->GetMovementBase()->GetComponentVelocity() : FVector::ZeroVector;
			if (!LiftVelocity.IsZero())
			{
				UUTReachSpec_Lift* LiftPath = NULL;
				FUTPathLink LiftPathLink;
				int32 ExitRouteIndex = INDEX_NONE;
				if ((B->GetCurrentPath().ReachFlags & R_JUMP) && B->GetCurrentPath().Spec.IsValid())
				{
					LiftPath = Cast<UUTReachSpec_Lift>(B->GetCurrentPath().Spec.Get());
					LiftPathLink = B->GetCurrentPath();
				}
				// see if bot's next path is a lift jump (need to check this for fast moving lifts, because CurrentPath won't change until bot reaches lift center which in certain cases might be too late)
				else if (B->GetMoveTarget().Node != NULL)
				{
					for (int32 i = 0; i < B->RouteCache.Num() - 1; i++)
					{
						if (B->RouteCache[i] == B->GetMoveTarget())
						{
							int32 LinkIndex = B->GetMoveTarget().Node->GetBestLinkTo(B->GetMoveTarget().TargetPoly, B->RouteCache[i + 1], CharacterOwner, CharacterOwner->GetNavAgentPropertiesRef(), GetUTNavData(GetWorld()));
							if (LinkIndex != INDEX_NONE && (B->GetMoveTarget().Node->Paths[LinkIndex].ReachFlags & R_JUMP) && B->GetMoveTarget().Node->Paths[LinkIndex].Spec.IsValid())
							{
								LiftPathLink = B->GetMoveTarget().Node->Paths[LinkIndex];
								LiftPath = Cast<UUTReachSpec_Lift>(B->GetMoveTarget().Node->Paths[LinkIndex].Spec.Get());
								ExitRouteIndex = i + 1;
							}
						}
					}
				}
				if (LiftPath != NULL)
				{
					const FVector PawnLoc = CharacterOwner->GetActorLocation();
					float XYSize = (LiftPath->LiftExitLoc - PawnLoc).Size2D();
					// test with slightly less than actual velocity to provide some room for error and so bots aren't perfect all the time
					const float LiftJumpZ = (LiftVelocity.Z + JumpZVelocity * 0.95f);
					bool bShouldJump = false;
					// special case for lift jumps that are meant to go nowhere (pickup on ceiling, etc)
					if (XYSize < CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * 2.0f)
					{
						const float ZDiff = LiftPath->LiftExitLoc.Z - PawnLoc.Z;
						// if there is any solution to the constant accel equation, then we can hit the target with a lift jump
						// 0 = 0.5*GravityZ*t^2 + JumpVel.Z*t + (-ZDiff)
						bShouldJump = FMath::Square<float>(LiftJumpZ) -(2.0f * GetGravityZ() * (-ZDiff)) > 0.0f;
					}
					else
					{
						float XYTime = XYSize / MaxWalkSpeed;
						float ZTime = 0.0f;
						{
							float Determinant = FMath::Square(LiftJumpZ) - 2.0 * GetGravityZ() * (PawnLoc.Z - LiftPath->LiftExitLoc.Z);
							if (Determinant >= 0.0f)
							{
								Determinant = FMath::Sqrt(Determinant);
								float Time1 = (-LiftJumpZ + Determinant) / GetGravityZ();
								float Time2 = (-LiftJumpZ - Determinant) / GetGravityZ();
								if (Time1 > 0.0f)
								{
									if (Time2 > 0.0f)
									{
										ZTime = FMath::Min<float>(Time1, Time2);
									}
									else
									{
										ZTime = Time1;
									}
								}
								else if (Time2 > 0.0f)
								{
									ZTime = Time2;
								}
							}
						}
						const float JumpEndZ = PawnLoc.Z + LiftJumpZ * XYTime + 0.5f * GetGravityZ() * FMath::Square<float>(XYTime);
						bShouldJump = (ZTime > XYTime || JumpEndZ >= LiftPath->LiftExitLoc.Z);

						// consider delaying more if lift is known to have significant travel time remaining
						// no reason to make jump unnecessarily difficult
						if (bShouldJump && LiftPath->Lift != NULL && ZTime <= XYTime && JumpEndZ < LiftPath->LiftExitLoc.Z + CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())
						{
							TArray<FVector> Stops = LiftPath->Lift->GetStops();
							if (Stops.Num() > 1)
							{
								float ClosestDist = FLT_MAX;
								int32 ClosestStop = INDEX_NONE;
								for (int32 i = 0; i < Stops.Num(); i++)
								{
									float Dist = (Stops[i] - LiftPath->LiftExitLoc).Size();
									if (Dist < ClosestDist)
									{
										ClosestStop = i;
										ClosestDist = Dist;
									}
								}
								if (ClosestStop != INDEX_NONE)
								{
									int32 PrevStop;
									if (ClosestStop == 0)
									{
										PrevStop = 1;
									}
									else if (ClosestStop == Stops.Num() - 1)
									{
										PrevStop = Stops.Num() - 2;
									}
									else if (((Stops[ClosestStop - 1] - Stops[ClosestStop]).GetSafeNormal() | LiftPath->Lift->GetVelocity().GetSafeNormal()) < ((Stops[ClosestStop + 1] - Stops[ClosestStop]).GetSafeNormal() | LiftPath->Lift->GetVelocity().GetSafeNormal()))
									{
										PrevStop = ClosestStop - 1;
									}
									else
									{
										PrevStop = ClosestStop + 1;
									}
									if ((LiftPath->Lift->GetActorLocation() - Stops[ClosestStop]).Size() > (LiftPath->Lift->GetActorLocation() - Stops[PrevStop]).Size())
									{
										bShouldJump = false;
									}
								}
							}
						}
					}
					if (bShouldJump)
					{
						// jump!
						FVector DesiredVel2D;
						if (LiftPath->bSkipInitialAirControl)
						{
							Velocity = FVector::ZeroVector;
						}
						else if (B->FindBestJumpVelocityXY(DesiredVel2D, CharacterOwner->GetActorLocation(), LiftPath->LiftExitLoc, LiftVelocity.Z + JumpZVelocity, GetGravityZ(), CharacterOwner->GetSimpleCollisionHalfHeight()))
						{
							Velocity = FVector(DesiredVel2D.X, DesiredVel2D.Y, 0.0f).GetClampedToMaxSize2D(MaxWalkSpeed);
						}
						else
						{
							Velocity = (LiftPath->LiftExitLoc - PawnLoc).GetSafeNormal2D() * MaxWalkSpeed;
						}
						DoJump(false);
						// redirect bot to next point on route if necessary
						if (ExitRouteIndex != INDEX_NONE)
						{
							TArray<FComponentBasedPosition> MovePoints;
							new(MovePoints) FComponentBasedPosition(LiftPath->LiftExitLoc);
							B->SetMoveTarget(B->RouteCache[ExitRouteIndex], MovePoints, LiftPathLink);
						}
						else if (B->GetMoveBasedPosition().Base != NULL && B->GetMoveBasedPosition().Base->GetOwner() == LiftPath->Lift)
						{
							// result of bug where bot didn't use an entry path to get on the lift due to navmesh structure, see UUTReachSpec_Lift::GetMovePoints()
							B->SetAdjustLoc(LiftPath->LiftExitLoc);
							B->MoveTimer = -1.0f;
						}
						else
						{
							// this makes sure the bot skips any leftover point on the lift center that it doesn't need anymore
							B->SetMoveTargetDirect(B->GetMoveTarget(), B->GetCurrentPath());
						}
					}
				}
			}
		}
	}
}

void UUTCharacterMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	bSlidingAlongWall = false;
	if (MovementMode == MOVE_Swimming)
	{
		FVector HorizontalVelocity = Velocity;
		HorizontalVelocity.Z = 0.f;
		float Speed2D = HorizontalVelocity.Size2D();
		if (Speed2D > MaxSwimSpeed)
		{
			// clamp speed to MaxSwimSpeed
			HorizontalVelocity = MaxSwimSpeed * HorizontalVelocity.GetSafeNormal();
			Velocity.X = HorizontalVelocity.X;
			Velocity.Y = HorizontalVelocity.Y;
		}
		else if (Speed2D > MaxWaterSpeed)
		{
			// damp speed if above MaxWaterSpeed
			float ScalingFactor = FMath::Max(0.6f, MaxWaterSpeed / Speed2D);
			Velocity.X *= ScalingFactor;
			Velocity.Y *= ScalingFactor;
		}
		if (PreviousMovementMode == MOVE_Falling)
		{
			ClearFallingStateFlags();
			AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
			if (UTCharOwner)
			{
				UTCharOwner->InventoryEvent(InventoryEventName::LandedWater);
			}
		}
	}
	else if ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOMMOVE_LineUp))
	{
		//not falling
		Velocity.Z = 0.f;
		
		// make sure we update our new floor/base on initial entry
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		AdjustFloorHeight();
		SetBaseFromFloor(CurrentFloor);
	}
}

void UUTCharacterMovement::ClearFallingStateFlags()
{
	bIsAgainstWall = false;
	bFallingInWater = false;
	bCountWallSlides = true;
	bIsFloorSliding = false;
	bIsDodging = false;
	bJumpAssisted = false;
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner)
	{
		UTCharOwner->bApplyWallSlide = false;
		if (UTCharOwner->Role != ROLE_Authority)
		{
			UTCharOwner->bRepFloorSliding = false;
		}
	}
	bExplicitJump = false;
	ClearRestrictedJump();
	CurrentMultiJumpCount = 0;
	CurrentWallDodgeCount = 0;
}

bool UUTCharacterMovement::CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump)
{
	bool bResult = Super::CheckFall(OldFloor, Hit, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump);
	if (!bResult)
	{
		// abort bot's move since it can't go any further
		AUTBot* B = Cast<AUTBot>(CharacterOwner->Controller);
		if (B != NULL)
		{
			B->MoveTimer = FMath::Min<float>(B->MoveTimer, -1.0f);
		}
	}
	return bResult;
}

FVector UUTCharacterMovement::GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir) const
{
	AUTBot* B = Cast<AUTBot>(CharacterOwner->Controller);
	if (B != NULL)
	{
		B->NotifyHitLedge();
	}
	return Super::GetLedgeMove(OldLocation, Delta, GravDir);
}

void UUTCharacterMovement::OnUnableToFollowBaseMove(const FVector& DeltaPosition, const FVector& OldLocation, const FHitResult& MoveOnBaseHit)
{
	UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();

	if (CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		// force it since on client
		UpdatedComponent->SetWorldLocationAndRotation(UpdatedComponent->GetComponentLocation() + DeltaPosition, UpdatedComponent->GetComponentQuat(), false);
		return;
	}

	// @TODO FIXMESTEVE handle case where we should adjust player position so he doesn't encroach, but don't need to make lift return
	// Test if lift is moving up/sideways, otherwise ignore this (may need more sophisticated test)
	if (MovementBase && Cast<AUTLift>(MovementBase->GetOwner()) && (MovementBase->GetOwner()->GetVelocity().Z >= 0.f))// && UpdatedComponent->IsOverlappingComponent(MovementBase))
	{
		Cast<AUTLift>(MovementBase->GetOwner())->OnEncroachActor(CharacterOwner);
	}

	// make sure bots on lift move to center in case they are causing the lift to fail by hitting their head on the sides
	AUTBot* B = Cast<AUTBot>(CharacterOwner->Controller);
	if (B != NULL)
	{
		UUTReachSpec_Lift* LiftPath = Cast<UUTReachSpec_Lift>(B->GetCurrentPath().Spec.Get());
		if (LiftPath != NULL)
		{
			B->SetAdjustLoc(LiftPath->LiftCenter + FVector(0.0f, 0.0f, B->GetCharacter()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
		}
	}
}

void UUTCharacterMovement::ApplyImpactVelocity(FVector JumpDir, bool bIsFullImpactImpulse)
{
	// provide scaled boost in facing direction, clamped to ImpactMaxHorizontalVelocity and ImpactMaxVerticalVelocity
	// @TODO FIXMESTEVE should use AddDampedImpulse()?
	float ImpulseMag = bIsFullImpactImpulse ? FullImpactImpulse : EasyImpactImpulse;
	FVector NewVelocity = Velocity + JumpDir * ImpulseMag;
	if (NewVelocity.Size2D() > ImpactMaxHorizontalVelocity)
	{
		float VelZ = NewVelocity.Z;
		NewVelocity = NewVelocity.GetSafeNormal2D() * ImpactMaxHorizontalVelocity;
		NewVelocity.Z = VelZ;
	}
	NewVelocity.Z = FMath::Min(NewVelocity.Z, ImpactMaxVerticalFactor*ImpulseMag);
	Velocity = NewVelocity;
	SetMovementMode(MOVE_Falling);
	bNotifyApex = true;
	NeedsClientAdjustment();
	AUTPlayerState* PS = CharacterOwner ? Cast<AUTPlayerState>(CharacterOwner->PlayerState) : NULL;
	if (PS)
	{
		PS->ModifyStatsValue(NAME_NumImpactJumps, 1);
	}
}

void UUTCharacterMovement::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	if (CharacterOwner == NULL)
	{
		return;
	}
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	Canvas->SetDrawColor(255, 255, 255);
	UFont* RenderFont = GEngine->GetSmallFont();
	FString T = FString::Printf(TEXT(""));
	if (IsInWater())
	{
		T = FString::Printf(TEXT("IN WATER"));
	}
	else if (bIsDodging)
	{
		T = FString::Printf(TEXT("DODGING %f"), CharacterOwner->GetWorld()->GetTimeSeconds());
	}
	else if (bIsFloorSliding)
	{
		T = FString::Printf(TEXT("DODGE ROLLING"));
	}
	Canvas->DrawText(RenderFont, T, 4.0f, YPos);
	YPos += YL;
	T = FString::Printf(TEXT("AVERAGE SPEED %f"), AvgSpeed);
	Canvas->DrawText(RenderFont, T, 4.0f, YPos);
	YPos += YL;
}

float UUTCharacterMovement::UpdateTimeStampAndDeltaTime(float DeltaTime, FNetworkPredictionData_Client_Character* ClientData)
{
	float UnModifiedTimeStamp = ClientData->CurrentTimeStamp + DeltaTime;
	DeltaTime = ClientData->UpdateTimeStampAndDeltaTime(DeltaTime, *CharacterOwner, *this);
	if (ClientData->CurrentTimeStamp < UnModifiedTimeStamp)
	{
		// client timestamp rolled over, so roll over our movement timers
		AdjustMovementTimers(UnModifiedTimeStamp - ClientData->CurrentTimeStamp);
	}
	return DeltaTime;
}

void UUTCharacterMovement::AdjustMovementTimers(float Adjustment)
{
	//UE_LOG(UTNet, Warning, TEXT("+++++++ROLLOVER time %f"), CurrentServerMoveTime); //MinTimeBetweenTimeStampResets
	DodgeResetTime -= Adjustment;
	FloorSlideTapTime -= Adjustment;
	FloorSlideEndTime -= Adjustment;
}

void UUTCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	AUTCharacter* UTOwner = Cast<AUTCharacter>(CharacterOwner);
	UMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bool bOwnerIsRagdoll = UTOwner && UTOwner->IsRagdoll();
	if (bOwnerIsRagdoll)
	{
		// ignore jump/slide key presses this frame since the character is in ragdoll and they don't apply
		UTOwner->bPressedJump = false;
		bPressedSlide = false;
		if (!UTOwner->GetController())
		{
			return;
		}
	}

	const FVector InputVector = ConsumeInputVector();
	if (!HasValidData() || ShouldSkipUpdate(DeltaTime) || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	// FIXMESTEVE do when character gets team, not every tick
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS && !GS->bTeamCollision && !bForceTeamCollision && UTOwner && Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			AUTCharacter* Char = It->IsValid() ? Cast<AUTCharacter>((*It).Get()) : nullptr;
			if (Char)
			{
				((UPrimitiveComponent *)(UpdatedComponent))->IgnoreActorWhenMoving(Char, GS->OnSameTeam(UTOwner, Char) && (UTOwner != Char) && !Char->IsPendingKillPending() && !Char->IsDead() && !Char->IsRagdoll());
			}
		}
	}

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		if (CharacterOwner->Role == ROLE_Authority)
		{
			// Check we are still in the world, and stop simulating if not.
			const bool bStillInWorld = (bCheatFlying || CharacterOwner->CheckStillInWorld());
			if (!bStillInWorld || !HasValidData())
			{
				return;
			}
		}

		// If we are a client we might have received an update from the server.
		const bool bIsClient = (GetNetMode() == NM_Client && CharacterOwner->Role == ROLE_AutonomousProxy);
		if (bIsClient && !bOwnerIsRagdoll)
		{
			ClientUpdatePositionAfterServerUpdate();
		}

		// Allow root motion to move characters that have no controller.
		if (CharacterOwner->IsLocallyControlled() || bRunPhysicsWithNoController || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
		{
			FNetworkPredictionData_Client_Character* ClientData = ((CharacterOwner->Role < ROLE_Authority) && (GetNetMode() == NM_Client)) ? GetPredictionData_Client_Character() : NULL;
			if (ClientData)
			{
				// Update our delta time for physics simulation.
				DeltaTime = UpdateTimeStampAndDeltaTime(DeltaTime, ClientData);
				CurrentServerMoveTime = ClientData->CurrentTimeStamp;
			}
			else
			{
				CurrentServerMoveTime = GetWorld()->GetTimeSeconds();
			}
			//UE_LOG(UTNet, Warning, TEXT("Correction COMPLETE velocity %f %f %f"), Velocity.X, Velocity.Y, Velocity.Z);
			// We need to check the jump state before adjusting input acceleration, to minimize latency
			// and to make sure acceleration respects our potentially new falling state.
			CharacterOwner->CheckJumpInput(DeltaTime);

			// apply input to acceleration
			Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
			AnalogInputModifier = ComputeAnalogInputModifier();

			if ((CharacterOwner->Role == ROLE_Authority) && !bOwnerIsRagdoll)
			{
				PerformMovement(DeltaTime);
			}
			else if (bIsClient)
			{
				ReplicateMoveToServer(DeltaTime, Acceleration);
			}
		}
		else if ((CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy) && !bOwnerIsRagdoll)
		{
			// Server ticking for remote client.
			// Between net updates from the client we need to update position if based on another object,
			// otherwise the object will move on intermediate frames and we won't follow it.
			MaybeUpdateBasedMovement(DeltaTime);
			SaveBaseLocation();
		}
		else if (!CharacterOwner->Controller && (CharacterOwner->Role == ROLE_Authority) && !bOwnerIsRagdoll)
		{
			// still update forces
			ApplyAccumulatedForces(DeltaTime);
			PerformMovement(DeltaTime);
		}
	}
	else if (!bOwnerIsRagdoll && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		AdjustProxyCapsuleSize();
		SimulatedTick(DeltaTime);
		CharacterOwner->RecalculateBaseEyeHeight();
	}
	if (bEnablePhysicsInteraction && !bOwnerIsRagdoll)
	{
		if (CurrentFloor.HitResult.IsValidBlockingHit())
		{
			// Apply downwards force when walking on top of physics objects
			if (UPrimitiveComponent* BaseComp = CurrentFloor.HitResult.GetComponent())
			{
				if (StandingDownwardForceScale != 0.f && BaseComp->IsAnySimulatingPhysics())
				{
					const float GravZ = GetGravityZ();
					const FVector ForceLocation = CurrentFloor.HitResult.ImpactPoint;
					BaseComp->AddForceAtLocation(FVector(0.f, 0.f, GravZ * Mass * StandingDownwardForceScale), ForceLocation, CurrentFloor.HitResult.BoneName);
				}
			}
		}
	}

	if (bOwnerIsRagdoll)
	{
		// ignore jump/slide key presses this frame since the character is in ragdoll and they don't apply
		UTOwner->bPressedJump = false;
		bPressedSlide = false;
	}
	AvgSpeed = AvgSpeed * (1.f - 2.f*DeltaTime) + 2.f*DeltaTime * Velocity.Size2D();
	if (CharacterOwner != NULL)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(CharacterOwner->Controller);
		if (PC != NULL && PC->PlayerInput != NULL)
		{
			PC->ApplyDeferredFireInputs();
		}
	}
}

void UUTCharacterMovement::ClearPendingImpulse()
{
	PendingImpulseToApply = FVector(0.f);
}

void UUTCharacterMovement::AddDampedImpulse(FVector Impulse, bool bSelfInflicted)
{
	if (HasValidData() && !Impulse.IsZero())
	{
		// handle scaling by mass
		FVector FinalImpulse = Impulse;
		if (Mass > SMALL_NUMBER)
		{
			FinalImpulse = FinalImpulse / Mass;
		}

		// dampen impulse if already traveling fast.  Shouldn't affect 1st rocket, but should affect 2 and 3
		// First dampen XY component that is in the direction of the current PendingVelocity
		float FinalImpulseZ = FinalImpulse.Z;
		FinalImpulse.Z = 0.f;
		FVector PendingVelocity = Velocity + PendingImpulseToApply;
		FVector PendingVelocityDir = PendingVelocity.GetSafeNormal();
		FVector AdditiveImpulse = PendingVelocityDir * (PendingVelocityDir | FinalImpulse);
		FVector OrthogonalImpulse = FinalImpulse - AdditiveImpulse;
		FVector ResultVelocity = PendingVelocity + AdditiveImpulse;
		float CurrentXYSpeed = PendingVelocity.Size2D();
		float ResultXYSpeed = ResultVelocity.Size2D();
		float XYDelta = ResultXYSpeed - CurrentXYSpeed;
		if (XYDelta > 0.f)
		{
			// reduce additive impulse further if current speed is already beyond dodge speed (implying this is 2nd or further impulse applied)
			float AboveDodgeFactor = CurrentXYSpeed / DodgeImpulseHorizontal;
			if (AboveDodgeFactor > 1.0f)
			{
				FinalImpulse = AdditiveImpulse / AboveDodgeFactor + OrthogonalImpulse;
			}

			float PctBelowRun = FMath::Clamp((MaxWalkSpeed - CurrentXYSpeed)/XYDelta, 0.f, 1.f);
			float PctBelowDodge = FMath::Clamp((DodgeImpulseHorizontal - CurrentXYSpeed)/XYDelta, 0.f ,1.f);
			float PctAboveDodge = FMath::Max(0.f, 1.f - PctBelowDodge);
			PctBelowDodge = FMath::Max(0.f, PctBelowDodge - PctBelowRun);
			FinalImpulse *= (PctBelowRun + PctBelowDodge + FMath::Max(0.5f, 1.f - PctAboveDodge)*PctAboveDodge);

			FVector FinalVelocityXY = PendingVelocity + FinalImpulse;
			FinalVelocityXY.Z = 0.f;
			float FinalXYSpeed = FinalVelocityXY.Size();
			if (FinalXYSpeed > DodgeMaxHorizontalVelocity)
			{
				FVector DesiredVelocity = FinalVelocityXY.GetSafeNormal() * DodgeMaxHorizontalVelocity;
				FinalImpulse = DesiredVelocity - PendingVelocity;
			}
		}
		FinalImpulse.Z = FinalImpulseZ;

		// Now for Z component
		float DampingThreshold = bSelfInflicted ? MaxUndampedImpulse : MaxAdditiveDodgeJumpSpeed;
		if ((FinalImpulse.Z > 0.f) && (FinalImpulse.Z + PendingVelocity.Z > DampingThreshold))
		{
			float PctBelowBoost = FMath::Clamp((DampingThreshold - PendingVelocity.Z) / DampingThreshold, 0.f, 1.f);
			FinalImpulse.Z *= (PctBelowBoost + (1.f - PctBelowBoost)*FMath::Max(0.25f, PctBelowBoost));
		}

		PendingImpulseToApply += FinalImpulse;
		NeedsClientAdjustment();
	}
}

FVector UUTCharacterMovement::GetImpartedMovementBaseVelocity() const
{
	FVector Result = Super::GetImpartedMovementBaseVelocity();

	if (!Result.IsZero())
	{
		// clamp total velocity to GroundSpeed+JumpZ+Imparted total TODO SEPARATE XY and Z
		float XYSpeed = ((Result.X == 0.f) && (Result.Y == 0.f)) ? 0.f : Result.Size2D();
		float MaxSpeedSq = FMath::Square(MaxWalkSpeed + Result.Size2D()) + FMath::Square(JumpZVelocity + Result.Z);
		if ((Velocity + Result).SizeSquared() > MaxSpeedSq)
		{
			Result = (Velocity + Result).GetSafeNormal() * FMath::Sqrt(MaxSpeedSq) - Velocity;
		}
		Result.Z = FMath::Max(Result.Z, 0.f);
	}

	return Result;
}

bool UUTCharacterMovement::IsRootedByWeapon() const
{
	AUTCharacter* UTOwner = Cast<AUTCharacter>(CharacterOwner);
	return (UTOwner != nullptr && UTOwner->GetWeapon() != nullptr && UTOwner->GetWeapon()->bRootWhileFiring && UTOwner->GetWeapon()->IsFiring());
}

bool UUTCharacterMovement::CanDodge()
{
/*	if (GetCurrentMovementTime() < DodgeResetTime)
	{
		UE_LOG(UTNet, Warning, TEXT("Failed dodge current move time %f dodge reset time %f"), GetCurrentMovementTime(), DodgeResetTime);
	}
	else
	{
		UE_LOG(UTNet, Warning, TEXT("SUCCEEDED candodge current move time %f dodge reset time %f"), GetCurrentMovementTime(), DodgeResetTime);
	}
*/
	return !bIsFloorSliding && bCanDodge && CanEverJump() && (GetCurrentMovementTime() > DodgeResetTime) && !IsRootedByWeapon();
}

bool UUTCharacterMovement::CanJump()
{
	return (IsMovingOnGround() || CanMultiJump()) && CanEverJump() && !bIsFloorSliding && !IsRootedByWeapon();
}

bool UUTCharacterMovement::IsCarryingFlag() const
{
	AUTCharacter* UTOwner = Cast<AUTCharacter>(CharacterOwner);
	return (UTOwner && UTOwner->GetCarriedObject());
}

void UUTCharacterMovement::PerformWaterJump()
{
	if (!HasValidData())
	{
		return;
	}

	float PawnCapsuleRadius, PawnCapsuleHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnCapsuleRadius, PawnCapsuleHalfHeight);
	FVector TraceStart = CharacterOwner->GetActorLocation();
	TraceStart.Z = TraceStart.Z - PawnCapsuleHalfHeight + PawnCapsuleRadius;
	FVector TraceEnd = TraceStart - FVector(0.f, 0.f, WallDodgeTraceDist);

	static const FName DodgeTag = FName(TEXT("Dodge"));
	FCollisionQueryParams QueryParams(DodgeTag, false, CharacterOwner);
	FHitResult Result;
	const bool bBlockingHit = GetWorld()->SweepSingleByChannel(Result, TraceStart, TraceEnd, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), FCollisionShape::MakeSphere(PawnCapsuleRadius), QueryParams);
	if (!bBlockingHit)
	{
		return;
	}
	DodgeResetTime = GetCurrentMovementTime() + WallDodgeResetInterval;
	if (!CharacterOwner->bClientUpdating)
	{
		//UE_LOG(UTNet, Warning, TEXT("Set dodge reset after wall dodge move time %f dodge reset time %f"), GetCurrentMovementTime(), DodgeResetTime);

		// @TODO FIXMESTEVE - character should be responsible for effects, should have blueprint event too
		AUTCharacter* UTCharacterOwner = Cast<AUTCharacter>(CharacterOwner);
		if (UTCharacterOwner)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), UTCharacterOwner->CharacterData.GetDefaultObject()->SwimPushSound, UTCharacterOwner, SRT_AllButOwner);
		}
	}
	LastWallDodgeNormal = Result.ImpactNormal;
	Velocity.Z = FMath::Max(Velocity.Z, SwimmingWallPushImpulse);
}

bool UUTCharacterMovement::CanWallDodge(const FVector &DodgeDir, const FVector &DodgeCross, FHitResult& Result, bool bIsLowGrav)
{
	FVector TraceEnd = -1.f*DodgeDir;
	float PawnCapsuleRadius, PawnCapsuleHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnCapsuleRadius, PawnCapsuleHalfHeight);
	float TraceBoxSize = FMath::Min(0.25f*PawnCapsuleHalfHeight, 0.7f*PawnCapsuleRadius);
	FVector TraceStart = CharacterOwner->GetActorLocation();
	TraceStart.Z -= 0.5f*TraceBoxSize;
	TraceEnd = TraceStart - (WallDodgeTraceDist + PawnCapsuleRadius - 0.5f*TraceBoxSize)*DodgeDir;

	static const FName DodgeTag = FName(TEXT("Dodge"));
	FCollisionQueryParams QueryParams(DodgeTag, false, CharacterOwner);
	const bool bBlockingHit = GetWorld()->SweepSingleByChannel(Result, TraceStart, TraceEnd, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), FCollisionShape::MakeSphere(TraceBoxSize), QueryParams);
	if (!bBlockingHit || Cast<ACharacter>(Result.Actor.Get()) || (!IsSwimming() && (CurrentWallDodgeCount > 0) && !bIsLowGrav && ((Result.ImpactNormal | LastWallDodgeNormal) > MaxConsecutiveWallDodgeDP)))
	{
		//UE_LOG(UTNet, Warning, TEXT("No wall to dodge"));
		return false;
	}
	return true;
}

bool UUTCharacterMovement::PerformDodge(FVector &DodgeDir, FVector &DodgeCross)
{
	if (!HasValidData())
	{
		return false;
	}
/*	
	FVector Loc = CharacterOwner->GetActorLocation();
	UE_LOG(UTNet, Warning, TEXT("Perform dodge at %f loc %f %f %f vel %f %f %f dodgedir %f %f %f from yaw %f"), GetCurrentSynchTime(), Loc.X, Loc.Y, Loc.Z, Velocity.X, Velocity.Y, Velocity.Z, DodgeDir.X, DodgeDir.Y, DodgeDir.Z, CharacterOwner->GetActorRotation().Yaw);
*/
	float HorizontalImpulse = DodgeImpulseHorizontal;
	bool bIsLowGrav = (GetGravityZ() > UPhysicsSettings::Get()->DefaultGravityZ);
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner)
	{
		bIsLowGrav = !UTCharOwner->bApplyWallSlide && bIsLowGrav;
	}
	NeedsClientAdjustment();
	bool bIsAWallDodge = false;
	bool bIsALiftJump = false;
	if (!IsMovingOnGround())
	{
		if (IsFalling() && (CurrentWallDodgeCount >= MaxWallDodges))
		{
			//UE_LOG(UTNet, Warning, TEXT("Exceeded max wall dodges"));
			return false;
		}
		// if falling/swimming, check if can perform wall dodge
		FHitResult Result;
		if (!CanWallDodge(DodgeDir, DodgeCross, Result, bIsLowGrav))
		{
			return false;
		}
		if ((Result.ImpactNormal | DodgeDir) < WallDodgeMinNormal)
		{
			// clamp dodge direction based on wall normal
			FVector ForwardDir = (Result.ImpactNormal ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
			if ((ForwardDir | DodgeDir) < 0.f)
			{
				ForwardDir *= -1.f;
			}
			DodgeDir = Result.ImpactNormal*WallDodgeMinNormal*WallDodgeMinNormal + ForwardDir*(1.f - WallDodgeMinNormal*WallDodgeMinNormal);
			DodgeDir = DodgeDir.GetSafeNormal();
			FVector NewDodgeCross = (DodgeDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
			DodgeCross = ((NewDodgeCross | DodgeCross) < 0.f) ? -1.f*NewDodgeCross : NewDodgeCross;
		}
		DodgeResetTime = GetCurrentMovementTime() + WallDodgeResetInterval;
		HorizontalImpulse = IsSwimming() ? SwimmingWallPushImpulse : WallDodgeImpulseHorizontal;
		CurrentWallDodgeCount++;
		LastWallDodgeNormal = Result.ImpactNormal;
		bIsAWallDodge = true;
		bCountWallSlides = true;
		bHasPlayedWallHitSound = false;
	}
	else if (!GetImpartedMovementBaseVelocity().IsZero())
	{
		// lift jump counts as wall dodge
		CurrentWallDodgeCount++;
		LastWallDodgeNormal = FVector(0.f, 0.f, 1.f);
		DodgeResetTime = GetCurrentMovementTime() + WallDodgeResetInterval;
		bIsALiftJump = true;
	}
	float VelocityZ = Velocity.Z;
	float MaxHorizontalVelocity = DodgeMaxHorizontalVelocity;
	// perform the dodge
	AUTPlayerState* UTPlayerState = UTCharOwner ? Cast<AUTPlayerState>(UTCharOwner->PlayerState) : nullptr;
	bool bSlowMovement = (UTPlayerState && UTPlayerState->CarriedObject && UTPlayerState->CarriedObject->bSlowsMovement);
	if (bSlowMovement)
	{
		HorizontalImpulse *= 0.8f;
		MaxHorizontalVelocity *= 0.8f;
	}
	Velocity = HorizontalImpulse*DodgeDir + (Velocity | DodgeCross)*DodgeCross;
	Velocity.Z = 0.f;
	float SpeedXY = FMath::Min(Velocity.Size(), MaxHorizontalVelocity);
	SpeedXY *= UTCharOwner ? UTCharOwner->MaxSpeedPctModifier : 1.0f;
	Velocity = SpeedXY*Velocity.GetSafeNormal();

	if (bSlowMovement)
	{
		Velocity.Z = IsMovingOnGround() ? 400.f : 200.f;
	}
	else if (IsMovingOnGround())
	{
		Velocity.Z = DodgeImpulseVertical;
	}
	else if (!IsSwimming() && (VelocityZ < MaxAdditiveDodgeJumpSpeed) && (VelocityZ > MinAdditiveDodgeFallSpeed))
	{
		float CurrentWallImpulse = (CurrentWallDodgeCount < 2) ? WallDodgeImpulseVertical : WallDodgeSecondImpulseVertical;

		if (!bIsLowGrav && (CurrentWallDodgeCount > 1))
		{
			VelocityZ = FMath::Min(0.f, VelocityZ);
		}
		else if ((VelocityZ < 0.f) && (VelocityZ > WallDodgeGraceVelocityZ))
		{
			if (Velocity.Z < -1.f * JumpZVelocity)
			{
				CurrentWallImpulse = FMath::Max(WallDodgeSecondImpulseVertical, WallDodgeImpulseVertical + Velocity.Z + JumpZVelocity);
			}
			// allowing dodge with loss of downward velocity is no free lunch for falling damage
			FHitResult Hit(1.f);
			Hit.ImpactNormal = FVector(0.f, 0.f, 1.f);
			Hit.Normal = Hit.ImpactNormal;
			Cast<AUTCharacter>(CharacterOwner)->TakeFallingDamage(Hit, VelocityZ - CurrentWallImpulse);
			VelocityZ = 0.f;
		}
		Velocity.Z = FMath::Min(VelocityZ + CurrentWallImpulse, MaxAdditiveDodgeJumpSpeed);
		//UE_LOG(UTNet, Warning, TEXT("Wall dodge at %f velZ %f"), CharacterOwner->GetWorld()->GetTimeSeconds(), Velocity.Z);
	}
	else
	{
		Velocity.Z = VelocityZ;
	}
	bExplicitJump = true;
	bIsDodging = true;
	bNotifyApex = true;
	if (IsMovingOnGround())
	{
		SetMovementMode(MOVE_Falling);
	}
	AUTPlayerState* PS = CharacterOwner ? Cast<AUTPlayerState>(CharacterOwner->PlayerState) : NULL;
	if (PS)
	{
		PS->ModifyStatsValue(bIsAWallDodge ? NAME_NumWallDodges : NAME_NumDodges, 1);  
		if (bIsALiftJump)
		{
			PS->ModifyStatsValue(NAME_NumLiftJumps, 1);
		}
	}
	return true;
}

void UUTCharacterMovement::HandlePressedSlide()
{
	bPressedSlide = true;
	if (IsMovingOnGround())
	{
		if (GetCurrentMovementTime() > FloorSlideEndTime + DodgeResetInterval)
		{
			CharacterOwner->bPressedJump = true;
		}
		else
		{
			bPressedSlide = false;
		}
	}
}

void UUTCharacterMovement::HandleSlideRequest()
{
	AUTCharacter* UTCharacterOwner = Cast<AUTCharacter>(CharacterOwner);
	if (!Acceleration.IsNearlyZero() && (Velocity.Size() > 0.5f * MaxWalkSpeed) && UTCharacterOwner && UTCharacterOwner->CanSlide())
	{
		HandlePressedSlide();
	}
}

void UUTCharacterMovement::HandleCrouchRequest()
{
	// if moving fast enough and pressing on move key, slide, else crouch
	AUTCharacter* UTCharacterOwner = Cast<AUTCharacter>(CharacterOwner);
	if (!Acceleration.IsNearlyZero() && (Velocity.Size() > 0.5f * MaxWalkSpeed) && UTCharacterOwner && UTCharacterOwner->CanSlide())
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(UTCharacterOwner->GetController());
		if (PC && PC->bCrouchTriggersSlide)
		{
			HandlePressedSlide();
		}
	}
	bWantsToCrouch = true;
}

void UUTCharacterMovement::HandleUnCrouchRequest()
{
	bWantsToCrouch = false;
	AUTPlayerController* PC = CharacterOwner ? Cast<AUTPlayerController>(CharacterOwner->GetController()) : nullptr;
	if (!PC || PC->bCrouchTriggersSlide)
	{
		UpdateFloorSlide(false);
	}
}

void UUTCharacterMovement::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	if (!bIsFloorSliding && CharacterOwner && CharacterOwner->bIsCrouched && (Velocity.Size2D() > MaxWalkSpeedCrouched))
	{
		float SavedVelZ = Velocity.Z;
		Velocity = MaxWalkSpeedCrouched * Velocity.GetSafeNormal2D();
		Velocity.Z = SavedVelZ;
	}
}

void UUTCharacterMovement::PerformFloorSlide(const FVector& DodgeDir, const FVector& FloorNormal)
{
	if (CharacterOwner)
	{
		FloorSlideTapTime = GetCurrentMovementTime();
		bIsFloorSliding = true;
		FloorSlideEndTime = GetCurrentMovementTime() + FloorSlideDuration;
		Acceleration = FloorSlideAcceleration * DodgeDir;
		DodgeResetTime = FloorSlideEndTime + DodgeResetInterval;
		float NewSpeed = FMath::Max(MaxFloorSlideSpeed, FMath::Min(Velocity.Size2D(), MaxInitialFloorSlideSpeed));
		if ((NewSpeed > MaxFloorSlideSpeed) && ((DodgeDir | FloorNormal) < 0.f))
		{
			// don't allow sliding up steep slopes at faster than MaxFloorSlideSpeed
			NewSpeed = FMath::Clamp(NewSpeed + FloorSlideSlopeBraking * (NewSpeed - MaxFloorSlideSpeed) * (DodgeDir | FloorNormal), MaxFloorSlideSpeed, NewSpeed);
		}
		Velocity = NewSpeed*DodgeDir;
		AUTCharacter* UTChar = Cast<AUTCharacter>(CharacterOwner);
		if (UTChar)
		{
			UTChar->MovementEventUpdated(EME_Slide, DodgeDir);
			if (UTChar->Role != ROLE_Authority)
			{
				UTChar->bRepFloorSliding = true;
			}
		}
		AUTPlayerState* PS = CharacterOwner ? Cast<AUTPlayerState>(CharacterOwner->PlayerState) : NULL;
		if (PS)
		{
			PS->ModifyStatsValue(NAME_NumFloorSlides, 1);
		}
	}
}

bool UUTCharacterMovement::IsCrouching() const
{
	return CharacterOwner && CharacterOwner->bIsCrouched && !bIsFloorSliding;
}

void UUTCharacterMovement::PerformMovement(float DeltaSeconds)
{
	if (!CharacterOwner)
	{
		return;
	}

	AUTCharacter* UTOwner = Cast<AUTCharacter>(CharacterOwner);
	bSlidingAlongWall = false;
	if (!UTOwner || !UTOwner->IsRagdoll())
	{
		FVector OldVelocity = Velocity;
		float RealGroundFriction = GroundFriction;
		if (Acceleration.IsZero())
		{
			GroundFriction = BrakingFriction;
		}
		else
		{
			// Flag this player as not being idle.
			AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(CharacterOwner->PlayerState);
			if (UTPlayerState)
			{
				UTPlayerState->NotIdle();
			}
		}
		if (bIsFloorSliding)
		{
			GroundFriction = 0.f;
			BrakingDecelerationWalking = BrakingDecelerationSliding;
		}
		else if (bWasFloorSliding && (MovementMode != MOVE_Falling))
		{
			Velocity *= FloorSlideEndingSpeedFactor;
		}
		bWasFloorSliding = bIsFloorSliding;

		bool bSavedWantsToCrouch = bWantsToCrouch;
		bWantsToCrouch = bWantsToCrouch || bIsFloorSliding;
		bForceMaxAccel = bIsFloorSliding;
		/*
		FVector Loc = CharacterOwner->GetActorLocation();
		if (CharacterOwner->Role < ROLE_Authority)
		{
		UE_LOG(UTNet, Warning, TEXT("CLIENT MOVE at %f deltatime %f from %f %f %f vel %f %f %f accel %f %f %f wants to crouch %d sliding %d dodgelanding %d pressed slide %d"), GetCurrentSynchTime(), DeltaSeconds, Loc.X, Loc.Y, Loc.Z, Velocity.X, Velocity.Y, Velocity.Z, Acceleration.X, Acceleration.Y, Acceleration.Z, bWantsToCrouch, bIsFloorSliding, bIsDodgeLanding, bPressedSlide);
		}
		else
		{
		UE_LOG(UTNet, Warning, TEXT("SERVER Move at %f deltatime %f from %f %f %f vel %f %f %f accel %f %f %f wants to crouch %d sliding %d dodgelanding %d pressed slide %d"), GetCurrentSynchTime(), DeltaSeconds, Loc.X, Loc.Y, Loc.Z, Velocity.X, Velocity.Y, Velocity.Z, Acceleration.X, Acceleration.Y, Acceleration.Z, bWantsToCrouch, bIsFloorSliding, bIsDodgeLanding, bPressedSlide);
		}
		*/
		FVector StartMoveLoc = GetActorLocation();
		Super::PerformMovement(DeltaSeconds);
		UpdateMovementStats(StartMoveLoc);
		bWantsToCrouch = bSavedWantsToCrouch;
		GroundFriction = RealGroundFriction;
		BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	}

	if (UTOwner != NULL)
	{
		UTOwner->PositionUpdated(bShotSpawned);
		bShotSpawned = false;
		// tick movement reduction timer
		UTOwner->WalkMovementReductionTime = FMath::Max(0.f, UTOwner->WalkMovementReductionTime - DeltaSeconds);
		if (UTOwner->WalkMovementReductionTime <= 0.0f)
		{
			UTOwner->WalkMovementReductionPct = 0.0f;
		}
	}
}

void UUTCharacterMovement::UpdateMovementStats(const FVector& StartLocation)
{
	if (CharacterOwner && CharacterOwner->Role == ROLE_Authority)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(CharacterOwner->PlayerState);
		if (PS)
		{
			float Dist = (GetActorLocation() - StartLocation).Size();
			FName MovementName = NAME_RunDist;
			if (MovementMode == MOVE_Falling)
			{
				AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
				MovementName = (UTCharOwner && UTCharOwner->bApplyWallSlide) ? NAME_WallRunDist : NAME_InAirDist;
			}
			else if (MovementMode == MOVE_Swimming)
			{
				MovementName = NAME_SwimDist;
			}
			else if (bIsFloorSliding)
			{
				MovementName = NAME_SlideDist;
			}
			PS->ModifyStatsValue(MovementName, Dist);
		}
	}
}

float UUTCharacterMovement::GetMaxAcceleration() const
{
	float Result;
	if (bIsFloorSliding)
	{
		Result = FloorSlideAcceleration;
	}
	else if (bIsDodgeLanding)
	{
		Result = DodgeLandingAcceleration;
	}
	else if (MovementMode == MOVE_Falling)
	{
		Result = MaxFallingAcceleration;
	}
	else if (MovementMode == MOVE_Swimming)
	{
		Result = MaxSwimmingAcceleration + MaxRelativeSwimmingAccelNumerator / (Velocity.Size() + MaxRelativeSwimmingAccelDenominator);
	}
	else
	{
		Result = Super::GetMaxAcceleration();
		if (Velocity.SizeSquared() < MaxFastAccelSpeed*MaxFastAccelSpeed)
		{
			//extra accel to start, smooth to avoid synch issues
			const float CurrentSpeed = Velocity.Size();
			const float Transition = FMath::Min(1.f, CurrentSpeed/ MaxFastAccelSpeed);
			Result = Result*Transition + FastInitialAcceleration*(1.f - Transition);
		}
	}
	if (MovementMode == MOVE_Walking && Cast<AUTCharacter>(CharacterOwner) != NULL)
	{
		Result *= (1.0f - ((AUTCharacter*)CharacterOwner)->GetWalkMovementReductionPct());
	}
	return Result;
}

float UUTCharacterMovement::GetMaxSpeed() const
{
	float FinalMaxSpeed = 0.0f;

	// ignore standard movement while character is a ragdoll
	if (Cast<AUTCharacter>(CharacterOwner) != NULL && ((AUTCharacter*)CharacterOwner)->IsRagdoll())
	{
		// small non-zero number used to avoid divide by zero issues
		FinalMaxSpeed = 0.01f;
	}
	else if (bIsTaunting || IsRootedByWeapon())
	{
		FinalMaxSpeed = 0.01f;
	}
	else if (bIsFloorSliding && (MovementMode == MOVE_Walking))
	{
		// higher max velocity if going down hill
		float CurrentSpeed = Velocity.Size();
		if ((CurrentSpeed > MaxFloorSlideSpeed) && (CurrentFloor.HitResult.ImpactNormal.Z < 1.f))
		{
			float TopSlideSpeed = FMath::Min(MaxInitialFloorSlideSpeed, CurrentSpeed);
			FinalMaxSpeed = FMath::Min(TopSlideSpeed, MaxFloorSlideSpeed + FMath::Max(0.f, (Velocity | CurrentFloor.HitResult.ImpactNormal)));
		}
		else
		{
			FinalMaxSpeed = MaxFloorSlideSpeed;
		}
	}
	else if (bFallingInWater && (MovementMode == MOVE_Falling))
	{
		FinalMaxSpeed = MaxWaterSpeed;
	}
	else if (MovementMode == MOVE_Swimming)
	{
		AUTWaterVolume* WaterVolume = Cast<AUTWaterVolume>(GetPhysicsVolume());
		FinalMaxSpeed = WaterVolume ? FMath::Min(MaxSwimSpeed, WaterVolume->MaxRelativeSwimSpeed) : MaxSwimSpeed;
	}
	else
	{
		FinalMaxSpeed = Super::GetMaxSpeed();
	}

	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner)
	{
		return FinalMaxSpeed * UTCharOwner->MaxSpeedPctModifier;
	}

	return FinalMaxSpeed;
}

void UUTCharacterMovement::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	AUTCharacter* UTOwner = Cast<AUTCharacter>(CharacterOwner);
	if (MovementMode == MOVE_Walking)
	{
		if (UTOwner != NULL)
		{
			Friction *= (1.0f - UTOwner->GetWalkMovementReductionPct());
			BrakingDeceleration *= (1.0f - UTOwner->GetWalkMovementReductionPct());
		}
		if (!bIsFloorSliding)
		{
			float MaxSpeed = GetMaxSpeed();
			if (Velocity.SizeSquared() > MaxSpeed*MaxSpeed)
			{
				Velocity = Velocity.GetSafeNormal() * MaxSpeed;
			}
		}
	}
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);

	// force away from stopped teammates
	UPrimitiveComponent* UpdatedPrim = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (UpdatedPrim && UTOwner && ((Acceleration.IsZero() && (MovementMode != MOVE_Falling)) || (Velocity.Size2D() < MaxWalkSpeedCrouched)))
	{
		for (int32 i = 0; i < UpdatedPrim->MoveIgnoreActors.Num(); i++)
		{
			AUTCharacter* OverlappedChar = Cast<AUTCharacter>(UpdatedPrim->MoveIgnoreActors[i]);
			// if overlapping, force away
			if (OverlappedChar)
			{
				FVector Diff = OverlappedChar->GetActorLocation() - UTOwner->GetActorLocation();
				// check for overlap.
				if ((Diff.SizeSquared2D() < 1.21f * FMath::Square(UTOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius() + OverlappedChar->GetCapsuleComponent()->GetUnscaledCapsuleRadius())) 
					&& (FMath::Abs(Diff.Z) < UTOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + OverlappedChar->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
				{
					if (!OverlappedChar->UTCharacterMovement || OverlappedChar->UTCharacterMovement->Acceleration.IsZero() || (OverlappedChar->GetVelocity().Size2D() < MaxWalkSpeedCrouched))
					{
						float VelZ = Velocity.Z;
						FVector DeflectionDir = Velocity.IsNearlyZero() ? -1.f*Diff.GetSafeNormal() : (Velocity ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
						if ((DeflectionDir | Diff) > 0.f)
						{
							DeflectionDir *= -1.f;
						}
						Velocity = FMath::Max(0.f, Velocity.Size() - 0.7f*MaxWalkSpeedCrouched) * Velocity.GetSafeNormal() + 0.7f*MaxWalkSpeedCrouched * DeflectionDir.GetSafeNormal();
						if (MovementMode == MOVE_Falling)
						{
							Velocity.Z = VelZ;
						}
					}
				}
			}
		}
	}

	//UE_LOG(UTNet, Warning, TEXT("At %f DeltaTime %f Velocity is %f %f %f from acceleration %f %f slide %d DODGELANDING %d"), GetCurrentSynchTime(), DeltaTime, Velocity.X, Velocity.Y, Velocity.Z, Acceleration.X, Acceleration.Y, bIsFloorSliding, bIsDodgeLanding);

	// workaround for engine path following code not setting Acceleration correctly
	if (bHasRequestedVelocity && Acceleration.IsZero())
	{
		Acceleration = Velocity.GetSafeNormal();
	}
}

void UUTCharacterMovement::ResetTimers()
{
	DodgeResetTime = 0.f;
	FloorSlideTapTime = 0.f;
	FloorSlideEndTime = 0.f;
	GetWorld()->GetTimerManager().ClearTimer(FloorSlideTapHandle);
}

float UUTCharacterMovement::FallingDamageReduction(float FallingDamage, const FHitResult& Hit)
{
	if (Hit.ImpactNormal.Z < GetWalkableFloorZ())
	{
		// Scale damage based on angle of wall we hit
		return FallingDamage * Hit.ImpactNormal.Z;
	}
	return 0.f;
}

void UUTCharacterMovement::RestrictJump(float RestrictedJumpTime)
{
	bRestrictedJump = true;
	GetWorld()->GetTimerManager().SetTimer(ClearRestrictedJumpHandle, this, &UUTCharacterMovement::ClearRestrictedJump, RestrictedJumpTime, false);
}

void UUTCharacterMovement::ClearRestrictedJump()
{
	bRestrictedJump = false;
	GetWorld()->GetTimerManager().ClearTimer(ClearRestrictedJumpHandle);
}

void UUTCharacterMovement::OnTeleported()
{
	if (!HasValidData())
	{
		return;
	}
	bool bWasFalling = (MovementMode == MOVE_Falling);
	bJustTeleported = true;

	// Find floor at current location
	UpdateFloorFromAdjustment();
	SaveBaseLocation();

	// Validate it. We don't want to pop down to walking mode from very high off the ground, but we'd like to keep walking if possible.
	UPrimitiveComponent* OldBase = CharacterOwner->GetMovementBase();
	UPrimitiveComponent* NewBase = NULL;

	if (OldBase && CurrentFloor.IsWalkableFloor() && CurrentFloor.FloorDist <= MAX_FLOOR_DIST && Velocity.Z <= 0.f)
	{
		// Close enough to land or just keep walking.
		NewBase = CurrentFloor.HitResult.Component.Get();
	}
	else
	{
		CurrentFloor.Clear();
	}

	float SavedVelocityZ = Velocity.Z;

	// If we were walking but no longer have a valid base or floor, start falling.
	SetDefaultMovementMode();
	if ((MovementMode == MOVE_Walking) && (!CurrentFloor.IsWalkableFloor() || (OldBase && !NewBase)))
	{
		// If we were walking but no longer have a valid base or floor, start falling.
		SetMovementMode(MOVE_Falling);
	}

	if (MovementMode != MOVE_Walking)
	{
		Velocity.Z = SavedVelocityZ;
	}

	if (bWasFalling && (MovementMode == MOVE_Walking))
	{
		ProcessLanded(CurrentFloor.HitResult, 0.f, 0);
	}
}

void UUTCharacterMovement::OnLineUp()
{
	if (CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
		if (ClientData)
		{
			ClientData->MeshTranslationOffset.Z = 0.0f;
		}
	}
	else
	{
		//UpdatedComponent->SetRelativeLocationAndRotation(FVector(ForceInitToZero), FRotator(ForceInitToZero));
	}
}

void UUTCharacterMovement::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	bIsAgainstWall = false;
	bFallingInWater = false;
	bCountWallSlides = true;
	bHasPlayedWallHitSound = false;
	if (CharacterOwner)
	{
		bIsFloorSliding = bWantsFloorSlide && !Acceleration.IsNearlyZero() && (Velocity.Size2D() > 0.7f * MaxWalkSpeed);
		if (CharacterOwner->ShouldNotifyLanded(Hit))
		{
			CharacterOwner->Landed(Hit);
		}
		if (bIsFloorSliding)
		{
			PerformFloorSlide(Velocity.GetSafeNormal2D(), Hit.ImpactNormal);
			//UE_LOG(UTNet, Warning, TEXT("FloorSlide within %f"), GetCurrentMovementTime() - FloorSlideTapTime);
			if (bIsDodging)
			{
				DodgeResetTime = FloorSlideEndTime + ((CurrentMultiJumpCount > 0) ? DodgeJumpResetInterval : DodgeResetInterval);
				//UE_LOG(UTNet, Warning, TEXT("Set dodge reset after landing move time %f dodge reset time %f"), GetCurrentMovementTime(), DodgeResetTime);
			}
		}
		else if (bIsDodging)
		{
			Velocity *= ((CurrentMultiJumpCount > 0) ? DodgeJumpLandingSpeedFactor : DodgeLandingSpeedFactor);
			DodgeResetTime = GetCurrentMovementTime() + ((CurrentMultiJumpCount > 0) ? DodgeJumpResetInterval : DodgeResetInterval);
			bIsDodgeLanding = true;
		}
		if (bIsFloorSliding || bIsDodgeLanding)
		{
			Acceleration = Acceleration.GetClampedToMaxSize(GetMaxAcceleration());
		}
		bIsDodging = false;
	}
	bJumpAssisted = false;
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner)
	{
		UTCharOwner->bApplyWallSlide = false;
		if (UTCharOwner->Role != ROLE_Authority)
		{
			UTCharOwner->bRepFloorSliding = bIsFloorSliding;
		}
	}
	bExplicitJump = false;
	ClearRestrictedJump();
	CurrentMultiJumpCount = 0;
	CurrentWallDodgeCount = 0;
	if (IsFalling())
	{
		SetPostLandedPhysics(Hit);
	}

	StartNewPhysics(remainingTime, Iterations);
}

void UUTCharacterMovement::UpdateWallSlide(bool bNewWantsWallSlide)
{
	bWantsWallSlide = bNewWantsWallSlide;
}

void UUTCharacterMovement::UpdateFloorSlide(bool bNewWantsFloorSlide)
{
	if (bNewWantsFloorSlide && !bWantsFloorSlide)
	{
		bSlideFromGround = IsMovingOnGround();
		FloorSlideTapTime = GetCurrentMovementTime();
		GetWorld()->GetTimerManager().ClearTimer(FloorSlideTapHandle);
	}
	else if (!bNewWantsFloorSlide && bWantsFloorSlide && (GetCurrentMovementTime() - FloorSlideTapTime < 0.25f))
	{
		// delay clearing bWantsFloorSlide after quick taps, to allow slightly early taps for landing slides
		GetWorld()->GetTimerManager().SetTimer(FloorSlideTapHandle, this, &UUTCharacterMovement::ClearFloorSlideTap, (bSlideFromGround ? 0.8f : FloorSlideBonusTapInterval), false);
		return;
	}
	bWantsFloorSlide = bNewWantsFloorSlide;
}

void UUTCharacterMovement::ClearFloorSlideTap()
{
	bWantsFloorSlide = false;
}

bool UUTCharacterMovement::WantsFloorSlide()
{ 
	return bWantsFloorSlide; 
}

bool UUTCharacterMovement::WantsWallSlide()
{
	return bWantsWallSlide;
}

bool UUTCharacterMovement::DoJump(bool bReplayingMoves)
{
	float RealJumpZVelocity = JumpZVelocity;
	JumpZVelocity = bPressedSlide ? FloorSlideJumpZ : RealJumpZVelocity;
	bool bResult = false;
	if (CharacterOwner && CharacterOwner->CanJump() && (IsFalling() ? DoMultiJump() : Super::DoJump(bReplayingMoves)))
	{
		if (Cast<AUTCharacter>(CharacterOwner) != NULL)
		{
			((AUTCharacter*)CharacterOwner)->OnJumped();
			((AUTCharacter*)CharacterOwner)->MovementEventUpdated(EME_Jump, Velocity.GetSafeNormal());
			if (!bPressedSlide)
			{
				((AUTCharacter*)CharacterOwner)->InventoryEvent(InventoryEventName::Jump);
			}
		}
		bNotifyApex = true;
		bExplicitJump = true;
		NeedsClientAdjustment(); 
		AUTPlayerState* PS = CharacterOwner ? Cast<AUTPlayerState>(CharacterOwner->PlayerState) : NULL;
		if (PS)
		{
			PS->ModifyStatsValue(NAME_NumJumps, 1);
			if (!GetImpartedMovementBaseVelocity().IsZero())
			{
				PS->ModifyStatsValue(NAME_NumLiftJumps, 1);
			}
		}
		JumpTime = GetCurrentMovementTime();
		bResult = true;
	}
	JumpZVelocity = RealJumpZVelocity;
	return bResult;
}

bool UUTCharacterMovement::DoMultiJump()
{
	if (CharacterOwner)
	{
		Velocity.Z = bIsDodging ? DodgeJumpImpulse : MultiJumpImpulse;
		CurrentMultiJumpCount++;
		if (CharacterOwner->IsA(AUTCharacter::StaticClass()))
		{
			((AUTCharacter*)CharacterOwner)->InventoryEvent(InventoryEventName::MultiJump);
		}
		return true;
	}
	return false;
}

bool UUTCharacterMovement::CanMultiJump()
{
	return ( (MaxMultiJumpCount > 0) && (CurrentMultiJumpCount < MaxMultiJumpCount) && (!bIsDodging || bAllowDodgeMultijumps) && (bIsDodging || bAllowJumpMultijumps) &&
			(bAlwaysAllowFallingMultiJump ? (Velocity.Z < MaxMultiJumpZSpeed) : (FMath::Abs(Velocity.Z) < MaxMultiJumpZSpeed)) && 
			(bIsDoubleJumpAvailableForFlagCarrier || !IsCarryingFlag()) );
}

void UUTCharacterMovement::ClearDodgeInput()
{
	//UE_LOG(UTNet, Warning, TEXT("ClearDodgeInput"));
	bPressedDodgeForward = false;
	bPressedDodgeBack = false;
	bPressedDodgeLeft = false;
	bPressedDodgeRight = false;
	bPressedSlide = false;
}

void UUTCharacterMovement::GetDodgeDirection(FVector& OutDodgeDir, FVector & OutDodgeCross) const
{
	float DodgeDirX = bPressedDodgeForward ? 1.f : (bPressedDodgeBack ? -1.f : 0.f);
	float DodgeDirY = bPressedDodgeLeft ? -1.f : (bPressedDodgeRight ? 1.f : 0.f);
	float DodgeCrossX = (bPressedDodgeLeft || bPressedDodgeRight) ? 1.f : 0.f;
	float DodgeCrossY = (bPressedDodgeForward || bPressedDodgeBack) ? 1.f : 0.f;
	FRotator TurnRot(0.f, CharacterOwner->GetActorRotation().Yaw, 0.f);
	FRotationMatrix TurnRotMatrix = FRotationMatrix(TurnRot);
	FVector X = TurnRotMatrix.GetScaledAxis(EAxis::X);
	FVector Y = TurnRotMatrix.GetScaledAxis(EAxis::Y);
	OutDodgeDir = (DodgeDirX*X + DodgeDirY*Y).GetSafeNormal();
	OutDodgeCross = (DodgeCrossX*X + DodgeCrossY*Y).GetSafeNormal();
}

void UUTCharacterMovement::CheckJumpInput(float DeltaTime)
{
	if (bPressedSlide)
	{
		UpdateFloorSlide(true);
	}
	if (CharacterOwner && CharacterOwner->bPressedJump)
	{
		if ((MovementMode == MOVE_Walking) || (MovementMode == MOVE_Falling))
		{
			DoJump(CharacterOwner->bClientUpdating);
		}
		else if ((MovementMode == MOVE_Swimming) && CanDodge())
		{
			PerformWaterJump();
		}
	}
	else if (bPressedDodgeForward || bPressedDodgeBack || bPressedDodgeLeft || bPressedDodgeRight)
	{
		AUTCharacter* UTCharacterOwner = Cast<AUTCharacter>(CharacterOwner);
		if (UTCharacterOwner)
		{
			FVector DodgeDir, DodgeCross;
			GetDodgeDirection(DodgeDir, DodgeCross);
			UTCharacterOwner->Dodge(DodgeDir, DodgeCross);
		}
	}

	if (CharacterOwner)
	{
		// If server, we already got these flags from the saved move
		if (CharacterOwner->IsLocallyControlled())
		{
			bIsFloorSliding = bIsFloorSliding && (GetCurrentMovementTime() < FloorSlideEndTime);
			bIsDodgeLanding = bIsDodgeLanding && (GetCurrentMovementTime() < DodgeResetTime + DodgeLandingTimeAdjust);

			AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
			if (UTCharOwner && (UTCharOwner->Role != ROLE_Authority))
			{
				UTCharOwner->bRepFloorSliding = bIsFloorSliding;
			}
		}

		if (!bIsFloorSliding && bWasFloorSliding)
		{
			AUTCharacter* UTCharacterOwner = Cast<AUTCharacter>(CharacterOwner);
			if (UTCharacterOwner)
			{
				UTCharacterOwner->UpdateCrouchedEyeHeight();
				UTCharacterOwner->DesiredJumpBob = FVector(0.f);
			}
		}
	}
}

bool UUTCharacterMovement::Is3DMovementMode() const
{
	return (MovementMode == MOVE_Flying) || (MovementMode == MOVE_Swimming);
}

/** @TODO FIXMESTEVE - update super class with this? */
FVector UUTCharacterMovement::ComputeSlideVectorUT(const float DeltaTime, const FVector& InDelta, const float Time, const FVector& Normal, const FHitResult& Hit)
{
	const bool bFalling = IsFalling();
	FVector Delta = InDelta;
	FVector Result = UMovementComponent::ComputeSlideVector(Delta, Time, Normal, Hit);
	
	// prevent boosting up slopes
	if (bFalling && Result.Z > 0.f)
	{
		// @TODO FIXMESTEVE - make this a virtual function in super class so just change this part
		float PawnRadius, PawnHalfHeight;
		CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
		if (Delta.Z < 0.f && (Hit.ImpactNormal.Z < MAX_STEP_SIDE_Z))
		{
			// We were moving downward, but a slide was going to send us upward. We want to aim
			// straight down for the next move to make sure we get the most upward-facing opposing normal.
			Result = FVector(0.f, 0.f, Delta.Z);
		}
		else if (bAllowSlopeDodgeBoost && (((CharacterOwner->GetActorLocation() - Hit.ImpactPoint).Size2D() > 0.93f * PawnRadius) || (Hit.ImpactNormal.Z > 0.2f))) // @TODO FIXMESTEVE tweak magic numbers
		{
			if (Result.Z > Delta.Z*Time)
			{
				Result.Z = FMath::Max(Result.Z * SlopeDodgeScaling, Delta.Z*Time);
			}
		}
		else
		{
			// Don't move any higher than we originally intended.
			const float ZLimit = Delta.Z * Time;
			if (Result.Z > ZLimit && ZLimit > KINDA_SMALL_NUMBER)
			{
				FVector SlideResult = Result;

				// Rescale the entire vector (not just the Z component) otherwise we change the direction and likely head right back into the impact.
				const float UpPercent = ZLimit / Result.Z;
				Result *= UpPercent;

				// Make remaining portion of original result horizontal and parallel to impact normal.
				const FVector RemainderXY = (SlideResult - Result) * FVector(1.f, 1.f, 0.f);
				const FVector NormalXY = Normal.GetSafeNormal2D();
				const FVector Adjust = Super::ComputeSlideVector(RemainderXY, 1.f, NormalXY, Hit);
				Result += Adjust;
			}
		}
	}
	return Result;
}

bool UUTCharacterMovement::CanCrouchInCurrentState() const
{
	return CanEverCrouch() && IsMovingOnGround();
}

void UUTCharacterMovement::CheckWallSlide(FHitResult const& Impact)
{
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner)
	{
		UTCharOwner->bApplyWallSlide = false;
		if (bWantsWallSlide && (Impact.ImpactNormal.Z > -0.1f) && (Velocity.Z < MaxWallRunRiseZ) && (Velocity.Z > MaxWallRunFallZ) && !Acceleration.IsZero() && !UTCharOwner->IsThirdPersonTaunting())
		{
			FVector VelocityAlongWall = Velocity + FMath::Abs(Velocity | Impact.ImpactNormal) * Impact.ImpactNormal;
			UTCharOwner->bApplyWallSlide = (VelocityAlongWall.Size2D() >= MinWallSlideSpeed);
			if (UTCharOwner->bApplyWallSlide && bCountWallSlides)
			{
				bCountWallSlides = false;
				AUTPlayerState* PS = CharacterOwner ? Cast<AUTPlayerState>(CharacterOwner->PlayerState) : NULL;
				if (PS)
				{
					PS->ModifyStatsValue(NAME_NumWallRuns, 1);
				}
			}
		}
	}
}

void UUTCharacterMovement::HandleImpact(FHitResult const& Impact, float TimeSlice, const FVector& MoveDelta)
{
	if (IsFalling())
	{
		CheckWallSlide(Impact);
	}
	AActor* ImpactActor = Impact.GetActor();
	if (ImpactActor && ImpactActor->GetRootComponent() && (ImpactActor->GetRootComponent()->Mobility == EComponentMobility::Movable))
	{
		NeedsClientAdjustment();
	}
	Super::HandleImpact(Impact, TimeSlice, MoveDelta);
}

bool UUTCharacterMovement::CanBaseOnLift(UPrimitiveComponent* LiftPrim, const FVector& LiftMoveDelta)
{
	// If character jumped off this lift and is still going up fast enough, then just push him along
	if (Velocity.Z > 0.f)
	{
		FVector LiftVel = MovementBaseUtility::GetMovementBaseVelocity(LiftPrim, NAME_None);
		if (LiftVel.Z > 0.f)
		{
			FHitResult Hit(1.f);
			FVector MoveDelta(0.f);
			MoveDelta.Z = LiftMoveDelta.Z;
			SafeMoveUpdatedComponent(MoveDelta, CharacterOwner->GetActorRotation(), true, Hit);
			return true;
		}
	}
	const FVector PawnLocation = CharacterOwner->GetActorLocation();
	FFindFloorResult FloorResult;
	FindFloor(PawnLocation, FloorResult, false);
	if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
	{
		if (IsFalling())
		{
			ProcessLanded(FloorResult.HitResult, 0.f, 1);
		}
		else if (IsMovingOnGround())
		{
			AdjustFloorHeight();
			SetBase(FloorResult.HitResult.Component.Get(), FloorResult.HitResult.BoneName);
		}
		else
		{
			return false;
		}
		return (CharacterOwner->GetMovementBase() == LiftPrim);
	}
	return false;
}

float UUTCharacterMovement::GetGravityZ() const
{
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner && UTCharOwner->bApplyWallSlide && (Velocity.Z < 0.f))
	{
		return Super::GetGravityZ() * WallRunGravityScaling * (1.f - FMath::Square(0.5f + 0.5f*WallSlideNormal.Z));
	}
	return Super::GetGravityZ();
}

float UUTCharacterMovement::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit, bool bHandleImpact)
{
	if (Hit.bBlockingHit)
	{
		bSlidingAlongWall = true;
	}
	return Super::SlideAlongSurface(Delta, Time, InNormal, Hit, bHandleImpact);
}

void UUTCharacterMovement::HandleSwimmingWallHit(const FHitResult& Hit, float DeltaTime)
{
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	if (UTCharOwner && !UTCharOwner->HeadIsUnderWater())
	{
		FHitResult UpHit(1.f);
		SafeMoveUpdatedComponent(FVector(0.f, 0.f, OutofWaterZ*DeltaTime) - Hit.Normal * 20.f*DeltaTime, UpdatedComponent->GetComponentQuat(), true, UpHit);
		Velocity.Z = OutofWaterZ;
	}
}

void UUTCharacterMovement::PhysSwimming(float deltaTime, int32 Iterations)
{
	if (Velocity.Size() > MaxWaterSpeed)
	{
		FVector VelDir = Velocity.GetSafeNormal();
		if ((VelDir | Acceleration) > 0.f)
		{
			Acceleration = Acceleration - (VelDir | Acceleration)*VelDir; 
		}
	}
	if (bWantsWallSlide)
	{
		// make sure upwards acceleration if jump key is pressed
		Acceleration.Z = FMath::Max(Acceleration.Z, 100.f);
	}

	ApplyWaterCurrent(deltaTime);
	if (!GetPhysicsVolume()->bWaterVolume && IsSwimming())
	{
		SetMovementMode(MOVE_Falling); //in case script didn't change it (w/ zone change)
	}

	//may have left water - if so, script might have set new physics mode
	if (!IsSwimming())
	{
		// include water current velocity if leave water (different frame of reference)
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	Super::PhysSwimming(deltaTime, Iterations);
}

float UUTCharacterMovement::ComputeAnalogInputModifier() const
{
	return 1.f;
}

bool UUTCharacterMovement::ShouldJumpOutOfWater(FVector& JumpDir)
{
	// only consider velocity, not view dir
	if (Velocity.Z > 0.f)
	{
		JumpDir = Acceleration.GetSafeNormal();
		return true;
	}
	return false;
}

void UUTCharacterMovement::ApplyWaterCurrent(float DeltaTime)
{
	if (!CharacterOwner)
	{
		return;
	}
	AUTWaterVolume* WaterVolume = Cast<AUTWaterVolume>(GetPhysicsVolume());
	if (!WaterVolume && bFallingInWater)
	{
		FVector FootLocation = GetActorLocation() - FVector(0.f, 0.f, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		// check if touching water with current
		TArray<FOverlapResult> Hits;
		static FName NAME_PhysicsVolumeTrace = FName(TEXT("PhysicsVolumeTrace"));
		FComponentQueryParams Params(NAME_PhysicsVolumeTrace, CharacterOwner->GetOwner());
		GetWorld()->OverlapMultiByChannel(Hits, FootLocation, FQuat::Identity, CharacterOwner->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(0.f), Params);

		for (int32 HitIdx = 0; HitIdx < Hits.Num(); HitIdx++)
		{
			const FOverlapResult& Link = Hits[HitIdx];
			AUTWaterVolume* const V = Cast<AUTWaterVolume>(Link.GetActor());
			if (V && (!WaterVolume || (V->Priority > WaterVolume->Priority)))
			{
				WaterVolume = V;
			}
		}
	}
	bFallingInWater = false;
	if (WaterVolume)
	{
		bFallingInWater = true;
		// apply any water current
		// current force is not added to velocity (velocity is relative to current, not limited by it)
		FVector WaterCurrent = WaterVolume ? WaterVolume->GetCurrentFor(CharacterOwner) : FVector(0.f);
		if (!WaterCurrent.IsNearlyZero())
		{
	
			// current force is not added to velocity (velocity is relative to current, not limited by it)
			FVector Adjusted = WaterCurrent*DeltaTime;
			FHitResult Hit(1.f);
			SafeMoveUpdatedComponent(Adjusted, CharacterOwner->GetActorRotation(), true, Hit);
			float remainingTime = DeltaTime * (1.f - Hit.Time);

			if (Hit.Time < 1.f && CharacterOwner)
			{
				//adjust and try again
				HandleImpact(Hit, remainingTime, Adjusted);
				SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
			}
		}
	}
}

/** @TODO FIXMESTEVE - physfalling copied from base version and edited.  At some point should probably add some hooks to base version and use those instead. */
void UUTCharacterMovement::PhysFalling(float deltaTime, int32 Iterations)
{
	ApplyWaterCurrent(deltaTime);
	if (!IsFalling())
	{
		// include water current velocity if leave water (different frame of reference)
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Bound final 2d portion of velocity
	const float Speed2d = Velocity.Size2D();
	const float BoundSpeed = FMath::Max(Speed2d, GetMaxSpeed() * AnalogInputModifier);

	//bound acceleration, falling object has minimal ability to impact acceleration
	FVector FallAcceleration = Acceleration;
	FallAcceleration.Z = 0.f;

	if ((CurrentWallDodgeCount > 0) && (Velocity.Z > 0.f) && ((FallAcceleration | LastWallDodgeNormal) < 0.f) && ((Velocity.GetSafeNormal2D() | LastWallDodgeNormal) < 0.f))
	{
		// don't air control back into wall you just dodged from  
		FallAcceleration = FallAcceleration - (FallAcceleration | LastWallDodgeNormal) * LastWallDodgeNormal;
		FallAcceleration = FallAcceleration.GetSafeNormal();
	}
	bool bSkipLandingAssist = true;
	AUTCharacter* UTCharOwner = Cast<AUTCharacter>(CharacterOwner);
	FHitResult Hit(1.f);
	bIsAgainstWall = false;
	if (!HasRootMotionSources())
	{
		// test for slope to avoid using air control to climb walls 
		float TickAirControl = GetCurrentAirControl();
		bool bCheckWallSlide = false;
		if (UTCharOwner)
		{
			bCheckWallSlide = UTCharOwner->bApplyWallSlide;
			UTCharOwner->bApplyWallSlide = false;
		}
		if (bCheckWallSlide || (TickAirControl > 0.0f && FallAcceleration.SizeSquared() > 0.f))
		{
			const float TestWalkTime = FMath::Max(deltaTime, 0.05f);
			const FVector TestWalk = bCheckWallSlide ? -1.f * WallSlideNormal * MaxSlideWallDist : ((TickAirControl * GetMaxAcceleration() * FallAcceleration.GetSafeNormal() + FVector(0.f, 0.f, GetGravityZ())) * TestWalkTime + Velocity) * TestWalkTime;
			if (!TestWalk.IsZero())
			{
				static const FName FallingTraceParamsTag = FName(TEXT("PhysFalling"));
				FHitResult Result(1.f);
				FCollisionQueryParams CapsuleQuery(FallingTraceParamsTag, false, CharacterOwner);
				FCollisionResponseParams ResponseParam;
				InitCollisionParams(CapsuleQuery, ResponseParam);
				CapsuleQuery.bReturnPhysicalMaterial = true;
				const FVector PawnLocation = CharacterOwner->GetActorLocation();
				const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
				const bool bHit = GetWorld()->SweepSingleByChannel(Result, PawnLocation, PawnLocation + TestWalk, FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
				if (bHit)
				{
					bSkipLandingAssist = false;
					// Only matters if we can't walk there
					if (!IsValidLandingSpot(Result.Location, Result))
					{
						// We are against the wall, store info about it
						bIsAgainstWall = true;
						bSlidingAlongWall = true;
						WallSlideNormal = Result.Normal;
						WallRunMaterial = Result.PhysMaterial.Get();
						CheckWallSlide(Result);
						if (UTCharOwner && UTCharOwner->bApplyWallSlide)
						{
							TickAirControl = GetCurrentAirControl();
							FallAcceleration = FallAcceleration - FMath::Max(0.f, (FallAcceleration | WallSlideNormal)) * WallSlideNormal - Result.Time * FallAcceleration.Size() * WallSlideNormal;
						}
						else if (WallSlideNormal.Z > 0.f)
						{
							TickAirControl = 0.f;
						}
					}
				}
			}
		}

		// Boost maxAccel to increase player's control when falling straight down
		if ((Speed2d < 25.f) && (TickAirControl > 0.f)) //allow initial burst
		{
			TickAirControl = FMath::Min(1.f, 2.f*TickAirControl);
		}

		float MaxAccel = GetMaxAcceleration() * TickAirControl;				

		FallAcceleration = FallAcceleration.GetClampedToMaxSize(MaxAccel);

		if (!bHasPlayedWallHitSound && UTCharOwner && UTCharOwner->IsLocallyControlled() && Cast<AUTPlayerController>(UTCharOwner->GetController()) && (GetCurrentMovementTime() > FMath::Max(DodgeResetTime - 0.05f, JumpTime+0.2f)) && (CurrentWallDodgeCount < MaxWallDodges))
		{
			bool bIsNearWall = false;
			FHitResult Result;
			if (!bIsAgainstWall)
			{
				// see if close enough to wall to wall dodge
				FRotator TurnRot(0.f, CharacterOwner->GetActorRotation().Yaw, 0.f);
				FRotationMatrix TurnRotMatrix = FRotationMatrix(TurnRot);
				FVector X = TurnRotMatrix.GetScaledAxis(EAxis::X);
				FVector Y = TurnRotMatrix.GetScaledAxis(EAxis::Y);

				//check ability to dodge left
				FVector DodgeDir = -1.f*Y;
				bIsNearWall = CanWallDodge(DodgeDir, X, Result, false);
				if (!bIsNearWall)
				{
					// moving left, check ability to dodge right
					bIsNearWall = CanWallDodge(Y, X, Result, false);
				}
				if (!bIsNearWall)
				{
					if ((Velocity | X) >= 0.f)
					{
						// moving forward, check able to dodge back
						DodgeDir = -1.f*X;
						bIsNearWall = CanWallDodge(DodgeDir, Y, Result, false);
					}
					else
					{
						// moving backward, check able to dodge back
						bIsNearWall = CanWallDodge(X, Y, Result, false);
					}
				}
			}
			if (bIsAgainstWall || bIsNearWall)
			{
				FVector WallNormal = bIsNearWall ? Result.ImpactNormal : WallSlideNormal;
				//UE_LOG(UT, Warning, TEXT("NEar wall speed %f DOT impact %f Zprod %f"), Velocity.Size2D(), (Velocity | WallNormal), Velocity.Z*WallNormal.Z);
				if ((Velocity | Result.ImpactNormal) < -0.1f*(bIsDodging ? DodgeMaxHorizontalVelocity : MaxWalkSpeed))
				{
					UUTGameplayStatics::UTPlaySound(GetWorld(), UTCharOwner->CharacterData.GetDefaultObject()->WallHitSound, UTCharOwner, SRT_None);
					bHasPlayedWallHitSound = true;
				}
			}
		}
	}
	
	float remainingTime = deltaTime;
	float timeTick = 0.1f;
	/*
	FVector Loc = CharacterOwner->GetActorLocation();
	if (CharacterOwner->Role < ROLE_Authority)
	{
		UE_LOG(UTNet, Warning, TEXT("CLIENT Fall at %f from %f %f %f vel %f %f %f delta %f"), GetCurrentSynchTime(), Loc.X, Loc.Y, Loc.Z, Velocity.X, Velocity.Y, Velocity.Z, deltaTime);
	}
	else
	{
		UE_LOG(UTNet, Warning, TEXT("SERVER Fall at %f from %f %f %f vel %f %f %f delta %f"), GetCurrentSynchTime(), Loc.X, Loc.Y, Loc.Z, Velocity.X, Velocity.Y, Velocity.Z, deltaTime);
	}
	*/
	while ((remainingTime > 0.f) && (Iterations < 8))
	{
		Iterations++;
		timeTick = (remainingTime > 0.05f)
			? FMath::Min(0.05f, remainingTime * 0.5f)
			: remainingTime;

		remainingTime -= timeTick;
		const FVector OldLocation = CharacterOwner->GetActorLocation();
		const FRotator PawnRotation = CharacterOwner->GetActorRotation();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;

		// Apply input
		if (!HasRootMotionSources())
		{
			// Acceleration = FallAcceleration for CalcVelocity, but we restore it after using it.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
			const float SavedVelZ = Velocity.Z;
			Velocity.Z = 0.f;
			CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
			Velocity.Z = SavedVelZ;
		}

		// Apply gravity
		Velocity = NewFallVelocity(Velocity, FVector(0.f, 0.f, GetGravityZ()), timeTick);

		if (bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f))
		{
			// Just passed jump apex since now going down
			bNotifyApex = false;
			NotifyJumpApex();
		}

		if (!HasRootMotionSources())
		{
			// make sure not exceeding acceptable speed
			Velocity = Velocity.GetClampedToMaxSize2D(BoundSpeed);
		}

		FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		if (IsSwimming()) //just entered water
		{
			remainingTime = remainingTime + timeTick * (1.f - Hit.Time);
			if (UTCharOwner)
			{
				UTCharOwner->bApplyWallSlide = false;
			}
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}
		else if (Hit.Time < 1.f)
		{
			//UE_LOG(UT, Warning, TEXT("HIT WALL %f"), Hit.Time);
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += timeTick * (1.f - Hit.Time);
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
				if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
				{
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					FFindFloorResult FloorResult;
					FindFloor(PawnLocation, FloorResult, false);
					if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
					{
						remainingTime += timeTick * (1.f - Hit.Time);
						ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
						return;
					}
				}
				if (!bSkipLandingAssist)
				{
					FindValidLandingSpot(UpdatedComponent->GetComponentLocation());
				}
				HandleImpact(Hit, deltaTime, Adjusted);

				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsFalling())
				{
					if (UTCharOwner)
					{
						UTCharOwner->bApplyWallSlide = false;
					}
					return;
				}

				const float FirstHitPercent = Hit.Time;
				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVectorUT(timeTick * (1.f - Hit.Time), Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				if ((Delta | Adjusted) > 0.f)
				{
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
					if (Hit.Time < 1.f) //hit second wall
					{
						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, timeTick * (1.f - FirstHitPercent), Delta);

						// If we've changed physics mode, abort.
						if (!HasValidData() || !IsFalling())
						{
							if (UTCharOwner)
							{
								UTCharOwner->bApplyWallSlide = false;
							}
							return;
						}

						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
						if (Hit.Time == 0.f)
						{
							// if we are stuck then try to side step
							FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
							if (SideDelta.IsNearlyZero())
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
							}
							SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
						}

						if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0)
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = CharacterOwner->GetActorLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}

				// Calculate average velocity based on actual movement after considering collisions
				if (!bJustTeleported)
				{
					// Use average velocity for XY movement (no acceleration except for air control in those axes), but want actual velocity in Z axis
					const float OldVelZ = OldVelocity.Z;
					OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
					OldVelocity.Z = OldVelZ;
				}
			}
		}

		if (!HasRootMotionSources() && !bJustTeleported && MovementMode != MOVE_None)
		{
			// refine the velocity by figuring out the average actual velocity over the tick, and then the final velocity.
			// This particularly corrects for situations where level geometry affected the fall.
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick; //actual average velocity
			if ((Velocity.Z < OldVelocity.Z) || (OldVelocity.Z >= 0.f))
			{
				Velocity = 2.f*Velocity - OldVelocity; //end velocity has 2* accel of avg
			}

			if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
			{
				Velocity.X = 0.f;
				Velocity.Y = 0.f;
			}

			Velocity = Velocity.GetClampedToMaxSize(GetPhysicsVolume()->TerminalVelocity);
		}
		//UE_LOG(UT, Warning, TEXT("FINAL VELOCITY at %f vel %f %f %f"), GetCurrentSynchTime(), Velocity.X, Velocity.Y, Velocity.Z);
		if (bDrawJumps)
		{
			DrawDebugLine(GetWorld(), OldLocation, CharacterOwner->GetActorLocation(), FColor::Green, true);// , 0.f, 0, 8.f);
		}
	}
}

float UUTCharacterMovement::GetCurrentAirControl()
{
	float Result = bIsDodging ? DodgeAirControl : AirControl;
	Result = (CurrentMultiJumpCount < 1) ? Result : MultiJumpAirControl;
	if (bRestrictedJump)
	{
		Result = 0.0f;
	}
	return Result;
}

void UUTCharacterMovement::NotifyJumpApex()
{
	if (Cast<AUTCharacter>(CharacterOwner))
	{
		Cast<AUTCharacter>(CharacterOwner)->NotifyJumpApex();
	}
	FindValidLandingSpot(UpdatedComponent->GetComponentLocation());
	Super::NotifyJumpApex();
}

void UUTCharacterMovement::FindValidLandingSpot(const FVector& CapsuleLocation)
{
	// Only try jump assist once, and not while still going up, and not if falling too fast
	if (bJumpAssisted || (Velocity.Z > 0.f) || (Cast<AUTCharacter>(CharacterOwner) != NULL && Velocity.Z < -1.f*((AUTCharacter*)CharacterOwner)->MaxSafeFallSpeed))
	{
		return;
	}
	bJumpAssisted = true;

	// See if stepping up/forward in acceleration direction would result in valid landing
	FHitResult Result(1.f);
	static const FName LandAssistTraceParamsTag = FName(TEXT("LandAssist"));
	FCollisionQueryParams CapsuleQuery(LandAssistTraceParamsTag, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleQuery, ResponseParam);
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	bool bHit = GetWorld()->SweepSingleByChannel(Result, PawnLocation, PawnLocation + FVector(0.f, 0.f, LandingStepUp), FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
	FVector HorizontalStart = bHit ? Result.Location : PawnLocation + FVector(0.f, 0.f, LandingStepUp);

	FVector HorizontalDir = Acceleration.GetSafeNormal2D() * MaxWalkSpeed * 0.05f;
	bHit = GetWorld()->SweepSingleByChannel(Result, HorizontalStart, HorizontalStart + HorizontalDir, FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
	bool bHorizontaltHit = bHit;
	FVector HorizontalNormal = bHorizontaltHit ? Result.ImpactNormal : FVector(0.f);
	FVector LandingStart = bHit ? Result.Location : HorizontalStart + HorizontalDir;
	bHit = GetWorld()->SweepSingleByChannel(Result, LandingStart, LandingStart - FVector(0.f, 0.f, LandingStepUp), FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
	bool bAlterVelocity = false;
	if (bHorizontaltHit && !IsValidLandingSpot(Result.Location, Result))
	{
		// second try along first hit wall
		HorizontalDir = HorizontalDir - (HorizontalDir | HorizontalNormal) * HorizontalNormal;
		bHit = GetWorld()->SweepSingleByChannel(Result, HorizontalStart, HorizontalStart + HorizontalDir, FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
		LandingStart = bHit ? Result.Location : HorizontalStart + HorizontalDir;
		bHit = GetWorld()->SweepSingleByChannel(Result, LandingStart, LandingStart - FVector(0.f, 0.f, LandingStepUp), FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
		bAlterVelocity = true;
	}
	if (IsValidLandingSpot(Result.Location, Result))
	{
		// Found a valid landing spot, so boost the player up onto it.
		bJustTeleported = true;
		if (Cast<AUTCharacter>(CharacterOwner))
		{
			Cast<AUTCharacter>(CharacterOwner)->OnLandingAssist();
		}
		if (bAlterVelocity)
		{
			Velocity = HorizontalDir.GetSafeNormal() * Velocity.Size();
		}
		Velocity.Z = LandingAssistBoost; 
	}
}

void UUTCharacterMovement::GetSimpleFloorInfo(FVector& ImpactPoint, FVector& Normal) const
{
	ImpactPoint = CurrentFloor.HitResult.ImpactPoint;
	Normal = CurrentFloor.HitResult.Normal;
}