// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCharacterMovement.h"
#include "UTBot.h"
#include "UTAIAction.h"
#include "UTAIAction_WaitForMove.h"
#include "UTAIAction_WaitForLanding.h"
#include "UTAIAction_TacticalMove.h"
#include "UTAIAction_RangedAttack.h"
#include "UTAIAction_Charge.h"
#include "UTDroppedPickup.h"
#include "UTPickupInventory.h"
#include "UTPickupHealth.h"
#include "UTSquadAI.h"
#include "UTReachSpec_HighJump.h"
#include "UTAvoidMarker.h"
#include "UTBotCharacter.h"
#include "UTDefensePoint.h"
#include "UTAIAction_Camp.h"
#include "UTLift.h"
#include "UTFlagRunGame.h"
#include "UTArmor.h"
#include "UTGameState.h"
#include "UTLineUpHelper.h"

void FBotEnemyInfo::Update(EAIEnemyUpdateType UpdateType, const FVector& ViewerLoc)
{
	if (Pawn != NULL)
	{
		// if we haven't received full info on this enemy in a while, assume our health estimate is off
		if (Pawn->GetWorld()->TimeSeconds - LastFullUpdateTime > 10.0f)
		{
			bHasExactHealth = false;
		}

		LastUpdateTime = Pawn->GetWorld()->TimeSeconds;
		switch (UpdateType)
		{
		case EUT_Seen:
			LastSeenLoc = Pawn->GetActorLocation();
			LastSeeingLoc = ViewerLoc;
			LastKnownLoc = LastSeenLoc;
			LastSeenTime = Pawn->GetWorld()->TimeSeconds;
			LastFullUpdateTime = Pawn->GetWorld()->TimeSeconds;
			bLostEnemy = false;
			if (!bHasExactHealth && UTChar != NULL)
			{
				EffectiveHealthPct = UTChar->GetEffectiveHealthPct(true);
			}
			break;
		case EUT_HeardExact:
			LastKnownLoc = Pawn->GetActorLocation();
			LastFullUpdateTime = Pawn->GetWorld()->TimeSeconds;
			bLostEnemy = false;
			break;
		case EUT_HeardApprox:
			// TODO: set a "general area" sphere?
			if (LastKnownLoc.IsZero())
			{
				// temp so there's some valid location
				LastKnownLoc = Pawn->GetActorLocation();
			}
			break;
		case EUT_TookDamage:
			LastHitByTime = Pawn->GetWorld()->TimeSeconds;
			// TODO: only update LastKnownLoc/LastFullUpdateTime if recently fired the projectile?
			LastKnownLoc = Pawn->GetActorLocation();
			LastFullUpdateTime = Pawn->GetWorld()->TimeSeconds;
			bLostEnemy = false;
			break;
		case EUT_DealtDamage:
			// TODO: only update LastKnownLoc if recently fired the projectile?
			LastKnownLoc = Pawn->GetActorLocation();
			LastFullUpdateTime = Pawn->GetWorld()->TimeSeconds;
			bLostEnemy = false;
			if (UTChar != NULL)
			{
				EffectiveHealthPct = UTChar->GetEffectiveHealthPct(false);
				bHasExactHealth = true;
			}
			break;
		case EUT_HealthUpdate:
			LastKnownLoc = Pawn->GetActorLocation();
			LastFullUpdateTime = Pawn->GetWorld()->TimeSeconds;
			bLostEnemy = false;
			if (UTChar != NULL && bHasExactHealth)
			{
				EffectiveHealthPct = UTChar->GetEffectiveHealthPct(false);
			}
			break;
		}
	}
}
bool FBotEnemyInfo::StaticIsValidInternal(APawn* InPawn, AUTCharacter* InUTChar, AActor* TeamHolder)
{
	if (InPawn == NULL || InPawn->bTearOff || InPawn->IsPendingKillPending() || (InUTChar != NULL && InUTChar->IsDead()))
	{
		return false;
	}
	else if (TeamHolder == NULL)
	{
		return true;
	}
	else if (Cast<AUTBot>(TeamHolder) != NULL)
	{
		return !((AUTBot*)TeamHolder)->IsTeammate(InPawn);
	}
	else
	{
		AUTGameState* GS = TeamHolder->GetWorld()->GetGameState<AUTGameState>();
		return (GS == NULL || !GS->OnSameTeam(TeamHolder, InPawn));
	}
}

AUTBot::AUTBot(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	TacticalHeightAdvantage = 650.0f;

	bWantsPlayerState = true;
	SightRadius = 20000.0f;
	RotationRate = FRotator(300.0f, 300.0f, 0.0f);
	PeripheralVision = 0.7f;
	TrackingReactionTime = 0.22f;
	TrackingInterpTime = 0.2f;
	MaxTrackingPredictionError = 0.2f;
	MaxTrackingOffsetError = 0.15f;
	TrackingErrorUpdateInterval = 0.4f;
	TrackingErrorUpdateTime = 0.f;
	LastIterativeLeadCheck = 1.0f;
	TacticalAimUpdateInterval = 0.2f;
	StoppedOffsetErrorReduction = 0.8f;
	BothStoppedOffsetErrorReduction = 0.6f;
	UsingSquadRouteIndex = INDEX_NONE;
	HuntInterceptFailedTime = -100000.0f;
	DirectionChangeOffsetPct = 0.5f;

	LastDeathTime = -10000.0f;
	LastUnderFireTime = -10000.0f;
	LastTranslocTime = -10000.0f;
	LastRespawnTime = -10000.0f;

	WaitForMoveAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_WaitForMove>(this, FName(TEXT("WaitForMove")));
	WaitForLandingAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_WaitForLanding>(this, FName(TEXT("WaitForLanding")));
	TacticalMoveAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_TacticalMove>(this, FName(TEXT("TacticalMove")));
	RangedAttackAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_RangedAttack>(this, FName(TEXT("RangedAttack")));
	ChargeAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_Charge>(this, FName(TEXT("Charge")));
	CampAction = ObjectInitializer.CreateDefaultSubobject<UUTAIAction_Camp>(this, FName(TEXT("Camp")));
}

void AUTBot::InitializeCharacter(UUTBotCharacter* NewCharacterData)
{
	CharacterData = NewCharacterData;
	Personality = CharacterData->Personality;

	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	if (PS != NULL)
	{
		PS->SetCharacterVoice(CharacterData->CharacterVoice.ToString());
		PS->SetCharacter(CharacterData->Character.ToString());
		PS->SetPlayerName(CharacterData->GetName());
		PS->PlayerCard = NewCharacterData;
		PS->OnPlayerCardUpdated();
		if (!CharacterData->HatType.ToString().IsEmpty())
		{
			PS->ServerReceiveHatClass(CharacterData->HatType.ToString());
			PS->ServerReceiveHatVariant(CharacterData->HatVariantId);
		}
		if (!CharacterData->EyewearType.ToString().IsEmpty())
		{
			PS->ServerReceiveEyewearClass(CharacterData->EyewearType.ToString());
			PS->ServerReceiveEyewearVariant(CharacterData->EyewearVariantId);
		}
		if (!PS->FavoriteWeapon && (Personality.FavoriteWeapon != NAME_None))
		{
			for (FActorIterator It(GetWorld()); It; ++It)
			{
				AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
				if (Pickup && Pickup->WeaponType)
				{
					if (IsFavoriteWeapon(Pickup->WeaponType))
					{
						PS->FavoriteWeapon = Pickup->WeaponType;
						break;
					}
				}
			}
		}
	}
}

float FBestInventoryEval::Eval(APawn* Asker, const FNavAgentProperties& AgentProps, AController* RequestOwner, const UUTPathNode* Node, const FVector& EntryLoc, int32 TotalDistance)
{
	float BestNodeWeight = 0.0f;
	for (TWeakObjectPtr<AActor> TestActor : Node->POIs)
	{
		if (TestActor.IsValid())
		{
			AUTPickup* TestPickup = Cast<AUTPickup>(TestActor.Get());
			if (TestPickup != NULL)
			{
				const float PickupDist = FMath::Max<float>(1.0f, float(TotalDistance) + (TestPickup->GetActorLocation() - EntryLoc).Size());
				const float RespawnOffset = TestPickup->GetRespawnTimeOffset(Asker);
				// if short distance and active, allow regardless of prediction time to make sure bots don't look excessively stupid walking by a pickup right in front of them
				bool bConsiderActive = (TestPickup->State.bActive && (TotalDistance == 0 || PickupDist < 2048.0f));
				if (!bConsiderActive)
				{
					// check respawn awareness versus time remaining
					if (RespawnOffset <= 0.0f)
					{
						bConsiderActive = (RespawnPredictionTime > RespawnOffset);
					}
					// if previously picked this powerup, keep on it unless it respawned and got taken already
					// this accounts for the bot reaching the powerup faster than expected (advanced movement, etc) resulting in the movement speed check below becoming false
					// which otherwise would result in the bot getting confused and backing away from the powerup before heading back to it
					else if (PrevGoal == TestPickup)
					{
						bConsiderActive = (RespawnPredictionTime > RespawnOffset && RespawnOffset < TestPickup->RespawnTime * 0.75f);
					}
					else
					{
						bConsiderActive = (FMath::Min<float>(PickupDist / MoveSpeed + 1.0f, RespawnPredictionTime) > RespawnOffset);
					}
				}
				if (bConsiderActive)
				{
					float NewWeight = TestPickup->BotDesireability(Asker, RequestOwner, PickupDist);
					if (AllowPickup(Asker, RequestOwner, TestPickup, NewWeight, PickupDist))
					{
						NewWeight /= PickupDist;
						if (NewWeight > BestNodeWeight)
						{
							BestNodeWeight = NewWeight;
							if (NewWeight > BestWeight)
							{
								BestPickup = TestPickup;
								BestWeight = NewWeight;
							}
						}
					}
				}
			}
			else
			{
				AUTDroppedPickup* TestDrop = Cast<AUTDroppedPickup>(TestActor.Get());
				if (TestDrop != NULL)
				{
					const float PickupDist = FMath::Max<float>(1.0f, float(TotalDistance) + (TestDrop->GetActorLocation() - EntryLoc).Size());
					float NewWeight = TestDrop->BotDesireability(Asker, RequestOwner, TotalDistance);
					if (AllowPickup(Asker, RequestOwner, TestDrop, NewWeight, PickupDist))
					{
						NewWeight /= PickupDist;
						if (NewWeight > BestNodeWeight)
						{
							BestNodeWeight = NewWeight;
							if (NewWeight > BestWeight)
							{
								BestPickup = TestDrop;
								BestWeight = NewWeight;
							}
						}
					}
				}
			}
		}
	}
	return BestNodeWeight;
}
bool FBestInventoryEval::GetRouteGoal(AActor*& OutGoal, FVector& OutGoalLoc) const
{
	if (BestPickup != NULL)
	{
		OutGoal = BestPickup;
		OutGoalLoc = OutGoal->GetActorLocation();
		return true;
	}
	else
	{
		return false;
	}
}
float FHideLocEval::Eval(APawn* Asker, const FNavAgentProperties& AgentProps, AController* RequestOwner, const UUTPathNode* Node, const FVector& EntryLoc, int32 TotalDistance)
{
	if (Node->bDestinationOnly || RejectNodes.Contains(Node))
	{
		return 0.0f;
	}
	else
	{
		// make sure point is not in avoid list
		for (const FSphere& TestAvoidLoc : AvoidLocs)
		{
			if ((TestAvoidLoc.Center - EntryLoc).Size() <= TestAvoidLoc.W)
			{
				return 0.0f;
			}
		}
		// make sure point is in attract list
		for (const FSphere& TestAttractLoc : AttractLocs)
		{
			if ((TestAttractLoc.Center - EntryLoc).Size() > TestAttractLoc.W)
			{
				return 0.0f;
			}
		}

		if (bUseLearningData && Node->HideAttempts > 2)
		{
			return Node->AvgHideDuration / 20.0f; // TODO: 20 seconds as ideal hide time picked arbitrarily
		}
		else
		{
			// rate based on number of linked paths and number of polys in this node (implying area size)
			// TODO: calculate actual poly area?
			// TODO: early out?
			return 0.5f / FMath::Max<int32>(1, Node->Paths.Num()) + 0.5f / FMath::Max<int32>(1, Node->Polys.Num());
		}
	}
}

void AUTBot::AutoUpdateSkillFor(AUTGameMode* Game)
{
	if (Game)
	{
		float NewSkill = Game->RedTeamSkill;  //also use for FFA
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		if (PS && PS->Team && (PS->Team->TeamIndex == 1))
		{
			NewSkill = Game->BlueTeamSkill;
		}
		InitializeSkill(NewSkill);
	}
}

void AUTBot::InitializeSkill(float NewBaseSkill)
{
	Skill = FMath::Clamp<float>(NewBaseSkill, 0.0f, 8.0f);
	
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	if (PS)
	{
		PS->TDMRank = 800.f + 100.f*Skill;
		PS->CTFRank = PS->TDMRank;
		PS->DMRank = PS->TDMRank;
		PS->DuelRank = PS->TDMRank;
		PS->ShowdownRank = PS->TDMRank;
		PS->TDMMatchesPlayed = 255;
		PS->DuelMatchesPlayed = 255;
		PS->CTFMatchesPlayed = 255;
		PS->DMMatchesPlayed = 255;
		PS->ShowdownMatchesPlayed = 255;
		PS->FlagRunMatchesPlayed = 255;
		PS->Ping = (90 - 10 * Skill) / 4;
	}

	float AimingSkill = Skill + Personality.Accuracy;

	TrackingReactionTime = GetClass()->GetDefaultObject<AUTBot>()->TrackingReactionTime * 7.0f / (AimingSkill + 2.0f);
	TrackingInterpTime = GetClass()->GetDefaultObject<AUTBot>()->TrackingInterpTime * 7.0f / (AimingSkill + 7.0f);

	// very little prediction error for really high skill bots
	// we still want some offset error because that will sometimes actually cause "correct" aim when combined with TrackingReactionTime
	MaxTrackingPredictionError = GetClass()->GetDefaultObject<AUTBot>()->MaxTrackingPredictionError * 5.0f / (AimingSkill + 2.0f);
	if (AimingSkill > 7.0f)
	{
		MaxTrackingPredictionError *= 0.3f;
	}
	MaxTrackingOffsetError = GetClass()->GetDefaultObject<AUTBot>()->MaxTrackingOffsetError * 6.0f / (AimingSkill + 2.0f);

	TrackingErrorUpdateInterval = GetClass()->GetDefaultObject<AUTBot>()->TrackingErrorUpdateInterval * 12.f / (AimingSkill + 9.f);
	TrackingPredictionError = MaxTrackingPredictionError;
	AdjustedMaxTrackingOffsetError = MaxTrackingOffsetError;

	bLeadTarget = Skill >= 3.0f;
	ResetPerceptionProperties();

	if (Skill + Personality.ReactionTime >= 7.0f)
	{
		RotationRate.Yaw = 604.0f;
	}
	else if (Skill + Personality.ReactionTime >= 4.0f)
	{
		RotationRate.Yaw = 38.5f + 55.0f * (Skill + Personality.ReactionTime);
	}
	else
	{
		RotationRate.Yaw = 123.0f + 30.0f * (Skill + Personality.ReactionTime);
	}
	RotationRate.Pitch = RotationRate.Yaw;
	// TODO: old reduced yaw rotation while acquiring enemy; may not need this with new aiming model
	//AdjustedYaw = (0.75 + 0.05 * ReactionTime) * RotationRate.Yaw;
	//AcquisitionYawRate = AdjustedYaw;

	// TODO: old code meant to make low skill bots have reduced movement speed; probably don't want this, messes with anims and makes bots look weird
	//SetMaxDesiredSpeed();

	TranslocInterval = FMath::Max<float>(0.0f, 5.0f - 1.0f * (Skill + Personality.MovementAbility));
}

void AUTBot::ResetPerceptionProperties()
{
	/*if (GetPawn() != NULL && (Pawn.bStationary || Pawn.Physics == PHYS_Flying))
	{
		bSlowerZAcquire = false;
		Pawn.PeripheralVision = -0.7;
	}
	else
	*/
	{
		if (Skill < 2.0f)
		{
			PeripheralVision = 0.7f;
			bSlowerZAcquire = true;
		}
		else if (Skill > 6.0f)
		{
			bSlowerZAcquire = false;
			PeripheralVision = 0.0f;
		}
		else
		{
			PeripheralVision = 1.0f - 0.16f * Skill;
			bSlowerZAcquire = (Skill < 5.f);
		}

		PeripheralVision = FMath::Min<float>(PeripheralVision - Personality.Alertness * 0.25f, 0.8f);
	}
	HearingRadiusMult = FMath::Clamp<float>((Skill + Personality.Alertness) / 6.5f, 0.0f, 0.9f);
}

void AUTBot::SetPawn(APawn* InPawn)
{
	if (InPawn == NULL)
	{
		StartNewAction(NULL);
	}

	if (GetPawn() != NULL)
	{
		GetPawn()->OnActorHit.RemoveDynamic(this, &AUTBot::NotifyBump);
		if (GetCharacter() != NULL)
		{
			GetCharacter()->OnCharacterMovementUpdated.RemoveDynamic(this, &AUTBot::PostMovementUpdate);
		}
	}

	Super::SetPawn(InPawn);

	UTChar = Cast<AUTCharacter>(GetPawn());

	if (GetPawn() != NULL)
	{
		GetPawn()->OnActorHit.AddDynamic(this, &AUTBot::NotifyBump);
		if (GetCharacter() != NULL)
		{
			GetCharacter()->OnCharacterMovementUpdated.AddDynamic(this, &AUTBot::PostMovementUpdate);
		}
	}

	ResetPerceptionProperties();
}

void AUTBot::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	ClearMoveTarget();
	bPickNewFireMode = true;
	
	// set weapon timer, if not already
	GetWorldTimerManager().SetTimer(CheckWeaponFiringTimerHandle, this, &AUTBot::CheckWeaponFiringTimed, 1.2f - 0.09f * FMath::Min<float>(10.0f, Skill + Personality.ReactionTime), true);

	// init respawn prediction time
	// this is here because we want some randomness (so all bots don't converge as one when their skill is the same)
	// but randomizing while moving around could result in bots oscillating back and forth when pickups are near the threshold
	RespawnPredictionTime = Skill + Personality.MapAwareness - 3.0f;
	if (FMath::Abs<float>(RespawnPredictionTime) > KINDA_SMALL_NUMBER)
	{
		RespawnPredictionTime = copysign((FMath::Abs<float>(RespawnPredictionTime) > 1.0f) ? FMath::Square(RespawnPredictionTime) : 1.0f, RespawnPredictionTime);
	}
	RespawnPredictionTime += -1.0f + 2.0f * FMath::FRand();
}

void AUTBot::PawnPendingDestroy(APawn* InPawn)
{
	LastDeathTime = GetWorld()->TimeSeconds;
	LastUnderFireTime = -10000.0f;
	Enemy = NULL;
	StartNewAction(NULL);
	MoveTarget.Clear();
	bHasTranslocator = false;
	ImpactJumpZ = 0.0f;
	UsingSquadRouteIndex = INDEX_NONE;
	bDisableSquadRoutes = false;
	SquadRouteGoal.Clear();
	if (Squad != NULL && Squad->Team != NULL)
	{
		Squad->Team->ClearPickupClaimFor(InPawn);
	}

	Super::PawnPendingDestroy(InPawn);
}

void AUTBot::Destroyed()
{
	if (UTChar != NULL)
	{
		UTChar->PlayerSuicide();
	}
	// TODO: vehicles
	else
	{
		UnPossess();
	}
	
	SetSquad(NULL);
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::Destroyed();
}

APlayerStart* AUTBot::PickSpawnPoint(const TArray<APlayerStart*> Choices)
{
	APlayerStart* Pick = (Squad != NULL) ? Squad->PickSpawnPointFor(this, Choices) : NULL;
	if ((Pick == NULL) && (Choices.Num() > 0))
	{
		// fallback to pure random
		Pick = Choices[FMath::RandHelper(Choices.Num())];
	}
	return Pick;
}

uint8 AUTBot::GetTeamNum() const
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	return (PS != NULL && PS->Team != NULL) ? PS->Team->TeamIndex : 255;
}

bool AUTBot::FindBestJumpVelocityXY(FVector& JumpVelocity, const FVector& StartLoc, const FVector& TargetLoc, float ZSpeed, float GravityZ, float PawnHeight)
{
	float Determinant = FMath::Square(ZSpeed) - 2.0 * GravityZ * (StartLoc.Z - TargetLoc.Z);
	if (Determinant < 0.0f)
	{
		// try a little lower (might still be viable due to the mantle logic)
		Determinant = FMath::Square(ZSpeed) - 2.0 * GravityZ * (StartLoc.Z - TargetLoc.Z - PawnHeight);
	}
	if (Determinant < 0.0f)
	{
		return false;
	}
	else
	{
		Determinant = FMath::Sqrt(Determinant);
		float Time = FMath::Max<float>((-ZSpeed + Determinant) / GravityZ, (-ZSpeed - Determinant) / GravityZ);
		if (Time > 0.0f)
		{
			JumpVelocity = (TargetLoc - StartLoc) / Time;
			JumpVelocity.Z = 0.0f;
			return true;
		}
		else
		{
			return false;
		}
	}
}

