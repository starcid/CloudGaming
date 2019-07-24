// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProjectile.h"
#include "UTProjectileMovementComponent.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Particles/ParticleSystemComponent.h"
#include "UTImpactEffect.h"
#include "UTTeleporter.h"
#include "UTWorldSettings.h"
#include "UTWeaponRedirector.h"
#include "UTProj_WeaponScreen.h"
#include "UTRepulsorBubble.h"
#include "UTTeamDeco.h"
#include "UTDemoNetDriver.h"
#include "UTDemoRecSpectator.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTProjectile, Log, All);

AUTProjectile::AUTProjectile(const class FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	// Use a sphere as a simple collision representation
	CollisionComp = ObjectInitializer.CreateOptionalDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	if (CollisionComp != NULL)
	{
		CollisionComp->InitSphereRadius(0.0f);
		CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");			// Collision profiles are defined in DefaultEngine.ini
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AUTProjectile::OnOverlapBegin);
		CollisionComp->bTraceComplexOnMove = true;
		CollisionComp->bReceivesDecals = false;
		RootComponent = CollisionComp;
	}

	OverlapRadius = 8.f;
	PawnOverlapSphere = ObjectInitializer.CreateOptionalDefaultSubobject<USphereComponent>(this, TEXT("AssistSphereComp"));
	if (PawnOverlapSphere != NULL)
	{
		PawnOverlapSphere->InitSphereRadius(OverlapRadius);
		PawnOverlapSphere->BodyInstance.SetCollisionProfileName("ProjectileOverlap");
		PawnOverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AUTProjectile::OnPawnSphereOverlapBegin);
		PawnOverlapSphere->bTraceComplexOnMove = false; 
		PawnOverlapSphere->bReceivesDecals = false;
		PawnOverlapSphere->SetupAttachment(RootComponent);
		PawnOverlapSphere->bShouldUpdatePhysicsVolume = false;
	}

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = ObjectInitializer.CreateDefaultSubobject<UUTProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AUTProjectile::OnStop);
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AUTProjectile::OnBounce);

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;

	DamageParams.BaseDamage = 20;
	DamageParams.DamageFalloff = 1.0;
	Momentum = 50000.0f;
	InstigatorVelocityPct = 0.f;
	bDamageOnBounce = true;
	InstigatorTeamNum = 255;
	Slomo = 1.f;

	SetReplicates(true);
	bNetTemporary = false;

	InitialReplicationTick.bCanEverTick = true;
	InitialReplicationTick.bTickEvenWhenPaused = true;
	InitialReplicationTick.SetTickFunctionEnable(true);
	ProjectileMovement->PrimaryComponentTick.AddPrerequisite(this, InitialReplicationTick);

	bAlwaysShootable = false;
	bIsEnergyProjectile = false;
	bFakeClientProjectile = false;
	bReplicateUTMovement = false;
	bReplicateMovement = false;
	bMoveFakeToReplicatedPos = true;
	bCanHitTeammates = false;
	SlomoTime = 5.f;

	bInitiallyWarnTarget = true;

	MyFakeProjectile = NULL;
	MasterProjectile = NULL;
	bHasSpawnedFully = false;
	bLowPriorityLight = false;
	bPendingSpecialReward = false;
	StatsHitCredit = 1.f;
	OffsetTime = 0.2f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	NetPriority = 2.f;
	MinNetUpdateFrequency = 100.0f;
}

void AUTProjectile::PreInitializeComponents()
{
	// FIXME: engine bug with blueprints and C++ delegate assignments
	// the previously set delegate for PawnOverlapSphere isn't changed to the new assignment in all blueprints derived from this
	PawnOverlapSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AUTProjectile::OnOverlapBegin);
	PawnOverlapSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AUTProjectile::OnPawnSphereOverlapBegin); // delegate code asserts on duplicates...
	PawnOverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AUTProjectile::OnPawnSphereOverlapBegin);

	Super::PreInitializeComponents();

	if (Instigator != NULL)
	{
		InstigatorController = Instigator->Controller;
	}

	if (PawnOverlapSphere != NULL)
	{
		if (OverlapRadius == 0.0f)
		{
			PawnOverlapSphere->DestroyComponent();
			PawnOverlapSphere = NULL;
		}
		else
		{
			PawnOverlapSphere->SetSphereRadius(OverlapRadius);
		}
	}

	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);
	for (int32 i = 0; i < MeshComponents.Num(); i++)
	{
		//UE_LOG(UT, Warning, TEXT("%s found mesh %s receive decals %d cast shadow %d"), *GetName(), *MeshComponents[i]->GetName(), MeshComponents[i]->bReceivesDecals, MeshComponents[i]->CastShadow);
		MeshComponents[i]->bUseAsOccluder = false;
		MeshComponents[i]->SetCastShadow(false);
		if (bDoVisualOffset && !OffsetVisualComponent)
		{
			OffsetVisualComponent = MeshComponents[i];
			FinalVisualOffset = OffsetVisualComponent->RelativeLocation;
			OffsetVisualComponent->RelativeLocation = InitialVisualOffset;
		}
	}

	OnRep_Instigator();

	if (bDoVisualOffset && !OffsetVisualComponent)
	{
		TArray<UParticleSystemComponent*> PSComponents;
		GetComponents<UParticleSystemComponent>(PSComponents);
		if ((PSComponents.Num() > 0) && PSComponents[0])
		{
			OffsetVisualComponent = PSComponents[0];
			FinalVisualOffset = OffsetVisualComponent->RelativeLocation;
			OffsetVisualComponent->RelativeLocation = InitialVisualOffset;
		}
	}
	OffsetTime = FMath::Max(OffsetTime, 0.01f);

	/*
	if (CollisionComp && (CollisionComp->GetUnscaledSphereRadius() > 0.f))
	{
	UE_LOG(UT, Warning, TEXT("%s has collision radius %f"), *GetName(), CollisionComp->GetUnscaledSphereRadius());
	}

	TArray<ULightComponent*> LightComponents;
	GetComponents<ULightComponent>(LightComponents);
	for (int32 i = 0; i < LightComponents.Num(); i++)
	{
		UE_LOG(UT, Warning, TEXT("%s found LIGHT %s cast shadow %d"), , LightComponents[i]->CastShadows);
	}*/
}

