// user-selectable content for a player character (mesh, etc)
// this is not done as UTCharacter subclasses for networking reasons (don't want players to be invisible if there is packet loss, loading issues, etc)
// defined as an Actor for the friendly editor tools but is never spawned directly (similar to UTImpactEffect)
// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "UTCharacterVoice.h"
#include "UTCharacterContent.generated.h"

USTRUCT(BlueprintType)
struct FGibSlotInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AUTGib> GibType;
};

UENUM(BlueprintType)
enum EDMSkinType
{
	EDMSkin_Red,
	EDMSkin_Blue,
	EDMSkin_Base,
	EDMSkin_None,
};

UCLASS(BlueprintType, Abstract, NotPlaceable)
class UNREALTOURNAMENT_API AUTCharacterContent : public AActor
{
	GENERATED_BODY()
public:
	friend class AUTCharacter;

	AUTCharacterContent(const FObjectInitializer& OI)
		: Super(OI)
	{
		RootComponent = OI.CreateDefaultSubobject<USceneComponent>(this, FName(TEXT("DummyRoot"))); // needed so Mesh has RelativeLocation/RelativeRotation in the editor
		Mesh = OI.CreateDefaultSubobject<USkeletalMeshComponent>(this, FName(TEXT("Mesh")));
		Mesh->SetupAttachment(RootComponent);
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetCollisionProfileName(FName(TEXT("CharacterMesh")));
		Mesh->bGenerateOverlapEvents = false;
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->bEnablePhysicsOnDedicatedServer = true; // needed for feign death; death ragdoll shouldn't be invoked on server
		Mesh->bReceivesDecals = false;
		DMSkinType = EDMSkin_Red;

		RelativeScale1p = FVector(1.0f, 1.0f, 1.0f);
		RelativeRotation1p = FRotator(0.0f, -90.0f, 0.0f);

		DisplayName = NSLOCTEXT("UT", "UntitledCharacter", "Untitled Character");

		static ConstructorHelpers::FObjectFinder<UClass> GibRef[] = { TEXT("/Game/RestrictedAssets/Blueprints/GibHumanHead.GibHumanHead_C"), 
			TEXT("/Game/RestrictedAssets/Blueprints/GibHumanLegL.GibHumanLegL_C"), TEXT("/Game/RestrictedAssets/Blueprints/GibHumanLegR.GibHumanLegR_C"),
			TEXT("/Game/RestrictedAssets/Blueprints/GibHumanRibs.GibHumanRibs_C"), TEXT("/Game/RestrictedAssets/Blueprints/GibHumanTorso.GibHumanTorso_C"),
			TEXT("/Game/RestrictedAssets/Blueprints/GibHumanArmL.GibHumanArmL_C"), TEXT("/Game/RestrictedAssets/Blueprints/GibHumanArmR.GibHumanArmR_C") };

		new(Gibs) FGibSlotInfo{ FName(TEXT("head")), GibRef[0].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("thigh_l")), GibRef[1].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("thigh_r")), GibRef[2].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("Spine_01")), GibRef[3].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("Spine_02")), GibRef[4].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("lowerarm_l")), GibRef[5].Object };
		new(Gibs) FGibSlotInfo{ FName(TEXT("lowerarm_r")), GibRef[6].Object };
	}

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, AssetRegistrySearchable)
	FText DisplayName;

	/** character gender */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	bool bIsFemale;

	/** if set hide from the menus (i.e. intended for testing or built-in to a specific gametype or mod) */
	UPROPERTY(EditDefaultsOnly)
	bool bHideInUI;

	/** if set a UTProfileItem is required for this character to be available */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Meta = (DisplayName = "Requires Online Item"))
	bool bRequiresItem;
	/** if set this achievement is required for this character to be available
	 * (note: achievements are currently client side only and not validated by server)
	 */
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable, Meta = (DisplayName = "Required Offline Achievement"))
	FName RequiredAchievement;
	/** if set this marketplace entitlement ID is required */
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	FString Entitlement;
	
	inline USkeletalMeshComponent* GetMesh() const
	{
		return Mesh;
	}

	/** mesh to swap in when the character is skeletized */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	USkeletalMesh* SkeletonMesh;

	/** Mesh converted to SkeletonMeshComponent on UTChar to be used for 1p arms */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "1P Skeleton Mesh")
	USkeletalMesh* SkeletonMesh1p;

	/** Scale to apply to SkeletonMesh1p when converted to SkeletonmeshComponent **/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "1P Skeleton Mesh")
	FVector RelativeScale1p;

	/** Rotation to apply to SkeletonMesh1p when converted to SkeletonmeshComponent **/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "1P Skeleton Mesh")
	FRotator RelativeRotation1p;

	/** Material Overrides to apply to SkeletonMesh1p when converted to SkeletonmeshComponent **/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "1P Skeleton Mesh")
	TArray<class UMaterialInterface*> OverideMaterials1p;

	/** Phsyics Asset Override to apply to SkeletonMesh1p when converted to SkeletonmeshComponent **/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "1P Skeleton Mesh")
	class UPhysicsAsset* PhysicsAssetOverride1p;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<FGibSlotInfo> Gibs;

	/** blood explosion played when gibbing */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<class AUTImpactEffect> GibExplosionEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* LandingSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* JumpSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* DodgeSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* PainSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* WallHitSound;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
		USoundBase* DeathSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
		USoundBase* HeadShotDeathSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* FloorSlideSound;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* DrowningSound;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Sounds)
	USoundBase* GaspSound;
	/** Played for character pushing off underwater. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* SwimPushSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* WaterEntrySound;
	/** Played for character entering water fast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* FastWaterEntrySound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* WaterExitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
	FCanvasIcon DefaultCharacterPortrait;

protected:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		TEnumAsByte<EDMSkinType> DMSkinType;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<UMaterialInterface*> TeamMaterials;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<UMaterialInterface*> TeamMaterials1p;

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UUTCharacterVoice> CharacterVoice;


};