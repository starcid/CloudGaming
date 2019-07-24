// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTImpactEffect.h"
#include "UTWorldSettings.h"
#include "Particles/ParticleSystemComponent.h"

FImpactEffectNamedParameters::FImpactEffectNamedParameters(float DamageRadius)
{
	static FName NAME_DamageRadiusScalar(TEXT("DamageRadiusScalar"));
	FParticleSysParam* Param = new(ParticleParams) FParticleSysParam;
	Param->Name = NAME_DamageRadiusScalar;
	Param->ParamType = EParticleSysParamType::PSPT_Scalar;
	Param->Scalar = DamageRadius;

	static FName NAME_DamageRadiusVector(TEXT("DamageRadiusVector"));
	Param = new(ParticleParams)FParticleSysParam;
	Param->Name = NAME_DamageRadiusVector;
	Param->ParamType = EParticleSysParamType::PSPT_Vector;
	Param->Vector = FVector(DamageRadius, DamageRadius, DamageRadius);
}

AUTImpactEffect::AUTImpactEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCheckInView = true;
	bForceForLocalPlayer = true;
	bRandomizeDecalRoll = true;
	AlwaysSpawnDistance = 500.0f;
	CullDistance = 20000.0f;
	DecalLifeScaling = 1.f;
	bCanBeDamaged = false;
	AudioSAT = SAT_WeaponFire;
}

