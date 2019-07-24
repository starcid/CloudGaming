// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponState.h"
#include "UTWeaponStateFiring.h"
#include "UTWeaponStateActive.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeaponStateUnequipping.h"
#include "UTWeaponStateInactive.h"
#include "UTWeaponStateFiringCharged.h"
#include "UTWeaponStateZooming.h"
#include "UTWeaponAttachment.h"
#include "UnrealNetwork.h"
#include "UTHUDWidget.h"
#include "EditorSupportDelegates.h"
#include "Particles/ParticleSystemComponent.h"
#include "UTImpactEffect.h"
#include "UTCharacterMovement.h"
#include "UTWorldSettings.h"
#include "UTPlayerCameraManager.h"
#include "UTHUD.h"
#include "UTGameViewportClient.h"
#include "UTCrosshair.h"
#include "UTDroppedPickup.h"
#include "UTAnnouncer.h"
#include "UTProj_ShockBall.h"
#include "UTTeamDeco.h"
#include "UTProj_WeaponScreen.h"
#include "Classes/Kismet/KismetMaterialLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTWeapon, Log, All);

AUTWeapon::AUTWeapon(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("PickupMesh0")))
{
	AmmoCost.Add(1);
	AmmoCost.Add(1);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;

	bWeaponStay = true;
	bCanThrowWeapon = true;

	Ammo = 20;
	MaxAmmo = 50;

	Group = -1;
	BringUpTime = 0.37f;
	PutDownTime = 0.3f;
	RefirePutDownTimePercent = 1.0f;
	WeaponBobScaling = 1.f;
	FiringViewKickback = -20.f;
	FiringViewKickbackY = 6.f;
	HUDViewKickback = FVector2D(0.03f, 0.1f);
	bNetDelayedShot = false;
	RespawnTime = 20.f;

	bFPFireFromCenter = true;
	bFPIgnoreInstantHitFireOffset = true;
	FireOffset = FVector(75.0f, 0.0f, 0.0f);
	FriendlyMomentumScaling = 1.f;
	FireEffectInterval = 1;
	FireEffectCount = 0;
	FireZOffset = 0.f;
	FireZOffsetTime = 0.f;

	InactiveState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateInactive>(this, TEXT("StateInactive"));
	ActiveState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateActive>(this, TEXT("StateActive"));
	EquippingState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateEquipping>(this, TEXT("StateEquipping"));
	UnequippingStateDefault = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateUnequipping>(this, TEXT("StateUnequipping"));
	UnequippingState = UnequippingStateDefault;

	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("Mesh1P"));
	Mesh->SetOnlyOwnerSee(true);
	Mesh->SetupAttachment(RootComponent);
	Mesh->bSelfShadowOnly = true;
	Mesh->bReceivesDecals = false;
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh->LightingChannels.bChannel1 = true;
	FirstPMeshOffset = FVector(0.f);
	FirstPMeshRotation = FRotator(0.f, 0.f, 0.f);

	for (int32 i = 0; i < 2; i++)
	{
		UUTWeaponStateFiring* NewState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateFiring, UUTWeaponStateFiring>(this, FName(*FString::Printf(TEXT("FiringState%i"), i)), false);
		if (NewState != NULL)
		{
			FiringState.Add(NewState);
			FireInterval.Add(1.0f);
		}
	}

	RotChgSpeed = 3.f;
	ReturnChgSpeed = 3.f;
	MaxYawLag = 4.4f;
	MaxPitchLag = 3.3f;
	FOVOffset = FVector(1.f);
	bProceduralLagRotation = true;
	AnimLagMultiplier = -4.f;;
	AnimLagSpeedReturn = 2.f;

	WeaponRenderScale = 0.8f;

	// default icon texture
	static ConstructorHelpers::FObjectFinder<UTexture> WeaponTexture(TEXT("Texture2D'/Game/RestrictedAssets/Proto/UI/HUD/Elements/UI_HUD_BaseB.UI_HUD_BaseB'"));
	HUDIcon.Texture = WeaponTexture.Object;

	BaseAISelectRating = 0.55f;
	DisplayName = NSLOCTEXT("PickupMessage", "WeaponPickedUp", "Weapon");
	HighlightText = NSLOCTEXT("Weapon", "HighlightText", "Weapon Master");
	bShowPowerupTimer = false;

	bCheckHeadSphere = false;
	bCheckMovingHeadSphere = false;
	bIgnoreShockballs = false;
	bTeammatesBlockHitscan = false;

	WeightSpeedPctModifier = 1.0f;

	AmmoWarningAmount = 5;
	AmmoDangerAmount = 2;

	LowAmmoSoundDelay = 0.2f;
	LowAmmoThreshold = 3;
	FireSoundAmp = SAT_WeaponFire;
	FireEventIndex = 0;

	WeaponSkinCustomizationTag = NAME_None;
	VerticalSpreadScaling = 1.f;
	MaxVerticalSpread = 1.f;

	bSecondaryIdle = false;
	bWallRunFire = false;
	bIdleOffset = false;
	bIdleEmpty = false;
	bIdleAlt = false;
}

void AUTWeapon::PostInitProperties()
{
	Super::PostInitProperties();

	if (Group == -1)
	{
		Group = DefaultGroup;
	}
	WeaponBarScale = 0.0f;

	if (DisplayName.IsEmpty() || (GetClass() != AUTWeapon::StaticClass() && DisplayName.EqualTo(GetClass()->GetSuperClass()->GetDefaultObject<AUTWeapon>()->DisplayName) && GetClass()->GetSuperClass()->GetDefaultObject<AUTWeapon>()->DisplayName.EqualTo(FText::FromName(GetClass()->GetSuperClass()->GetFName()))))
	{
		DisplayName = FText::FromName(GetClass()->GetFName());
	}
}

#if WITH_EDITOR

static FName NAME_UTWeapon_DefaultGroup("DefaultGroup");

void AUTWeapon::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	
	// When editing DefaultGroup, the Group property should be updated as well if it's equal DefaultGroup 
	if (PropertyAboutToChange && PropertyAboutToChange->GetFName() == NAME_UTWeapon_DefaultGroup && Group == DefaultGroup)
	{
		Group = -1;
	}
}

void AUTWeapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == NAME_UTWeapon_DefaultGroup && Group == -1)
	{
		Group = DefaultGroup;
	}
}

#endif // WITH_EDITOR

UMeshComponent* AUTWeapon::GetPickupMeshTemplate_Implementation(FVector& OverrideScale) const
{
	if (AttachmentType != NULL)
	{
		OverrideScale = AttachmentType.GetDefaultObject()->PickupScaleOverride;
		return AttachmentType.GetDefaultObject()->Mesh;
	}
	else
	{
		return Super::GetPickupMeshTemplate_Implementation(OverrideScale);
	}
}

void AUTWeapon::InstanceMuzzleFlashArray(AActor* Weap, TArray<UParticleSystemComponent*>& MFArray)
{
	TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
	UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(Weap->GetClass(), ParentBPClassStack);
	for (int32 i = ParentBPClassStack.Num() - 1; i >= 0; i--)
	{
		const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[i];
		if (CurrentBPGClass->SimpleConstructionScript)
		{
			TArray<USCS_Node*> ConstructionNodes = CurrentBPGClass->SimpleConstructionScript->GetAllNodes();
			for (int32 j = 0; j < ConstructionNodes.Num(); j++)
			{
				for (int32 k = 0; k < MFArray.Num(); k++)
				{
					if (Cast<UParticleSystemComponent>(ConstructionNodes[j]->ComponentTemplate) == MFArray[k])
					{
						MFArray[k] = Cast<UParticleSystemComponent>((UObject*)FindObjectWithOuter(Weap, ConstructionNodes[j]->ComponentTemplate->GetClass(), ConstructionNodes[j]->GetVariableName()));
					}
				}
			}
		}
	}
}

void AUTWeapon::BeginPlay()
{
	Super::BeginPlay();

	InstanceMuzzleFlashArray(this, MuzzleFlash);
	// sanity check some settings
	for (int32 i = 0; i < MuzzleFlash.Num(); i++)
	{
		if (MuzzleFlash[i] != NULL)
		{
			if (RootComponent == NULL && MuzzleFlash[i]->IsRegistered())
			{
				MuzzleFlash[i]->DeactivateSystem();
				MuzzleFlash[i]->KillParticlesForced();
				MuzzleFlash[i]->UnregisterComponent(); // SCS components were registered without our permission
				MuzzleFlash[i]->bWasActive = false;
			}
			MuzzleFlash[i]->bAutoActivate = false;
			MuzzleFlash[i]->SecondsBeforeInactive = 0.0f;
			MuzzleFlash[i]->SetOnlyOwnerSee(false); // we handle this in AUTPlayerController::UpdateHiddenComponents() instead
			MuzzleFlash[i]->bUseAttachParentBound = true;
		}
	}

	// might have already been activated if at startup, see ClientGivenTo_Internal()
	if (CurrentState == NULL)
	{
		GotoState(InactiveState);
	}
	checkSlow(CurrentState != NULL);

	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(GetWorldSettings());
	if (GetMesh() && Settings->bUseCapsuleDirectShadowsForCharacter)
	{
		GetMesh()->bCastCapsuleDirectShadow = true;
	}
}

void AUTWeapon::GotoState(UUTWeaponState* NewState)
{
	if (NewState == NULL || !NewState->IsIn(this))
	{
		UE_LOG(UT, Warning, TEXT("Attempt to send %s to invalid state %s"), *GetName(), *GetFullNameSafe(NewState));
	}
	else if (ensureMsgf(UTOwner != NULL || NewState == InactiveState, TEXT("Attempt to send %s to state %s while not owned"), *GetName(), *GetNameSafe(NewState)))
	{
		if (CurrentState != NewState)
		{
			UUTWeaponState* PrevState = CurrentState;
			if (CurrentState != NULL)
			{
				CurrentState->EndState(); // NOTE: may trigger another GotoState() call
			}
			if (CurrentState == PrevState)
			{
				CurrentState = NewState;
				CurrentState->BeginState(PrevState); // NOTE: may trigger another GotoState() call
				StateChanged();
			}
		}
	}
}

void AUTWeapon::GotoFireMode(uint8 NewFireMode)
{
	if (FiringState.IsValidIndex(NewFireMode))
	{
		CurrentFireMode = NewFireMode;
		GotoState(FiringState[NewFireMode]);
	}
}

void AUTWeapon::GotoEquippingState(float OverflowTime)
{
	GotoState(EquippingState);
	if (CurrentState == EquippingState)
	{
		EquippingState->StartEquip(OverflowTime);
	}
}

void AUTWeapon::GivenTo(AUTCharacter* NewOwner, bool bAutoActivate)
{
	Super::GivenTo(NewOwner, bAutoActivate);

	// if character has ammo on it, transfer to weapon
	for (int32 i = 0; i < NewOwner->SavedAmmo.Num(); i++)
	{
		if (NewOwner->SavedAmmo[i].Type == GetClass())
		{
			AddAmmo(NewOwner->SavedAmmo[i].Amount);
			NewOwner->SavedAmmo.RemoveAt(i);
			break;
		}
	}

	if (bMustBeHolstered && UTOwner && (HasAnyAmmo() || bCanRegenerateAmmo))
	{
		AttachToHolster();
	}
}

void AUTWeapon::ClientGivenTo_Internal(bool bAutoActivate)
{
	// make sure we initialized our state; this can be triggered if the weapon is spawned at game startup, since BeginPlay() will be deferred
	if (CurrentState == NULL)
	{
		GotoState(InactiveState);
	}

	Super::ClientGivenTo_Internal(bAutoActivate);

	AUTPlayerController* UTPlayerController = Cast<AUTPlayerController>(UTOwner->Controller);
	if (UTPlayerController != nullptr)
	{
		// assign GroupSlot if required
		int32 MaxGroupSlot = 0;
		bool bDuplicateSlot = false;
		int32 MyGroup = UTPlayerController->GetWeaponGroup(this);
		for (TInventoryIterator<AUTWeapon> It(UTOwner); It; ++It)
		{
			if (*It != this && UTPlayerController->GetWeaponGroup(*It) == MyGroup)
			{
				MaxGroupSlot = FMath::Max<int32>(MaxGroupSlot, It->GroupSlot);
				bDuplicateSlot = true;
			}
		}
		if (bDuplicateSlot)
		{
			GroupSlot = MaxGroupSlot + 1;
		}

		if (bAutoActivate)
		{
			UTPlayerController->CheckAutoWeaponSwitch(this);
		}
	}
}

bool AUTWeapon::ShouldDropOnDeath()
{
	return (DroppedPickupClass != nullptr) && HasAnyAmmo();
}

void AUTWeapon::DropFrom(const FVector& StartLocation, const FVector& TossVelocity)
{
	if (Role == ROLE_Authority)
	{
		if (UTOwner != NULL && bMustBeHolstered)
		{
			DetachFromHolster();
		}

		if (!HasAnyAmmo() && !bCanRegenerateAmmo)
		{
			Destroy();
		}
		else
		{
			SetZoomState(EZoomState::EZS_NotZoomed);
			Super::DropFrom(StartLocation, TossVelocity);
			if (UTOwner == NULL && CurrentState != InactiveState)
			{
				UE_LOG(UT, Warning, TEXT("Weapon %s wasn't properly sent to Inactive state after being dropped!"), *GetName());
				GotoState(InactiveState);
			}
		}
	}
}

void AUTWeapon::InitializeDroppedPickup(class AUTDroppedPickup* Pickup)
{
	Super::InitializeDroppedPickup(Pickup);
	Pickup->SetWeaponSkin(WeaponSkin);
}

void AUTWeapon::Removed()
{
	GotoState(InactiveState);
	DetachFromOwner();
	if (bMustBeHolstered)
	{
		DetachFromHolster();
	}

	Super::Removed();
}

void AUTWeapon::ClientRemoved_Implementation()
{
	GotoState(InactiveState);
	if (Role < ROLE_Authority) // already happened on authority in Removed()
	{
		DetachFromOwner();
		if (bMustBeHolstered)
		{
			DetachFromHolster();
		}
	}

	AUTCharacter* OldOwner = UTOwner;

	Super::ClientRemoved_Implementation();

	if (OldOwner != NULL && (OldOwner->GetWeapon() == this || OldOwner->GetPendingWeapon() == this))
	{
		OldOwner->ClientWeaponLost(this);
	}
}

bool AUTWeapon::FollowsInList(AUTWeapon* OtherWeapon)
{
	// return true if this weapon is after OtherWeapon in the weapon list
	if (!OtherWeapon)
	{
		return true;
	}

	AUTPlayerController* UTPlayerController = Cast<AUTPlayerController>(UTOwner->Controller);

	int32 MyWeaponGroup = UTPlayerController ? UTPlayerController->GetWeaponGroup(this) : DefaultGroup;
	int32 OtherWeaponGroup = UTPlayerController ? UTPlayerController->GetWeaponGroup(OtherWeapon) : OtherWeapon->DefaultGroup;

	// if same group, order by slot, else order by group number
	return (MyWeaponGroup == OtherWeaponGroup) ? (GroupSlot > OtherWeapon->GroupSlot) : (MyWeaponGroup> OtherWeaponGroup);
}

void AUTWeapon::StartFire(uint8 FireModeNum)
{
	if (UTOwner && UTOwner->IsFiringDisabled())
	{
		return;
	}
	if (bRootWhileFiring && UTOwner != nullptr && UTOwner->GetCharacterMovement() != nullptr && UTOwner->GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		return;
	}
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS && GS->PreventWeaponFire())
	{
		return;
	}
	bool bClientFired = BeginFiringSequence(FireModeNum, false);
	if (Role < ROLE_Authority)
	{
		UUTWeaponStateFiring* CurrentFiringState = FiringState.IsValidIndex(FireModeNum) ? FiringState[FireModeNum] : nullptr;
		if (CurrentFiringState)
		{
			FireEventIndex++;
			if (FireEventIndex == 255)
			{
				FireEventIndex = 0;
			}
		}
		if (UTOwner)
		{
			float ZOffset = uint8(FMath::Clamp(UTOwner->GetPawnViewLocation().Z - UTOwner->GetActorLocation().Z + 127.5f, 0.f, 255.f));
			if (ZOffset != uint8(FMath::Clamp(UTOwner->BaseEyeHeight + 127.5f, 0.f, 255.f)))
			{
				ServerStartFireOffset(FireModeNum, FireEventIndex, ZOffset, bClientFired);
				QueueResendFire(true, FireModeNum, FireEventIndex, ZOffset, bClientFired);
				return;
			}
		}
		ServerStartFire(FireModeNum, FireEventIndex, bClientFired);
		QueueResendFire(true, FireModeNum, FireEventIndex, 0, bClientFired);
	}

}