void AUTBot::Tick(float DeltaTime)
{
	if (NavData == NULL)
	{
		NavData = GetUTNavData(GetWorld());
	}
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS->IsMatchIntermission() || (GS->IsLineUpActive()))
	{
		return;
	}
	APawn* MyPawn = GetPawn();
	if (MyPawn == NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		// add some additional delay for low skill bots
		if ((PS == NULL || PS->RespawnTime <= 0.0f) && GetWorld()->TimeSeconds - LastDeathTime > 3.9f - (Skill * 1.3f))
		{
			GetWorld()->GetAuthGameMode()->RestartPlayer(this);
		}
		MyPawn = GetPawn();
	}
	if (MyPawn != NULL && NavData != NULL)
	{
		// all bots need a SquadAI, even if it's a squad of one
		if (Squad == NULL)
		{
			AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if (Game != NULL)
			{
				Game->AssignDefaultSquadFor(this);
			}
			if (Squad == NULL)
			{
				UE_LOG(UT, Warning, TEXT("Bot %s failed to get Squad from game mode!"), (PlayerState != NULL) ? *PlayerState->PlayerName : *GetName());
				// force default so we always have one
				SetSquad(GetWorld()->SpawnActor<AUTSquadAI>());
			}
		}

		if (MoveTarget.IsValid())
		{
			const bool bIsFalling = (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling);
			if (!bIsFalling && NavData->HasReachedTarget(MyPawn, MyPawn->GetNavAgentPropertiesRef(), MoveTarget))
			{
				// reached
				ClearMoveTarget();
			}
			else
			{
				MoveTimer -= DeltaTime;
				if (MoveTimer < 0.0f && !bIsFalling)
				{
					// timed out
					ClearMoveTarget();
				}
				else
				{
					// TODO: if falling, check if we have enough jump velocity to land on further point on path

					// clear points that we've reached or passed
					const FVector MyLoc = MyPawn->GetActorLocation();
					float DistFromTarget = (MoveTarget.GetLocation(MyPawn) - MyLoc).Size();
					const FVector Extent = MyPawn->GetSimpleCollisionCylinderExtent();
					FBox MyBox(0);
					MyBox += MyLoc + Extent;
					MyBox += MyLoc - Extent;
					if (bAdjusting && MyBox.IsInside(AdjustLoc))
					{
						bAdjusting = false;
					}
					if (!bAdjusting)
					{
						bool bRemovedPoints = false;
						for (int32 i = MoveTargetPoints.Num() - 2; i >= 0; i--)
						{
							// if path requires complex movement (jumps, etc) then require touching second to last point
							// since the final part of the path may require more precision
							if (i >= MoveTargetPoints.Num() - 2 && CurrentPath.ReachFlags != 0)
							{
								if (MyBox.ExpandBy(MyBox.GetSize() * FVector(-0.25f, -0.25f, 0.0f)).IsInside(MoveTargetPoints[i].Get()))
								{
									LastReachedMovePoint = MoveTargetPoints[i].Get();
									MoveTargetPoints.RemoveAt(0, i + 1);
									bRemovedPoints = true;
									break;
								}
							}
							else
							{
								if (MyBox.IsInside(MoveTargetPoints[i].Get()))
								{
									LastReachedMovePoint = MoveTargetPoints[i].Get();
									MoveTargetPoints.RemoveAt(0, i + 1);
									bRemovedPoints = true;
									break;
								}
								else if (DistFromTarget < (MoveTarget.GetLocation(MyPawn) - MoveTargetPoints[i].Get()).Size() && !NavData->RaycastWithZCheck(GetNavAgentLocation(), MoveTargetPoints[i + 1].Get() - FVector(0.0f, 0.0f, Extent.Z)))
								{
									LastReachedMovePoint = MoveTargetPoints[i].Get();
									MoveTargetPoints.RemoveAt(0, i + 1);
									bRemovedPoints = true;
									break;
								}
							}
						}
						if (bRemovedPoints)
						{
							// check if translocator target is redundant now
							if (!TranslocTarget.IsZero() && ((MoveTargetPoints[0].Get() - TranslocTarget).Size() > (MoveTargetPoints[0].Get() - MyLoc).Size() || (MoveTargetPoints.Num() == 1 && Cast<UUTReachSpec_HighJump>(CurrentPath.Spec.Get()) != NULL)))
							{
								TranslocTarget = FVector::ZeroVector;
								ClearFocus(SCRIPTEDMOVE_FOCUS_PRIORITY);
							}
							UpdateMovementOptions(false);
						}
					}

					// failure checks
					FVector MovePoint = GetMovePoint();
					if ((MovePoint - MyLoc).Size2D() < MyPawn->GetSimpleCollisionRadius())
					{
						static FName NAME_AIZCheck(TEXT("AIZCheck"));

						float ZDiff = MyLoc.Z - MovePoint.Z;
						bool bZFail = false;
						if (!(CurrentPath.ReachFlags & R_JUMP))
						{
							bZFail = FMath::Abs<float>(ZDiff) > MyPawn->GetSimpleCollisionHalfHeight();
						}
						else if (ZDiff < -MyPawn->GetSimpleCollisionHalfHeight())
						{
							bZFail = true;
						}
						else
						{
							// for jump/fall path make sure we don't just need to get closer to the edge
							FVector TargetPoint = MovePoint;
							bZFail = GetWorld()->LineTraceTestByChannel(FVector(TargetPoint.X, TargetPoint.Y, MyLoc.Z), TargetPoint, ECC_Pawn, FCollisionQueryParams(NAME_AIZCheck, false, MyPawn));
						}
						if (bZFail)
						{
							if (GetCharacter() != NULL)
							{
								// don't fail yet if falling in proper direction, might still reach via jump/gravity
								if (GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Walking || (ZDiff > 0.0f) != (GetCharacter()->GetCharacterMovement()->MovementMode > 0.0f))
								{
									// failed - directly above or below target
									ClearMoveTarget();
								}
							}
							else if (GetWorld()->SweepTestByChannel(MyLoc, MovePoint, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(MyPawn->GetSimpleCollisionCylinderExtent() * FVector(0.9f, 0.9f, 0.1f)), FCollisionQueryParams(NAME_AIZCheck, false, MyPawn)))
							{
								// failed - directly above or below target
								ClearMoveTarget();
							}
						}
					}
				}
			}
		}

		// check for enemy visibility
		// check current enemy every frame, others on a slightly random timer to avoid hitches
		if (Enemy != NULL)
		{
			if (!FBotEnemyInfo::StaticIsValid(Enemy, this))
			{
				// enemy was destroyed or changed team directly instead of killed so we didn't get notify
				SetEnemy(NULL);
			}
			else
			{
				if (CanSee(Enemy, false))
				{
					SeePawn(Enemy);
				}
				else if (CurrentAction != NULL)
				{
					CurrentAction->EnemyNotVisible();
				}
				UpdateTrackingError(false);
			}
		}
		SightCounter -= DeltaTime;
		if (SightCounter < 0.0f)
		{
			for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
			{
				if (It->IsValid())
				{
					AController* C = It->Get();
					if (C != this && C->GetPawn() != NULL && C->GetPawn() != Enemy && (bSeeFriendly || !IsTeammate(C)) && CanSee(C->GetPawn(), true))
					{
						SeePawn(C->GetPawn());
					}
				}
			}
			SightCounter += 0.15f + 0.1f * FMath::SRand();
		}

		// process current action
		{
			UUTAIAction* SavedAction = CurrentAction;
			if (SavedAction == NULL)
			{
				WhatToDoNext();
			}
			else if (SavedAction->Update(DeltaTime))
			{
				if (SavedAction == CurrentAction) // could have ended itself...
				{
					CurrentAction->Ended(false);
					CurrentAction = NULL;
					WhatToDoNext();
				}
				else if (SavedAction == NULL) // could also interrupt directly to another action
				{
					WhatToDoNext();
				}
			}
		}
		// start new action, if requested
		// make sure updates above didn't result in losing Pawn (stop firing -> suicide, etc)
		if (bPendingWhatToDoNext && GetPawn() != NULL)
		{
			bExecutingWhatToDoNext = true;
			ExecuteWhatToDoNext();
			bExecutingWhatToDoNext = false;
			bPendingWhatToDoNext = false;
			if (GetPawn() != NULL)
			{
				if (CurrentAction == NULL)
				{
					UE_LOG(UT, Warning, TEXT("%s (%s) failed to get an action from ExecuteWhatToDoNext()"), *GetName(), *PlayerState->PlayerName);
					GoalString = TEXT("BUG - NO ACTION - Setting CampAction");
					StartNewAction(CampAction);
				}
			
				SetDefaultFocus();
				// switch to best weapon after deciding what we want to do
				// if we have a new movement destination, init that first before deciding (may want to use translocator, etc)
				if (!MoveTarget.IsValid() || MoveTargetPoints.Num() > 0)
				{
					SwitchToBestWeapon();
				}
			}
		}

		if (MoveTarget.IsValid() && GetPawn() != NULL)
		{
			if (MoveTargetPoints.Num() == 0)
			{
				if (MoveTarget.IsDirectTarget())
				{
					// a little hacky... this location isn't actually used as the below movement code always moves to MoveTarget directly when on the last point
					MoveTargetPoints.Add(FComponentBasedPosition(MoveTarget.GetLocation(GetPawn())));
				}
				else
				{
					// TODO: raycast for direct reachability?
					float TotalDistance = 0.0f;
					if (NavData->GetMovePoints(MyPawn->GetNavAgentLocation(), MyPawn, MyPawn->GetNavAgentPropertiesRef(), MoveTarget, RouteCache, MoveTargetPoints, CurrentPath, &TotalDistance))
					{
						UpdateMovementOptions(true);
						MoveTimer = TotalDistance / FMath::Max<float>(100.0f, (MyPawn->GetMovementComponent() != NULL) ? MyPawn->GetMovementComponent()->GetMaxSpeed() : 0.0f) + 1.0f;
						if (Cast<APawn>(MoveTarget.Actor.Get()) != NULL)
						{
							MoveTimer += 2.0f; // TODO: maybe do this for any moving target?
						}
					}
					else
					{
						ClearMoveTarget();
					}
				}
				SwitchToBestWeapon();
			}
			if (MoveTarget.IsValid())
			{
				FVector TargetLoc = GetMovePoint();
				SetFocalPoint(TargetLoc, EAIFocusPriority::Move); // lowest priority we use, only applied when nothing else or too low skill to strafe
				if (GetCharacter() != NULL)
				{
					GetCharacter()->GetCharacterMovement()->bCanWalkOffLedges = (!CurrentPath.IsSet() || (CurrentPath.ReachFlags & R_JUMP) || (CurrentPath.Spec.IsValid() && CurrentPath.Spec->AllowWalkOffLedges(CurrentPath, GetPawn(), GetMoveBasedPosition())))
																				&& (CurrentAction == NULL || CurrentAction->AllowWalkOffLedges());
				}
				if (bFinishRotation)
				{
					const float DesiredYaw = FRotator::ClampAxis((GetFocalPoint() - MyPawn->GetActorLocation()).Rotation().Yaw);
					bFinishRotation = FMath::Abs<float>(FRotator::ClampAxis(ControlRotation.Yaw) - DesiredYaw) > KINDA_SMALL_NUMBER;
				}
				if (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling && GetCharacter()->GetCharacterMovement()->AirControl > 0.0f && GetCharacter()->GetCharacterMovement()->MaxWalkSpeed > 0.0f)
				{
					// note: bRestrictedJump check last so reachspec can react to/override that flag if it knows better
					if ((!CurrentPath.Spec.IsValid() || !CurrentPath.Spec->OverrideAirControl(CurrentPath, GetPawn(), GetMoveBasedPosition(), MoveTarget)) && !bRestrictedJump)
					{
						// figure out desired 2D velocity and set air control to achieve that
						FVector DesiredVel2D;
						if ( FindBestJumpVelocityXY(DesiredVel2D, MyPawn->GetActorLocation(), TargetLoc, GetCharacter()->GetCharacterMovement()->Velocity.Z, GetCharacter()->GetCharacterMovement()->GetGravityZ(), MyPawn->GetSimpleCollisionHalfHeight()) ||
							(UTChar != NULL && UTChar->UTCharacterMovement->CanMultiJump() && FindBestJumpVelocityXY(DesiredVel2D, MyPawn->GetActorLocation(), TargetLoc, UTChar->UTCharacterMovement->MultiJumpImpulse, GetCharacter()->GetCharacterMovement()->GetGravityZ(), MyPawn->GetSimpleCollisionHalfHeight())) )
						{
							FVector MyVel2D = GetCharacter()->GetCharacterMovement()->Velocity;
							MyVel2D.Z = 0.0f;
							float MaxTickSpeedChange = FMath::Max<float>(1.0f, GetCharacter()->GetCharacterMovement()->GetMaxAcceleration() * GetCharacter()->GetCharacterMovement()->AirControl * DeltaTime);
							FVector NewAccel = ((DesiredVel2D - MyVel2D) / MaxTickSpeedChange).GetClampedToMaxSize2D(1.0f);
							NewAccel.Z = 0.0f;
							MyPawn->GetMovementComponent()->AddInputVector(NewAccel);
						}
						else
						{
							// just assume max, get as close as we can and maybe we get lucky
							MyPawn->GetMovementComponent()->AddInputVector((TargetLoc - MyPawn->GetActorLocation()).GetSafeNormal2D());
						}
					}
				}
				// do nothing if path says we need to wait
				else if (bAdjusting || !CurrentPath.Spec.IsValid() || !CurrentPath.Spec->WaitForMove(CurrentPath, GetPawn(), GetMoveBasedPosition(), MoveTarget))
				{
					if (GetCharacter() != NULL && (GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Flying || GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Swimming))
					{
						const FVector Dir = (TargetLoc - MyPawn->GetActorLocation()).GetSafeNormal();
						MyPawn->GetMovementComponent()->AddInputVector(Dir);
						if (Dir.Z > 0.25f && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Swimming && UTChar != NULL)
						{
							UTChar->UTCharacterMovement->PerformWaterJump();
						}
					}
					else if (bFinishRotation)
					{
						MoveTimer += DeltaTime;
					}
					else if (MyPawn->GetMovementComponent() != NULL) // FIXME: remote redeemer doesn't set this, need to control a different way...
					{
						FVector Accel = (TargetLoc - MyPawn->GetActorLocation()).GetSafeNormal2D();
						if (bUseSerpentineMovement && GetCharacter() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Walking)
						{
							const FVector MyLoc = MyPawn->GetActorLocation();
							const FVector PathDir = (TargetLoc - LastReachedMovePoint).GetSafeNormal();
							// amount that Pawn exceeds min agent radius
							// nav building already has a buffer of AgentRadius away from walls so we need not include that amount of radius when checking there is enough space
							const float ExtraRadius = FMath::Max<float>(0.0f, MyPawn->GetSimpleCollisionRadius() - NavData->AgentRadius);
							// try to determine how wide the path is
							// if in doubt, we'll use a default guess that results in reasonable strafing since we're going to do a navmesh trace to make sure it's valid
							float MaxSideDist = MyPawn->GetSimpleCollisionRadius() * 1.5f;
							if (CurrentPath.IsSet())
							{
								float PathSize = float(CurrentPath.CollisionRadius);
								// attempt to find the navmesh edge between the polys we're traversing and intersect our default movement direction with it
								// that gives us the amount of leeway to the side we can move and still make it through
								NavNodeRef SrcPoly = NavData->UTFindNearestPoly(LastReachedMovePoint - FVector(0.0f, 0.0f, MyPawn->GetSimpleCollisionHalfHeight()), MyPawn->GetSimpleCollisionCylinderExtent());
								NavNodeRef DestPoly = NavData->UTFindNearestPoly(TargetLoc - FVector(0.0f, 0.0f, MyPawn->GetSimpleCollisionHalfHeight()), MyPawn->GetSimpleCollisionCylinderExtent());
								TArray<FNavigationPortalEdge> Edges;
								NavData->GetPolyNeighbors(SrcPoly, Edges);
								for (const FNavigationPortalEdge& TestEdge : Edges)
								{
									if (TestEdge.ToRef == DestPoly)
									{
										FVector OutP1, OutP2;
										FMath::SegmentDistToSegmentSafe(TestEdge.Left, TestEdge.Right, LastReachedMovePoint, TargetLoc, OutP1, OutP2);
										PathSize = FMath::Min<float>((OutP1 - TestEdge.Left).Size(), (OutP1 - TestEdge.Right).Size()) - ExtraRadius;
										break;
									}
								}
								MaxSideDist = FMath::Max<float>(MaxSideDist, PathSize);
							}
							const FVector Side = (PathDir ^ FVector(0.0f, 0.0f, 1.0f)) * MaxSideDist * SerpentineDir;
							// push the start point back somewhat for this calculation so we don't get bad results on the start of the path where we might be slightly behind the 'official' start point
							// because reaching it only requires that our capsule touches
							const FVector ClosestPoint = FMath::ClosestPointOnSegment(MyLoc, LastReachedMovePoint + (LastReachedMovePoint - TargetLoc).GetSafeNormal() * MyPawn->GetSimpleCollisionRadius(), TargetLoc);
							float DistFromStrafeGoal = (ClosestPoint - MyLoc).Size2D();
							if ((Side | (MyLoc - ClosestPoint)) < 0.0f)
							{
								DistFromStrafeGoal += MaxSideDist;
							}
							else
							{
								DistFromStrafeGoal = MaxSideDist - DistFromStrafeGoal;
							}
							if (DistFromStrafeGoal < 0.0f)
							{
								// switch directions
								SerpentineDir *= -1.0f;
							}
							else if (DistFromStrafeGoal * 2.0f < (TargetLoc - MyLoc).Size()) // maybe should be Size2D(), but worried about effect when on a slope
							{
								// make sure strafe adjustment stays on the navmesh
								if (!NavData->RaycastWithZCheck(GetPawn()->GetNavAgentLocation(), ClosestPoint + Side.GetSafeNormal() * (Side.Size() + ExtraRadius)))
								{
									Accel = (Accel + Side.GetSafeNormal()).GetSafeNormal();
								}
								else
								{
									// TODO: maybe make up for no strafe by dodging?
									bUseSerpentineMovement = false;
								}
							}
						}
						// adjust direction to avoid active FearSpots
						FVector FearAdjust(FVector::ZeroVector);
						for (int32 i = FearSpots.Num() - 1; i >= 0; i--)
						{
							if (FearSpots[i] == NULL || FearSpots[i]->IsPendingKillPending() || !GetPawn()->IsOverlappingActor(FearSpots[i]))
							{
								FearSpots.RemoveAt(i);
							}
							else
							{
								FearAdjust += (GetPawn()->GetActorLocation() - FearSpots[i]->GetActorLocation()) / FearSpots[i]->GetSimpleCollisionRadius();
							}
						}
						if (!FearAdjust.IsZero())
						{
							FearAdjust.Normalize();
							float FearToDesiredAngle = (FearAdjust | Accel);
							if (FearToDesiredAngle < 0.7f)
							{
								if (FearToDesiredAngle < -0.7f)
								{
									const FVector LeftDir = (Accel ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
									FearAdjust = 2.f * LeftDir;
									if ((LeftDir | FearAdjust) < 0.f)
									{
										FearAdjust *= -1.f;
									}
								}
								Accel = (Accel + FearAdjust).GetSafeNormal();
							}
						}
						MyPawn->GetMovementComponent()->AddInputVector(Accel);
					}
				}
			}
		}
	}

	Super::Tick(DeltaTime);
}

// @TODO FIXMESTEVE tracking offset error should go down at higher skills over time as long as enemy is still visible and tracked
void AUTBot::UpdateTrackingError(bool bNewEnemy)
{
	if (bNewEnemy)
	{
		TrackedVelocity = FVector(0.f);
	}
	else if (bLargeTrackedVelocityChange)
	{
		// possibly increase tracking error if enemy just had sudden direction change
		AdjustedMaxTrackingOffsetError = FMath::Max(AdjustedMaxTrackingOffsetError, DirectionChangeOffsetPct*MaxTrackingOffsetError);
	}
	if (bNewEnemy || GetWorld()->TimeSeconds > TrackingErrorUpdateTime)
	{
		TrackingPredictionError = MaxTrackingPredictionError * (2.f * FMath::FRand() - 1.f);
		if (bNewEnemy)
		{
			// reduce target switch error if enemy was detected prior to target switch
			float KnownEnemyReduction = 0.0f;
			if (Enemy != NULL)
			{
				// TODO: would really like "time enemy has been constantly visible" here, so enemies that stay in bot's view are easier to switch to
				if (IsEnemyVisible(Enemy))
				{
					KnownEnemyReduction += 0.01f;
					if (UTLineOfSightTo(Enemy, FVector::ZeroVector, false, Enemy->GetTargetLocation(GetPawn()) + Enemy->GetVelocity()))
					{
						KnownEnemyReduction += 0.02f;
						if (UTLineOfSightTo(Enemy, FVector::ZeroVector, false, Enemy->GetTargetLocation(GetPawn()) + Enemy->GetVelocity() * 3.0f))
						{
							KnownEnemyReduction += 0.03f;
						}
					}
				}
				else
				{
					const FBotEnemyInfo* Info = GetEnemyInfo(Enemy, true);
					if (GetWorld()->TimeSeconds - Info->LastFullUpdateTime < 0.25f && Info->LastFullUpdateTime > GetEnemyInfo(Enemy, false)->LastFullUpdateTime)
					{
						// teammates told me enemy was here
						KnownEnemyReduction += 0.02f;
					}
				}
			}
			AdjustedMaxTrackingOffsetError = MaxTrackingOffsetError * (1.0f - KnownEnemyReduction * (Skill + Personality.Alertness * 2.0f));
		}
		bool bStoppedEnemy = (Enemy == NULL || (TrackedVelocity.IsNearlyZero() && GetEnemyInfo(Enemy, true)->CanUseExactLocation(GetWorld()->TimeSeconds)));
		bool bAmStopped = (GetPawn() != NULL && GetPawn()->GetVelocity().IsNearlyZero());
		// consider crouching as 'stopped' for aiming purposes
		if (!bAmStopped && GetCharacter() != NULL)
		{
			bAmStopped = (GetCharacter()->GetVelocity().Size() <= GetCharacter()->GetCharacterMovement()->MaxWalkSpeedCrouched);
		}
		if (bStoppedEnemy || bAmStopped)
		{
			AdjustedMaxTrackingOffsetError *= ((bStoppedEnemy && bAmStopped) ? BothStoppedOffsetErrorReduction : StoppedOffsetErrorReduction) * FMath::FRandRange(0.9f, 1.1f);
		}
		if (!bNewEnemy)
		{
			float AcquisitionRate = 0.9f / FMath::Max<float>(1.0f, 0.25f * (Skill - Personality.ReactionTime * 2.0f));
			float LossRate = 1.15f + 0.3f * FMath::Max<float>(0.0f, 1.0f - (Skill / 7.0f)) - Personality.ReactionTime * 0.1f;
			AdjustedMaxTrackingOffsetError *= IsEnemyVisible(Enemy) ? AcquisitionRate : LossRate;
		}
		// don't let tracking offset error get to zero; looks bad and sometimes the error corrects for flaws (intentional or otherwise) in the base aim
		AdjustedMaxTrackingOffsetError = FMath::Clamp<float>(AdjustedMaxTrackingOffsetError, MaxTrackingOffsetError * 0.075f, MaxTrackingOffsetError);
		TrackingOffsetError = AdjustedMaxTrackingOffsetError * (2.f * FMath::FRand() - 1.f);
		TrackingErrorUpdateTime = GetWorld()->GetTimeSeconds() + TrackingErrorUpdateInterval;
	}
}

void AUTBot::SetMoveTarget(const FRouteCacheItem& NewMoveTarget, const TArray<FComponentBasedPosition>& NewMovePoints, const FUTPathLink& NewCurrentPath)
{
	PrevMoveTarget = MoveTarget;
	MoveTarget = NewMoveTarget;
	MoveTargetPoints = NewMovePoints;
	bAdjusting = false;
	CurrentPath = NewCurrentPath;
	if (NavData == NULL || !NavData->GetPolyCenter(NavData->UTFindNearestPoly(GetPawn()->GetNavAgentLocation(), GetPawn()->GetSimpleCollisionCylinderExtent()), LastReachedMovePoint))
	{
		LastReachedMovePoint = GetPawn()->GetActorLocation();
	}
	TranslocTarget = FVector::ZeroVector;
	// default movement code will generate points and set MoveTimer, this just makes sure we don't abort before even getting there
	MoveTimer = FMath::Max<float>(MoveTimer, 1.0f);

	// if previous focus was MoveTarget, update
	if (PrevMoveTarget.Actor.IsValid() && GetFocusActorForPriority(EAIFocusPriority::Gameplay) == PrevMoveTarget.Actor.Get())
	{
		// note: if new target is NULL (a point instead of an Actor) this will fall back to EAIFocusPriority::Move
		SetFocus(NewMoveTarget.Actor.Get(), EAIFocusPriority::Gameplay);
	}
}

void AUTBot::UpdateMovementOptions(bool bNewPath)
{
	bUseSerpentineMovement = false;
	SerpentineDir = (FMath::FRand() < 0.5f) ? 1.0f : -1.0f;
	if (GetEnemy() != NULL && Skill + Personality.MovementAbility > 2.7f + FMath::FRand())
	{
		if (GetFocusActor() == Enemy || IsEnemyVisible(Enemy) || HasOtherVisibleEnemy() || GetWorld()->TimeSeconds - GetEnemyInfo(Enemy, false)->LastSeenTime < 2.0f)
		{
			bUseSerpentineMovement = true;
		}
		// if alert enough, check if any enemy I know about can probably see/shoot me
		else if (Skill + Personality.Alertness > 3.7f + FMath::FRand() || FMath::FRand() < Personality.Alertness)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
			const TArray<const FBotEnemyInfo>& EnemyList = (PS != NULL && PS->Team != NULL) ? PS->Team->GetEnemyList() : *(const TArray<const FBotEnemyInfo>*)&LocalEnemyList;
			for (const FBotEnemyInfo& EnemyEntry : EnemyList)
			{
				if ( EnemyEntry.IsValid() && GetWorld()->TimeSeconds - EnemyEntry.LastFullUpdateTime < 1.0f &&
					(GetWorld()->TimeSeconds - EnemyEntry.LastSeenTime < 2.0f || GetWorld()->LineTraceTestByChannel(EnemyEntry.LastKnownLoc, GetPawn()->GetActorLocation(), ECC_Visibility)) )
				{
					bUseSerpentineMovement = true;
					break;
				}
			}
		}
	}

	ConsiderTranslocation();
	// ConsiderTranslocation() may call StopFiring() -> could result in suicide
	if (GetPawn() == NULL)
	{
		return;
	}

	// if moving evasively, check if should dodge to move target
	if ( bUseSerpentineMovement && Skill >= 3.0f && GetUTChar() != NULL && MoveTarget.IsValid() && (CurrentPath.IsSet() || (Enemy != NULL && MoveTarget.Actor.Get() == Enemy)) && CurrentPath.Spec.Get() == NULL && CurrentPath.ReachFlags == 0 &&
		((MoveTarget.Actor.Get() != NULL && MoveTarget.Actor->GetRootComponent()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap) || (GetMovePoint() - GetPawn()->GetActorLocation()).Size() > GetUTChar()->UTCharacterMovement->DodgeImpulseHorizontal * 0.5f) )
	{
		const FVector MoveDir = (GetMovePoint() - GetPawn()->GetActorLocation()).GetSafeNormal();
		float DodgeChance = Skill * 0.05f + Personality.Jumpiness * 0.5f;
		// more likely to dodge to the side
		float AngleToPath = FMath::Abs<float>(MoveDir | GetPawn()->GetActorRotation().Vector());
		if (AngleToPath < 0.75f && AngleToPath > 0.25f)
		{
			DodgeChance *= 2.0f;
		}
		if (FMath::FRand() < DodgeChance)
		{
			FRotationMatrix RotMat(GetPawn()->GetActorRotation());
			FVector DodgeDirs[] = { RotMat.GetUnitAxis(EAxis::X), -RotMat.GetUnitAxis(EAxis::X), RotMat.GetUnitAxis(EAxis::Y), -RotMat.GetUnitAxis(EAxis::Y) };
			float BestAngle = -1.0f;
			int32 BestIndex = INDEX_NONE;
			for (int32 i = 0; i < ARRAY_COUNT(DodgeDirs); i++)
			{
				float Angle = DodgeDirs[i] | MoveDir;
				if (Angle > BestAngle)
				{
					BestIndex = i;
					BestAngle = Angle;
				}
			}
			checkSlow(BestIndex != INDEX_NONE);
			GetUTChar()->Dodge(DodgeDirs[BestIndex].GetSafeNormal2D(), (DodgeDirs[BestIndex] ^ FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal());
		}
	}

	if (bNewPath && Skill + Personality.MovementAbility < 2.0f)
	{
		float DesiredYaw = FRotator::ClampAxis((GetMovePoint() - GetPawn()->GetActorLocation()).Rotation().Yaw);
		if (FMath::Abs<float>(FMath::FixedTurn(ControlRotation.Yaw, DesiredYaw, 45.0f + 30.0f * FMath::Max<float>(0.0f, Skill + Personality.MovementAbility)) - DesiredYaw) > KINDA_SMALL_NUMBER)
		{
			bFinishRotation = true;
		}
	}
	
	SetDefaultFocus();
}

void AUTBot::ConsiderTranslocation()
{
	// consider translocating if can't hit current target from where we are anyway
	// TODO: also check if movement is higher priority than attacking? (e.g. attack squad while not winning on the scoreboard)
	if ( bAllowTranslocator && TranslocTarget.IsZero() && GetWorld()->TimeSeconds - LastTranslocTime > TranslocInterval && (MoveTargetPoints.Num() > 1 || Cast<UUTReachSpec_HighJump>(CurrentPath.Spec.Get()) == NULL) &&
		Squad->ShouldUseTranslocator(this) )
	{
		TArray<FVector> TestPoints;
		TestPoints.Reserve(RouteCache.Num() + MoveTargetPoints.Num());
		for (int32 i = RouteCache.Num() - 1; i >= 0; i--)
		{
			TestPoints.Add(RouteCache[i].GetLocation(GetPawn()));
		}
		for (int32 i = MoveTargetPoints.Num() - 1; i >= 0; i--)
		{
			TestPoints.Add(MoveTargetPoints[i].Get());
		}
		const float GravityZ = (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement() != NULL) ? GetCharacter()->GetCharacterMovement()->GetGravityZ() : GetWorld()->GetGravityZ();
		for (const FVector& TargetLoc : TestPoints)
		{
			float Dist = (TargetLoc - GetPawn()->GetActorLocation()).Size();
			if (Dist > 1100.0f && Dist < 3500.0f && !GetWorld()->SweepTestByChannel(GetPawn()->GetActorLocation(), TargetLoc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(FName(TEXT("Transloc")), false, GetPawn())))
			{
				FVector TossVel;
				if (TransDiscTemplate == NULL || UUTGameplayStatics::UTSuggestProjectileVelocity(GetWorld(), TossVel, GetPawn()->GetActorLocation(), TargetLoc, NULL, FLT_MAX, TransDiscTemplate->ProjectileMovement->InitialSpeed, TransDiscTemplate->CollisionComp->GetUnscaledSphereRadius(), GravityZ))
				{
					TranslocTarget = TargetLoc;
				}
				break;
			}
		}
		if (!TranslocTarget.IsZero())
		{
			if (UTChar != NULL)
			{
				UTChar->StopFiring();
			}
			SwitchToBestWeapon();
			SetFocalPoint(TranslocTarget, SCRIPTEDMOVE_FOCUS_PRIORITY);
			// force immediate full aim update so bot doesn't throw to wrong location initially
			LastTacticalAimUpdateTime = -1.0f;
		}
	}
}

void AUTBot::SetDefaultFocus()
{
	const bool bStrafeCheck = !MoveTarget.IsValid() || Skill + Personality.MovementAbility > 1.7f + FMath::FRand();
	const bool bChargingFire = GetUTChar() != nullptr && GetUTChar()->GetWeapon() != nullptr && GetUTChar()->GetWeapon()->IsPreparingAttack();
	const FVector MoveDir = (GetMovePoint() - GetPawn()->GetActorLocation()).GetSafeNormal();
	if (GetTarget() != NULL && bLastCanAttackSuccess && (bStrafeCheck || bChargingFire || ((GetTarget()->GetTargetLocation() - GetPawn()->GetActorLocation()).GetSafeNormal() | MoveDir) > 0.85f))
	{
		SetFocus(GetTarget());
	}
	else if (Enemy != NULL && GetWorld()->TimeSeconds - FMath::Max<float>(LastEnemyChangeTime, GetEnemyInfo(Enemy, false)->LastSeenTime) < 3.0f && (bStrafeCheck || ((GetEnemyLocation(Enemy, false) - GetPawn()->GetActorLocation()).GetSafeNormal() | MoveDir) > 0.85f))
	{
		SetFocus(Enemy);
	}
	else if (Enemy != NULL && GetUTChar() != NULL && GetUTChar()->GetWeapon() != NULL && GetUTChar()->GetWeapon()->bMeleeWeapon)
	{
		SetFocus(Enemy);
	}
	else if (CurrentAction == NULL || !CurrentAction->SetFocusForNoTarget())
	{
		if (DefensePoint != NULL && DefensePoint == MoveTarget.Actor)
		{
			SetFocalPoint(DefensePoint->GetActorLocation() + DefensePoint->GetActorRotation().Vector() * 1000.0f);
		}
		else if (MoveTarget.Actor.IsValid())
		{
			SetFocus(MoveTarget.Actor.Get());
		}
		else
		{
			// the movement code automatically sets Move priority (pri 1, below gameplay) to where we are going
			ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

void AUTBot::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	Canvas->SetDrawColor(255, 255, 255);
	Canvas->DrawText(GEngine->GetSmallFont(), FString::Printf(TEXT("MoveTarget: %s[%s] - Focus: %s[%s]"), MoveTarget.Actor.IsValid() ? *MoveTarget.Actor->GetName() : TEXT(""), *MoveTarget.GetLocation(GetPawn()).ToString(), (GetFocusActor() != nullptr) ? *GetFocusActor()->GetName() : TEXT(""), *GetFocalPoint().ToString()), 4.0f, YPos);
	YPos += YL;
	Canvas->SetDrawColor(0, 255, 0);
	Canvas->DrawText(GEngine->GetSmallFont(), FString::Printf(TEXT("ORDERS: %s"), (Squad != nullptr) ? *Squad->GetCurrentOrders(this).ToString() : TEXT("NO SQUAD")), 4.0f, YPos);
	YPos += YL;
	Canvas->DrawText(GEngine->GetSmallFont(), GoalString, 4.0f, YPos);
	YPos += YL;
	YPos += YL;
	Canvas->SetDrawColor(255, 0, 0);
	Canvas->DrawText(GEngine->GetSmallFont(), FString::Printf(TEXT("ENEMIES (current: %s (visible: %s), last rated %f ago)"), (Enemy != NULL && Enemy->PlayerState != NULL) ? *Enemy->PlayerState->PlayerName : *GetNameSafe(Enemy), (Enemy != NULL && IsEnemyVisible(Enemy)) ? TEXT("True") : TEXT("False"), GetWorld()->TimeSeconds - LastPickEnemyTime), 4.0f, YPos);
	YPos += YL;
	for (const FBotEnemyRating& RatingInfo : LastPickEnemyRatings)
	{
		Canvas->DrawText(GEngine->GetSmallFont(), FString::Printf(TEXT("%s rated %4.2f"), *RatingInfo.PlayerName, RatingInfo.Rating), 4.0f, YPos);
		YPos += YL;
	}
	YPos += YL;

	if (GetPawn() != NULL)
	{
		if (MoveTarget.IsValid())
		{
			for (int32 i = 0; i < MoveTargetPoints.Num() - 1; i++)
			{
				DrawDebugLine(GetWorld(), (i == 0) ? GetPawn()->GetActorLocation() : MoveTargetPoints[i - 1].Get(), MoveTargetPoints[i].Get(), FColor(0, 255, 0));
			}
			DrawDebugLine(GetWorld(), (MoveTargetPoints.Num() > 1) ? MoveTargetPoints[MoveTargetPoints.Num() - 2].Get() : GetPawn()->GetActorLocation(), MoveTarget.GetLocation(GetPawn()), FColor(0, 255, 0));
			if (bAdjusting)
			{
				DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), AdjustLoc, FColor(255, 0, 0));
			}
		}
		DrawDebugRoute(GetWorld(), GetPawn(), RouteCache);
		if (Squad != NULL)
		{
			Squad->DrawDebugSquadRoute(this);
		}
		if (!TranslocTarget.IsZero())
		{
			DrawDebugSphere(GetWorld(), TranslocTarget, 24.0f, 8, FColor(0, 0, 255));
		}
		DrawDebugSphere(GetWorld(), GetFocalPoint(), 24.0f, 8, FColor(255, 255, 0));
	}
}

