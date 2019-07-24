// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProjectileMovementComponent.h"
#include "UTProj_FlakShard.h"
#include "Particles/ParticleSystemComponent.h"

AUTProj_FlakShard::AUTProj_FlakShard(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mesh = ObjectInitializer.CreateOptionalDefaultSubobject<UStaticMeshComponent>(this, FName(TEXT("Mesh")));
	if (Mesh != NULL)
	{
		Mesh->SetCollisionProfileName(FName(TEXT("NoCollision")));
		Mesh->SetupAttachment(RootComponent);
		Mesh->bReceivesDecals = false;
		Mesh->bUseAsOccluder = false;
	}

	Trail = ObjectInitializer.CreateOptionalDefaultSubobject<UParticleSystemComponent>(this, FName(TEXT("Trail")));
	if (Trail != NULL)
	{
		Trail->SetupAttachment(Mesh);
	}

	HeatFadeTime = 1.0f;
	HotTrailColor = FLinearColor(2.0f, 1.65f, 0.65f, 1.0f);
	ColdTrailColor = FLinearColor(0.165f, 0.135f, 0.097f, 0.0f);

	ProjectileMovement->InitialSpeed = 6500.0f;
	ProjectileMovement->MaxSpeed = 8000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 0.0f;

	// Damage
	DamageParams.BaseDamage = 15.0f;
	DamageParams.MinimumDamage = 5.0f;
	Momentum = 24000.f;

	DamageAttenuation = 15.0f;
	DamageAttenuationDelay = 0.5f;
	MinDamageSpeed = 800.f;

	SelfDamageAttenuation = 25.0f;
	SelfDamageAttenuationDelay = 0.11f;

	InitialLifeSpan = 2.f;
	BouncesRemaining = 2;
	FirstBounceDamping = 0.9f;
	BounceDamping = 0.75f;
	BounceDamagePct = 0.5f;
	RandomBounceCone = 0.3f;
	FullGravityDelay = 0.5f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bNetTemporary = true;
	StatsHitCredit = 0.111f;
	OverlapRadius = 20.f;
	MaxOverlapRadius = 40.f;
	FinalOverlapRadius = 8.f;
	RadiusShrinkRate = 150.f;
	RadiusGrowthRate = 1200.f;
	bGrowOverlap = true;
}

void AUTProj_FlakShard::DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(OtherActor);
	if (UTC && FleshImpactSound)
	{
		UUTGameplayStatics::UTPlaySound(GetWorld(), FleshImpactSound, UTC, SRT_IfSourceNotReplicated, false, FVector::ZeroVector, NULL, NULL, true, SAT_PainSound);
	}
	Super::DamageImpactedActor_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
}

void AUTProj_FlakShard::CatchupTick(float CatchupTickDelta)
{
	Super::CatchupTick(CatchupTickDelta);
	FullGravityDelay -= CatchupTickDelta;
	// @TODO FIXMESTEVE - not synchronizing this correctly on other clients unless we replicate it.  Needed?
}

void AUTProj_FlakShard::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Mesh != NULL)
	{
		Mesh->SetRelativeRotation(FRotator(360.0f * FMath::FRand(), 360.0f * FMath::FRand(), 360.0f * FMath::FRand()));
		if (MeshMI == NULL)
		{
			MeshMI = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		}
	}
}

void AUTProj_FlakShard::ProcessHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	if (GetVelocity().Size() < MinDamageSpeed)
	{
		ShutDown();
	}
	else
	{
		Super::ProcessHit_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
	}
}