void AUTWeapon::ResendNextFireEvent()
{
/*	if ((Role == ROLE_Authority) && (GetNetmode() != NM_Standalone))
	{
		UE_LOG(UT, Warning, TEXT("*********************************Server side weapon timer BAD!"));
		return;
	}*/
	if (!UTOwner || UTOwner->IsPendingKillPending() || (UTOwner->GetWeapon() != this))
	{
		ResendFireEvents.Empty();
		GetWorldTimerManager().ClearTimer(ResendFireHandle);
		return;
	}
	if (ResendFireEvents.Num() > 0)
	{
		FPendingFireEvent SendEvent = ResendFireEvents[0];
		if (SendEvent.bIsStartFire)
		{
			// UE_LOG(UT, Warning, TEXT("Resend StartFire %d event %d ZOffset %d"), SendEvent.FireModeNum, SendEvent.FireEventIndex, SendEvent.ZOffset);
			if (SendEvent.ZOffset == 0)
			{
				ResendServerStartFire(SendEvent.FireModeNum, SendEvent.FireEventIndex, SendEvent.bClientFired);
			}
			else
			{
				ResendServerStartFireOffset(SendEvent.FireModeNum, SendEvent.FireEventIndex, SendEvent.ZOffset, SendEvent.bClientFired);
			}
		}
		else
		{
			// UE_LOG(UT, Warning, TEXT("Resend StopFire %d event %d"), SendEvent.FireModeNum, SendEvent.FireEventIndex);
			ServerStopFire(SendEvent.FireModeNum, SendEvent.FireEventIndex);
		}
		ResendFireEvents.RemoveAt(0);
	}
	else if (UTOwner->GetWeapon() == this)
	{
		uint8 FireSettings = 0;
		int32 NumModes = FMath::Min(8, int32(GetNumFireModes()));
		for (int32 i = 0; i < NumModes; i++)
		{
			if (UTOwner->IsPendingFire(i))
			{
				FireSettings += 1 << i;
			}
		}
		//UE_LOG(UT, Warning, TEXT("UpdateFiringStates %d"), FireSettings);
		ServerUpdateFiringStates(FireSettings);
	}
	if (ResendFireEvents.Num() == 0)
	{
		if (!UTOwner || UTOwner->IsPendingKillPending() || (UTOwner->GetWeapon() != this))
		{
			GetWorldTimerManager().ClearTimer(ResendFireHandle);
		}
		else
		{
			//UE_LOG(UT, Warning, TEXT("SLOW LOOP"));
			GetWorldTimerManager().SetTimer(ResendFireHandle, this, &AUTWeapon::ResendNextFireEvent, 0.2f, true);
		}
	}
}

bool AUTWeapon::ServerUpdateFiringStates_Validate(uint8 FireSettings)
{
	return true;
}

void AUTWeapon::ClearFireEvents()
{
	ResendFireEvents.Empty();
	GetWorldTimerManager().ClearTimer(ResendFireHandle);
	if (Role == ROLE_Authority)
	{
		FireEventIndex = 0;
	}
}

void AUTWeapon::ServerUpdateFiringStates_Implementation(uint8 FireSettings)
{
//	UE_LOG(UT, Warning, TEXT("ServerUpdateFiringStates %d"), FireSettings);
	int32 NumModes = FMath::Min(8, int32(GetNumFireModes()));
	for (int32 i = 0; i < NumModes; i++)
	{
		bool bWantsFire = (FireSettings & (1 << i)) != 0;
		if ( FiringState[i] && (UTOwner->IsPendingFire(i) != bWantsFire))
		{
			// UE_LOG(UT, Warning, TEXT("%s IN %s Update firing %d to %d"), *GetName(), *CurrentState->GetName(), i, bWantsFire);
			if (bWantsFire)
			{
				ServerStartFire(i, -1, true);
			}
			else
			{
				ServerStopFire(i, -1);
			}
		}
	}
}

void AUTWeapon::QueueResendFire(bool bIsStartFire, uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired)
{
	// add a two resend fire events to the queue
	FPendingFireEvent NewFireEvent(bIsStartFire, FireModeNum, InFireEventIndex, ZOffset, bClientFired);
	ResendFireEvents.Add(NewFireEvent);
	ResendFireEvents.Add(NewFireEvent);
	if (!GetWorldTimerManager().IsTimerActive(ResendFireHandle) || (GetWorldTimerManager().GetTimerRemaining(ResendFireHandle) > 0.04f))
	{
		GetWorldTimerManager().SetTimer(ResendFireHandle, this, &AUTWeapon::ResendNextFireEvent, 0.04f, true);
	}
}

bool AUTWeapon::ValidateFireEventIndex(uint8 FireModeNum, uint8 InFireEventIndex)
{
	UUTWeaponStateFiring* CurrentFiringState = FiringState.IsValidIndex(FireModeNum) ? FiringState[FireModeNum] : nullptr;
	if (CurrentFiringState)
	{
		if (InFireEventIndex == 255)
		{
			return true;
		}
		if ((FireEventIndex >= InFireEventIndex) && (int32(FireEventIndex) < int32(InFireEventIndex)+128))
		{
			//UE_LOG(UT, Warning, TEXT("Skipping current %d in %d"), FireEventIndex, InFireEventIndex);
			return false;
		}
		//UE_LOG(UT, Warning, TEXT("Firing current %d in %d"), FireEventIndex, InFireEventIndex);
		FireEventIndex = InFireEventIndex;
		return true;
	}
	//UE_LOG(UT, Warning, TEXT("NO CurrentFiringState %d for %s"), FireModeNum, *GetName());
	return false;
}

void AUTWeapon::ServerStartFire_Implementation(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex) && UTOwner && !UTOwner->IsFiringDisabled())
	{
		if (CurrentState == InactiveState && !UTOwner->IsLocallyControlled())
		{
			UTOwner->ClientVerifyWeapon();
		}
		FireZOffsetTime = 0.f;
		BeginFiringSequence(FireModeNum, bClientFired);
		//UE_LOG(UT, Warning, TEXT("**** %s StartFire %d"), *GetName(), FireEventIndex);
	}
/*	else
	{
		UE_LOG(UT, Warning, TEXT("%s skip serverstartfire %d"), *GetName(), FireEventIndex);
	}*/
}

bool AUTWeapon::ServerStartFire_Validate(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired)
{
	return true;
}

void AUTWeapon::ServerStartFireOffset_Implementation(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex) && UTOwner && !UTOwner->IsFiringDisabled())
	{
		if (CurrentState == InactiveState && !UTOwner->IsLocallyControlled())
		{
			UTOwner->ClientVerifyWeapon();
		}
		FireZOffset = ZOffset - 127;
		FireZOffsetTime = GetWorld()->GetTimeSeconds();
		BeginFiringSequence(FireModeNum, bClientFired);
		//UE_LOG(UT, Warning, TEXT("***** %s StartFireOffset %d"), *GetName(), FireEventIndex);
	}
/*	else
	{
		UE_LOG(UT, Warning, TEXT("%s skip serverstartfire offset %d"), *GetName(), FireEventIndex);
	}*/
}

bool AUTWeapon::ServerStartFireOffset_Validate(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired)
{
	return true;
}

void AUTWeapon::ResendServerStartFire_Implementation(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex) && UTOwner && !UTOwner->IsFiringDisabled())
	{
		//UE_LOG(UT, Warning, TEXT("****RESENDStartFire mode %d %d"), FireModeNum, FireEventIndex);
		if (CurrentState == InactiveState && !UTOwner->IsLocallyControlled())
		{
			UTOwner->ClientVerifyWeapon();
		}
		FireZOffsetTime = 0.f;
		bNetDelayedShot = true;
		BeginFiringSequence(FireModeNum, bClientFired);
		bNetDelayedShot = false;
		//UE_LOG(UT, Warning, TEXT("****RESENDStartFire %d"), FireEventIndex);
	}
}

bool AUTWeapon::ResendServerStartFire_Validate(uint8 FireModeNum, uint8 InFireEventIndex, bool bClientFired)
{
	return true;
}

void AUTWeapon::ResendServerStartFireOffset_Implementation(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex) && UTOwner && !UTOwner->IsFiringDisabled())
	{
		//UE_LOG(UT, Warning, TEXT("****RESENDStartFireOffset mode %d %d"), FireModeNum, InFireEventIndex);
		if (CurrentState == InactiveState && !UTOwner->IsLocallyControlled())
		{
			UTOwner->ClientVerifyWeapon();
		}
		FireZOffset = ZOffset - 127;
		FireZOffsetTime = GetWorld()->GetTimeSeconds();
		bNetDelayedShot = true;
		BeginFiringSequence(FireModeNum, bClientFired);
		bNetDelayedShot = false;
		//UE_LOG(UT, Warning, TEXT("*****RESENDStartFireOffset %d"), InFireEventIndex);
	}
}

bool AUTWeapon::ResendServerStartFireOffset_Validate(uint8 FireModeNum, uint8 InFireEventIndex, uint8 ZOffset, bool bClientFired)
{
	return true;
}

bool AUTWeapon::WillSpawnShot(float DeltaTime)
{
	return (CurrentState != NULL) && CanFireAgain() && CurrentState->WillSpawnShot(DeltaTime);
}

bool AUTWeapon::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	if (UTOwner)
	{
		// Flag this player as not being idle
		if (Role == ROLE_Authority)
		{
			AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTOwner->PlayerState);
			if (UTPlayerState != nullptr)
			{
				UTPlayerState->NotIdle();
			}
		}

		UTOwner->SetPendingFire(FireModeNum, true);
		if (FiringState.IsValidIndex(FireModeNum) && CurrentState != EquippingState && CurrentState != UnequippingState)
		{
			FiringState[FireModeNum]->PendingFireStarted();
		}
		bool bResult = CurrentState->BeginFiringSequence(FireModeNum, bClientFired);
		if (CurrentState->IsFiring() && CurrentFireMode != FireModeNum)
		{
			OnMultiPress(FireModeNum);
		}
		return bResult;
	}
	return false;
}

void AUTWeapon::StopFire(uint8 FireModeNum)
{
	EndFiringSequence(FireModeNum);
	if (Role < ROLE_Authority)
	{
		UUTWeaponStateFiring* CurrentFiringState = FiringState.IsValidIndex(FireModeNum) ? FiringState[FireModeNum] : nullptr;
		if (CurrentFiringState)
		{
			FireEventIndex++;
			if (FireEventIndex == 255)
			{
				FireEventIndex = 0;
			}
		}
		if (GetWorld()->GetTimeSeconds() - LastContinuedFiring < 0.1f)
		{
			ServerStopFireRecent(FireModeNum, FireEventIndex);
		}
		else
		{
			ServerStopFire(FireModeNum, FireEventIndex);
		}
		QueueResendFire(false, FireModeNum, FireEventIndex, 0, false);
	}
}

void AUTWeapon::ServerStopFireRecent_Implementation(uint8 FireModeNum, uint8 InFireEventIndex)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex))
	{
		//UE_LOG(UT, Warning, TEXT("****StopFireRecent %d"), FireEventIndex);
		if (GetWorld()->GetTimeSeconds() - LastContinuedFiring > 0.2f)
		{
			//UE_LOG(UT, Warning, TEXT("MISSED RECENT"));
			if (FiringState.IsValidIndex(FireModeNum))
			{
				FiringState[FireModeNum]->PendingFireSequence = FireModeNum;
			}
		}
		EndFiringSequence(FireModeNum);
	}
}

bool AUTWeapon::ServerStopFireRecent_Validate(uint8 FireModeNum, uint8 InFireEventIndex)
{
	return true;
}

void AUTWeapon::ServerStopFire_Implementation(uint8 FireModeNum, uint8 InFireEventIndex)
{
	if (ValidateFireEventIndex(FireModeNum, InFireEventIndex))
	{
	//	UE_LOG(UT, Warning, TEXT("****StopFire %d"), InFireEventIndex);
		EndFiringSequence(FireModeNum);
	}
}

bool AUTWeapon::ServerStopFire_Validate(uint8 FireModeNum, uint8 InFireEventIndex)
{
	return true;
}

void AUTWeapon::EndFiringSequence(uint8 FireModeNum)
{
	if (UTOwner)
	{
		UTOwner->SetPendingFire(FireModeNum, false);
	}
	if (FiringState.IsValidIndex(FireModeNum) && CurrentState != EquippingState && CurrentState != UnequippingState)
	{
		FiringState[FireModeNum]->PendingFireStopped();
	}
	CurrentState->EndFiringSequence(FireModeNum);
}

void AUTWeapon::BringUp(float OverflowTime)
{
	AttachToOwner();
	OnBringUp();
	CurrentState->BringUp(OverflowTime);
	if ((Ammo <= LowAmmoThreshold) && (Ammo > 0) && (LowAmmoSound != nullptr)) 
	{
		PlayLowAmmoSound();
	}

	AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	AUTPlayerController* TutPlayer = (GameMode && GameMode->bPlayInventoryTutorialAnnouncements && UTOwner && (GetNetMode() == NM_Standalone)) ? Cast<AUTPlayerController>(UTOwner->GetController()) : nullptr;
	if (TutPlayer)
	{
		for (int32 Index = 0; Index < TutorialAnnouncements.Num(); Index++)
		{
			TutPlayer->PlayTutorialAnnouncement(Index, this);
		}
	}
}

float AUTWeapon::GetPutDownTime()
{
	return PutDownTime;
}

float AUTWeapon::GetBringUpTime()
{
	return BringUpTime;
}

bool AUTWeapon::PutDown()
{
	if (eventPreventPutDown())
	{
		return false;
	}
	else
	{
		SetZoomState(EZoomState::EZS_NotZoomed);
		CurrentState->PutDown();

		// Clear out the active crosshair
		if (ActiveCrosshair == nullptr)
		{
			ActiveCrosshair = nullptr;
		}

		return true;
	}
}

void AUTWeapon::SetupSpecialMaterials()
{
	//DO nothing by default
}

void AUTWeapon::UnEquip()
{

	GotoState(UnequippingState);
}

void AUTWeapon::AttachToHolster()
{
	UTOwner->SetHolsteredWeaponAttachmentClass(AttachmentType);
	UTOwner->UpdateHolsteredWeaponAttachment();
}

void AUTWeapon::DetachFromHolster()
{
	if (UTOwner != NULL)
	{
		UTOwner->SetHolsteredWeaponAttachmentClass(NULL);
		UTOwner->UpdateHolsteredWeaponAttachment();
	}
}

void AUTWeapon::AttachToOwner_Implementation()
{
	if (UTOwner == NULL)
	{
		return;
	}

	SetupSpecialMaterials();

	if (bMustBeHolstered)
	{
		// detach from holster if becoming held
		DetachFromHolster();
	}

	// attach
	if (Mesh != NULL && Mesh->SkeletalMesh != NULL)
	{
		UpdateWeaponHand();
		Mesh->AttachToComponent(UTOwner->FirstPersonMesh, FAttachmentTransformRules::KeepRelativeTransform, (UTOwner->FirstPersonMesh->SkeletalMesh != nullptr) ? HandsAttachSocket : NAME_None);
		if (HandsAttachSocket != NAME_None)
		{
			Mesh->bUseAttachParentBound = true;
		}
		if (ShouldPlay1PVisuals())
		{
			UTOwner->OnFirstPersonWeaponEquipped(this);
			Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose; // needed for anims to be ticked even if weapon is not currently displayed, e.g. sniper zoom
			Mesh->LastRenderTime = GetWorld()->TimeSeconds;
			Mesh->bRecentlyRendered = true;
			if (OverlayMesh != NULL)
			{
				OverlayMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose;
				OverlayMesh->LastRenderTime = GetWorld()->TimeSeconds;
				OverlayMesh->bRecentlyRendered = true;
			}
			UpdateViewBob(0.0f);
		}
	}

	// register components now
	bAttachingToOwner = true;
	RegisterAllComponents();
	RegisterAllActorTickFunctions(true, true); // 4.11 changed components to only get tick registered automatically if they're registered prior to BeginPlay()!
	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdateOverlays();
		UpdateOutline();
		SetSkin(UTOwner->GetSkin());
	}

	static FName FNameScale = TEXT("Scale");
	for (int i = 0; i < MeshMIDs.Num(); i++)
	{
		if (MeshMIDs[i])
		{
			MeshMIDs[i]->SetScalarParameterValue(FNameScale, WeaponRenderScale);
		}
	}
	for (int i = 0; i < UTOwner->FirstPersonMeshMIDs.Num(); i++)
	{
		if (UTOwner->FirstPersonMeshMIDs[i])
		{
			UTOwner->FirstPersonMeshMIDs[i]->SetScalarParameterValue(FNameScale, WeaponRenderScale);
		}
	}
}

