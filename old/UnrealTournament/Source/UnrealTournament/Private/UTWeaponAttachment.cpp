// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeaponAttachment.h"
#include "Particles/ParticleSystemComponent.h"
#include "UTImpactEffect.h"
#include "UTWorldSettings.h"

AUTWeaponAttachment::AUTWeaponAttachment(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent, USceneComponent>(this, TEXT("DummyRoot"), false);
	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("Mesh3P"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh->bLightAttachmentsAsGroup = true;
	Mesh->bReceivesDecals = false;
	Mesh->bUseAttachParentBound = true;
	Mesh->LightingChannels.bChannel1 = true;
	AttachSocket = FName((TEXT("WeaponPoint")));
	HolsterSocket = FName((TEXT("spine_02")));
	HolsterOffset = FVector(0.f, 16.f, 0.f);
	HolsterRotation = FRotator(0.f, 60.f, 75.f);
	PickupScaleOverride = FVector(2.0f, 2.0f, 2.0f);
	WeaponStance = 0;

	bCopyWeaponImpactEffect = true;

	MaxBulletWhipDist = 250.0f;
	BulletWhipDelay = 0.3f;
}

void AUTWeaponAttachment::BeginPlay()
{
	Super::BeginPlay();

	AUTWeapon::InstanceMuzzleFlashArray(this, MuzzleFlash);

	UTOwner = Cast<AUTCharacter>(Instigator);
	if (UTOwner != NULL)
	{
		WeaponType = UTOwner->GetWeaponClass();
		if (WeaponType != NULL && bCopyWeaponImpactEffect && ImpactEffect.Num() == 0)
		{
			ImpactEffect = WeaponType.GetDefaultObject()->ImpactEffect;
			ImpactEffectSkipDistance = WeaponType.GetDefaultObject()->ImpactEffectSkipDistance;
			MaxImpactEffectSkipTime = WeaponType.GetDefaultObject()->MaxImpactEffectSkipTime;
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("UTWeaponAttachment: Bad Instigator: %s"), *GetNameSafe(Instigator));
	}

	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(GetWorldSettings());
	if (Mesh && Settings->bUseCapsuleDirectShadowsForCharacter)
	{
		Mesh->bCastCapsuleDirectShadow = true;
	}
}

void AUTWeaponAttachment::Destroyed()
{
	DetachFromOwner();
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::Destroyed();
}

void AUTWeaponAttachment::RegisterAllComponents()
{
	// sanity check some settings
	for (int32 i = 0; i < MuzzleFlash.Num(); i++)
	{
		if (MuzzleFlash[i] != NULL)
		{
			MuzzleFlash[i]->bAutoActivate = false;
			MuzzleFlash[i]->SecondsBeforeInactive = 0.0f;
			MuzzleFlash[i]->SetOwnerNoSee(false);
			// can't force this because the muzzle flash component might also contain the fire effect (beam)
			//MuzzleFlash[i]->bUseAttachParentBound = true;
		}
	}
	Super::RegisterAllComponents();
}

void AUTWeaponAttachment::AttachToOwner_Implementation()
{
	AttachToOwnerNative();
}

void AUTWeaponAttachment::AttachToOwnerNative()
{
	Mesh->AttachToComponent(UTOwner->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
	Mesh->SetRelativeLocation(AttachOffset);
	Mesh->bRecentlyRendered = UTOwner->GetMesh()->bRecentlyRendered;
	Mesh->LastRenderTime = UTOwner->GetMesh()->LastRenderTime;
	UpdateOverlays();
	SetSkin(UTOwner->GetSkin());
}

void AUTWeaponAttachment::HolsterToOwner_Implementation()
{
	HolsterToOwnerNative();
}

void AUTWeaponAttachment::HolsterToOwnerNative()
{
	Mesh->AttachToComponent(UTOwner->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, HolsterSocket);
	Mesh->SetRelativeLocation(HolsterOffset);
	Mesh->SetRelativeRotation(HolsterRotation);
	Mesh->bRecentlyRendered = UTOwner->GetMesh()->bRecentlyRendered;
	Mesh->LastRenderTime = UTOwner->GetMesh()->LastRenderTime;
	SetSkin(UTOwner->GetSkin());
}

void AUTWeaponAttachment::DetachFromOwner_Implementation()
{
	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AUTWeaponAttachment::UpdateOverlays()
{
	if (WeaponType != NULL)
	{
		WeaponType.GetDefaultObject()->UpdateOverlaysShared(this, UTOwner, Mesh, OverlayEffectParams, OverlayMesh);
	}
}

void AUTWeaponAttachment::UpdateOutline(bool bOn, uint8 StencilValue)
{
	if (bOn)
	{
		if (CustomDepthMesh == NULL)
		{
			CustomDepthMesh = DuplicateObject<USkeletalMeshComponent>(Mesh, this);
			CustomDepthMesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			{
				// TODO: scary that these get copied, need an engine solution and/or safe way to duplicate objects during gameplay
				CustomDepthMesh->PrimaryComponentTick = CustomDepthMesh->GetClass()->GetDefaultObject<USkeletalMeshComponent>()->PrimaryComponentTick;
				CustomDepthMesh->PostPhysicsComponentTick = CustomDepthMesh->GetClass()->GetDefaultObject<USkeletalMeshComponent>()->PostPhysicsComponentTick;
			}
			CustomDepthMesh->SetMasterPoseComponent(Mesh);
			CustomDepthMesh->BoundsScale = 15000.f;
			CustomDepthMesh->InvalidateCachedBounds();
			CustomDepthMesh->UpdateBounds();
			CustomDepthMesh->bRenderInMainPass = false;
			CustomDepthMesh->bRenderCustomDepth = true;
		}
		if (StencilValue != CustomDepthMesh->CustomDepthStencilValue)
		{
			CustomDepthMesh->CustomDepthStencilValue = StencilValue;
			CustomDepthMesh->MarkRenderStateDirty();
		}
		if (!CustomDepthMesh->IsRegistered())
		{
			CustomDepthMesh->RegisterComponent();
			CustomDepthMesh->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			CustomDepthMesh->SetWorldScale3D(Mesh->GetComponentScale());
		}
	}
	else if (CustomDepthMesh != NULL)
	{
		CustomDepthMesh->UnregisterComponent();
	}
}

void AUTWeaponAttachment::SetSkin(UMaterialInterface* NewSkin)
{
	// save off existing materials if we haven't yet done so
	while (SavedMeshMaterials.Num() < Mesh->GetNumMaterials())
	{
		SavedMeshMaterials.Add(Mesh->GetMaterial(SavedMeshMaterials.Num()));
	}
	if (NewSkin != NULL)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
		{
			Mesh->SetMaterial(i, NewSkin);
		}
	}
	else
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
		{
			Mesh->SetMaterial(i, SavedMeshMaterials[i]);
		}
	}
}

bool AUTWeaponAttachment::CancelImpactEffect(const FHitResult& ImpactHit) const
{
	return (WeaponType != nullptr) ? WeaponType.GetDefaultObject()->CancelImpactEffect(ImpactHit) : GetDefault<AUTWeapon>()->CancelImpactEffect(ImpactHit);
}

void AUTWeaponAttachment::PlayFiringEffects()
{
	// stop any firing effects for other firemodes
	// this is needed because if the user swaps firemodes very quickly replication might not receive a discrete stop and start new
	StopFiringEffects(true);
	if (WeaponType && (WeaponType.GetDefaultObject()->BaseAISelectRating > 0.f))
	{
		UTOwner->LastWeaponFireTime = GetWorld()->GetTimeSeconds();
	}

	AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	bool bEffectsRelevant = (WS == NULL || WS->EffectIsRelevant(UTOwner, UTOwner->GetActorLocation(), true, Cast<APlayerController>(UTOwner->GetController()) != nullptr, 50000.0f, 2000.0f));
	if (!bEffectsRelevant && !UTOwner->FlashLocation.Position.IsZero())
	{
		bEffectsRelevant = WS->EffectIsRelevant(UTOwner, UTOwner->FlashLocation.Position, true, Cast<APlayerController>(UTOwner->GetController()) != nullptr, 50000.0f, 2000.0f);
		if (!bEffectsRelevant)
		{
			// do frustum check versus fire line; can't use simple vis to location because the fire line may be visible while both endpoints are not
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				if (It->PlayerController != NULL)
				{
					FSceneViewProjectionData Projection;
					if (It->GetProjectionData(It->ViewportClient->Viewport, eSSP_FULL, Projection))
					{
						FConvexVolume Frustum;
						GetViewFrustumBounds(Frustum, Projection.ComputeViewProjectionMatrix(), false);
						FPoly TestPoly;
						TestPoly.Init();
						TestPoly.InsertVertex(0, UTOwner->GetActorLocation());
						TestPoly.InsertVertex(1, UTOwner->FlashLocation.Position);
						TestPoly.InsertVertex(2, (UTOwner->GetActorLocation() + UTOwner->FlashLocation.Position) * 0.5f + FVector(0.0f, 0.0f, 1.0f));
						if (Frustum.ClipPolygon(TestPoly))
						{
							bEffectsRelevant = true;
							break;
						}
					}
				}
			}
		}
	}

	if (!bEffectsRelevant)
	{
		return;
	}

	// update owner mesh and self so effects are spawned in the correct place
	if (!Mesh->ShouldUpdateTransform(false))
	{
		if (UTOwner->GetMesh() != NULL)
		{
			UTOwner->GetMesh()->RefreshBoneTransforms();
			UTOwner->GetMesh()->UpdateComponentToWorld();
		}
		Mesh->RefreshBoneTransforms();
		Mesh->UpdateComponentToWorld();
	}

	if (OverrideFiringEffects())
	{
		return;
	}

	// muzzle flash
	if (MuzzleFlash.IsValidIndex(UTOwner->FireMode) && MuzzleFlash[UTOwner->FireMode] != NULL && MuzzleFlash[UTOwner->FireMode]->Template != NULL)
	{
		// if we detect a looping particle system, then don't reactivate it
		if (!MuzzleFlash[UTOwner->FireMode]->bIsActive || MuzzleFlash[UTOwner->FireMode]->bSuppressSpawning || !IsLoopingParticleSystem(MuzzleFlash[UTOwner->FireMode]->Template))
		{
			MuzzleFlash[UTOwner->FireMode]->ActivateSystem();
		}
	}

	// fire effects
	static FName NAME_HitLocation(TEXT("HitLocation"));
	static FName NAME_LocalHitLocation(TEXT("LocalHitLocation"));
	const FVector SpawnLocation = (MuzzleFlash.IsValidIndex(UTOwner->FireMode) && MuzzleFlash[UTOwner->FireMode] != NULL) ? MuzzleFlash[UTOwner->FireMode]->GetComponentLocation() : UTOwner->GetActorLocation() + UTOwner->GetActorRotation().RotateVector(FVector(UTOwner->GetSimpleCollisionCylinderExtent().X, 0.0f, 0.0f));
	if (FireEffect.IsValidIndex(UTOwner->FireMode) && FireEffect[UTOwner->FireMode] != NULL)
	{
		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireEffect[UTOwner->FireMode], SpawnLocation, (UTOwner->FlashLocation.Position - SpawnLocation).Rotation(), true);
		PSC->SetVectorParameter(NAME_HitLocation, UTOwner->FlashLocation.Position);
		PSC->SetVectorParameter(NAME_LocalHitLocation, PSC->ComponentToWorld.InverseTransformPosition(UTOwner->FlashLocation.Position));
		ModifyFireEffect(PSC);
	}
	// perhaps the muzzle flash also contains hit effect (constant beam, etc) so set the parameter on it instead
	else if (MuzzleFlash.IsValidIndex(UTOwner->FireMode) && MuzzleFlash[UTOwner->FireMode] != NULL)
	{
		MuzzleFlash[UTOwner->FireMode]->SetVectorParameter(NAME_HitLocation, UTOwner->FlashLocation.Position);
		MuzzleFlash[UTOwner->FireMode]->SetVectorParameter(NAME_LocalHitLocation, MuzzleFlash[UTOwner->FireMode]->ComponentToWorld.InverseTransformPosition(UTOwner->FlashLocation.Position));
	}

	if (!UTOwner->FlashLocation.Position.IsZero() && ((UTOwner->FlashLocation.Position - LastImpactEffectLocation).Size() >= ImpactEffectSkipDistance || GetWorld()->TimeSeconds - LastImpactEffectTime >= MaxImpactEffectSkipTime))
	{
		if (ImpactEffect.IsValidIndex(UTOwner->FireMode) && ImpactEffect[UTOwner->FireMode] != NULL)
		{
			FHitResult ImpactHit = AUTWeapon::GetImpactEffectHit(UTOwner, SpawnLocation, UTOwner->FlashLocation.Position);
			if (!CancelImpactEffect(ImpactHit))
			{
				ImpactEffect[UTOwner->FireMode].GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(ImpactHit.Normal.Rotation(), ImpactHit.Location), ImpactHit.Component.Get(), NULL, UTOwner->Controller);
			}
		}
		LastImpactEffectLocation = UTOwner->FlashLocation.Position;
		LastImpactEffectTime = GetWorld()->TimeSeconds;
	}

	PlayBulletWhip();
}