bool AUTProjectile::DisableEmitterLights() const
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	Scalability::FQualityLevels QualitySettings = UserSettings->ScalabilityQuality;
	return (bLowPriorityLight || (QualitySettings.EffectsQuality < 2)) && Instigator && !Cast<APlayerController>(Instigator->GetController());
}

void AUTProjectile::OnRep_Instigator()
{
	if (Instigator != NULL)
	{
		InstigatorTeamNum = GetTeamNum(); // this checks Instigator first

		InstigatorController = Instigator->Controller;
		if (Cast<AUTCharacter>(Instigator))
		{
			((AUTCharacter*)(Instigator))->LastFiredProjectile = this;
		}
	}

	// turn off other player's projectile flight lights at low/medium effects quality
	bool bTurnOffLights = Instigator && DisableEmitterLights();
	TArray<ULightComponent*> LightComponents;
	GetComponents<ULightComponent>(LightComponents);
	for (int32 i = 0; i < LightComponents.Num(); i++)
	{
		if (bTurnOffLights)
		{
			LightComponents[i]->SetVisibility(false);
		}
		LightComponents[i]->SetCastShadows(false);
		LightComponents[i]->bAffectTranslucentLighting = false;
	}

	if (bTurnOffLights)
	{
		TArray<UParticleSystemComponent*> ParticleComponents;
		GetComponents<UParticleSystemComponent>(ParticleComponents);
		for (int32 i = 0; i < ParticleComponents.Num(); i++)
		{
			for (int32 Idx = 0; Idx < ParticleComponents[i]->EmitterInstances.Num(); Idx++)
			{
				if (ParticleComponents[i]->EmitterInstances[Idx])
				{
					ParticleComponents[i]->EmitterInstances[Idx]->LightDataOffset = 0;
				}
			}
		}
	}
}

void AUTProjectile::BeginPlay()
{
	if (IsPendingKillPending())
	{
		// engine bug that we need to do this
		return;
	}
	Super::BeginPlay();

	bHasSpawnedFully = true;
	if (Role == ROLE_Authority)
	{
		ProjectileMovement->Velocity.Z += TossZ;

		UNetDriver* NetDriver = GetNetDriver();
		if (NetDriver != NULL && NetDriver->IsServer())
		{
			InitialReplicationTick.Target = this;
			InitialReplicationTick.RegisterTickFunction(GetLevel());
		}

		if (bInitiallyWarnTarget && InstigatorController != NULL && !bExploded)
		{
			AUTBot* TargetBot = NULL;

			AUTPlayerController* PC = Cast<AUTPlayerController>(InstigatorController);
			if (PC != NULL)
			{
				if (PC->LastShotTargetGuess != NULL)
				{
					TargetBot = Cast<AUTBot>(PC->LastShotTargetGuess->Controller);
				}
			}
			else
			{
				AUTBot* MyBot = Cast<AUTBot>(InstigatorController);
				if (MyBot != NULL && Cast<APawn>(MyBot->GetTarget()) != NULL)
				{
					TargetBot = Cast<AUTBot>(((APawn*)MyBot->GetTarget())->Controller);
				}
			}
			if (TargetBot != NULL)
			{
				TargetBot->ReceiveProjWarning(this);
			}
		}
	}
	else
	{
		AUTPlayerController* MyPlayer = Cast<AUTPlayerController>(InstigatorController ? InstigatorController : GEngine->GetFirstLocalPlayerController(GetWorld()));
		if (MyPlayer)
		{
			// Move projectile to match where it is on server now (to make up for replication time)
			float CatchupTickDelta = MyPlayer->GetPredictionTime();
			if (CatchupTickDelta > 0.f)
			{
				CatchupTick(CatchupTickDelta);
			}

			// look for associated fake client projectile
			AUTProjectile* BestMatch = NULL;
			FVector VelDir = GetVelocity().GetSafeNormal();
			int32 BestMatchIndex = 0;
			float BestDist = 0.f;
			for (int32 i = 0; i < MyPlayer->FakeProjectiles.Num(); i++)
			{
				AUTProjectile* Fake = MyPlayer->FakeProjectiles[i];
				if (!Fake || Fake->IsPendingKillPending())
				{
					MyPlayer->FakeProjectiles.RemoveAt(i, 1);
					i--;
				}
				else if (Fake->GetClass() == GetClass())
				{
					// must share direction unless falling! 
					if (CanMatchFake(Fake, VelDir))
					{
						if (BestMatch)
						{
							// see if new one is better
							float NewDist = (Fake->GetActorLocation() - GetActorLocation()).SizeSquared();
							if (BestDist > NewDist)
							{
								BestMatch = Fake;
								BestMatchIndex = i;
								BestDist = NewDist;
							}
						}
						else
						{
							BestMatch = Fake;
							BestMatchIndex = i;
							BestDist = (BestMatch->GetActorLocation() - GetActorLocation()).SizeSquared();
						}
					}
				}
			}
			if (BestMatch)
			{
				MyPlayer->FakeProjectiles.RemoveAt(BestMatchIndex, 1);
				BeginFakeProjectileSynch(BestMatch);
			}
			else if (MyPlayer != NULL && MyPlayer->bIsDebuggingProjectiles && MyPlayer->GetPredictionTime() > 0.0f)
			{
				// debug logging of failed match
				UE_LOG(UT, Warning, TEXT("%s FAILED to find fake projectile match with velocity %f %f %f"), *GetName(), GetVelocity().X, GetVelocity().Y, GetVelocity().Z);
				for (int32 i = 0; i < MyPlayer->FakeProjectiles.Num(); i++)
				{
					AUTProjectile* Fake = MyPlayer->FakeProjectiles[i];
					if (Fake)
					{
						UE_LOG(UT, Warning, TEXT("     - REJECTED potential match %s DP %f"), *Fake->GetName(), (Fake->GetVelocity().GetSafeNormal() | VelDir));
					}
				}
			}
		}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		else
		{
			APlayerController* FirstPlayer = GEngine->GetFirstLocalPlayerController(GetWorld());
			UE_LOG(UT, Warning, TEXT("%s spawned with no local player found!  Instigator %s First Local Player %s"), *GetName(), InstigatorController ? *InstigatorController->GetName() : TEXT("NONE"), FirstPlayer ? *FirstPlayer->GetName() : TEXT("NONE"));
			TArray<APlayerController*> PlayerControllers;
			GEngine->GetAllLocalPlayerControllers(PlayerControllers);
			for (APlayerController* PlayerController : PlayerControllers)
			{
				UE_LOG(UT, Warning, TEXT("Found local player %s"), PlayerController ? *PlayerController->GetName() : TEXT("None"));
			}
		}
#endif
	}
}