void AUTWeapon::UpdateWeaponHand()
{
	if (Mesh != NULL && UTOwner != NULL && !UTOwner->IsPendingKillPending())
	{
		FirstPMeshOffset = FVector::ZeroVector;
		FirstPMeshRotation = FRotator::ZeroRotator;

		if (MuzzleFlashDefaultTransforms.Num() == 0)
		{
			for (UParticleSystemComponent* PSC : MuzzleFlash)
			{
				MuzzleFlashDefaultTransforms.Add((PSC == NULL) ? FTransform::Identity : PSC->GetRelativeTransform());
				MuzzleFlashSocketNames.Add((PSC == NULL) ? NAME_None : PSC->GetAttachSocketName());
			}
		}
		else
		{
			for (int32 i = 0; i < FMath::Min3<int32>(MuzzleFlash.Num(), MuzzleFlashDefaultTransforms.Num(), MuzzleFlashSocketNames.Num()); i++)
			{
				if (MuzzleFlash[i] != NULL)
				{
					MuzzleFlash[i]->AttachToComponent(MuzzleFlash[i]->GetAttachParent(), FAttachmentTransformRules::SnapToTargetIncludingScale, MuzzleFlashSocketNames[i]);
					MuzzleFlash[i]->bUseAttachParentBound = true;
					MuzzleFlash[i]->SetRelativeTransform(MuzzleFlashDefaultTransforms[i]);
				}
			}
		}

		// If we're attached, make sure that we update to the right hands socket
		if (Mesh->GetAttachParent() && Mesh->GetAttachSocketName() != HandsAttachSocket)
		{
			Mesh->AttachToComponent(Mesh->GetAttachParent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false), HandsAttachSocket);
		}

		if (HandsAttachSocket == NAME_None)
		{
			UTOwner->FirstPersonMesh->SetRelativeTransform(FTransform::Identity);
			UTOwner->FirstPersonMesh->SetRelativeLocation(UTOwner->GetClass()->GetDefaultObject<AUTCharacter>()->FirstPersonMesh->RelativeLocation);
			UTOwner->FirstPersonMeshBoundSphere->SetRelativeLocation(UTOwner->GetClass()->GetDefaultObject<AUTCharacter>()->FirstPersonMeshBoundSphere->RelativeLocation);
		}
		else
		{
			USkeletalMeshComponent* DefaultHands = UTOwner->GetClass()->GetDefaultObject<AUTCharacter>()->FirstPersonMesh;
			UTOwner->FirstPersonMesh->RelativeLocation = DefaultHands->RelativeLocation;
			UTOwner->FirstPersonMesh->RelativeRotation = DefaultHands->RelativeRotation;
			UTOwner->FirstPersonMesh->RelativeScale3D = DefaultHands->RelativeScale3D;
			UTOwner->FirstPersonMesh->UpdateComponentToWorld();
		}

		USkeletalMeshComponent* AdjustMesh = (HandsAttachSocket != NAME_None) ? UTOwner->FirstPersonMesh : Mesh;
		USkeletalMeshComponent* AdjustMeshArchetype = Cast<USkeletalMeshComponent>(AdjustMesh->GetArchetype());

		switch (GetWeaponHand())
		{
			case EWeaponHand::HAND_Center:
				AdjustMesh->SetRelativeLocationAndRotation(AdjustMeshArchetype->RelativeLocation + LowMeshOffset, AdjustMeshArchetype->RelativeRotation);
				break;
			case EWeaponHand::HAND_Right:
				AdjustMesh->SetRelativeLocationAndRotation(AdjustMeshArchetype->RelativeLocation, AdjustMeshArchetype->RelativeRotation);
				break;
			case EWeaponHand::HAND_Left:
			{
				// TODO: should probably mirror, but mirroring breaks sockets at the moment (engine bug)
				AdjustMesh->SetRelativeLocation(AdjustMeshArchetype->RelativeLocation * FVector(1.0f, -1.0f, 1.0f));
				FRotator AdjustedRotation = (FRotationMatrix(AdjustMeshArchetype->RelativeRotation) * FScaleMatrix(FVector(1.0f, 1.0f, -1.0f))).Rotator();
				AdjustMesh->SetRelativeRotation(AdjustedRotation);
				break;
			}
			case EWeaponHand::HAND_Hidden:
			{
				AdjustMesh->SetRelativeLocationAndRotation(AdjustMeshArchetype->RelativeLocation + VeryLowMeshOffset, AdjustMeshArchetype->RelativeRotation);
				break;
			}
		}
	}
}

EWeaponHand AUTWeapon::GetWeaponHand() const
{
	if (UTOwner == NULL && Role == ROLE_Authority)
	{
		return EWeaponHand::HAND_Right;
	}
	else
	{
		AUTPlayerController* Viewer = NULL;
		if (UTOwner != NULL)
		{
			if (Role == ROLE_Authority)
			{
				Viewer = Cast<AUTPlayerController>(UTOwner->Controller);
			}
			if (Viewer == NULL)
			{
				Viewer = UTOwner->GetLocalViewer();
				if (Viewer == NULL && UTOwner->Controller != NULL && UTOwner->Controller->IsLocalPlayerController())
				{
					// this can happen during initial replication; Controller might be set but ViewTarget not
					Viewer = Cast<AUTPlayerController>(UTOwner->Controller);
				}
			}
		}
		return (Viewer != NULL) ? Viewer->GetWeaponHand() : EWeaponHand::HAND_Right;
	}
}

void AUTWeapon::DetachFromOwner_Implementation()
{
	StopFiringEffects();
	// make sure particle system really stops NOW since we're going to unregister it
	for (int32 i = 0; i < MuzzleFlash.Num(); i++)
	{
		if (MuzzleFlash[i] != NULL)
		{
			UParticleSystem* SavedTemplate = MuzzleFlash[i]->Template;
			MuzzleFlash[i]->DeactivateSystem();
			MuzzleFlash[i]->KillParticlesForced();
			// FIXME: KillParticlesForced() doesn't kill particles immediately for GPU particles, but the below does...
			MuzzleFlash[i]->SetTemplate(NULL);
			MuzzleFlash[i]->SetTemplate(SavedTemplate);
		}
	}
	if (Mesh != NULL && Mesh->SkeletalMesh != NULL)
	{
		Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		if (OverlayMesh != NULL)
		{
			OverlayMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
		}
	}
	if (CustomDepthMesh != NULL)
	{
		CustomDepthMesh->DestroyComponent();
		CustomDepthMesh = NULL;
	}
	// unregister components so they go away
	UnregisterAllComponents();

	if (bMustBeHolstered && (HasAnyAmmo() || bCanRegenerateAmmo) && UTOwner != NULL && !UTOwner->IsDead() && !IsPendingKillPending())
	{
		AttachToHolster();
	}
}

bool AUTWeapon::IsChargedFireMode(uint8 TestMode) const
{
	return FiringState.IsValidIndex(TestMode) && Cast<UUTWeaponStateFiringCharged>(FiringState[TestMode]) != NULL;
}

bool AUTWeapon::ShouldPlay1PVisuals() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	else
	{
		// note we can't check Mesh->LastRenderTime here because of the hidden weapon setting!
		return UTOwner && UTOwner->GetLocalViewer() && !UTOwner->GetLocalViewer()->IsBehindView();
	}
}

void AUTWeapon::PlayWeaponAnim(UAnimMontage* WeaponAnim, UAnimMontage* HandsAnim, float RateOverride)
{
	if (RateOverride <= 0.0f)
	{
		RateOverride = UTOwner ? UTOwner->GetFireRateMultiplier() : 1.f;
	}
	if (UTOwner != NULL && !UTOwner->IsPendingKillPending())
	{
		if (WeaponAnim != NULL)
		{
			UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(WeaponAnim, RateOverride);
			}
		}
		if (HandsAnim != NULL)
		{
			UAnimInstance* AnimInstance = UTOwner->FirstPersonMesh->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(HandsAnim, RateOverride);
			}
		}
	}
}

UAnimMontage* AUTWeapon::GetFiringAnim(uint8 FireMode, bool bOnHands) const
{
	const TArray<UAnimMontage*>& AnimArray = bOnHands ? FireAnimationHands : FireAnimation;
	return (AnimArray.IsValidIndex(CurrentFireMode) ? AnimArray[CurrentFireMode] : NULL);
}

void AUTWeapon::PlayFiringSound(uint8 EffectFiringMode)
{
	// try and play the sound if specified
	if (FireSound.IsValidIndex(EffectFiringMode) && FireSound[EffectFiringMode] != NULL)
	{
		if (FPFireSound.IsValidIndex(CurrentFireMode) && FPFireSound[CurrentFireMode] != NULL && Cast<APlayerController>(UTOwner->Controller) != NULL && UTOwner->IsLocallyControlled())
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), FPFireSound[CurrentFireMode], UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, FireSoundAmp);
		}
		else
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), FireSound[EffectFiringMode], UTOwner, SRT_AllButOwner, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, FireSoundAmp);
		}
	}
}

void AUTWeapon::PlayFiringEffects()
{
	if (UTOwner != NULL)
	{
		//UE_LOG(UT, Warning, TEXT("PlayFiringEffects at %f"), GetWorld()->GetTimeSeconds());
		uint8 EffectFiringMode = (Role == ROLE_Authority || UTOwner->Controller != NULL) ? CurrentFireMode : UTOwner->FireMode;
		PlayFiringSound(EffectFiringMode);

		// reload sound on local shooter
		if ((GetNetMode() != NM_DedicatedServer) && UTOwner && UTOwner->GetLocalViewer())
		{
			if (ReloadSound.IsValidIndex(EffectFiringMode) && ReloadSound[EffectFiringMode] != NULL)
			{
				UUTGameplayStatics::UTPlaySound(GetWorld(), ReloadSound[EffectFiringMode], UTOwner, SRT_None);
			}
			if ((Ammo <= LowAmmoThreshold) && (Ammo > 0) && (LowAmmoSound != nullptr))
			{
				AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
				if (!GameMode || GameMode->bAmmoIsLimited)
				{
					GetWorldTimerManager().SetTimer(PlayLowAmmoSoundHandle, this, &AUTWeapon::PlayLowAmmoSound, LowAmmoSoundDelay, false);
				}
			}
		}
		UTOwner->TargetEyeOffset.X = FiringViewKickback;
		UTOwner->TargetEyeOffset.Y = FiringViewKickbackY;
		AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
		if (PC != NULL)
		{
			PC->AddHUDImpulse(HUDViewKickback);
		}
		if (ShouldPlay1PVisuals())
		{
			// try and play a firing animation if specified
			PlayWeaponAnim(GetFiringAnim(EffectFiringMode, false), GetFiringAnim(EffectFiringMode, true));

			// muzzle flash
			if (MuzzleFlash.IsValidIndex(EffectFiringMode) && MuzzleFlash[EffectFiringMode] != NULL && MuzzleFlash[EffectFiringMode]->Template != NULL)
			{
				// if we detect a looping particle system, then don't reactivate it
				if (!MuzzleFlash[EffectFiringMode]->bIsActive || MuzzleFlash[EffectFiringMode]->bSuppressSpawning || !IsLoopingParticleSystem(MuzzleFlash[EffectFiringMode]->Template))
				{
					MuzzleFlash[EffectFiringMode]->ActivateSystem();
				}
			}
		}
	}
}

void AUTWeapon::PlayLowAmmoSound()
{
	if (UTOwner && UTOwner->GetLocalViewer() && Cast<AUTPlayerController>(UTOwner->GetController()))
	{
		((AUTPlayerController*)(UTOwner->GetController()))->UTClientPlaySound(LowAmmoSound);
	}
}

void AUTWeapon::StopFiringEffects_Implementation()
{
	for (UParticleSystemComponent* MF : MuzzleFlash)
	{
		if (MF != NULL)
		{
			MF->DeactivateSystem();
		}
	}
}

FHitResult AUTWeapon::GetImpactEffectHit(APawn* Shooter, const FVector& StartLoc, const FVector& TargetLoc)
{
	// trace for precise hit location and hit normal
	FHitResult Hit;
	FVector TargetToGun = (StartLoc - TargetLoc).GetSafeNormal();
	if (Shooter->GetWorld()->LineTraceSingleByChannel(Hit, TargetLoc + TargetToGun * 32.0f, TargetLoc - TargetToGun * 32.0f, COLLISION_TRACE_WEAPON, FCollisionQueryParams(FName(TEXT("ImpactEffect")), true, Shooter)))
	{
		return Hit;
	}
	else
	{
		return FHitResult(NULL, NULL, TargetLoc, TargetToGun);
	}
}

void AUTWeapon::GetImpactSpawnPosition(const FVector& TargetLoc, FVector& SpawnLocation, FRotator& SpawnRotation)
{
	if (UTOwner != NULL && (ZoomState != EZoomState::EZS_NotZoomed))
	{
		SpawnRotation = UTOwner->CharacterCameraComponent->GetComponentRotation();
		SpawnLocation = UTOwner->CharacterCameraComponent->GetComponentLocation() + SpawnRotation.RotateVector(FVector(-50.0f, 0.0f, -50.0f));
	}
	else
	{
		SpawnLocation = (MuzzleFlash.IsValidIndex(CurrentFireMode) && MuzzleFlash[CurrentFireMode] != NULL) ? MuzzleFlash[CurrentFireMode]->GetComponentLocation() : UTOwner->GetActorLocation() + UTOwner->GetControlRotation().RotateVector(FireOffset);
		SpawnRotation = (MuzzleFlash.IsValidIndex(CurrentFireMode) && MuzzleFlash[CurrentFireMode] != NULL) ? MuzzleFlash[CurrentFireMode]->GetComponentRotation() : (TargetLoc - SpawnLocation).Rotation();
	}
}

bool AUTWeapon::CancelImpactEffect(const FHitResult& ImpactHit) const
{
	return (!ImpactHit.Actor.IsValid() && !ImpactHit.Component.IsValid()) || Cast<AUTCharacter>(ImpactHit.Actor.Get());
}

void AUTWeapon::PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		// fire effects
		static FName NAME_HitLocation(TEXT("HitLocation"));
		static FName NAME_LocalHitLocation(TEXT("LocalHitLocation"));
		FireEffectCount++;
		if (FireEffect.IsValidIndex(FireMode) && (FireEffect[FireMode] != NULL) && (FireEffectCount >= FireEffectInterval))
		{
			FVector AdjustedSpawnLocation = SpawnLocation;
			// panini project the location, if necessary
			if (Mesh != NULL)
			{
				for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
				{
					if (It->PlayerController != NULL && It->PlayerController->GetViewTarget() == UTOwner)
					{
						UUTGameViewportClient* UTViewport = Cast<UUTGameViewportClient>(It->ViewportClient);
						if (UTViewport != NULL)
						{
							FVector PaniniAdjustedSpawnLocation = UTViewport->PaniniProjectLocationForPlayer(*It, SpawnLocation, Mesh->GetMaterial(0));
							if (!PaniniAdjustedSpawnLocation.ContainsNaN())
							{
								AdjustedSpawnLocation = PaniniAdjustedSpawnLocation;
							}
							else
							{
								UE_LOG(UT, Warning, TEXT("Panini projection returned NaN %s"), *GetName());
								PaniniAdjustedSpawnLocation.DiagnosticCheckNaN();
							}

							break;
						}
					}
				}
			}
			FireEffectCount = 0;
			AdjustedSpawnLocation.DiagnosticCheckNaN();
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireEffect[FireMode], AdjustedSpawnLocation, SpawnRotation, true);

			// limit dist to target
			FVector AdjustedTargetLoc = (MaxTracerDist > 0.0f && ((TargetLoc - AdjustedSpawnLocation).SizeSquared() > FMath::Square<float>(MaxTracerDist)))
				? AdjustedSpawnLocation + MaxTracerDist * (TargetLoc - AdjustedSpawnLocation).GetSafeNormal()
				: TargetLoc;
			PSC->SetVectorParameter(NAME_HitLocation, AdjustedTargetLoc);
			PSC->SetVectorParameter(NAME_LocalHitLocation, PSC->ComponentToWorld.InverseTransformPosition(AdjustedTargetLoc));
			ModifyFireEffect(PSC);
		}
		// perhaps the muzzle flash also contains hit effect (constant beam, etc) so set the parameter on it instead
		else if (MuzzleFlash.IsValidIndex(FireMode) && MuzzleFlash[FireMode] != NULL)
		{
			MuzzleFlash[FireMode]->SetVectorParameter(NAME_HitLocation, TargetLoc);
			MuzzleFlash[FireMode]->SetVectorParameter(NAME_LocalHitLocation, MuzzleFlash[FireMode]->ComponentToWorld.InverseTransformPositionNoScale(TargetLoc));
		}

		// Always spawn effects instigated by local player unless beyond cull distance
		if ((TargetLoc - LastImpactEffectLocation).Size() >= ImpactEffectSkipDistance || GetWorld()->TimeSeconds - LastImpactEffectTime >= MaxImpactEffectSkipTime)
		{
			if (ImpactEffect.IsValidIndex(FireMode) && ImpactEffect[FireMode] != NULL)
			{
				FHitResult ImpactHit = GetImpactEffectHit(UTOwner, SpawnLocation, TargetLoc);
				if (ImpactHit.Component.IsValid() && !CancelImpactEffect(ImpactHit))
				{
					ImpactEffect[FireMode].GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(ImpactHit.Normal.Rotation(), ImpactHit.Location), ImpactHit.Component.Get(), NULL, UTOwner->Controller);
				}
			}
			LastImpactEffectLocation = TargetLoc;
			LastImpactEffectTime = GetWorld()->TimeSeconds;
		}
	}
}

void AUTWeapon::DeactivateSpawnProtection()
{
	if (UTOwner)
	{
		UTOwner->DeactivateSpawnProtection();
	}
}

