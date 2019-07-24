// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTPlayerCameraManager.h"
#include "UTGameObjective.h"
#include "UTViewPlaceholder.h"
#include "UTNoCameraVolume.h"
#include "UTRemoteRedeemer.h"
#include "UTDemoRecSpectator.h"
#include "UTLineUpHelper.h"
#include "UTPlayerState.h"
#include "UTFlag.h"

AUTPlayerCameraManager::AUTPlayerCameraManager(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FreeCamOffset = FVector(-256.f, 0.f, 90.f);
	EndGameFreeCamOffset = FVector(-256.f, 0.f, 45.f);
	EndGameFreeCamDistance = 55.0f;

	DeathCamDistance = 200.f;
	FlagBaseFreeCamOffset = FVector(0, 0, 90);
	bUseClientSideCameraUpdates = false;
	
	PrimaryActorTick.bTickEvenWhenPaused = true;
	ViewPitchMin = -89.0f;
	ViewPitchMax = 89.0f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OutlineFinder(TEXT("/Game/RestrictedAssets/Materials/M_OutlinePP.M_OutlinePP"));
	OutlineMat = OutlineFinder.Object;

	DefaultPPSettings.SetBaseValues();
	DefaultPPSettings.bOverride_AmbientCubemapIntensity = true;
	DefaultPPSettings.bOverride_BloomIntensity = true;
	DefaultPPSettings.bOverride_BloomDirtMaskIntensity = true;
	DefaultPPSettings.bOverride_AutoExposureMinBrightness = true;
	DefaultPPSettings.bOverride_AutoExposureMaxBrightness = true;
	DefaultPPSettings.bOverride_LensFlareIntensity = true;
	DefaultPPSettings.bOverride_MotionBlurAmount = true;
	DefaultPPSettings.bOverride_ScreenSpaceReflectionIntensity = true;
	DefaultPPSettings.BloomIntensity = 0.20f;
	DefaultPPSettings.BloomDirtMaskIntensity = 0.0f;
	DefaultPPSettings.AutoExposureMinBrightness = 1.0f;
	DefaultPPSettings.AutoExposureMaxBrightness = 1.0f;
	DefaultPPSettings.VignetteIntensity = 0.20f;
	DefaultPPSettings.MotionBlurAmount = 0.0f;
	DefaultPPSettings.ScreenSpaceReflectionIntensity = 0.0f;
	DefaultPPSettings.AddBlendable(OutlineMat, 1.0f);

	StylizedPPSettings.AddZeroed();
	StylizedPPSettings[0].bOverride_BloomIntensity = true;
	StylizedPPSettings[0].bOverride_MotionBlurAmount = true;
	StylizedPPSettings[0].BloomIntensity = 0.20f;
	StylizedPPSettings[0].MotionBlurAmount = 0.0f;
	StylizedPPSettings[0].bOverride_MotionBlurAmount = true;
	StylizedPPSettings[0].bOverride_MotionBlurMax = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldDepthBlurAmount = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldFocalDistance = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldScale = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldNearBlurSize = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldFarBlurSize = true;

	LastThirdPersonCameraLoc = FVector::ZeroVector;
	ThirdPersonCameraSmoothingSpeed = 6.0f;
	CurrentCameraRoll = 0.f;
	WallSlideCameraRoll = 12.5f;
	DeathCamFOV = 100.f;
}