bool AUTProjectile::CanMatchFake(AUTProjectile* InFakeProjectile, const FVector& VelDir) const
{
	return (ProjectileMovement->ProjectileGravityScale > 0.f) || ((InFakeProjectile->GetVelocity().GetSafeNormal() | VelDir) > 0.95f);
}

void AUTProjectile::CatchupTick(float CatchupTickDelta)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->TickComponent(CatchupTickDelta, LEVELTICK_All, NULL);
	}
}

void AUTProjectile::BeginFakeProjectileSynch(AUTProjectile* InFakeProjectile)
{
	if (InFakeProjectile->IsPendingKillPending() || InFakeProjectile->bExploded)
	{
		// Fake projectile is no longer valid to sync to
		return;
	}

	// @TODO FIXMESTEVE - teleport to bestmatch location and interpolate?
	MyFakeProjectile = InFakeProjectile;
	MyFakeProjectile->MasterProjectile = this;

	float Error = (GetActorLocation() - MyFakeProjectile->GetActorLocation()).Size();
	if (((GetActorLocation() - MyFakeProjectile->GetActorLocation()) | MyFakeProjectile->GetVelocity()) > 0.f)
	{
		Error *= -1.f;
	}
	//UE_LOG(UT, Warning, TEXT("%s CORRECTION %f in msec %f"), *GetName(), Error, 1000.f * Error/GetVelocity().Size());

	if (bMoveFakeToReplicatedPos)
	{
		MyFakeProjectile->ReplicatedMovement.Location = GetActorLocation();
		MyFakeProjectile->ReplicatedMovement.Rotation = GetActorRotation();
		MyFakeProjectile->PostNetReceiveLocationAndRotation();
	}
	else
	{
		ReplicatedMovement.Location = MyFakeProjectile->GetActorLocation();
		ReplicatedMovement.Rotation = MyFakeProjectile->GetActorRotation();
		PostNetReceiveLocationAndRotation();
	}
	AUTPlayerController* MyPlayer = Cast<AUTPlayerController>(InstigatorController ? InstigatorController : GEngine->GetFirstLocalPlayerController(GetWorld()));
	if (MyPlayer && (GetLifeSpan() != 0.f))
	{
		// remove forward prediction from lifespan
		float CatchupTickDelta = MyPlayer->GetPredictionTime();
		SetLifeSpan(FMath::Max(0.001f, GetLifeSpan() - CatchupTickDelta));
	}
	MyFakeProjectile->SetLifeSpan(GetLifeSpan());
	if (bNetTemporary)
	{
		// @TODO FIXMESTEVE - will have issues if there are replicated properties that haven't been received yet
		MyFakeProjectile = NULL;
		Destroy();
		//UE_LOG(UT, Warning, TEXT("%s DESTROY pending kill %d"), *GetName(), IsPendingKillPending());
	}
	else
	{
		// @TODO FIXMESTEVE Can I move components instead of having two actors?
		// @TODO FIXMESTEVE if not, should interp fake projectile to my location instead of teleporting?
		SetActorHiddenInGame(true);
		TArray<USceneComponent*> Components;
		GetComponents<USceneComponent>(Components);
		for (int32 i = 0; i < Components.Num(); i++)
		{
			Components[i]->SetVisibility(false);
		}
	}
}

