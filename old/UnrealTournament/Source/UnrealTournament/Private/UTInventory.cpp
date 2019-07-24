// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UnrealNetwork.h"
#include "UTDroppedPickup.h"

AUTInventory::AUTInventory(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetReplicates(true);
	bOnlyRelevantToOwner = true;

	RespawnTime = 30.0f;

	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent, USceneComponent>(this, TEXT("DummyRoot"), false);
	PickupMesh = ObjectInitializer.CreateOptionalDefaultSubobject<UStaticMeshComponent>(this, TEXT("PickupMesh0"), false);
	if (PickupMesh != NULL)
	{
		PickupMesh->SetupAttachment(RootComponent);
		PickupMesh->bAutoRegister = false;
	}

	DroppedPickupClass = AUTDroppedPickup::StaticClass();
	BasePickupDesireability = 0.5f;
	DisplayName = NSLOCTEXT("PickupMessage", "InventoryPickedUp", "Item");
	InitialFlashTime = 0.3f;
	InitialFlashScale = 5.f;
	InitialFlashColor = FLinearColor::White;
	bShowPowerupTimer = true;
	bFixedRespawnInterval = false;

	MenuDescription = NSLOCTEXT("UTWeapon","DefaultDescription","This space let intentionally blank");
	IconColor = FLinearColor::White;
	
	bBoostPowerupSuppliedItem = false;
	RemainingBoostsGivenOverride = 1;

	HUDText = FText::GetEmpty();
	PreSpawnTime = 3.f;
	bShouldPrecacheTutorialAnnouncements = true;
}

void AUTInventory::PostInitProperties()
{
	Super::PostInitProperties();

	// attempt to set defaults for event early outs based on whether the class has implemented them
	// note that this only works for blueprints, C++ classes need to manually set
	if (Cast<UBlueprintGeneratedClass>(GetClass()) != NULL)
	{
		if (!bCallDamageEvents)
		{
			static FName NAME_ModifyDamageTaken(TEXT("ModifyDamageTaken"));
			static FName NAME_PreventHeadShot(TEXT("PreventHeadShot"));
			UFunction* Func = FindFunction(NAME_ModifyDamageTaken);
			bCallDamageEvents = (Func != NULL && Func->Script.Num() > 0);
			if (!bCallDamageEvents)
			{
				Func = FindFunction(NAME_PreventHeadShot);
				bCallDamageEvents = (Func != NULL && Func->Script.Num() > 0);
			}
		}
		if (!bCallOwnerEvent)
		{
			static FName NAME_OwnerEvent(TEXT("OwnerEvent"));
			UFunction* Func = FindFunction(NAME_OwnerEvent);
			bCallOwnerEvent = (Func != NULL && Func->Script.Num() > 0);
		}
	}
}

void AUTInventory::PreInitializeComponents()
{
	// get rid of components that are only supposed to be part of the pickup mesh
	// TODO: would be better to not create in the first place, no reasonable engine hook to filter
	TArray<UActorComponent*> SerializedComponents = BlueprintCreatedComponents;
	SerializedComponents += GetInstanceComponents();

	for (int32 i = 0; i < SerializedComponents.Num(); i++)
	{
		USceneComponent* SceneComp = Cast<USceneComponent>(SerializedComponents[i]);
		if (SceneComp != NULL && SceneComp->GetAttachParent() != NULL && SceneComp->GetAttachParent() == PickupMesh && !SceneComp->GetAttachParent()->IsRegistered())
		{
			TArray<USceneComponent*> ChildComps;
			SceneComp->GetChildrenComponents(true, ChildComps);
			for (USceneComponent* Child : ChildComps)
			{
				Child->DestroyComponent();
			}
			SceneComp->DestroyComponent();
		}
	}
	
	Super::PreInitializeComponents();
}

UMeshComponent* AUTInventory::GetPickupMeshTemplate_Implementation(FVector& OverrideScale) const
{
	return PickupMesh;
}

void AUTInventory::AddOverlayMaterials_Implementation(AUTGameState* GS) const
{
}

void AUTInventory::InitAsTriggeredBoost(class AUTCharacter* TriggeringCharacter)
{
	bAlwaysDropOnDeath = false;
	DroppedPickupClass = nullptr;
	bBoostPowerupSuppliedItem = true;
	UUTGameplayStatics::UTPlaySound(GetWorld(), PickupSound, TriggeringCharacter, SRT_All, false, FVector::ZeroVector, NULL, TriggeringCharacter, true);
}