void AUTBot::ApplyWeaponAimAdjust(FVector TargetLoc, FVector& FocalPoint)
{
	AUTWeapon* MyWeap = NULL;
	if (UTChar != NULL)
	{
		// if switching weapons, aim using upcoming weapon instead
		if (UTChar->GetPendingWeapon() != NULL && (UTChar->GetWeapon() == NULL || !UTChar->GetWeapon()->IsFiring()))
		{
			MyWeap = UTChar->GetPendingWeapon();
		}
		else
		{
			MyWeap = UTChar->GetWeapon();
		}
	}
	if (MyWeap != NULL)
	{
		// tactical aim adjustments
		// if weapon returns a custom aim target it's responsible for this part (including leading, if desired)
		bool bInstantHit = true;
		if (TargetLoc == FocalPoint)
		{
			AUTProjectile* DefaultProj = NULL;
			if (MyWeap->ProjClass.IsValidIndex(NextFireMode) && MyWeap->ProjClass[NextFireMode] != NULL)
			{
				DefaultProj = MyWeap->ProjClass[NextFireMode].GetDefaultObject();
				// handle leading
				bool bReallyLeadTarget = bLeadTarget;
				// always lead critical enemies at low skill unless they're human as that better normalizes offense/defense effectiveness
				if (!bReallyLeadTarget && GetTarget() == Enemy && Squad->MustKeepEnemy(this, Enemy))
				{
					bReallyLeadTarget = (Skill >= 2.0f || Cast<APlayerController>(Enemy->Controller) == nullptr);
				}
				if (bReallyLeadTarget && GetTarget() != NULL && GetTarget() == GetFocusActor())
				{
					FVector FireLocation = GetPawn()->GetActorLocation();
					FireLocation.Z += GetPawn()->BaseEyeHeight;
					float TravelTime = DefaultProj->StaticGetTimeToLocation(FocalPoint, FireLocation);
					if (TravelTime > 0.0f)
					{
						bInstantHit = false;

						ACharacter* EnemyChar = Cast<ACharacter>(GetTarget());
						if (EnemyChar != NULL && EnemyChar->GetCharacterMovement()->MovementMode == MOVE_Falling)
						{
							// take gravity and landing into account
							TrackedVelocity.Z = TrackedVelocity.Z + 0.5f * TravelTime * EnemyChar->GetCharacterMovement()->GetGravityZ();
						}
						// TODO: if target is walking on slope that needs to be taken into account
						FocalPoint += TrackedVelocity * TravelTime;

						if (BlockedAimTarget == GetTarget() || GetWorld()->TimeSeconds - LastTacticalAimUpdateTime < TacticalAimUpdateInterval)
						{
							BlockedAimTarget = NULL;

							// make sure enemy is not hemmed by wall (or landing)
							FCollisionQueryParams Params(FName(TEXT("AimWallCheck")), true, GetPawn());
							Params.AddIgnoredActor(GetTarget());
							FHitResult Hit;
							if (GetWorld()->LineTraceSingleByChannel(Hit, (GetTarget() == Enemy) ? GetEnemyLocation(Enemy, false) : GetTarget()->GetActorLocation(), FocalPoint, COLLISION_TRACE_WEAPON, Params))
							{
								BlockedAimTarget = GetTarget();
								FocalPoint = Hit.Location - 24.f * TrackedVelocity.GetSafeNormal();
							}

							// make sure have a clean shot at where I think he's going
							FVector ProjStart = GetPawn()->GetActorLocation();
							ProjStart.Z += GetPawn()->BaseEyeHeight;
							ProjStart += (FocalPoint - ProjStart).GetSafeNormal() * MyWeap->FireOffset.X;
							if (GetWorld()->LineTraceSingleByChannel(Hit, ProjStart, FocalPoint, COLLISION_TRACE_WEAPON, Params))
							{
								BlockedAimTarget = GetTarget();
								// apply previous iterative value
								FocalPoint = TargetLoc + LastIterativeLeadCheck * (FocalPoint - TargetLoc);
								if (GetWorld()->LineTraceSingleByChannel(Hit, ProjStart, FocalPoint, COLLISION_TRACE_WEAPON, Params) && EnemyChar != NULL)
								{
									// see if head would work
									FocalPoint.Z += EnemyChar->BaseEyeHeight;

									if (GetWorld()->LineTraceSingleByChannel(Hit, ProjStart, FocalPoint, COLLISION_TRACE_WEAPON, Params))
									{
										// iteratively track down correct aim spot over multiple ticks
										LastIterativeLeadCheck *= 0.5f;
									}
									else
									{
										LastIterativeLeadCheck = FMath::Clamp<float>(LastIterativeLeadCheck + 0.5f * (1.f - LastIterativeLeadCheck), 0.f, 1.f);
									}
								}
								else
								{
									LastIterativeLeadCheck = FMath::Clamp<float>(LastIterativeLeadCheck + 0.5f * (1.f - LastIterativeLeadCheck), 0.f, 1.f);
								}
								FocalPoint = TargetLoc + LastIterativeLeadCheck * (FocalPoint - TargetLoc);
							}
							else
							{
								LastIterativeLeadCheck = 1.f;
							}
						}
					}
				}
			}

			if (GetWorld()->TimeSeconds - LastTacticalAimUpdateTime < TacticalAimUpdateInterval)
			{
				FocalPoint += TacticalAimOffset;
			}
			else
			{
				LastTacticalAimUpdateTime = GetWorld()->TimeSeconds;

				TargetLoc = FocalPoint;
				const FVector FireStart = MyWeap->GetFireStartLoc(NextFireMode);

				// handle tossed projectiles
				if (DefaultProj != NULL && DefaultProj->ProjectileMovement != NULL && DefaultProj->ProjectileMovement->ProjectileGravityScale > 0.0f)
				{
					// TODO: calculate toss direction, set FocalPoint to toss dir
					const float GravityZ = ((GetCharacter() != NULL) ? GetCharacter()->GetCharacterMovement()->GetGravityZ() : GetWorld()->GetDefaultGravityZ()) * DefaultProj->ProjectileMovement->ProjectileGravityScale;
					const float ProjRadius = (DefaultProj->CollisionComp != NULL) ? DefaultProj->CollisionComp->GetCollisionShape().GetExtent().X : 0.0f;
					const FVector StartLoc = GetPawn()->GetActorLocation() + (FocalPoint - GetPawn()->GetActorLocation()).Rotation().RotateVector(MyWeap->FireOffset);
					float ProjSpeed = DefaultProj->ProjectileMovement->InitialSpeed;
					// if firing upward, add minimum possible TossZ contribution to effective speed to improve toss prediction
					if (DefaultProj->TossZ > 0.0f)
					{
						ProjSpeed += FMath::Max<float>(0.0f, (FocalPoint - StartLoc).GetSafeNormal().Z * DefaultProj->TossZ);
					}
					FVector TossVel;
					const float ZTolerance = (GetFocusActor() != NULL) ? (GetFocusActor()->GetSimpleCollisionHalfHeight() * 0.8f) : FLT_MAX;
					if (UUTGameplayStatics::UTSuggestProjectileVelocity(this, TossVel, StartLoc, FocalPoint, GetTarget(), ZTolerance, ProjSpeed, ProjRadius, GravityZ))
					{
						TossVel.Z -= DefaultProj->TossZ;
						TargetLoc = GetPawn()->GetActorLocation() + TossVel.GetSafeNormal() * 2000.0f;
					}
					// TODO: move this to a function called when preparing move, no need to do this repeatedly
					else if (FocalPoint == TranslocTarget && Cast<UUTReachSpec_HighJump>(CurrentPath.Spec.Get()) != NULL)
					{
						// try adjusting target to edge of desired navmesh polygon
						FVector HitLoc;
						if (NavData->Raycast(TranslocTarget - FVector(0.0f, 0.0f, GetPawn()->GetSimpleCollisionHalfHeight()), GetPawn()->GetActorLocation(), HitLoc, NavData->GetDefaultQueryFilter()))
						{
							float ZDiff = TranslocTarget.Z - HitLoc.Z;
							HitLoc.Z += ZDiff * 0.5f;
							FVector Extent = GetPawn()->GetSimpleCollisionCylinderExtent() * FVector(2.0f, 2.0f, 1.0f);
							Extent.Z += FMath::Abs<float>(ZDiff) * 0.5f;
							NavNodeRef WallPoly = NavData->UTFindNearestPoly(HitLoc, Extent);
							if (WallPoly != INVALID_NAVNODEREF)
							{
								TArray<FLine> Walls = NavData->GetPolyWalls(WallPoly);
								if (Walls.Num() > 0)
								{
									FVector TestLoc = TranslocTarget;
									float BestDist = FLT_MAX;
									for (const FLine& TestWall : Walls)
									{
										float Dist = (TestWall.GetCenter() - GetPawn()->GetActorLocation()).Size();
										if (Dist < BestDist)
										{
											TestLoc = TestWall.GetCenter();
											BestDist = Dist;
										}
									}
									TestLoc.Z += GetPawn()->GetSimpleCollisionHalfHeight();
									if ( UUTGameplayStatics::UTSuggestProjectileVelocity(this, TossVel, StartLoc, TestLoc, NULL, ZTolerance, ProjSpeed, ProjRadius, GravityZ) ||
										UGameplayStatics::SuggestProjectileVelocity(this, TossVel, StartLoc, TestLoc, ProjSpeed, true, ProjRadius, GravityZ, ESuggestProjVelocityTraceOption::DoNotTrace) )
									{
										TranslocTarget = TestLoc;
										SetFocalPoint(TestLoc, SCRIPTEDMOVE_FOCUS_PRIORITY);
										TossVel.Z -= DefaultProj->TossZ;
										TargetLoc = GetPawn()->GetActorLocation() + TossVel.GetSafeNormal() * 2000.0f;
									}
								}
							}
						}
					}
					else if (UGameplayStatics::SuggestProjectileVelocity(this, TossVel, StartLoc, FocalPoint, ProjSpeed, !TranslocTarget.IsZero(), ProjRadius, GravityZ, ESuggestProjVelocityTraceOption::DoNotTrace))
					{
						// any valid arc better than a straight line, even if we think it's blocked
						// besides better chance of hit anyway with aimerror, etc there's also a chance that the trace is a false positive because it's approximated for speed
						// TODO: maybe take arc that gets farther before blocker is found?
						TossVel.Z -= DefaultProj->TossZ;
						TargetLoc = GetPawn()->GetActorLocation() + TossVel.GetSafeNormal() * 2000.0f;
					}
					else if (FocalPoint == TranslocTarget)
					{
						// use a generic high angle toss for translocation; disc might get close enough to be worth it
						const FVector Dir = (TranslocTarget - GetPawn()->GetActorLocation()).GetSafeNormal();
						TargetLoc = GetPawn()->GetActorLocation() + (Dir + 0.5f * (FVector(0.0f, 0.0f, 1.0f) - Dir)).GetSafeNormal() * 2000.0f;
					}
				}
				else
				{
					FCollisionQueryParams Params(FName(TEXT("ApplyWeaponAimAdjust")), true, GetPawn());
					Params.AddIgnoredActor(GetFocusActor());

					const float TargetHeight = (GetFocusActor() != NULL) ? GetFocusActor()->GetSimpleCollisionHalfHeight() : 0.0f;
					AUTCharacter* EnemyChar = Cast<AUTCharacter>(GetFocusActor());
					bool bDefendMelee = (EnemyChar != NULL && EnemyChar->GetWeapon() != NULL && EnemyChar->GetWeapon()->bMeleeWeapon);

					bool bClean = false; // so will fail first check unless shooting at feet

					if ( MyWeap->bRecommendSplashDamage && EnemyChar != NULL && (Skill >= 4.0f + FMath::Max<float>(0.0f, Personality.Accuracy + Personality.Tactics) || bDefendMelee)
						&& ( (EnemyChar->GetCharacterMovement()->MovementMode == MOVE_Falling && (GetPawn()->GetActorLocation().Z + 180.0f >= FocalPoint.Z))
							|| (GetPawn()->GetActorLocation().Z - 1.0f >= FocalPoint.Z && (bDefendMelee || Skill > 6.5f * FMath::FRand() - 0.5f)) ) )
					{
						FHitResult Hit;
						bClean = !GetWorld()->LineTraceSingleByChannel(Hit, TargetLoc, TargetLoc - FVector(0.0f, 0.0f, TargetHeight + 13.0f), COLLISION_TRACE_WEAPONNOCHARACTER, Params);
						if (!bClean)
						{
							TargetLoc = Hit.Location + FVector(0.0f, 0.0f, 6.0f);
							bClean = !GetWorld()->LineTraceTestByChannel(FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params);
						}
						else
						{
							bClean = (EnemyChar->GetCharacterMovement()->MovementMode == MOVE_Falling && !GetWorld()->LineTraceTestByChannel(FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params));
						}
					}
					bool bCheckedHead = false;
					bool bHeadClean = false;
					if (MyWeap->bSniping)
					{
						float SkillThreshold;
						if (IsStopped())
						{
							SkillThreshold = 4.0f + 6.0f * FMath::FRand();
						}
						else
						{
							SkillThreshold = 5.5f + 6.0f * FMath::FRand();
						}
						if (IsFavoriteWeapon(MyWeap->GetClass()) || FMath::FRand() < Personality.Accuracy)
						{
							SkillThreshold -= 1.5f;
						}
						if (Skill + Personality.Accuracy < SkillThreshold)
						{
							// try head
							TargetLoc.Z = FocalPoint.Z + 0.9f * TargetHeight;
							bClean = !GetWorld()->LineTraceTestByChannel(FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params);
							bCheckedHead = true;
							bHeadClean = bClean;
						}
					}

					if (!bClean)
					{
						// try middle
						TargetLoc.Z = FocalPoint.Z;
						bClean = !GetWorld()->LineTraceTestByChannel(FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params);
					}

					if (!bClean)
					{
						// try head
						TargetLoc.Z = FocalPoint.Z + 0.9f * TargetHeight;
						bClean = bCheckedHead ? bHeadClean : !GetWorld()->LineTraceTestByChannel(FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params);
					}
					if (!bClean && Enemy != NULL && GetFocusActor() == Enemy)
					{
						const FBotEnemyInfo* EnemyInfo = GetEnemyInfo(Enemy, false);
						if (GetWorld()->TimeSeconds - EnemyInfo->LastSeenTime < 10.0f)
						{
							TargetLoc = EnemyInfo->LastSeenLoc;
							if (GetPawn()->GetActorLocation().Z >= TargetLoc.Z)
							{
								TargetLoc.Z -= 0.4f * TargetHeight;
							}
							FHitResult Hit;
							if (GetWorld()->LineTraceSingleByChannel(Hit, FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params))
							{
								TargetLoc = EnemyInfo->LastSeenLoc + 2.0f * TargetHeight * Hit.Normal;
								if (MyWeap != NULL && MyWeap->GetDamageRadius(NextFireMode) > 0.0f && Skill >= 4.0f && GetWorld()->LineTraceSingleByChannel(Hit, FireStart, TargetLoc, COLLISION_TRACE_WEAPONNOCHARACTER, Params))
								{
									TargetLoc += 2.0f * TargetHeight * Hit.Normal;
								}
							}
						}
					}
				}
				TacticalAimOffset = TargetLoc - FocalPoint;
				FocalPoint = TargetLoc;
			}
		}
	}
}