void AUTProjectile::SendInitialReplication()
{
	// force immediate replication for projectiles with extreme speed or radial effects
	// this prevents clients from being hit by invisible projectiles in almost all cases, because it'll exist locally before it has even been moved
	UNetDriver* NetDriver = GetNetDriver();
	if (NetDriver != NULL && NetDriver->IsServer() && !IsPendingKillPending() && (ProjectileMovement->Velocity.Size() >= 7500.0f || DamageParams.OuterRadius > 0.0f))
	{
		NetDriver->ReplicationFrame++;
		for (int32 i = 0; i < NetDriver->ClientConnections.Num(); i++)
		{
			if (NetDriver->ClientConnections[i]->State == USOCK_Open && NetDriver->ClientConnections[i]->PlayerController != NULL && NetDriver->ClientConnections[i]->IsNetReady(0))
			{
				AActor* ViewTarget = NetDriver->ClientConnections[i]->PlayerController->GetViewTarget();
				if (ViewTarget == NULL)
				{
					ViewTarget = NetDriver->ClientConnections[i]->PlayerController;
				}
				FVector ViewLocation = ViewTarget->GetActorLocation();
				{
					FRotator ViewRotation = NetDriver->ClientConnections[i]->PlayerController->GetControlRotation();
					NetDriver->ClientConnections[i]->PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
				}
				// Workaround to skip deprecation warning where it calls the PlayerController version of this function
				if (IsNetRelevantFor(static_cast<AActor*>(NetDriver->ClientConnections[i]->PlayerController), ViewTarget, ViewLocation))
				{
					UActorChannel* Ch = NetDriver->ClientConnections[i]->ActorChannels.FindRef(this);
					if (Ch == NULL)
					{
						// can't - protected: if (NetDriver->IsLevelInitializedForActor(this, NetDriver->ClientConnections[i]))
						if (NetDriver->ClientConnections[i]->ClientWorldPackageName == GetWorld()->GetOutermost()->GetFName() && NetDriver->ClientConnections[i]->ClientHasInitializedLevelFor(this))
						{
							Ch = (UActorChannel *)NetDriver->ClientConnections[i]->CreateChannel(CHTYPE_Actor, 1);
							if (Ch != NULL)
							{
								Ch->SetChannelActor(this);
							}
						}
					}
					if (Ch != NULL && Ch->OpenPacketId.First == INDEX_NONE)
					{
						// bIsReplicatingActor being true should be impossible but let's be sure
						if (!Ch->bIsReplicatingActor)
						{
							Ch->ReplicateActor();

							// force a replicated location update at the end of the frame after the physics as well
							bForceNextRepMovement = true;
						}
					}
				}
			}
		}
	}
}

void AUTProjectile::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (&ThisTickFunction == &InitialReplicationTick)
	{
		SendInitialReplication();
		InitialReplicationTick.UnRegisterTickFunction();
	}
	else
	{
		if (OffsetVisualComponent)
		{
			float Pct = FMath::Max((GetWorld()->GetTimeSeconds() - CreationTime) / OffsetTime, 0.f);
			if (Pct >= 1.f)
			{
				OffsetVisualComponent->RelativeLocation = FinalVisualOffset;
				OffsetVisualComponent = nullptr;
			}
			else
			{
				OffsetVisualComponent->RelativeLocation = Pct*FinalVisualOffset + (1.f - Pct)*InitialVisualOffset;
			}
		}
		Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	}
}

void AUTProjectile::NotifyClientSideHit(AUTPlayerController* InstigatedBy, FVector HitLocation, AActor* DamageCauser, int32 Damage)
{
}

void AUTProjectile::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	DOREPLIFETIME(AActor, bHidden);
	DOREPLIFETIME(AActor, bTearOff);
	DOREPLIFETIME(AActor, bCanBeDamaged);

	// POLGE TODO: Fix the issues with this being private
	//DOREPLIFETIME(AActor, AttachmentReplication);

	DOREPLIFETIME(AActor, Instigator);
	DOREPLIFETIME_CONDITION(AUTProjectile, UTProjReplicatedMovement, COND_SimulatedOrPhysics);
	DOREPLIFETIME(AUTProjectile, Slomo);
}

void AUTProjectile::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	if ((bForceNextRepMovement || bReplicateUTMovement) && (Role == ROLE_Authority))
	{
		GatherCurrentMovement();
		bForceNextRepMovement = false;
	}
}

void AUTProjectile::GatherCurrentMovement()
{
	/* @TODO FIXMESTEVE support projectiles uing rigid body physics
	UPrimitiveComponent* RootPrimComp = Cast<UPrimitiveComponent>(GetRootComponent());
	if (RootPrimComp && RootPrimComp->IsSimulatingPhysics())
	{
		FRigidBodyState RBState;
		RootPrimComp->GetRigidBodyState(RBState);

		ReplicatedMovement.FillFrom(RBState);
	}
	else 
	*/
	if (RootComponent != NULL)
	{
		// If we are attached, don't replicate absolute position
		if (RootComponent->GetAttachParent() != NULL)
		{
			Super::GatherCurrentMovement();
		}
		else
		{
			UTProjReplicatedMovement.Location = RootComponent->GetComponentLocation();
			UTProjReplicatedMovement.Rotation = RootComponent->GetComponentRotation();
			UTProjReplicatedMovement.LinearVelocity = GetVelocity();
		}
	}
}

void AUTProjectile::OnRep_UTProjReplicatedMovement()
{
	if (Role == ROLE_SimulatedProxy)
	{
		//ReplicatedAccel = UTReplicatedMovement.Acceleration;
		ReplicatedMovement.Location = UTProjReplicatedMovement.Location;
		ReplicatedMovement.Rotation = UTProjReplicatedMovement.Rotation;
		ReplicatedMovement.LinearVelocity = UTProjReplicatedMovement.LinearVelocity;
		ReplicatedMovement.AngularVelocity = FVector(0.f);
		ReplicatedMovement.bSimulatedPhysicSleep = false;
		ReplicatedMovement.bRepPhysics = false;

		OnRep_ReplicatedMovement();
	}
}

