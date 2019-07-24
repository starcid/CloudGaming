// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
/** Handles individual scoring in CTF matches. */

#pragma once
#include "UTBaseScoring.h"
#include "UTATypes.h"
#include "UTCTFGameState.h"
#include "UTCTFScoring.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTCTFScoring : public AUTBaseScoring
{
	GENERATED_UCLASS_BODY()

	/** Cached reference to the CTF game state */
	UPROPERTY(BlueprintReadOnly, Category = CTF)
	class AUTCTFGameState* CTFGameState;

	// CTF Scoring.. made config for now so we can easily tweak it all.  Won't be config in the final.
	// Each player who carried the flag get's a % of this pool and is guarenteed at least 1 point
	UPROPERTY()
		uint32 FlagRunScorePool;

	// The player who initially picked up the flag will give thee points
	UPROPERTY()
		uint32 FlagFirstPickupPoints;

	// The player who caps the flag will get these points
	UPROPERTY()
		uint32 FlagCapPoints;

	// # of points the player receives for returning the flag
	UPROPERTY()
		uint32 FlagReturnPoints;

	// Points multiplied by time your flag was held before being returned
	UPROPERTY()
		float FlagReturnHeldBonus;

	// Points multiplied by time your flag was held before carrier was killed
	UPROPERTY()
		float FlagKillHeldBonus;

	UPROPERTY()
		uint32 MaxFlagHeldBonus;

	// Players who are near the flag get bonuses when they kill
	UPROPERTY()
		uint32 BaseKillScore;

	// Bonus for killing players threatening the flag carrier
	UPROPERTY()
		uint32 FlagCombatKillBonus;

	// Bonus for killing enemy flag carrier.
	UPROPERTY()
		float FlagCarrierKillBonus;

	// Bonus for assist by returning enemy flag allowing team to score.
	UPROPERTY()
		float FlagReturnAssist;

	// Bonus for assist by killing enemies threatening FC allowing team to score.
	UPROPERTY()
		float FlagSupportAssist;

	// Bonus score for everyone on team after flag cap.
	UPROPERTY()
		float TeamCapBonus;

	// Points per second for holding flag when own flag is out
	UPROPERTY()
		float FlagHolderPointsPerSecond;

	UPROPERTY()
		float RecentActionTimeThreshold;

	virtual void BeginPlay() override;
	virtual void FlagHeldTimer();

	virtual void InitFor(class AUTGameMode* Game) override;
	virtual void ScoreDamage(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker) override;
	virtual void ScoreKill(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType) override;
	virtual void ScoreObject(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason, int32 FlagCapScore=1) override;

	virtual bool WasThreateningFlagCarrier(AUTPlayerState *VictimPS, APawn* KilledPawn, AUTPlayerState *KillerPS);

	/** Return how long flag was held before current scoring action. */
	virtual float GetTotalHeldTime(AUTCarriedObject* GameObject);
};
