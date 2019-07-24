// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Online.h"
#include "ParticleDefinitions.h"

UNREALTOURNAMENT_API DECLARE_LOG_CATEGORY_EXTERN(UT, Log, All);
UNREALTOURNAMENT_API DECLARE_LOG_CATEGORY_EXTERN(UTNet, Log, All);
UNREALTOURNAMENT_API DECLARE_LOG_CATEGORY_EXTERN(UTLoading, Log, All);
UNREALTOURNAMENT_API DECLARE_LOG_CATEGORY_EXTERN(UTConnection, Log, All);

#define COLLISION_PROJECTILE ECC_GameTraceChannel1
#define COLLISION_TRACE_WEAPON ECC_GameTraceChannel2
#define COLLISION_PROJECTILE_SHOOTABLE ECC_GameTraceChannel3
#define COLLISION_TELEPORTING_OBJECT ECC_GameTraceChannel4
#define COLLISION_PAWNOVERLAP ECC_GameTraceChannel5
#define COLLISION_TRACE_WEAPONNOCHARACTER ECC_GameTraceChannel6
#define COLLISION_TRANSDISK ECC_GameTraceChannel7
#define COLLISION_GAMEVOLUME ECC_GameTraceChannel8

#include "UTATypes.h"
#include "UTTeamInterface.h"
#include "UTResetInterface.h"
#include "UTGameplayStatics.h"
#include "UTGameUserSettings.h"
#include "UTTextRenderComponent.h"
#include "UTLocalPlayer.h"
#include "UTBaseGameMode.h"
#include "UTLocalMessage.h"
#include "UTPlayerState.h"
#include "UTCharacter.h"
#include "UTBot.h"
#include "UTTeamInfo.h"
#include "UTGameState.h"
#include "UTHUD.h"
#include "UTHUDWidget.h"
#include "UTDamageType.h"
#include "UTBasePlayerController.h"
#include "UTPlayerController.h"
#include "UTProjectile.h"
#include "UTInventory.h"
#include "UTWeapon.h"
#include "UTPickup.h"
#include "UTGameSession.h"
#include "UTGameObjective.h"
#include "UTCarriedObjectMessage.h"
#include "UTCarriedObject.h"
#include "UTGameMode.h"
#include "UTTeamGameMode.h"
#include "StatManager.h"
#include "OnlineEntitlementsInterface.h"

/** handy response params for world-only checks */
extern FCollisionResponseParams WorldResponseParams;

/** utility to find out if a particle system loops */
extern bool IsLoopingParticleSystem(const UParticleSystem* PSys);

/** utility to detach and unregister a component and all its children */
extern void UnregisterComponentTree(USceneComponent* Comp);

/** utility to retrieve the highest priority physics volume overlapping the passed in primitive */
extern APhysicsVolume* FindPhysicsVolume(UWorld* World, const FVector& TestLoc, const FCollisionShape& Shape);
/** get GravityZ at the given location of the given world */
extern float GetLocationGravityZ(UWorld* World, const FVector& TestLoc, const FCollisionShape& Shape);

/** utility to create a copy of a mesh for outline effect purposes
 * the mesh is initialized but not registered
 */
extern UMeshComponent* CreateCustomDepthOutlineMesh(UMeshComponent* Archetype, AActor* Owner);

/** workaround for FCanvasIcon not having a constructor you can pass in the values to */
FORCEINLINE FCanvasIcon MakeCanvasIcon(UTexture* Tex, float InU, float InV, float InUL, float InVL)
{
	FCanvasIcon Result;
	Result.Texture = Tex;
	Result.U = InU;
	Result.V = InV;
	Result.UL = InUL;
	Result.VL = InVL;
	return Result;
}

/** returns entitlement ID required for the given asset, if any */
extern UNREALTOURNAMENT_API FString GetRequiredEntitlementFromAsset(const FAssetData& Asset);
extern UNREALTOURNAMENT_API FString GetRequiredEntitlementFromObj(UObject* Asset);
extern UNREALTOURNAMENT_API FString GetRequiredEntitlementFromPackageName(FName PackageName);

