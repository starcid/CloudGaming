// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTJumpPad.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"
#include "AI/NavigationSystemHelpers.h"
#include "AI/NavigationOctree.h"
#include "UTReachSpec_JumpPad.h"
#include "UTGameEngine.h"
#include "UTNavArea_Default.h"

AUTJumpPad::AUTJumpPad(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneRoot;
	RootComponent->bShouldUpdatePhysicsVolume = true;

	// Setup the mesh
	Mesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("JumpPadMesh"));
	Mesh->SetupAttachment(RootComponent);

	TriggerBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("TriggerBox"));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AUTJumpPad::TriggerBeginOverlap);

	JumpSound = nullptr;
	JumpTarget = FVector(100.0f, 0.0f, 0.0f);
	JumpTime = 1.0f;
	bMaintainVelocity = false;

	bLargeUnitUsable = true;

#if WITH_EDITORONLY_DATA
	JumpPadComp = ObjectInitializer.CreateDefaultSubobject<UUTJumpPadRenderingComponent>(this, TEXT("JumpPadComp"));
	JumpPadComp->PostPhysicsComponentTick.bCanEverTick = false;
	JumpPadComp->SetupAttachment(RootComponent);
#endif // WITH_EDITORONLY_DATA
}

void AUTJumpPad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Launch the pending actors
	if (PendingJumpActors.Num() > 0)
	{
		for (AActor* Actor : PendingJumpActors)
		{
			Launch(Actor);
		}
		PendingJumpActors.Reset();
	}
}

bool AUTJumpPad::IsEnabled_Implementation() const
{
	return TriggerBox != NULL && TriggerBox->IsCollisionEnabled() && TriggerBox->bGenerateOverlapEvents;
}

bool AUTJumpPad::CanLaunch_Implementation(AActor* TestActor)
{
	return (Cast<ACharacter>(TestActor) != NULL && TestActor->Role >= ROLE_AutonomousProxy);
}