void AUTWeapon::FireShot()
{
	UTOwner->DeactivateSpawnProtection();
	ConsumeAmmo(CurrentFireMode);

	if (!FireShotOverride() && GetUTOwner() != NULL) // script event may kill user
	{
		if (ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL)
		{
			FireProjectile();
		}
		else if (InstantHitInfo.IsValidIndex(CurrentFireMode) && InstantHitInfo[CurrentFireMode].DamageType != NULL)
		{
			if (InstantHitInfo[CurrentFireMode].ConeDotAngle > 0.0f)
			{
				FireCone();
			}
			else
			{
				FHitResult OutHit;
				FireInstantHit(true, &OutHit);
				if (bTrackHitScanReplication && (UTOwner == nullptr || !UTOwner->IsLocallyControlled() || Cast<APlayerController>(UTOwner->GetController()) != nullptr))
				{
					HitScanHitChar = Cast<AUTCharacter>(OutHit.Actor.Get());
					if ((Role < ROLE_Authority) && HitScanHitChar)
					{
						ServerHitScanHit(HitScanHitChar, FireEventIndex);
					}
					else if ((Role == ROLE_Authority) && !HitScanHitChar && ReceivedHitScanHitChar && GetWorld()->GetGameState<AUTGameState>() && GetWorld()->GetGameState<AUTGameState>()->bDebugHitScanReplication)
					{
						HitScanIndex = FireEventIndex;
						HitScanTime = (GetUTOwner() && GetUTOwner()->UTCharacterMovement) ? GetUTOwner()->UTCharacterMovement->GetCurrentMovementTime() : 0.f;
						HitScanCharLoc = HitScanHitChar ? HitScanHitChar->GetActorLocation() : FVector::ZeroVector;
						HitScanHeight = HitScanHitChar ? HitScanHitChar->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 108.f;
						HitScanStart = GetFireStartLoc();
						HitScanEnd = OutHit.Location;
						AUTPlayerController* UTPC = UTOwner ? Cast<AUTPlayerController>(UTOwner->Controller) : NULL;
						float PredictionTime = UTPC ? UTPC->GetPredictionTime() : 0.f;
						FVector MissedLoc = ReceivedHitScanHitChar->GetRewindLocation(PredictionTime, UTPC);
						ClientMissedHitScan(HitScanStart, HitScanEnd, MissedLoc, HitScanTime, HitScanIndex);
					}
				}
				ReceivedHitScanHitChar = nullptr;
			}
		}
		//UE_LOG(UT, Warning, TEXT("FireShot"));
		PlayFiringEffects();
	}
	if (GetUTOwner() != NULL)
	{
		GetUTOwner()->InventoryEvent(InventoryEventName::FiredWeapon);
	}
	FireZOffsetTime = 0.f;
}

bool AUTWeapon::ServerHitScanHit_Validate(AUTCharacter* HitScanChar, uint8 HitScanEventIndex)
{
	return true;
}

void AUTWeapon::ServerHitScanHit_Implementation(AUTCharacter* HitScanChar, uint8 HitScanEventIndex)
{
	ReceivedHitScanHitChar = HitScanChar;
	ReceivedHitScanIndex = HitScanEventIndex;
}

void AUTWeapon::ClientMissedHitScan_Implementation(FVector_NetQuantize MissedHitScanStart, FVector_NetQuantize MissedHitScanEnd, FVector_NetQuantize MissedHitScanLoc, float MissedHitScanTime, uint8 MissedHitScanIndex)
{
	DrawDebugLine(GetWorld(), HitScanStart, HitScanEnd, FColor::Green, false, 8.f);
	DrawDebugLine(GetWorld(), MissedHitScanStart, MissedHitScanEnd, FColor::Yellow, false, 8.f);
	DrawDebugCapsule(GetWorld(), HitScanCharLoc, HitScanHeight, 40.f, FQuat::Identity, FColor::Green, false, 8.f);
	AUTPlayerController* PC = GetUTOwner() ? Cast<AUTPlayerController>(GetUTOwner()->GetController()) : nullptr;
	if (PC)
	{
		PC->ClientSay(PC->UTPlayerState, FString::Printf(TEXT("HIT MISMATCH LOCAL index %d time %f      SERVER index %d time %f error distance %f"), HitScanIndex, HitScanTime, MissedHitScanIndex, MissedHitScanTime, (HitScanCharLoc - MissedHitScanLoc).Size()), ChatDestinations::System);
	}
}


bool AUTWeapon::IsFiring() const
{
	// first person spectating needs to use the replicated info from the owner
	if (Role < ROLE_Authority && UTOwner != nullptr && UTOwner->Role < ROLE_AutonomousProxy && UTOwner->Controller == nullptr && CurrentState == InactiveState)
	{
		return UTOwner->FlashCount > 0 || !UTOwner->FlashLocation.Position.IsZero();
	}
	else
	{
		return CurrentState->IsFiring();
	}
}

void AUTWeapon::AddAmmo(int32 Amount)
{
	if (Role == ROLE_Authority)
	{
		Ammo = FMath::Clamp<int32>(Ammo + Amount, 0, MaxAmmo);

		// trigger weapon switch if necessary
		if (UTOwner != NULL && UTOwner->IsLocallyControlled())
		{
			OnRep_Ammo();
		}
	}
}

void AUTWeapon::SwitchToBestWeaponIfNoAmmo()
{
	if (UTOwner && UTOwner->IsLocallyControlled() && UTOwner->GetPendingWeapon() == NULL && !HasAnyAmmo())
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
		if (PC != NULL)
		{
			if (!bCanRegenerateAmmo)
			{
				//UE_LOG(UT, Warning, TEXT("********** %s ran out of ammo for %s"), *GetName(), *PC->GetHumanReadableName());
				PC->SwitchToBestWeapon();
			}
		}
		else
		{
			AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
			if (B != NULL)
			{
				B->SwitchToBestWeapon();
			}
		}
	}
}

void AUTWeapon::OnRep_Ammo()
{
	SwitchToBestWeaponIfNoAmmo();
}

void AUTWeapon::OnRep_AttachmentType()
{
	if (UTOwner)
	{
		GetUTOwner()->UpdateWeaponAttachment();
	}
	else
	{	
		AttachmentType = NULL;
	}
}

void AUTWeapon::ConsumeAmmo(uint8 FireModeNum)
{
	if (Role == ROLE_Authority )
	{
		AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (AmmoCost.IsValidIndex(FireModeNum) && (!GameMode || GameMode->bAmmoIsLimited || (Ammo > 9)))
		{
			AddAmmo(-AmmoCost[FireModeNum]);
		}
		else if (!AmmoCost.IsValidIndex(FireModeNum))
		{
			UE_LOG(UT, Warning, TEXT("Invalid fire mode %i in %s::ConsumeAmmo()"), int32(FireModeNum), *GetName());
		}
	}
}

bool AUTWeapon::HasAmmo(uint8 FireModeNum)
{
	return (AmmoCost.IsValidIndex(FireModeNum) && Ammo >= FMath::Min(1,AmmoCost[FireModeNum]));
}

bool AUTWeapon::NeedsAmmoDisplay_Implementation() const
{
	for (int32 i = GetNumFireModes() - 1; i >= 0; i--)
	{
		if (AmmoCost[i] > 0)
		{
			return true;
		}
	}
	return false;
}

bool AUTWeapon::HasAnyAmmo()
{
	bool bHadCost = false;

	// only consider zero cost firemodes as having ammo if they all have no cost
	// the assumption here is that for most weapons with an ammo-using firemode,
	// any that don't use ammo are support firemodes that can't function effectively without the other one
	for (int32 i = GetNumFireModes() - 1; i >= 0; i--)
	{
		if (AmmoCost[i] > 0)
		{
			bHadCost = true;
			if (HasAmmo(i))
			{
				return true;
			}
		}
	}
	return !bHadCost;
}

FVector AUTWeapon::GetFireStartLoc(uint8 FireMode)
{
	// default to current firemode
	if (FireMode == 255)
	{
		FireMode = CurrentFireMode;
	}
	if (UTOwner == NULL)
	{
		UE_LOG(UT, Warning, TEXT("%s::GetFireStartLoc(): No Owner (died while firing?)"), *GetName());
		return FVector::ZeroVector;
	}
	else
	{
		const bool bIsFirstPerson = Cast<APlayerController>(UTOwner->Controller) != NULL; // FIXMESTEVE TODO: first person view check (need to make sure sync'ed with server)
		FVector BaseLoc;
		if (bFPFireFromCenter && bIsFirstPerson)
		{
			BaseLoc = UTOwner->GetPawnViewLocation();
			if (GetWorld()->GetTimeSeconds() - FireZOffsetTime < 0.06f)
			{
				BaseLoc.Z = FireZOffset + UTOwner->GetActorLocation().Z;
			}
		}
		else
		{
			BaseLoc = UTOwner->GetActorLocation();
		}

		if (bNetDelayedShot)
		{
			// adjust for delayed shot to position client shot from
			BaseLoc = BaseLoc + UTOwner->GetDelayedShotPosition() - UTOwner->GetActorLocation();
		}
		// ignore offset for instant hit shots in first person
		if (FireOffset.IsZero() || (bIsFirstPerson && bFPIgnoreInstantHitFireOffset && InstantHitInfo.IsValidIndex(FireMode) && InstantHitInfo[FireMode].DamageType != NULL))
		{
			return BaseLoc;
		}
		else
		{
			FVector AdjustedFireOffset = FVector::ZeroVector;
			AdjustedFireOffset.X = FireOffset.X;

			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			FVector FinalLoc = BaseLoc + GetBaseFireRotation().RotateVector(AdjustedFireOffset);
			// trace back towards Instigator's collision, then trace from there to desired location, checking for intervening world geometry
			FCollisionShape Collider;
			if (ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL && ProjClass[CurrentFireMode].GetDefaultObject()->CollisionComp != NULL)
			{
				Collider = FCollisionShape::MakeSphere(ProjClass[CurrentFireMode].GetDefaultObject()->CollisionComp->GetUnscaledSphereRadius());
			}
			else
			{
				Collider = FCollisionShape::MakeSphere(0.0f);
			}

			static FName NAME_WeaponStartLoc(TEXT("WeaponStartLoc"));
			FCollisionQueryParams Params(NAME_WeaponStartLoc, true, UTOwner);
			FHitResult Hit;
			if (GetWorld()->SweepSingleByChannel(Hit, BaseLoc, FinalLoc, FQuat::Identity, COLLISION_TRACE_WEAPON, Collider, Params))
			{
				FinalLoc = Hit.Location - (FinalLoc - BaseLoc).GetSafeNormal();
			}
			return FinalLoc;
		}
	}
}

FRotator AUTWeapon::GetBaseFireRotation()
{
	if (UTOwner == NULL)
	{
		UE_LOG(UT, Warning, TEXT("%s::GetBaseFireRotation(): No Owner (died while firing?)"), *GetName());
		return FRotator::ZeroRotator;
	}
	else if (bNetDelayedShot)
	{
		return UTOwner->GetDelayedShotRotation();
	}
	else
	{
		return UTOwner->GetViewRotation();
	}
}

FRotator AUTWeapon::GetAdjustedAim_Implementation(FVector StartFireLoc)
{
	FRotator BaseAim = GetBaseFireRotation();
	GuessPlayerTarget(StartFireLoc, BaseAim.Vector());
	if (Spread.IsValidIndex(CurrentFireMode) && Spread[CurrentFireMode] > 0.0f)
	{
		// add in any spread
		FRotationMatrix Mat(BaseAim);
		FVector X, Y, Z;
		Mat.GetScaledAxes(X, Y, Z);

		NetSynchRandomSeed();
		float RandY = 0.5f * (FMath::FRand() + FMath::FRand() - 1.f);
		float RandZ = FMath::Sqrt(0.25f - FMath::Square(RandY)) * (FMath::FRand() + FMath::FRand() - 1.f);
		return (X + RandY * Spread[CurrentFireMode] * Y + FMath::Clamp(RandZ * VerticalSpreadScaling, -1.f*MaxVerticalSpread, MaxVerticalSpread) * Spread[CurrentFireMode] * Z).Rotation();
	}
	else
	{
		return BaseAim;
	}
}

AUTPlayerController* AUTWeapon::GetCurrentTargetPC()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
	APawn* CurrentTarget = PC ? PC->LastShotTargetGuess : TargetedCharacter;
	if (CurrentTarget && !CurrentTarget->GetController())
	{
		AUTCharacter* CurrentTargetChar = Cast<AUTCharacter>(CurrentTarget);
		if (CurrentTargetChar && CurrentTargetChar->OldPlayerState)
		{
			return Cast<AUTPlayerController>(CurrentTargetChar->OldPlayerState->GetOwner());
		}
	}

	return CurrentTarget ? Cast<AUTPlayerController>(CurrentTarget->GetController()) : nullptr;
}

void AUTWeapon::GuessPlayerTarget(const FVector& StartFireLoc, const FVector& FireDir)
{
	if (Role == ROLE_Authority && UTOwner != NULL)
	{
		TargetedCharacter = nullptr;
		AUTPlayerState* PS = nullptr;
		AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
		if (PC != NULL)
		{
			float MaxRange = 100000.0f; // TODO: calc projectile mode range?
			if (InstantHitInfo.IsValidIndex(CurrentFireMode) && InstantHitInfo[CurrentFireMode].DamageType != NULL)
			{
				MaxRange = InstantHitInfo[CurrentFireMode].TraceRange * 1.2f; // extra since player may miss intended target due to being out of range
			}
			float BestAim = 0.f;
			float BestDist = 0.f;
			float BestOffset = 0.f;
			PC->LastShotTargetGuess = UUTGameplayStatics::ChooseBestAimTarget(PC, StartFireLoc, FireDir, 0.85f, MaxRange, 500.f, APawn::StaticClass(), &BestAim, &BestDist, &BestOffset);
			TargetedCharacter = Cast<AUTCharacter>(PC->LastShotTargetGuess);
			PS = PC->UTPlayerState;
		}
		else if (Cast<AUTBot>(UTOwner->GetController()))
		{
			TargetedCharacter = Cast<AUTCharacter>(((AUTBot*)(UTOwner->GetController()))->GetEnemy());
			PS = Cast<AUTPlayerState>(UTOwner->GetController()->PlayerState);
		}
		if (TargetedCharacter)
		{
			TargetedCharacter->TargetedBy(UTOwner, PS);
		}
	}
}

void AUTWeapon::NetSynchRandomSeed()
{
	AUTPlayerController* OwningPlayer = UTOwner ? Cast<AUTPlayerController>(UTOwner->GetController()) : NULL;
	if (OwningPlayer && UTOwner)
	{
		FMath::RandInit(10000.f*UTOwner->GetCurrentSynchTime(bNetDelayedShot));
	}
}

bool AUTWeapon::ShouldTraceIgnore(AActor* TestActor)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (bIgnoreShockballs && Cast<AUTProj_ShockBall>(TestActor))
	{
		return true;
	}
	else if (UTOwner != nullptr && (Cast<AUTProj_WeaponScreen>(TestActor) != nullptr || (Cast<AUTTeamDeco>(TestActor) != nullptr && !((AUTTeamDeco*)(TestActor))->bBlockTeamProjectiles)))
	{
		return (GS != nullptr && !GS->bTeamProjHits && GS->OnSameTeam(UTOwner, TestActor));
	}
	else
	{
		return false;
	}
}