// @TODO FIXMESTEVE SPLIT OUT true spectator controls
FName AUTPlayerCameraManager::GetCameraStyleWithOverrides() const
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	static const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));
	static const FName NAME_Default = FName(TEXT("Default"));
	static const FName NAME_LineUpCam = FName(TEXT("LineUpCam"));
	static const FName NAME_PreRallyCam = FName(TEXT("PreRallyCam"));
	static const FName NAME_RallyCam = FName(TEXT("RallyCam"));

	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		if (GameState->ActiveLineUpHelper && GameState->IsLineUpActive())
		{
			return NAME_LineUpCam;
		}
		if (GameState->IsMatchIntermission())
		{
			return NAME_FreeCam;
		}
		if ((CameraStyle == NAME_RallyCam) && GameState->IsMatchInProgress())
		{
			return NAME_RallyCam;
		}
	}

	AActor* CurrentViewTarget = GetViewTarget();
	ACameraActor* CameraActor = Cast<ACameraActor>(CurrentViewTarget);
	if (CameraActor)
	{
		return NAME_Default;
	}

	AUTCharacter* UTCharacter = Cast<AUTCharacter>(CurrentViewTarget);
	if (UTCharacter == NULL)
	{
		return ((CurrentViewTarget == PCOwner->GetPawn()) || (CurrentViewTarget == PCOwner->GetSpectatorPawn())) ? NAME_FirstPerson : NAME_FreeCam;
	}
	else if (UTCharacter->IsDead() || UTCharacter->IsRagdoll() || UTCharacter->IsThirdPersonTaunting() || (Cast<AUTDemoRecSpectator>(PCOwner) && ((AUTDemoRecSpectator *)(PCOwner))->IsKillcamSpectator()))
	{
		// force third person if target is dead, ragdoll or emoting
		return NAME_FreeCam;
	}

	return (GameState != NULL) ? GameState->OverrideCameraStyle(PCOwner, CameraStyle) : CameraStyle;

}

void AUTPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		CameraStyle = NAME_Default;
		LastThirdPersonCameraLoc = FVector::ZeroVector;
		ViewTarget.CheckViewTarget(PCOwner);
		// our camera is now viewing there
		FMinimalViewInfo NewPOV;
		NewPOV.FOV = DefaultFOV;
		NewPOV.OrthoWidth = DefaultOrthoWidth;
		NewPOV.bConstrainAspectRatio = false;
		NewPOV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		NewPOV.PostProcessBlendWeight = 1.0f;

		const bool bK2Camera = BlueprintUpdateCamera(ViewTarget.Target, NewPOV.Location, NewPOV.Rotation,NewPOV.FOV);
		if (!bK2Camera)
		{
			ViewTarget.Target->CalcCamera(DeltaTime, NewPOV);
		}

		// Cache results
		FillCameraCache(NewPOV);
	}
	else
	{
		Super::UpdateCamera(DeltaTime);
	}
}

float AUTPlayerCameraManager::RatePlayerCamera(AUTPlayerState* InPS, AUTCharacter *Character, APlayerState* CurrentCamPS)
{
	// 100 is about max
	float Score = 1.f;
	if (InPS->bOutOfLives || InPS->bOnlySpectator)
	{
		return -100.f;
	}
	else if (InPS == CurrentCamPS)
	{
		Score += CurrentCamBonus;
	}
	float LastActionTime = GetWorld()->GetTimeSeconds() - FMath::Max(Character->LastTakeHitTime, Character->LastWeaponFireTime);
	Score += FMath::Max(0.f, CurrentActionBonus - LastActionTime);

	if (InPS->CarriedObject)
	{
		Score += FlagCamBonus;
	}

	if (Character->GetWeaponOverlayFlags() != 0)
	{
		Score += PowerupBonus;
	}

	if (CurrentCamPS)
	{
		Score += (InPS->Score > CurrentCamPS->Score) ? HigherScoreBonus : -1.f * HigherScoreBonus;
	}

	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		Score += GameState->ScoreCameraView(InPS, Character);
	}
	// todo - have redeemer, armor, etc

	return Score;
}

