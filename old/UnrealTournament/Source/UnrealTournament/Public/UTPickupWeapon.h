// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPickupInventory.h"
#include "UTWeapon.h"
#include "UTInventory.h"

#include "UTPickupWeapon.generated.h"

USTRUCT(BlueprintType)
struct FWeaponPickupCustomer
{
	GENERATED_USTRUCT_BODY()

	/** the pawn that picked it up */
	UPROPERTY(BlueprintReadWrite, Category = Customer)
	APawn* P;
	/** next time pickup is allowed */
	UPROPERTY(BlueprintReadWrite, Category = Customer)
	float NextPickupTime;

	FWeaponPickupCustomer()
	{}
	FWeaponPickupCustomer(APawn* InP, float InPickupTime)
		: P(InP), NextPickupTime(InPickupTime)
	{}
};

UCLASS(Blueprintable, CustomConstructor, HideCategories=(Inventory, Pickup))
class UNREALTOURNAMENT_API AUTPickupWeapon : public AUTPickupInventory
{
	GENERATED_UCLASS_BODY()

	AUTPickupWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{
		Collision->InitCapsuleSize(78.0f, 80.0f);
		RespawnTime = 20.0f;
		NextPickupTime = 30.f;
	}

protected:
	/** copy of mesh displayed when inventory is not available - performs depth testing for GhostMesh
	 * for normal pickups this is done by changing the main Mesh when taken but that can't be used for weapons with WeaponStay on
	 * as the meshes need to have per-player visibility settings
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = Pickup)
	UMeshComponent* GhostDepthMesh;

public:
	inline const UMeshComponent* GetGhostDepthMesh() const
	{
		return GhostDepthMesh;
	}

	/** weapon type that can be picked up here */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<AUTWeapon> WeaponType;

	/** list of characters that have picked up this weapon recently, used when weapon stay is on to avoid repeats */
	UPROPERTY(BlueprintReadWrite, Category = PickupWeapon)
	TArray<FWeaponPickupCustomer> Customers;

	/** Next pickup time with weapon stay */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
		float NextPickupTime;

	virtual bool IsTaken(APawn* TestPawn) override;
	virtual void AddHiddenComponents(bool bTaken, TSet<FPrimitiveComponentId>& HiddenComponents) override
	{
		Super::AddHiddenComponents(bTaken, HiddenComponents);
		if (!bTaken)
		{
			if (GetGhostDepthMesh() != NULL)
			{
				HiddenComponents.Add(GetGhostDepthMesh()->ComponentId);
			}
		}
	}

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetInventoryType(TSubclassOf<AUTInventory> NewType) override;
	virtual void InventoryTypeUpdated_Implementation() override;
	virtual float GetNextPickupTime() override;

	virtual void ProcessTouch_Implementation(APawn* TouchedBy) override;

	FTimerHandle CheckTouchingHandle;

	/** checks for anyone touching the pickup and checks if they should get the item
	 * this is necessary because this type of pickup doesn't toggle collision when weapon stay is on
	 */
	void CheckTouching();

	/** called to do clientside simulated handling of weapon pickups with weapon stay on */
	virtual void LocalPickupHandling(APawn* TouchedBy);

	virtual void SetPickupHidden(bool bNowHidden) override;
	virtual void PlayTakenEffects(bool bReplicate) override;

	virtual float BotDesireability_Implementation(APawn* Asker, AController* RequestOwner, float TotalDistance) override
	{
		return (IsTaken(Asker) ? 0.0f : Super::BotDesireability_Implementation(Asker, RequestOwner, TotalDistance));
	}

	virtual float GetRespawnTimeOffset(APawn* Asker) const override;

#if WITH_EDITOR
	virtual void CreateEditorPickupMesh() override
	{
		if (GetWorld() != NULL && GetWorld()->WorldType == EWorldType::Editor)
		{
			CreatePickupMesh(this, EditorMesh, WeaponType, FloatHeight, RotationOffset, false);
			if (EditorMesh != NULL)
			{
				EditorMesh->SetHiddenInGame(true);
			}
		}
	}
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};