void AUTWeapon::HitScanTrace(const FVector& StartLocation, const FVector& EndTrace, float TraceRadius, FHitResult& Hit, float PredictionTime)
{
	ECollisionChannel TraceChannel = COLLISION_TRACE_WEAPONNOCHARACTER;
	FCollisionQueryParams QueryParams(GetClass()->GetFName(), true, UTOwner);
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (TraceRadius <= 0.0f)
	{
		int32 SkipActorCount = 3;
		while (SkipActorCount > 0)
		{
			int32 NewSkipActorCount = 0;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndTrace, TraceChannel, QueryParams))
			{
				Hit.Location = EndTrace;
			}
			else if (Hit.Actor.IsValid() && ShouldTraceIgnore(Hit.GetActor()))
			{
				QueryParams.AddIgnoredActor(Hit.Actor.Get());
				NewSkipActorCount = SkipActorCount--;
			}
			SkipActorCount = NewSkipActorCount;
		}
	}
	else
	{
		int32 SkipActorCount = 2;
		while (SkipActorCount > 0)
		{
			int32 NewSkipActorCount = 0;
			if (!GetWorld()->SweepSingleByChannel(Hit, StartLocation, EndTrace, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), QueryParams))
			{
				Hit.Location = EndTrace;
			}
			else
			{
				Hit.Location += (EndTrace - StartLocation).GetSafeNormal() * TraceRadius; // so impact point is still on the surface of the target collision
				if (Hit.Actor.IsValid())
				{
					if (bIgnoreShockballs && Cast<AUTProj_ShockBall>(Hit.Actor.Get()))
					{
						QueryParams.AddIgnoredActor(Hit.Actor.Get());
						NewSkipActorCount = SkipActorCount--;
					}
					else if (UTOwner && Cast<AUTTeamDeco>(Hit.Actor.Get()) && !((AUTTeamDeco *)(Hit.Actor.Get()))->bBlockTeamProjectiles)
					{
						if (GS && GS->OnSameTeam(UTOwner, Hit.Actor.Get()))
						{
							QueryParams.AddIgnoredActor(Hit.Actor.Get());
							NewSkipActorCount = SkipActorCount--;
						}
					}
					else if (Cast<AUTProj_WeaponScreen>(Hit.Actor.Get()) != nullptr && GS != nullptr && GS->OnSameTeam(UTOwner, ((AUTProj_WeaponScreen*)Hit.Actor.Get())->Instigator))
					{
						QueryParams.AddIgnoredActor(Hit.Actor.Get());
						NewSkipActorCount = SkipActorCount--;
					}
				}
			}
			SkipActorCount = NewSkipActorCount;
		}
	}
	if (!(Hit.Location - StartLocation).IsNearlyZero())
	{
		AUTCharacter* ClientSideHitActor = (bTrackHitScanReplication && ReceivedHitScanHitChar && ((ReceivedHitScanIndex == FireEventIndex) || (ReceivedHitScanIndex == FireEventIndex - 1))) ? ReceivedHitScanHitChar : nullptr;
		AUTCharacter* BestTarget = NULL;
		FVector BestPoint(0.f);
		FVector BestCapsulePoint(0.f);
		float BestCollisionRadius = 0.f;
		for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
		{
			AUTCharacter* Target = Cast<AUTCharacter>(*Iterator);
			if (Target && (Target != UTOwner) && (bTeammatesBlockHitscan || !GS || GS->bTeamProjHits || !GS->OnSameTeam(UTOwner, Target)))
			{
				float ExtraHitPadding = (Target == ClientSideHitActor) ? 40.f : 0.f;
				// find appropriate rewind position, and test against trace from StartLocation to Hit.Location
				FVector TargetLocation = ((PredictionTime > 0.f) && (Role == ROLE_Authority)) ? Target->GetRewindLocation(PredictionTime) : Target->GetActorLocation();

				// now see if trace would hit the capsule
				float CollisionHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				if (Target->UTCharacterMovement && Target->UTCharacterMovement->bIsFloorSliding)
				{
					TargetLocation.Z = TargetLocation.Z - CollisionHeight + Target->SlideTargetHeight;
					CollisionHeight = Target->SlideTargetHeight;
				}
				float CollisionRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();

				bool bCheckOutsideHit = false;
				bool bHitTarget = false;
				FVector ClosestPoint(0.f);
				FVector ClosestCapsulePoint = TargetLocation;
				if (CollisionRadius >= CollisionHeight)
				{
					ClosestPoint = FMath::ClosestPointOnSegment(TargetLocation, StartLocation, Hit.Location);
					bHitTarget = ((ClosestPoint - TargetLocation).SizeSquared() < FMath::Square(CollisionHeight + TraceRadius));
					if (!bHitTarget && (ExtraHitPadding > 0.f))
					{
						bHitTarget = ((ClosestPoint - TargetLocation).SizeSquared() < FMath::Square(CollisionHeight + TraceRadius + ExtraHitPadding));
						bCheckOutsideHit = true;
					}
				}
				else
				{
					FVector CapsuleSegment = FVector(0.f, 0.f, CollisionHeight - CollisionRadius);
					FMath::SegmentDistToSegmentSafe(StartLocation, Hit.Location, TargetLocation - CapsuleSegment, TargetLocation + CapsuleSegment, ClosestPoint, ClosestCapsulePoint);
					bHitTarget = ((ClosestPoint - ClosestCapsulePoint).SizeSquared() < FMath::Square(CollisionRadius + TraceRadius));
					if (!bHitTarget && (ExtraHitPadding > 0.f))
					{
						bHitTarget = ((ClosestPoint - ClosestCapsulePoint).SizeSquared() < FMath::Square(CollisionRadius + TraceRadius + ExtraHitPadding));
						bCheckOutsideHit = true;
					}
				}
				if (bCheckOutsideHit)
				{
					FHitResult CheckHit;
					FVector PointToCheck = ClosestCapsulePoint + CollisionRadius*(ClosestPoint - ClosestCapsulePoint).GetSafeNormal();
					bHitTarget = !GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, PointToCheck, TraceChannel, QueryParams);
				}
				if (bHitTarget && (!BestTarget || ((ClosestPoint - StartLocation).SizeSquared() - ExtraHitPadding < (BestPoint - StartLocation).SizeSquared())))
				{
					BestTarget = Target;
					BestPoint = ClosestPoint;
					BestCapsulePoint = ClosestCapsulePoint;
					BestCollisionRadius = CollisionRadius;
				}
			}
		}
		if (BestTarget)
		{
			// we found a player to hit, so update hit result

			// first find proper hit location on surface of capsule
			float ClosestDistSq = (BestPoint - BestCapsulePoint).SizeSquared();
			float BackDist = FMath::Sqrt(FMath::Max(0.f, BestCollisionRadius*BestCollisionRadius - ClosestDistSq));

			Hit.Location = BestPoint + BackDist * (StartLocation - EndTrace).GetSafeNormal();
			Hit.Normal = (Hit.Location - BestCapsulePoint).GetSafeNormal();
			Hit.ImpactNormal = Hit.Normal; 
			Hit.Actor = BestTarget;
			Hit.bBlockingHit = true;
			Hit.Component = BestTarget->GetCapsuleComponent();
			Hit.ImpactPoint = BestPoint; //FIXME
			Hit.Time = (BestPoint - StartLocation).Size() / (EndTrace - StartLocation).Size();
		}
	}
}

void AUTWeapon::FireInstantHit(bool bDealDamage, FHitResult* OutHit)
{
	UE_LOG(LogUTWeapon, Verbose, TEXT("%s::FireInstantHit()"), *GetName());

	checkSlow(InstantHitInfo.IsValidIndex(CurrentFireMode));

	const FVector SpawnLocation = GetFireStartLoc();
	const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);
	const FVector FireDir = SpawnRotation.Vector();
	const FVector EndTrace = SpawnLocation + FireDir * InstantHitInfo[CurrentFireMode].TraceRange;

	FHitResult Hit;
	AUTPlayerController* UTPC = UTOwner ? Cast<AUTPlayerController>(UTOwner->Controller) : NULL;
	AUTPlayerState* PS = (UTOwner && UTOwner->Controller) ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;
	float PredictionTime = UTPC ? UTPC->GetPredictionTime() : 0.f;
	HitScanTrace(SpawnLocation, EndTrace, InstantHitInfo[CurrentFireMode].TraceHalfSize, Hit, PredictionTime);

	if (UTPC && bCheckHeadSphere && (Cast<AUTCharacter>(Hit.Actor.Get()) == NULL) && ((Spread.Num() <= GetCurrentFireMode()) || (Spread[GetCurrentFireMode()] == 0.f)) && (UTOwner->GetVelocity().IsNearlyZero() || bCheckMovingHeadSphere))
	{
		// in some cases the head sphere is partially outside the capsule
		// so do a second search just for that
		AUTCharacter* AltTarget = Cast<AUTCharacter>(UUTGameplayStatics::ChooseBestAimTarget(UTPC, SpawnLocation, FireDir, 0.7f, (Hit.Location - SpawnLocation).Size(), 150.f, AUTCharacter::StaticClass()));
		if (AltTarget != NULL && (AltTarget->GetVelocity().IsNearlyZero() || bCheckMovingHeadSphere) && AltTarget->IsHeadShot(SpawnLocation, FireDir, 1.1f, UTOwner, PredictionTime))
		{
			Hit = FHitResult(AltTarget, AltTarget->GetCapsuleComponent(), SpawnLocation + FireDir * ((AltTarget->GetHeadLocation() - SpawnLocation).Size() - AltTarget->GetCapsuleComponent()->GetUnscaledCapsuleRadius()), -FireDir);
		}
	}

	if (Role == ROLE_Authority)
	{
		if (PS && (ShotsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(ShotsStatsName, 1);
		}
		UTOwner->SetFlashLocation(Hit.Location, CurrentFireMode);
		// warn bot target, if any
		if (UTPC != NULL)
		{
			APawn* PawnTarget = Cast<APawn>(Hit.Actor.Get());
			if (PawnTarget != NULL)
			{
				UTPC->LastShotTargetGuess = PawnTarget;
			}
			// if not dealing damage, it's the caller's responsibility to send warnings if desired
			if (bDealDamage && UTPC->LastShotTargetGuess != NULL)
			{
				AUTBot* EnemyBot = Cast<AUTBot>(UTPC->LastShotTargetGuess->Controller);
				if (EnemyBot != NULL)
				{
					EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
				}
			}
		}
		else if (bDealDamage)
		{
			AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
			if (B != NULL)
			{
				APawn* PawnTarget = Cast<APawn>(Hit.Actor.Get());
				if (PawnTarget == NULL)
				{
					PawnTarget = Cast<APawn>(B->GetTarget());
				}
				if (PawnTarget != NULL)
				{
					AUTBot* EnemyBot = Cast<AUTBot>(PawnTarget->Controller);
					if (EnemyBot != NULL)
					{
						EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
					}
				}
			}
		}
	}
	else if (PredictionTime > 0.f)
	{
		PlayPredictedImpactEffects(Hit.Location);
	}
	if (Hit.Actor != NULL && Hit.Actor->bCanBeDamaged && bDealDamage)
	{
		if ((Role == ROLE_Authority) && PS && (HitsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(HitsStatsName, 1);
		}
		OnHitScanDamage(Hit, FireDir);
		Hit.Actor->TakeDamage(InstantHitInfo[CurrentFireMode].Damage, FUTPointDamageEvent(InstantHitInfo[CurrentFireMode].Damage, Hit, FireDir, InstantHitInfo[CurrentFireMode].DamageType, FireDir * GetImpartedMomentumMag(Hit.Actor.Get())), UTOwner->Controller, this);
	}
	if (OutHit != NULL)
	{
		*OutHit = Hit;
	}
}

void AUTWeapon::PlayPredictedImpactEffects(FVector ImpactLoc)
{
	if (!UTOwner)
	{
		return;
	}
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(UTOwner->Controller);
	float SleepTime = UTPC ? UTPC->GetProjectileSleepTime() : 0.f;
	if (SleepTime > 0.f)
	{
		if (GetWorldTimerManager().IsTimerActive(PlayDelayedImpactEffectsHandle))
		{
			// play the delayed effect now, since we are about to replace it
			PlayDelayedImpactEffects();
		}
		FVector SpawnLocation;
		FRotator SpawnRotation;
		GetImpactSpawnPosition(ImpactLoc, SpawnLocation, SpawnRotation);
		DelayedHitScan.ImpactLocation = ImpactLoc;
		DelayedHitScan.FireMode = CurrentFireMode;
		DelayedHitScan.SpawnLocation = SpawnLocation;
		DelayedHitScan.SpawnRotation = SpawnRotation;
		GetWorldTimerManager().SetTimer(PlayDelayedImpactEffectsHandle, this, &AUTWeapon::PlayDelayedImpactEffects, SleepTime, false);
	}
	else
	{
		FVector SpawnLocation;
		FRotator SpawnRotation;
		GetImpactSpawnPosition(ImpactLoc, SpawnLocation, SpawnRotation);
		UTOwner->SetFlashLocation(ImpactLoc, CurrentFireMode);
	}
}

void AUTWeapon::PlayDelayedImpactEffects()
{
	if (UTOwner)
	{
		UTOwner->SetFlashLocation(DelayedHitScan.ImpactLocation, DelayedHitScan.FireMode);
	}
}

float AUTWeapon::GetImpartedMomentumMag(AActor* HitActor)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	AUTCharacter* HitChar = Cast<AUTCharacter>(HitActor);
	if (HitChar && HitChar->IsDead())
	{
		return 20000.f;
	}
	if ((FriendlyMomentumScaling != 1.f) && GS != NULL && HitChar && GS->OnSameTeam(HitActor, UTOwner))
	{
		return  InstantHitInfo[CurrentFireMode].Momentum *FriendlyMomentumScaling;
	}

	return InstantHitInfo[CurrentFireMode].Momentum;
}

void AUTWeapon::K2_FireInstantHit(bool bDealDamage, FHitResult& OutHit)
{
	if (bTrackHitScanReplication && (UTOwner == nullptr || !UTOwner->IsLocallyControlled() || Cast<APlayerController>(UTOwner->GetController()) != nullptr))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s::FireInstantHit(): from script!"), *GetName()), ELogVerbosity::Warning);
	}
	if (!InstantHitInfo.IsValidIndex(CurrentFireMode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s::FireInstantHit(): Fire mode %i doesn't have instant hit info"), *GetName(), int32(CurrentFireMode)), ELogVerbosity::Warning);
	}
	else if (GetUTOwner() != NULL)
	{
		FireInstantHit(bDealDamage, &OutHit);
	}
	else
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s::FireInstantHit(): Weapon is not owned (owner died while script was running?)"), *GetName()), ELogVerbosity::Warning);
	}
}

AUTProjectile* AUTWeapon::FireProjectile()
{
	UE_LOG(LogUTWeapon, Verbose, TEXT("%s::FireProjectile()"), *GetName());

	if (GetUTOwner() == NULL)
	{
		UE_LOG(LogUTWeapon, Warning, TEXT("%s::FireProjectile(): Weapon is not owned (owner died during firing sequence)"), *GetName());
		return NULL;
	}

	checkSlow(ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL);
	if (Role == ROLE_Authority)
	{
		UTOwner->IncrementFlashCount(CurrentFireMode);
		AUTPlayerState* PS = UTOwner->Controller ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;
		if (PS && (ShotsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(ShotsStatsName, 1);
		}
	}
	// spawn the projectile at the muzzle
	const FVector SpawnLocation = GetFireStartLoc();
	const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);
	return SpawnNetPredictedProjectile(ProjClass[CurrentFireMode], SpawnLocation, SpawnRotation);
}

AUTProjectile* AUTWeapon::SpawnNetPredictedProjectile(TSubclassOf<AUTProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	//DrawDebugSphere(GetWorld(), SpawnLocation, 10, 10, FColor::Green, true);
	AUTPlayerController* OwningPlayer = UTOwner ? Cast<AUTPlayerController>(UTOwner->GetController()) : NULL;
	float CatchupTickDelta = 
		((GetNetMode() != NM_Standalone) && OwningPlayer)
		? OwningPlayer->GetPredictionTime()
		: 0.f;
/*
	if (OwningPlayer)
	{
		float CurrentMoveTime = (UTOwner && UTOwner->UTCharacterMovement) ? UTOwner->UTCharacterMovement->GetCurrentSynchTime() : GetWorld()->GetTimeSeconds();
		if (UTOwner->Role < ROLE_Authority)
		{
			UE_LOG(UT, Warning, TEXT("CLIENT SpawnNetPredictedProjectile at %f yaw %f "), CurrentMoveTime, SpawnRotation.Yaw);
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("SERVER SpawnNetPredictedProjectile at %f yaw %f TIME %f"), CurrentMoveTime, SpawnRotation.Yaw, GetWorld()->GetTimeSeconds());
		}
	}
*/
	if ((CatchupTickDelta > 0.f) && (Role != ROLE_Authority))
	{
		float SleepTime = OwningPlayer->GetProjectileSleepTime();
		if (SleepTime > 0.f)
		{
			// lag is so high need to delay spawn
			if (!GetWorldTimerManager().IsTimerActive(SpawnDelayedFakeProjHandle))
			{
				DelayedProjectile.ProjectileClass = ProjectileClass;
				DelayedProjectile.SpawnLocation = SpawnLocation;
				DelayedProjectile.SpawnRotation = SpawnRotation;
				GetWorldTimerManager().SetTimer(SpawnDelayedFakeProjHandle, this, &AUTWeapon::SpawnDelayedFakeProjectile, SleepTime, false);
			}
			return NULL;
		}
	}
	FActorSpawnParameters Params;
	Params.Instigator = UTOwner;
	Params.Owner = UTOwner;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUTProjectile* NewProjectile = 
		((Role == ROLE_Authority) || (CatchupTickDelta > 0.f))
		? GetWorld()->SpawnActor<AUTProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params)
		: NULL;
	if (NewProjectile)
	{
		if (NewProjectile->OffsetVisualComponent)
		{
			switch (GetWeaponHand())
			{
				case EWeaponHand::HAND_Center:
					NewProjectile->InitialVisualOffset = NewProjectile->InitialVisualOffset + LowMeshOffset;
					NewProjectile->OffsetVisualComponent->RelativeLocation = NewProjectile->InitialVisualOffset;
					break;
				case EWeaponHand::HAND_Hidden:
				{
					NewProjectile->InitialVisualOffset = NewProjectile->InitialVisualOffset + VeryLowMeshOffset;
					NewProjectile->OffsetVisualComponent->RelativeLocation = NewProjectile->InitialVisualOffset;
					break;
				}
			}
		}
		if (UTOwner)
		{
			UTOwner->LastFiredProjectile = NewProjectile;
			NewProjectile->ShooterLocation = UTOwner->GetActorLocation();
			NewProjectile->ShooterRotation = UTOwner->GetActorRotation();
		}
		if (Role == ROLE_Authority)
		{
			NewProjectile->HitsStatsName = HitsStatsName;
			if ((CatchupTickDelta > 0.f) && NewProjectile->ProjectileMovement)
			{
				// server ticks projectile to match with when client actually fired
				// TODO: account for CustomTimeDilation?
				if (NewProjectile->PrimaryActorTick.IsTickFunctionEnabled())
				{
					NewProjectile->TickActor(CatchupTickDelta * NewProjectile->CustomTimeDilation, LEVELTICK_All, NewProjectile->PrimaryActorTick);
				}
				NewProjectile->ProjectileMovement->TickComponent(CatchupTickDelta * NewProjectile->CustomTimeDilation, LEVELTICK_All, NULL);
				NewProjectile->SetForwardTicked(true);
				if (NewProjectile->GetLifeSpan() > 0.f)
				{
					NewProjectile->SetLifeSpan(0.1f + FMath::Max(0.01f, NewProjectile->GetLifeSpan() - CatchupTickDelta));
				}
			}
			else
			{
				NewProjectile->SetForwardTicked(false);
			}
		}
		else
		{
			NewProjectile->InitFakeProjectile(OwningPlayer);
			NewProjectile->SetLifeSpan(FMath::Min(NewProjectile->GetLifeSpan(), 2.f * FMath::Max(0.f, CatchupTickDelta)));
		}
	}
	return NewProjectile;
}