void AUTWeaponAttachment::PlayBulletWhip()
{
	if (BulletWhip != NULL && !UTOwner->FlashLocation.Position.IsZero())
	{
		// delay bullet whip to better separate sound from shot
		if (GetWorld()->GetTimerManager().IsTimerActive(BulletWhipHandle))
		{
			DelayedBulletWhip();
		}
		BulletWhipStart = UTOwner->GetActorLocation();
		BulletWhipEnd = UTOwner->FlashLocation.Position;
		BulletWhipHearers.Empty();

		const FVector Dir = (BulletWhipEnd - BulletWhipStart).GetSafeNormal();
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
			if (PC != NULL && PC->GetViewTarget() != UTOwner && (!GS || !GS->OnSameTeam(UTOwner, PC)))
			{
				FVector ViewLoc;
				FRotator ViewRot;
				PC->GetPlayerViewPoint(ViewLoc, ViewRot);
				const FVector ClosestPt = FMath::ClosestPointOnSegment(ViewLoc, BulletWhipStart, BulletWhipEnd);
				if (ClosestPt != BulletWhipStart && ClosestPt != BulletWhipEnd && (ClosestPt - ViewLoc).Size() <= MaxBulletWhipDist)
				{
					// trace to make sure missed shot isn't on the other side of a wall
					FCollisionQueryParams Params(FName(TEXT("BulletWhip")), true, UTOwner);
					Params.AddIgnoredActor(PC->GetPawn());
					if (!GetWorld()->LineTraceTestByChannel(ClosestPt, ViewLoc, COLLISION_TRACE_WEAPON, Params))
					{
						BulletWhipHearers.Add(PC);
					}
				}
			}
		}

		if (BulletWhipHearers.Num() > 0)
		{
			if (BulletWhipDelay <= 0.f)
			{
				DelayedBulletWhip();
			}
			else
			{
				GetWorld()->GetTimerManager().SetTimer(BulletWhipHandle, this, &AUTWeaponAttachment::DelayedBulletWhip, BulletWhipDelay, false);
			}
		}
	}
}