void AUTInventory::Destroyed()
{
	if (UTOwner != NULL)
	{
		UTOwner->RemoveInventory(this);
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::Destroyed();
}

bool AUTInventory::AllowPickupBy(AUTCharacter* Other) const 
{ 
	return true; 
}

void AUTInventory::GivenTo(AUTCharacter* NewOwner, bool bAutoActivate)
{
	Instigator = NewOwner;
	SetOwner(NewOwner);
	UTOwner = NewOwner;
	PrimaryActorTick.AddPrerequisite(UTOwner, UTOwner->PrimaryActorTick);
	eventGivenTo(NewOwner, bAutoActivate);
	ClientGivenTo(Instigator, bAutoActivate);
}

void AUTInventory::Removed()
{
	eventRemoved();

	if (UTOwner != NULL)
	{
		PrimaryActorTick.RemovePrerequisite(UTOwner, UTOwner->PrimaryActorTick);
	}

	ClientRemoved(); // must be first, since it won't replicate after Owner is lost

	Instigator = NULL;
	SetOwner(NULL);
	UTOwner = NULL;
	NextInventory = NULL;
}

void AUTInventory::CheckPendingClientGivenTo()
{
	if (bPendingClientGivenTo && Instigator != NULL)
	{
		bPendingClientGivenTo = false;
		ClientGivenTo_Implementation(nullptr, bPendingAutoActivate);
	}
}
void AUTInventory::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	// this is for inventory replicated to non-owner
	if (!bPendingClientGivenTo && (GetUTOwner() == NULL || Instigator != NULL))
	{
		UTOwner = Cast<AUTCharacter>(Instigator);
	}
	CheckPendingClientGivenTo();
}

void AUTInventory::ClientGivenTo_Implementation(APawn* NewInstigator, bool bAutoActivate)
{
	if (NewInstigator != nullptr)
	{
		Instigator = NewInstigator;
	}
	FlashTimer = InitialFlashTime;

	if (Instigator == NULL || !Cast<AUTCharacter>(Instigator)->IsInInventory(this))
	{
		bPendingClientGivenTo = true;
		bPendingAutoActivate = bAutoActivate;
		GetWorld()->GetTimerManager().SetTimer(CheckPendingClientGivenToHandle, this, &AUTInventory::CheckPendingClientGivenTo, 0.1f, false);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckPendingClientGivenToHandle);
		bPendingClientGivenTo = false;
		ClientGivenTo_Internal(bAutoActivate);
		eventClientGivenTo(bAutoActivate);
	}
}

void AUTInventory::ClientGivenTo_Internal(bool bAutoActivate)
{
	checkSlow(Instigator != NULL);
	SetOwner(Instigator);
	UTOwner = Cast<AUTCharacter>(Instigator);
	checkSlow(UTOwner != NULL);
	PrimaryActorTick.AddPrerequisite(UTOwner, UTOwner->PrimaryActorTick);
}

void AUTInventory::ClientRemoved_Implementation()
{
	if (UTOwner != NULL)
	{
		PrimaryActorTick.RemovePrerequisite(UTOwner, UTOwner->PrimaryActorTick);
	}
	eventClientRemoved();
	SetOwner(NULL);
	UTOwner = NULL;
	Instigator = NULL;

	// This will come through replication
	if (Role == ROLE_Authority)
	{
		NextInventory = NULL;
	}
}

void AUTInventory::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTInventory, NextInventory, COND_None);
	DOREPLIFETIME_CONDITION(AUTInventory, bBoostPowerupSuppliedItem, COND_OwnerOnly);
}

void AUTInventory::DropFrom(const FVector& StartLocation, const FVector& TossVelocity)
{
	if (Role == ROLE_Authority)
	{
		AUTCharacter* FormerInstigator = Cast<AUTCharacter>(Instigator);

		if (UTOwner != NULL)
		{
			UTOwner->RemoveInventory(this);
		}
		Instigator = NULL;
		SetOwner(NULL);
		if (DroppedPickupClass != NULL && !IsPendingKillPending())
		{
			// pull back spawn location if it is embedded in world geometry
			FVector AdjustedStartLoc = StartLocation;
			UCapsuleComponent* TestCapsule = DroppedPickupClass.GetDefaultObject()->Collision;
			if (TestCapsule != NULL)
			{
				FCollisionQueryParams QueryParams(FName(TEXT("DropPlacement")), false);
				FHitResult Hit;
				if ( GetWorld()->SweepSingleByChannel(Hit, StartLocation - FVector(TossVelocity.X, TossVelocity.Y, 0.0f) * 0.25f, StartLocation, FQuat::Identity, TestCapsule->GetCollisionObjectType(), TestCapsule->GetCollisionShape(), QueryParams, TestCapsule->GetCollisionResponseToChannels()) &&
					 !Hit.bStartPenetrating )
				{
					AdjustedStartLoc = Hit.Location;
				}
			}

			FActorSpawnParameters Params;
			Params.Instigator = FormerInstigator;
			AUTDroppedPickup* Pickup = GetWorld()->SpawnActor<AUTDroppedPickup>(DroppedPickupClass, AdjustedStartLoc, TossVelocity.Rotation(), Params);
			if (Pickup != NULL)
			{
				Pickup->Movement->Velocity = TossVelocity;
				InitializeDroppedPickup(Pickup);
				AUTGameMode* GM = GetWorld()->GetAuthGameMode<AUTGameMode>();
				if (FormerInstigator && FormerInstigator->OldPlayerState && GM && GM->bAllowPickupAnnouncements && (PickupAnnouncementName != NAME_None))
				{
					FormerInstigator->OldPlayerState->AnnounceStatus(PickupAnnouncementName, 2, false);
				}
			}
			else
			{
				Destroy();
			}
		}
		else
		{
			Destroy();
		}
	}
}