void AUTProj_FlakShard::OnBounce(const struct FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (ImpactResult.Actor.IsValid() && ImpactResult.Actor->bCanBeDamaged)
	{
		ProcessHit(ImpactResult.Actor.Get(), ImpactResult.Component.Get(), ImpactResult.ImpactPoint, ImpactResult.ImpactNormal);
		return;
	}
	InitialVisualOffset = FinalVisualOffset;
	bGrowOverlap = false;
	if (GetWorld()->GetTimeSeconds() - CreationTime > 2.f * FullGravityDelay)
	{
		ShutDown();
		return;
	}
	float CurrentBounceDamping = (ProjectileMovement->ProjectileGravityScale == 0.f) ? FirstBounceDamping : BounceDamping;
	Super::OnBounce(ImpactResult, ImpactVelocity);

	if (PawnOverlapSphere != NULL)
	{
		PawnOverlapSphere->SetSphereRadius(FinalOverlapRadius, false);
	}

	// manually handle bounce velocity to match UT3 for now
	ProjectileMovement->Velocity = CurrentBounceDamping * (ImpactVelocity - 2.0f * ImpactResult.Normal * (ImpactVelocity | ImpactResult.Normal));

	// add random bounce factor
	ProjectileMovement->Velocity = ProjectileMovement->Velocity.Size() * FMath::VRandCone(ProjectileMovement->Velocity, RandomBounceCone);

	// Set gravity on bounce
	ProjectileMovement->ProjectileGravityScale = 1.f;
	
	// No longer impart momentum after bounce
	Momentum = 0.f;

	// Limit number of bounces
	BouncesRemaining--;
	if (BouncesRemaining <= 0)
	{
		ProjectileMovement->bShouldBounce = false;
	}
}

FRadialDamageParams AUTProj_FlakShard::GetDamageParams_Implementation(AActor* OtherActor, const FVector& HitLocation, float& OutMomentum) const
{
	FRadialDamageParams Result = Super::GetDamageParams_Implementation(OtherActor, HitLocation, OutMomentum);
	if (OtherActor == Instigator)
	{
		// attenuate self damage
		Result.BaseDamage = FMath::Max<float>(1.f, Result.BaseDamage - SelfDamageAttenuation * FMath::Max<float>(0.0f, GetWorld()->GetTimeSeconds() - CreationTime - SelfDamageAttenuationDelay));
	}
	else
	{
		Result.BaseDamage = FMath::Max<float>(Result.MinimumDamage, Result.BaseDamage - DamageAttenuation * FMath::Max<float>(0.0f, GetWorld()->GetTimeSeconds() - CreationTime - DamageAttenuationDelay));
		if (ProjectileMovement && (ProjectileMovement->ProjectileGravityScale != 0.f))
		{
			Result.BaseDamage *= BounceDamagePct;
			OutMomentum = 0.f;
		}
	}
	return Result;
}

void AUTProj_FlakShard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PawnOverlapSphere != NULL)
	{
		float CurrentRadius = PawnOverlapSphere->GetUnscaledSphereRadius();
		bGrowOverlap = bGrowOverlap && (CurrentRadius < MaxOverlapRadius);
		if (bGrowOverlap)
		{
			PawnOverlapSphere->SetSphereRadius(FMath::Min(MaxOverlapRadius, CurrentRadius + RadiusGrowthRate*DeltaTime), false);
		}
		else if (CurrentRadius > FinalOverlapRadius)
		{
			float NewRadius = FMath::Max(FinalOverlapRadius, CurrentRadius - RadiusShrinkRate*DeltaTime);
			PawnOverlapSphere->SetSphereRadius(NewRadius, false);
		}
	}

	if (InitialLifeSpan - GetLifeSpan() > FullGravityDelay)
	{
		ProjectileMovement->ProjectileGravityScale = 1.f;
		Momentum = 0.f;
	}

	if (HeatFadeTime > 0.0f)
	{
		float FadePct = FMath::Min<float>(1.0f, (GetWorld()->TimeSeconds - CreationTime) / HeatFadeTime);
		if (Trail != NULL)
		{
			FLinearColor TrailColor = FMath::Lerp<FLinearColor>(HotTrailColor, ColdTrailColor, FadePct);
			Trail->SetColorParameter(NAME_Color, TrailColor);
			static FName NAME_StartAlpha(TEXT("StartAlpha"));
			Trail->SetFloatParameter(NAME_StartAlpha, TrailColor.A);
		}
		if (MeshMI != NULL)
		{
			static FName NAME_FadePct(TEXT("FadePct"));
			MeshMI->SetScalarParameterValue(NAME_FadePct, FadePct);
		}
	}
}
