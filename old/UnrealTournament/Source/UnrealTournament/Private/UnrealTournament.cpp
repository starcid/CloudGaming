// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "AssetRegistryModule.h"
#include "UTWorldSettings.h"
#include "UTLevelSummary.h"
#include "UnrealNetwork.h"
#include "UTProfileItem.h"
#include "UTCharacterContent.h"
#include "UTTaunt.h"
#include "UTBotCharacter.h"
#include "StatNames.h"
#include "Runtime/PakFile/Public/IPlatformFilePak.h"
#include "Misc/NetworkVersion.h"
#include "ARFilter.h"
#include "Runtime/Launch/Resources/Version.h"

#if WITH_PROFILE
#include "OnlineSubsystemMcp.h"
#include "GameServiceMcp.h"
#endif

class FUTModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUTModule, UnrealTournament, "UnrealTournament");
 
DEFINE_LOG_CATEGORY(UT);
DEFINE_LOG_CATEGORY(UTNet);
DEFINE_LOG_CATEGORY(UTLoading);
DEFINE_LOG_CATEGORY(UTConnection);

static uint32 UTGetNetworkVersion()
{
	uint32 Override;
	if ( FParse::Value(FCommandLine::Get(), TEXT("utversionoverride="), Override) )
	{
		return Override;
	}

	return BUILT_FROM_CHANGELIST;
}

const FString ITEM_STAT_PREFIX = TEXT("ITEM_");

// init editor hooks
#if WITH_EDITOR

#include "SlateBasics.h"
#include "UTDetailsCustomization.h"

static void AddLevelSummaryAssetTags(const UWorld* InWorld, TArray<UObject::FAssetRegistryTag>& OutTags)
{
	// add level summary data to the asset registry as part of the world
	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(InWorld->GetWorldSettings());
	if (Settings != NULL && Settings->GetLevelSummary() != NULL)
	{
		Settings->GetLevelSummary()->GetAssetRegistryTags(OutTags);
	}
}

void FUTModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("UTWeapon", FOnGetDetailCustomizationInstance::CreateStatic(&FUTDetailsCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout("UTWeaponAttachment", FOnGetDetailCustomizationInstance::CreateStatic(&FUTDetailsCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

	FWorldDelegates::GetAssetTags.AddStatic(&AddLevelSummaryAssetTags);

	// set up our handler for network versioning
	FNetworkVersion::GetLocalNetworkVersionOverride.BindStatic(&UTGetNetworkVersion);

	GConfig->LoadFile(FPaths::GeneratedConfigDir() + TEXT("Mod.ini"));
}

#else

void FUTModule::StartupModule()
{
	// set up our handler for network versioning
	FNetworkVersion::GetLocalNetworkVersionOverride.BindStatic(&UTGetNetworkVersion);
}
#endif

FCollisionResponseParams WorldResponseParams = []()
{
	FCollisionResponseParams Result(ECR_Ignore);
	Result.CollisionResponse.WorldStatic = ECR_Block;
	Result.CollisionResponse.WorldDynamic = ECR_Block;
	return Result;
}();

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleLODLevel.h"

bool IsLoopingParticleSystem(const UParticleSystem* PSys)
{
	for (int32 i = 0; i < PSys->Emitters.Num(); i++)
	{
		if (PSys->Emitters[i]->GetLODLevel(0)->RequiredModule->EmitterLoops <= 0 && PSys->Emitters[i]->GetLODLevel(0)->RequiredModule->bEnabled)
		{
			return true;
		}
	}
	return false;
}

void UnregisterComponentTree(USceneComponent* Comp)
{
	if (Comp != NULL)
	{
		TArray<USceneComponent*> Children;
		Comp->GetChildrenComponents(true, Children);
		Comp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Comp->UnregisterComponent();
		for (USceneComponent* Child : Children)
		{
			Child->UnregisterComponent();
		}
	}
}

APhysicsVolume* FindPhysicsVolume(UWorld* World, const FVector& TestLoc, const FCollisionShape& Shape)
{
	APhysicsVolume* NewVolume = World->GetDefaultPhysicsVolume();

	// check for all volumes that overlap the component
	TArray<FOverlapResult> Hits;
	static FName NAME_PhysicsVolumeTrace = FName(TEXT("PhysicsVolumeTrace"));
	FComponentQueryParams Params(NAME_PhysicsVolumeTrace, NULL);

	World->OverlapMultiByChannel(Hits, TestLoc, FQuat::Identity, ECC_Pawn, Shape, Params);

	for (int32 HitIdx = 0; HitIdx < Hits.Num(); HitIdx++)
	{
		const FOverlapResult& Link = Hits[HitIdx];
		APhysicsVolume* const V = Cast<APhysicsVolume>(Link.GetActor());
		if (V != NULL && V->Priority > NewVolume->Priority && (V->bPhysicsOnContact || V->EncompassesPoint(TestLoc)))
		{
			NewVolume = V;
		}
	}

	return NewVolume;
}

float GetLocationGravityZ(UWorld* World, const FVector& TestLoc, const FCollisionShape& Shape)
{
	APhysicsVolume* Volume = FindPhysicsVolume(World, TestLoc, Shape);
	return (Volume != NULL) ? Volume->GetGravityZ() : World->GetDefaultGravityZ();
}

UMeshComponent* CreateCustomDepthOutlineMesh(UMeshComponent* Archetype, AActor* Owner)
{
	UMeshComponent* CustomDepthMesh = DuplicateObject<UMeshComponent>(Archetype, Owner);
	CustomDepthMesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	if (Cast<USkeletalMeshComponent>(CustomDepthMesh) != nullptr)
	{
		// TODO: scary that these get copied, need an engine solution and/or safe way to duplicate objects during gameplay
		((USkeletalMeshComponent*)CustomDepthMesh)->PrimaryComponentTick = CustomDepthMesh->GetClass()->GetDefaultObject<USkeletalMeshComponent>()->PrimaryComponentTick;
		((USkeletalMeshComponent*)CustomDepthMesh)->PostPhysicsComponentTick = CustomDepthMesh->GetClass()->GetDefaultObject<USkeletalMeshComponent>()->PostPhysicsComponentTick;
	}
	CustomDepthMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // make sure because could be in ragdoll
	CustomDepthMesh->SetSimulatePhysics(false);
	CustomDepthMesh->SetCastShadow(false);
	if (Cast<USkinnedMeshComponent>(CustomDepthMesh) != nullptr)
	{
		((USkinnedMeshComponent*)CustomDepthMesh)->SetMasterPoseComponent((USkinnedMeshComponent*)Archetype);
	}
	for (int32 i = 0; i < CustomDepthMesh->GetNumMaterials(); i++)
	{
		CustomDepthMesh->SetMaterial(i, UMaterial::GetDefaultMaterial(MD_Surface));
	}
	CustomDepthMesh->BoundsScale = 15000.f;
	CustomDepthMesh->bVisible = true;
	CustomDepthMesh->bHiddenInGame = false;
	CustomDepthMesh->bRenderInMainPass = false;
	CustomDepthMesh->bRenderCustomDepth = true;
	CustomDepthMesh->AttachToComponent(Archetype, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CustomDepthMesh->RelativeLocation = FVector::ZeroVector;
	CustomDepthMesh->RelativeRotation = FRotator::ZeroRotator;
	CustomDepthMesh->RelativeScale3D = FVector(1.0f, 1.0f, 1.0f);
	return CustomDepthMesh;
}

static TMap<FName, FString> HackedEntitlementTable = []()
{
	TMap<FName, FString> Result;
	Result.Add(TEXT("BP_Round_HelmetGoggles"), TEXT("91afa66fbf744726af33dba391657296"));
	Result.Add(TEXT("BP_Round_HelmetGoggles_C"), TEXT("91afa66fbf744726af33dba391657296"));
	Result.Add(TEXT("BP_Round_HelmetLeader"), TEXT("91afa66fbf744726af33dba391657296"));
	Result.Add(TEXT("BP_Round_HelmetLeader_C"), TEXT("91afa66fbf744726af33dba391657296"));
	Result.Add(TEXT("BP_SkullMask"), TEXT("606862e8a0ec4f5190f67c6df9d4ea81"));
	Result.Add(TEXT("BP_SkullMask_C"), TEXT("606862e8a0ec4f5190f67c6df9d4ea81"));
	Result.Add(TEXT("BP_SkullHornsMask"), TEXT("606862e8a0ec4f5190f67c6df9d4ea81"));
	Result.Add(TEXT("BP_SkullHornsMask_C"), TEXT("606862e8a0ec4f5190f67c6df9d4ea81"));
	Result.Add(TEXT("BP_BaseballHat"), TEXT("8747335f79dd4bec8ddc03214c307950"));
	Result.Add(TEXT("BP_BaseballHat_C"), TEXT("8747335f79dd4bec8ddc03214c307950"));
	Result.Add(TEXT("BP_BaseballHat_Leader"), TEXT("8747335f79dd4bec8ddc03214c307950"));
	Result.Add(TEXT("BP_BaseballHat_Leader_C"), TEXT("8747335f79dd4bec8ddc03214c307950"));
	Result.Add(TEXT("BP_CardboardHat"), TEXT("9a1ad6c3c10e438f9602c14ad1b67bfa"));
	Result.Add(TEXT("BP_CardboardHat_C"), TEXT("9a1ad6c3c10e438f9602c14ad1b67bfa"));
	Result.Add(TEXT("BP_CardboardHat_Leader"), TEXT("9a1ad6c3c10e438f9602c14ad1b67bfa"));
	Result.Add(TEXT("BP_CardboardHat_Leader_C"), TEXT("9a1ad6c3c10e438f9602c14ad1b67bfa"));
	Result.Add(TEXT("BP_Char_Oct2015"), TEXT("527E7E209F4142F8835BA696919E2BEC"));
	Result.Add(TEXT("BP_Char_Oct2015_C"), TEXT("527E7E209F4142F8835BA696919E2BEC"));
	Result.Add(TEXT("DM-Lea"), TEXT("0d5e275ca99d4cf0b03c518a6b279e26"));
	Result.Add(TEXT("CTF-Pistola"), TEXT("48d281f487154bb29dd75bd7bb95ac8e"));
	Result.Add(TEXT("DM-Batrankus"), TEXT("d8ac8a7ce06d44ab8e6b7284184e556e"));
	Result.Add(TEXT("DM-Backspace"), TEXT("08af4962353443058766998d6b881707"));
	Result.Add(TEXT("DM-Salt"), TEXT("27f36270a1ec44509e72687c4ba6845a"));
	Result.Add(TEXT("CTF-Polaris"), TEXT("a99f379bfb9b41c69ddf0bfbc4a48860"));
	Result.Add(TEXT("DM-Unsaved"), TEXT("65fb5029cddb4de7b5fa155b6992e6a3"));
	return Result;
}();

FString GetRequiredEntitlementFromAsset(const FAssetData& Asset)
{
	const FString* ReqEntitlementId = Asset.TagsAndValues.Find(TEXT("Entitlement"));
	if (ReqEntitlementId != NULL && !ReqEntitlementId->IsEmpty())
	{
		return *ReqEntitlementId;
	}
	else
	{
		// FIXME: total temp hack since we don't have any way to embed entitlement IDs with the asset yet...
		FString* Found = HackedEntitlementTable.Find(Asset.AssetName);
		return (Found != NULL) ? *Found : FString();
	}
}
FString GetRequiredEntitlementFromObj(UObject* Asset)
{
	if (Asset == NULL)
	{
		return FString();
	}
	else
	{
		FString Result;

		UClass* BPClass = Cast<UClass>(Asset);
		if (BPClass != NULL)
		{
			UStrProperty* EntitlementProp = FindField<UStrProperty>(BPClass, TEXT("Entitlement"));
			if (EntitlementProp != NULL)
			{
				Result = EntitlementProp->GetPropertyValue_InContainer(BPClass->GetDefaultObject());
			}
		}
		else
		{
			UStrProperty* EntitlementProp = FindField<UStrProperty>(Asset->GetClass(), TEXT("Entitlement"));
			if (EntitlementProp != NULL)
			{
				Result = EntitlementProp->GetPropertyValue_InContainer(Asset);
			}
		}
		if (!Result.IsEmpty())
		{
			return Result;
		}
		else
		{
			// FIXME: total temp hack since we don't have any way to embed entitlement IDs with the asset yet...
			FString* Found = HackedEntitlementTable.Find(Asset->GetFName());
			return (Found != NULL) ? *Found : FString();
		}
	}
}
FString GetRequiredEntitlementFromPackageName(FName PackageName)
{
	FAssetData Asset;
	Asset.AssetName = PackageName;
	return GetRequiredEntitlementFromAsset(Asset);
}

bool LocallyHasEntitlement(const FString& Entitlement)
{
	if (Entitlement.IsEmpty())
	{
		// no entitlement required
		return true;
	}
	else
	{
		if (IOnlineSubsystem::Get() != NULL)
		{
			IOnlineIdentityPtr IdentityInterface = IOnlineSubsystem::Get()->GetIdentityInterface();
			IOnlineEntitlementsPtr EntitlementInterface = IOnlineSubsystem::Get()->GetEntitlementsInterface();
			if (IdentityInterface.IsValid() && EntitlementInterface.IsValid())
			{
				for (int32 i = 0; i < MAX_LOCAL_PLAYERS; i++)
				{
					TSharedPtr<const FUniqueNetId> Id = IdentityInterface->GetUniquePlayerId(i);
					if (Id.IsValid())
					{
						if (EntitlementInterface->GetItemEntitlement(*Id.Get(), Entitlement).IsValid())
						{
							return true;
						}
					}
				}
			}
		}
		return false;
	}
}

bool LocallyOwnsItemFor(const FString& Path)
{
	const TIndirectArray<FWorldContext>& AllWorlds = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : AllWorlds)
	{
		for (FLocalPlayerIterator It(GEngine, Context.World()); It; ++It)
		{
			UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(*It);
			if (LP != NULL && LP->OwnsItemFor(Path))
			{
				return true;
			}
		}
	}

	return false;
}

bool LocallyHasAchievement(FName Achievement)
{
	if (Achievement == NAME_None)
	{
		return true;
	}
	else
	{
		const TIndirectArray<FWorldContext>& AllWorlds = GEngine->GetWorldContexts();
		for (const FWorldContext& Context : AllWorlds)
		{
			for (FLocalPlayerIterator It(GEngine, Context.World()); It; ++It)
			{
				UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(*It);
				if (LP != NULL && LP->GetProgressionStorage() != NULL && LP->GetProgressionStorage()->Achievements.Contains(Achievement))
				{
					return true;
				}
			}
		}

		return false;
	}
}

void GetAllAssetData(UClass* BaseClass, TArray<FAssetData>& AssetList, bool bRequireEntitlements)
{
	// calling this with UBlueprint::StaticClass() is probably a bug where the user intended to call GetAllBlueprintAssetData()
	if (BaseClass == UBlueprint::StaticClass())
	{
#if DO_GUARD_SLOW
		UE_LOG(UT, Fatal, TEXT("GetAllAssetData() should not be used for blueprints; call GetAllBlueprintAssetData() instead"));
#else
		UE_LOG(UT, Error, TEXT("GetAllAssetData() should not be used for blueprints; call GetAllBlueprintAssetData() instead"));
#endif
		return;
	}

	// force disable local entitlement checks on dedicated server
	bRequireEntitlements = bRequireEntitlements && !IsRunningDedicatedServer();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> RootPaths;
	FPackageName::QueryRootContentPaths(RootPaths);

#if WITH_EDITOR
	// HACK: workaround for terrible registry performance when scanning; limit search paths to improve perf a bit
	RootPaths.Remove(TEXT("/Engine/"));
	RootPaths.Remove(TEXT("/Game/"));
	RootPaths.Remove(TEXT("/Paper2D/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Maps/"));
	RootPaths.Add(TEXT("/Game/Maps/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Blueprints/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Pickups/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Weapons/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Character/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/ProfileItems/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Lea/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Batrankus/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Backspace/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Polaris/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Unsaved/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Salt/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Pistola/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Stu/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/SR/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Loot/"));
	// Cooked data has the asset data already set up
	AssetRegistry.ScanPathsSynchronous(RootPaths);
#endif

	FARFilter ARFilter;
	if (BaseClass != NULL)
	{
		ARFilter.ClassNames.Add(BaseClass->GetFName());
		// Add any old names to the list in case things haven't been resaved
		TArray<FName> OldNames = FLinkerLoad::FindPreviousNamesForClass(BaseClass->GetPathName(), false);
		ARFilter.ClassNames.Append(OldNames);
	}
	ARFilter.bRecursivePaths = true;
	ARFilter.bIncludeOnlyOnDiskAssets = true;
	ARFilter.bRecursiveClasses = true;

	AssetRegistry.GetAssets(ARFilter, AssetList);

	// query entitlements for any assets and remove those that are not usable
	if (bRequireEntitlements)
	{
		for (int32 i = AssetList.Num() - 1; i >= 0; i--)
		{
			if (!LocallyHasEntitlement(GetRequiredEntitlementFromAsset(AssetList[i])))
			{
				AssetList.RemoveAt(i);
			}
			else
			{
				const FString* ReqAchievement = AssetList[i].TagsAndValues.Find(FName(TEXT("RequiredAchievement")));
				if (ReqAchievement != NULL && !LocallyHasAchievement(**ReqAchievement))
				{
					UE_LOG(UT, Verbose, TEXT("Don't have achievement for %s"), *AssetList[i].AssetName.ToString());
					AssetList.RemoveAt(i);
				}
				else
				{
					const FString* NeedsItem = AssetList[i].TagsAndValues.Find(FName(TEXT("bRequiresItem")));
					if (NeedsItem != NULL && NeedsItem->ToBool() && !LocallyOwnsItemFor(AssetList[i].ObjectPath.ToString()))
					{
						AssetList.RemoveAt(i);
					}
				}
			}
		}
	}
}

void GetAllBlueprintAssetData(UClass* BaseClass, TArray<FAssetData>& AssetList, bool bRequireEntitlements)
{
	// force disable local entitlement checks on dedicated server
	bRequireEntitlements = bRequireEntitlements && !IsRunningDedicatedServer();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> RootPaths;
	FPackageName::QueryRootContentPaths(RootPaths);

#if WITH_EDITOR
	// HACK: workaround for terrible registry performance when scanning; limit search paths to improve perf a bit
	RootPaths.Remove(TEXT("/Engine/"));
	RootPaths.Remove(TEXT("/Game/"));
	RootPaths.Remove(TEXT("/Paper2D/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Maps/"));
	RootPaths.Add(TEXT("/Game/Maps/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Blueprints/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Pickups/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Weapons/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/Character/"));
	RootPaths.Add(TEXT("/Game/RestrictedAssets/UI/Crosshairs/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/PK/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Teams/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/Cosmetic_Items/"));
	RootPaths.Add(TEXT("/Game/EpicInternal/WeaponSkins/"));
	// Cooked data has the asset data already set up
	AssetRegistry.ScanPathsSynchronous(RootPaths);
#endif

	FARFilter ARFilter;
	ARFilter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	/*for (int32 PathIndex = 0; PathIndex < RootPaths.Num(); PathIndex++)
	{
		ARFilter.PackagePaths.Add(FName(*RootPaths[PathIndex]));
	}*/

	ARFilter.bRecursivePaths = true;
	ARFilter.bRecursiveClasses = true;
	ARFilter.bIncludeOnlyOnDiskAssets = true;

	if (BaseClass == NULL)
	{
		AssetRegistry.GetAssets(ARFilter, AssetList);
	}
	else
	{
		// TODO: the below filtering is torturous because the asset registry does not contain full inheritance information for blueprints
		// nor does it return full class paths when you request a class tree

		TArray<FAssetData> LocalAssetList;
		AssetRegistry.GetAssets(ARFilter, LocalAssetList);

		TSet<FString> UnloadedBaseClassPaths;
		// first pass: determine the inheritance that we can trivially verify are the correct class because their parent is in memory
		for (int32 i = 0; i < LocalAssetList.Num(); i++)
		{
			const FString* LoadedParentClass = LocalAssetList[i].TagsAndValues.Find("ParentClass");
			if (LoadedParentClass != NULL && !LoadedParentClass->IsEmpty())
			{
				UClass* Class = FindObject<UClass>(ANY_PACKAGE, **LoadedParentClass);
				if (Class == NULL)
				{
					// apparently you have to 'load' native classes once for FindObject() to reach them
					// figure out if this parent is such a class and if so, allow LoadObject()
					FString ParentPackage = *LoadedParentClass;
					ConstructorHelpers::StripObjectClass(ParentPackage);
					if (ParentPackage.StartsWith(TEXT("/Script/")))
					{
						ParentPackage = ParentPackage.LeftChop(ParentPackage.Len() - ParentPackage.Find(TEXT(".")));
						if (FindObject<UPackage>(NULL, *ParentPackage) != NULL)
						{
							Class = LoadObject<UClass>(NULL, **LoadedParentClass, NULL, LOAD_NoWarn | LOAD_Quiet);
						}
					}
				}
				if (Class != NULL)
				{
					if (Class->IsChildOf(BaseClass))
					{
						AssetList.Add(LocalAssetList[i]);
						const FString* GenClassPath = LocalAssetList[i].TagsAndValues.Find("GeneratedClass");
						if (GenClassPath != NULL)
						{
							UnloadedBaseClassPaths.Add(*GenClassPath);
						}
					}
					LocalAssetList.RemoveAt(i);
					i--;
				}
			}
			else
			{
				// asset info is missing; fail
				LocalAssetList.RemoveAt(i);
				i--;
			}
		}
		// now go through the remainder and match blueprints against an unloaded parent
		// if we find no new matching assets, the rest must be the wrong super
		bool bFoundAny = false;
		do 
		{
			bFoundAny = false;
			for (int32 i = 0; i < LocalAssetList.Num(); i++)
			{
				if (UnloadedBaseClassPaths.Find(*LocalAssetList[i].TagsAndValues.Find("ParentClass")))
				{
					AssetList.Add(LocalAssetList[i]);
					const FString* GenClassPath = LocalAssetList[i].TagsAndValues.Find("GeneratedClass");
					if (GenClassPath != NULL)
					{
						UnloadedBaseClassPaths.Add(*GenClassPath);
					}
					LocalAssetList.RemoveAt(i);
					i--;
					bFoundAny = true;
				}
			}
		} while (bFoundAny && LocalAssetList.Num() > 0);
	}

	// query entitlements for any assets and remove those that are not usable
	if (bRequireEntitlements)
	{
		for (int32 i = AssetList.Num() - 1; i >= 0; i--)
		{
			if (!LocallyHasEntitlement(GetRequiredEntitlementFromAsset(AssetList[i])))
			{
				AssetList.RemoveAt(i);
			}
			else
			{
				const FString* ReqAchievement = AssetList[i].TagsAndValues.Find(FName(TEXT("RequiredAchievement")));
				if (ReqAchievement != NULL && !LocallyHasAchievement(**ReqAchievement))
				{
					UE_LOG(UT, Verbose, TEXT("Don't have achievement for %s"), *AssetList[i].AssetName.ToString());
					AssetList.RemoveAt(i);
				}
				else
				{
					const FString* NeedsItem = AssetList[i].TagsAndValues.Find(FName(TEXT("bRequiresItem")));
					if (NeedsItem != NULL && NeedsItem->ToBool() && !LocallyOwnsItemFor(AssetList[i].ObjectPath.ToString()))
					{
						AssetList.RemoveAt(i);
					}
				}
			}
		}
	}
}

FString GetModPakFilenameFromPkg(const FString& PkgName)
{
	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile")));
	if (PakFileMgr == NULL)
	{
		return FString();
	}
	else
	{
		FString Filename;
		if (FPackageName::DoesPackageExist(PkgName, NULL, &Filename))
		{
			FPakFile* Pak = NULL;
			PakFileMgr->FindFileInPakFiles(*Filename, &Pak);
			return (Pak != NULL && !FPaths::GetBaseFilename(Pak->GetFilename()).StartsWith(TEXT("unrealtournament-"))) ? Pak->GetFilename() : FString();
		}
		else
		{
			return FString();
		}
	}
}
FString GetModPakFilenameFromPath(FString ObjPathName)
{
	ConstructorHelpers::StripObjectClass(ObjPathName);
	int32 DotIndex = ObjPathName.Find(TEXT("."));
	return (DotIndex == INDEX_NONE) ? FString() : GetModPakFilenameFromPkg(ObjPathName.Left(DotIndex));
}

void SetTimerUFunc(UObject* Obj, FName FuncName, float Time, bool bLooping)
{
	if (Obj != NULL)
	{
		const UWorld* const World = GEngine->GetWorldFromContextObject(Obj);
		if (World != NULL)
		{
			UFunction* const Func = Obj->FindFunction(FuncName);
			if (Func == NULL)
			{
				UE_LOG(UT, Warning, TEXT("SetTimer: Object %s does not have a function named '%s'"), *Obj->GetName(), *FuncName.ToString());
			}
			else if (Func->ParmsSize > 0)
			{
				// User passed in a valid function, but one that takes parameters
				// FTimerDynamicDelegate expects zero parameters and will choke on execution if it tries
				// to execute a mismatched function
				UE_LOG(UT, Warning, TEXT("SetTimer passed a function (%s) that expects parameters."), *FuncName.ToString());
			}
			else
			{
				FTimerDynamicDelegate Delegate;
				Delegate.BindUFunction(Obj, FuncName);

				FTimerHandle Handle = World->GetTimerManager().K2_FindDynamicTimerHandle(Delegate);
				World->GetTimerManager().SetTimer(Handle, Delegate, Time, bLooping);
			}
		}
	}
}
bool IsTimerActiveUFunc(UObject* Obj, FName FuncName, float* TotalTime, float* ElapsedTime)
{
	if (Obj != NULL)
	{
		const UWorld* const World = GEngine->GetWorldFromContextObject(Obj);
		if (World != NULL)
		{
			UFunction* const Func = Obj->FindFunction(FuncName);
			if (Func == NULL)
			{
				UE_LOG(UT, Warning, TEXT("IsTimerActive: Object %s does not have a function named '%s'"), *Obj->GetName(), *FuncName.ToString());
				return false;
			}
			else
			{
				FTimerDynamicDelegate Delegate;
				Delegate.BindUFunction(Obj, FuncName);

				FTimerHandle Handle = World->GetTimerManager().K2_FindDynamicTimerHandle(Delegate);
				if (World->GetTimerManager().IsTimerActive(Handle))
				{
					if (TotalTime != NULL)
					{
						*TotalTime = World->GetTimerManager().GetTimerRate(Handle);
					}
					if (ElapsedTime != NULL)
					{
						*ElapsedTime = World->GetTimerManager().GetTimerElapsed(Handle);
					}
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
void ClearTimerUFunc(UObject* Obj, FName FuncName)
{
	if (Obj != NULL)
	{
		const UWorld* const World = GEngine->GetWorldFromContextObject(Obj);
		if (World != NULL)
		{
			UFunction* const Func = Obj->FindFunction(FuncName);
			if (Func == NULL)
			{
				UE_LOG(UT, Warning, TEXT("ClearTimer: Object %s does not have a function named '%s'"), *Obj->GetName(), *FuncName.ToString());
			}
			else
			{
				FTimerDynamicDelegate Delegate;
				Delegate.BindUFunction(Obj, FuncName);

				FTimerHandle Handle = World->GetTimerManager().K2_FindDynamicTimerHandle(Delegate);
				return World->GetTimerManager().ClearTimer(Handle);
			}
		}
	}
}

const FString GetEpicAppName()
{
	FString EpicAppName;
#if WITH_PROFILE
	FOnlineSubsystemMcp* OnlineSubMcp = IOnlineSubsystem::Get() ? (FOnlineSubsystemMcp*)IOnlineSubsystem::Get() : nullptr;
	if (OnlineSubMcp && OnlineSubMcp->GetMcpGameService().IsValid())
	{
		EpicAppName = OnlineSubMcp->GetMcpGameService()->GetAppName();
	}
#endif
	return EpicAppName;
}

const FString GetBackendBaseUrl()
{
	FString BaseURL;
#if WITH_PROFILE
	FOnlineSubsystemMcp* OnlineSubMcp = IOnlineSubsystem::Get() ? (FOnlineSubsystemMcp*)IOnlineSubsystem::Get() : nullptr;
	if (OnlineSubMcp && OnlineSubMcp->GetMcpGameService().IsValid())
	{
		BaseURL = OnlineSubMcp->GetMcpGameService()->GetBaseUrl();
	}
#endif
	return BaseURL;
}

FHttpRequestPtr ReadBackendStats(const FHttpRequestCompleteDelegate& ResultDelegate, const FString& StatsID, const FString& QueryWindow)
{
	FHttpRequestPtr StatsReadRequest = FHttpModule::Get().CreateRequest();
	if (StatsReadRequest.IsValid())
	{
		FString BaseURL = GetBackendBaseUrl();
		FString CommandURL = TEXT("/api/stats/accountId/");
		FString FinalStatsURL = BaseURL + CommandURL + StatsID + TEXT("/bulk/window/") + QueryWindow;

		StatsReadRequest->SetURL(FinalStatsURL);
		StatsReadRequest->OnProcessRequestComplete() = ResultDelegate;
		StatsReadRequest->SetVerb(TEXT("GET"));
		StatsReadRequest->SetHeader(TEXT("Cache-Control"), TEXT("no-cache"));

		UE_LOG(LogGameStats, Verbose, TEXT("%s"), *FinalStatsURL);

		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem != NULL && OnlineSubsystem->GetIdentityInterface().IsValid())
		{
			FString AuthToken = OnlineSubsystem->GetIdentityInterface()->GetAuthToken(0);
			StatsReadRequest->SetHeader(TEXT("Authorization"), FString(TEXT("bearer ")) + AuthToken);
		}
		StatsReadRequest->ProcessRequest();
	}
	return StatsReadRequest;
}

void ParseProfileItemJson(const FString& Data, TArray<FProfileItemEntry>& ItemList, int32& XP)
{
	TArray< TSharedPtr<FJsonValue> > StatsJson;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Data);
	if (FJsonSerializer::Deserialize(JsonReader, StatsJson))
	{
		if (StatsJson.Num() == 0)
		{
			// valid response, but user has no stats
			XP = 0;
		}
		else
		{
			// compose a TMap of the items for search efficiency
			TMap< FName, uint32 > StatValueMap;
			for (TSharedPtr<FJsonValue> TestValue : StatsJson)
			{
				if (TestValue->Type == EJson::Object)
				{
					TSharedPtr<FJsonObject> Obj = TestValue->AsObject();
					if (Obj->HasField(TEXT("name")))
					{
						uint32 Value = 0;
						Obj->TryGetNumberField(TEXT("value"), Value);
						StatValueMap.Add(FName(*Obj->GetStringField(TEXT("name"))), Value);
					}
				}
			}

			XP = int32(StatValueMap.FindRef(NAME_PlayerXP));

			ItemList.Reset();

			TArray<FAssetData> AllItems;
			GetAllAssetData(UUTProfileItem::StaticClass(), AllItems, false);
			for (const FAssetData& TestItem : AllItems)
			{
				FName FieldName(*(ITEM_STAT_PREFIX + TestItem.AssetName.ToString()));
				uint32 Value = StatValueMap.FindRef(FieldName);
				if (Value > 0)
				{
					UUTProfileItem* Obj = Cast<UUTProfileItem>(TestItem.GetAsset());
					if (Obj != NULL)
					{
						new(ItemList)FProfileItemEntry(Obj, Value);
					}
				}
			}
		}
	}
}

bool NeedsProfileItem(UObject* TestObj)
{
	UClass* TestCls = Cast<UClass>(TestObj);
	TSubclassOf<AUTCosmetic> CosmeticCls(TestCls);
	TSubclassOf<AUTCharacterContent> CharacterCls(TestCls);
	TSubclassOf<AUTTaunt> TauntCls(TestCls);
	const UUTBotCharacter* BotChar = Cast<UUTBotCharacter>(TestObj);
	return (CosmeticCls != NULL && CosmeticCls.GetDefaultObject()->bRequiresItem) || (CharacterCls != NULL && CharacterCls.GetDefaultObject()->bRequiresItem) || (TauntCls != NULL && TauntCls.GetDefaultObject()->bRequiresItem) || (BotChar != NULL && BotChar->bRequiresItem);
}

void GiveProfileItems(TSharedPtr<const FUniqueNetId> UniqueId, const TArray<FProfileItemEntry>& ItemList)
{
	if (UniqueId.IsValid() && ItemList.Num() > 0)
	{
		TSharedPtr<FJsonObject> StatsJson = MakeShareable(new FJsonObject);
		for (const FProfileItemEntry& Entry : ItemList)
		{
			if (Entry.Item != NULL)
			{
				StatsJson->SetNumberField(ITEM_STAT_PREFIX + Entry.Item->GetName(), Entry.Count);
			}
		}

		FString OutputJsonString;
		TArray<uint8> BackendStatsData;
		TSharedRef< TJsonWriter< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > > Writer = TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&OutputJsonString);
		FJsonSerializer::Serialize(StatsJson.ToSharedRef(), Writer);
		{
			FMemoryWriter MemoryWriter(BackendStatsData);
			MemoryWriter.Serialize(TCHAR_TO_ANSI(*OutputJsonString), OutputJsonString.Len() + 1);
		}

		FString StatsID = UniqueId->ToString();

		FString BaseURL = GetBackendBaseUrl();
		FString CommandURL = TEXT("/api/stats/accountId/");
		FString FinalStatsURL = BaseURL + CommandURL + StatsID + TEXT("/bulk?ownertype=1");

		FHttpRequestPtr StatsWriteRequest = FHttpModule::Get().CreateRequest();
		if (StatsWriteRequest.IsValid())
		{
			StatsWriteRequest->SetURL(FinalStatsURL);
			StatsWriteRequest->SetVerb(TEXT("POST"));
			StatsWriteRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

			if (IOnlineSubsystem::Get()->GetIdentityInterface().IsValid())
			{
				FString AuthToken = IOnlineSubsystem::Get()->GetIdentityInterface()->GetAuthToken(0);
				StatsWriteRequest->SetHeader(TEXT("Authorization"), FString(TEXT("bearer ")) + AuthToken);
			}

			UE_LOG(LogGameStats, VeryVerbose, TEXT("%s"), *OutputJsonString);

			StatsWriteRequest->SetContent(BackendStatsData);
			StatsWriteRequest->ProcessRequest();
		}
	}
}

const TArray<int32>& GetLevelTable()
{
	const int32 MAX_LEVEL = 50;
	const int32 STARTING_INCREMENT = 50;
	const int32 TENTH_LEVEL_INCREMENT_BOOST[] = { 0, 5, 5, 10, 10, 10 };
	checkSlow(MAX_LEVEL < ARRAY_COUNT(TENTH_LEVEL_INCREMENT_BOOST) * 10 - 1);

	// note: req to next level, so element 0 is XP required for level 1
	static TArray<int32> LevelReqs = [&]()
	{
		TArray<int32> Result;
		Result.Add(0);
		int32 Increment = STARTING_INCREMENT;
		int32 Step = STARTING_INCREMENT;
		Result.Add(Step);
		for (int32 i = 2; i < MAX_LEVEL; i++)
		{
			Increment += TENTH_LEVEL_INCREMENT_BOOST[i / 10];
			Step += Increment;
			Result.Add(Result.Last() + Step);
		}
		return Result;
	}();

	return LevelReqs;
}

int32 GetLevelForXP(int32 XPValue)
{
	const TArray<int32>& LevelTable = GetLevelTable();

	for (int32 i = 0; i < LevelTable.Num(); i++)
	{
		if (XPValue < LevelTable[i])
		{
			return i;
		}
	}

	return LevelTable.Num();
}

int32 GetXPForLevel(int32 Level)
{
	const TArray<int32>& LevelTable = GetLevelTable();
	if (Level <= LevelTable.Num() && Level > 0)
	{
		return LevelTable[Level - 1];
	}
	return 0;
}

FText GetBotSkillName(int32 Difficulty)
{
	switch (Difficulty)
	{
		case 0: return NSLOCTEXT("BotSkillLevels", "Novice", "Novice");
		case 1: return NSLOCTEXT("BotSkillLevels", "Average", "Average");
		case 2: return NSLOCTEXT("BotSkillLevels", "Experienced", "Experienced");
		case 3: return NSLOCTEXT("BotSkillLevels", "Skilled", "Skilled");
		case 4: return NSLOCTEXT("BotSkillLevels", "Adept", "Adept");
		case 5: return NSLOCTEXT("BotSkillLevels", "Masterful", "Masterful");
		case 6: return NSLOCTEXT("BotSkillLevels", "Inhuman", "Inhuman");
		case 7: return NSLOCTEXT("BotSkillLevels", "Godlike", "Godlike");
		default:
			return (Difficulty > 7) ? NSLOCTEXT("BotSkillLevels", "Godlike", "Godlike") : NSLOCTEXT("BotSkillLevels", "Broken", "Broken");
	}
}

FString GetMutatorShortName(const FString& inMutatorPath)
{
	int32 PeriodIndex = INDEX_NONE;
	inMutatorPath.FindChar(TCHAR('.'), PeriodIndex);
	if (PeriodIndex != INDEX_NONE)
	{
		FString MutatorName = inMutatorPath.Right(inMutatorPath.Len() - PeriodIndex -1);
		MutatorName = MutatorName.Replace(TEXT("_C"), TEXT(""),ESearchCase::IgnoreCase);
		MutatorName = MutatorName.Replace(TEXT("UTMutator_"), TEXT(""),ESearchCase::IgnoreCase);
		MutatorName = MutatorName.Replace(TEXT("Mutator_"), TEXT(""),ESearchCase::IgnoreCase);

		return MutatorName;
	}
	else
	{
		return inMutatorPath;
	}
}