void AUTProjectile::PostNetReceiveLocationAndRotation()
{
	if (!bMoveFakeToReplicatedPos && MyFakeProjectile)
	{
		// use fake proj position
		ReplicatedMovement.Location = MyFakeProjectile->GetActorLocation();
		ReplicatedMovement.Rotation = MyFakeProjectile->GetActorRotation();
	}
	Super::PostNetReceiveLocationAndRotation();
	if (!bMoveFakeToReplicatedPos && MyFakeProjectile)
	{
		return;
	}

	// forward predict to get to position on server now
	if (!bFakeClientProjectile)
	{
		AUTPlayerController* MyPlayer = Cast<AUTPlayerController>(InstigatorController ? InstigatorController : GEngine->GetFirstLocalPlayerController(GetWorld()));
		if (MyPlayer)
		{
			float CatchupTickDelta = MyPlayer->GetPredictionTime();
			if ((CatchupTickDelta > 0.f) && ProjectileMovement)
			{
				ProjectileMovement->TickComponent(CatchupTickDelta, LEVELTICK_All, NULL);
			}
		}
	}

	if (MyFakeProjectile)
	{
		MyFakeProjectile->ReplicatedMovement.Location = GetActorLocation();
		MyFakeProjectile->ReplicatedMovement.Rotation = GetActorRotation();
		MyFakeProjectile->PostNetReceiveLocationAndRotation();
	}
	else if (Role != ROLE_Authority)
	{
		// tick particle systems for e.g. SpawnPerUnit trails
		if (!bTearOff && !bExploded) // if torn off ShutDown() will do this
		{
			TArray<USceneComponent*> Components;
			GetComponents<USceneComponent>(Components);
			for (int32 i = 0; i < Components.Num(); i++)
			{
				UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Components[i]);
				if (PSC != NULL)
				{
					PSC->TickComponent(0.0f, LEVELTICK_All, NULL);
				}
			}
		}
	}
}

void AUTProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	ProjectileMovement->Velocity = NewVelocity;
	if (MyFakeProjectile)
	{
		MyFakeProjectile->ProjectileMovement->Velocity = NewVelocity;
	}
}

void AUTProjectile::OnRep_Slomo()
{
	CustomTimeDilation = Slomo;
	bForceNextRepMovement = true;
	if (Slomo != 1.f)
	{
		GetWorldTimerManager().SetTimer(SlomoTimerHandle, this, &AUTProjectile::EndSlomo, SlomoTime);
	}
}

void AUTProjectile::EndSlomo()
{
	Slomo = 1.f;
	OnRep_Slomo();
}

void AUTProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bInOverlap)
	{
		TGuardValue<bool> OverlapGuard(bInOverlap, true);

		UE_LOG(LogUTProjectile, Verbose, TEXT("%s::OnOverlapBegin OtherActor:%s bFromSweep:%d"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"), int32(bFromSweep));

		FHitResult Hit;

		if (bFromSweep)
		{
			Hit = SweepResult;
		}
		else if (CollisionComp != NULL)
		{
			USphereComponent* TestComp = (PawnOverlapSphere != NULL && PawnOverlapSphere->GetUnscaledSphereRadius() > CollisionComp->GetUnscaledSphereRadius()) ? PawnOverlapSphere : CollisionComp;
			OtherComp->SweepComponent(Hit, GetActorLocation() - GetVelocity() * 10.f, GetActorLocation() + GetVelocity(), FQuat::Identity, TestComp->GetCollisionShape(), TestComp->bTraceComplexOnMove);
		}
		else
		{
			OtherComp->LineTraceComponent(Hit, GetActorLocation() - GetVelocity() * 10.f, GetActorLocation() + GetVelocity(), FCollisionQueryParams(GetClass()->GetFName(), false, this));
		}

		ProcessHit(OtherActor, OtherComp, Hit.Location, Hit.Normal);
	}
}

void AUTProjectile::OnPawnSphereOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != nullptr)
	{
		FVector OtherLocation;
		if (bFromSweep)
		{
			OtherLocation = SweepResult.Location;
		}
		else
		{
			OtherLocation = OtherActor->GetActorLocation();
		}

		FCollisionQueryParams Params(FName(TEXT("PawnSphereOverlapTrace")), true, this);
		Params.AddIgnoredActor(OtherActor);

		// since PawnOverlapSphere doesn't hit blocking objects, it is possible it is touching a target through a wall
		// make sure that the hit is valid before proceeding
		if (!GetWorld()->LineTraceTestByChannel(OtherLocation, GetActorLocation(), COLLISION_TRACE_WEAPON, Params))
		{
			OnOverlapBegin(CollisionComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
		}
	}
}

void AUTProjectile::OnStop(const FHitResult& Hit)
{
	ProcessHit(Hit.Actor.Get(), Hit.Component.Get(), Hit.Location, Hit.Normal);
}