void AUTBot::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = Super::GetFocalPoint(); // our version returns adjusted result
	if (!FocalPoint.IsZero())
	{
		APawn* P = GetPawn();
		if (P != NULL)
		{
			const float WorldTime = GetWorld()->TimeSeconds;

			FVector NewTrackedVelocity = (GetFocusActor() != NULL) ? GetFocusActor()->GetVelocity() : FVector::ZeroVector;
			bLastCanAttackSuccess = false;
			bStopSuppressiveWeapon = false;

			// warning: assumption that if bot wants to shoot an enemy Pawn it always sets it as Enemy
			if (Enemy != NULL && GetFocusActor() == Enemy)
			{
				AUTCharacter* EnemyUTC = Cast<AUTCharacter>(Enemy);
				TArray<FSavedPosition> SavedPositions;
				if (bLeadTarget ? GetEnemyInfo(Enemy, true)->CanUseExactLocation(WorldTime) : IsEnemyVisible(Enemy))
				{
					if (EnemyUTC != NULL && EnemyUTC->SavedPositions.Num() > 0 && EnemyUTC->SavedPositions[0].Time <= WorldTime - TrackingReactionTime)
					{
						EnemyUTC->GetSimplifiedSavedPositions(SavedPositions, true);
					}
				}
				bool bGotPredictedPosition = false;
				bLargeTrackedVelocityChange = false;
				if (SavedPositions.Num() > 1)
				{
					// determine his position and velocity at the appropriate point in the past
					for (int32 i = 1; i < SavedPositions.Num(); i++)
					{
						if (SavedPositions[i].Time > WorldTime - TrackingReactionTime)
						{
							FVector TargetLoc = SavedPositions[i - 1].Position + (SavedPositions[i].Position - SavedPositions[i - 1].Position) * (WorldTime - TrackingReactionTime - SavedPositions[i - 1].Time) / (SavedPositions[i].Time - SavedPositions[i - 1].Time);
							NewTrackedVelocity = SavedPositions[i - 1].Velocity + (SavedPositions[i].Velocity - SavedPositions[i - 1].Velocity) * (WorldTime - TrackingReactionTime - SavedPositions[i - 1].Time) / (SavedPositions[i].Time - SavedPositions[i - 1].Time);
							float VelInterpTime = FMath::Min((GetWorld()->GetTimeSeconds() - TrackingTimeStamp) / TrackingInterpTime, 1.f);
							TrackingTimeStamp = GetWorld()->GetTimeSeconds();
							TrackedVelocity = (1.f - VelInterpTime)*TrackedVelocity + VelInterpTime*NewTrackedVelocity;
							TrackedVelocity.Z = NewTrackedVelocity.Z; // check it doesn't disappear, check vs dodging, adad, faster catch up when stop
							bLargeTrackedVelocityChange = EnemyUTC && !SavedPositions[i - 1].Velocity.IsNearlyZero() && !SavedPositions[i].Velocity.IsNearlyZero() && (SavedPositions[i - 1].Velocity.Z == 0.f) && (SavedPositions[i].Velocity.Z != 0.f) && (SavedPositions[i].Velocity.Size2D() > 1.2f*EnemyUTC->GetCharacterMovement()->MaxWalkSpeed);
							// handle the case where the predicted velocity puts the target right through us (e.g. close range enemy is charging)
							// in that case clamp the prediction to collide with us so that the bot doesn't target the opposite direction
							if ( FMath::PointDistToSegment(GetPawn()->GetActorLocation(), TargetLoc, TargetLoc + TrackedVelocity * (TrackingReactionTime + TrackingPredictionError)) < GetPawn()->GetSimpleCollisionRadius() &&
								((Enemy->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal2D() | (TargetLoc - GetPawn()->GetActorLocation()).GetSafeNormal2D()) > 0.2f )
							{
								float SavedZ = TrackedVelocity.Z;
								TrackedVelocity = TrackedVelocity.GetSafeNormal2D() * (((TargetLoc - GetPawn()->GetActorLocation()).Size() - GetPawn()->GetSimpleCollisionRadius()) / (TrackingReactionTime + TrackingPredictionError));
								TrackedVelocity.Z = SavedZ;
							}
							FVector SideDir = ((TargetLoc - P->GetActorLocation()) ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
							//DrawDebugSphere(GetWorld(), TargetLoc + NewTrackedVelocity*TrackingReactionTime, 40.f, 8, FColor::White, false);
							//DrawDebugSphere(GetWorld(), TargetLoc + TrackedVelocity*TrackingReactionTime, 40.f, 8, FColor::Yellow, false);
							//DrawDebugSphere(GetWorld(), TargetLoc + TrackedVelocity*(TrackingReactionTime + TrackingPredictionError), 40.f, 8, FColor::Red, false);
							TargetLoc = TargetLoc + TrackedVelocity * (TrackingReactionTime + TrackingPredictionError) + SideDir * (TrackingOffsetError * FMath::Min<float>(500.f, (TargetLoc - P->GetActorLocation()).Size()));
							//DrawDebugSphere(GetWorld(), TargetLoc, 40.f, 8, FColor::Red, false); // FIXME THIS SEEMS TOO SMALL AT SKILL 4
							if (EnemyUTC != NULL)
							{
								TargetLoc += EnemyUTC->GetLocationCenterOffset();
							}

							if (CanAttack(Enemy, TargetLoc, false, !bPickNewFireMode, &NextFireMode, &FocalPoint))
							{
								bLastCanAttackSuccess = true;
								bPickNewFireMode = false;
								ApplyWeaponAimAdjust(TargetLoc, FocalPoint);
							}
							// if suppressive weapon spam last seen loc using current fire mode
							else if (UTChar != nullptr && UTChar->GetWeapon() != nullptr && UTChar->GetWeapon()->bRecommendSuppressiveFire)
							{
								if (CanAttack(Enemy, GetEnemyInfo(Enemy, false)->LastSeenLoc, true, true))
								{
									// note: bLastCanAttackSuccess left at false so AI will give up on this after a short time (see CheckWeaponFiring())
									FocalPoint = GetEnemyInfo(Enemy, false)->LastSeenLoc;
									ApplyWeaponAimAdjust(FocalPoint, FocalPoint);
								}
								else
								{
									// we defer this to the next weapon update so it doesn't look so mechanical
									bStopSuppressiveWeapon = true;
								}
							}
							else
							{
								FocalPoint = TargetLoc;
							}

							bGotPredictedPosition = true;
							break;
						}
					}
				}
				if (!bGotPredictedPosition)
				{
					// important to match the behavior above of only using own info when !bLeadTarget to avoid mismatches that cause temporary perfect aim when receiving enemy info from team
					const FVector TargetLoc = bLeadTarget ? GetEnemyLocation(Enemy, true) : GetEnemyInfo(Enemy, false)->LastSeenLoc;
					if (CanAttack(Enemy, TargetLoc, false, !bPickNewFireMode, &NextFireMode, &FocalPoint))
					{
						bLastCanAttackSuccess = true;
						bPickNewFireMode = false;
						ApplyWeaponAimAdjust(TargetLoc, FocalPoint);
					}
					else
					{
						FocalPoint = GetEnemyLocation(Enemy, false);
					}
				}
			}
			else
			{
				float VelInterpTime = FMath::Min(DeltaTime/TrackingInterpTime, 1.f);
				TrackedVelocity = (1.f - VelInterpTime)*TrackedVelocity + VelInterpTime*NewTrackedVelocity;
				if (Target != NULL && GetFocusActor() == Target)
				{
					FVector TargetLoc = GetFocusActor()->GetTargetLocation();
					if (CanAttack(GetFocusActor(), TargetLoc, false, !bPickNewFireMode, &NextFireMode, &FocalPoint))
					{
						bLastCanAttackSuccess = true;
						bPickNewFireMode = false;
						ApplyWeaponAimAdjust(TargetLoc, FocalPoint);
					}
				}
				// check for aiming weapon at scripted target as part of a special movement action (translocator, impact jump, etc)
				else if (FocusInformation.Priorities.IsValidIndex(SCRIPTEDMOVE_FOCUS_PRIORITY) && FocusInformation.Priorities[SCRIPTEDMOVE_FOCUS_PRIORITY].Position != FAISystem::InvalidLocation)
				{
					// note: lack of CanAttack() here is intentional as the target location may be intentionally OOE (e.g. firing straight down for impact jump)
					ApplyWeaponAimAdjust(FocalPoint, FocalPoint);
				}
			}

			FinalFocalPoint = FocalPoint; // for later GetFocalPoint() queries

			FVector Direction = FocalPoint - P->GetActorLocation();
			FRotator DesiredRotation = Direction.Rotation();

			// Don't pitch view of walking pawns when simply traversing path and not looking at a target
			// (make an exception for scripted move focus points since they may require pitch for weapon aiming)
			if ( GetPawn()->GetMovementComponent() && GetPawn()->GetMovementComponent()->IsMovingOnGround() && GetFocusActor() == NULL &&
				(!FocusInformation.Priorities.IsValidIndex(SCRIPTEDMOVE_FOCUS_PRIORITY) || FocusInformation.Priorities[SCRIPTEDMOVE_FOCUS_PRIORITY].Position == FAISystem::InvalidLocation) )
			{
				DesiredRotation.Pitch = 0.f;
			}

			FRotator NewControlRotation(FMath::FixedTurn(ControlRotation.Pitch, DesiredRotation.Pitch, RotationRate.Pitch * DeltaTime), FMath::FixedTurn(ControlRotation.Yaw, DesiredRotation.Yaw, RotationRate.Yaw * DeltaTime), 0);

			SetControlRotation(NewControlRotation);
			if (bUpdatePawn)
			{
				P->FaceRotation(NewControlRotation, DeltaTime);
			}
		}
	}
}

void AUTBot::NotifyWalkingOffLedge()
{
	// jump if needed by path
	if (GetCharacter() != NULL && MoveTarget.IsValid() && (CurrentPath.ReachFlags & R_JUMP)) // TODO: maybe also if chasing enemy?
	{
		FVector Diff = GetMovePoint() - GetCharacter()->GetActorLocation();
		float XYTime = Diff.Size2D() / GetCharacter()->GetCharacterMovement()->MaxWalkSpeed;
		float DesiredJumpZ = Diff.Z / XYTime - 0.5f * GetCharacter()->GetCharacterMovement()->GetGravityZ() * XYTime;
		// TODO: if high skill also check if path is walkable from simple fall location to dest via navmesh raytrace to minimize in air time
		if (DesiredJumpZ > 0.0f)
		{
			// try forward dodge instead if target is a little too far but is below and path is clear
			bool bDodged = false;
			if (DesiredJumpZ > GetCharacter()->GetCharacterMovement()->JumpZVelocity && GetUTChar() != NULL && GetUTChar()->UTCharacterMovement->CanDodge())
			{
				float DodgeXYTime = Diff.Size2D() / GetUTChar()->UTCharacterMovement->DodgeImpulseHorizontal;
				float DodgeDesiredJumpZ = Diff.Z / DodgeXYTime - 0.5f * GetCharacter()->GetCharacterMovement()->GetGravityZ() * DodgeXYTime;
				// TODO: need FRouteCacheItem function that conditionally Z adjusts
				FCollisionQueryParams TraceParams(FName(TEXT("Dodge")), false, GetPawn());
				if (DodgeDesiredJumpZ <= GetUTChar()->UTCharacterMovement->DodgeImpulseVertical && !GetWorld()->LineTraceTestByChannel(GetCharacter()->GetActorLocation(), GetMovePoint() + FVector(0.0f, 0.0f, 60.0f), ECC_Pawn, TraceParams))
				{
					// TODO: very minor cheat here - non-cardinal dodge
					//		to avoid would need to add the ability for the AI to reject the fall in the first place and delay until it rotates to correct rotation
					//FRotationMatrix YawMat(FRotator(0.f, GetUTChar()->GetActorRotation().Yaw, 0.f));
					FRotationMatrix YawMat(FRotator(0.f, Diff.Rotation().Yaw, 0.f));
					// forward dodge
					FVector X = YawMat.GetScaledAxis(EAxis::X).GetSafeNormal();
					FVector Y = YawMat.GetScaledAxis(EAxis::Y).GetSafeNormal();
					GetUTChar()->Dodge(X, Y);
					bDodged = true;
				}
			}
			if (!bDodged)
			{
				// if need super jump and would hit head going straight there, don't move XY until we get some height first
				if (Cast<UUTReachSpec_HighJump>(CurrentPath.Spec.Get()) != NULL && GetWorld()->LineTraceTestByChannel(GetPawn()->GetActorLocation(), GetMovePoint(), ECC_Pawn, FCollisionQueryParams(FName(TEXT("JumpCeiling")), false, GetPawn())))
				{
					GetCharacter()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
				}
				else
				{
					GetCharacter()->GetCharacterMovement()->Velocity = Diff.GetSafeNormal2D() * (GetCharacter()->GetCharacterMovement()->MaxWalkSpeed * FMath::Min<float>(1.0f, DesiredJumpZ / FMath::Max<float>(GetCharacter()->GetCharacterMovement()->JumpZVelocity, 1.0f)));
				}
				GetCharacter()->GetCharacterMovement()->DoJump(false);
			}
		}
		else
		{
			// clamp initial XY speed if target is directly below
			float ZTime = FMath::Sqrt(Diff.Z / (0.5f * GetCharacter()->GetCharacterMovement()->GetGravityZ()));
			GetCharacter()->GetCharacterMovement()->Velocity = Diff.GetSafeNormal2D() * FMath::Min<float>(GetCharacter()->GetCharacterMovement()->MaxWalkSpeed, Diff.Size2D() / ZTime);
		}
	}
}

void AUTBot::NotifyBump(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	checkSlow(SelfActor == GetPawn());
	// update locational info for enemies we bump into, since we might be looking the other way (strafing)
	APawn* P = Cast<APawn>(OtherActor);
	if (P != NULL && P->Controller != NULL && !IsTeammate(P))
	{
		UpdateEnemyInfo(P, EUT_Seen);
	}
}

void AUTBot::ApplyCrouch()
{
	if (GetCharacter() != NULL)
	{
		GetCharacter()->GetCharacterMovement()->bWantsToCrouch = true;
	}
}

void AUTBot::NotifyMoveBlocked(const FHitResult& Impact)
{
	// ignore impacts involving lift we are standing on - lift will tell us if we need to move to allow it to complete its path
	if ((Cast<AUTLift>(Impact.Actor.Get()) == NULL || APawn::GetMovementBaseActor(GetPawn()) != Impact.Actor) && (CurrentAction == NULL || !CurrentAction->NotifyMoveBlocked(Impact)) && GetCharacter() != NULL)
	{
		if (GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Walking)
		{
			APawn* HitPawn = Cast<APawn>(Impact.Actor.Get());
			if (HitPawn != NULL && !IsTeammate(HitPawn))
			{
				UpdateEnemyInfo(HitPawn, EUT_HeardExact);
			}
			// adjust around friendly or inactive Pawns
			if (HitPawn != NULL && (HitPawn->GetController() == NULL || IsTeammate(HitPawn)))
			{
				FVector VelDir = (MoveTarget.GetLocation(GetPawn()) - GetPawn()->GetActorLocation()).GetSafeNormal2D();
				FVector OtherDir = (HitPawn->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal2D();
				if ((VelDir | OtherDir) > 0.8f)
				{
					FVector SideDir(VelDir.Y, -1.0f * VelDir.X, 0.0f);
					if ((SideDir | OtherDir) > 0.0f)
					{
						SideDir *= -1.0f;
					}
					FVector NewAdjustLoc = GetPawn()->GetActorLocation() + 3.0f * HitPawn->GetSimpleCollisionRadius() * (0.5f * VelDir + SideDir);
					// make sure adjust location isn't through a wall
					FHitResult AdjustTraceHit;
					if (GetWorld()->LineTraceSingleByChannel(AdjustTraceHit, GetPawn()->GetActorLocation(), AdjustLoc, ECC_Pawn, FCollisionQueryParams(), WorldResponseParams))
					{
						AdjustLoc = AdjustTraceHit.Location - AdjustTraceHit.Normal;
					}
					SetAdjustLoc(NewAdjustLoc);
				}
			}
			else if (HitPawn != NULL && GetMoveTarget().Actor == HitPawn)
			{
				if (Skill + Personality.MovementAbility >= 4.0f)
				{
					DoTacticalMove();
				}
				else
				{
					DoRangedAttackOn(HitPawn);
				}
			}
			// crouch if path says we should
			// FIXME: what if going for detour in the middle of crouch path? (dropped pickup, etc)
			else if ((CurrentPath.IsSet() && CurrentPath.CollisionHeight < FMath::TruncToInt(GetCharacter()->GetSimpleCollisionHalfHeight())) || (!CurrentPath.IsSet() && MoveTarget.Node != nullptr && MoveTarget.Node->MinPolyEdgeSize.Height < FMath::TruncToInt(GetCharacter()->GetSimpleCollisionHalfHeight())))
			{
				if ((CurrentPath.IsSet() ? CurrentPath.CollisionHeight : MoveTarget.Node->MinPolyEdgeSize.Height) < FMath::TruncToInt(GetCharacter()->GetCharacterMovement()->CrouchedHalfHeight))
				{
					// capabilities changed since path was found
					MoveTimer = -1.0f;
				}
				else
				{
					// we're in the middle of the movement code and setting crouch here will get clobbered
					// (see UUTCharacterMovement::PerformMovement())
					GetWorldTimerManager().SetTimerForNextTick(this, &AUTBot::ApplyCrouch);
				}
			}
			else
			{
				const FVector MovePoint = GetMovePoint();
				const FVector MyLoc = GetCharacter()->GetActorLocation();
				// get as close as possible to path
				bool bGotAdjustLoc = false;
				if (!bAdjusting && CurrentPath.EndPoly != INVALID_NAVNODEREF)
				{
					FCollisionQueryParams Params(FName(TEXT("MoveBlocked")), false, GetPawn());
					if (!GetWorld()->LineTraceTestByChannel(LastReachedMovePoint + FVector(0.0f, 0.0f, GetCharacter()->GetCharacterMovement()->MaxStepHeight), MovePoint, ECC_Pawn, Params))
					{
						// path requires adjustment or jump from the start
						// check if jump would be valid
						float JumpApexTime = GetCharacter()->GetCharacterMovement()->JumpZVelocity / -GetCharacter()->GetCharacterMovement()->GetGravityZ();
						float JumpHeight = GetCharacter()->GetCharacterMovement()->JumpZVelocity * JumpApexTime + 0.5 * GetCharacter()->GetCharacterMovement()->GetGravityZ() * FMath::Square(JumpApexTime);
						if (!GetCharacter()->CanJump() || GetWorld()->LineTraceTestByChannel(LastReachedMovePoint + FVector(0.0f, 0.0f, JumpHeight), MovePoint, ECC_Pawn, Params))
						{
							// test opposite hit direction, then sides of movement dir
							const FVector Side = (MovePoint - MyLoc).GetSafeNormal() ^ FVector(0.0f, 0.0f, 1.0f);
							const float AdjustDist = GetPawn()->GetSimpleCollisionRadius() * 1.5f;
							FVector TestLocs[] = { MyLoc + Impact.Normal * AdjustDist, MyLoc + Side * AdjustDist, MyLoc - Side * AdjustDist };
							for (int32 i = 0; i < ARRAY_COUNT(TestLocs); i++)
							{
								if (!GetWorld()->LineTraceTestByChannel(MyLoc, TestLocs[i], ECC_Pawn, Params))
								{
									SetAdjustLoc(TestLocs[i]);
									bGotAdjustLoc = true;
									break;
								}
							}
						}
					}
					else if ((MovePoint - LastReachedMovePoint).SizeSquared2D() > 1.0f)
					{
						// get XY distance from desired path
						// note that the size of the result of ClosestPointOnSegment() with 2D vectors doesn't match that of using 3D vectors followed by Size2D()
						FVector LastPointNoZ(LastReachedMovePoint.X, LastReachedMovePoint.Y, 0.0f);
						FVector CurrentPointNoZ(MovePoint.X, MovePoint.Y, 0.0f);
						FVector MyLocNoZ(MyLoc.X, MyLoc.Y, 0.0f);
						FVector ClosestPoint = FMath::ClosestPointOnSegment(MyLocNoZ, LastPointNoZ, CurrentPointNoZ);
						if ((ClosestPoint - MyLoc).SizeSquared2D() > FMath::Square(GetCharacter()->GetCapsuleComponent()->GetUnscaledCapsuleRadius()))
						{
							// set Z of closest point to match the XY that was previously determined
							ClosestPoint.Z = (ClosestPoint - LastPointNoZ).Size() / (CurrentPointNoZ - LastPointNoZ).Size() * (MovePoint.Z - LastReachedMovePoint.Z) + LastReachedMovePoint.Z;
							// try directly back to path center, then try backing up a bit towards path start
							FVector Side = (MovePoint - MyLoc).GetSafeNormal() ^ FVector(0.0f, 0.0f, 1.0f);
							if ((Side | (ClosestPoint - MyLoc).GetSafeNormal()) < 0.0f)
							{
								Side *= -1.0f;
							}
							FVector TestLocs[] = { ClosestPoint, ClosestPoint + (LastReachedMovePoint - ClosestPoint).GetSafeNormal() * GetCharacter()->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * 2.0f,
								MyLoc + Side * (ClosestPoint - MyLoc).Size() };
							for (int32 i = 0; i < ARRAY_COUNT(TestLocs); i++)
							{
								if (!GetWorld()->LineTraceTestByChannel(MyLoc, TestLocs[i], ECC_Pawn, Params))
								{
									SetAdjustLoc(TestLocs[i]);
									bGotAdjustLoc = true;
									break;
								}
							}
						}
					}
				}
				if (!bGotAdjustLoc)
				{
					// jump if it's a jump path or if it's a small ledge just larger than stepheight (accounts for coming at slopes/stairs from the wrong angle)
					// TODO: maybe also if chasing enemy?
					const FVector TraceHeightAdjust(0.0f, 0.0f, GetCharacter()->GetCharacterMovement()->MaxStepHeight + 10.0f);
					if (MoveTarget.IsValid() && ((CurrentPath.ReachFlags & R_JUMP) || !GetWorld()->SweepTestByChannel(MyLoc + TraceHeightAdjust, MyLoc + (MovePoint - MyLoc).GetSafeNormal2D() * GetPawn()->GetSimpleCollisionRadius() * 4.0f + TraceHeightAdjust, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(GetPawn()->GetSimpleCollisionCylinderExtent()), FCollisionQueryParams(NAME_None, false), WorldResponseParams)))
					{
						FVector Diff = MovePoint - MyLoc;
						// make sure hit wall in actual direction we should be going (commonly this check fails when multiple jumps are required and AI hasn't adjusted velocity to new direction yet)
						if (((Impact.Normal * -1.0f) | Diff.GetSafeNormal()) > 0.0f)
						{
							float XYTime = Diff.Size2D() / GetCharacter()->GetCharacterMovement()->MaxWalkSpeed;
							float DesiredJumpZ = Diff.Z / XYTime - 0.5 * GetCharacter()->GetCharacterMovement()->GetGravityZ() * XYTime;
							if (DesiredJumpZ > 0.0f)
							{
								GetCharacter()->GetCharacterMovement()->Velocity = Diff.GetSafeNormal2D() * (GetCharacter()->GetCharacterMovement()->MaxWalkSpeed * FMath::Min<float>(1.0f, DesiredJumpZ / FMath::Max<float>(GetCharacter()->GetCharacterMovement()->JumpZVelocity, 1.0f)));
							}
							GetCharacter()->GetCharacterMovement()->DoJump(false);
						}
					}
					else if (Impact.Time <= 0.0f)
					{
						ClearMoveTarget();
					}
				}
			}
		}
		else if (GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling)
		{
			FVector WallNormal2D = Impact.Normal.GetSafeNormal2D();
			if (GetCurrentPath().ReachFlags & R_JUMP)
			{
				if (GetMovePoint().Z > GetPawn()->GetActorLocation().Z && GetCharacter()->GetCharacterMovement()->Velocity.Z < 0.0f && (WallNormal2D | (GetPawn()->GetActorLocation() - GetMovePoint()).GetSafeNormal2D()) > 0.5f)
				{
					// missed jump, hit ledge we were expecting to get over
					FHitResult GroundHit;
					if (GetWorld()->SweepSingleByChannel(GroundHit, GetPawn()->GetActorLocation(), GetPawn()->GetActorLocation() - FVector(0.0f, 0.0f, 500.0f), FQuat::Identity, ECC_Pawn, GetCharacter()->GetCapsuleComponent()->GetCollisionShape(-2.0f), FCollisionQueryParams(), WorldResponseParams))
					{
						// safety underneath us, just fall
						// we don't clear MoveTimer here because if the ground is close enough it may be possible to start a new jump and reach the target, so it's reasonable to try again
						SetAdjustLoc(GroundHit.Location);
					}
					else
					{
						MoveTimer = -1.0f;
					}
				}
			}
			if (UTChar != NULL && UTChar->CanDodge() && !WallNormal2D.IsNearlyZero())
			{
				bool bDodged = false;
				if (bPlannedWallDodge)
				{
					// dodge already checked, just do it
					PendingWallDodgeDir = WallNormal2D;
				}
				// check for spontaneous wall dodge
				// if hit wall because of incoming damage momentum, add additional reaction time check
				else if ( (Skill + Personality.Jumpiness > 4.0f && UTChar->UTCharacterMovement->bIsDodging) ||
						( !UTChar->UTCharacterMovement->bIsDodging && Enemy != NULL && (GetWorld()->TimeSeconds - GetEnemyInfo(Enemy, false)->LastSeenTime < 5.0f || HasOtherVisibleEnemy()) &&
							Skill >= 3.0f && GetWorld()->TimeSeconds - UTChar->LastTakeHitTime > 2.0f - Skill * 0.2f - Personality.ReactionTime * 0.5f ) )
				{
					if (FMath::FRand() < 0.4f + 0.4f * Personality.Jumpiness && (Enemy == NULL || Impact.Actor != Enemy))
					{
						FVector Start = GetPawn()->GetActorLocation();
						// reject if on special path, unless above dest already and dodge is in its direction, or in direct combat and prefer evasiveness
						if ( (!CurrentPath.Spec.IsValid() && CurrentPath.ReachFlags == 0) || (Start.Z > GetMovePoint().Z && (WallNormal2D | (GetMovePoint() - Start).GetSafeNormal2D()) > 0.7f && (Start - GetMovePoint()).Size2D() > GetPawn()->GetSimpleCollisionRadius() * 3.0f) ||
							(Enemy != NULL && IsEnemyVisible(Enemy) && FMath::FRand() < Personality.Jumpiness) )
						{
							Start.Z += 50.0f;
							FVector DuckDir = WallNormal2D * 1000.0f; // technically not reliable since we're in air, but every once in a while a bot wall dodging off a cliff is pretty realistic
							FCollisionShape PawnShape = GetCharacter()->GetCapsuleComponent()->GetCollisionShape();
							FCollisionQueryParams Params(FName(TEXT("WallDodge")), false, GetPawn());
							float MinDist = (Personality.Jumpiness > 0.0f) ? 150.0f : 350.0f;

							FHitResult Hit;
							bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + DuckDir, FQuat::Identity, ECC_Pawn, PawnShape, Params);
							if (!bHit || (Hit.Location - Start).Size() > MinDist)
							{
								if (!bHit)
								{
									Hit.Location = Start + DuckDir;
								}
								// now check for floor
								if (GetWorld()->SweepTestByChannel(Hit.Location, Hit.Location - FVector(0.0f, 0.0f, 2.5f * GetCharacter()->GetCharacterMovement()->MaxStepHeight + GetCharacter()->GetCharacterMovement()->GetGravityZ() * -0.25f), FQuat::Identity, ECC_Pawn, PawnShape, Params))
								{
									// found one, so try the wall dodge!
									PendingWallDodgeDir = WallNormal2D;
								}
							}
						}
					}
				}
			}
		}
	}
}

void AUTBot::NotifyHitLedge()
{
	if (CurrentAction == NULL || !CurrentAction->NotifyHitLedge())
	{
		// TODO: consider jump off in some cases?
	}
}

void AUTBot::PostMovementUpdate(float DeltaTime, FVector OldLocation, FVector OldVelocity)
{
	if (!PendingWallDodgeDir.IsZero())
	{
		if (UTChar != NULL && UTChar->Dodge(PendingWallDodgeDir, (PendingWallDodgeDir ^ FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal()))
		{
			// possibly maintain readiness for a second wall dodge if we hit another wall
			bPlannedWallDodge = FMath::FRand() < Personality.Jumpiness || (Personality.Jumpiness >= 0.0 && FMath::FRand() < (Skill + Personality.Tactics) * 0.1f);
		}
		PendingWallDodgeDir = FVector::ZeroVector;
	}
}

void AUTBot::NotifyLanded(const FHitResult& Hit)
{
	bRestrictedJump = false;
	bPlannedWallDodge = false;
	if (UTChar != NULL)
	{
		UTChar->UTCharacterMovement->UpdateWallSlide(false);
	}
}

void AUTBot::NotifyJumpApex()
{
	bRestrictedJump = false;
	if (UTChar != NULL && UTChar->CanJump())
	{
		UUTReachSpec_HighJump* JumpSpec = Cast<UUTReachSpec_HighJump>(CurrentPath.Spec.Get());
		if (JumpSpec != NULL && JumpSpec->RequiredJumpZ > UTChar->GetCharacterMovement()->JumpZVelocity)
		{
			UTChar->GetCharacterMovement()->DoJump(false);
		}
		// check if missed a jump that should have been doable with normal jumpZ
		// if bot is skilled enough, let it use multi-jump to get there anyway
		else if ((CurrentPath.ReachFlags & R_JUMP) && (Skill > 4.0f || Personality.MovementAbility >= 0.5f))
		{
			FVector Diff = GetMovePoint() - UTChar->GetActorLocation();
			float XYTime = Diff.Size2D() / UTChar->GetCharacterMovement()->MaxWalkSpeed;
			float DesiredJumpZ = Diff.Z / XYTime - 0.5f * UTChar->GetCharacterMovement()->GetGravityZ() * XYTime;
			if (DesiredJumpZ > 0.0f)
			{
				// make sure really going to miss, and not just land somewhat short but able to walk the last part
				bool bNeedMultiJump = true;
				if (Diff.Z < 0.0f)
				{
					const float VelocityZ = UTChar->GetCharacterMovement()->Velocity.Z;
					const float Determinant = FMath::Square<float>(VelocityZ) -2.0f * UTChar->GetCharacterMovement()->GetGravityZ() * -Diff.Z;
					if (Determinant > 0.0f)
					{
						const float ZTime = FMath::Max<float>((-VelocityZ + FMath::Sqrt(Determinant)) / UTChar->GetCharacterMovement()->GetGravityZ(), (-VelocityZ - FMath::Sqrt(Determinant)) / UTChar->GetCharacterMovement()->GetGravityZ());
						FVector TestLoc = UTChar->GetActorLocation() + Diff.GetSafeNormal2D() * UTChar->GetCharacterMovement()->MaxWalkSpeed * ZTime;
						TestLoc.Z = GetMovePoint().Z;
						// make sure no wall that we need to get over
						static FName NotifyJumpApexTraceTag(TEXT("NotifyJumpApex"));
						if (!GetWorld()->LineTraceTestByChannel(UTChar->GetActorLocation(), TestLoc, ECC_Pawn, FCollisionQueryParams(NotifyJumpApexTraceTag, false), WorldResponseParams))
						{
							// test if projected landing is on navmesh and walk reachable
							TestLoc.Z -= UTChar->GetCharacterMovement()->MaxStepHeight; // mirrors AUTCharacter::GetNavAgentLocation()
							if (NavData->FindNearestNode(TestLoc, UTChar->GetSimpleCollisionCylinderExtent()) == CurrentPath.End)
							{
								bNeedMultiJump = false;
							}
						}
					}
				}
				if (bNeedMultiJump)
				{
					UTChar->GetCharacterMovement()->DoJump(false);
				}
			}
		}
		// maybe multi-jump for evasiveness
		else if (Enemy != NULL && IsEnemyVisible(Enemy) && ((Skill >= 3.0f && Personality.MovementAbility >= 0.0f && FMath::FRand() > 0.04f * Skill) || FMath::FRand() < Personality.Jumpiness))
		{
			// make sure won't bump head on ceiling
			const FVector MyLoc = UTChar->GetActorLocation();
			float MultiJumpZ = UTChar->UTCharacterMovement->bIsDodging ? UTChar->UTCharacterMovement->DodgeJumpImpulse : UTChar->UTCharacterMovement->MultiJumpImpulse;
			const FVector JumpTraceEnd = MyLoc + FVector(0.0f, 0.0f, MultiJumpZ * 0.5f);
			if (!GetWorld()->LineTraceTestByChannel(MyLoc, JumpTraceEnd, ECC_Pawn, FCollisionQueryParams(FName(TEXT("Jump")), false, UTChar)))
			{
				UTChar->GetCharacterMovement()->DoJump(false);
				// pick more appropriate landing spot for air control
				FVector NewLandingSpot = FVector::ZeroVector;
				float BestDistSq = FLT_MAX;
				if (MoveTarget.IsValid() && !MoveTarget.Actor.IsValid())
				{
					TArray<FVector> TestSpots;
					TestSpots.SetNumUninitialized(RouteCache.Num() + MoveTargetPoints.Num());
					for (const FRouteCacheItem& TestItem : RouteCache)
					{
						TestSpots.Add(TestItem.GetLocation(UTChar));
					}
					for (const FComponentBasedPosition& TestPoint : MoveTargetPoints)
					{
						if (TestPoint.Base == nullptr)
						{
							TestSpots.Add(TestPoint.Get());
						}
					}
					for (FVector GoalLoc : TestSpots)
					{
						float DistSq = (GoalLoc - MyLoc).SizeSquared();
						if (DistSq < BestDistSq)
						{
							FVector JumpVel;
							if ( FindBestJumpVelocityXY(JumpVel, MyLoc, GoalLoc, UTChar->GetCharacterMovement()->Velocity.Z, UTChar->GetCharacterMovement()->GetGravityZ(), UTChar->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) &&
								!GetWorld()->LineTraceTestByChannel(JumpTraceEnd, GoalLoc, ECC_Pawn, FCollisionQueryParams(FName(TEXT("Jump")), false, UTChar)) )
							{
								NewLandingSpot = GoalLoc;
								BestDistSq = DistSq;
							}
						}
					}
					if (!NewLandingSpot.IsZero())
					{
						SetMoveTargetDirect(FRouteCacheItem(NewLandingSpot));
						MoveTimer = -1.0f;
					}
				}
			}
		}
	}
}

float AUTBot::RateWeapon(AUTWeapon* W)
{
	if (W != NULL && W->GetUTOwner() == GetUTChar() && GetUTChar() != NULL && W->HasAnyAmmo())
	{
		float Rating = W->GetAISelectRating();
		// prefer favorite weapon
		if (IsFavoriteWeapon(W->GetClass()))
		{
			Rating += 0.1f;
		}
		// slightly prefer current weapon (account for weapon switch cost, avoid oscillation, etc)
		if (W == GetUTChar()->GetWeapon())
		{
			Rating += 0.05f;
		}
		return Rating;
	}
	else
	{
		return -1000.0f;
	}
}

void AUTBot::SwitchToBestWeapon()
{
	if (GetUTChar() != NULL)
	{
		float BestRating = -100.0f;
		AUTWeapon* Best = NULL;

		for (TInventoryIterator<AUTWeapon> It(GetUTChar()); It; ++It)
		{
			float NewRating = RateWeapon(*It);
			if (NewRating > BestRating)
			{
				Best = *It;
				BestRating = NewRating;
			}
		}

		GetUTChar()->SwitchWeapon(Best);
	}
}

bool AUTBot::IsFavoriteWeapon(TSubclassOf<AUTWeapon> TestClass)
{
	if (TestClass == NULL)
	{
		return false;
	}
	else
	{
		for (UClass* CurrentClass = *TestClass; CurrentClass != NULL; CurrentClass = CurrentClass->GetSuperClass())
		{
			if (CurrentClass->GetFName() == Personality.FavoriteWeapon)
			{
				return true;
			}
		}
		return false;
	}
}

bool AUTBot::WeaponProficiencyCheck()
{
	if (UTChar == NULL || UTChar->GetWeapon() == NULL)
	{
		return false;
	}
	else
	{
		float Proficiency = Skill;
		if (IsFavoriteWeapon(UTChar->GetWeapon()->GetClass()))
		{
			Proficiency += 2.0f;
		}
		return (Proficiency > 3.0f + FMath::FRand() * 3.0f);
	}
}

bool AUTBot::NeedsWeapon()
{
	if (UTChar == NULL)
	{
		return false;
	}
	else
	{
		if (UTChar->GetWeapon() == NULL)
		{
			return true;
		}
		else if (UTChar->GetWeapon()->BaseAISelectRating >= 0.5f)
		{
			return false;
		}
		else
		{
			// make sure we don't have a good weapon we just don't have selected right now due to special circumstances (e.g. translocator)
			for (TInventoryIterator<AUTWeapon> It(UTChar); It; ++It)
			{
				if (It->BaseAISelectRating >= 0.5f && It->HasAnyAmmo())
				{
					return false;
				}
			}
			return true;
		}
	}
}

void AUTBot::CheckWeaponFiring(bool bFromWeapon)
{
	if (UTChar == NULL)
	{
		GetWorldTimerManager().ClearTimer(CheckWeaponFiringTimerHandle); // timer will get restarted in Possess() if we get a new Pawn
	}
	else if (UTChar->GetWeapon() != NULL && UTChar->GetPendingWeapon() == NULL && (bFromWeapon || !UTChar->GetWeapon()->IsFiring())) // if weapon is firing, it should query bot when it's done for better responsiveness than a timer
	{
		// TODO: reimplement old 'skip firing even though can hit target' logic
		if (UTChar->GetWeapon()->ShouldAIDelayFiring())
		{
			UTChar->StopFiring();
		}
		else
		{
			AActor* TestTarget = Target;
			if (TestTarget == NULL)
			{
				// TODO: check time since last enemy loc update versus reaction time
				TestTarget = Enemy;
			}
			// TODO: if no target, ask weapon if it should fire anyway (mine layers, traps, fortifications, etc)
			// TODO: think about how to prevent Focus/Target/Enemy mismatches
			if (TestTarget != NULL && GetFocusActor() == TestTarget && bLastCanAttackSuccess && (!NeedToTurn(FinalFocalPoint) || UTChar->GetWeapon()->IsChargedFireMode(NextFireMode)))
			{
				LastFireSuccessTime = GetWorld()->TimeSeconds;
				for (uint8 i = 0; i < UTChar->GetWeapon()->GetNumFireModes(); i++)
				{
					if (i == NextFireMode)
					{
						if (!UTChar->IsPendingFire(i))
						{
							UTChar->StartFire(i);
						}
					}
					else if (UTChar->IsPendingFire(i))
					{
						UTChar->StopFire(i);
					}

					// if blew self up, abort
					if (UTChar == NULL)
					{
						break;
					}
				}
			}
			else if (TestTarget == NULL || !bFromWeapon || !UTChar->GetWeapon()->bRecommendSuppressiveFire || bStopSuppressiveWeapon || GetWorld()->TimeSeconds - LastFireSuccessTime > 1.0f)
			{
				UTChar->StopFiring();
			}
		}
		bPickNewFireMode = true;
	}
}
void AUTBot::CheckWeaponFiringTimed()
{
	CheckWeaponFiring(false);
}

bool AUTBot::NeedToTurn(const FVector& TargetLoc, bool bForcePrecise)
{
	if (GetPawn() == NULL)
	{
		return false;
	}
	else
	{
		// we're intentionally disregarding the weapon's start position here, since it may change based on its firing offset, nearby geometry if that offset is outside the cylinder, etc
		// we'll correct for the discrepancy in GetAdjustedAim() while firing
		const FVector StartLoc = GetPawn()->GetActorLocation();

		float Threshold;
		if (bForcePrecise)
		{
			Threshold = 0.997f;
		}
		else
		{
			Threshold = 0.92f + 0.01f * FMath::Clamp<float>(Skill + Personality.Accuracy, 0.0f, 7.0f);
			// be more lenient if close range, since will probably hit anyway
			Threshold *= 0.75f + (0.25f * FMath::Min<float>(1.0f, (TargetLoc - StartLoc).Size() / 512.0f));
		}

		return ((TargetLoc - StartLoc).GetSafeNormal() | GetControlRotation().Vector()) < Threshold;
	}
}

void AUTBot::StartNewAction(UUTAIAction* NewAction)
{
	if (CurrentAction != NULL)
	{
		CurrentAction->Ended(true);
	}
	CurrentAction = NewAction;
	if (CurrentAction != NULL)
	{
		CurrentAction->Started();
	}
}

bool AUTBot::CanAttack(AActor* InTarget, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8* BestFireMode, FVector* OptimalTargetLoc)
{
	if (GetUTChar() == NULL || GetUTChar()->GetWeapon() == NULL)
	{
		return false;
	}
	else
	{
		uint8 TempFireMode = 0;
		FVector TempTargetLoc = FVector::ZeroVector;
		if (BestFireMode == NULL)
		{
			BestFireMode = &TempFireMode;
		}
		if (OptimalTargetLoc == NULL)
		{
			OptimalTargetLoc = &TempTargetLoc;
		}
		return GetUTChar()->GetWeapon()->CanAttack(InTarget, TargetLoc, bDirectOnly, bPreferCurrentMode, *BestFireMode, *OptimalTargetLoc);
	}
}

bool AUTBot::CheckFutureSight(float DeltaTime)
{
	FVector FutureLoc = GetPawn()->GetActorLocation();
	if (GetCharacter() != NULL)
	{
		if (!GetCharacter()->GetCharacterMovement()->GetCurrentAcceleration().IsZero())
		{
			FutureLoc += GetCharacter()->GetCharacterMovement()->GetMaxSpeed() * DeltaTime * GetCharacter()->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal();
		}

		if (GetCharacter()->GetMovementBase() != NULL)
		{
			FutureLoc += GetCharacter()->GetMovementBase()->GetComponentVelocity() * DeltaTime;
		}
	}
	FCollisionQueryParams Params(FName(TEXT("CheckFutureSight")), false, GetPawn());
	FCollisionObjectQueryParams ResultParams(ECC_WorldStatic);
	ResultParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	//make sure won't run into something
	if (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode != MOVE_Walking && GetWorld()->LineTraceTestByObjectType(GetPawn()->GetActorLocation(), FutureLoc, ResultParams, Params))
	{
		return false;
	}
	else
	{
		// check if can still see target
		// ignore aiming adjustments that are a result of aiming for splash, lobs, etc
		const FVector PreTacticalFocalPoint = GetFocalPoint() - TacticalAimOffset;
		return !GetWorld()->LineTraceTestByObjectType(FutureLoc, PreTacticalFocalPoint + TrackedVelocity, ResultParams, Params);
	}
}

void AUTBot::SetupSpecialPathAbilities()
{
	bAllowImpactJump = (UTChar != NULL && ImpactJumpZ > 0.0f && UTChar->GetEffectiveHealthPct(false) > 0.8f && (Skill >= 4.5f || Personality.MovementAbility >= 0.5f));
	bAllowTranslocator = (bHasTranslocator && UTChar != NULL && UTChar->GetCarriedObject() == NULL && GetWorld()->TimeSeconds - LastTranslocTime > TranslocInterval && Squad->AllowTranslocator(this));
}

bool AUTBot::FindInventoryGoal(float MinWeight)
{
	if (LastFindInventoryTime == GetWorld()->TimeSeconds && LastFindInventoryWeight >= MinWeight)
	{
		return false;
	}
	else if (/*GameHasNoInventory() || */UTChar == NULL || !UTChar->bCanPickupItems)
	{
		return false;
	}
	else
	{
		LastFindInventoryTime = GetWorld()->TimeSeconds;
		LastFindInventoryWeight = MinWeight;

		FBestInventoryEval NodeEval(RespawnPredictionTime, (GetCharacter() != NULL) ? GetCharacter()->GetCharacterMovement()->MaxWalkSpeed : GetDefault<AUTCharacter>()->GetCharacterMovement()->MaxWalkSpeed, (MinWeight > 0.0f) ? FMath::TruncToInt(5.0f / MinWeight) : 0);
		// check for teammates about to pick up things and reject them from evaluation
		// unlike super pickups, we're just checking direct move goals or allies camping the pickup already;
		// desire for minor pickups change rapidly and there's not a lot of value in trying to claim far ahead
		// we're primarily just trying to avoid cases of teammates near each other (especially from a spawn room) competing for the same pickup
		// when obviously only one of them is going to get it
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (PS != nullptr && PS->Team != nullptr && GS != nullptr)
		{
			auto CheckPickup = [GS, &NodeEval](AActor* A)
			{
				if (A->IsA(AUTPickup::StaticClass()) || A->IsA(AUTDroppedPickup::StaticClass()))
				{
					if (!GS->bWeaponStay)
					{
						NodeEval.ClaimedPickups.Add(A);
					}
					else
					{
						AUTPickupInventory* ItemPickup = Cast<AUTPickupInventory>(A);
						if (ItemPickup != nullptr)
						{
							TSubclassOf<AUTWeapon> WeaponClass(*ItemPickup->GetInventoryType());
							if (WeaponClass == nullptr || !WeaponClass.GetDefaultObject()->bWeaponStay)
							{
								NodeEval.ClaimedPickups.Add(A);
							}
						}
					}
				}
			};
			for (AController* C : PS->Team->GetTeamMembers())
			{
				if (C != nullptr && C != this && C->GetPawn() != nullptr)
				{
					TSet<AActor*> Overlaps;
					GetOverlappingActors(Overlaps, AActor::StaticClass());
					for (AActor* A : Overlaps)
					{
						CheckPickup(A);
					}
					AUTBot* B = Cast<AUTBot>(C);
					if (B != nullptr && B->GetMoveTarget().Actor.IsValid())
					{
						CheckPickup(B->GetMoveTarget().Actor.Get());
					}
				}
			}
		}

		return NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), MinWeight, false, RouteCache);
	}
}