void AUTJumpPad::Launch_Implementation(AActor* Actor)
{
	// For now just filter for ACharacter. Maybe certain projectiles/vehicles/ragdolls/etc should bounce in the future
	ACharacter* Char = Cast<ACharacter>(Actor);
	if (Char && !Char->IsPendingKillPending())
	{
		//Launch the character to the target
		Char->LaunchCharacter(CalculateJumpVelocity(Char), !bMaintainVelocity, true);

		if (RestrictedJumpTime > 0.0f)
		{
			AUTCharacter* UTChar = Cast<AUTCharacter>(Char);
			if (UTChar != NULL)
			{
				UTChar->UTCharacterMovement->RestrictJump(RestrictedJumpTime);
			}
		}

		// Play Jump sound if we have one
		UUTGameplayStatics::UTPlaySound(GetWorld(), JumpSound, Char, SRT_AllButOwner, false);

		// if it's a bot, refocus it to its desired endpoint for any air control adjustments
		AUTBot* B = Cast<AUTBot>(Char->Controller);
		AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
		if (B != NULL && NavData != NULL)
		{
			if (bAIReducedAirControl)
			{
				B->bRestrictedJump = true;
				if (Char->GetCharacterMovement() != nullptr)
				{
					Char->GetCharacterMovement()->bNotifyApex = true;
				}
			}
			const float MinMoveTimer = bJumpThroughWater ? (JumpTime + 1.0f) : JumpTime;
			bool bRepathOnLand = false;
			bool bExpectedJumpPad = B->GetMoveTarget().Actor == this || (B->GetMoveTarget().Node != NULL && B->GetMoveTarget().Node->POIs.Contains(this));
			if (!bExpectedJumpPad)
			{
				UUTReachSpec_JumpPad* JumpPadPath = Cast<UUTReachSpec_JumpPad>(B->GetCurrentPath().Spec.Get());
				bExpectedJumpPad = ((JumpPadPath != NULL && JumpPadPath->JumpPad == this) || B->GetMoveTarget().TargetPoly == GetUTNavData(GetWorld())->UTFindNearestPoly(GetActorLocation(), GetSimpleCollisionCylinderExtent()));
			}
			if (bExpectedJumpPad)
			{
				bRepathOnLand = true;
				for (int32 i = 0; i < B->RouteCache.Num() - 1; i++)
				{
					if (B->RouteCache[i].Actor == this)
					{
						TArray<FComponentBasedPosition> MovePoints;
						new(MovePoints) FComponentBasedPosition(B->RouteCache[i + 1].GetLocation(Char));
						B->SetMoveTarget(B->RouteCache[i + 1], MovePoints);
						B->MoveTimer = FMath::Max<float>(B->MoveTimer, MinMoveTimer);
						bRepathOnLand = false;
						break;
					}
				}
				if (bRepathOnLand)
				{
					// still pop point 0 since we know it's on/near us via bExpectedJumpPad, but repath on land in case the bot's path is messed up
					if (B->RouteCache.Num() > 1)
					{
						TArray<FComponentBasedPosition> MovePoints;
						new(MovePoints) FComponentBasedPosition(B->RouteCache[1].GetLocation(Char));
						B->SetMoveTarget(B->RouteCache[1], MovePoints);
					}
					else
					{
						B->SetMoveTargetDirect(FRouteCacheItem((ActorToWorld().TransformPosition(JumpTarget))));
					}
					B->MoveTimer = FMath::Max<float>(B->MoveTimer, MinMoveTimer);
				}
			}
			else if (!B->GetMoveTarget().IsValid() || B->MoveTimer <= 0.0f)
			{
				// bot got knocked onto jump pad, is stuck on it, or otherwise is surprised to be here
				bRepathOnLand = true;
			}
			if (bRepathOnLand)
			{
				// make sure bot aborts move when it lands
				B->MoveTimer = FMath::Min<float>(B->MoveTimer, MinMoveTimer - 0.1f);
				// if bot might be stuck or the jump pad just goes straight up (such that it will never land without air control), we need to force something to happen
				if ((B->MoveTimer <= 0.0f || JumpTarget.Size2D() < 1.0f) && (!B->GetMoveTarget().IsValid() || (B->GetMovePoint() - Char->GetActorLocation()).Size2D() < 50.0f || (B->GetMovePoint() - GetActorLocation()).Size2D() < 50.0f))
				{
					UUTPathNode* MyNode = NavData->FindNearestNode(GetActorLocation(), NavData->GetPOIExtent(this));
					if (MyNode != NULL)
					{
						for (const FUTPathLink& Path : MyNode->Paths)
						{
							UUTReachSpec_JumpPad* JumpPadPath = Cast<UUTReachSpec_JumpPad>(Path.Spec.Get());
							if (JumpPadPath != NULL && JumpPadPath->JumpPad == this)
							{
								B->SetMoveTargetDirect(FRouteCacheItem(Path.End.Get(), NavData->GetPolySurfaceCenter(Path.EndPoly), Path.EndPoly));
								break;
							}
						}
					}
				}
			}
		}
	}
}

void AUTJumpPad::TriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Add the actor to be launched if it hasn't already
	if (!PendingJumpActors.Contains(OtherActor) && CanLaunch(OtherActor))
	{
		PendingJumpActors.Add(OtherActor);
	}
}

FVector AUTJumpPad::CalculateJumpVelocity(AActor* JumpActor)
{
	FVector Target = ActorToWorld().TransformPosition(JumpTarget) - JumpActor->GetActorLocation();
	const float GravityZ = GetLocationGravityZ(GetWorld(), GetActorLocation(), TriggerBox->GetCollisionShape());
	if (GravityZ > AuthoredGravityZ)
	{
		// workaround for jump pads that place their target too close to the ground so in low grav the smaller boost can't get the full pawn capsule over ledges
		Target.Z += GetDefault<AUTCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	}

	float SizeZ = Target.Z / JumpTime + 0.5f * -GravityZ * JumpTime;
	float SizeXY = Target.Size2D() / JumpTime;

	FVector Velocity = Target.GetSafeNormal2D() * SizeXY + FVector(0.0f, 0.0f, SizeZ);

	// Scale the velocity if Character has gravity scaled
	ACharacter* Char = Cast<ACharacter>(JumpActor);
	if (Char != NULL && Char->GetCharacterMovement() != NULL && Char->GetCharacterMovement()->GravityScale != 1.0f)
	{
		Velocity *= FMath::Sqrt(Char->GetCharacterMovement()->GravityScale);
	}
	return Velocity;
}

