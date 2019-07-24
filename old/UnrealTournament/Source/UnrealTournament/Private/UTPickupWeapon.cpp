// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTPickup.h"
#include "UTPickupInventory.h"
#include "UTPickupWeapon.h"
#include "UTWorldSettings.h"

void AUTPickupWeapon::BeginPlay()
{
	if (!IsPendingKillPending())
	{
		AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
		if (WS != NULL)
		{
			WS->PerPlayerPickups.Add(this);
		}
	}

	AUTPickup::BeginPlay(); // skip AUTPickupInventory so we can propagate WeaponType as InventoryType

	if (TimerEffect != NULL)
	{
		TimerEffect->SetVisibility(true); // note: HiddenInGame used to hide when weapon is available, weapon stay, etc
	}
	if (Role == ROLE_Authority)
	{
		SetInventoryType(TSubclassOf<AUTInventory>(WeaponType));
	}
	else
	{
		InventoryTypeUpdated();
	}
}

void AUTPickupWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	if (WS != NULL)
	{
		WS->PerPlayerPickups.Remove(this);
	}
}

void AUTPickupWeapon::SetInventoryType(TSubclassOf<AUTInventory> NewType)
{
	WeaponType = *NewType;
	if (WeaponType == NULL)
	{
		NewType = NULL;
	}
	Super::SetInventoryType(NewType);
}
void AUTPickupWeapon::InventoryTypeUpdated_Implementation()
{
	// make sure client matches the variables, since only InventoryType is replicated
	if (Role < ROLE_Authority)
	{
		if (InventoryType != NULL)
		{
			bDelayedSpawn = InventoryType.GetDefaultObject()->bDelayedSpawn;
		}
		WeaponType = *InventoryType;
	}
	Super::InventoryTypeUpdated_Implementation();

	if (GhostDepthMesh != NULL)
	{
		UnregisterComponentTree(GhostMesh);
		GhostMesh = NULL;
	}
	if (GhostMesh != NULL && Mesh != NULL)
	{
		GhostDepthMesh = DuplicateObject<UMeshComponent>(Mesh, this);
		GhostDepthMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		//PLK - HOPEFULLY THIS ISN't BROKEN DUE TO THE PRIVATIZATION OF ATTACHCHILDREN
		//GhostDepthMesh->AttachChildren.Empty();
		GhostDepthMesh->SetRenderCustomDepth(true);
		GhostDepthMesh->SetRenderInMainPass(false);
		GhostDepthMesh->CastShadow = false;
		GhostDepthMesh->RegisterComponent();
		GhostDepthMesh->bShouldUpdatePhysicsVolume = false;
		GhostDepthMesh->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale);
		if (GhostDepthMesh->bAbsoluteScale) // SnapToTarget doesn't handle absolute...
		{
			GhostDepthMesh->SetWorldScale3D(Mesh->GetComponentScale());
		}
	}
}

bool AUTPickupWeapon::IsTaken(APawn* TestPawn)
{
	for (int32 i = Customers.Num() - 1; i >= 0; i--)
	{
		if (Customers[i].P == NULL || Customers[i].P->bTearOff || Customers[i].P->IsPendingKillPending())
		{
			Customers.RemoveAt(i);
		}
		else if (Customers[i].P == TestPawn)
		{
			return (GetWorld()->TimeSeconds < Customers[i].NextPickupTime);
		}
	}
	return false;
}

float AUTPickupWeapon::GetRespawnTimeOffset(APawn* Asker) const
{
	if (!State.bActive)
	{
		return Super::GetRespawnTimeOffset(Asker);
	}
	else
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay || (GS != NULL && !GS->bWeaponStay))
		{
			return Super::GetRespawnTimeOffset(Asker);
		}
		else
		{
			for (int32 i = Customers.Num() - 1; i >= 0; i--)
			{
				if (Customers[i].P == Asker)
				{
					return (Customers[i].NextPickupTime - GetWorld()->TimeSeconds);
				}
			}
			return -100000.0f;
		}
	}
}