void AUTProjectile::OnBounce(const struct FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (bDamageOnBounce && ImpactResult.Actor.IsValid() && ImpactResult.Actor->bCanBeDamaged)
	{
		ProcessHit(ImpactResult.Actor.Get(), ImpactResult.Component.Get(), ImpactResult.ImpactPoint, ImpactResult.ImpactNormal);
		return;
	}
	if ((MyFakeProjectile == NULL) && (Cast<AUTProjectile>(ImpactResult.Actor.Get()) == NULL || InteractsWithProj(Cast<AUTProjectile>(ImpactResult.Actor.Get()))))
	{
		InitialVisualOffset = FinalVisualOffset;

		// Spawn bounce effect
		if (GetNetMode() != NM_DedicatedServer)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BounceEffect, ImpactResult.Location, ImpactResult.ImpactNormal.Rotation(), true);
		}
		// Play bounce sound
		if (BounceSound != NULL)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), BounceSound, this, SRT_IfSourceNotReplicated, false, FVector::ZeroVector, NULL, NULL, true, SAT_WeaponFoley);
		}
	}

	if (!bCanHitInstigator)
	{
		bCanHitInstigator = true;
		if (Instigator != NULL && IsOverlappingActor(Instigator))
		{
			ProcessHit(Instigator, Cast<UPrimitiveComponent>(Instigator->GetRootComponent()), GetActorLocation(), ImpactVelocity.GetSafeNormal());
		}
	}
}

bool AUTProjectile::InteractsWithProj(AUTProjectile* OtherProj)
{
	if ((bAlwaysShootable || OtherProj->bAlwaysShootable || (bIsEnergyProjectile && OtherProj->bIsEnergyProjectile)) && !bFakeClientProjectile && !OtherProj->bFakeClientProjectile)
	{
		// interact if not same team
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		return  GS == nullptr || !GS->OnSameTeam(this, OtherProj);
	}
	return false;
}

void AUTProjectile::InitFakeProjectile(AUTPlayerController* OwningPlayer)
{
	bFakeClientProjectile = true;
	if (OwningPlayer)
	{
		OwningPlayer->FakeProjectiles.Add(this);
	}
}

bool AUTProjectile::ShouldIgnoreHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	// don't blow up on non-blocking volumes
	// special case not blowing up on teleporters on overlap so teleporters have the option to teleport the projectile
	// don't blow up on weapon redirectors that teleport weapons fire
	// don't blow up from our side on weapon shields; let the shield do that so it can change damage/kill credit
	// ignore client-side actors if will bounce
	// special case not blowing up on Repulsor bubble so that we can reflect / absorb projectiles
	AUTTeamDeco* Deco = Cast<AUTTeamDeco>(OtherActor);
	if (Deco && !Deco->bBlockTeamProjectiles)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		return GS != nullptr && GS->OnSameTeam(this, Deco);
	}
	return (((Cast<AUTTeleporter>(OtherActor) != NULL || Cast<AVolume>(OtherActor) != NULL) && !GetVelocity().IsZero())
		|| (Cast<AUTRepulsorBubble>(OtherActor) != NULL)
		|| (Cast<AUTProjectile>(OtherActor) != NULL && !InteractsWithProj(Cast<AUTProjectile>(OtherActor)))
		|| Cast<AUTProj_WeaponScreen>(OtherActor) != NULL
		|| Cast<AUTGib>(OtherActor) != NULL
		|| ((Role != ROLE_Authority) && OtherActor && OtherActor->bTearOff));
}

void AUTProjectile::ProcessHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	UE_LOG(UT, Verbose, TEXT("%s::ProcessHit fake %d has master %d has fake %d OtherActor:%s"), *GetName(), bFakeClientProjectile, (MasterProjectile != NULL), (MyFakeProjectile != NULL), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	// note: on clients we assume spawn time impact is invalid since in such a case the projectile would generally have not survived to be replicated at all
	if (OtherActor != this && (OtherActor != Instigator || Instigator == NULL || bCanHitInstigator) && OtherComp != NULL && !bExploded && (Role == ROLE_Authority || bHasSpawnedFully))
	{
		if (ShouldIgnoreHit(OtherActor, OtherComp))
		{
			if ((Role != ROLE_Authority) && OtherActor && OtherActor->bTearOff)
			{ 
				DamageImpactedActor(OtherActor, OtherComp, HitLocation, HitNormal);
			}
		}
		else
		{
			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			if (!bCanHitTeammates && GS != nullptr && !GS->bTeamProjHits && Cast<AUTCharacter>(OtherActor) != nullptr && Instigator != OtherActor && GS->OnSameTeam(OtherActor, this))
			{
				// ignore team hits
				return;
			}
			if (MyFakeProjectile && !MyFakeProjectile->IsPendingKillPending())
			{
				MyFakeProjectile->ProcessHit_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
				Destroy();
				return;
			}
			if (OtherActor != NULL)
			{
				DamageImpactedActor(OtherActor, OtherComp, HitLocation, HitNormal);
			}

			ImpactedActor = OtherActor;
			Explode(HitLocation, HitNormal, OtherComp);
			ImpactedActor = NULL;

			if (Cast<AUTProjectile>(OtherActor) != NULL)
			{
				// since we'll probably be destroyed or lose collision here, make sure we trigger the other projectile so shootable projectiles colliding is consistent (both explode)

				UPrimitiveComponent* MyCollider = CollisionComp;
				if (CollisionComp == NULL || CollisionComp->GetCollisionObjectType() != COLLISION_PROJECTILE_SHOOTABLE)
				{
					// our primary collision component isn't the shootable one; try to find one that is
					TArray<UPrimitiveComponent*> Components;
					GetComponents<UPrimitiveComponent>(Components);
					for (int32 i = 0; i < Components.Num(); i++)
					{
						if (Components[i]->GetCollisionObjectType() == COLLISION_PROJECTILE_SHOOTABLE)
						{
							MyCollider = Components[i];
							break;
						}
					}
				}

				((AUTProjectile*)OtherActor)->ProcessHit(this, MyCollider, HitLocation, -HitNormal);
			}
		}
	}
}