void AUTWeapon::SpawnDelayedFakeProjectile()
{
	AUTPlayerController* OwningPlayer = UTOwner ? Cast<AUTPlayerController>(UTOwner->GetController()) : NULL;
	if (OwningPlayer)
	{
		FActorSpawnParameters Params;
		Params.Instigator = UTOwner;
		Params.Owner = UTOwner;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AUTProjectile* NewProjectile = GetWorld()->SpawnActor<AUTProjectile>(DelayedProjectile.ProjectileClass, DelayedProjectile.SpawnLocation, DelayedProjectile.SpawnRotation, Params);
		if (NewProjectile)
		{
			NewProjectile->InitFakeProjectile(OwningPlayer);
			NewProjectile->SetLifeSpan(FMath::Min(NewProjectile->GetLifeSpan(), 0.002f * FMath::Max(0.f, OwningPlayer->MaxPredictionPing + OwningPlayer->PredictionFudgeFactor)));
			if (NewProjectile->OffsetVisualComponent)
			{
				switch (GetWeaponHand())
				{
				case EWeaponHand::HAND_Center:
					NewProjectile->InitialVisualOffset = NewProjectile->InitialVisualOffset + LowMeshOffset;
					NewProjectile->OffsetVisualComponent->RelativeLocation = NewProjectile->InitialVisualOffset;
					break;
				case EWeaponHand::HAND_Hidden:
				{
					NewProjectile->InitialVisualOffset = NewProjectile->InitialVisualOffset + VeryLowMeshOffset;
					NewProjectile->OffsetVisualComponent->RelativeLocation = NewProjectile->InitialVisualOffset;
					break;
				}
				}
			}
		}
	}
}

void AUTWeapon::FireCone()
{
	UE_LOG(LogUTWeapon, Verbose, TEXT("%s::FireCone()"), *GetName());

	checkSlow(InstantHitInfo.IsValidIndex(CurrentFireMode));
	checkSlow(InstantHitInfo[CurrentFireMode].ConeDotAngle > 0.0f);

	const FVector SpawnLocation = GetFireStartLoc();
	const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);
	const FVector FireDir = SpawnRotation.Vector();
	const FVector EndTrace = SpawnLocation + FireDir * InstantHitInfo[CurrentFireMode].TraceRange;

	AUTPlayerController* UTPC = UTOwner ? Cast<AUTPlayerController>(UTOwner->Controller) : NULL;
	AUTPlayerState* PS = (UTOwner && UTOwner->Controller) ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	float PredictionTime = UTPC ? UTPC->GetPredictionTime() : 0.f;
	
	FCollisionResponseParams TraceResponseParams = WorldResponseParams;
	TraceResponseParams.CollisionResponse.SetResponse(COLLISION_PROJECTILE_SHOOTABLE, ECR_Block);
	TArray<FOverlapResult> OverlapHits;
	TArray<FHitResult> RealHits;
	GetWorld()->OverlapMultiByChannel(OverlapHits, SpawnLocation, FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(InstantHitInfo[CurrentFireMode].TraceRange));
	for (const FOverlapResult& Overlap : OverlapHits)
	{
		if (Overlap.GetActor() != nullptr)
		{
			FVector ObjectLoc = Overlap.GetComponent()->Bounds.Origin;
			if (((ObjectLoc - SpawnLocation).GetSafeNormal() | FireDir) >= InstantHitInfo[CurrentFireMode].ConeDotAngle)
			{
				bool bClear;
				int32 Retries = 2;
				FCollisionQueryParams QueryParams(NAME_None, true, UTOwner);
				do 
				{
					FHitResult Hit;
					if (InstantHitInfo[CurrentFireMode].TraceHalfSize <= 0.0f)
					{
						bClear = !GetWorld()->LineTraceSingleByChannel(Hit, SpawnLocation, ObjectLoc, COLLISION_TRACE_WEAPONNOCHARACTER, QueryParams, TraceResponseParams);
					}
					else
					{
						bClear = !GetWorld()->SweepSingleByChannel(Hit, SpawnLocation, ObjectLoc, FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(InstantHitInfo[CurrentFireMode].TraceHalfSize), QueryParams, TraceResponseParams);
					}
					if (bClear || Hit.GetActor() == nullptr || !ShouldTraceIgnore(Hit.GetActor()))
					{
						break;
					}
					else
					{
						QueryParams.AddIgnoredActor(Hit.GetActor());
					}
				} while (Retries-- > 0);
				if (bClear)
				{
					// trace only against target to get good hit info
					FHitResult Hit;
					if (!Overlap.GetComponent()->LineTraceComponent(Hit, SpawnLocation, ObjectLoc, FCollisionQueryParams(NAME_None, true, UTOwner)))
					{
						Hit = FHitResult(Overlap.GetActor(), Overlap.GetComponent(), ObjectLoc, -FireDir);
					}
					RealHits.Add(Hit);
				}
			}
		}
	}
	// do characters separately to handle forward prediction
	for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
	{
		AUTCharacter* Target = Cast<AUTCharacter>(*Iterator);
		if (Target && (Target != UTOwner) && (bTeammatesBlockHitscan || !GS || !GS->OnSameTeam(UTOwner, Target)))
		{
			// find appropriate rewind position, and test against trace from StartLocation to Hit.Location
			FVector TargetLocation = ((PredictionTime > 0.f) && (Role == ROLE_Authority)) ? Target->GetRewindLocation(PredictionTime) : Target->GetActorLocation();
			const FVector Diff = TargetLocation - SpawnLocation;
			if (Diff.Size() <= InstantHitInfo[CurrentFireMode].TraceRange && (Diff.GetSafeNormal() | FireDir) >= InstantHitInfo[CurrentFireMode].ConeDotAngle)
			{
				// now see if trace would hit the capsule
				float CollisionHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				if (Target->UTCharacterMovement && Target->UTCharacterMovement->bIsFloorSliding)
				{
					TargetLocation.Z = TargetLocation.Z - CollisionHeight + Target->SlideTargetHeight;
					CollisionHeight = Target->SlideTargetHeight;
				}
				float CollisionRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();

				bool bHitTarget = false;
				FVector ClosestPoint(0.f);
				FVector ClosestCapsulePoint = TargetLocation;
				if (CollisionRadius >= CollisionHeight)
				{
					ClosestPoint = TargetLocation;
				}
				else
				{
					FVector CapsuleSegment = FVector(0.f, 0.f, CollisionHeight - CollisionRadius);
					FMath::SegmentDistToSegmentSafe(SpawnLocation, TargetLocation, TargetLocation - CapsuleSegment, TargetLocation + CapsuleSegment, ClosestPoint, ClosestCapsulePoint);
				}
				// first find proper hit location on surface of capsule
				float ClosestDistSq = (ClosestPoint - ClosestCapsulePoint).SizeSquared();
				float BackDist = FMath::Sqrt(FMath::Max(0.f, CollisionRadius*CollisionRadius - ClosestDistSq));
				const FVector HitLocation = ClosestPoint + BackDist * (SpawnLocation - EndTrace).GetSafeNormal();

				bool bClear;
				int32 Retries = 2;
				FCollisionQueryParams QueryParams(NAME_None, true, UTOwner);
				do
				{
					FHitResult Hit;
					if (InstantHitInfo[CurrentFireMode].TraceHalfSize <= 0.0f)
					{
						bClear = !GetWorld()->LineTraceTestByChannel(SpawnLocation, HitLocation, COLLISION_TRACE_WEAPONNOCHARACTER, QueryParams, TraceResponseParams);
					}
					else
					{
						bClear = !GetWorld()->SweepTestByChannel(SpawnLocation, HitLocation, FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(InstantHitInfo[CurrentFireMode].TraceHalfSize), QueryParams, TraceResponseParams);
					}
					if (bClear || Hit.GetActor() == nullptr || !ShouldTraceIgnore(Hit.GetActor()))
					{
						break;
					}
					else
					{
						QueryParams.AddIgnoredActor(Hit.GetActor());
					}
				} while (Retries-- > 0);
				if (bClear)
				{
					FHitResult* NewHit = new(RealHits) FHitResult;
					NewHit->Location = HitLocation;
					NewHit->Normal = (EndTrace - ClosestCapsulePoint).GetSafeNormal();
					NewHit->ImpactNormal = NewHit->Normal;
					NewHit->Actor = Target;
					NewHit->bBlockingHit = true;
					NewHit->Component = Target->GetCapsuleComponent();
					NewHit->ImpactPoint = ClosestPoint; //FIXME
					NewHit->Time = (ClosestPoint - SpawnLocation).Size() / (EndTrace - SpawnLocation).Size();
				}
			}
		}
	}
	RealHits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });

	if (Role == ROLE_Authority)
	{
		if (PS && (ShotsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(ShotsStatsName, 1);
		}
		UTOwner->IncrementFlashCount(CurrentFireMode);
		// warn bot target, if any
		if (UTPC != NULL)
		{
			APawn* PawnTarget = RealHits.Num() > 0 ? Cast<APawn>(RealHits[0].Actor.Get()) : nullptr;
			if (PawnTarget != NULL)
			{
				UTPC->LastShotTargetGuess = PawnTarget;
			}
			if (UTPC->LastShotTargetGuess != NULL)
			{
				AUTBot* EnemyBot = Cast<AUTBot>(UTPC->LastShotTargetGuess->Controller);
				if (EnemyBot != NULL)
				{
					EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
				}
			}
		}
		else
		{
			AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
			if (B != NULL)
			{
				APawn* PawnTarget = RealHits.Num() > 0 ? Cast<APawn>(RealHits[0].Actor.Get()) : nullptr;
				if (PawnTarget == NULL)
				{
					PawnTarget = Cast<APawn>(B->GetTarget());
				}
				if (PawnTarget != NULL)
				{
					AUTBot* EnemyBot = Cast<AUTBot>(PawnTarget->Controller);
					if (EnemyBot != NULL)
					{
						EnemyBot->ReceiveInstantWarning(UTOwner, FireDir);
					}
				}
			}
		}
	}
	for (const FHitResult& Hit : RealHits)
	{
		if (UTOwner && Hit.Actor != NULL && Hit.Actor->bCanBeDamaged)
		{
			if ((Role == ROLE_Authority) && PS && (HitsStatsName != NAME_None))
			{
				PS->ModifyStatsValue(HitsStatsName, 1);
			}
			Hit.Actor->TakeDamage(InstantHitInfo[CurrentFireMode].Damage, FUTPointDamageEvent(InstantHitInfo[CurrentFireMode].Damage, Hit, FireDir, InstantHitInfo[CurrentFireMode].DamageType, FireDir * GetImpartedMomentumMag(Hit.Actor.Get())), UTOwner->Controller, this);
		}
	}
}

float AUTWeapon::GetRefireTime(uint8 FireModeNum)
{
	if (FireInterval.IsValidIndex(FireModeNum))
	{
		float Result = FireInterval[FireModeNum];
		if (UTOwner != NULL)
		{
			Result /= UTOwner->GetFireRateMultiplier();
		}
		return FMath::Max<float>(0.01f, Result);
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Invalid firing mode %i in %s::GetRefireTime()"), int32(FireModeNum), *GetName());
		return 0.1f;
	}
}

void AUTWeapon::UpdateTiming()
{
	CurrentState->UpdateTiming();
}

bool AUTWeapon::StackLockerPickup(AUTInventory* ContainedInv)
{
	// end with currentammo+1 or dropped weapon's ammo, whichever is greater
	int32 DefaultAmmo = GetClass()->GetDefaultObject<AUTWeapon>()->Ammo;
	int32 Amount = Cast<AUTWeapon>(ContainedInv) ? Cast<AUTWeapon>(ContainedInv)->Ammo : DefaultAmmo;
	Amount = FMath::Min(Amount, DefaultAmmo - Ammo);
	Amount = FMath::Max(Amount, 0);
	AddAmmo(Amount);
	return true;
}

bool AUTWeapon::StackPickup_Implementation(AUTInventory* ContainedInv)
{
	int32 Amount = GetClass()->GetDefaultObject<AUTWeapon>()->Ammo;
	if (ContainedInv != nullptr)
	{
		if (bFromLocker && ContainedInv->bFromLocker)
		{
			return StackLockerPickup(ContainedInv);
		}
		Amount = Cast<AUTWeapon>(ContainedInv)->Ammo;
	}
	AddAmmo(Amount);
	return true;
}

bool AUTWeapon::ShouldLagRot()
{
	return bProceduralLagRotation;
}

float AUTWeapon::LagWeaponRotation(float NewValue, float LastValue, float DeltaTime, float MaxDiff, int32 Index)
{
	// check if NewValue is clockwise from LastValue
	NewValue = FMath::UnwindDegrees(NewValue);
	LastValue = FMath::UnwindDegrees(LastValue);

	float LeadMag = 0.f;
	float RotDiff = NewValue - LastValue;
	if ((RotDiff == 0.f) || (OldRotDiff[Index] == 0.f))
	{
		LeadMag = ShouldLagRot() ? OldLeadMag[Index] : 0.f;
		if ((RotDiff == 0.f) && (OldRotDiff[Index] == 0.f))
		{
			OldMaxDiff[Index] = 0.f;
		}
	}
	else if ((RotDiff > 0.f) == (OldRotDiff[Index] > 0.f))
	{
		if (ShouldLagRot())
		{
			MaxDiff = FMath::Min(1.f, FMath::Abs(RotDiff) / (66.f * DeltaTime)) * MaxDiff;
			if (OldMaxDiff[Index] != 0.f)
			{
				MaxDiff = FMath::Max(OldMaxDiff[Index], MaxDiff);
			}

			OldMaxDiff[Index] = MaxDiff;
			LeadMag = (NewValue > LastValue) ? -1.f * MaxDiff : MaxDiff;
		}
		LeadMag = (DeltaTime < 1.f / RotChgSpeed)
					? LeadMag = (1.f- RotChgSpeed*DeltaTime)*OldLeadMag[Index] + RotChgSpeed*DeltaTime*LeadMag
					: LeadMag = 0.f;
	}
	else
	{
		OldMaxDiff[Index] = 0.f;
		if (DeltaTime < 1.f/ReturnChgSpeed)
		{
			LeadMag = (1.f - ReturnChgSpeed*DeltaTime)*OldLeadMag[Index] + ReturnChgSpeed*DeltaTime*LeadMag;
		}
	}
	OldLeadMag[Index] = LeadMag;
	OldRotDiff[Index] = RotDiff;

	return LeadMag;
}

void AUTWeapon::Tick(float DeltaTime)
{
	// try to gracefully detect being active with no owner, which should never happen because we should always end up going through Removed() and going to the inactive state
	if (CurrentState != InactiveState && (UTOwner == NULL || UTOwner->IsPendingKillPending()) && CurrentState != NULL)
	{
		UE_LOG(UT, Warning, TEXT("%s lost Owner while active (state %s"), *GetName(), *GetNameSafe(CurrentState));
		GotoState(InactiveState);
	}

	Super::Tick(DeltaTime);

	// note that this relies on us making BeginPlay() always called before first tick; see UUTGameEngine::LoadMap()
	if (CurrentState != InactiveState)
	{
		CurrentState->Tick(DeltaTime);
	}

	TickZoom(DeltaTime);
}