bool AUTBot::TryPathToward(AActor* Goal, bool bAllowDetours, bool bAllowPartial, const FString& SuccessGoalString)
{
	if (Goal == NULL)
	{
		return false;
	}
	else if (NavData->HasReachedTarget(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), FRouteCacheItem(Goal)))
	{
		GoalString = FString::Printf(TEXT("%s (already there, camp)"), *SuccessGoalString);
		DoCamp();
		return true;
	}
	else
	{
		if (UTChar == NULL || !UTChar->bCanPickupItems)
		{
			bAllowDetours = false;
		}
		FSingleEndpointEval NodeEval(Goal, bAllowPartial);
		float Weight = 0.0f;
		if (NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), Weight, bAllowDetours, RouteCache))
		{
			GoalString = SuccessGoalString;
			SetMoveTarget(RouteCache[0]);
			StartWaitForMove();
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool AUTBot::IsTeammate(AActor* TestActor)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	return (GS != NULL && GS->OnSameTeam(this, TestActor));
}

const TArray<const FBotEnemyInfo>& AUTBot::GetEnemyList(bool bPreferTeamList) const
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	if (bPreferTeamList && PS != NULL && PS->Team != NULL)
	{
		return PS->Team->GetEnemyList();
	}
	else
	{
		return *(TArray<const FBotEnemyInfo>*)&LocalEnemyList;
	}
}

const FBotEnemyInfo* AUTBot::GetEnemyInfo(APawn* TestEnemy, bool bCheckTeam)
{
	if (Enemy == NULL)
	{
		return NULL;
	}
	AUTPlayerState* PS = bCheckTeam ? Cast<AUTPlayerState>(PlayerState) : NULL;
	const TArray<const FBotEnemyInfo>& EnemyList = (PS != NULL && PS->Team != NULL) ? PS->Team->GetEnemyList() : *(const TArray<const FBotEnemyInfo>*)&LocalEnemyList;
	for (int32 i = 0; i < EnemyList.Num(); i++)
	{
		if (EnemyList[i].GetPawn() == TestEnemy)
		{
			return &EnemyList[i];
		}
	}
	// code assumes Enemy has a valid entry, so check local list too if necessary
	// this triggering probably means a notification bug where the AI hasn't been told about an enemy being killed or destroyed
	if (TestEnemy == Enemy && bCheckTeam && PS != NULL && PS->Team != NULL)
	{
		UE_LOG(UT, Warning, TEXT("Bot %s has enemy %s that is not in team's enemy list! (enemy dead: %s)"), *PlayerState->PlayerName, (TestEnemy->PlayerState != NULL) ? *TestEnemy->PlayerState->PlayerName : *TestEnemy->GetName(), TestEnemy->IsPendingKillPending() ? TEXT("True") : TEXT("False"));
		return GetEnemyInfo(TestEnemy, false);
	}
	return NULL;
}

FVector AUTBot::GetEnemyLocation(APawn* TestEnemy, bool bAllowPrediction)
{
	const FBotEnemyInfo* Info = GetEnemyInfo(TestEnemy, true);
	if (Info == NULL)
	{
		return FVector::ZeroVector;
	}
	// return exact loc if seen
	else if (Info->IsCurrentlyVisible(GetWorld()->TimeSeconds))
	{
		return TestEnemy->GetActorLocation();
	}
	else if (!bAllowPrediction || GetWorld()->TimeSeconds - Info->LastFullUpdateTime <= GetWorld()->GetDeltaSeconds())
	{
		return Info->LastKnownLoc;
	}
	else
	{
		// TODO:
		return Info->LastKnownLoc;
	}
}

bool AUTBot::IsEnemyVisible(APawn* TestEnemy)
{
	if (TestEnemy != nullptr)
	{
		// only use local enemies for personal visibility
		for (const FBotEnemyInfo& Info : LocalEnemyList)
		{
			if (Info.GetPawn() == TestEnemy)
			{
				return Info.IsCurrentlyVisible(GetWorld()->TimeSeconds);
			}
		}
	}
	return false;
}

bool AUTBot::HasOtherVisibleEnemy()
{
	// only use local enemies for personal visibility
	for (const FBotEnemyInfo& Info : LocalEnemyList)
	{
		if (Info.GetPawn() != Enemy && Info.IsCurrentlyVisible(GetWorld()->TimeSeconds))
		{
			return true;
		}
	}
	return false;
}

TArray<APawn*> AUTBot::GetEnemiesNear(const FVector& TestLoc, float MaxDist, bool bAllowPrediction)
{
	TArray<APawn*> Results;

	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	const TArray<const FBotEnemyInfo>& EnemyList = (PS != NULL && PS->Team != NULL) ? PS->Team->GetEnemyList() : *(const TArray<const FBotEnemyInfo>*)&LocalEnemyList;
	for (int32 i = 0; i < EnemyList.Num(); i++)
	{
		if (EnemyList[i].IsValid() && GetWorld()->TimeSeconds - EnemyList[i].LastUpdateTime < 5.0f && (TestLoc - GetEnemyLocation(EnemyList[i].GetPawn(), bAllowPrediction)).Size() <= MaxDist)
		{
			Results.Add(EnemyList[i].GetPawn());
		}
	}

	return Results;
}

float AUTBot::GetLastAnyEnemySeenTime() const
{
	float LastTime = -100.0f;
	for (const FBotEnemyInfo& EnemyInfo : LocalEnemyList)
	{
		LastTime = FMath::Max<float>(LastTime, EnemyInfo.LastSeenTime);
	}
	return LastTime;
}

bool AUTBot::LostContact(float MaxTime)
{
	if (Enemy == NULL)
	{
		return true;
	}
	else
	{
		// lose invisible enemies faster
		AUTCharacter* EnemyUTChar = Cast<AUTCharacter>(Enemy);
		if (EnemyUTChar != NULL && EnemyUTChar->IsInvisible())
		{
			MaxTime = FMath::Max<float>(2.0f, MaxTime - 2.0f);
		}

		if (GetWorld()->TimeSeconds - LastEnemyChangeTime <= MaxTime)
		{
			return false;
		}
		else
		{
			// TODO: maybe use team list if enemy is high priority?
			for (const FBotEnemyInfo& Info : LocalEnemyList)
			{
				if (Info.GetPawn() == Enemy)
				{
					return GetWorld()->TimeSeconds - Info.LastFullUpdateTime > MaxTime;
				}
			}

			return true;
		}
	}
}

void AUTBot::SetSquad(AUTSquadAI* NewSquad)
{
	if (NewSquad != NULL && NewSquad->GetTeamNum() != GetTeamNum())
	{
		UE_LOG(UT, Warning, TEXT("AUTBot::SetSquad(): NewSquad is on the wrong team!"));
	}
	else
	{
		if (Squad != NULL)
		{
			Squad->RemoveMember(this);
		}
		Squad = NewSquad;
		if (Squad != NULL)
		{
			Squad->AddMember(this);
		}
	}
}

void AUTBot::SetDefensePoint(AUTDefensePoint* NewDefensePoint)
{
	if (DefensePoint != NULL)
	{
		if (DefensePoint->CurrentUser == this)
		{
			DefensePoint->CurrentUser = NULL;
		}
		DefensePoint = NULL;
	}
	DefensePoint = NewDefensePoint;
	if (DefensePoint != NULL)
	{
		if (DefensePoint->CurrentUser != NULL)
		{
			DefensePoint->CurrentUser->SetDefensePoint(NULL);
		}
		DefensePoint->CurrentUser = this;
	}
}

void AUTBot::WhatToDoNext()
{
	ensure(!bExecutingWhatToDoNext);

	bPendingWhatToDoNext = true;
}

void AUTBot::ExecuteWhatToDoNext()
{
	DECLARE_CYCLE_STAT(TEXT("Bot decision time"), STAT_AI_ExecuteWhatToDoNext, STATGROUP_AI);

	SCOPE_CYCLE_COUNTER(STAT_AI_ExecuteWhatToDoNext);

	// generally will get replaced but this helps track unintended fallthroughs in the decision code
	GoalString = FString::Printf(TEXT("WhatToDoNext at %f"), GetWorld()->TimeSeconds);

	// remember previous MoveTarget if we failed to reach it
	// need to make sure we don't consider it a fail if the decision code gets restarted mid-move for unrelated reasons
	const FRouteCacheItem OldMoveTarget = (MoveTimer <= 0.0f && !MoveTarget.IsValid()) ? PrevMoveTarget : FRouteCacheItem();
	
	Target = NULL;
	TranslocTarget = FVector::ZeroVector;
	if (GetCharacter() != NULL)
	{
		GetCharacter()->GetCharacterMovement()->bWantsToCrouch = false;
	}
	bRestrictedJump = false;

	if (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		StartNewAction(WaitForLandingAction);
	}
	else
	{
		/* TODO: investigate value
		if ((StartleActor != None) && !StartleActor.bDeleteMe)
		{
			StartleActor.GetBoundingCylinder(StartleRadius, StartleHeight);
			if (VSize(StartleActor.Location - Pawn.Location) < StartleRadius)
			{
				Startle(StartleActor);
				return;
			}
		}*/

		// make sure enemy is valid
		if (Enemy != NULL && !FBotEnemyInfo::StaticIsValid(Enemy))
		{
			SetEnemy(NULL);
		}
		if (Enemy == NULL)
		{
			PickNewEnemy(); // note: not guaranteed to give one
		}
		else
		{
			// maybe lose enemy if haven't had any contact and isn't a high priority
			if (!Squad->MustKeepEnemy(this, Enemy) && !IsEnemyVisible(Enemy))
			{
				// decide if should lose enemy
				if (Squad->IsDefending(this))
				{
					if (LostContact(4.0f))
					{
						Squad->LostEnemy(this);
					}
				}
				else if (LostContact(7.0f))
				{
					Squad->LostEnemy(this);
				}
			}
		}

		// if we're being spawn camped but are in a protected spawn area, try to shoot enemy from here
		bool bHideInSpawnArea = false;
		if (UTChar != nullptr && !UTChar->bDamageHurtsHealth && Enemy != nullptr && UTChar->LastGameVolume != nullptr && UTChar->LastGameVolume->bIsTeamSafeVolume && IsEnemyVisible(Enemy) && CanAttack(Enemy, Enemy->GetActorLocation(), true))
		{
			GoalString = TEXT("Stay in spawn protected volume and shoot spawn campers");
			// randomly move around a little to make sure we don't get stuck
			if (FMath::FRand() < 0.25f)
			{
				const FVector Forward = UTChar->GetActorRotation().Vector();
				const FVector MoveAmt = Forward * (FMath::FRand() * -300.0f) + (Forward ^ FVector(0.0f, 0.0f, 1.0f)) * FMath::FRandRange(-500.0f, 500.0f);
				FNavLocation Dest;
				if (NavData->ProjectPoint(UTChar->GetNavAgentLocation() + MoveAmt, Dest, NavData->GetHumanPathSize().GetExtent()) && !NavData->RaycastWithZCheck(GetNavAgentLocation(), Dest.Location))
				{
					SetMoveTargetDirect(FRouteCacheItem(Dest.Location, Dest.NodeRef));
					return;
				}
			}
			DoRangedAttackOn(Enemy);
			return;
		}

		// low skill bots have a chance to get distracted by nearby visible enemy and try to fight them off
		// this helps minimize bots running away too effectively at low skill levels because their movement speed is higher than other bots' and newbie humans' ability to target them
		bool bLowSkillEnemyFocus = false;
		if (!bLeadTarget && Enemy != nullptr && IsEnemyVisible(Enemy) && (Enemy->GetActorLocation() - GetPawn()->GetActorLocation()).Size() < 2000.0f)
		{
			bLowSkillEnemyFocus = FMath::FRand() < 0.3f - (Skill * 0.1f);
		}
		if ((bLowSkillEnemyFocus || Squad == NULL || !Squad->CheckSquadObjectives(this)) && !ShouldDefendPosition())
		{
			if (Enemy != NULL && !NeedsWeapon())
			{
				ChooseAttackMode();
			}
			// if no other action, look for items
			if (CurrentAction == NULL)
			{
				if (FindInventoryGoal(0.0f))
				{
					GoalString = FString::Printf(TEXT("Wander to inventory %s"), *GetNameSafe(RouteCache.Last().Actor.Get()));
					SetMoveTarget(RouteCache[0]);
					StartNewAction(WaitForMoveAction);
				}
			}

			// FALLBACK: just wander randomly
			if (CurrentAction == NULL && GetPawn() != NULL)
			{
				GoalString = TEXT("Lost, wander randomly...");
				FRandomDestEval NodeEval;
				float Weight = 0.0f;
				if (NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), Weight, true, RouteCache) && RouteCache.Num() > 1)
				{
					SetMoveTarget(RouteCache[0]);
					StartNewAction(WaitForMoveAction);
				}
				else
				{
					// if we're on a moving platform, we might be off the navmesh due to it not handling moving things
					// try just waiting until the platform stops
					if (GetCharacter() == NULL || GetCharacter()->GetMovementBase() == NULL || GetCharacter()->GetMovementBase()->GetComponentVelocity().IsZero())
					{
						SetMoveTargetDirect(FRouteCacheItem(GetPawn()->GetActorLocation() + FMath::VRand() * FVector(500.0f, 500.0f, 0.0f)));
					}
					else
					{
						GoalString = TEXT("Wait for lift to stop moving");
					}
					StartNewAction(WaitForMoveAction);
				}
			}
		}
		else
		{
			FName Orders = Squad->GetCurrentOrders(this);
			if (Orders != AnnouncedOrders && FMath::FRand() < 0.1f)
			{
				// no attack/defend announcements in FlagRun
				if (GetWorld()->GetAuthGameMode<AUTFlagRunGame>() == nullptr)
				{
					if (Orders == NAME_Attack)
					{
						SendVoiceMessage(StatusMessage::ImOnOffense);
					}
					else if (Orders == NAME_Defend)
					{
						SendVoiceMessage(StatusMessage::ImOnDefense);
					}
				}
				AnnouncedOrders = Orders;
			}
		}

		if (OldMoveTarget.IsValid() && OldMoveTarget == MoveTarget && CurrentAction == WaitForMoveAction && GetPawn() != nullptr)
		{
			RepeatMoveFailCount++;
			if (RepeatMoveFailCount > 6)
			{
				GoalString = FString::Printf(TEXT("Give up failed move attempt, try random wander (prev: %s)"), *GoalString);
				SetMoveTargetDirect(FRouteCacheItem(GetPawn()->GetActorLocation() + FMath::VRand() * FVector(500.0f, 500.0f, 0.0f)));
			}
			else if (RepeatMoveFailCount > 3)
			{
				FUTPathLink Path;
				NavNodeRef StartPoly = NavData->FindAnchorPoly(GetPawn()->GetNavAgentLocation(), GetPawn(), GetPawn()->GetNavAgentPropertiesRef());
				if (StartPoly != INVALID_NAVNODEREF)
				{
					UUTPathNode* StartNode = NavData->GetNodeFromPoly(StartPoly);
					if (StartNode != NULL)
					{
						int32 LinkIndex = StartNode->GetBestLinkTo(StartPoly, MoveTarget, GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), NavData);
						if (LinkIndex != INDEX_NONE)
						{
							Path = StartNode->Paths[LinkIndex];
						}
					}
				}
				if (Path.IsSet() && Path.Spec.IsValid())
				{
					Path.Spec->DisableDueToFailure();
				}
				// try jumping, in case we're stuck on a ledge
				else if (GetCharacter() != nullptr)
				{
					GetCharacter()->GetCharacterMovement()->DoJump(false);
				}
			}
		}

		// check for boost powers
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		if (PS != nullptr && PS->GetRemainingBoosts() > 0 && UTChar != nullptr)
		{
			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			if (GS != nullptr)
			{
				float BestRating = 0.0f;
				int32 BestIndex = INDEX_NONE;
				int32 i = 0;
				for (TSubclassOf<AUTInventory> BoostType = GS->GetSelectableBoostByIndex(PS, i); BoostType != nullptr; i++, BoostType = GS->GetSelectableBoostByIndex(PS, i))
				{
					float NewRating = BoostType.GetDefaultObject()->GetBoostPowerRating(this);
					if (NewRating > BestRating)
					{
						BestIndex = i;
						BestRating = NewRating;
					}
				}
				float BoostPowerRatingThreshold = (PS->RemainingLives == 1) ? 0.0f : 0.49f; // TODO: more complex decision on usage threshold
				if (BestIndex != INDEX_NONE && BestRating > BoostPowerRatingThreshold)
				{
					PS->ServerSetBoostItem(BestIndex);
					UTChar->TriggerBoostPower();
				}
			}
		}
	}
}

void AUTBot::SendVoiceMessage(FName MessageName)
{
	float MinInterval = (MessageName == StatusMessage::NeedBackup) ? 45.0f : 20.0f;
	float* LastTime = LastVoiceMessageTime.Find(MessageName);
	if (LastTime == NULL || GetWorld()->TimeSeconds - *LastTime > MinInterval)
	{
		LastVoiceMessageTime.Add(MessageName, GetWorld()->TimeSeconds);

		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		if (PS != NULL)
		{
			PS->AnnounceStatus(MessageName);
		}
	}
}

void AUTBot::Say(const FString& Msg, bool bTeam)
{
	if (!bTeam || GetWorld()->GetAuthGameMode<AUTTeamGameMode>() != NULL)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTBasePlayerController* UTPC = Cast<AUTBasePlayerController>(*Iterator);
			if (UTPC != NULL)
			{
				if (!bTeam || UTPC->GetTeamNum() == GetTeamNum())
				{
					// Dont send spectator chat to players
					if (UTPC->PlayerState != NULL)
					{
						UTPC->ClientSay(Cast<AUTPlayerState>(PlayerState), Msg, (bTeam ? ChatDestinations::Team : ChatDestinations::Local));
					}
				}
			}
		}
	}
}

bool AUTBot::IsSniping() const
{
	return ( DefensePoint != NULL && DefensePoint->bSniperSpot && GetUTChar() != NULL && GetUTChar()->GetWeapon() != NULL && GetUTChar()->GetWeapon()->bSniping &&
			NavData != NULL && NavData->HasReachedTarget(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), FRouteCacheItem(DefensePoint)) );
}

void AUTBot::ChooseAttackMode()
{
	float EnemyStrength = RelativeStrength(Enemy);
	AUTWeapon* MyWeap = (GetUTChar() != NULL) ? GetUTChar()->GetWeapon() : NULL;

	// send under attack voice message if under duress
	if (EnemyStrength > 0.0f && FMath::FRand() < 0.25f)
	{
		SendVoiceMessage((GetUTChar() != NULL && GetUTChar()->GetCarriedObject() != NULL) ? StatusMessage::DefendFC : StatusMessage::NeedBackup);
	}
	/*if (Vehicle(Pawn) != None)
	{
		VehicleFightEnemy(true, EnemyStrength);
	}
	else
	*/
	{
		// always retreat if enemy is spawn protected
		AUTCharacter* EnemyChar = Cast<AUTCharacter>(Enemy);
		if (EnemyChar != nullptr && EnemyChar->LastGameVolume != nullptr && EnemyChar->LastGameVolume->bIsTeamSafeVolume && IsEnemyVisible(EnemyChar))
		{
			GoalString = TEXT("Retreat because enemy is in spawn volume");
			DoRetreat();
			return;
		}
		else if (/*!bFrustrated && */!Squad->MustKeepEnemy(this, Enemy))
		{
			float RetreatThreshold = Personality.Aggressiveness;
			if (!NeedsWeapon())
			{
				// low skill bots default to not retreating
				RetreatThreshold += FMath::Max<float>(0.0f, 0.35f - Skill * 0.05f);
			}
			if (EnemyStrength > RetreatThreshold)
			{
				GoalString = TEXT("Retreat");
				// send retreating voice message
				if (FMath::FRand() < 0.05f)
				{
					SendVoiceMessage(StatusMessage::NeedBackup);
				}
				DoRetreat();
				return;
			}
		}

		if (!Squad->HasHighPriorityObjective(this) && Skill + Personality.Tactics > 2.0f && (EnemyStrength > -0.3f || NeedsWeapon()))
		{
			float WeaponRating;
			if (MyWeap == NULL)
			{
				WeaponRating = 0.0f;
			}
			else if (NeedsWeapon())
			{
				WeaponRating = (EnemyStrength > 0.3f) ? 0.0f : (MyWeap->GetAISelectRating() / 4000.0f);
			}
			else
			{
				WeaponRating = MyWeap->GetAISelectRating() / ((EnemyStrength > 0.3f) ? 4000.0f : 2000.0f);
			}

			// fallback to better pickup?
			if (FindInventoryGoal(WeaponRating))
			{
				GoalString = FString::Printf(TEXT("Fallback to better pickup %s"), *GetNameSafe(RouteCache.Last().Actor.Get()));
				//GotoState('FallBack');
				SetMoveTarget(RouteCache[0]);
				StartNewAction(WaitForMoveAction); // TODO: FallbackAction?
				return;
			}
		}

		GoalString = "ChooseAttackMode FightEnemy";
		FightEnemy(true, EnemyStrength);
	}
}