bool AUTImpactEffect::SpawnEffect_Implementation(UWorld* World, const FTransform& InTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, ESoundReplicationType SoundReplication, const FImpactEffectNamedParameters& EffectParams) const
{
	if (World == NULL)
	{
		return false;
	}
	else
	{
		UUTGameplayStatics::UTPlaySound(World, Audio, SpawnedBy, SoundReplication, false, InTransform.GetLocation(), NULL, NULL, false, AudioSAT);

		if (World->GetNetMode() == NM_DedicatedServer)
		{
			return false;
		}
		else
		{
			bool bSpawn = true;
			if (bCheckInView)
			{
				AUTWorldSettings* WS = Cast<AUTWorldSettings>(World->GetWorldSettings());
				bSpawn = (WS == NULL || WS->EffectIsRelevant(SpawnedBy, InTransform.GetLocation(), SpawnedBy != NULL, bForceForLocalPlayer && Cast<APlayerController>(InstigatedBy), CullDistance, AlwaysSpawnDistance, false));
			}
			if (bSpawn)
			{
				// create components
				TArray<USceneComponent*> NativeCompList;
				GetComponents<USceneComponent>(NativeCompList);
				TArray<USCS_Node*> ConstructionNodes;
				{
					TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
					UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(GetClass(), ParentBPClassStack);
					for (int32 i = ParentBPClassStack.Num() - 1; i >= 0; i--)
					{
						const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[i];
						if (CurrentBPGClass->SimpleConstructionScript)
						{
							ConstructionNodes += CurrentBPGClass->SimpleConstructionScript->GetAllNodes();
						}
					}
				}
				CreateEffectComponents(World, InTransform, HitComp, SpawnedBy, InstigatedBy, EffectParams, bAttachToHitComp ? HitComp : NULL, NAME_None, NativeCompList, ConstructionNodes);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

void AUTImpactEffect::CallSpawnEffect(UObject* WorldContextObject, TSubclassOf<AUTImpactEffect> Effect, const FTransform& InTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, ESoundReplicationType SoundReplication, const FImpactEffectNamedParameters& EffectParams)
{
	if (WorldContextObject == NULL)
	{
		UE_LOG(UT, Warning, TEXT("SpawnEffect(): No world context"));
	}
	else if (Effect == NULL)
	{
		UE_LOG(UT, Warning, TEXT("SpawnEffect(): No effect specified"));
	}
	else
	{
		Effect.GetDefaultObject()->SpawnEffect(WorldContextObject->GetWorld(), InTransform, HitComp, SpawnedBy, InstigatedBy, SoundReplication);
	}
}

bool AUTImpactEffect::ShouldCreateComponent_Implementation(const USceneComponent* TestComp, FName CompTemplateName, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy) const
{
	if (HitComp != NULL && TestComp->IsA(UDecalComponent::StaticClass()) && (!HitComp->bReceivesDecals || !HitComp->ShouldRender()) && !HitComp->IsA(UBrushComponent::StaticClass())) // special case to attach to blocking volumes to project on world geo behind it
	{
		return false;
	}
	else
	{
		return true;
	}
}

void AUTImpactEffect::ComponentCreated_Implementation(USceneComponent* NewComp, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, FImpactEffectNamedParameters EffectParams) const
{
	UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
	if (Prim != NULL)
	{
		if (WorldTimeParam != NAME_None)
		{
			for (int32 i = Prim->GetNumMaterials() - 1; i >= 0; i--)
			{
				if (Prim->GetMaterial(i) != NULL)
				{
					UMaterialInstanceDynamic* MID = Prim->CreateAndSetMaterialInstanceDynamic(i);
					MID->SetScalarParameterValue(WorldTimeParam, Prim->GetWorld()->TimeSeconds);
				}
			}
		}
		if (Prim->IsA(UParticleSystemComponent::StaticClass()))
		{
			((UParticleSystemComponent*)Prim)->bAutoDestroy = true;
			((UParticleSystemComponent*)Prim)->SecondsBeforeInactive = 0.0f;
			((UParticleSystemComponent*)Prim)->InstanceParameters += EffectParams.ParticleParams;

			UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
			Scalability::FQualityLevels QualitySettings = UserSettings->ScalabilityQuality;
			if ((QualitySettings.EffectsQuality < 2) && !Cast<APlayerController>(InstigatedBy))
			{
				for (int32 Idx = 0; Idx < ((UParticleSystemComponent*)Prim)->EmitterInstances.Num(); Idx++)
				{
					if (((UParticleSystemComponent*)Prim)->EmitterInstances[Idx])
					{
						((UParticleSystemComponent*)Prim)->EmitterInstances[Idx]->LightDataOffset = 0;
					}
				}
			}
		}
		else if (Prim->IsA(UAudioComponent::StaticClass()))
		{
			((UAudioComponent*)Prim)->bAutoDestroy = true;
		}
	}
	else
	{
		UDecalComponent* Decal = Cast<UDecalComponent>(NewComp);
		if (Decal != NULL)
		{
			if (bRandomizeDecalRoll)
			{
				Decal->RelativeRotation.Roll += 360.0f * FMath::FRand();
			}
			if (HitComp != NULL && HitComp->Mobility == EComponentMobility::Movable)
			{
				Decal->bAbsoluteScale = true;
				Decal->AttachToComponent(HitComp, FAttachmentTransformRules::KeepWorldTransform);
			}
			Decal->UpdateComponentToWorld();
		}
		else
		{
			ULightComponent* Light = Cast<ULightComponent>(NewComp);
			if (Light && Light->CastShadows)
			{
				UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
				Scalability::FQualityLevels QualitySettings = UserSettings->ScalabilityQuality;
				if (QualitySettings.EffectsQuality < 3)
				{
					Light->SetCastShadows(false);
					Light->bAffectTranslucentLighting = false;
				}
		/*		else if (Light->bAffectTranslucentLighting)
				{
					UE_LOG(UT, Warning, TEXT("%s Light affects translucent!"), *GetName());
				}*/
			}
		}
	}
}

void AUTImpactEffect::CreateEffectComponents(UWorld* World, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, const FImpactEffectNamedParameters& EffectParams, USceneComponent* CurrentAttachment, FName TemplateName, const TArray<USceneComponent*>& NativeCompList, const TArray<USCS_Node*>& BPNodes) const
{
	AUTWorldSettings* WS = Cast<AUTWorldSettings>(World->GetWorldSettings());
	for (int32 i = 0; i < NativeCompList.Num(); i++)
	{
		if (NativeCompList[i]->GetAttachParent() == CurrentAttachment && ShouldCreateComponent(NativeCompList[i], NativeCompList[i]->GetFName(), BaseTransform, HitComp, SpawnedBy, InstigatedBy))
		{
			USceneComponent* NewComp = NewObject<USceneComponent>(World, NativeCompList[i]->GetClass(), NAME_None, RF_NoFlags, NativeCompList[i]);
			NewComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			//PLK - HOPEFULLY THIS ISN't BROKEN DUE TO THE PRIVATIZATION OF ATTACHCHILDREN
			//NewComp->AttachChildren.Empty();
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
			if (Prim != NULL)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			if (CurrentAttachment != NULL)
			{
				NewComp->AttachToComponent(CurrentAttachment, FAttachmentTransformRules::KeepRelativeTransform, NewComp->GetAttachSocketName());
			}
			if (CurrentAttachment == NULL || CurrentAttachment == HitComp)
			{
				NewComp->SetWorldTransform(FTransform(NewComp->RelativeRotation, NewComp->RelativeLocation, NewComp->RelativeScale3D) * BaseTransform);
			}
			NewComp->RegisterComponentWithWorld(World);
			if (bNoLODForLocalPlayer)
			{
				SetNoLocalPlayerLOD(World, NewComp, InstigatedBy);
			}
			ComponentCreated(NewComp, HitComp, SpawnedBy, InstigatedBy, EffectParams);
			if (WS != NULL)
			{
				WS->AddImpactEffect(NewComp, DecalLifeScaling);
			}
			// recurse
			CreateEffectComponents(World, BaseTransform, HitComp, SpawnedBy, InstigatedBy, EffectParams, NewComp, NativeCompList[i]->GetFName(), NativeCompList, BPNodes);
		}
	}
	for (int32 i = 0; i < BPNodes.Num(); i++)
	{
		USceneComponent* ComponentTemplate = Cast<USceneComponent>(BPNodes[i]->GetActualComponentTemplate(Cast<UBlueprintGeneratedClass>(GetClass())));
		if (ComponentTemplate != NULL && BPNodes[i]->ParentComponentOrVariableName == TemplateName && ShouldCreateComponent(ComponentTemplate, TemplateName, BaseTransform, HitComp, SpawnedBy, InstigatedBy))
		{
			USceneComponent* NewComp = NewObject<USceneComponent>(World, ComponentTemplate->GetClass(), NAME_None, RF_NoFlags, ComponentTemplate);
			NewComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			//PLK - HOPEFULLY THIS ISN't BROKEN DUE TO THE PRIVATIZATION OF ATTACHCHILDREN
			//NewComp->AttachChildren.Empty();
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
			if (Prim != NULL)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			if (CurrentAttachment != NULL)
			{
				NewComp->AttachToComponent(CurrentAttachment, FAttachmentTransformRules::KeepRelativeTransform, BPNodes[i]->AttachToName);
			}
			if (CurrentAttachment == NULL || CurrentAttachment == HitComp)
			{
				NewComp->SetWorldTransform(FTransform(NewComp->RelativeRotation, NewComp->RelativeLocation, NewComp->RelativeScale3D) * BaseTransform);
			}
			NewComp->RegisterComponentWithWorld(World);
			if (bNoLODForLocalPlayer)
			{
				SetNoLocalPlayerLOD(World, NewComp, InstigatedBy);
			}
			ComponentCreated(NewComp, HitComp, SpawnedBy, InstigatedBy, EffectParams);
			if (WS != NULL)
			{
				WS->AddImpactEffect(NewComp, DecalLifeScaling);
			}
			// recurse
			CreateEffectComponents(World, BaseTransform, HitComp, SpawnedBy, InstigatedBy, EffectParams, NewComp, BPNodes[i]->GetVariableName(), NativeCompList, BPNodes);
		}
	}
}

void AUTImpactEffect::SetNoLocalPlayerLOD(UWorld* World, USceneComponent* NewComp, AController* InstigatedBy) const
{
	if (InstigatedBy != NULL && InstigatedBy->IsLocalPlayerController())
	{
		// see if this is a particle system, if so switch to direct LOD
		UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(NewComp);
		if (PSC)
		{
			PSC->SetLODLevel(0);
		}
	}
}