static const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));
void AUTPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	static const FName NAME_PreRallyCam = FName(TEXT("PreRallyCam"));
	static const FName NAME_RallyCam = FName(TEXT("RallyCam"));
	static const FName NAME_GameOver = FName(TEXT("GameOver"));
	static const FName NAME_LineUpCam = FName(TEXT("LineUpCam"));

	FName SavedCameraStyle = CameraStyle;
	CameraStyle = GetCameraStyleWithOverrides();
	bool bUseDeathCam = false;
	//if we have a line up active, change our ViewTarget to be the line-up target and setup camera settings
	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	UCameraComponent* LineUpCam = ((CameraStyle == NAME_LineUpCam) && (GameState && GameState->IsLineUpActive() && GameState->ActiveLineUpHelper)) ? GameState->GetCameraComponentForLineUp(GameState->ActiveLineUpHelper->ActiveType) : nullptr;
	if (LineUpCam)
	{
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		//Set target to line up zone that owns this camera
		OutVT.Target = LineUpCam->GetOwner();
		OutVT.POV.Location = LineUpCam->GetComponentLocation();
		OutVT.POV.Rotation = LineUpCam->GetComponentRotation();

		ApplyCameraModifiers(DeltaTime, OutVT.POV);

		// Synchronize the actor with the view target results
		SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	}
	
	// smooth third person camera all the time
	if (OutVT.Target == PCOwner)
	{
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		ApplyCameraModifiers(DeltaTime, OutVT.POV);
		
		// if not during active gameplay, use spawn location/rotation
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
		if (UTPC && (!GameState || (GameState->GetMatchState() == MatchState::CountdownToBegin) || (GameState->GetMatchState() == MatchState::WaitingToStart) || (GameState->GetMatchState() == MatchState::MatchIntermission)))
		{
			if (GameState && (GameState->GetMatchState() == MatchState::CountdownToBegin) && UTPC->UTPlayerState && UTPC->UTPlayerState->RespawnChoiceA)
			{
				OutVT.POV.Location = UTPC->UTPlayerState->RespawnChoiceA->GetActorLocation();
				OutVT.POV.Rotation = UTPC->UTPlayerState->RespawnChoiceA->GetActorRotation();
			}
			else
			{
				if (PCOwner->GetSpawnLocation().IsZero())
				{
					PCOwner->SetInitialLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation);
				}
				OutVT.POV.Location = PCOwner->GetSpawnLocation();
				OutVT.POV.Rotation = UTPC->SpawnRotation;
			}
		}
		else if (OutVT.POV.Location.IsZero() || IsValidCamLocation(PCOwner->GetFocalLocation()))
		{
			OutVT.POV.Location = PCOwner->GetFocalLocation();
			OutVT.POV.Rotation = PCOwner->GetControlRotation();
		}
	}
	else if (CameraStyle == NAME_PreRallyCam)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		AActor* TargetActor = OutVT.Target;
		FVector Loc = OutVT.Target->GetActorLocation();

		float CameraDistance = FreeCamDistance;
		FVector CameraOffset = FreeCamOffset;
		FRotator Rotator = (!UTPC || UTPC->bSpectatorMouseChangesView) ? PCOwner->GetControlRotation() : UTPC->GetSpectatingRotation(Loc, DeltaTime);

		FVector Pos = Loc + FRotationMatrix(Rotator).TransformVector(CameraOffset) - Rotator.Vector() * CameraDistance;
		FHitResult Result;
		CheckCameraSweep(Result, TargetActor, Loc, Pos);
		OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
		OutVT.POV.Rotation = Rotator;
		ApplyCameraModifiers(DeltaTime, OutVT.POV);

		// Synchronize the actor with the view target results
		SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	}
	else if (CameraStyle == NAME_RallyCam)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
		float TimeScaling = 2.f * ((UTPC != nullptr) ? FMath::Clamp(UTPC->EndRallyTime - GetWorld()->GetTimeSeconds(), 0.f, 1.f) : 0.f);
		OutVT.POV.FOV = DefaultFOV + (170.f - DefaultFOV) * TimeScaling;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;
		FVector Loc = OutVT.Target->GetActorLocation();
		float CameraDistance = FreeCamDistance;
		FVector CameraOffset = FreeCamOffset;

		FRotator Rotator = (!UTPC || UTPC->bSpectatorMouseChangesView) ? PCOwner->GetControlRotation() : UTPC->GetSpectatingRotation(Loc, DeltaTime);
		FVector Pos = Loc + FRotationMatrix(Rotator).TransformVector(CameraOffset) - Rotator.Vector() * CameraDistance;
		FHitResult Result;
		CheckCameraSweep(Result, OutVT.Target, Loc, Pos);
		OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
		OutVT.POV.Rotation = Rotator;
		ApplyCameraModifiers(DeltaTime, OutVT.POV);

		// Synchronize the actor with the view target results
		SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	}
	else if (CameraStyle == NAME_FreeCam)
	{
		AUTCharacter* UTCharacter = Cast<AUTCharacter>(OutVT.Target);
		AUTGameObjective* UTFlagBase = Cast<AUTGameObjective>(OutVT.Target);
		AUTCarriedObject* UTFlag = Cast<AUTCarriedObject>(OutVT.Target);
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		FVector DesiredLoc = (Cast<AController>(OutVT.Target) && !LastThirdPersonCameraLoc.IsZero()) ? LastThirdPersonCameraLoc : OutVT.Target->GetActorLocation();
		AActor* TargetActor = OutVT.Target;
		if (UTCharacter != nullptr && UTCharacter->IsRagdoll() && UTCharacter->GetCapsuleComponent() != nullptr)
		{
			// we must use the capsule location here as the ragdoll's root component can be rubbing a wall
			DesiredLoc = UTCharacter->GetCapsuleComponent()->GetComponentLocation();
		}
		else if (UTFlagBase != nullptr)
		{
			DesiredLoc += FlagBaseFreeCamOffset;
		}
		else if (UTFlag && (GetWorld()->GetTimeSeconds() < UTFlag->LastTeleportedTime + 2.5f))
		{
			DesiredLoc = UTFlag->LastTeleportedLoc;
			LastThirdPersonTarget = UTFlag;
			TargetActor = UTFlag;
		}
		else if (Cast<APlayerState>(OutVT.Target))
		{
			DesiredLoc = LastThirdPersonCameraLoc;
		}

		//If camera is jumping to a new location and we have LOS then interp 
		bool bSnapCamera = ((UTCharacter && UTCharacter->IsDead()) || LastThirdPersonCameraLoc.IsZero() || (TargetActor != LastThirdPersonTarget));
		if (!bSnapCamera && ((DesiredLoc - LastThirdPersonCameraLoc).SizeSquared() > 250000.f))
		{
			FHitResult Result;
			CheckCameraSweep(Result, TargetActor, DesiredLoc, LastThirdPersonCameraLoc);
			bSnapCamera = Result.bBlockingHit;
		}
		FVector Loc = bSnapCamera ? DesiredLoc : FMath::VInterpTo(LastThirdPersonCameraLoc, DesiredLoc, DeltaTime, ThirdPersonCameraSmoothingSpeed);

		AUTCharacter* BaseChar = UTCharacter;
		if (!BaseChar)
		{
			BaseChar = UTFlag && UTFlag->Holder ? Cast<AUTCharacter>(UTFlag->GetAttachmentReplication().AttachParent) : NULL;
		}
		if (BaseChar && BaseChar->GetMovementBase() && MovementBaseUtility::UseRelativeLocation(BaseChar->GetMovementBase()))
		{
			// don't smooth vertically if on lift
			Loc.Z = DesiredLoc.Z;
		}

		LastThirdPersonCameraLoc = Loc;
		LastThirdPersonTarget = TargetActor;

		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
		AUTDemoRecSpectator* UTDemoRecSpec = Cast<AUTDemoRecSpectator>(PCOwner);

		bool bViewingInstantReplay = UTDemoRecSpec ? UTDemoRecSpec->IsKillcamSpectator() : false;
		bool bGameOver = (UTPC != nullptr && UTPC->GetStateName() == NAME_GameOver);
		bUseDeathCam = !bViewingInstantReplay && !bGameOver && UTCharacter && (UTCharacter->IsDead() || (UTCharacter->Health <= 0) || (UTCharacter->IsRagdoll() && UTPC->DeathCamFocus)) && UTPC && (!UTPC->DeathCamFocus || (UTPC->DeathCamFocus != TargetActor));

		float CameraDistance = FreeCamDistance;
		FVector CameraOffset = FreeCamOffset;
		if (bGameOver)
		{
			CameraDistance = EndGameFreeCamDistance;
			CameraOffset = EndGameFreeCamOffset;
		}
		FRotator Rotator = (!UTPC || UTPC->bSpectatorMouseChangesView) ? PCOwner->GetControlRotation() : UTPC->GetSpectatingRotation(Loc, DeltaTime);
		if (bUseDeathCam)
		{
			if (UTPC->DeathCamFocus && !UTPC->DeathCamFocus->IsPendingKillPending() && (GetWorld()->GetTimeSeconds() - UTPC->DeathCamTime < 2.f))
			{
				DeathCamFocalPoint = UTPC->DeathCamFocus->GetActorLocation();
			}
			float ZoomFactor = FMath::Clamp(1.5f*UTPC->GetFrozenTime() - 0.8f, 0.f, 1.f);
			float DistanceScaling = 1.f - ZoomFactor;
			FVector Pos = DeathCamPosition - Rotator.Vector() * DeathCamDistance;
			FHitResult Result;
			CheckCameraSweep(Result, TargetActor, DeathCamPosition, Pos);
			OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
 			bool bZoomIn = (UTPC->GetFrozenTime() > 0.5f) && UTPC->DeathCamFocus;
			if (bZoomIn)
			{
				// zoom in
				float ViewDist = (DeathCamFocalPoint - OutVT.POV.Location).SizeSquared();
				float ZoomedFOV = DefaultFOV * FMath::Clamp(360000.f / FMath::Max(1.f, ViewDist), 0.5f, 1.f);
				DeathCamFOV = DefaultFOV * (1.f - ZoomFactor) + ZoomedFOV*ZoomFactor;
			}
			OutVT.POV.FOV = DeathCamFOV;
			if (UTPC->IsInState(NAME_Inactive) && !DeathCamFocalPoint.IsZero()	&& (!UTPC->IsFrozen() || (UTPC->GetFrozenTime() > 0.25f)))
			{
				// custom camera control for dead players
				// still for a short while, then look at killer
				FRotator ViewRotation = UTPC->GetControlRotation();
				ViewRotation.Yaw = FMath::UnwindDegrees(ViewRotation.Yaw);
				ViewRotation.Pitch = FMath::UnwindDegrees(ViewRotation.Pitch);
				ViewRotation.Roll = 0.f;
				FRotator DesiredViewRotation = (DeathCamFocalPoint - OutVT.POV.Location).Rotation();
				DesiredViewRotation.Yaw = FMath::UnwindDegrees(DesiredViewRotation.Yaw);
				DesiredViewRotation.Pitch = bZoomIn ? FMath::UnwindDegrees(DesiredViewRotation.Pitch) : FMath::Clamp(FMath::UnwindDegrees(DesiredViewRotation.Pitch), -8.f, -5.f);
				float DeltaYaw = FMath::RadiansToDegrees(FMath::FindDeltaAngleRadians(FMath::DegreesToRadians(ViewRotation.Yaw), FMath::DegreesToRadians(DesiredViewRotation.Yaw)));
				ViewRotation.Yaw += 15.f*DeltaTime*DeltaYaw;
				float DeltaPitch = FMath::RadiansToDegrees(FMath::FindDeltaAngleRadians(FMath::DegreesToRadians(ViewRotation.Pitch), FMath::DegreesToRadians(DesiredViewRotation.Pitch)));
				ViewRotation.Pitch += 15.f*DeltaTime*DeltaPitch;
				UTPC->SetControlRotation(ViewRotation);
				Rotator = ViewRotation;

				if (UTPC->DeathCamFocus && (GetWorld()->GetTimeSeconds() - UTPC->DeathCamFocus->GetLastRenderTime() > 0.2f) && !UTPC->LineOfSightTo(UTPC->DeathCamFocus))
				{
					UTPC->DeathCamFocus = nullptr;
				}
			}
			else
			{
				Rotator.Pitch = FRotator::NormalizeAxis(Rotator.Pitch);
				Rotator.Pitch = FMath::Clamp(Rotator.Pitch, -85.f, -5.f);
			}
		}
		else
		{
			DeathCamFocalPoint = FVector::ZeroVector;
			if (Cast<AUTProjectile>(TargetActor) && !TargetActor->IsPendingKillPending())
			{
				Rotator = TargetActor->GetVelocity().Rotation();
				CameraDistance = 60.f;
				Loc = DesiredLoc;
			}

			FVector Pos = Loc + FRotationMatrix(Rotator).TransformVector(CameraOffset) - Rotator.Vector() * CameraDistance;
			FHitResult Result;
			CheckCameraSweep(Result, TargetActor, Loc, Pos);
			OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
		}
		OutVT.POV.Rotation = Rotator;
		ApplyCameraModifiers(DeltaTime, OutVT.POV);

		// Synchronize the actor with the view target results
		SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	}
	else
	{
		bool bSavedAlwaysApplyModifiers = bAlwaysApplyModifiers;
		bAlwaysApplyModifiers = bAlwaysApplyModifiers || CameraStyle == NAME_FirstPerson;

		LastThirdPersonCameraLoc = FVector::ZeroVector;
		Super::UpdateViewTarget(OutVT, DeltaTime);
		AUTCharacter* UTCharacter = Cast<AUTCharacter>(OutVT.Target);
		if (UTCharacter)
		{
			float DesiredRoll = 0.f;
			if (UTCharacter->bApplyWallSlide)
			{
				FVector Cross = UTCharacter->GetActorRotation().Vector() ^ FVector(0.f, 0.f, 1.f);
				DesiredRoll = -1.f * (Cross.GetSafeNormal() | UTCharacter->UTCharacterMovement->WallSlideNormal) * WallSlideCameraRoll;
			}
			else if (UTCharacter->UTCharacterMovement->bIsFloorSliding)
			{
				FVector Cross = UTCharacter->GetVelocity().GetSafeNormal() ^ FVector(0.f, 0.f, 1.f);
				DesiredRoll = -1.f * (Cross.GetSafeNormal() | UTCharacter->GetActorRotation().Vector()) * WallSlideCameraRoll;

			}
			float AdjustRate = FMath::Min(1.f, 10.f*DeltaTime);
			CurrentCameraRoll = (1.f - AdjustRate) * CurrentCameraRoll + AdjustRate*DesiredRoll;
			OutVT.POV.Rotation.Roll = CurrentCameraRoll;
		}

		bAlwaysApplyModifiers = bSavedAlwaysApplyModifiers;
	}

	bIsForcingGoodCamLoc = false;
	if (!IsValidCamLocation(OutVT.POV.Location))
	{
		OutVT.POV.Location = LastGoodCamLocation;
		OutVT.POV.Rotation = (GetViewTarget()->GetActorLocation() - LastGoodCamLocation).Rotation();
		bIsForcingGoodCamLoc = true;
	}
	LastGoodCamLocation = OutVT.POV.Location;
	if (!bUseDeathCam)
	{
		DeathCamPosition = LastGoodCamLocation;
		DeathCamFOV = DefaultFOV;
	}
	CameraStyle = SavedCameraStyle;
}