void AUTInventory::InitializeDroppedPickup(AUTDroppedPickup* Pickup)
{
	Pickup->SetInventory(this);
}

void AUTInventory::OnViewTargetChange_Implementation(AUTPlayerController* NewViewTarget)
{

}

bool AUTInventory::StackLockerPickup(AUTInventory* ContainedInv)
{
	return StackPickup(ContainedInv);
}

bool AUTInventory::StackPickup_Implementation(AUTInventory* ContainedInv)
{
	return false;
}

bool AUTInventory::HandleGivenTo_Implementation(AUTCharacter* NewOwner)
{
	return false;
}

bool AUTInventory::ModifyDamageTaken_Implementation(int32& Damage, FVector& Momentum, AUTInventory*& HitArmor, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
	return false;
}

bool AUTInventory::HandleArmorEffects(AUTCharacter* HitPawn) const
{
	return PlayArmorEffects(HitPawn);
}

bool AUTInventory::PlayArmorEffects_Implementation(AUTCharacter* HitPawn) const
{
	return false;
}

bool AUTInventory::PreventHeadShot_Implementation(FVector HitLocation, FVector ShotDirection, float WeaponHeadScaling, bool bConsumeArmor)
{
	return false;
}

bool AUTInventory::ShouldDisplayHitEffect_Implementation(int32 AttemptedDamage, int32 DamageAmount, int32 FinalHealth, int32 FinalArmor)
{
	bool bShowArmorDamage = (AttemptedDamage > 0) && (FinalHealth > 0) && (FinalHealth + AttemptedDamage > 100) && (int32((FinalHealth + AttemptedDamage) / AttemptedDamage) != int32(100 / AttemptedDamage));
	return bShowArmorDamage || (FinalArmor > 50);
}

void AUTInventory::OwnerEvent_Implementation(FName EventName)
{
}

bool AUTInventory::OverrideGiveTo_Implementation(AUTPickup* Pickup)
{
	return false;
}

bool AUTInventory::HUDShouldRender_Implementation(UUTHUDWidget* TargetWidget)
{
	return false;
}

void AUTInventory::DrawInventoryHUD_Implementation(UUTHUDWidget* Widget, FVector2D Pos, FVector2D Size)
{
}

float AUTInventory::BotDesireability_Implementation(APawn* Asker, AController* RequestOwner, AActor* Pickup, float PathDistance) const
{
	return BasePickupDesireability;
}
float AUTInventory::DetourWeight_Implementation(APawn* Asker, AActor* Pickup, float PathDistance) const
{
	return 0.0f;
}
int32 AUTInventory::GetEffectiveHealthModifier_Implementation(bool bOnlyVisible) const
{
	return 0;
}

void AUTInventory::UpdateHUDText()
{
}

FText AUTInventory::GetHUDText() const
{
	return HUDText;
}

void AUTInventory::PrecacheTutorialAnnouncements(UUTAnnouncer* Announcer) const
{
	if (bShouldPrecacheTutorialAnnouncements)
	{
		for (int32 i = 0; i < TutorialAnnouncements.Num(); i++)
		{
			Announcer->PrecacheAnnouncement(TutorialAnnouncements[i]);
		}
	}
}

FName AUTInventory::GetTutorialAnnouncement(int32 Switch) const
{
	return (Switch < TutorialAnnouncements.Num()) ? TutorialAnnouncements[Switch] : NAME_None;
}

float AUTInventory::GetBoostPowerRating_Implementation(AUTBot* B) const
{
	return 0.0f;
}

void AUTInventory::DrawBoostHUD_Implementation(AUTHUD* Hud, UCanvas* C, APawn* P) const
{
}