void AUTProjectile::DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	if (bFakeClientProjectile)
	{
		return;
	}
	AController* ResolvedInstigator = InstigatorController;
	TSubclassOf<UDamageType> ResolvedDamageType = MyDamageType;
	bool bSameTeamDamage = false;
	if (FFInstigatorController != NULL && InstigatorController != NULL)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS != NULL && GS->OnSameTeam(OtherActor, InstigatorController))
		{
			bSameTeamDamage = true;
			ResolvedInstigator = FFInstigatorController;
			if (FFDamageType != NULL)
			{
				ResolvedDamageType = FFDamageType;
			}
		}
	}
	if ((Role == ROLE_Authority) && (HitsStatsName != NAME_None) && !bSameTeamDamage)
	{
		AUTPlayerState* PS = InstigatorController ? Cast<AUTPlayerState>(InstigatorController->PlayerState) : NULL;
		if (PS)
		{
			PS->ModifyStatsValue(HitsStatsName, StatsHitCredit);
		}
	}

	// treat as point damage if projectile has no radius
	if (DamageParams.OuterRadius > 0.0f)
	{
		FUTRadialDamageEvent Event;
		Event.BaseMomentumMag = Momentum;
		Event.Params = GetDamageParams(OtherActor, HitLocation, Event.BaseMomentumMag);
		Event.Params.MinimumDamage = Event.Params.BaseDamage; // force full damage for direct hit
		Event.DamageTypeClass = ResolvedDamageType;
		Event.Origin = HitLocation;
		new(Event.ComponentHits) FHitResult(OtherActor, OtherComp, HitLocation, HitNormal);
		Event.ComponentHits[0].TraceStart = HitLocation - GetVelocity();
		Event.ComponentHits[0].TraceEnd = HitLocation + GetVelocity();
		Event.ShotDirection = GetVelocity().GetSafeNormal();
		Event.BaseMomentumMag = ((Momentum == 0.f) && Cast<AUTCharacter>(OtherActor) && ((AUTCharacter*)(OtherActor))->IsDead()) ? 20000.f : Momentum;
		OtherActor->TakeDamage(Event.Params.BaseDamage, Event, ResolvedInstigator, this);
	}
	else
	{
		FUTPointDamageEvent Event;
		float AdjustedMomentum = Momentum;
		Event.Damage = GetDamageParams(OtherActor, HitLocation, AdjustedMomentum).BaseDamage;
		Event.DamageTypeClass = ResolvedDamageType;
		Event.HitInfo = FHitResult(OtherActor, OtherComp, HitLocation, HitNormal);
		Event.ShotDirection = GetVelocity().GetSafeNormal();
		AdjustedMomentum = ((AdjustedMomentum == 0.f) && Cast<AUTCharacter>(OtherActor) && ((AUTCharacter*)(OtherActor))->IsDead()) ? 20000.f : Momentum; 
		Event.Momentum = Event.ShotDirection * AdjustedMomentum;
		OtherActor->TakeDamage(Event.Damage, Event, ResolvedInstigator, this);
	}
}

void AUTProjectile::Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp)
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		UDemoNetDriver* DemoDriver = GetWorld()->DemoNetDriver;
		if (DemoDriver)
		{
			AUTDemoRecSpectator* DemoRecSpec = Cast<AUTDemoRecSpectator>(DemoDriver->SpectatorController);
			if (DemoRecSpec && (GetWorld()->GetTimeSeconds() - DemoRecSpec->LastKillcamSeekTime) < 2.0f)
			{
				bExploded = true;
				Destroy();
				return;
			}
		}
	}

	if (!bExploded)
	{
		bExploded = true;
		float AdjustedMomentum = Momentum;
		FRadialDamageParams AdjustedDamageParams = ((MasterProjectile != NULL) ? MasterProjectile : this)->GetDamageParams(NULL, HitLocation, AdjustedMomentum);
		if (!bFakeClientProjectile)
		{
			if (AdjustedDamageParams.OuterRadius > 0.0f)
			{
				TArray<AActor*> IgnoreActors;
				if (ImpactedActor != NULL)
				{
					IgnoreActors.Add(ImpactedActor);
				}
				TArray<FVector> AltOrigins;
				AltOrigins.Add(HitLocation + HitNormal * GetDefault<AUTCharacter>()->UTCharacterMovement->MaxStepHeight);
				if (!ProjectileMovement->Velocity.IsZero())
				{
					AltOrigins.Add(HitLocation - ProjectileMovement->Velocity.GetSafeNormal() * GetDefault<AUTCharacter>()->UTCharacterMovement->MaxStepHeight);
				}
				StatsHitCredit = 0.f;
				UUTGameplayStatics::UTHurtRadius(this, AdjustedDamageParams.BaseDamage, AdjustedDamageParams.MinimumDamage, AdjustedMomentum, HitLocation + HitNormal, AdjustedDamageParams.InnerRadius, AdjustedDamageParams.OuterRadius, AdjustedDamageParams.DamageFalloff,
					MyDamageType, IgnoreActors, this, InstigatorController, FFInstigatorController, FFDamageType, 0.0f, &AltOrigins);
				if ((Role == ROLE_Authority) && (HitsStatsName != NAME_None))
				{
					AUTPlayerState* PS = InstigatorController ? Cast<AUTPlayerState>(InstigatorController->PlayerState) : NULL;
					if (PS)
					{
						PS->ModifyStatsValue(HitsStatsName, StatsHitCredit / AdjustedDamageParams.BaseDamage);
					}
				}
			}
			if (Role == ROLE_Authority)
			{
				bTearOff = true;
				bReplicateUTMovement = true; // so position of explosion is accurate even if flight path was a little off
			}
		}

		// explosion effect unless I have a fake projectile doing it for me
		if (MyFakeProjectile == NULL && ExplosionEffects != NULL)
		{
			if (!InstigatorController && Instigator)
			{
				InstigatorController = Instigator->GetController();
			}
			ExplosionEffects.GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(HitNormal.Rotation(), HitLocation), HitComp, this, InstigatorController, SRT_IfSourceNotReplicated, FImpactEffectNamedParameters(AdjustedDamageParams.OuterRadius));
		}
		ShutDown();
	}
}

