// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPlayerCameraManager.generated.h"

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

	/** post process settings used when there are no post process volumes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = PostProcess)
	FPostProcessSettings DefaultPPSettings;

	/** post process settings used when we want to be in stylized mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = PostProcess)
	TArray<FPostProcessSettings> StylizedPPSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = PostProcess)
	UMaterialInterface* OutlineMat;

	FVector LastThirdPersonCameraLoc;

	FVector LastGoodCamLocation;

	bool bIsForcingGoodCamLoc;

	virtual bool IsValidCamLocation(FVector InLoc);

	UPROPERTY()
		AActor* LastThirdPersonTarget;

	UPROPERTY(Config)
	float ThirdPersonCameraSmoothingSpeed;

	UPROPERTY()
	FVector FlagBaseFreeCamOffset;
	
	UPROPERTY()
	FVector EndGameFreeCamOffset;

	/** Offset to Z free camera position */
	UPROPERTY()
	float EndGameFreeCamDistance;

	/** Offset to Z death camera position */
	UPROPERTY()
		float DeathCamDistance;

	/** Death camera position */
	UPROPERTY()
		FVector DeathCamPosition;

	/** What Deathcam is looking at */
	UPROPERTY()
		FVector DeathCamFocalPoint;

	UPROPERTY()
		float DeathCamFOV;

	/** Spectator cam auto prioritization. */
	/** Bonus for currently viewed camera. */
	UPROPERTY(Config)
		float CurrentCamBonus;
	
	/** Bonus for having flag. */
	UPROPERTY(Config)
		float FlagCamBonus;
	
	/** Bonus for having powerup. */
	UPROPERTY(Config)
		float PowerupBonus;
	
	/** Bonus for having higher score than current focus. */
	UPROPERTY(Config)
		float HigherScoreBonus;
	
	/** Action bonus (declines over time since last action. */
	UPROPERTY(Config)
		float CurrentActionBonus;

	UPROPERTY()
		float CurrentCameraRoll;

	/** Camera tilt in degrees when wall sliding */
	UPROPERTY()
		float WallSlideCameraRoll;

	/** Sweep to find valid third person camera offset. */
	virtual void CheckCameraSweep(FHitResult& OutHit, AActor* TargetActor, const FVector& Start, const FVector& End);

	/** get CameraStyle after state based and gametype based override logic
	 * generally UT code should always query the current camera style through this method to account for ragdoll, etc
	 */
	virtual FName GetCameraStyleWithOverrides() const;

	virtual void UpdateCamera(float DeltaTime) override;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
	virtual void ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	virtual bool AllowPhotographyMode() const override;

	/** Rate player as camera focus for spectating. */
	virtual float RatePlayerCamera(AUTPlayerState* InPS, AUTCharacter *Character, APlayerState* CurrentCamPS);

	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;

};