void AUTBot::FightEnemy(bool bCanCharge, float EnemyStrength)
{
	/*if (Vehicle(Pawn) != None)
	{
		VehicleFightEnemy(bCanCharge, EnemyStrength);
		return;
	}*/
	/*if (Pawn.IsInPain() && FindInventoryGoal(0.0))
	{
		GoalString = "Fallback out of pain volume " $ RouteGoal $ " hidden " $ RouteGoal.bHidden;
		GotoState('FallBack');
		return;
	}*/
	AUTWeapon* MyWeap = (GetUTChar() != NULL) ? GetUTChar()->GetWeapon() : NULL;
	if (MyWeap == NULL)
	{
		if (FindInventoryGoal(0.0f))
		{
			SetMoveTarget(RouteCache[0]);
			//GotoState('FallBack');
			StartNewAction(WaitForMoveAction);
		}
	}
	/*else if ( (Enemy == FailedHuntEnemy) && (WorldInfo.TimeSeconds == FailedHuntTime) )
	{
		GoalString = "FAILED HUNT - HANG OUT";
		if ( LineOfSightTo(Enemy) )
			bCanCharge = false;
		else if ( FindInventoryGoal(0) )
		{
			SetAttractionState();
			return;
		}
		else
		{
			WanderOrCamp();
			return;
		}
	}*/
	else
	{
		bool bOldForcedCharge = false;// bMustCharge;
		//bMustCharge = false;
		const FVector EnemyLoc = GetEnemyLocation(Enemy, true);
		float EnemyDist = (GetPawn()->GetActorLocation() - EnemyLoc).Size();
		bool bFarAway = false;
		// TODO: used to use CombatStyle here... seems almost the same as Aggressiveness, maybe don't need both
		float AdjustedCombatStyle = Personality.Aggressiveness + MyWeap->SuggestAttackStyle();
		CurrentAggression = 1.5f * FMath::FRand() - 0.8f + 2.0f * AdjustedCombatStyle - 0.5 * EnemyStrength
								+ FMath::FRand() * ((Enemy->GetVelocity() - GetPawn()->GetVelocity()).GetSafeNormal() | (EnemyLoc - GetPawn()->GetActorLocation()).GetSafeNormal());
		if (GetDefensePoint() != nullptr)
		{
			CurrentAggression -= 0.5f;
		}
		AUTWeapon* EnemyWeap = (Cast<AUTCharacter>(Enemy) != NULL) ? ((AUTCharacter*)Enemy)->GetWeapon() : NULL;
		if (EnemyWeap != NULL)
		{
			CurrentAggression += 2.0f * EnemyWeap->SuggestDefenseStyle();
		}
		//if (enemyDist > MAXSTAKEOUTDIST)
		//	Aggression += 0.5;
		Squad->ModifyAggression(this, CurrentAggression);
		if (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement() != NULL && (GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Walking || GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling))
		{
			float ZDiff = GetPawn()->GetActorLocation().Z - EnemyLoc.Z;
			if (ZDiff > TacticalHeightAdvantage)
			{
				CurrentAggression = FMath::Max<float>(0.0f, CurrentAggression - 1.0f + AdjustedCombatStyle);
			}
			else if ((Skill < 4.0f || Personality.Aggressiveness >= 0.5f) && EnemyDist > 3000.0f)
			{
				bFarAway = true;
				CurrentAggression += 0.5;
			}
			else if (ZDiff < -GetPawn()->GetSimpleCollisionHalfHeight())  // below enemy
			{
				// unless really aggressive, don't try to charge enemy with substantial height advantage
				if (ZDiff < -TacticalHeightAdvantage && Personality.Aggressiveness < 0.7f)
				{
					CurrentAggression += Personality.Aggressiveness * 0.5f;
				}
				else
				{
					CurrentAggression += Personality.Aggressiveness;
				}
			}
		}

		const bool bCanAttack = CanAttack(Enemy, EnemyLoc, true); // TODO: maybe not bDirectOnly = true? Unsure if best to check for indirect attacks from here
		// CanAttack() may call into script so we can't make assumptions
		if (GetPawn() == NULL)
		{
			return;
		}
		if (!bCanAttack)
		{
			if (Squad->MustKeepEnemy(this, Enemy))
			{
				GoalString = "Hunt priority enemy";
				DoHunt(Enemy);
			}
			else if (!bCanCharge)
			{
				GoalString = "Stake Out - no charge";
				//DoStakeOut();
				DoRangedAttackOn(Enemy); // FIXME temp replacement
			}
			else if (Squad->IsDefending(this) && LostContact(4.0f) && CanAttack(Enemy, EnemyLoc, false))
			{
				GoalString = FString::Printf(TEXT("Stake Out %s"), *GetEnemyInfo(Enemy, false)->LastSeenLoc.ToString());
				//DoStakeOut();
				DoRangedAttackOn(Enemy); // FIXME temp replacement
			}
			else if (((CurrentAggression < 1.0f && !LostContact(3.0f + 2.0f * FMath::FRand())) || IsSniping()) && GetWorld()->TimeSeconds - GetEnemyInfo(Enemy, false)->LastSeenTime < 2.5f + FMath::Max<float>(-1.0f, 3.0f * (FMath::FRand() + 2.0f * (EnemyStrength - CurrentAggression))))
			{
				GoalString = "Stake Out2";
				//DoStakeOut();
				DoRangedAttackOn(Enemy); // FIXME temp replacement
			}
			else if ( Skill + Personality.Tactics >= 3.5f + FMath::FRand() && !LostContact(1.0f) /*&& VSize(EnemyLoc - GetPawn()->GetActorLocation()) < MAXSTAKEOUTDIST*/ &&
				!NeedsWeapon() && !MyWeap->bMeleeWeapon &&
				FMath::FRand() < 0.75f && !IsEnemyVisible(Enemy) && (Enemy->Controller == NULL || !Enemy->Controller->LineOfSightTo(GetPawn())) &&
				!HasOtherVisibleEnemy() )
			{
				GoalString = "Stake Out 3";
				//DoStakeOut();
				DoRangedAttackOn(Enemy); // FIXME temp replacement
			}
			else
			{
				GoalString = "Hunt";
				DoHunt(Enemy);
			}
		}
		else
		{
			// see enemy - decide whether to charge it or strafe around/stand and fire
			SetFocus(Enemy);

			if (MyWeap->bMeleeWeapon || (bCanCharge && bOldForcedCharge))
			{
				GoalString = "Charge";
				DoCharge();
			}
			/*else if (MyWeap->RecommendLongRangedAttack())
			{
			GoalString = "Long Ranged Attack";
			DoRangedAttackOn(Enemy);
			}*/
			else if (bCanCharge && (Skill < 5.0f || Personality.Aggressiveness >= 0.5f) && bFarAway && CurrentAggression > 1.0f && FMath::FRand() < 0.5)
			{
				GoalString = "Charge closer";
				DoCharge();
			}
			else if (MyWeap->bPrioritizeAccuracy || IsSniping() || (FMath::FRand() > 0.17f * (Skill + Personality.Tactics - 1.0f)/* && !DefendMelee(EnemyDist)*/))
			{
				GoalString = "Ranged Attack";
				DoRangedAttackOn(Enemy);
			}
			else if (bCanCharge && CurrentAggression > 1.0f)
			{
				GoalString = "Charge 2";
				DoCharge();
			}
			else
			{
				GoalString = "Do tactical move";
				if (!MyWeap->bRecommendSplashDamage && FMath::FRand() < 0.7f && 3.0f * Personality.Jumpiness + FMath::FRand() * Skill > 3.0f)
				{
					GoalString = "Try to Duck";
					FVector Y = FRotationMatrix(GetControlRotation()).GetScaledAxis(EAxis::Y);
					if (FMath::FRand() < 0.5f)
					{
						TryEvasiveAction(Y * -1.0f);
					}
					else
					{
						TryEvasiveAction(Y);
					}
				}
				DoTacticalMove();
			}
		}
	}
}

void AUTBot::DoRetreat()
{
	if (Squad->PickRetreatDestination(this))
	{
		//GotoState('Retreating');
		//StartNewAction(RetreatAction);
		StartNewAction(WaitForMoveAction);
	}
	// if nothing, then tactical move
	else if (LineOfSightTo(Enemy))
	{
		//GoalString = "No retreat because frustrated";
		//bFrustrated = true;
		if (GetUTChar() != NULL && GetUTChar()->GetWeapon() != NULL && GetUTChar()->GetWeapon()->bMeleeWeapon)
		{
			DoCharge();
		}
		/*else if (Vehicle(Pawn) != None)
		{
			GotoState('VehicleCharging');
		}*/
		else
		{
			DoTacticalMove();
		}
	}
	else
	{
		//GoalString = "Stakeout because no retreat dest";
		//DoStakeOut();
	}
}

void AUTBot::DoCharge()
{
	if (GetTarget() == NULL)
	{
		UE_LOG(UT, Warning, TEXT("AI ERROR: %s got into DoCharge() with no target!"), *(PlayerState != NULL ? PlayerState->PlayerName : GetName()));
		DoTacticalMove();
	}
	else
	{
		if (GetTarget() == Enemy && !NavData->RaycastWithZCheck(GetPawn()->GetNavAgentLocation(), Enemy->GetNavAgentLocation()))
		{
			SetMoveTargetDirect(FRouteCacheItem(Enemy));
			StartNewAction(ChargeAction);
		}
		else
		{
			float Weight = 0.0f;
			FSingleEndpointEval NodeEval(GetTarget());
			NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), Weight, true, RouteCache);
			if (RouteCache.Num() > 0)
			{
				SetMoveTarget(RouteCache[0]);
				StartNewAction(ChargeAction);
			}
			else
			{
				DoTacticalMove();
			}
		}
	}
}

void AUTBot::DoTacticalMove()
{
	/*if (!Pawn.bCanStrafe)
	{
		if (Pawn.HasRangedAttack())
			DoRangedAttackOn(Enemy);
		else
			WanderOrCamp();
	}
	else
	*/
	{
		StartNewAction(TacticalMoveAction);
	}
}

void AUTBot::DoRangedAttackOn(AActor* NewTarget)
{
	Target = NewTarget;

	// leave vehicle if it's not useful for shooting things
	/*V = UTVehicle(Pawn);
	if (V != None && V.bShouldLeaveForCombat)
	{
		LeaveVehicle(false);
	}*/

	SetFocus(Target);
	StartNewAction(RangedAttackAction);
}

void AUTBot::DoHunt(APawn* NewHuntTarget)
{
	if (NewHuntTarget == NULL)
	{
		NewHuntTarget = Enemy;
	}
	if (NewHuntTarget == NULL || !FBotEnemyInfo::StaticIsValid(NewHuntTarget) || GetEnemyInfo(NewHuntTarget, false) == NULL)
	{
		AUTCharacter* UTC = Cast<AUTCharacter>(NewHuntTarget);
		UE_LOG(UT, Warning, TEXT("Bot %s in DoHunt() with no or invalid enemy %s (flag %s)"), *PlayerState->PlayerName, (NewHuntTarget != NULL && NewHuntTarget->PlayerState != NULL) ? *NewHuntTarget->PlayerState->PlayerName : *GetNameSafe(NewHuntTarget), *GetNameSafe((UTC != NULL) ? UTC->GetCarriedObject() : NULL));
		GoalString = TEXT("BUG - HUNT WITH BAD TARGET - Force CampAction");
		StartNewAction(CampAction);
	}
	else if (GetPawn() == NULL)
	{
		UE_LOG(UT, Warning, TEXT("Bot %s in DoHunt() with no Pawn"), *PlayerState->PlayerName);
	}
	else
	{
		if (HuntingTarget != NewHuntTarget)
		{
			HuntingCheckedSpots.Empty();
			HuntingInterceptNode = NULL;
			HuntInterceptFailedTime = -100000.0f;
		}
		const FBotEnemyInfo EnemyInfo = *GetEnemyInfo(NewHuntTarget, true);
		TArray<FPredictedGoal> CheckSpots;
		Squad->GetPossibleEnemyGoals(this, &EnemyInfo, CheckSpots);
		// eliminate spots we've checked already or have visibility to now
		TArray<FPredictedGoal> RemainingSpots = CheckSpots;
		for (int32 i = RemainingSpots.Num() - 1; i >= 0; i--)
		{
			bool bRemoved = false;
			for (const FVector& CheckedSpot : HuntingCheckedSpots)
			{
				if ((CheckedSpot - RemainingSpots[i].Location).Size() < GetPawn()->GetSimpleCollisionRadius() + GetPawn()->GetSimpleCollisionHalfHeight())
				{
					RemainingSpots.RemoveAt(i);
					bRemoved = true;
					break;
				}
			}
			static FName DoHuntTraceTag(TEXT("DoHuntTrace"));
			if ( !bRemoved && ( NavData->HasReachedTarget(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), FRouteCacheItem(RemainingSpots[i].Location)) ||
								!GetWorld()->LineTraceTestByChannel(GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, GetPawn()->BaseEyeHeight), RemainingSpots[i].Location + FVector(0.0f, 0.0f, GetPawn()->BaseEyeHeight), ECC_Visibility, FCollisionQueryParams(DoHuntTraceTag, true), WorldResponseParams) ) )
			{
				HuntingCheckedSpots.Add(RemainingSpots[i].Location);
				RemainingSpots.RemoveAt(i);
			}
		}
		TArray<FRouteCacheItem> HuntEndpoints;
		if (RemainingSpots.Num() == 0 && GetWorld()->TimeSeconds - EnemyInfo.LastFullUpdateTime < 2.0f && (EnemyInfo.LastKnownLoc - GetPawn()->GetActorLocation()).Size() > GetPawn()->GetSimpleCollisionRadius() * 4.0f)
		{
			// we know where the enemy is or was recently, just go with that for now
			NavNodeRef Poly = NavData->FindAnchorPoly(EnemyInfo.LastKnownLoc, EnemyInfo.GetPawn(), EnemyInfo.GetPawn()->GetNavAgentPropertiesRef());
			if (Poly == INVALID_NAVNODEREF)
			{
				// enemy may be jumping, etc so try tracing to ground
				// note: navmesh raycasts are sadly 2D only so we can't trace against the mesh, have to use world geo first
				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, EnemyInfo.LastKnownLoc, EnemyInfo.LastKnownLoc - FVector(0.0f, 0.0f, 10000.0f), ECC_Pawn, FCollisionQueryParams(), WorldResponseParams))
				{
					Poly = NavData->FindAnchorPoly(Hit.Location, EnemyInfo.GetPawn(), EnemyInfo.GetPawn()->GetNavAgentPropertiesRef());
				}
			}
			if (Poly != INVALID_NAVNODEREF)
			{
				new(HuntEndpoints) FRouteCacheItem(NavData->GetNodeFromPoly(Poly), EnemyInfo.LastKnownLoc, Poly);
			}
		}
		else
		{
			if (RemainingSpots.Num() == 0)
			{
				// start over
				RemainingSpots = CheckSpots;
				HuntingCheckedSpots.Empty();
			}
			if (RemainingSpots.Num() == 0)
			{
				// use existing route goal if we have one
				if (RouteCache.Num() > 1 && !NavData->HasReachedTarget(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), RouteCache.Last()))
				{
					HuntEndpoints.Add(RouteCache.Last());
				}
				else
				{
					// pick a random point for now
					FRandomDestEval NodeEval;
					float Weight = 0.0f;
					if (NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), Weight, true, RouteCache))
					{
						HuntEndpoints.Add(RouteCache.Last());
					}
				}
			}
			else if (EnemyInfo.LastFullUpdateTime > HuntInterceptFailedTime)
			{
				// calculate minimum amount of time enemy will have progressed from our starting point before we could possibly catch up
				// note: this is an optimization, more correct would be to evaluate in the below pathfinding step
				const float MoveSpeed = FMath::Max<float>(1.0f, GetPawn()->GetMovementComponent() ? GetPawn()->GetMovementComponent()->GetMaxSpeed() : 0);
				const float DistFromEnemy = (GetPawn()->GetActorLocation() - EnemyInfo.LastKnownLoc).Size();
				const float TimeSinceEnemyUpdate = GetWorld()->TimeSeconds - EnemyInfo.LastFullUpdateTime;

				TArray<FRouteCacheItem> PathPredictionGoals;
				for (const FPredictedGoal& TestSpot : CheckSpots)
				{
					NavNodeRef Poly = NavData->UTFindNearestPoly(TestSpot.Location, NavData->GetPOIExtent(EnemyInfo.GetPawn()));
					if (Poly != INVALID_NAVNODEREF)
					{
						FRouteCacheItem NewItem(NavData->GetNodeFromPoly(Poly), TestSpot.Location, Poly);
						// for enemy path prediction include any point that we could prevent them from reaching (using straight line distance for simplicity and performance)
						// or that are critical so we have to try even if it's probably hopeless
						const float DistFromGoal = (TestSpot.Location - GetPawn()->GetActorLocation()).Size();
						const float SkipTime = FMath::Max<float>(TimeSinceEnemyUpdate, FMath::Min<float>(DistFromEnemy, DistFromGoal) / MoveSpeed);
						if (TestSpot.bCritical || (TestSpot.Location - EnemyInfo.LastKnownLoc).Size() > MoveSpeed * SkipTime)
						{
							PathPredictionGoals.Add(NewItem);
						}
						// include as our possible endpoint goals that the enemy may be headed to
						if (RemainingSpots.Contains(TestSpot))
						{
							HuntEndpoints.Add(NewItem);
						}
					}
				}
				float DistFromEnemyOrGoal = DistFromEnemy;
				for (const FRouteCacheItem& TestGoal : PathPredictionGoals)
				{
					DistFromEnemyOrGoal = FMath::Min<float>(DistFromEnemy, (TestGoal.GetLocation(GetPawn()) - GetPawn()->GetActorLocation()).Size());
				}
				float SkipTime = FMath::Max<float>(TimeSinceEnemyUpdate, DistFromEnemyOrGoal / MoveSpeed);
				// pathfind as the target towards any of the predicted goals
				// add the path found to the list of intercept endpoints
				FMultiPathNodeEval NodeEval(PathPredictionGoals);
				float Weight = 0.0f;
				TArray<FRouteCacheItem> EnemyRouteCache;
				TArray<int32> EnemyRouteCosts;
				if (NavData->FindBestPath(EnemyInfo.GetPawn(), EnemyInfo.GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, EnemyInfo.LastKnownLoc, Weight, false, EnemyRouteCache, &EnemyRouteCosts))
				{
					// remove those points that the enemy is predicted to have or will have passed by the time we could get there
					
					// the last item is the target we already have, so ignore that
					FRouteCacheItem Goal = EnemyRouteCache.Last();
					EnemyRouteCache.Pop();
					EnemyRouteCosts.Pop();
					for (int32 i = 0; i < EnemyRouteCosts.Num(); i++)
					{
						SkipTime -= float(EnemyRouteCosts[i]) / MoveSpeed;
						if (SkipTime <= 0.0f)
						{
							// if we're already on the enemy's predicted route, close in by moving to the start point
							const UUTPathNode* Node = NavData->GetNodeFromPoly(NavData->FindAnchorPoly(GetPawn()->GetNavAgentLocation(), GetPawn(), GetPawn()->GetNavAgentPropertiesRef()));
							bool bOnEnemyRoute = (Node != NULL && EnemyRouteCache.ContainsByPredicate([Node](const FRouteCacheItem& TestItem) { return TestItem.Node == Node; }));
							if (!bOnEnemyRoute && HuntingInterceptNode != NULL && EnemyRouteCache.ContainsByPredicate([this](const FRouteCacheItem& TestItem) { return TestItem.Node == HuntingInterceptNode; }))
							{
								Node = HuntingInterceptNode;
								bOnEnemyRoute = true;
							}
							if (bOnEnemyRoute)
							{
								// if we're on the enemy's point 0, then they are either ahead of us or took a different path, so just head to our guess of their next goal
								if (EnemyRouteCache[0].Node == Node)
								{
									HuntingInterceptNode = NULL;
									HuntInterceptFailedTime = GetWorld()->TimeSeconds;
								}
								else
								{
									// if already on point we predicted enemy to be at, follow it up to their start point (might still be ahead of them if they are slower than anticipated)
									HuntingInterceptNode = Node;
									bool bSetInterceptPoint = false;
									for (int32 j = i; j >= 1; j--)
									{
										if (EnemyRouteCache[j].Node == Node)
										{
											HuntEndpoints.Reset();
											HuntEndpoints.Add(EnemyRouteCache[j - 1]);
											bSetInterceptPoint = true;
										}
									}
									if (!bSetInterceptPoint)
									{
										// add anticipated intercept point
										HuntEndpoints.Reset();
										HuntEndpoints.Add((i > 0 && EnemyRouteCache[i].Node != nullptr && EnemyRouteCache[i].Node->bDestinationOnly) ? EnemyRouteCache[i - 1] : EnemyRouteCache[i]);
									}
								}
							}
							else
							{
								HuntEndpoints.RemoveAll([&](const FRouteCacheItem& TestItem) { return (TestItem.GetLocation(NULL) - Goal.GetLocation(NULL)).IsNearlyZero(); });
								const FVector PredictedEnemyLoc = EnemyRouteCache[i].GetLocation(GetPawn());
								const float PredictedEnemyDistSq = (PredictedEnemyLoc - GetPawn()->GetActorLocation()).SizeSquared();
								// add all points along the enemy's route that are between the enemy's predicted location and our current location
								// (minimize backtracking to get on the intercept path)
								for (int32 j = i; j < EnemyRouteCache.Num() && (EnemyRouteCache[j].GetLocation(GetPawn()) - PredictedEnemyLoc).SizeSquared() < PredictedEnemyDistSq; j++)
								{
									if (!EnemyRouteCache[j].Node->bDestinationOnly)
									{
										HuntEndpoints.Add(EnemyRouteCache[j]);
									}
								}
							}
							break;
						}
					}
				}
			}
		}
		if (HuntEndpoints.Num() > 0)
		{
			HuntingTarget = NewHuntTarget;
			// path to first found possible enemy location
			FMultiPathNodeEval NodeEval(HuntEndpoints);
			float Weight = 0.0f;
			if (NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, GetPawn()->GetNavAgentLocation(), Weight, true, RouteCache))
			{
				SetMoveTarget(RouteCache[0]);
				StartNewAction(ChargeAction); // TODO: hunting action
			}
			else
			{
				HuntingTarget = NULL;
				HuntingCheckedSpots.Empty();
				DoTacticalMove();
			}
		}
		else
		{
			HuntingTarget = NULL;
			HuntingCheckedSpots.Empty();
			// this function is not supposed to fail so just do a combat move
			if (Enemy == NULL)
			{
				SetEnemy(NewHuntTarget);
			}
			DoTacticalMove();
		}
	}
}

float AUTBot::RelativeStrength(APawn* Other)
{
	const FBotEnemyInfo* Info = GetEnemyInfo(Other, true);
	if (Info == NULL)
	{
		return 0.0f;
	}
	else
	{
		// TODO: account for implicit strength of pawn class (relevant for vehicles - 100% tank health isn't the same as 100% human health)
		float Relation = Info->EffectiveHealthPct - ((UTChar != NULL) ? UTChar->GetEffectiveHealthPct(false) : 1.0f);

		if (UTChar != NULL && UTChar->GetWeapon() != NULL)
		{
			Relation -= 0.5f * UTChar->DamageScaling * UTChar->GetFireRateMultiplier() * UTChar->GetWeapon()->GetAISelectRating();
			if (UTChar->GetWeapon()->BaseAISelectRating < 0.5f)
			{
				Relation += 0.3f;
				if (Info->GetUTChar() != NULL && Info->GetUTChar()->GetWeapon() != NULL && Info->GetUTChar()->GetWeapon()->BaseAISelectRating > 0.5f)
				{
					Relation += 0.3f;
				}
			}
		}
		if (Info->GetUTChar() != NULL && Info->GetUTChar()->GetWeapon() != NULL)
		{
			Relation += 0.5f * Info->GetUTChar()->DamageScaling * Info->GetUTChar()->GetFireRateMultiplier() * Info->GetUTChar()->GetWeapon()->BaseAISelectRating;
		}

		if (GetWorld()->TimeSeconds - Info->LastFullUpdateTime < 10.0f)
		{
			if (Info->LastKnownLoc.Z > GetPawn()->GetActorLocation().Z + TacticalHeightAdvantage)
			{
				Relation += 0.2f;
			}
			else if (GetPawn()->GetActorLocation().Z > Info->LastKnownLoc.Z + TacticalHeightAdvantage)
			{
				Relation -= 0.15;
			}
		}

		return Relation;
	}
}

bool AUTBot::ShouldDefendPosition()
{
	return false; // TODO
}

bool AUTBot::IsAcceptableTranslocation(const FVector& TeleportLoc, const FVector& DesiredDest)
{
	float DownDist = FMath::Max<float>(500.0f, TeleportLoc.Z - DesiredDest.Z + 100.0f);
	FCollisionShape TestShape = FCollisionShape::MakeCapsule(GetPawn()->GetSimpleCollisionCylinderExtent() * 0.5f);

	FCollisionQueryParams Params(FName(TEXT("TransDiskAI")), false, GetPawn());
	if (!GetWorld()->SweepTestByChannel(TeleportLoc, TeleportLoc - FVector(0.0f, 0.0f, DownDist), FQuat::Identity, ECC_Pawn, TestShape, Params))
	{
		return false;
	}
	else
	{
		// if we're moving, our velocity will be maintained, so check for ground in that direction as well
		FVector MyVel = GetPawn()->GetVelocity();
		MyVel.Z = 0.0f;
		MyVel *= 0.25f;
		FVector ForwardLoc = TeleportLoc;
		if (!MyVel.IsNearlyZero())
		{
			ForwardLoc += MyVel;
			FHitResult Hit;
			if (GetWorld()->SweepSingleByChannel(Hit, TeleportLoc, ForwardLoc, FQuat::Identity, ECC_Pawn, TestShape, Params))
			{
				ForwardLoc = Hit.Location + Hit.Normal;
			}
		}
		if ((ForwardLoc - TeleportLoc).SizeSquared2D() > 2.0f && !GetWorld()->SweepTestByChannel(ForwardLoc, ForwardLoc - FVector(0.0f, 0.0f, DownDist), FQuat::Identity, ECC_Pawn, TestShape, Params))
		{
			return false;
		}
		else if (!GetWorld()->LineTraceTestByChannel(TeleportLoc, DesiredDest, ECC_Pawn, Params))
		{
			return true;
		}
		else
		{
			// see if we can get there from here via walk or jump
			if (!NavData->RaycastWithZCheck(TeleportLoc, DesiredDest - FVector(0.0f, 0.0f, GetPawn()->GetSimpleCollisionHalfHeight())))
			{
				return true;
			}
			else
			{
				NavNodeRef TeleportPoly = NavData->UTFindNearestPoly(TeleportLoc, GetPawn()->GetSimpleCollisionCylinderExtent());
				if (TeleportPoly != INVALID_NAVNODEREF)
				{
					UUTPathNode* Node = NavData->GetNodeFromPoly(TeleportPoly);
					if (Node != NULL)
					{
						FVector AdjustedDest = DesiredDest;
						NavNodeRef DestPoly = NavData->UTFindNearestPoly(AdjustedDest, GetPawn()->GetSimpleCollisionCylinderExtent());
						if (DestPoly == INVALID_NAVNODEREF)
						{
							AdjustedDest.Z -= GetPawn()->GetSimpleCollisionHalfHeight();
							DestPoly = NavData->UTFindNearestPoly(AdjustedDest, GetPawn()->GetSimpleCollisionCylinderExtent());
						}
						if (DestPoly != INVALID_NAVNODEREF)
						{
							FRouteCacheItem NewTarget(NavData->GetNodeFromPoly(DestPoly), AdjustedDest, DestPoly);
							return (NewTarget.Node.IsValid() && Node->GetBestLinkTo(TeleportPoly, NewTarget, GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), NavData) != INDEX_NONE);
						}
					}
				}
				return false;
			}
		}
	}
}