void AUTProjectile::Destroyed()
{
	if (MyFakeProjectile)
	{
		MyFakeProjectile->Destroy();
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::Destroyed();
}

void AUTProjectile::ShutDown()
{
	if (MyFakeProjectile)
	{
		MyFakeProjectile->ShutDown();
	}
	if (!IsPendingKillPending())
	{
		SetActorEnableCollision(false);
		ProjectileMovement->SetActive(false);
		// hide components that aren't particle systems; deactivate particle systems so they die off naturally; stop ambient sounds
		bool bFoundParticles = false;
		TArray<USceneComponent*> Components;
		GetComponents<USceneComponent>(Components);
		for (int32 i = 0; i < Components.Num(); i++)
		{
			UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Components[i]);
			if (PSC != NULL)
			{
				// tick the particles one last time for e.g. SpawnPerUnit effects (particularly noticeable improvement for fast moving projectiles)
				PSC->TickComponent(0.0f, LEVELTICK_All, NULL);
				PSC->DeactivateSystem();
				PSC->bAutoDestroy = true;
				bFoundParticles = true;
			}
			else
			{
				UAudioComponent* Audio = Cast<UAudioComponent>(Components[i]);
				if (Audio != NULL)
				{
					// only stop looping (ambient) sounds - note that the just played explosion sound may be encountered here
					if (Audio->Sound != NULL && Audio->Sound->GetDuration() >= INDEFINITELY_LOOPING_DURATION)
					{
						Audio->Stop();
					}
				}
				else
				{
					Components[i]->SetHiddenInGame(true);
					Components[i]->SetVisibility(false);
				}
			}
		}
		// if some particles remain, defer destruction a bit to give them time to die on their own
		SetLifeSpan((bFoundParticles && GetNetMode() != NM_DedicatedServer) ? 2.0f : 0.2f);

		OnShutdown();
	}

	bExploded = true;
}

void AUTProjectile::TornOff()
{
	if (bExploded)
	{
		ShutDown(); // make sure it took effect; LifeSpan in particular won't unless we're authority
	}
	else
	{
		Explode(GetActorLocation(), FVector(1.0f, 0.0f, 0.0f));
	}
}

FRadialDamageParams AUTProjectile::GetDamageParams_Implementation(AActor* OtherActor, const FVector& HitLocation, float& OutMomentum) const
{
	OutMomentum = Momentum;
	return DamageParams;
}

float AUTProjectile::StaticGetTimeToLocation(const FVector& TargetLoc, const FVector& StartLoc) const
{
	const float Dist = (TargetLoc - StartLoc).Size();
	if (ProjectileMovement == NULL)
	{
		UE_LOG(UT, Warning, TEXT("Unable to calculate time to location for %s; please implement GetTimeToLocation()"), *GetName());
		return 0.0f;
	}
	else
	{
		UUTProjectileMovementComponent* UTMovement = Cast<UUTProjectileMovementComponent>(ProjectileMovement);
		if (UTMovement == NULL || UTMovement->AccelRate == 0.0f)
		{
			return (Dist / ProjectileMovement->InitialSpeed);
		}
		else
		{

			// figure out how long it would take if we accelerated the whole way
			float ProjTime = (-UTMovement->InitialSpeed + FMath::Sqrt(FMath::Square<float>(UTMovement->InitialSpeed) - (2.0 * UTMovement->AccelRate * -Dist))) / UTMovement->AccelRate;
			// figure out how long it will actually take to accelerate to max speed
			float AccelTime = (UTMovement->MaxSpeed - UTMovement->InitialSpeed) / UTMovement->AccelRate;
			if (ProjTime > AccelTime)
			{
				// figure out distance traveled while accelerating to max speed
				const float AccelDist = (UTMovement->MaxSpeed * AccelTime) + (0.5 * UTMovement->AccelRate * FMath::Square<float>(AccelTime));
				// add time to accelerate to max speed plus time to travel remaining dist at max speed
				ProjTime = AccelTime + ((Dist - AccelDist) / UTMovement->MaxSpeed);
			}
			return ProjTime;
		}
	}
}
float AUTProjectile::GetTimeToLocation(const FVector& TargetLoc) const
{
	ensure(!IsTemplate()); // if this trips you meant to call StaticGetTimeToLocation()

	return StaticGetTimeToLocation(TargetLoc, GetActorLocation());
}

float AUTProjectile::GetMaxDamageRadius_Implementation() const
{
	return DamageParams.OuterRadius;
}

void AUTProjectile::PrepareForIntermission()
{
	if (ProjectileMovement != nullptr)
	{
		ProjectileMovement->StopMovementImmediately();
	}
	SetLifeSpan(10.f*GetLifeSpan());
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		UAudioComponent* Audio = Cast<UAudioComponent>(Components[i]);
		if (Audio != NULL)
		{
			Audio->Stop();
		}
	}
}