void AUTJumpPad::AddSpecialPaths(class UUTPathNode* MyNode, class AUTRecastNavMesh* NavData)
{
	FVector MyLoc = GetActorLocation();
	NavNodeRef MyPoly = NavData->UTFindNearestPoly(MyLoc, GetSimpleCollisionCylinderExtent());
	if (MyPoly != INVALID_NAVNODEREF)
	{
		const FVector JumpTargetWorld = ActorToWorld().TransformPosition(JumpTarget);
		const float GravityZ = GetLocationGravityZ(GetWorld(), GetActorLocation(), TriggerBox->GetCollisionShape());
		const FCapsuleSize HumanSize = NavData->GetHumanPathSize();
		const FCapsuleSize PathSize = bLargeUnitUsable ? NavData->GetMaxPathSize() : HumanSize;
		bJumpThroughWater = false;
		{
			TArray<FHitResult> Hits;
			GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), JumpTargetWorld, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(HumanSize.Radius, HumanSize.Height), FCollisionQueryParams());
			for (const FHitResult& Hit : Hits)
			{
				if (!Hit.bBlockingHit)
				{
					APhysicsVolume* Volume = Cast<APhysicsVolume>(Hit.Actor.Get());
					if (Volume != NULL && Volume->bWaterVolume)
					{
						bJumpThroughWater = true;
						break;
					}
				}
			}
		}
		const uint32 ReachFlags = R_JUMP | (bJumpThroughWater ? R_SWIM : 0);
		FVector HumanExtent = FVector(HumanSize.Radius, HumanSize.Radius, HumanSize.Height);
		{
			NavNodeRef TargetPoly = NavData->UTFindNearestPoly(JumpTargetWorld, HumanExtent);
			if (TargetPoly == INVALID_NAVNODEREF)
			{
				// jump target may be in air
				// extrapolate downward a bit to try to find poly
				FVector JumpVel = CalculateJumpVelocity(this);
				if (JumpVel.Size2D() > 1.0f)
				{
					JumpVel.Z += GravityZ * JumpTime;
					FVector CurrentLoc = JumpTargetWorld;
					const float TimeSlice = 0.1f;
					for (int32 i = 0; i < 5; i++)
					{
						FVector NextLoc = CurrentLoc + JumpVel * TimeSlice;
						JumpVel.Z += GravityZ * TimeSlice;
						FHitResult Hit;
						if (GetWorld()->SweepSingleByChannel(Hit, CurrentLoc, NextLoc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(HumanExtent), FCollisionQueryParams(), WorldResponseParams))
						{
							TargetPoly = NavData->UTFindNearestPoly(Hit.Location, HumanExtent);
							break; // hit ground so have to stop here
						}
						else
						{
							TargetPoly = NavData->UTFindNearestPoly(NextLoc, HumanExtent);
							if (TargetPoly != INVALID_NAVNODEREF)
							{
								break;
							}
						}
						CurrentLoc = NextLoc;
					}
				}
			}
			UUTPathNode* TargetNode = NavData->GetNodeFromPoly(TargetPoly);
			if (TargetPoly != INVALID_NAVNODEREF && TargetNode != NULL)
			{
				UUTReachSpec_JumpPad* JumpSpec = NewObject<UUTReachSpec_JumpPad>(MyNode);
				JumpSpec->JumpPad = this;
				FUTPathLink* NewLink = new(MyNode->Paths) FUTPathLink(MyNode, MyPoly, TargetNode, TargetPoly, JumpSpec, PathSize.Radius, PathSize.Height, ReachFlags);
				for (NavNodeRef SrcPoly : MyNode->Polys)
				{
					NewLink->Distances.Add(NavData->CalcPolyDistance(SrcPoly, MyPoly) + FMath::TruncToInt(1000.0f * JumpTime));
				}
			}
		}

		// if we support air control, look for additional jump targets that could be reached by air controlling against the jump pad's standard velocity
		if (RestrictedJumpTime < JumpTime && NavData->ScoutClass != NULL && NavData->ScoutClass.GetDefaultObject()->GetCharacterMovement()->AirControl > 0.0f)
		{
			// intentionally place start loc high up to avoid clipping the edges of any irrelevant geometry
			MyLoc.Z += NavData->ScoutClass.GetDefaultObject()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * 3.0f;

			const float EffectiveRestrictedTime = (bJumpThroughWater || bAIReducedAirControl) ? FMath::Max<float>(JumpTime * 0.5f, RestrictedJumpTime) : RestrictedJumpTime;
			const float AirControlPct = NavData->ScoutClass.GetDefaultObject()->GetCharacterMovement()->AirControl;
			const float AirAccelRate = AirControlPct * NavData->ScoutClass.GetDefaultObject()->GetCharacterMovement()->MaxAcceleration;
			const FCollisionShape ScoutShape = FCollisionShape::MakeCapsule(NavData->ScoutClass.GetDefaultObject()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), NavData->ScoutClass.GetDefaultObject()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
			const FVector JumpVel = CalculateJumpVelocity(this);
			const float XYSpeed = FMath::Max<float>(JumpVel.Size2D(), NavData->ScoutClass.GetDefaultObject()->GetCharacterMovement()->MaxWalkSpeed); // TODO: clamp contribution of MaxWalkSpeed based on size of jump / time available to apply air control?
			const bool bHighHorizontalSpeed = XYSpeed > NavData->ScoutClass.GetDefaultObject()->GetCharacterMovement()->MaxWalkSpeed;
			const float JumpZ = JumpVel.Z;
			// if the jump pad primarily executes a "super-jump" (velocity almost entirely on Z axis, allows full air control) then a normal character jump test will give us good results
			const bool bIsZOnlyJump = JumpTarget.Size2D() < 0.1f * JumpTarget.Size();
			const float JumpTargetDist = JumpTarget.Size();
			const FVector JumpDir2D = (JumpTargetWorld - MyLoc).GetSafeNormal2D();
			const FVector HeightAdjust(0.0f, 0.0f, NavData->AgentHeight * 0.5f);
			const TArray<const UUTPathNode*>& NodeList = NavData->GetAllNodes();
			for (const UUTPathNode* TargetNode : NodeList)
			{
				if (TargetNode != MyNode)
				{
					for (NavNodeRef TargetPoly : TargetNode->Polys)
					{
						FVector TargetLoc = NavData->GetPolyCenter(TargetPoly) + HeightAdjust;
						// ignore points where walking would definitely be faster
						if (bHighHorizontalSpeed || GetWorld()->LineTraceTestByChannel(MyLoc, TargetLoc, ECC_Pawn, FCollisionQueryParams(NAME_None, false, this), WorldResponseParams))
						{
							const float Dist = (TargetLoc - MyLoc).Size();
							bool bPossible = bIsZOnlyJump;
							if (!bPossible)
							{
								// time we have to air control
								float AirControlTime = (JumpTime - EffectiveRestrictedTime) * (Dist / JumpTargetDist);
								// extra distance acquirable via air control
								float AirControlDist2D = FMath::Min<float>(XYSpeed * AirControlTime, 0.5f * AirAccelRate * FMath::Square<float>(AirControlTime));
								// apply air control dist towards target, but remove any in jump direction as the jump pad generally exceeds the character's normal max speed (so air control in that dir would have no effect)
								FVector AirControlAdjustment = (TargetLoc - JumpTargetWorld).GetSafeNormal2D() * AirControlDist2D;
								FVector TowardsJumpDir = JumpDir2D * (JumpDir2D | AirControlAdjustment);
								AirControlAdjustment -= TowardsJumpDir.GetSafeNormal() * FMath::Max<float>(0.0f, TowardsJumpDir.Size() - (XYSpeed - JumpVel.Size2D())); // allow some if jump 2D speed is less than default air speed
								bPossible = (TargetLoc - JumpTargetWorld).Size() < AirControlAdjustment.Size();
							}
							if (bPossible && NavData->JumpTraceTest(MyLoc, TargetLoc, MyPoly, TargetPoly, ScoutShape, XYSpeed, GravityZ, JumpZ, JumpZ, NULL, NULL))
							{
								// TODO: account for MaxFallSpeed
								bool bFound = false;
								for (FUTPathLink& ExistingLink : MyNode->Paths)
								{
									if (ExistingLink.End == TargetNode && ExistingLink.StartEdgePoly == MyPoly)
									{
										ExistingLink.AdditionalEndPolys.Add(TargetPoly);
										bFound = true;
										break;
									}
								}

								if (!bFound)
								{
									UUTReachSpec_JumpPad* JumpSpec = NewObject<UUTReachSpec_JumpPad>(MyNode);
									JumpSpec->JumpPad = this;
									FUTPathLink* NewLink = new(MyNode->Paths) FUTPathLink(MyNode, MyPoly, TargetNode, TargetPoly, JumpSpec, PathSize.Radius, PathSize.Height, ReachFlags);
									for (NavNodeRef SrcPoly : MyNode->Polys)
									{
										NewLink->Distances.Add(NavData->CalcPolyDistance(SrcPoly, MyPoly) + FMath::TruncToInt(1000.0f * JumpTime)); // TODO: maybe Z adjust cost if this target is higher/lower and the jump will end slightly faster/slower?
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

#if WITH_EDITOR
void AUTJumpPad::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

}
void AUTJumpPad::CheckForErrors()
{
	Super::CheckForErrors();

	FVector JumpVelocity = CalculateJumpVelocity(this);
	// figure out default game mode from which we will derive the default character
	TSubclassOf<AGameModeBase> GameClass = GetWorld()->GetWorldSettings()->DefaultGameMode;
	if (GameClass == NULL)
	{
		FString GameClassPath = UGameMapsSettings::GetGlobalDefaultGameMode();
		GameClass = LoadClass<AGameModeBase>(NULL, *GameClassPath, NULL, 0, NULL);
	}
	const ACharacter* DefaultChar = GetDefault<AUTCharacter>();
	
	TSubclassOf<AUTGameMode> UTGameClass = *GameClass;
	if (UTGameClass)
	{
		DefaultChar = Cast<ACharacter>(Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *UTGameClass.GetDefaultObject()->PlayerPawnObject.ToStringReference().ToString(), NULL, LOAD_NoWarn)));
	}
	else
	{
		DefaultChar = Cast<ACharacter>(GameClass.GetDefaultObject()->DefaultPawnClass.GetDefaultObject());
	}

	if (DefaultChar != NULL && DefaultChar->GetCharacterMovement() != NULL)
	{
		JumpVelocity *= FMath::Sqrt(DefaultChar->GetCharacterMovement()->GravityScale);
	}
	// check if velocity is faster than physics will allow
	APhysicsVolume* PhysVolume = (RootComponent != NULL) ? RootComponent->GetPhysicsVolume() : GetWorld()->GetDefaultPhysicsVolume();
	if (JumpVelocity.Size() > PhysVolume->TerminalVelocity)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		Arguments.Add(TEXT("Speed"), FText::AsNumber(JumpVelocity.Size()));
		Arguments.Add(TEXT("TerminalVelocity"), FText::AsNumber(PhysVolume->TerminalVelocity));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(NSLOCTEXT("UTJumpPad", "TerminalVelocityWarning", "{ActorName} : Jump pad speed on default character would be {Speed} but terminal velocity is {TerminalVelocity}!"), Arguments)))
			->AddToken(FMapErrorToken::Create(FName(TEXT("JumpPadTerminalVelocity"))));
	}
}
#endif // WITH_EDITOR

void AUTJumpPad::GetNavigationData(FNavigationRelevantData& Data) const
{
	// the modifier doesn't actually affect costs or reachability but simply causes the poly generation to split around the jump pad's bounds, so it has its own nav mesh region we can put a PathNode on without affecting adjacent areas
	FTransform ModTransform = ActorToWorld();
	ModTransform.RemoveScaling();
	Data.Modifiers.Add(FAreaNavModifier(GetNavigationBounds().GetExtent(), ModTransform, UUTNavArea_Default::StaticClass()));
}