EBotMonitoringStatus AUTBot::ShouldTriggerTranslocation(const FVector& CurrentDest, const FVector& DestVelocity)
{
	if (GetPawn() == NULL)
	{
		return BMS_Abort;
	}
	else if (!TranslocTarget.IsZero())
	{
		FVector Diff = TranslocTarget - CurrentDest;
		if ( (Diff.Size2D() < FMath::Max<float>(DestVelocity.Size() * 0.04f, 120.0f) || (TranslocTarget - (CurrentDest + DestVelocity * GetWorld()->DeltaTimeSeconds)).Size2D() > Diff.Size2D()) &&
			IsAcceptableTranslocation(CurrentDest, TranslocTarget) )
		{
			// translocate!
			return BMS_Activate;
		}
		else if ((Diff.GetSafeNormal2D() | DestVelocity.GetSafeNormal2D()) <= 0.0f || (Diff.Z > 0.0f && DestVelocity.Z <= 0.0f))
		{
			// check if disk ended up further along bot's path even though not at desired destination
			if ((GetMovePoint() - CurrentDest).Size() < (GetMovePoint() - GetPawn()->GetActorLocation()).Size() * 0.75f && IsAcceptableTranslocation(CurrentDest, GetMovePoint()))
			{
				return BMS_Activate;
			}
			else
			{
				if (MoveTargetPoints.Num() > 1)
				{
					for (int32 i = 1; i < MoveTargetPoints.Num(); i++)
					{
						// don't test Z; we'll trace for fall/climb reachability in IsAcceptableTranslocation()
						FVector PrevLocNoZ = MoveTargetPoints[i - 1].Get();
						PrevLocNoZ.Z = 0.0f;
						const FVector TestLoc = (i == MoveTargetPoints.Num() - 1) ? MoveTarget.GetLocation(GetPawn()) : MoveTargetPoints[i].Get();
						const FVector TestLocNoZ(TestLoc.X, TestLoc.Y, 0.0f);
						const FVector CurrentDestNoZ(CurrentDest.X, CurrentDest.Y, 0.0f);
						if (FMath::PointDistToSegment(CurrentDestNoZ, PrevLocNoZ, TestLocNoZ) < GetPawn()->GetSimpleCollisionRadius())
						{
							return IsAcceptableTranslocation(CurrentDest, TestLoc) ? BMS_Activate : BMS_Abort;
						}
					}
				}
				for (const FRouteCacheItem& RouteItem : RouteCache)
				{
					const FVector RouteLoc = RouteItem.GetLocation(GetPawn());
					if ((RouteLoc - CurrentDest).Size2D() < GetPawn()->GetSimpleCollisionRadius() * 2.0f)
					{
						return IsAcceptableTranslocation(CurrentDest, RouteLoc) ? BMS_Activate : BMS_Abort;
					}
				}
				return BMS_Abort;
			}
		}
		else
		{
			return BMS_Monitoring;
		}
	}
	// if it gets us closer to enemy we are charging or chasing (and won't hit him outright for telefrag)
	else if (Enemy != NULL && GetFocusActor() == Enemy && (IsCharging() || Squad->MustKeepEnemy(this, Enemy)))
	{
		const FVector EnemyLoc = GetEnemyLocation(Enemy, true);
		if ( (EnemyLoc - CurrentDest).Size() < (EnemyLoc - GetPawn()->GetActorLocation()).Size() - 500.0f &&
			(DestVelocity.Size2D() < 250.0f || (DestVelocity.GetSafeNormal() | (EnemyLoc - CurrentDest).GetSafeNormal()) < 0.7f) &&
			IsAcceptableTranslocation(CurrentDest, CurrentDest) )
		{
			return BMS_Activate;
		}
		else
		{
			return DestVelocity.IsZero() ? BMS_Abort : BMS_Monitoring;
		}
	}
	else if (DestVelocity.IsZero())
	{
		// recall since unused
		// TODO: high Tactics bots should consider leaving translocator disc in enemy base
		return BMS_Abort;
	}
	else
	{
		return BMS_Monitoring;
	}
}

bool AUTBot::MovingComboCheck()
{
	if (Skill < 2.0f)
	{
		return false;
	}
	else
	{
		bool bUsingFavoriteWeapon = UTChar != NULL && UTChar->GetWeapon() != NULL && IsFavoriteWeapon(UTChar->GetWeapon()->GetClass());
		if (Skill >= 5.0f && bUsingFavoriteWeapon)
		{
			return true;
		}
		else if (Skill >= 7.0f)
		{
			return (FMath::FRand() < 0.9f);
		}
		else
		{
			return (Skill - (bUsingFavoriteWeapon ? 2.0f : 3.0f) + Personality.MovementAbility > 4.0f * FMath::FRand());
		}
	}
}

bool AUTBot::CanCombo()
{
	if (IsStopped())
	{
		return true;
	}
	else if (GetCharacter() != NULL && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling && FMath::FRand() > 0.1 * Skill + 0.15 * Personality.ReactionTime + 0.15 * Personality.MovementAbility)
	{
		return false;
	}
	// if directed forward movement towards target then allow while moving regardless of skill
	else if (MoveTarget.Actor != NULL && MoveTarget.Actor == GetTarget())
	{
		return true;
	}
	else
	{
		return MovingComboCheck();
	}
}

EBotMonitoringStatus AUTBot::ShouldTriggerCombo(const FVector& CurrentLoc, const FVector& ProjVelocity, const FRadialDamageParams& DamageParams)
{
	// high skill bots check for any enemy in range, not just focused enemy
	bool bCheckAllEnemies = Skill + Personality.Alertness >= 5.5f;
	bool bCheckVisibleEnemies = Skill + Personality.Tactics >= 3.5f || (UTChar != NULL && UTChar->GetWeapon() != NULL && IsFavoriteWeapon(UTChar->GetWeapon()->GetClass()));
	
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	const TArray<const FBotEnemyInfo>& EnemyList = (PS != NULL && PS->Team != NULL) ? PS->Team->GetEnemyList() : *(const TArray<const FBotEnemyInfo>*)&LocalEnemyList;

	bool bPastAllEnemies = true;
	bool bCloseToEnemies = false;
	for (const FBotEnemyInfo& EnemyInfo : EnemyList)
	{
		if (EnemyInfo.IsValid(this) && (bCheckAllEnemies || EnemyInfo.GetPawn() == Enemy || (bCheckVisibleEnemies && EnemyInfo.IsCurrentlyVisible(GetWorld()->TimeSeconds))))
		{
			const FVector EnemyLoc = GetEnemyLocation(EnemyInfo.GetPawn(), true);
			float Dist = (EnemyLoc - CurrentLoc).Size();
			// TODO: health vs expected damage check if skilled instead of 0.5 * Radius
			if (Dist <= 0.5f * DamageParams.OuterRadius + EnemyInfo.GetPawn()->GetSimpleCollisionRadius())
			{
				// close enough
				return BMS_Activate;
			}
			else if (Dist <= DamageParams.OuterRadius + EnemyInfo.GetPawn()->GetSimpleCollisionRadius())
			{
				bCloseToEnemies = true;
				if ((ProjVelocity | (EnemyLoc - CurrentLoc)) <= 0.0f)
				{
					// not going to get any better than this
					return BMS_Activate;
				}
			}
			if (FMath::PointDistToLine(EnemyLoc, ProjVelocity.GetSafeNormal(), CurrentLoc) < DamageParams.OuterRadius + FMath::Max<float>((EnemyInfo.GetUTChar() != NULL) ? EnemyInfo.GetUTChar()->GetCharacterMovement()->GetMaxSpeed() : 0.0f, EnemyInfo.GetPawn()->GetVelocity().Size()))
			{
				bPastAllEnemies = false;
			}
		}
	}
	if (bCloseToEnemies)
	{
		return BMS_PrepareActivation;
	}
	else if (bPastAllEnemies)
	{
		return BMS_Abort;
	}
	else
	{
		return BMS_Monitoring;
	}
}

struct FAppearancePointEval : public FUTNodeEvaluator
{
	TArray<FVector>& FoundPoints;
protected:
	int32 DistanceLimit;
	float MinResultDistanceSq;
	const AUTRecastNavMesh* NavData;
	const UUTPathNode* AskerAnchor;
	float TargetHalfHeight;
	bool bStopAtAsker;
	FCollisionQueryParams TraceParams;
	TSet<NavNodeRef> TestedPolys;

public:
	virtual float Eval(APawn* Asker, const FNavAgentProperties& AgentProps, AController* RequestOwner, const UUTPathNode* Node, const FVector& EntryLoc, int32 TotalDistance) override
	{
		// if the early out at Asker's node was requested, end when we reach it
		return (bStopAtAsker && Node == AskerAnchor) ? 10.0f : 0.0f;
	}

	virtual uint32 GetTransientCost(const FUTPathLink& Link, APawn* Asker, const FNavAgentProperties& AgentProps, AController* RequestOwner, NavNodeRef StartPoly, int32 TotalDistance) override
	{
		if (TotalDistance > DistanceLimit)
		{
			return BLOCKED_PATH_COST;
		}
		else
		{
			// trace test any previously untested nodes along this path
			TArray<NavNodeRef> Polys;
			NavData->FindPolyPath(NavData->GetPolyCenter(StartPoly), AgentProps, FRouteCacheItem(Link.Start.Get(), NavData->GetPolyCenter(Link.StartEdgePoly), Link.StartEdgePoly), Polys, false);
			for (NavNodeRef TestPoly : Polys)
			{
				if (!TestedPolys.Contains(TestPoly))
				{
					FVector TraceEnd = NavData->GetPolySurfaceCenter(TestPoly);
					TraceEnd.Z += TargetHalfHeight;
					bool bTooClose = false;
					for (const FVector& TestPt : FoundPoints)
					{
						if ((TestPt - TraceEnd).SizeSquared() < MinResultDistanceSq)
						{
							bTooClose = true;
							break;
						}
					}
					if (!bTooClose && !NavData->GetWorld()->LineTraceTestByChannel(Asker->GetActorLocation(), TraceEnd, ECC_Visibility, TraceParams, WorldResponseParams))
					{
						FoundPoints.Add(TraceEnd);
					}
					TestedPolys.Add(TestPoly);
				}
			}

			// for proceeding the search, bias towards paths that lead back to Asker
			const FVector StartLoc = NavData->GetPolyCenter(StartPoly);
			const FVector EndLoc = NavData->GetPolyCenter(Link.EndPoly);
			const FVector PathDir = (EndLoc - StartLoc).GetSafeNormal();
			const FVector DirToAsker = (Asker->GetActorLocation() - StartLoc).GetSafeNormal();
			return FMath::TruncToInt(1000.f * (1.0f - (PathDir | DirToAsker)));
		}
	}

	FAppearancePointEval(AActor* InTarget, TArray<FVector>& InFoundPoints, int32 InDistanceLimit = 5000, float InMinResultDistance = 1000.0f, bool bInStopAtAsker = true)
		: FoundPoints(InFoundPoints), DistanceLimit(InDistanceLimit), MinResultDistanceSq(FMath::Square<float>(InMinResultDistance)), NavData(NULL), AskerAnchor(NULL), TargetHalfHeight(InTarget->GetSimpleCollisionHalfHeight()), bStopAtAsker(bInStopAtAsker)
	{
		TestedPolys.Reserve(100);
	}

	virtual bool InitForPathfinding(APawn* Asker, const FNavAgentProperties& AgentProps, const FVector& StartLoc, AUTRecastNavMesh* InNavData) override
	{
		NavData = InNavData;
		AskerAnchor = NavData->GetNodeFromPoly(InNavData->FindAnchorPoly(Asker->GetNavAgentLocation(), Asker, AgentProps));
		TraceParams = FCollisionQueryParams(FName(TEXT("AppearancePointEval")), false, Asker);
		return true;
	}
};

void AUTBot::GuessAppearancePoints(AActor* InTarget, FVector TargetLoc, bool bDoSkillChecks, TArray<FVector>& FoundPoints)
{
	FoundPoints.Reset();
	if (NavData != NULL && InTarget != NULL && GetPawn() != NULL)
	{
		const bool bCheckForwardAndBack = !bDoSkillChecks || Skill + Personality.Tactics + Personality.Alertness >= 1.5f + 1.5f * FMath::FRand();
		APawn* P = Cast<APawn>(InTarget);
		const FBotEnemyInfo* MyEnemyInfo = (P != NULL) ? GetEnemyInfo(P, false) : NULL;
		if (MyEnemyInfo != NULL)
		{
			const FBotEnemyInfo* TeamEnemyInfo = GetEnemyInfo(P, true);
			TargetLoc = TeamEnemyInfo->LastKnownLoc;
			// if last seen loc is still valid, start with that
			if ( !MyEnemyInfo->LastSeenLoc.IsZero() && (MyEnemyInfo->LastSeenLoc - TargetLoc).Size() < (GetPawn()->GetActorLocation() - TargetLoc).Size() &&
				!GetWorld()->LineTraceTestByChannel(GetPawn()->GetActorLocation(), MyEnemyInfo->LastSeenLoc, ECC_Visibility, FCollisionQueryParams(FName(TEXT("AppearanceLastSeen")), false, GetPawn()), WorldResponseParams))
			{
				FoundPoints.Add(MyEnemyInfo->LastSeenLoc);
			}
			if (bCheckForwardAndBack)
			{
				// TODO: do directional back and forward prediction to come up with more points (using enemy info)
			}
			// check if enemy goal point (objective, etc) is in our LOS and go for that
			TArray<FPredictedGoal> Goals;
			Squad->GetPossibleEnemyGoals(this, MyEnemyInfo, Goals);
			for (const FPredictedGoal& PossibleGoal : Goals)
			{
				if (!GetWorld()->LineTraceTestByChannel(GetPawn()->GetActorLocation(), PossibleGoal.Location, ECC_Visibility, FCollisionQueryParams(FName(TEXT("AppearanceGoal")), false, GetPawn()), WorldResponseParams))
				{
					FoundPoints.Add(PossibleGoal.Location);
				}
			}
		}
		else if (bCheckForwardAndBack && !InTarget->GetVelocity().IsZero())
		{
			// TODO: do directional forward prediction to come up with more points (using actor velocity)
		}

		if (!bDoSkillChecks || Skill + Personality.Tactics + Personality.Alertness >= 5.0f || Personality.Tactics > FMath::FRand())
		{
			// use pathing to test additional points that lead back to us
			FAppearancePointEval NodeEval(InTarget, FoundPoints);
			float UnusedWeight = 0.0f;
			TArray<FRouteCacheItem> UnusedRoute;
			NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, TargetLoc - FVector(0.0f, 0.0f, InTarget->GetSimpleCollisionHalfHeight()), UnusedWeight, false, UnusedRoute);
		}
	}
}

bool AUTBot::MayBecomeVisible(APawn* TestEnemy, float MaxWaitTime)
{
	const FBotEnemyInfo* EnemyInfo = GetEnemyInfo(TestEnemy, true);
	if (EnemyInfo == NULL || GetWorld()->TimeSeconds - EnemyInfo->LastFullUpdateTime >= MaxWaitTime)
	{
		// no data or too stale, so assume not
		return false;
	}
	else
	{
		UPawnMovementComponent* EnemyMovementComp = TestEnemy->GetMovementComponent();
		const float EnemySpeed = (EnemyMovementComp != NULL) ? EnemyMovementComp->GetMaxSpeed() : TestEnemy->GetVelocity().Size();
		TArray<FVector> FoundPoints;
		FAppearancePointEval NodeEval(TestEnemy, FoundPoints, FMath::TruncToInt(EnemySpeed * MaxWaitTime));
		float UnusedWeight = 0.0f;
		TArray<FRouteCacheItem> UnusedRoute;
		NavData->FindBestPath(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), this, NodeEval, EnemyInfo->LastKnownLoc - FVector(0.0f, 0.0f, TestEnemy->GetSimpleCollisionHalfHeight()), UnusedWeight, false, UnusedRoute);
		return FoundPoints.Num() > 0;
	}
}

void AUTBot::UTNotifyKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	if (KilledPawn == Enemy)
	{
		Enemy = NULL;
		if (GetPawn() != NULL)
		{
			PickNewEnemy();
			if (Killer == this)
			{
				// TODO: maybe taunt
			}
		}
	}
}

void AUTBot::NotifyTakeHit(AController* InstigatedBy, int32 Damage, FVector Momentum, const FDamageEvent& DamageEvent)
{
	LastUnderFireTime = GetWorld()->TimeSeconds;
	if (InstigatedBy != NULL && InstigatedBy != this && InstigatedBy->GetPawn() != NULL && (Enemy == NULL || !LineOfSightTo(Enemy)) && Squad != NULL && !IsTeammate(InstigatedBy))
	{
		UpdateEnemyInfo(InstigatedBy->GetPawn(), EUT_TookDamage);
	}
}

void AUTBot::NotifyCausedHit(APawn* HitPawn, int32 Damage)
{
	if (HitPawn != GetPawn() && HitPawn->Controller != NULL && !HitPawn->bTearOff && !IsTeammate(HitPawn))
	{
		UpdateEnemyInfo(HitPawn, EUT_DealtDamage);
	}
}

void AUTBot::NotifyPickup(APawn* PickedUpBy, AActor* Pickup, float AudibleRadius, bool bWillBecomeInactive)
{
	if (GetPawn() != NULL && GetPawn() != PickedUpBy && !IsTeammate(PickedUpBy))
	{
		bool bCanUpdateEnemyInfo = Pickup->IsA(AUTPickupHealth::StaticClass());
		if (!bCanUpdateEnemyInfo)
		{
			TSubclassOf<AUTInventory> InventoryType = NULL;

			AUTDroppedPickup* DP = Cast<AUTDroppedPickup>(Pickup);
			if (DP != NULL)
			{
				InventoryType = DP->GetInventoryType();
			}
			else
			{
				AUTPickupInventory* InvPickup = Cast<AUTPickupInventory>(Pickup);
				if (InvPickup != NULL)
				{
					InventoryType = InvPickup->GetInventoryType();
				}
			}
			// assume inventory items that manipulate damage change effective health
			if (InventoryType != NULL)
			{
				bCanUpdateEnemyInfo = InventoryType.GetDefaultObject()->bCallDamageEvents || InventoryType->IsChildOf(AUTArmor::StaticClass());
			}
		}
		if ((Pickup->GetActorLocation() - GetPawn()->GetActorLocation()).Size() < AudibleRadius * HearingRadiusMult * 0.5f || IsEnemyVisible(PickedUpBy) || LineOfSightTo(Pickup))
		{
			if (bCanUpdateEnemyInfo)
			{
				UpdateEnemyInfo(PickedUpBy, EUT_HealthUpdate);
			}
			// maybe send team message
			AUTPickup* Item = Cast<AUTPickup>(Pickup);
			if (Item != NULL && Item->BaseDesireability >= 1.0f && PickedUpBy->PlayerState != NULL)
			{
				static float PickupMessageTime = -1.0f;
				if (PickupMessageTime != GetWorld()->TimeSeconds)
				{
					PickupMessageTime = GetWorld()->TimeSeconds;
					Say(FText::Format(NSLOCTEXT("UTBot", "EnemyPickup", "Enemy {0} got {1}."), FText::FromString(PickedUpBy->PlayerState->PlayerName), Item->GetDisplayName()).ToString(), true);
				}
			}
			if (bWillBecomeInactive && MoveTarget.Actor.Get() == Pickup && CurrentAction != nullptr)
			{
				CurrentAction->PickupTargetTaken();
			}
		}
	}
	else if (GetPawn() == PickedUpBy)
	{
		// maybe send team message
		AUTPickup* Item = Cast<AUTPickup>(Pickup);
		if (Item != NULL && Item->BaseDesireability >= 1.0f)
		{
			Say(FText::Format(NSLOCTEXT("UTBot", "GotPickup", "I got the {0}!"), Item->GetDisplayName()).ToString(), true);
		}
	}
	else if (IsTeammate(PickedUpBy) && bWillBecomeInactive && MoveTarget.Actor.Get() == Pickup && CurrentAction != nullptr)
	{
		CurrentAction->PickupTargetTaken();
	}
	// clear any claims on this pickup since it's likely no longer available
	if (Squad != NULL && Squad->Team != NULL)
	{
		Squad->Team->ClearClaimedPickup(Pickup);
	}
	
}