void AUTWeapon::UpdateViewBob(float DeltaTime)
{
	AUTPlayerController* MyPC = UTOwner ? UTOwner->GetLocalViewer() : NULL;
	if (MyPC != NULL && Mesh != NULL && UTOwner->GetWeapon() == this && ShouldPlay1PVisuals())
	{
		// if weapon is up in first person, view bob with movement
		USceneComponent* BobbedMesh = (HandsAttachSocket != NAME_None) ? (USceneComponent*)UTOwner->FirstPersonMeshBoundSphere : (USceneComponent*)Mesh;
		if (HandsAttachSocket != NAME_None)
		{
			FirstPMeshOffset = UTOwner->GetClass()->GetDefaultObject<AUTCharacter>()->FirstPersonMeshBoundSphere->GetRelativeTransform().GetLocation();
			FirstPMeshRotation = BobbedMesh->GetRelativeTransform().Rotator();
		}
		else if (FirstPMeshOffset.IsZero())
		{
			FirstPMeshOffset = BobbedMesh->GetRelativeTransform().GetLocation();
			FirstPMeshRotation = BobbedMesh->GetRelativeTransform().Rotator();
		}
		FVector ScaledMeshOffset = FirstPMeshOffset;
		const float FOVScaling = (MyPC != NULL) ? ((MyPC->PlayerCameraManager->GetFOVAngle() - 100.f) * 0.05f) : 1.0f;
		if (FOVScaling > 0.f)
		{
			ScaledMeshOffset.X *= (1.f + (FOVOffset.X - 1.f) * FOVScaling);
			ScaledMeshOffset.Y *= (1.f + (FOVOffset.Y - 1.f) * FOVScaling);
			ScaledMeshOffset.Z *= (1.f + (FOVOffset.Z - 1.f) * FOVScaling);
		}

		if (GetWeaponHand() == EWeaponHand::HAND_Hidden)
		{
			ScaledMeshOffset += VeryLowMeshOffset;
		}
		else if(GetWeaponHand() == EWeaponHand::HAND_Center)
		{
			ScaledMeshOffset += LowMeshOffset;
		}

		BobbedMesh->SetRelativeLocation(ScaledMeshOffset);
		FVector BobOffset = UTOwner->GetWeaponBobOffset(DeltaTime, this);
		BobbedMesh->SetWorldLocation(BobbedMesh->GetComponentLocation() + BobOffset);

		FRotator NewRotation = UTOwner ? UTOwner->GetControlRotation() : FRotator(0.f, 0.f, 0.f);
		FRotator FinalRotation = NewRotation;

		// Add some rotation leading
		if (UTOwner && UTOwner->Controller)
		{
			FinalRotation.Yaw = LagWeaponRotation(NewRotation.Yaw, LastRotation.Yaw, DeltaTime, MaxYawLag, 0);
			FinalRotation.Pitch = LagWeaponRotation(NewRotation.Pitch, LastRotation.Pitch, DeltaTime, MaxPitchLag, 1);
			FinalRotation.Roll = NewRotation.Roll;
		}
		LastRotation = NewRotation;
		BobbedMesh->SetRelativeRotation(FinalRotation + FirstPMeshRotation);
	}
}

void AUTWeapon::Destroyed()
{
	Super::Destroyed();

	// this makes sure timers, etc go away
	GotoState(InactiveState);
}

bool AUTWeapon::CanFireAgain()
{
	return (GetUTOwner() && (GetUTOwner()->GetPendingWeapon() == NULL) && HasAmmo(GetCurrentFireMode()) && (!bRootWhileFiring || UTOwner->GetCharacterMovement() == nullptr || UTOwner->GetCharacterMovement()->MovementMode == MOVE_Falling));
}

bool AUTWeapon::HandleContinuedFiring()
{
	if (!CanFireAgain() || !GetUTOwner()->IsPendingFire(GetCurrentFireMode()))
	{
		GotoActiveState();
		return false;
	}
	LastContinuedFiring = GetWorld()->GetTimeSeconds();

	OnContinuedFiring();
	return true;
}

// hooks meant for subclasses/blueprints, empty by default
void AUTWeapon::OnStartedFiring_Implementation()
{}
void AUTWeapon::OnStoppedFiring_Implementation()
{}
void AUTWeapon::OnContinuedFiring_Implementation()
{}
void AUTWeapon::OnMultiPress_Implementation(uint8 OtherFireMode)
{}

void AUTWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTWeapon, Ammo, COND_None);
	DOREPLIFETIME_CONDITION(AUTWeapon, MaxAmmo, COND_OwnerOnly);
	DOREPLIFETIME(AUTWeapon, AttachmentType);

	DOREPLIFETIME_CONDITION(AUTWeapon, ZoomCount, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AUTWeapon, ZoomState, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AUTWeapon, CurrentZoomMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AUTWeapon, ZoomTime, COND_InitialOnly);
}

FLinearColor AUTWeapon::GetCrosshairColor(UUTHUDWidget* WeaponHudWidget) const
{
	FLinearColor CrosshairColor = FLinearColor::White;
	return WeaponHudWidget->UTHUDOwner->GetCrosshairColor(CrosshairColor);
}

bool AUTWeapon::ShouldDrawFFIndicator(APlayerController* Viewer, AUTPlayerState *& HitPlayerState) const
{
	bool bDrawFriendlyIndicator = false;
	HitPlayerState = nullptr;
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS != NULL)
	{
		FVector CameraLoc;
		FRotator CameraRot;
		Viewer->GetPlayerViewPoint(CameraLoc, CameraRot);
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, CameraLoc + CameraRot.Vector() * 50000.0f, COLLISION_TRACE_WEAPON, FCollisionQueryParams(FName(TEXT("CrosshairFriendIndicator")), true, UTOwner));
		if (Hit.Actor != NULL)
		{
			AUTCharacter* Char = Cast<AUTCharacter>(Hit.Actor.Get());
			if (Char != NULL)
			{
				bDrawFriendlyIndicator = GS->OnSameTeam(Hit.Actor.Get(), UTOwner);

				int32 MyVisibilityMask = 0;
				if (UTOwner)
				{
					MyVisibilityMask = UTOwner->VisibilityMask;
				}
				bool bCanSee = true;
				if (Char->VisibilityMask > 0 && (MyVisibilityMask & Char->VisibilityMask) == 0)
				{
					bCanSee = false;
				}

				if (Char != NULL && !Char->IsFeigningDeath() && bCanSee)
				{
					if (Char->PlayerState != nullptr)
					{
						HitPlayerState = Cast<AUTPlayerState>(Char->PlayerState);
					}
					else if (Char->DrivenVehicle != nullptr && Char->DrivenVehicle->PlayerState != nullptr)
					{
						HitPlayerState = Cast<AUTPlayerState>(Char->DrivenVehicle->PlayerState);
					}
				}
			}
		}
	}
	return bDrawFriendlyIndicator;
}

float AUTWeapon::GetCrosshairScale(AUTHUD* HUD)
{
	return HUD->GetCrosshairScale();
}

void AUTWeapon::DrawWeaponCrosshair_Implementation(UUTHUDWidget* WeaponHudWidget, float RenderDelta)
{
	bool bDrawCrosshair = true;
	for (int32 i = 0; i < FiringState.Num(); i++)
	{
		bDrawCrosshair = FiringState[i]->DrawHUD(WeaponHudWidget) && bDrawCrosshair;
	}

	if (bDrawCrosshair)
	{
		ActiveCrosshair = WeaponHudWidget->UTHUDOwner->GetCrosshairForWeapon(WeaponCustomizationTag, ActiveCrosshairCustomizationInfo);			
		if (ActiveCrosshair != nullptr)
		{
			ActiveCrosshair->NativeDrawCrosshair(WeaponHudWidget->UTHUDOwner, WeaponHudWidget->GetCanvas(), this, RenderDelta, ActiveCrosshairCustomizationInfo);
		}
		else
		{
			// fall back crosshair
			UTexture2D* CrosshairTexture = WeaponHudWidget->UTHUDOwner->DefaultCrosshairTex;
			if (CrosshairTexture != NULL)
			{
				float W = CrosshairTexture->GetSurfaceWidth();
				float H = CrosshairTexture->GetSurfaceHeight();
				float CrosshairScale = WeaponHudWidget->UTHUDOwner->GetCrosshairScale();

				WeaponHudWidget->DrawTexture(CrosshairTexture, 0, 0, W * CrosshairScale, H * CrosshairScale, 0.0, 0.0, 16, 16, 1.0, WeaponHudWidget->UTHUDOwner->GetCrosshairColor(FLinearColor::White), FVector2D(0.5f, 0.5f));
			}
		}

		// friendly and enemy indicators
		AUTPlayerState* PS;
		if (ShouldDrawFFIndicator(WeaponHudWidget->UTHUDOwner->PlayerOwner, PS))
		{
			UTexture2D* CrosshairTexture = WeaponHudWidget->UTHUDOwner->DefaultCrosshairTex;
			if (CrosshairTexture != NULL)
			{
				float W = CrosshairTexture->GetSurfaceWidth();
				float H = CrosshairTexture->GetSurfaceHeight();
				float CrosshairScale = WeaponHudWidget->UTHUDOwner->GetCrosshairScale();
				WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 0, 0, W * CrosshairScale, H * CrosshairScale, 407, 940, 72, 72, 1.0, FLinearColor::Green, FVector2D(0.5f, 0.5f));
			}
		}
		else
		{
			UpdateCrosshairTarget(PS, WeaponHudWidget, RenderDelta);
		}
	}
}


void AUTWeapon::UpdateCrosshairTarget(AUTPlayerState* NewCrosshairTarget, UUTHUDWidget* WeaponHudWidget, float RenderDelta)
{
	if (NewCrosshairTarget != NULL)
	{
		TargetPlayerState = NewCrosshairTarget;
		TargetLastSeenTime = GetWorld()->GetTimeSeconds();
	}

	if (TargetPlayerState != NULL)
	{
		float TimeSinceSeen = GetWorld()->GetTimeSeconds() - TargetLastSeenTime;
		static float MAXNAMEDRAWTIME = 0.3f;
		if (TimeSinceSeen < MAXNAMEDRAWTIME)
		{
			static float MAXNAMEFULLALPHA = 0.22f;
			float Alpha = (TimeSinceSeen < MAXNAMEFULLALPHA) ? 1.f : (1.f - ((TimeSinceSeen - MAXNAMEFULLALPHA) / (MAXNAMEDRAWTIME - MAXNAMEFULLALPHA)));

			float H = WeaponHudWidget->UTHUDOwner->DefaultCrosshairTex->GetSurfaceHeight();
			FText PlayerName = FText::FromString(TargetPlayerState->PlayerName);
			WeaponHudWidget->DrawText(PlayerName, 0.f, H * 2.f, WeaponHudWidget->UTHUDOwner->SmallFont, false, FVector2D(0.f, 0.f), FLinearColor::Black, true, FLinearColor::Black, 1.0f, Alpha, FLinearColor::Red, FLinearColor(0.0f,0.0f,0.0f,0.0f), ETextHorzPos::Center);
		}
		else
		{
			TargetPlayerState = NULL;
		}
	}
}

TArray<UMeshComponent*> AUTWeapon::Get1PMeshes_Implementation() const
{
	TArray<UMeshComponent*> Result;
	Result.Add(Mesh);
	Result.Add(OverlayMesh);
	Result.Add(CustomDepthMesh);
	return Result;
}

void AUTWeapon::UpdateOverlaysShared(AActor* WeaponActor, AUTCharacter* InOwner, USkeletalMeshComponent* InMesh, const TArray<FParticleSysParam>& InOverlayEffectParams, USkeletalMeshComponent*& InOverlayMesh) const
{
	AUTGameState* GS = WeaponActor ? WeaponActor->GetWorld()->GetGameState<AUTGameState>() : NULL;
	if (GS != NULL && InOwner != NULL && InMesh != NULL)
	{
		FOverlayEffect TopOverlay = GS->GetFirstOverlay(InOwner->GetWeaponOverlayFlags(), Cast<AUTWeapon>(WeaponActor) != NULL);
		if (TopOverlay.IsValid())
		{
			if (InOverlayMesh == NULL)
			{
				InOverlayMesh = DuplicateObject<USkeletalMeshComponent>(InMesh, WeaponActor);
				InOverlayMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				InOverlayMesh->SetMasterPoseComponent(InMesh);
			}
			if (!InOverlayMesh->IsRegistered())
			{
				InOverlayMesh->RegisterComponent();
				InOverlayMesh->AttachToComponent(InMesh, FAttachmentTransformRules::SnapToTargetIncludingScale);
				InOverlayMesh->SetWorldScale3D(InMesh->GetComponentScale());
			}
			// if it's just particles and no material, hide the overlay mesh so it doesn't render, but we'll still use it as the attach point for the particles
			if (TopOverlay.Material != nullptr)
			{
				InOverlayMesh->SetHiddenInGame(false);

				static FName FNameScale = TEXT("Scale");
				UMaterialInstanceDynamic* MID = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), TopOverlay.Material);
				if (MID)
				{
					MID->SetScalarParameterValue(FNameScale, WeaponRenderScale);
				}

				for (int32 i = 0; i < InOverlayMesh->GetNumMaterials(); i++)
				{
					InOverlayMesh->SetMaterial(i, MID);
				}
			}
			else
			{
				InOverlayMesh->SetHiddenInGame(true);
			}
			if (TopOverlay.Particles != NULL)
			{
				UParticleSystemComponent* PSC = NULL;
				for (USceneComponent* Child : InOverlayMesh->GetAttachChildren()) 
				{
					PSC = Cast<UParticleSystemComponent>(Child);
					if (PSC != NULL)
					{
						break;
					}
				}
				if (PSC == NULL)
				{
					PSC = NewObject<UParticleSystemComponent>(InOverlayMesh);
					PSC->SecondsBeforeInactive = 0.0f;
					PSC->SetAbsolute(false, false, true);
					PSC->RegisterComponent();
				}
				PSC->AttachToComponent(InOverlayMesh, FAttachmentTransformRules::KeepRelativeTransform, TopOverlay.ParticleAttachPoint);

				UParticleSystem* OverrideParticles = nullptr;
				if (true) // fixmesteve only for Udamage
				{
					OverrideParticles = Cast<AUTWeaponAttachment>(WeaponActor) ? UDamageOverrideEffect3P : UDamageOverrideEffect1P;
				}
				if (OverrideParticles != nullptr)
				{
					PSC->SetTemplate(OverrideParticles);
					PSC->InstanceParameters = InOverlayEffectParams;
				}
				else
				{
					PSC->SetTemplate(TopOverlay.Particles);
					PSC->InstanceParameters = InOverlayEffectParams;
					static FName NAME_Weapon(TEXT("Weapon"));
					static FName NAME_1PWeapon(TEXT("1PWeapon"));
					PSC->SetActorParameter(NAME_Weapon, WeaponActor);
					PSC->SetActorParameter(NAME_1PWeapon, WeaponActor);
				}
			}
			else
			{
				for (USceneComponent* Child : InOverlayMesh->GetAttachChildren())
				{
					UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Child);
					if (PSC != NULL)
					{
						if (PSC->IsActive())
						{
							PSC->bAutoDestroy = true;
							PSC->DeactivateSystem();
							PSC->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
							// failsafe destruction timer
							FTimerHandle Useless;
							TWeakObjectPtr<UParticleSystemComponent> WeakPSC = PSC;
							PSC->GetWorld()->GetTimerManager().SetTimer(Useless, FTimerDelegate::CreateLambda([WeakPSC]() { if (WeakPSC.IsValid()) { WeakPSC->DestroyComponent(); } }), 5.0f, false);
						}
						else
						{
							PSC->DestroyComponent();
						}
						break;
					}
				}
			}
		}
		else if (InOverlayMesh != NULL && InOverlayMesh->IsRegistered())
		{
			TArray<USceneComponent*> ChildrenCopy = InOverlayMesh->GetAttachChildren();
			for (USceneComponent* Child : ChildrenCopy)
			{
				UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Child);
				if (PSC != NULL && PSC->IsActive())
				{
					PSC->bAutoDestroy = true;
					PSC->DeactivateSystem();
					PSC->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
					// failsafe destruction timer
					FTimerHandle Useless;
					TWeakObjectPtr<UParticleSystemComponent> WeakPSC = PSC;
					PSC->GetWorld()->GetTimerManager().SetTimer(Useless, FTimerDelegate::CreateLambda([WeakPSC]() { if (WeakPSC.IsValid()) { WeakPSC->DestroyComponent(); } }), 5.0f, false);
				}
				else
				{
					Child->DestroyComponent(false);
				}
			}
			// we have to destroy the component instead of simply leaving it unregistered because of the way first person weapons handle component registration via AttachToOwner()
			// otherwise the overlay will get registered when it's unwanted
			InOverlayMesh->DestroyComponent(false);
			InOverlayMesh = NULL;
		}
	}
}
void AUTWeapon::UpdateOverlays()
{
	UpdateOverlaysShared(this, GetUTOwner(), Mesh, OverlayEffectParams, OverlayMesh);
}

