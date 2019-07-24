// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Kismet/KismetSystemLibrary.h"
#include "UTGameInstance.h"

#include "UTGameplayStatics.generated.h"

UENUM()
enum ESoundReplicationType
{
	SRT_All, // replicate to all in audible range
	SRT_AllButOwner, // replicate to all but the owner of SourceActor
	SRT_IfSourceNotReplicated, // only replicate to clients on which SourceActor does not exist
	SRT_None, // no replication; local only
	SRT_MAX
};

UENUM()
enum ESoundAmplificationType
{
	SAT_None, 
	SAT_Footstep, 
	SAT_WeaponFire, 
	SAT_WeaponFoley,
	SAT_PainSound,
	SAT_MAX
};

UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UUTGameplayStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{}

	/** plays a sound with optional replication parameters
	* additionally will check that clients will actually be able to hear the sound (don't replicate if out of sound's audible range)
	* if called on client, always local only
	* @param AmpedListener - amplify volume for this listener using the Target amplification settings of the SoundAmplificationType
	* @param Instigator - Pawn that caused the sound to be played (if any) - if SourceActor is a Pawn it defaults to that
	* @param bNotifyAI - whether AI can hear this sound (subject to sound radius and bot skill)
	*/
	UFUNCTION(BlueprintCallable, Category = Sound, meta = (HidePin = "TheWorld", DefaultToSelf = "SourceActor", AutoCreateRefTerm = "SoundLoc"))
	static void UTPlaySound(UWorld* TheWorld, USoundBase* TheSound, AActor* SourceActor = NULL, ESoundReplicationType RepType = SRT_All, bool bStopWhenOwnerDestroyed = false, const FVector& SoundLoc = FVector::ZeroVector, class AUTPlayerController* AmpedListener = NULL, APawn* Instigator = NULL, bool bNotifyAI = true, ESoundAmplificationType AmpType = SAT_None);

	/** retrieves gravity; if no location is specified, level default gravity is returned */
	UFUNCTION(BlueprintCallable, Category = World, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", AutoCreateRefTerm = "TestLoc"))
	static float GetGravityZ(UObject* WorldContextObject, const FVector& TestLoc = FVector::ZeroVector);

	/** Hurt locally authoritative actors within the radius. Uses the Weapon trace channel.
	 * Also allows passing in momentum (instead of using value hardcoded in damage type - allows for gameplay code to scale, e.g. for a charging weapon)
	* @param BaseDamage - The base damage to apply, i.e. the damage at the origin.
	* @param MinimumDamage - Minimum damage (at max radius)
	* @param BaseMomentumMag - The base momentum (impulse) to apply, scaled the same way damage is and oriented from Origin to the surface of the hit object
	* @param Origin - Epicenter of the damage area.
	* @param DamageInnerRadius - Radius of the full damage area, from Origin
	* @param DamageOuterRadius - Radius of the minimum damage area, from Origin
	* @param DamageFalloff - Falloff exponent of damage from DamageInnerRadius to DamageOuterRadius
	* @param DamageTypeClass - Class that describes the damage that was done.
	* @param IgnoreActors - Actors to never hit; these Actors also don't block the trace that makes sure targets aren't behind a wall, etc. DamageCauser is automatically in this list.
	* @param DamageCauser - Actor that actually caused the damage (e.g. the grenade that exploded)
	* @param InstigatedByController - Controller that was responsible for causing this damage (e.g. player who threw the grenade)
	* @param FFInstigatedBy (optional) - Controller that gets credit for damage to enemies on the same team as InstigatedByController (including damaging itself)
	*									this is used to grant two way kill credit for mechanics where the opposition is partially responsible for the damage (e.g. blowing up an enemy's projectile in flight)
	* @param FFDamageType (optional) - when FFInstigatedBy is assigned for damage credit, optionally also use this damage type instead of the default (primarily for death message clarity)
	* @param CollisionFreeRadius (optional) - allow damage to be dealt inside this radius even if visibility checks would fail
	* @param AltVisibilityOrigins (optional) - list of alternate locations to also check for visibility to affected Actors. If any visibility origin succeeds, the target takes damage.
	* @return true if damage was applied to at least one actor.
	*/
	UFUNCTION(BlueprintCallable, Category = "Game|Damage", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", AutoCreateRefTerm = "IgnoreActors"))
	static bool UTHurtRadius( UObject* WorldContextObject, float BaseDamage, float MinimumDamage, float BaseMomentumMag, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff,
								TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, AController* FFInstigatedBy = NULL, TSubclassOf<UDamageType> FFDamageType = NULL, float CollisionFreeRadius = 0.0f
#if CPP // hack: construct not supported by UHT
								, const TArray<FVector>* AltVisibilityOrigins = NULL
#endif
								);

	static bool ComponentIsVisibleFrom(UPrimitiveComponent* VictimComp, FVector const& Origin, AActor const* IgnoredActor, FHitResult& OutHitResult, const TArray<FVector>* AltVisibilityOrigins);

// DEPRECATED - use ChooseBestAimTarget()
	UFUNCTION(meta=(DeprecatedFunction, DeprecationMessage = "Use ChooseBestAimTarget"),BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Targeting")
	static APawn* PickBestAimTarget(AController* AskingC, FVector StartLoc, FVector FireDir, float MinAim, float MaxRange, TSubclassOf<APawn> TargetClass = NULL
#if CPP // hack: UHT doesn't support this (or any 'optional out' type construct)
	, float* BestAim = NULL, float* BestDist = NULL
#endif
	);

	/** select visible controlled enemy Pawn for which direction from StartLoc is closest to FireDir and within aiming cone/distance constraints
	* commonly used for autoaim, homing locks, etc
	* @param AskingC - Controller that is looking for a target; may not be NULL
	* @param StartLoc - start location of fire (instant hit trace start, projectile spawn loc, etc)
	* @param FireDir - fire direction
	* @param MinAim - minimum dot product of directions that can be returned (maximum 0)
	* @param MaxRange - maximum range to search
	* @param TargetClass - optional subclass of Pawn to look for; default is all pawns
	* @param BestAim - if specified, filled with actual dot product of returned target
	* @param BestDist - if specified, filled with actual distance to returned target
	* @param BestOffset - if specified, filled with actual distance offset from aim vector to returned target
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Targeting")
		static APawn* ChooseBestAimTarget(AController* AskingC, FVector StartLoc, FVector FireDir, float MinAim, float MaxRange, float MaxOffsetDist, TSubclassOf<APawn> TargetClass = NULL
#if CPP // hack: UHT doesn't support this (or any 'optional out' type construct)
			, float* BestAim = NULL, float* BestDist = NULL, float* BestOffset = NULL
#endif
		);
	/** alternative, more robust version of SuggestProjectileVelocity()
	 * in particular, this handles that targets generally have collision size and hitting the exact target is not required
	 * so the simplistic quadratic equation solution will often result in poor or no valid toss vectors (depending on collision geometry)
	 * when there are alternative arcs that will get close enough
	 * note, however, that this routine is slower than the engine implementation
	 * @param TossVelocity - (output) Result launch velocity.
	 * @param StartLoc - Intended launch location
	 * @param EndLoc - target location
	 * @param TargetActor - target actor (NULL is valid)
	 * @param ZOvershootTolerance - amount an alternate arc can go over (higher than) the intended target and still be considered valid (e.g. target height)
	 *								note that this value only applies OVER the intended target - no returned arc will ever undershoot; adjust your EndLocation to the bottom of the target's collision if necessary
	 * @param TossSpeed - Launch speed
	 * @param CollisionRadius - Radius of the projectile (assumed spherical), used when tracing
	 * @param OverrideGravityZ - Optional gravity override.  0 means "do not override".
	 * @param MaxSubdivisions - maximum number of subdivisions between the low angle (minimum possible) and high angle (maximum possible) to test for being within the tolerance value. Substantially affects performance.
	 * @param TraceOption - Controls whether or not to validate a clear path by tracing along the calculated arc. NOTE: if you pass DoNotTrace this function becomes identical to the engine implementation, since the low arc will never fail
	 * @return whether an acceptable solution was found and placed in TossVelocity. The lowest valid arc is always the one returned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ProjectileMovement")
	static bool UTSuggestProjectileVelocity(UObject* WorldContextObject, FVector& TossVelocity, const FVector& StartLoc, const FVector& EndLoc, AActor* TargetActor, float ZOvershootTolerance, float TossSpeed, float CollisionRadius = 0.f, float OverrideGravityZ = 0.0f, int32 MaxSubdivisions = 5, ESuggestProjVelocityTraceOption::Type TraceOption = ESuggestProjVelocityTraceOption::TraceFullPath);

	/** returns PlayerController at the specified player index */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Player", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	APlayerController* GetLocalPlayerController(UObject* WorldContextObject, int32 PlayerIndex = 0);

	/** saves the config properties to the .ini file
	 * if you pass a Class, saves the values in its default object
	 * if you pass anything else, the instance's values are saved as its class's new defaults
	 */
	UFUNCTION(BlueprintCallable, Category = "Config", meta = (DisplayName = "SaveConfig"))
	static void K2_SaveConfig(UObject* Obj);

	UFUNCTION(BlueprintCallable, Category = "UT")
	static bool IsForcingSingleSampleShadowFromStationaryLights();
	
	/** Not replicated. Plays a sound cue on an actor, sound wave may change depending on team affiliation compared to the listener */
	UFUNCTION(BlueprintCosmetic, BlueprintCallable, Category = "UT", meta = (DefaultToSelf = "SoundTarget"))
	static class UAudioComponent* PlaySoundTeamAdjusted(USoundCue* SoundToPlay, AActor* SoundInstigator, AActor* SoundTarget, bool Attached);

	UFUNCTION(BlueprintCosmetic, BlueprintCallable, Category = "UT")
	static void AssignTeamAdjustmentValue(UAudioComponent* AudioComponent, AActor* SoundInstigator);

	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static bool HasTokenBeenPickedUpBefore(UObject* WorldContextObject, FName TokenUniqueID);

	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static int32 HowManyTokensHaveBeenPickedUpBefore(UObject* WorldContextObject, TArray<FName> TokenUniqueIDs);

	/** Token pick up noted in temporary storage, not committed to profile storage until TokenCommit called */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void TokenPickedUp(UObject* WorldContextObject, FName TokenUniqueID);

	/** Remove a token pick up from temporary storage so it won't get committed to profile storage */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void TokenRevoke(UObject* WorldContextObject, FName TokenUniqueID);

	/** Save tokens picked up this level to profile */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void TokensCommit(UObject* WorldContextObject);

	/** Reset tokens picked up this level so they don't get saved to profile */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void TokensReset(UObject* WorldContextObject);
		
	/** Set a best time for a named timing section */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void SetBestTime(UObject* WorldContextObject, FName TimingSection, float InBestTime);
	
	/** Get the best time for a named timing section */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static bool GetBestTime(UObject* WorldContextObject, FName TimingSection, float& OutBestTime);

	/**
	* Simplified line trace node that returns only hit data, for simplicitly in blueprints when the full HitResult is not needed.
	* Does a collision trace along the given line and returns the first hit encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param Start Start of line segment.
	* @param End End of line segment.
	* @param ObjectTypes Array of Object Types to trace
	* @param bTraceComplex True to test against complex collision, false to test against simplified collision.
	* @return True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", DisplayName = "LineTraceForObjectsSimple", Keywords = "raycast"))
	static bool LineTraceForObjectsSimple(UObject* WorldContextObject, const FVector Start, const FVector End, const TArray< TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, EDrawDebugTrace::Type DrawDebugType, FVector& HitLocation, FVector& HitNormal, bool bIgnoreSelf);

	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (WorldContext = "WorldContextObject"))
	static bool LineTraceForWorldBlockingOnly(UObject* WorldContextObject, const FVector Start, const FVector End, EDrawDebugTrace::Type DrawDebugType, FVector& HitLocation, FVector& HitNormal);

	/** get current level name
	 * bShortName true: DM-SomeMap
	 * bShortName false: /Game/RestrictedAssets/Maps/DM-SomeMap
	 */
	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static FString GetLevelName(UObject* WorldContextObject, bool bShortName = true);

	/**
	 * Find an option in the options string and return it as a float.
	 * @param Options		The string containing the options.
	 * @param Key			The key to find the value of in Options.
	 * @return				The value associated with Key as a float if Key found in Options string, otherwise DefaultValue.
	 */
	UFUNCTION(BlueprintPure, Category = "Game Options")
	static float GetFloatOption(const FString& Options, const FString& Key, float DefaultValue);

	/** 
	 * Tries to pick the best possible context given a Targetclass for a given pawn.  Avoids using traces by just using the LastRenderTime
	 * but uses the TActorIterator so it could potentially be slow.
	 *
	 * @param PawnTarget - The pawn who's context we are looking for.  Can not be NULL
	 * @param MinAim - minimum dot product of directions that can be returned (maximum 0)
	 * @param MaxRange - maximum range to search
	 * @param TargetClass - optional subclass of Pawn to look for; default is all pawns
	 * @param BestAim - if specified, filled with actual dot product of returned target
	 * @param BestDist - if specified, filled with actual distance to returned target
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Targeting")
	static AActor* GetCurrentAimContext(AUTCharacter* PawnTarget, float MinAim, float MaxRange, TSubclassOf<AActor> TargetClass = NULL
#if CPP // hack: UHT doesn't support this (or any 'optional out' type construct)
	, float* BestAim = NULL, float* BestDist = NULL
#endif
	);

	UFUNCTION(BlueprintCallable, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayInEditor(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "UTAnalytics")
	static void RecordEvent_UTTutorialCompleted(AUTPlayerController* UTPC, FString TutorialMap);

	UFUNCTION(BlueprintCallable, Category = "UTAnalytics")
	static void RecordEvent_UTTutorialPlayInstruction(AUTPlayerController* UTPC, FString AnnouncementName, int32 InstructionID);

	/** returns the game mode class
	 * this function works on clients whereas GetGameMode() -> GetClass() does not
	 */
	UFUNCTION(BlueprintPure, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static TSubclassOf<AGameModeBase> GetGameClass(UObject* WorldContextObject)
	{
		UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject);
		AGameStateBase* GS = (World != nullptr) ? World->GetGameState() : nullptr;
		return (GS != nullptr) ? GS->GameModeClass : nullptr;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "UT", meta = (WorldContext = "WorldContextObject"))
	static void ExecuteDatabaseQuery(UObject* WorldContextObject, const FString& DatabaseQuery, TArray<FDatabaseRow>& OutDatabaseRows);

	/** Loads a string value from the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static bool GetModConfigString(const FString& ConfigSection, const FString& ConfigKey, FString& Value);

	/** Loads an array of string values from the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static int32 GetModConfigStringArray(const FString& ConfigSection, const FString& ConfigKey, TArray<FString>& Value);

	/** Loads an int value from the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static bool GetModConfigInt(const FString& ConfigSection, const FString& ConfigKey, int32& Value);
	
	/** Loads a float value from the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static bool GetModConfigFloat(const FString& ConfigSection, const FString& ConfigKey, float& Value);

	/** Saves a string value to the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static void SetModConfigString(const FString& ConfigSection, const FString& ConfigKey, const FString& Value);

	/** Saves an array of string values to the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static void SetModConfigStringArray(const FString& ConfigSection, const FString& ConfigKey, const TArray<FString>& Value);

	/** Saves an int value to the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static void SetModConfigInt(const FString& ConfigSection, const FString& ConfigKey, int32 Value);

	/** Saves a float value to the Mod.ini */
	UFUNCTION(BlueprintCallable, Category = "Config")
	static void SetModConfigFloat(const FString& ConfigSection, const FString& ConfigKey, float Value);

	UFUNCTION(BlueprintCallable, Category = "Config")
	static void SaveModConfig();

	UFUNCTION(BlueprintCallable, Category = "Config")
	static void ReloadModConfig();
};