bool AUTPlayerCameraManager::IsValidCamLocation(FVector InLoc)
{
	APhysicsVolume* PV = Cast<APawn>(GetViewTarget()) ? ((APawn *)GetViewTarget())->GetPawnPhysicsVolume() : NULL;
	if (PV && (Cast<AKillZVolume>(PV) || Cast<AUTNoCameraVolume>(PV)))
	{
		return false;
	}

	// check the variations of KillZ
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings(true);
	return !WorldSettings->bEnableWorldBoundsChecks || (InLoc.Z > WorldSettings->KillZ);
}

void AUTPlayerCameraManager::CheckCameraSweep(FHitResult& OutHit, AActor* TargetActor, const FVector& Start, const FVector& End)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	FCollisionQueryParams BoxParams(NAME_FreeCam, false, TargetActor);

	AUTFlag* Flag = Cast<AUTFlag>(TargetActor);
	if (Flag)
	{
		if (Flag->Holder)
		{
			BoxParams.AddIgnoredActor(Flag->Holder);
		}
		if (Flag->HomeBase)
		{
			BoxParams.AddIgnoredActor(Flag->HomeBase);
		}
		if (Flag->GetAttachmentReplication().AttachParent)
		{
			BoxParams.AddIgnoredActor(Flag->GetAttachmentReplication().AttachParent);
		}
	}
	GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
}