void AUTBot::ReceiveProjWarning(AUTProjectile* Incoming)
{
	if (Incoming != NULL && !Incoming->GetVelocity().IsZero() && GetPawn() != NULL)
	{
		// bots may duck if not falling or swimming
		if (Skill >= 2.0f && (Enemy != NULL || FMath::FRand() < Personality.Alertness)) // TODO: if 1 on 1 (T)DM be more alert? maybe enemy will usually be set so doesn't matter
		{
			LastUnderFireTime = GetWorld()->TimeSeconds;

			// TODO: should adjust target location if projectile can explode in air like shock combo (i.e. account for damage radius and dodge earlier)
			float ProjTime = Incoming->GetTimeToLocation(GetPawn()->GetActorLocation());
			// check if tight FOV
			bool bShouldDodge = true;
			if (ProjTime < 1.2 || WarningProj != NULL)
			{
				FVector EnemyDir = Incoming->GetActorLocation() - GetPawn()->GetActorLocation();
				EnemyDir.Z = 0;
				FRotationMatrix R(GetControlRotation());
				FVector X = R.GetScaledAxis(EAxis::X);
				X.Z = 0;
				if ((EnemyDir.GetSafeNormal() | X.GetSafeNormal()) < PeripheralVision)
				{
					bShouldDodge = false;
				}
			}
			if (bShouldDodge)
			{
				UpdateEnemyInfo(Incoming->Instigator, EUT_TookDamage);
				SetWarningTimer(Incoming, NULL, ProjTime);
			}
		}
		else if (Enemy == NULL)
		{
			UpdateEnemyInfo(Incoming->Instigator, EUT_TookDamage);
		}
	}
}
void AUTBot::ReceiveInstantWarning(AUTCharacter* Shooter, const FVector& FireDir)
{
	if (Shooter != NULL && Shooter->GetWeapon() != NULL && GetPawn() != NULL)
	{
		const bool bRepeaterWeapon = Shooter->GetWeapon()->bRecommendSuppressiveFire;
		if (Skill >= (bRepeaterWeapon ? 3.0f : 4.0f) && (Enemy != NULL || FMath::FRand() < Personality.Alertness) && FMath::FRand() < 0.2f * (Skill + Personality.Tactics) - 0.33f) // TODO: if 1 on 1 (T)DM be more alert? maybe enemy will usually be set so doesn't matter
		{
			FVector EnemyDir = Shooter->GetActorLocation() - GetPawn()->GetActorLocation();
			EnemyDir.Z = 0;
			FRotationMatrix R(GetControlRotation());
			FVector X = R.GetScaledAxis(EAxis::X);
			X.Z = 0;
			if ((EnemyDir.GetSafeNormal() | X.GetSafeNormal()) >= PeripheralVision)
			{
				LastUnderFireTime = GetWorld()->TimeSeconds;

				UpdateEnemyInfo(Shooter, EUT_TookDamage);

				float DodgeTime;
				if (bRepeaterWeapon)
				{
					DodgeTime = 2.0f - (0.1f + 0.1f * FMath::FRand()) * (Skill + Personality.ReactionTime);
				}
				else
				{
					DodgeTime = Shooter->GetWeapon()->GetRefireTime(Shooter->GetWeapon()->GetCurrentFireMode()) - 0.15 - 0.1 * FMath::FRand(); // TODO: based on collision size 
				}
				if (DodgeTime > 0.0)
				{
					SetWarningTimer(NULL, Shooter, DodgeTime);
				}
			}
		}
		else if (Enemy == NULL)
		{
			UpdateEnemyInfo(Shooter, EUT_TookDamage);
		}
	}
}
void AUTBot::SetWarningTimer(AUTProjectile* Incoming, AUTCharacter* Shooter, float TimeToImpact)
{
	// check that optimal dodge time is far enough in the future for our skills
	if (TimeToImpact >= 0.35f - 0.03f * (Skill + Personality.ReactionTime) && TimeToImpact >= 2.0f - (0.265f + FMath::FRand() * 0.2f) * (Skill + Personality.ReactionTime))
	{
		float WarningDelay;
		if (Skill + Personality.ReactionTime >= 7.0f)
		{
			WarningDelay = FMath::Max<float>(0.08f, FMath::Max<float>(0.35f - 0.025f * (Skill + Personality.ReactionTime) * (1.0f + FMath::FRand()), TimeToImpact - 0.65f));
		}
		else
		{
			WarningDelay = FMath::Max<float>(0.08f, FMath::Max<float>(0.35f - 0.02f * (Skill + Personality.ReactionTime) * (1.0f + FMath::FRand()), TimeToImpact - 0.65f));
		}
		if (!GetWorldTimerManager().IsTimerActive(ProcessIncomingWarningHandle) || WarningDelay < GetWorldTimerManager().GetTimerRate(ProcessIncomingWarningHandle))
		{
			WarningProj = Incoming;
			WarningShooter = (WarningProj != NULL) ? NULL : Shooter;
			// TODO: if in air, consider air control towards wall for wall dodge
			GetWorldTimerManager().SetTimer(ProcessIncomingWarningHandle, this, &AUTBot::ProcessIncomingWarning, WarningDelay, false);
		}
	}
}
void AUTBot::ProcessIncomingWarning()
{
	if (GetUTChar() != NULL && GetUTChar()->UTCharacterMovement != NULL && GetUTChar()->UTCharacterMovement->CanDodge())
	{
		if (WarningProj != NULL)
		{
			if (!WarningProj->IsPendingKillPending() && !WarningProj->bExploded)
			{
				FVector ProjVel = WarningProj->GetVelocity();
				if (!ProjVel.IsZero())
				{
					FVector Dir = ProjVel.GetSafeNormal();
					const float TimeToTarget = WarningProj->GetTimeToLocation(GetPawn()->GetActorLocation());
					FVector FuturePos = GetPawn()->GetActorLocation() + GetPawn()->GetVelocity() * TimeToTarget;
					FVector LineDist = FuturePos - (WarningProj->GetActorLocation() + (Dir | (FuturePos - WarningProj->GetActorLocation())) * Dir);
					float Dist = LineDist.Size();
					if (Dist <= 500.0f + GetPawn()->GetSimpleCollisionRadius())
					{
						bool bShouldDodge = true;
						if (Dist > 1.2f * GetPawn()->GetSimpleCollisionHalfHeight())
						{
							if (WarningProj->DamageParams.BaseDamage <= 40 && GetUTChar()->Health > WarningProj->DamageParams.BaseDamage)
							{
								// probably will miss and even if not we can take the hit
								bShouldDodge = false;
							}
							else if (Dist > GetPawn()->GetSimpleCollisionHalfHeight() + 100.0f + WarningProj->GetMaxDamageRadius())
							{
								// projectile's natural flight will miss by more than its best explosive range
								// check that it won't hit a wall on the way that could make it still dangerous (enemy shooting at floor, etc)
								FCollisionQueryParams Params(FName(TEXT("ProjWarning")), false, GetPawn());
								Params.AddIgnoredActor(WarningProj);
								bShouldDodge = GetWorld()->LineTraceTestByChannel(WarningProj->GetActorLocation(), WarningProj->GetActorLocation() + ProjVel, COLLISION_TRACE_WEAPONNOCHARACTER, Params);
							}
						}

						if (bShouldDodge)
						{
							// dodge away from projectile
							FRotationMatrix R(GetControlRotation());
							FVector X = R.GetScaledAxis(EAxis::X);
							X.Z = 0;

							//if (!TryDuckTowardsMoveTarget(Dir, Y))
							{
								FVector Y = R.GetScaledAxis(EAxis::Y);
								if ((ProjVel | Y) > 0.0f)
								{
									TryEvasiveAction(Y * -1.0f);
								}
								else
								{
									TryEvasiveAction(Y);
								}
							}
						}
					}
				}
			}
		}
		// skip if shooter is no longer focus and bot isn't skilled enough to keep track of prior threat
		else if (WarningShooter != NULL && !WarningShooter->IsPendingKillPending() && !WarningShooter->IsDead() && (WarningShooter == Enemy || Personality.Tactics >= 0.5f || Skill + Personality.Tactics >= 5.0f) && LineOfSightTo(WarningShooter))
		{
			// dodge perpendicular to shooter
			FVector Dir = (WarningShooter->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal();

			FRotationMatrix R(GetControlRotation());
			FVector X = R.GetScaledAxis(EAxis::X);
			X.Z = 0;

			//if (!TryDuckTowardsMoveTarget(Dir, Y))
			{
				FVector Y = R.GetScaledAxis(EAxis::Y);
				if ((Dir | Y) > 0.0f)
				{
					TryEvasiveAction(Y * -1.0f);
				}
				else
				{
					TryEvasiveAction(Y);
				}
			}
		}
	}
	WarningProj = NULL;
	WarningShooter = NULL;
}

void AUTBot::AddFearSpot(AUTAvoidMarker* NewSpot)
{
	if (GetPawn() != NULL && Skill + Personality.Alertness > 1.0f + 4.5f * FMath::FRand() && LineOfSightTo(NewSpot))
	{
		FearSpots.Add(NewSpot);
	}
}

bool AUTBot::TryEvasiveAction(FVector DuckDir)
{
	//if (UTVehicle(Pawn) != None)
//		return UTVehicle(Pawn).Dodge(DCLICK_None);
	//if (Pawn.bStationary)
//		return false;
	if (IsStopped() && GetEnemy() != NULL)
	{
		StartNewAction(TacticalMoveAction);
	}
//	else if (FRand() < 0.6)
//		bChangeStrafe = IsStrafing();


	if (Skill < 3.0f || GetUTChar() == NULL) // TODO: maybe strafe if not UTCharacter?
	{
		return false;
	}
	else  if (GetCharacter()->bIsCrouched || GetCharacter()->GetCharacterMovement()->bWantsToCrouch)
	{
		return false;
	}
	else
	{
		DuckDir.Z = 0;
		DuckDir *= 700.0f;
		FCollisionShape PawnShape = GetCharacter()->GetCapsuleComponent()->GetCollisionShape();
		FVector Start = GetPawn()->GetActorLocation();
		Start.Z += 50.0f;
		FCollisionQueryParams Params(FName(TEXT("TryEvasiveAction")), false, GetPawn());
		FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
		ResponseParams.CollisionResponse.Pawn = ECR_Ignore;

		FHitResult Hit;
		bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + DuckDir, FQuat::Identity, ECC_Pawn, PawnShape, Params, ResponseParams);

		// allow tighter corridors for bots that are willing to wall dodge spam around it
		float MinDist = (Personality.Jumpiness > 0.0f) ? 150.0f : 350.0f;
		float Dist = (Hit.Location - GetPawn()->GetActorLocation()).Size();
		bool bWallHit = false;
		bool bSuccess = false;
		FVector WallHitLoc;
		if (!bHit || Dist > MinDist)
		{
			if (!bHit)
			{
				Hit.Location = Start + DuckDir;
			}
			bHit = GetWorld()->SweepSingleByChannel(Hit, Hit.Location, Hit.Location - FVector(0.0f, 0.0f, 2.5f * GetCharacter()->GetCharacterMovement()->MaxStepHeight), FQuat::Identity, ECC_Pawn, PawnShape, Params, ResponseParams);
			bSuccess = (bHit && Hit.Normal.Z >= 0.7);
		}
		else
		{
			bWallHit = (Skill + 2.0f * Personality.Jumpiness) > 4.0f;
			WallHitLoc = Hit.Location + (-Hit.Normal) * 5.0f; // slightly into wall for air controlling into wall dodge
			MinDist = 70.0f + MinDist - Dist;
		}

		if (!bSuccess)
		{
			DuckDir *= -1.0f;
			bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + DuckDir, FQuat::Identity, ECC_Pawn, PawnShape, Params, ResponseParams);
			bSuccess = (!bHit || (Hit.Location - GetPawn()->GetActorLocation()).Size() > MinDist);
			if (bSuccess)
			{
				if (!bHit)
				{
					Hit.Location = Start + DuckDir;
				}

				bHit = GetWorld()->SweepSingleByChannel(Hit, Hit.Location, Hit.Location - FVector(0.0f, 0.0f, 2.5f * GetCharacter()->GetCharacterMovement()->MaxStepHeight), FQuat::Identity, ECC_Pawn, PawnShape, Params, ResponseParams);
				bSuccess = (bHit && Hit.Normal.Z >= 0.7);
			}
		}
		if (!bSuccess)
		{
			//if (bChangeStrafe)
			//	ChangeStrafe();
			return false;
		}

		if (bWallHit && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Falling && Skill + 2.0f * Personality.Jumpiness > 3.0f + 3.0f * FMath::FRand())
		{
			bPlannedWallDodge = true;
			return true;
		}
		else if ( bWallHit && Personality.Jumpiness > 0.0f && GetCharacter()->CanJump() && GetCharacter()->GetCharacterMovement()->MovementMode == MOVE_Walking &&
				(GetCharacter()->GetCharacterMovement()->Velocity.Size() < 0.1f * GetCharacter()->GetCharacterMovement()->MaxWalkSpeed || (GetCharacter()->GetCharacterMovement()->Velocity.GetSafeNormal2D() | (WallHitLoc - Start).GetSafeNormal2D()) >= 0.0f) &&
				FMath::FRand() < Personality.Jumpiness * 0.5f )
		{
			// jump towards wall for wall dodge
			GetCharacter()->GetCharacterMovement()->Velocity = (GetCharacter()->GetCharacterMovement()->Velocity + (WallHitLoc - Start)).GetClampedToMaxSize(GetCharacter()->GetCharacterMovement()->MaxWalkSpeed);
			GetCharacter()->GetCharacterMovement()->DoJump(false);
			MoveTargetPoints.Insert(FComponentBasedPosition(WallHitLoc), 0);
			bPlannedWallDodge = true;
			return true;
		}
		else
		{
			//bInDodgeMove = true;
			//DodgeLandZ = GetPawn()->GetActorLocation().Z;
			DuckDir.Normalize();
			GetUTChar()->Dodge(DuckDir, (DuckDir ^ FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal());
			return true;
		}
	}
}

int32 AUTBot::GetRouteDist() const
{
	if (NavData == NULL || GetPawn() == NULL)
	{
		return -1.0f;
	}
	else if (RouteCache.Num() == 0)
	{
		return 0.0f;
	}
	else
	{
		NavNodeRef AnchorPoly = NavData->FindAnchorPoly(GetPawn()->GetNavAgentLocation(), GetPawn(), GetPawn()->GetNavAgentPropertiesRef());
		const UUTPathNode* Anchor = NavData->GetNodeFromPoly(AnchorPoly);
		int32 RouteStartIdx = 0;
		int32 Dist = 0;
		if (Anchor == NULL)
		{
			Anchor = RouteCache[0].Node.Get();
			AnchorPoly = RouteCache[0].TargetPoly;
			Dist = FMath::TruncToInt((RouteCache[0].GetLocation(GetPawn()) - GetPawn()->GetActorLocation()).Size());
		}
		FUTReachParams ReachParams(GetPawn(), GetPawn()->GetNavAgentPropertiesRef());
		for (int32 i = RouteStartIdx; i < RouteCache.Num() && Anchor != NULL; i++)
		{
			int32 LinkIndex = Anchor->GetBestLinkTo(AnchorPoly, RouteCache[i], GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), NavData);
			if (LinkIndex == INDEX_NONE)
			{
				// hmmm... not sure if distance so far is correct here, maybe should be BLOCKED_PATH_COST?
				return Dist;
			}
			const FUTPathLink& Link = Anchor->Paths[LinkIndex];
			Dist += Link.CostFor(GetPawn(), GetPawn()->GetNavAgentPropertiesRef(), ReachParams, GetPawn()->Controller, AnchorPoly, NavData);
			Anchor = Link.End.Get();
			AnchorPoly = Link.EndPoly;
		}
		return Dist;
	}
}

void AUTBot::PickNewEnemy()
{
	if ((Enemy == NULL || Enemy->Controller == NULL || !Squad->MustKeepEnemy(this, Enemy) || !CanAttack(Enemy, GetEnemyLocation(Enemy, true), false)) && GetPawn() != NULL)
	{
		LastPickEnemyTime = GetWorld()->TimeSeconds;

		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		const TArray<const FBotEnemyInfo>& EnemyList = (PS != NULL && PS->Team != NULL) ? PS->Team->GetEnemyList() : *(const TArray<const FBotEnemyInfo>*)&LocalEnemyList;

		LastPickEnemyRatings.Empty();
		APawn* BestEnemy = NULL;
		float BestRating = -10000.0f;
		for (const FBotEnemyInfo& EnemyInfo : EnemyList)
		{
			if (EnemyInfo.IsValid())
			{
				bool bLostEnemy = EnemyInfo.bLostEnemy;
				if (&EnemyList != (const TArray<const FBotEnemyInfo>*)&LocalEnemyList)
				{
					for (const FBotEnemyInfo& LocalEnemyInfo : LocalEnemyList)
					{
						if (LocalEnemyInfo.GetPawn() == EnemyInfo.GetPawn())
						{
							// TODO: if bot is defending, possibly ignore this if teammates have confirmed their location very recently
							bLostEnemy = LocalEnemyInfo.bLostEnemy;
							break;
						}
					}
				}
				if (!bLostEnemy || Squad->MustKeepEnemy(this, EnemyInfo.GetPawn()))
				{
					float Rating = RateEnemy(EnemyInfo);
					// enemy rating may call weapon script, anything could happen
					if (GetPawn() == NULL)
					{
						return;
					}
					new(LastPickEnemyRatings) FBotEnemyRating(EnemyInfo.GetPawn(), Rating);
					if (Rating > BestRating)
					{
						BestEnemy = EnemyInfo.GetPawn();
						BestRating = Rating;
					}
				}
			}
		}
		SetEnemy(BestEnemy);
	}
}

float AUTBot::RateEnemy(const FBotEnemyInfo& EnemyInfo)
{
	float NewStrength = RelativeStrength(EnemyInfo.GetPawn());
	// more likely to pursue strong enemies if aggressive
	float ThreatValue = FMath::Clamp<float>(NewStrength, 0.0f, 1.0f) * Personality.Aggressiveness;
	// more likely to pursue weak enemies if tactical (TODO: some other personality attribute? We check Tactics a lot...)
	if (NewStrength < 0.0f && Personality.Tactics > 0.0f)
	{
		ThreatValue += NewStrength * -0.5f * Personality.Tactics;
	}
	float Dist = (EnemyInfo.LastKnownLoc - GetPawn()->GetActorLocation()).Size();
	if (Dist < 4500.0f)
	{
		ThreatValue += 0.2;
		if (Dist < 3300.0f)
		{
			ThreatValue += 0.2;
			if (Dist < 2200.0f)
			{
				ThreatValue += 0.2;
				if (Dist < 1100.0f)
				{
					ThreatValue += 0.2;
				}
			}
		}
	}

	bool bThreatVisible = IsEnemyVisible(EnemyInfo.GetPawn()); // intentional that we use bot's personal visibility here instead of team
	bool bThreatAttackable = CanAttack(EnemyInfo.GetPawn(), EnemyInfo.LastKnownLoc, false);
	// CanAttack() calls weapon script event, anything could happen
	if (GetPawn() == NULL)
	{
		return 0.0f;
	}
	else
	{
		if (bThreatVisible)
		{
			ThreatValue += 1.0f;
			ThreatValue += FMath::Max<float>(0.0f, 1.0f - ((GetWorld()->TimeSeconds - EnemyInfo.LastHitByTime) / 2.0f));
		}
		else if (bThreatAttackable)
		{
			ThreatValue += 0.5f;
			ThreatValue += FMath::Max<float>(0.0f, 1.0f - ((GetWorld()->TimeSeconds - EnemyInfo.LastHitByTime) / 2.0f)) * 0.5f;
		}
		if (Enemy != NULL && EnemyInfo.GetPawn() != Enemy)
		{
			if (!bThreatVisible)
			{
				if (!bThreatAttackable)
				{
					ThreatValue -= 5.0f;
				}
				else
				{
					ThreatValue -= 2.0f * FMath::Min<float>(1.0f, Dist / 3300.0f);
				}
			}
			else if (GetWorld()->TimeSeconds - GetEnemyInfo(Enemy, false)->LastSeenTime > 2.0f)
			{
				ThreatValue += 1;
			}
			if (Dist > 0.7f * (GetEnemyInfo(Enemy, true)->LastKnownLoc - GetPawn()->GetActorLocation()).Size())
			{
				ThreatValue -= 0.25;
			}
			ThreatValue -= 0.2;

			/*if (B.IsHunting() && (NewStrength < 0.2)
				&& (WorldInfo.TimeSeconds - FMax(B.LastSeenTime, B.AcquireTime) < 2.5))
				ThreatValue -= 0.3;*/
		}

		// TODO: further personality adjust (hate for enemy that kills me, enemy that took powerup I was going for, etc)

		return Squad->ModifyEnemyRating(ThreatValue, EnemyInfo, this);
	}
}

bool AUTBot::IsImportantEnemyUpdate(APawn* TestEnemy, EAIEnemyUpdateType UpdateType)
{
	if (GetPawn() == nullptr || UpdateType == EUT_HealthUpdate || UpdateType == EUT_HeardApprox || UpdateType == EUT_DealtDamage)
	{
		// updates that don't give us a fix on their position and movement aren't worth re-evaluating for
		return false;
	}
	// always re-evaluate if important enemy status changed and not currently targeted
	else if (TestEnemy != Enemy && Squad->MustKeepEnemy(this, TestEnemy))
	{
		return true;
	}
	else
	{
		const FBotEnemyInfo* MyEnemyInfo = GetEnemyInfo(TestEnemy, false);
		if (MyEnemyInfo == NULL || MyEnemyInfo->bLostEnemy || CurrentAction == CampAction)
		{
			return true;
		}
		else
		{
			float MinLostContactTime = 0.5f;
			if (Skill + Personality.Alertness < 5.0f)
			{
				MinLostContactTime += 6.5f - (Skill + Personality.Alertness);
			}
			else
			{
				MinLostContactTime += 1.5f - (Skill + Personality.Alertness - 5.0f) * 0.5f;
			}
			return (GetWorld()->TimeSeconds - MyEnemyInfo->LastFullUpdateTime >= MinLostContactTime);
		}
	}
}

void AUTBot::UpdateEnemyInfo(APawn* NewEnemy, EAIEnemyUpdateType UpdateType)
{
	if (NewEnemy != NULL && !NewEnemy->bTearOff && !NewEnemy->IsPendingKillPending() && Squad != NULL && !IsTeammate(NewEnemy)) //  && (!AreAIIgnoringPlayers() || Cast<APlayerController>(NewEnemy->Controller) == NULL || Game->bOfflineChallenge)
	{
		bool bImportant = IsImportantEnemyUpdate(NewEnemy, UpdateType);

		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
		if (PS != NULL && PS->Team != NULL)
		{
			PS->Team->UpdateEnemyInfo(NewEnemy, UpdateType);
		}
		bool bFound = false;
		for (int32 i = 0; i < LocalEnemyList.Num(); i++)
		{
			if (!LocalEnemyList[i].IsValid(this))
			{
				// we assume our current enemy is in the list!
				if (LocalEnemyList[i].GetPawn() == Enemy)
				{
					SetEnemy(NULL);
				}
				LocalEnemyList.RemoveAt(i--, 1);
			}
			else if (LocalEnemyList[i].GetPawn() == NewEnemy)
			{
				LocalEnemyList[i].Update(UpdateType);
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			new(LocalEnemyList) FBotEnemyInfo(NewEnemy, UpdateType);
		}
		if (bImportant)
		{
			PickNewEnemy();
			// maybe send voice message
			if (Enemy == NewEnemy && (UpdateType == EUT_Seen || UpdateType == EUT_DealtDamage) && (GetWorld()->GetAuthGameMode<AUTFlagRunGame>() == nullptr))
			{
				AUTCharacter* C = Cast<AUTCharacter>(NewEnemy);
				if (C != NULL && C->GetCarriedObject() != NULL)
				{
					SendVoiceMessage(StatusMessage::EnemyFCHere);
				}
			}
		}
	}
}

void AUTBot::RemoveEnemy(APawn* OldEnemy)
{
	for (TArray<FBotEnemyInfo>::TIterator It(LocalEnemyList); It; ++It)
	{
		if (It->GetPawn() == OldEnemy)
		{
			It->bLostEnemy = true;
			if (OldEnemy == Enemy)
			{
				SetEnemy(NULL);
			}
			break;
		}
	}
}

void AUTBot::SetEnemy(APawn* NewEnemy)
{
	if (NewEnemy != Enemy)
	{
		if (Target == Enemy)
		{
			Target = NULL;
		}
		if (GetFocusActor() == Enemy)
		{
			SetFocus(NewEnemy);
		}
		Enemy = NewEnemy;
		AUTCharacter* EnemyP = Cast<AUTCharacter>(Enemy);
		if (EnemyP != NULL)
		{
			if (EnemyP->IsDead())
			{
				UE_LOG(UT, Warning, TEXT("Bot got dead enemy %s"), *EnemyP->GetName());
				Enemy = NULL;
			}
			else
			{
				EnemyP->MaxSavedPositionAge = FMath::Max<float>(EnemyP->MaxSavedPositionAge, TrackingReactionTime);
			}
		}
		// make sure we always have local info for enemies we focus on (simplifies decision code)
		if (Enemy != NULL && !GetEnemyInfo(Enemy, false))
		{
			const FBotEnemyInfo* TeamEnemyInfo = GetEnemyInfo(Enemy, true);
			if (TeamEnemyInfo == NULL)
			{
				UpdateEnemyInfo(Enemy, EUT_HeardApprox);
			}
			else
			{
				// copy some details from team
				FBotEnemyInfo* NewEnemyInfo = new(LocalEnemyList) FBotEnemyInfo(*TeamEnemyInfo);
				NewEnemyInfo->LastSeenTime = -100000.0f;
				NewEnemyInfo->LastHitByTime = -100000.0f;
			}
		}
		LastEnemyChangeTime = GetWorld()->TimeSeconds;
		LastFireSuccessTime = -100.0f; // this is so AI will abort suppressive fire immediately if new enemy is clearly behind a wall
		if (Enemy != NULL)
		{
			UpdateTrackingError(true);
		}
		if (bExecutingWhatToDoNext)
		{
			// force update of visibility info if this is during decision logic
			if (Enemy != NULL && CanSee(Enemy, false))
			{
				SeePawn(Enemy);
			}
		}
		// don't interrupt in progress translocation
		else if (TranslocTarget.IsZero() || UTChar == NULL || UTChar->GetWeapon() == NULL || UTChar->GetWeapon()->BaseAISelectRating >= 0.5f || (Enemy != NULL && Squad->MustKeepEnemy(this, Enemy)))
		{
			WhatToDoNext();
		}
	}
}

void AUTBot::HearSound(APawn* Other, const FVector& SoundLoc, float SoundRadius)
{
	if (Other != NULL && Other != GetPawn() && !Other->IsOwnedBy(this) && !IsTeammate(Other))
	{
		UpdateEnemyInfo(Other, ((SoundLoc - GetPawn()->GetActorLocation()).Size() < SoundRadius * HearingRadiusMult * 0.5f) ? EUT_HeardExact : EUT_HeardApprox);
	}
}

void AUTBot::SetTarget(AActor* NewTarget)
{
	Target = NewTarget;
}

void AUTBot::SeePawn(APawn* Other)
{
	if (!IsTeammate(Other))
	{
		UpdateEnemyInfo(Other, EUT_Seen);
	}
}

bool AUTBot::CanSee(APawn* Other, bool bMaySkipChecks)
{
	if (Other == NULL || GetPawn() == NULL || (Other->IsA(AUTCharacter::StaticClass()) && ((AUTCharacter*)Other)->IsInvisible()) || Other->IsA(ASpectatorPawn::StaticClass()))
	{
		return false;
	}
	else if (Other == Enemy)
	{
		// TODO: should still check basic FOV, shouldn't we?
		return LineOfSightTo(Other);
	}
	else
	{
		bLOSflag = !bLOSflag;

		const FVector MyLoc = GetPawn()->GetActorLocation();
		const FVector OtherLoc = Other->GetActorLocation();

		// fixed max sight distance
		if ((OtherLoc - MyLoc).SizeSquared() > FMath::Square(SightRadius))
		{
			return false;
		}
		else
		{
			float Dist = (OtherLoc - MyLoc).Size();

			// may skip if more than 1/5 of maxdist away (longer time to acquire)
			// TODO: check if Other has recently been seen already?
			if (bMaySkipChecks && FMath::FRand() * Dist > 0.1f * SightRadius)
			{
				return false;
			}
			else
			{
				// check field of view
				FVector SightDir = (OtherLoc - MyLoc).GetSafeNormal();
				const FVector LookDir = GetPawn()->GetViewRotation().Vector();
				if ((SightDir | LookDir) < PeripheralVision)
				{
					return false;
				}
				else if (bMaySkipChecks && bSlowerZAcquire && FMath::FRand() * Dist > 0.1f * SightRadius)
				{
					// lower FOV vertically
					SightDir.Z *= 2.f;
					SightDir.Normalize();
					if ((SightDir | LookDir) < PeripheralVision)
					{
						return false;
					}
					else
					{
						// notice other pawns at very different heights more slowly
						float HeightMod = FMath::Abs(OtherLoc.Z - MyLoc.Z);
						if (FMath::FRand() * Dist < HeightMod)
						{
							return false;
						}
						else
						{
							return Super::LineOfSightTo(Other, FVector(ForceInit), bMaySkipChecks);
						}
					}
				}
				else
				{
					return LineOfSightTo(Other, FVector(ForceInit), bMaySkipChecks);
				}
			}
		}
	}
}
bool AUTBot::LineOfSightTo(const class AActor* Other, FVector ViewPoint, bool bAlternateChecks) const
{
	return (Other == NULL) ? false : UTLineOfSightTo(Other, ViewPoint, bAlternateChecks, Other->GetTargetLocation(GetPawn()));
}
bool AUTBot::UTLineOfSightTo(const AActor* Other, FVector ViewPoint, bool bAlternateChecks, FVector TargetLocation) const
{
	if (Other == NULL)
	{
		return false;
	}
	else
	{
		const bool bOtherIsRagdoll = Cast<AUTCharacter>(Other) != NULL && ((AUTCharacter*)Other)->IsRagdoll();

		if (ViewPoint.IsZero())
		{
			AActor*	ViewTarg = GetViewTarget();
			ViewPoint = ViewTarg->GetActorLocation();
			if (ViewTarg == GetPawn())
			{
				ViewPoint.Z += GetPawn()->BaseEyeHeight; //look from eyes
			}
		}

		static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
		if (TargetLocation.IsZero())
		{
			TargetLocation = Other->GetTargetLocation(GetPawn());
		}

		FCollisionQueryParams CollisionParams(NAME_LineOfSight, true, GetPawn());
		CollisionParams.AddIgnoredActor(Other);

		bool bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, TargetLocation, ECC_Visibility, CollisionParams);
		if (bOtherIsRagdoll && bHit)
		{
			// actor location will be near/in the ground for ragdolls, push up
			TargetLocation.Z += Other->GetSimpleCollisionHalfHeight();
			bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, TargetLocation, ECC_Visibility, CollisionParams);
		}
		if (Other == Enemy)
		{
			if (bHit)
			{
				bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, TargetLocation + FVector(0.0f, 0.0f, Enemy->BaseEyeHeight + (bOtherIsRagdoll ? Enemy->GetSimpleCollisionHalfHeight() : 0.0f)), ECC_Visibility, CollisionParams);
			}
			if (!bHit)
			{
				return true;
			}
			// only check sides if width of other is significant compared to distance
			else if (FMath::Square(Other->GetSimpleCollisionRadius()) / (Other->GetActorLocation() - ViewPoint).SizeSquared() * 0.0001f)
			{
				return false;
			}
		}
		else if (!bHit)
		{
			return true;
		}
		else
		{
			float DistSq = (Other->GetActorLocation() - ViewPoint).SizeSquared();
			if (DistSq > FMath::Square(SightRadius))
			{
				return false;
			}
			bool bTargetIsPawn = Cast<APawn>(Other) != NULL;
			if (!bTargetIsPawn && Cast<UCapsuleComponent>(Other->GetRootComponent()) == NULL)
			{
				return false;
			}
			// lower distance requirement for advanced checks for monsters or against non-Pawns
			if ((PlayerState == NULL || !bTargetIsPawn) && DistSq > FMath::Square(SightRadius * 0.25f))
			{
				return false;
			}
			// try viewpoint to head
			if ((!bAlternateChecks || !bLOSflag) && !GetWorld()->LineTraceTestByChannel(ViewPoint, TargetLocation + FVector(0.0f, 0.0f, Other->GetSimpleCollisionHalfHeight() * (bOtherIsRagdoll ? 2.0f : 1.0f)), ECC_Visibility, CollisionParams))
			{
				return true;
			}
			if (bAlternateChecks && !bLOSflag)
			{
				return false;
			}
			if (FMath::Square(Other->GetSimpleCollisionRadius()) / DistSq < 0.00015f)
			{
				return false;
			}
		}

		//try checking sides - look at dist to four side points, and cull furthest and closest
		FVector Points[4];
		const FVector OtherLoc = TargetLocation;
		const float OtherRadius = Other->GetSimpleCollisionRadius();
		Points[0] = OtherLoc - FVector(OtherRadius, -1 * OtherRadius, 0);
		Points[1] = OtherLoc + FVector(OtherRadius, OtherRadius, 0);
		Points[2] = OtherLoc - FVector(OtherRadius, OtherRadius, 0);
		Points[3] = OtherLoc + FVector(OtherRadius, -1 * OtherRadius, 0);
		int32 imin = 0;
		int32 imax = 0;
		float currentmin = Points[0].SizeSquared();
		float currentmax = currentmin;
		for (int32 i = 1; i < 4; i++)
		{
			float nextsize = Points[i].SizeSquared();
			if (nextsize > currentmax)
			{
				currentmax = nextsize;
				imax = i;
			}
			else if (nextsize < currentmin)
			{
				currentmin = nextsize;
				imin = i;
			}
		}

		for (int32 i = 0; i < 4; i++)
		{
			if (i != imin && i != imax && !GetWorld()->LineTraceTestByChannel(ViewPoint, Points[i], ECC_Visibility, CollisionParams))
			{
				return true;
			}
		}
		return false;
	}
}