void AUTWeapon::UpdateOutline()
{
	if (UTOwner == nullptr)
	{
		if (CustomDepthMesh != nullptr && CustomDepthMesh->IsRegistered())
		{
			CustomDepthMesh->UnregisterComponent();
		}
	}
	else
	{
		// show outline on weapon if ENEMIES have outline
		bool bOutlined = false;
		// this is a little hacky because the flag carrier outline replicates through PlayerState and isn't using UTCharacter's team mask
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTOwner->PlayerState);
		if (PS != nullptr && PS->bSpecialPlayer)
		{
			bOutlined = true;
		}
		else if (!UTOwner->IsOutlined(255))
		{
			for (int32 i = 0; i <= 8; i++)
			{
				if (UTOwner->IsOutlined(i) && UTOwner->GetTeamNum() != i)
				{
					bOutlined = true;
					break;
				}
			}
		}
		// 0 is a null value for the stencil so use team + 1
		// last bit in stencil is a bitflag so empty team uses 127
		uint8 NewStencilValue = (UTOwner->GetTeamNum() == 255) ? 127 : (UTOwner->GetTeamNum() + 1);
		NewStencilValue |= 128; // always show when unoccluded
		if (bOutlined)
		{
			if (CustomDepthMesh == NULL)
			{
				CustomDepthMesh = Cast<USkeletalMeshComponent>(CreateCustomDepthOutlineMesh(Mesh, this));
				// we can't use the default material because we need to apply the panini shader
				for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
				{
					CustomDepthMesh->SetMaterial(i, Mesh->GetMaterial(i));
				}
				CustomDepthMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
			}
			if (CustomDepthMesh->CustomDepthStencilValue != NewStencilValue)
			{
				CustomDepthMesh->CustomDepthStencilValue = NewStencilValue;
				CustomDepthMesh->MarkRenderStateDirty();
			}
			if (!CustomDepthMesh->IsRegistered())
			{
				CustomDepthMesh->RegisterComponent();
				CustomDepthMesh->LastRenderTime = GetMesh()->LastRenderTime;
				CustomDepthMesh->bRecentlyRendered = true;
			}
		}
		else
		{
			if (CustomDepthMesh != NULL && CustomDepthMesh->IsRegistered())
			{
				CustomDepthMesh->UnregisterComponent();
			}
		}
	}
}

void AUTWeapon::SetSkin(UMaterialInterface* NewSkin)
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
	
	if (GetMesh())
	{
		MeshMIDs.Empty();
		for (int i = 0; i < GetMesh()->GetNumMaterials(); i++)
		{
			if (GetMesh()->GetMaterial(i))
			{
				MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(i));
			}
		}
	}

	SetupSpecialMaterials();
}

float AUTWeapon::GetDamageRadius_Implementation(uint8 TestMode) const
{
	if (ProjClass.IsValidIndex(TestMode) && ProjClass[TestMode] != NULL)
	{
		return ProjClass[TestMode].GetDefaultObject()->DamageParams.OuterRadius;
	}
	else
	{
		return 0.0f;
	}
}

float AUTWeapon::BotDesireability_Implementation(APawn* Asker, AController* RequestOwner, AActor* Pickup, float PathDistance) const
{
	AUTCharacter* P = Cast<AUTCharacter>(Asker);
	if (P == NULL)
	{
		return 0.0f;
	}
	else
	{
		float Desire = BasePickupDesireability;

		AUTBot* B = Cast<AUTBot>(RequestOwner);
		if (B != NULL && B == P->Controller && B->IsFavoriteWeapon(GetClass()))
		{
			Desire *= 1.5f;
		}

		// see if bot already has a weapon of this type
		AUTWeapon* AlreadyHas = P->FindInventoryType<AUTWeapon>(GetClass());
		if (AlreadyHas != NULL)
		{
			//if (Bot.bHuntPlayer)
			//	return 0;
			if (Ammo == 0 || AlreadyHas->MaxAmmo == 0)
			{
				// weapon pickup doesn't give ammo and/or weapon has infinite ammo so we don't care once we have it
				return 0;
			}
			else if (AlreadyHas->Ammo >= AlreadyHas->MaxAmmo)
			{
				return 0.25f * Desire;
			}
			// bot wants this weapon for the ammo it holds
			else
			{
				return Desire * FMath::Max<float>(0.25f, float(AlreadyHas->MaxAmmo - AlreadyHas->Ammo) / float(AlreadyHas->MaxAmmo));
			}
		}
		//else if (Bot.bHuntPlayer && (desire * 0.833 < P.Weapon.AIRating - 0.1))
		//{
		//	return 0;
		//}
		else
		{
			return (B != NULL && B->NeedsWeapon()) ? (2.0f * Desire) : Desire;
		}
	}
}
float AUTWeapon::DetourWeight_Implementation(APawn* Asker, AActor* Pickup, float PathDistance) const
{
	AUTCharacter* P = Cast<AUTCharacter>(Asker);
	if (P == NULL)
	{
		return 0.0f;
	}
	// detour if currently equipped weapon
	else if (P->GetWeaponClass() == GetClass())
	{
		return BotDesireability(Asker, Asker->Controller, Pickup, PathDistance);
	}
	else
	{
		// detour if favorite weapon
		AUTBot* B = Cast<AUTBot>(P->Controller);
		if (B != NULL && B->IsFavoriteWeapon(GetClass()))
		{
			return BotDesireability(Asker, Asker->Controller, Pickup, PathDistance);
		}
		else
		{
			// detour if out of ammo for this weapon
			AUTWeapon* AlreadyHas = P->FindInventoryType<AUTWeapon>(GetClass());
			if (AlreadyHas == NULL || AlreadyHas->Ammo == 0)
			{
				return BotDesireability(Asker, Asker->Controller, Pickup, PathDistance);
			}
			else
			{
				// otherwise not important enough
				return 0.0f;
			}
		}
	}
}

float AUTWeapon::GetAISelectRating_Implementation()
{
	return HasAnyAmmo() ? BaseAISelectRating : 0.0f;
}

float AUTWeapon::SuggestAttackStyle_Implementation()
{
	return 0.0f;
}
float AUTWeapon::SuggestDefenseStyle_Implementation()
{
	return 0.0f;
}

bool AUTWeapon::IsPreparingAttack_Implementation()
{
	return false;
}

bool AUTWeapon::ShouldAIDelayFiring_Implementation()
{
	return false;
}

bool AUTWeapon::CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc)
{
	OptimalTargetLoc = TargetLoc;
	bool bVisible = false;
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B != NULL)
	{
		APawn* TargetPawn = Cast<APawn>(Target);
		// by default bots do not try shooting enemies when the enemy info is stale
		// since even if the target location is visible the enemy is probably not near there anymore
		// subclasses can override if their fire mode(s) are suitable for speculative or predictive firing
		const FBotEnemyInfo* EnemyInfo = (TargetPawn != NULL) ? B->GetEnemyInfo(TargetPawn, true) : NULL;
		if (EnemyInfo != NULL && GetWorld()->TimeSeconds - EnemyInfo->LastFullUpdateTime > 1.0f)
		{
			bVisible = false;
		}
		else
		{
			bVisible = B->UTLineOfSightTo(Target, GetFireStartLoc(), false, TargetLoc);
		}
	}
	else
	{
		const FVector StartLoc = GetFireStartLoc();
		FCollisionQueryParams Params(FName(TEXT("CanAttack")), false, Instigator);
		Params.AddIgnoredActor(Target);
		bVisible = !GetWorld()->LineTraceTestByChannel(StartLoc, TargetLoc, COLLISION_TRACE_WEAPON, Params);
	}
	if (bVisible)
	{
		// skip zoom modes by default
		TArray< uint8, TInlineAllocator<4> > ValidAIModes;
		for (uint8 i = 0; i < GetNumFireModes(); i++)
		{
			if (Cast<UUTWeaponStateZooming>(FiringState[i]) == NULL)
			{
				ValidAIModes.Add(i);
			}
		}
		if (!bPreferCurrentMode && ValidAIModes.Num() > 0)
		{
			BestFireMode = ValidAIModes[FMath::RandHelper(ValidAIModes.Num())];
		}
		return true;
	}
	else
	{
		return false;
	}
}

void AUTWeapon::NotifyKillWhileHolding_Implementation(TSubclassOf<UDamageType> DmgType)
{
}

int32 AUTWeapon::GetWeaponKillStats(AUTPlayerState* PS) const
{
	int32 KillCount = 0;
	if (PS)
	{
		if (KillStatsName != NAME_None)
		{
			KillCount += PS->GetStatsValue(KillStatsName);
		}
		if (AltKillStatsName != NAME_None)
		{
			KillCount += PS->GetStatsValue(AltKillStatsName);
		}
	}
	return KillCount;
}

int32 AUTWeapon::GetWeaponKillStatsForRound(AUTPlayerState* PS) const
{
	int32 KillCount = 0;
	if (PS)
	{
		if (KillStatsName != NAME_None)
		{
			KillCount += PS->GetRoundStatsValue(KillStatsName);
		}
		if (AltKillStatsName != NAME_None)
		{
			KillCount += PS->GetRoundStatsValue(AltKillStatsName);
		}
	}
	return KillCount;
}

int32 AUTWeapon::GetWeaponDeathStats(AUTPlayerState* PS) const
{
	int32 DeathCount = 0;
	if (PS)
	{
		if (DeathStatsName != NAME_None)
		{
			DeathCount += PS->GetStatsValue(DeathStatsName);
		}
		if (AltDeathStatsName != NAME_None)
		{
			DeathCount += PS->GetStatsValue(AltDeathStatsName);
		}
	}
	return DeathCount;
}

float AUTWeapon::GetWeaponHitsStats(AUTPlayerState* PS) const
{
	return (PS && (HitsStatsName != NAME_None)) ? PS->GetStatsValue(HitsStatsName) : 0.f;
}

float AUTWeapon::GetWeaponShotsStats(AUTPlayerState* PS) const
{
	return (PS && (ShotsStatsName != NAME_None)) ? PS->GetStatsValue(ShotsStatsName) : 0.f;
}

// TEMP for testing 1p offsets
void AUTWeapon::TestWeaponLoc(float X, float Y, float Z)
{
	Mesh->SetRelativeLocation(FVector(X, Y, Z));
}
void AUTWeapon::TestWeaponRot(float Pitch, float Yaw, float Roll)
{
	Mesh->SetRelativeRotation(FRotator(Pitch, Yaw, Roll));
}
void AUTWeapon::TestWeaponScale(float X, float Y, float Z)
{
	Mesh->SetRelativeScale3D(FVector(X, Y, Z));
}

void AUTWeapon::FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation)
{
	if (FlashCount > 0 || !InFlashLocation.IsZero())
	{
		CurrentFireMode = InFireMode;
		PlayFiringEffects();
	}
	else
	{
		StopFiringEffects();
	}
}

void AUTWeapon::FiringExtraUpdated_Implementation(uint8 NewFlashExtra, uint8 InFireMode)
{

}

void AUTWeapon::FiringEffectsUpdated_Implementation(uint8 InFireMode, FVector InFlashLocation)
{
	FVector SpawnLocation;
	FRotator SpawnRotation;
	GetImpactSpawnPosition(InFlashLocation, SpawnLocation, SpawnRotation);
	PlayImpactEffects(InFlashLocation, InFireMode, SpawnLocation, SpawnRotation);
}

UMaterialInstanceDynamic* AUTWeapon::GetZoomMaterial(uint8 FireModeNum) const
{
	UUTWeaponStateZooming* ZoomFireMode = FiringState.IsValidIndex(FireModeNum) ? Cast<UUTWeaponStateZooming>(FiringState[FireModeNum]) : NULL;
	if (ZoomFireMode != NULL && ZoomFireMode->OverlayMI != NULL)
	{
		return ZoomFireMode->OverlayMI;
	}
	else
	{
		return NULL;
	}
}

void AUTWeapon::TickZoom(float DeltaTime)
{
	if (GetUTOwner() != nullptr && ZoomModes.IsValidIndex(CurrentZoomMode))
	{
		if (ZoomState != EZoomState::EZS_NotZoomed)
		{
			if (ZoomState == EZoomState::EZS_ZoomingIn)
			{
				ZoomTime += DeltaTime;

				if (ZoomTime >= ZoomModes[CurrentZoomMode].Time)
				{
					OnZoomedIn();
				}
			}
			else if (ZoomState == EZoomState::EZS_ZoomingOut)
			{
				ZoomTime -= DeltaTime;

				if (ZoomTime <= 0.0f)
				{
					OnZoomedOut();
				}
			}
			ZoomTime = FMath::Clamp(ZoomTime, 0.0f, ZoomModes[CurrentZoomMode].Time);

			//Do the FOV change
			if (GetNetMode() != NM_DedicatedServer)
			{
				float StartFov = ZoomModes[CurrentZoomMode].StartFOV;
				float EndFov = ZoomModes[CurrentZoomMode].EndFOV;

				//Use the players default FOV if the FOV is zero
				AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController());
				if (UTPC != nullptr)
				{
					if (StartFov == 0.0f)
					{
						StartFov = UTPC->ConfigDefaultFOV;
					}
					if (EndFov == 0.0f)
					{
						EndFov = UTPC->ConfigDefaultFOV;
					}
				}

				//Calculate the FOV based on the zoom time
				float FOV = 90.0f;
				if (ZoomModes[CurrentZoomMode].Time == 0.0f)
				{
					FOV = StartFov;
				}
				else
				{
					FOV = FMath::Lerp(StartFov, EndFov, FMath::Max(0.f, ZoomTime - 0.15f) / ZoomModes[CurrentZoomMode].Time);
					FOV = FMath::Clamp(FOV, EndFov, StartFov);
				}

				//Set the FOV
				AUTPlayerCameraManager* Camera = Cast<AUTPlayerCameraManager>(GetUTOwner()->GetPlayerCameraManager());
				if (Camera != NULL && Camera->GetCameraStyleWithOverrides() == FName(TEXT("Default")))
				{
					Camera->SetFOV(FOV);
				}
			}
		}
	}
}


void AUTWeapon::LocalSetZoomMode(uint8 NewZoomMode)
{
	if (ZoomModes.IsValidIndex(CurrentZoomMode))
	{
		CurrentZoomMode = NewZoomMode;
	}
	else
	{
		UE_LOG(LogUTWeapon, Warning, TEXT("%s::LocalSetZoomMode(): Invalid Zoom Mode: %d"), *GetName(), NewZoomMode);
	}
}

void AUTWeapon::SetZoomMode(uint8 NewZoomMode)
{
	//Only Locally controlled players set the zoom mode so the server stays in sync
	if (GetUTOwner() && GetUTOwner()->IsLocallyControlled() && CurrentZoomMode != NewZoomMode)
	{
		if (Role < ROLE_Authority)
		{
			ServerSetZoomMode(NewZoomMode);
		}
		LocalSetZoomMode(NewZoomMode);
	}
}

bool AUTWeapon::ServerSetZoomMode_Validate(uint8 NewZoomMode) { return true; }
void AUTWeapon::ServerSetZoomMode_Implementation(uint8 NewZoomMode)
{
	LocalSetZoomMode(NewZoomMode);
}

void AUTWeapon::LocalSetZoomState(uint8 NewZoomState)
{
	if (ZoomModes.IsValidIndex(CurrentZoomMode))
	{
		if (NewZoomState != ZoomState)
		{
			ZoomState = (EZoomState::Type)NewZoomState;

			//Need to reset the zoom time since this state might be skipped on spec clients if states switch too fast
			if (ZoomState == EZoomState::EZS_NotZoomed)
			{
				ZoomCount++;
				OnRep_ZoomCount();
			}
			OnRep_ZoomState();
		}
	}
	else
	{
		UE_LOG(LogUTWeapon, Warning, TEXT("%s::LocalSetZoomState(): Invalid Zoom Mode: %d"), *GetName(), CurrentZoomMode);
	}
}

void AUTWeapon::SetZoomState(TEnumAsByte<EZoomState::Type> NewZoomState)
{
	//Only Locally controlled players set the zoom state so the server stays in sync
	if (GetUTOwner() && GetUTOwner()->IsLocallyControlled() && NewZoomState != ZoomState)
	{
		if (Role < ROLE_Authority)
		{
			ServerSetZoomState(NewZoomState);
		}
		LocalSetZoomState(NewZoomState);
	}
}

bool AUTWeapon::ServerSetZoomState_Validate(uint8 NewZoomState) { return true; }
void AUTWeapon::ServerSetZoomState_Implementation(uint8 NewZoomState)
{
	LocalSetZoomState(NewZoomState);
}

void AUTWeapon::OnRep_ZoomState_Implementation()
{
	if (GetNetMode() != NM_DedicatedServer && ZoomState == EZoomState::EZS_NotZoomed && GetUTOwner() && GetUTOwner()->GetPlayerCameraManager())
	{
		GetUTOwner()->GetPlayerCameraManager()->UnlockFOV();
	}
}

void AUTWeapon::OnRep_ZoomCount()
{
	//For spectators we don't want to clear the time if ZoomTime was just replicated (COND_InitialOnly). Can't do custom rep or we'll loose the COND_SkipOwner
	//CreationTime will be 0 for regular spectator, for demo rec it will be close to GetWorld()->TimeSeconds
	if (CreationTime != 0.0f && GetWorld()->TimeSeconds - CreationTime > 0.2)
	{
		ZoomTime = 0.0f;
	}
}

void AUTWeapon::OnZoomedIn_Implementation()
{
	SetZoomState(EZoomState::EZS_Zoomed);
}

void AUTWeapon::OnZoomedOut_Implementation()
{
	SetZoomState(EZoomState::EZS_NotZoomed);
}

bool AUTWeapon::CanSwitchTo()
{
	return HasAnyAmmo();
}

void AUTWeapon::RegisterAllComponents()
{
	// in editor and preview, just register all the components right now
	if (GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview || GetWorld()->WorldType == EWorldType::GamePreview)
	{
		Super::RegisterAllComponents();
		return;
	}

	TInlineComponentArray<USceneComponent*> AllSceneComponents;
	GetComponents(AllSceneComponents);

	for (USceneComponent* Child : AllSceneComponents)
	{
		Child->bAutoRegister = bAttachingToOwner;
	}

	Super::RegisterAllComponents();
}