static TAutoConsoleVariable<int32> CVarBloomDirt(TEXT("r.BloomDirt"), 0, TEXT("Enables screen dirt effect in high light/bloom"), ECVF_Scalability);

void AUTPlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ApplyCameraModifiers(DeltaTime, InOutPOV);

	// if no PP volumes, force our default PP in at the beginning
	if (GetWorld()->PostProcessVolumes.Num() == 0)
	{
		PostProcessBlendCache.Insert(DefaultPPSettings, 0);
		PostProcessBlendCacheWeights.Insert(1.0f, 0);
	}

	AUTPlayerController* UTPCOwner = Cast<AUTPlayerController>(PCOwner);
	if (UTPCOwner && UTPCOwner->StylizedPPIndex != INDEX_NONE)
	{
		PostProcessBlendCache.Insert(StylizedPPSettings[UTPCOwner->StylizedPPIndex], 0);
		PostProcessBlendCacheWeights.Insert(1.0f, 0);
	}

	if (!CVarBloomDirt.GetValueOnGameThread())
	{
		FPostProcessSettings OverrideSettings;
		OverrideSettings.bOverride_BloomDirtMaskIntensity = true;
		OverrideSettings.BloomDirtMaskIntensity = 0.0f;
		PostProcessBlendCache.Add(OverrideSettings);
		PostProcessBlendCacheWeights.Add(1.0f);
	}

	{
		FPostProcessSettings BlendableOverrides;
		// this is already in DefaultPPSettings so we don't need to add if we used those
		if (GetWorld()->PostProcessVolumes.Num() > 0)
		{
			BlendableOverrides.AddBlendable(OutlineMat, 1.0f);
		}

		UMaterialInterface* PPOverlay = NULL;
		AUTRemoteRedeemer* Redeemer = Cast<AUTRemoteRedeemer>(GetViewTargetPawn());
		if (Redeemer != NULL)
		{
			PPOverlay = Redeemer->GetPostProcessMaterial();
		}
		if (PPOverlay != NULL)
		{
			BlendableOverrides.AddBlendable(PPOverlay, 1.0f);
		}

		PostProcessBlendCache.Add(BlendableOverrides);
		PostProcessBlendCacheWeights.Add(1.0f);
	}
}

void AUTPlayerCameraManager::ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
	AUTPlayerState* PS = UTPC ? UTPC->UTPlayerState : NULL;
	if (UTPC && PS && (PS->bOnlySpectator || PS->bOutOfLives) && !UTPC->bSpectatorMouseChangesView && (GetViewTarget() != PCOwner->GetSpectatorPawn()))
	{
		LimitViewPitch(OutViewRotation, ViewPitchMin, ViewPitchMax);
		return;
	}
	Super::ProcessViewRotation(DeltaTime, OutViewRotation, OutDeltaRot);
}

bool AUTPlayerCameraManager::AllowPhotographyMode() const
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
	AUTPlayerState* PS = UTPC ? UTPC->UTPlayerState : NULL;
	if (UTPC && PS && PS->bOnlySpectator)
	{
		return true;
	}

	return false;
}