void AUTWeaponAttachment::DelayedBulletWhip()
{
	if (BulletWhip != NULL)
	{
		for (int32 i=0; i<BulletWhipHearers.Num(); i++)
		{
			if (BulletWhipHearers[i] != nullptr)
			{
				FVector ViewLoc;
				FRotator ViewRot;
				BulletWhipHearers[i]->GetPlayerViewPoint(ViewLoc, ViewRot);
				const FVector ClosestPt = FMath::ClosestPointOnSegment(ViewLoc, BulletWhipStart, BulletWhipEnd);
				BulletWhipHearers[i]->ClientHearSound(BulletWhip, this, ClosestPt, false, false, SAT_None);
			}
		}
	}
}

void AUTWeaponAttachment::FiringExtraUpdated()
{
}

void AUTWeaponAttachment::StopFiringEffects_Implementation(bool bIgnoreCurrentMode)
{
	// we need to default to stopping all modes' firing effects as we can't rely on the replicated value to be correct at this point
	for (uint8 i = 0; i < MuzzleFlash.Num(); i++)
	{
		if (MuzzleFlash[i] != NULL && (!bIgnoreCurrentMode || !MuzzleFlash.IsValidIndex(UTOwner->FireMode) || MuzzleFlash[UTOwner->FireMode] == NULL || (i != UTOwner->FireMode && MuzzleFlash[i] != MuzzleFlash[UTOwner->FireMode])))
		{
			MuzzleFlash[i]->DeactivateSystem();
		}
	}
}

void AUTWeaponAttachment::ModifyFireEffect_Implementation(class UParticleSystemComponent* Effect)
{}
