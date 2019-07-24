// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HAL/IConsoleManager.h"
#include "Engine/MaterialMerging.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "LODActor.generated.h"

class UStaticMesh;

/**
 * LODActor is an instance of an autogenerated StaticMesh Actors by Hierarchical LOD System
 * This is essentially just StaticMeshActor that you can't move or edit, but it contains multiple actors reference
 *
 * @see https://docs.unrealengine.com/latest/INT/Engine/Actors/LODActor/
 * @see UStaticMesh
 */

UCLASS(notplaceable, hidecategories = (Object, Collision, Display, Input, Blueprint, Transform, Physics))
class ENGINE_API ALODActor : public AActor
{
GENERATED_UCLASS_BODY()

private_subobject:
	// disable display of this component
	UPROPERTY(Category=LODActor, VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComponent;

public:
	UPROPERTY(Category=LODActor, VisibleAnywhere)
	TArray<AActor*> SubActors;

	/** what distance do you want this to show up instead of SubActors */
	UPROPERTY()
	float LODDrawDistance;
	
	/** The hierarchy level of this actor; the first tier of HLOD is level 1, the second tier is level 2 and so on. */
	UPROPERTY(Category=LODActor, VisibleAnywhere)
	int32 LODLevel;

	/** assets that were created for this, so that we can delete them */
	UPROPERTY(Category=LODActor, VisibleAnywhere)
	TArray<UObject*> SubObjects;

	//~ Begin AActor Interface
#if WITH_EDITOR
	virtual void CheckForErrors() override;
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const override;
	virtual void EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	virtual void EditorApplyMirror(const FVector& MirrorScale, const FVector& PivotLocation) override;
#endif // WITH_EDITOR	
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false) const override;
	virtual void PostRegisterAllComponents() override;
	//~ End AActor Interface

	/** Sets StaticMesh and IsPreviewActor to true if InStaticMesh equals nullptr */
	void SetStaticMesh(UStaticMesh* InStaticMesh);

	const bool IsBuilt() { return StaticMeshComponent->GetStaticMesh() != nullptr;  }

#if WITH_EDITOR
	/**
	* Adds InAcor to the SubActors array and set its LODParent to this
	* @param InActor - Actor to add
	*/
	void AddSubActor(AActor* InActor);

	/**
	* Removes InActor from the SubActors array and sets its LODParent to nullptr
	* @param InActor - Actor to remove
	*/
	const bool RemoveSubActor(AActor* InActor);

	/**
	* Returns whether or not this LODActor is dirty
	* @return const bool
	*/
	const bool IsDirty() const { return bDirty; }

	/**
	* Sets whether or not this LODActor is dirty and should have its LODMesh (re)build
	* @param bNewState - New dirty state
	*/
	void SetIsDirty(const bool bNewState);
	
	/**
	 * Determines whether or not this LODActor has valid SubActors and can be built
	 * @return true if the subactor(s) contain at least two static mesh components
	 */
	const bool HasValidSubActors() const;

	/**
	 * Determines whether or not this LODActor has any SubActors
	 * @return true if it contains any subactors
	 */
	const bool HasAnySubActors() const;

	/** Toggles forcing the StaticMeshComponent drawing distance to 0 or LODDrawDistance */
	void ToggleForceView();

	/** Sets forcing the StaticMeshComponent drawing distance to 0 or LODDrawDistance according to InState*/
	void SetForcedView(const bool InState);

	/** Sets the state of whether or not this LODActor is hidden from the Editor view, used for forcing a HLOD to show*/
	void SetHiddenFromEditorView(const bool InState, const int32 ForceLODLevel);

	/** Returns the number of triangles this LODActor's SubActors contain */
	const uint32 GetNumTrianglesInSubActors();

	/** Returns the number of triangles this LODActor's SubActors contain */
	const uint32 GetNumTrianglesInMergedMesh();
	
	/** Updates the LODParents for the SubActors (and the drawing distance)*/
	void UpdateSubActorLODParents();

	/** Cleans the SubActor array (removes all NULL entries) */
	void CleanSubActorArray();

	/** Cleans the SubObject array (removes all NULL entries) */
	void CleanSubObjectsArray();

	/** Recalculates the drawing distance according to a fixed FOV of 90 and the transition screen size*/
	void RecalculateDrawingDistance(const float TransitionScreenSize);
#endif // WITH_EDITOR
	
	//~ Begin UObject Interface.
	virtual FString GetDetailedInfoInternal() const override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void Serialize(FArchive& Ar) override;
#endif // WITH_EDITOR	
	//~ End UObject Interface.		
protected:
#if WITH_EDITORONLY_DATA
	/** Whether or not this LODActor is not build or needs rebuilding */
	UPROPERTY()
	bool bDirty;
#endif // WITH_EDITORONLY_DATA
public:
#if WITH_EDITORONLY_DATA
	/** Cached number of triangles contained in the SubActors*/
	UPROPERTY()
	uint32 NumTrianglesInSubActors;

	/** Cached number of triangles contained in the SubActors*/
	UPROPERTY()
	uint32 NumTrianglesInMergedMesh;

	/** Flag whether or not to use the override MaterialSettings when creating the proxy mesh */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings)
	bool bOverrideMaterialMergeSettings;

	/** Override Material Settings, used when creating the proxy mesh */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings, meta = (editcondition = "bOverrideMaterialMergeSettings"))
	FMaterialProxySettings MaterialSettings;

	/** Flag whether or not to use the override TransitionScreenSize for this proxy mesh */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings)
	bool bOverrideTransitionScreenSize;

	/** 
	 * Override transition screen size value, determines the screen size at which the proxy is visible 
	 * The screen size is based around the projected diameter of the bounding 
	 * sphere of the model. i.e. 0.5 means half the screen's maximum dimension.
	 */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings, meta = (editcondition = "bOverrideTransitionScreenSize"))
	float TransitionScreenSize;

	/** Flag whether or not to use the override ScreenSize when creating the proxy mesh */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings)
	bool bOverrideScreenSize;

	/** Override screen size value used in mesh reduction, when creating the proxy mesh */
	UPROPERTY(EditAnywhere, Category = HierarchicalLODSettings, meta = (editcondition = "bOverrideScreenSize"))
	int32 ScreenSize;
#endif // WITH_EDITORONLY_DATA

	/** Returns StaticMeshComponent subobject **/
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

private:
	// Called when CVars are changed to check to see if the maximum HLOD level value has changed
	static void OnCVarsChanged();

	// Called to make sure autoregistration/manual registration state matches based on the LOD override cvar and this actor's lod level
	void UpdateRegistrationToMatchMaximumLODLevel();

	// This will determine the shadowing flags for the static mesh component according to all sub actors
	void DetermineShadowingFlags();

private:
 	// Have we already tried to register components? (a cache to avoid having to query the owning world when the global HLOD max level setting is changed)
 	uint8 bHasActorTriedToRegisterComponents : 1;

	/**
	 * If true on post load we need to calculate resolution independent Display Factors from the
	 * loaded LOD screen sizes.
	 */
	uint8 bRequiresLODScreenSizeConversion : 1;

	// Sink for when CVars are changed to check to see if the maximum HLOD level value has changed
	static FAutoConsoleVariableSink CVarSink;
}; 