/** returns whether any locally logged in player (via OSS) has the specified entitlement */
extern UNREALTOURNAMENT_API bool LocallyHasEntitlement(const FString& Entitlement);
/** returns whether any local player has the profile item required to use the specified object (cosmetic, character, etc) */
extern UNREALTOURNAMENT_API bool LocallyOwnsItemFor(const FString& Path);
/** returns whether any local player has the given achievement */
extern UNREALTOURNAMENT_API bool LocallyHasAchievement(FName Achievement);

/** returns asset data for all assets of the specified class 
 * do not use for Blueprints as you can only query for all blueprints period; use GetAllBlueprintAssetData() to query the blueprint's underlying class
 * if bRequireEntitlements is set, assets on disk for which no local player has the required entitlement will not be returned
 *
 * WARNING: the asset registry does a class name search not a path search so the returned assets may not actually be the class you want in the case of name conflicts
 *			if you load any returned assets always verify that you got back what you were expecting!
 */
extern UNREALTOURNAMENT_API void GetAllAssetData(UClass* BaseClass, TArray<FAssetData>& AssetList, bool bRequireEntitlements = true);
/** returns asset data for all blueprints of the specified base class in the asset registry
 * this does not actually load assets, so it's fast in a cooked build, although the first time it is run
 * in an uncooked build it will hitch while scanning the asset registry
 * if bRequireEntitlements is set, assets on disk for which no local player has the required entitlement will not be returned
 */
extern UNREALTOURNAMENT_API void GetAllBlueprintAssetData(UClass* BaseClass, TArray<FAssetData>& AssetList, bool bRequireEntitlements = true);

/** if the passed in package was loaded from a pak file other than the main one returns its filename; otherwise empty string
 * primarily used for mod autodownload/redirects
 */
extern UNREALTOURNAMENT_API FString GetModPakFilenameFromPkg(const FString& PkgName);
extern UNREALTOURNAMENT_API FString GetModPakFilenameFromPath(FString ObjPathName);

/** timer manipulation for UFUNCTIONs that doesn't require a timer handle */
extern UNREALTOURNAMENT_API void SetTimerUFunc(UObject* Obj, FName FuncName, float Time, bool bLooping = false);
extern UNREALTOURNAMENT_API bool IsTimerActiveUFunc(UObject* Obj, FName FuncName, float* TotalTime = NULL, float* ElapsedTime = NULL);
extern UNREALTOURNAMENT_API void ClearTimerUFunc(UObject* Obj, FName FuncName);

/** get the epic launcher app name we're running from */
extern UNREALTOURNAMENT_API const FString GetEpicAppName();
/** get the mcp backend URL */
extern UNREALTOURNAMENT_API const FString GetBackendBaseUrl();

/** reads stats data for a user and calls the delegate when done
 * NOTE: stats data currently also contains profile item counters!
 * @param StatsId - user ID to query
 * @param QueryWindow - query time period (e.g. "monthly")
 */
extern UNREALTOURNAMENT_API FHttpRequestPtr ReadBackendStats(const FHttpRequestCompleteDelegate& ResultDelegate, const FString& StatsId, const FString& QueryWindow = TEXT("alltime"));

/** reads profile item json and fills in the items array
 * array is emptied first
 */
extern UNREALTOURNAMENT_API void ParseProfileItemJson(const FString& Data, TArray<struct FProfileItemEntry>& ItemList, int32& XP);
/** returns whether the given object requires an inventory item to grant rights to it */
extern UNREALTOURNAMENT_API bool NeedsProfileItem(UObject* TestObj);
/** sends backend request to give item(s) to a player */
extern UNREALTOURNAMENT_API void GiveProfileItems(TSharedPtr<const FUniqueNetId> UniqueId, const TArray<FProfileItemEntry>& ItemList);

/** prefix for stat names for our hacky "inventory as stats" implementation */
extern const FString ITEM_STAT_PREFIX;

/** Gets the XP requirements for each level */
extern UNREALTOURNAMENT_API const TArray<int32>& GetLevelTable();

/** looks up XP in level table */
extern UNREALTOURNAMENT_API int32 GetLevelForXP(int32 XPValue);

/** get the xp needed for this level */
extern UNREALTOURNAMENT_API int32 GetXPForLevel(int32 Level);

/** @return localized name for a bot skil level */
extern UNREALTOURNAMENT_API FText GetBotSkillName(int32 Difficulty);

extern UNREALTOURNAMENT_API FString GetMutatorShortName(const FString& inMutatorPath);