void AUTPickupWeapon::ProcessTouch_Implementation(APawn* TouchedBy)
{
	if (State.bActive)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay || (GS != NULL && !GS->bWeaponStay))
		{
			Super::ProcessTouch_Implementation(TouchedBy);
		}
		// note that we don't currently call AllowPickupBy() and associated GameMode/Mutator overrides in the weapon stay case
		// in part due to client synchronization issues
		else if (!IsTaken(TouchedBy) && Cast<AUTCharacter>(TouchedBy) != NULL && !((AUTCharacter*)TouchedBy)->IsRagdoll() && ((AUTCharacter*)TouchedBy)->bCanPickupItems &&
				(Role == ROLE_Authority || !TouchedBy->IsLocallyControlled() || Cast<AUTPlayerController>(TouchedBy->Controller) == nullptr))
		{
			if (Role == ROLE_Authority)
			{
				GiveTo(TouchedBy);

				AUTGameMode* UTGameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
				if (UTGameMode != nullptr && UTGameMode->NumBots > 0)
				{
					float Radius = 0.0f;
					if (TakenSound != NULL)
					{
						Radius = TakenSound->GetMaxAudibleDistance();
						const FAttenuationSettings* Settings = TakenSound->GetAttenuationSettingsToApply();
						if (Settings != NULL)
						{
							Radius = FMath::Max<float>(Radius, Settings->GetMaxDimension());
						}
					}
					for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
					{
						AUTBot* B = Cast<AUTBot>(It->Get());
						if (B != NULL)
						{
							B->NotifyPickup(TouchedBy, this, Radius, false);
						}
					}
				}
			}
			if (!GetWorldTimerManager().IsTimerActive(CheckTouchingHandle))
			{
				GetWorldTimerManager().SetTimer(CheckTouchingHandle, this, &AUTPickupWeapon::CheckTouching, RespawnTime, false);
			}
			LocalPickupHandling(TouchedBy);
			if (Role == ROLE_Authority && !TouchedBy->IsLocallyControlled())
			{
				// send RPC to remote player to guarantee matched pickup state
				AUTPlayerController* PC = Cast<AUTPlayerController>(TouchedBy->Controller);
				if (PC != nullptr)
				{
					PC->ClientGotWeaponStayPickup(this, TouchedBy);
				}
			}
		}

		// As State.bActive was true, we are going to be invisible one way or another after the player has touched it.
		// Calls to Super::ProcessTouch are not always reliable in cases of WeaponStay as State.bActive is always true. Make sure we show ghost mesh.
		if (GhostMesh != NULL)
		{
			GhostMesh->SetVisibility(true, true);
			if (GhostDepthMesh != NULL)
			{
				GhostDepthMesh->SetVisibility(true, true);
			}
		}
	}
}

float AUTPickupWeapon::GetNextPickupTime()
{
	return NextPickupTime;
}

void AUTPickupWeapon::LocalPickupHandling(APawn* TouchedBy)
{
	new(Customers) FWeaponPickupCustomer(TouchedBy, GetWorld()->TimeSeconds + NextPickupTime);
	PlayTakenEffects(false);
	UUTGameplayStatics::UTPlaySound(GetWorld(), TakenSound, TouchedBy, SRT_IfSourceNotReplicated, false, FVector::ZeroVector, NULL, NULL, false);
	if (TouchedBy->IsLocallyControlled())
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(TouchedBy->Controller);
		if (PC != NULL)
		{
			// TODO: does not properly support splitscreen
			if (BaseEffect != NULL && BaseTemplateTaken != NULL)
			{
				BaseEffect->SetTemplate(BaseTemplateTaken);
			}
			if (TimerEffect != NULL)
			{
				TimerEffect->SetFloatParameter(NAME_Progress, 0.0f);
				TimerEffect->SetFloatParameter(NAME_RespawnTime, NextPickupTime);
				TimerEffect->SetHiddenInGame(false);
			}
			PC->AddPerPlayerPickup(this);
		}
	}
}

void AUTPickupWeapon::CheckTouching()
{
	TArray<AActor*> Touching;
	GetOverlappingActors(Touching, APawn::StaticClass());
	for (AActor* TouchingActor : Touching)
	{
		APawn* P = Cast<APawn>(TouchingActor);
		if (P != NULL)
		{
			ProcessTouch(P);
		}
	}
	// see if we should reset the timer
	float NextCheckInterval = 0.0f;
	for (const FWeaponPickupCustomer& PrevCustomer : Customers)
	{
		NextCheckInterval = FMath::Max<float>(NextCheckInterval, PrevCustomer.NextPickupTime - GetWorld()->TimeSeconds);
	}
	if (NextCheckInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(CheckTouchingHandle, this, &AUTPickupWeapon::CheckTouching, NextCheckInterval, false);
	}
}

void AUTPickupWeapon::PlayTakenEffects(bool bReplicate)
{
	if (bReplicate)
	{
		Super::PlayTakenEffects(bReplicate);
	}
	else if (GetNetMode() != NM_DedicatedServer)
	{
		AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
		if (WS == NULL || WS->EffectIsRelevant(this, GetActorLocation(), true, false, 10000.0f, 1000.0f, false))
		{
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(TakenParticles, RootComponent, NAME_None, TakenEffectTransform.GetLocation(), TakenEffectTransform.GetRotation().Rotator());
			if (PSC != NULL)
			{
				PSC->SetRelativeScale3D(TakenEffectTransform.GetScale3D());
			}
		}
	}
}

void AUTPickupWeapon::SetPickupHidden(bool bNowHidden)
{
	Super::SetPickupHidden(bNowHidden);
	if (GhostMesh != NULL && GhostDepthMesh != NULL)
	{
		GhostDepthMesh->SetVisibility(GhostMesh->bVisible, true);
	}
}

#if WITH_EDITOR
void AUTPickupWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (TimerEffect != NULL && GetWorld() != NULL && GetWorld()->WorldType == EWorldType::Editor)
	{
		TimerEffect->SetVisibility(WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay);
	}
}
void AUTPickupWeapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// only show timer sprite for superweapons
	if (TimerEffect != NULL)
	{
		TimerEffect->SetVisibility(WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay);
	}
}
#endif