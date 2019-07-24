// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTBlitzFlag.h"
#include "UTCharacter.h"
#include "Net/UnrealNetwork.h"
#include "UTFlagRunGame.h"
#include "UTFlagRunGameState.h"
#include "UTATypes.h"
#include "UTRallyPoint.h"
#include "UTCTFMajorMessage.h"
#include "UTDefensePoint.h"
#include "UTBlitzDeliveryPoint.h"
#include "UTCTFGameMessage.h"
#include "UTFlagRunGameMessage.h"
#include "UTPickupHealth.h"
#include "UTPickupMessage.h"
#include "UTHUDWidget_WeaponCrosshair.h"
#include "UTATypes.h"
#include "UTAnalytics.h"

AUTRallyPoint::AUTRallyPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(ACharacter::CapsuleComponentName);
	Capsule->SetCollisionProfileName(FName(TEXT("Pickup")));
	Capsule->InitCapsuleSize(192.f, 192.0f);
	//Capsule->bShouldUpdatePhysicsVolume = false;
	Capsule->Mobility = EComponentMobility::Static;
	Capsule->OnComponentBeginOverlap.AddDynamic(this, &AUTRallyPoint::OnOverlapBegin);
	Capsule->OnComponentEndOverlap.AddDynamic(this, &AUTRallyPoint::OnOverlapEnd);
	RootComponent = Capsule;
	bShowAvailableEffect = true;

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

	if (!IsRunningCommandlet())
	{
		if (ArrowComponent)
		{
			ArrowComponent->ArrowColor = FColor(150, 200, 255);

			ArrowComponent->ArrowSize = 1.0f;
			ArrowComponent->SetupAttachment(Capsule);
			ArrowComponent->bIsScreenSizeScaled = true;
		}
	}
#endif // WITH_EDITORONLY_DATA

	PrimaryActorTick.bCanEverTick = true;
	bCollideWhenPlacing = true;

	RallyReadyDelay = 3.f;
	MinimumRallyTime = 20.f;
	MinPersistentRemaining = 1.f;
	UpdateRallyReadyCountdown(RallyReadyDelay);
	bIsEnabled = true;
	RallyOffset = 0;
	RallyPointState = RallyPointStates::Off;
	RallyAvailableDistance = 4000.f;
	bReplicateMovement = false;
	RallyBeaconText = NSLOCTEXT("UTRallyPoint", "RallyHere", " RALLY HERE! ");
	LocationText = FText::GetEmpty();
	EnemyRallyWarning = StatusMessage::EnemyRally;
}

#if WITH_EDITORONLY_DATA
/** Returns ArrowComponent subobject **/
UArrowComponent* AUTRallyPoint::GetArrowComponent() const { return ArrowComponent; }
#endif

void AUTRallyPoint::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTRallyPoint, bIsEnabled);
	DOREPLIFETIME(AUTRallyPoint, bShowAvailableEffect);
	DOREPLIFETIME(AUTRallyPoint, RallyPointState);
	DOREPLIFETIME(AUTRallyPoint, AmbientSound);
	DOREPLIFETIME(AUTRallyPoint, ReplicatedCountdown);
	DOREPLIFETIME(AUTRallyPoint, ReplicatedRallyTimeRemaining);
}

void AUTRallyPoint::BeginPlay()
{
	Super::BeginPlay();

	// associate as team locker with team volume I am in
	TArray<UPrimitiveComponent*> OverlappingComponents;
	Capsule->GetOverlappingComponents(OverlappingComponents);
	MyGameVolume = nullptr;
	int32 BestPriority = -1.f;

	for (auto CompIt = OverlappingComponents.CreateIterator(); CompIt; ++CompIt)
	{
		UPrimitiveComponent* OtherComponent = *CompIt;
		if (OtherComponent && OtherComponent->bGenerateOverlapEvents)
		{
			AUTGameVolume* V = Cast<AUTGameVolume>(OtherComponent->GetOwner());
			if (V && V->Priority > BestPriority)
			{
				if (V->IsOverlapInVolume(*Capsule))
				{
					BestPriority = V->Priority;
					MyGameVolume = V;
				}
			}
		}
	}
	if (MyGameVolume)
	{
		MyGameVolume->RallyPoints.AddUnique(this);
	}

	GlowDecalMaterialInstance = GlowDecalMaterial ? UMaterialInstanceDynamic::Create(GlowDecalMaterial, this) : nullptr;
	if (GlowDecalMaterialInstance)
	{
		FRotator DecalRotation = GetActorRotation();
		DecalRotation.Pitch -= 90.f;
		AvailableDecal = UGameplayStatics::SpawnDecalAtLocation(this, GlowDecalMaterialInstance, FVector(192.f, 192.f, 192.f), GetActorLocation() - FVector(0.f, 0.f, 190.f), DecalRotation);

		static FName NAME_Color(TEXT("Color"));
		GlowDecalMaterialInstance->SetVectorParameterValue(NAME_Color, FVector(1.f, 1.f, 1.f));
		static FName NAME_EmissiveBrightness(TEXT("EmissiveBrightness"));
		GlowDecalMaterialInstance->SetScalarParameterValue(NAME_EmissiveBrightness, 5.f);
	}
	OnAvailableEffectChanged();
}

void AUTRallyPoint::GenerateDefensePoints()
{
	AUTFlagRunGameState* GameState = GetWorld()->GetGameState<AUTFlagRunGameState>();	// TODO: a bit hacky here as rally points are only used in FR currently, where we want to associate defense points with the capture point
	if (GameState && GameState->DeliveryPoint)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() - FVector(0.0f, 0.0f, 500.0f), ECC_Pawn, FCollisionQueryParams(NAME_None, false, this)))
		{
			FRotator SpawnRot = (GetActorLocation() - GameState->DeliveryPoint->GetActorLocation()).Rotation();
			SpawnRot.Pitch = 0;
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AUTDefensePoint* NewPoint = GetWorld()->SpawnActor<AUTDefensePoint>(Hit.Location + FVector(0.0f, 0.0f, 50.0f), SpawnRot, Params);
			if (NewPoint != nullptr)
			{
				NewPoint->Objective = GameState->DeliveryPoint;
				NewPoint->BasePriority = 2;
				GameState->DeliveryPoint->DefensePoints.Add(NewPoint);
			}
		}
	}
}

void AUTRallyPoint::Reset_Implementation()
{
	bShowAvailableEffect = false;
	RallyPointState = RallyPointStates::Off;
	FlagNearbyChanged(false);
	SetAmbientSound(nullptr, false);
	GetWorldTimerManager().ClearTimer(EndRallyHandle);
}

void AUTRallyPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ((Role == ROLE_Authority) && bIsEnabled)
	{
		AUTCharacter* TouchingCharacter = Cast<AUTCharacter>(OtherActor);
		AUTBlitzFlag* CharFlag = TouchingCharacter ? Cast<AUTBlitzFlag>(TouchingCharacter->GetCarriedObject()) : nullptr;
		if (CharFlag != NULL)
		{
			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			if ((GS == NULL) || (GS->IsMatchInProgress() && !GS->IsMatchIntermission() && !GS->HasMatchEnded()))
			{
				TouchingFC = TouchingCharacter;
				StartRallyCharging();
			}
		}
		else if ((RallyPointState == RallyPointStates::Off) && TouchingCharacter && Cast<AUTPlayerController>(TouchingCharacter->GetController()))
		{
			GetWorldTimerManager().SetTimer(WarnNoFlagHandle, FTimerDelegate::CreateUObject(this, &AUTRallyPoint::WarnNoFlag, TouchingCharacter), 1.5f, false);
		}
	}
}

void AUTRallyPoint::WarnNoFlag(AUTCharacter* TouchingCharacter)
{
	if (TouchingCharacter && !TouchingCharacter->IsPendingKillPending() && Cast<AUTPlayerController>(TouchingCharacter->GetController()))
	{
		Cast<AUTPlayerController>(TouchingCharacter->GetController())->ClientReceiveLocalizedMessage(UUTFlagRunGameMessage::StaticClass(), 30, TouchingCharacter->PlayerState);
	}
}

void AUTRallyPoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if ((Role == ROLE_Authority) && bIsEnabled)
	{
		AUTCharacter* TouchingCharacter = Cast<AUTCharacter>(OtherActor);
		if (TouchingCharacter != nullptr)
		{
			AUTBlitzFlag* CharFlag = Cast<AUTBlitzFlag>(TouchingCharacter->GetCarriedObject());
			if (CharFlag != nullptr)
			{
				EndRallyCharging();
				TouchingFC = nullptr;
			}
			GetWorldTimerManager().ClearTimer(WarnNoFlagHandle);
		}
	}
}

void AUTRallyPoint::SetRallyPointState(FName NewState)
{
	RallyPointState = NewState;
	if (Role == ROLE_Authority)
	{
		AUTFlagRunGameState* GameState = GetWorld()->GetGameState<AUTFlagRunGameState>();
		if (GameState)
		{
			if (GameState->CurrentRallyPoint == this)
			{
				if ((RallyPointState != RallyPointStates::Powered) && (RallyPointState != RallyPointStates::Charging))
				{
					GameState->CurrentRallyPoint = nullptr;
					GameState->bEnemyRallyPointIdentified = false;
					if (GameState->PendingRallyPoint && ((GameState->PendingRallyPoint->RallyPointState == RallyPointStates::Powered) || (GameState->PendingRallyPoint->RallyPointState == RallyPointStates::Charging)))
					{
						GameState->CurrentRallyPoint = GameState->PendingRallyPoint;
						GameState->PendingRallyPoint = nullptr;
					}
				}
			}
			else if (RallyPointState == RallyPointStates::Powered)
			{
				if (GameState->CurrentRallyPoint)
				{
					GameState->CurrentRallyPoint->RallyPoweredTurnOff();
				}
				GameState->CurrentRallyPoint = this;
				if (GameState->PendingRallyPoint == this)
				{
					GameState->PendingRallyPoint = nullptr;
				}
			}
			else if (RallyPointState == RallyPointStates::Charging)
			{
				if (GameState->CurrentRallyPoint)
				{
					if (GameState->PendingRallyPoint)
					{
						GameState->PendingRallyPoint->RallyPoweredTurnOff();
					}
					GameState->PendingRallyPoint = this;
				}
				else
				{
					GameState->CurrentRallyPoint = this;
					if (GameState->PendingRallyPoint)
					{
						GameState->PendingRallyPoint->RallyPoweredTurnOff();
						GameState->PendingRallyPoint = nullptr;
					}
				}
			}
			else if (GameState->PendingRallyPoint == this)
			{
				GameState->PendingRallyPoint = nullptr;
			}
		}
	}
}

void AUTRallyPoint::UpdateRallyReadyCountdown(float NewValue)
{
	RallyReadyCountdown = FMath::Clamp(NewValue, 0.f, RallyReadyDelay);
	ReplicatedCountdown = FMath::Max(0, int32(10.f*RallyReadyCountdown));
	OldClientCountdown = ClientCountdown;
	ClientCountdown = RallyReadyCountdown;
}

void AUTRallyPoint::OnReplicatedCountdown()
{
	OldClientCountdown = ClientCountdown;
	ClientCountdown = 0.1f * ReplicatedCountdown;
}

void AUTRallyPoint::StartRallyCharging()
{
	if (RallyPointState == RallyPointStates::Powered)
	{
		return;
	}
	UpdateRallyReadyCountdown(FMath::Max(RallyReadyCountdown, MinPersistentRemaining));
	SetRallyPointState(RallyPointStates::Charging);
	OnRallyChargingChanged();
	if ((Role == ROLE_Authority) && TouchingFC && TouchingFC->GetCarriedObject() && TouchingFC->GetCarriedObject()->bCurrentlyPinged && !GetWorldTimerManager().IsTimerActive(EnemyRallyWarningHandle) && (GetWorld()->GetTimeSeconds() - LastEnemyRallyWarning > 9.f))
	{
		GetWorldTimerManager().SetTimer(EnemyRallyWarningHandle, this, &AUTRallyPoint::WarnEnemyRally, 0.2f, false);
	}
}

void AUTRallyPoint::WarnEnemyRally()
{
	if ((GetWorld()->GetTimeSeconds() - LastEnemyRallyWarning < 8.f) || (RallyPointState != RallyPointStates::Charging))
	{
		return;
	}
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	LastEnemyRallyWarning = GetWorld()->GetTimeSeconds();
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
		if (UTPS && UTPS->Team && ((UTPS->Team->TeamIndex == 0) != GS->bRedToCap))
		{
			UTPS->AnnounceStatus(EnemyRallyWarning);
			break;
		}
	}
}

void AUTRallyPoint::RallyPoweredTurnOff()
{
	GetWorldTimerManager().ClearTimer(EndRallyHandle);
//	UpdateRallyReadyCountdown(RallyReadyDelay);
	SetRallyPointState(RallyPointStates::Off);
	OnRallyChargingChanged();
}

void AUTRallyPoint::RallyPoweredComplete()
{
	// go to either off or start charging again depending on if FC is touching
	TSet<AActor*> Touching;
	Capsule->GetOverlappingActors(Touching);
	AUTBlitzFlag* CharFlag = nullptr;
	TouchingFC = nullptr;
	for (AActor* TouchingActor : Touching)
	{
		CharFlag = Cast<AUTCharacter>(TouchingActor) ? Cast<AUTBlitzFlag>(((AUTCharacter*)TouchingActor)->GetCarriedObject()) : nullptr;
		if (CharFlag)
		{
			TouchingFC = Cast<AUTCharacter>(TouchingActor);
			break;
		}
	}
	UpdateRallyReadyCountdown(RallyReadyDelay);
	FName NextState = (CharFlag != nullptr) ? RallyPointStates::Charging : RallyPointStates::Off;
	SetRallyPointState(NextState);
	OnRallyChargingChanged();

	if (Role == ROLE_Authority)
	{
		int32 MessageIndex = (NextState == RallyPointStates::Off) ? 25 : 26;
		AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
		if (UTGS != nullptr)
		{
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
				if (PC && PC->UTPlayerState && PC->UTPlayerState->Team && (UTGS->bRedToCap == (PC->UTPlayerState->Team->TeamIndex == 0)))
				{
					PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), MessageIndex, nullptr);
				}
			}
		}
	}
}

void AUTRallyPoint::EndRallyCharging()
{
	if (RallyPointState == RallyPointStates::Powered)
	{
		return;
	}
//	UpdateRallyReadyCountdown(FMath::Max(RallyReadyDelay, MinPersistentRemaining));
	SetRallyPointState(RallyPointStates::Off);
	OnRallyChargingChanged();
}

void AUTRallyPoint::FlagNearbyChanged(bool bIsNearby)
{
	bShowAvailableEffect = bIsNearby;

	if (Role == ROLE_Authority)
	{
		if (!bIsNearby)
		{
//			UpdateRallyReadyCountdown(RallyReadyDelay);
			SetRallyPointState(RallyPointStates::Off);
		}
		else if (RallyPointState == RallyPointStates::Off)
		{
			AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
			AUTCharacter* NearbyFC = FlagRunGame && FlagRunGame->ActiveFlag ? FlagRunGame->ActiveFlag->HoldingPawn : nullptr;
			if (NearbyFC)
			{
				TSet<AActor*> Touching;
				Capsule->GetOverlappingActors(Touching);
				AUTBlitzFlag* CharFlag = nullptr;
				TouchingFC = nullptr;
				for (AActor* TouchingActor : Touching)
				{
					CharFlag = Cast<AUTCharacter>(TouchingActor) ? Cast<AUTBlitzFlag>(((AUTCharacter*)TouchingActor)->GetCarriedObject()) : nullptr;
					if (CharFlag)
					{
						TouchingFC = Cast<AUTCharacter>(TouchingActor);
						SetRallyPointState(RallyPointStates::Charging);
						break;
					}
				}
			}
		}
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnAvailableEffectChanged();
	}
}

void AUTRallyPoint::OnRallyChargingChanged()
{
	AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	bool bRedColor = UTGS && UTGS->bRedToCap;
	if (RallyPointState == RallyPointStates::Powered)
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SetAmbientSound(FullyPoweredSound, false);
			ChangeAmbientSoundPitch(PoweringUpSound, 1.5f);
			if (!RallyPoweredEffectPSC && (bRedColor ? RallyPoweredEffectRed : RallyPoweredEffectBlue))
			{
				RallyPoweredEffectPSC = UGameplayStatics::SpawnEmitterAtLocation(this, bRedColor ? RallyPoweredEffectRed : RallyPoweredEffectBlue, GetActorLocation(), GetActorRotation());
			}
		}

		AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
		AUTCharacter* NearbyFC = FlagRunGame && FlagRunGame->ActiveFlag ? FlagRunGame->ActiveFlag->HoldingPawn : nullptr;
		if ((Role == ROLE_Authority) && FlagRunGame && NearbyFC && (FUTAnalytics::IsAvailable()))
		{
			FUTAnalytics::FireEvent_RallyPointCompleteActivate(FlagRunGame, Cast<AUTPlayerState>(NearbyFC->PlayerState));
		}
	}
	else
	{
		if ((GetNetMode() != NM_DedicatedServer) && (RallyPoweredEffectPSC != nullptr))
		{
			// clear it
			RallyPoweredEffectPSC->ActivateSystem(false);
			RallyPoweredEffectPSC->UnregisterComponent();
			RallyPoweredEffectPSC = nullptr;

			// spawn rallyfinished
			UGameplayStatics::SpawnEmitterAtLocation(this, bRedColor ? RallyFinishedEffectRed : RallyFinishedEffectBlue, GetActorLocation(), GetActorRotation());
		}
		if (RallyPointState == RallyPointStates::Charging)
		{
			if (GetNetMode() != NM_DedicatedServer)
			{
				SetAmbientSound(PoweringUpSound, false);
				ChangeAmbientSoundPitch(PoweringUpSound, 0.5f);
				UUTGameplayStatics::UTPlaySound(GetWorld(), FCTouchedSound, this, SRT_All);
				RallyChargingEffectPSC = UGameplayStatics::SpawnEmitterAtLocation(this, bRedColor ? RallyChargingEffectRed : RallyChargingEffectBlue, GetActorLocation(), GetActorRotation());
			}
			bHaveGameState = (UTGS != nullptr);
			if ((Role == ROLE_Authority) && (UTGS != nullptr))
			{
				AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
				AUTCharacter* NearbyFC = FlagRunGame && FlagRunGame->ActiveFlag ? FlagRunGame->ActiveFlag->HoldingPawn : nullptr;
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
					if (PC && PC->UTPlayerState && (PC->GetPawn() != NearbyFC) && PC->UTPlayerState->Team && (UTGS->bRedToCap == (PC->UTPlayerState->Team->TeamIndex == 0)))
					{
						PC->UTClientPlaySound(FCTouchedSound);
					}
				}

				//if this is a fresh attempt, send an analytic
				if (FUTAnalytics::IsAvailable() && NearbyFC && FlagRunGame && (RallyReadyCountdown == RallyReadyDelay))
				{
					FUTAnalytics::FireEvent_RallyPointBeginActivate(FlagRunGame, Cast<AUTPlayerState>(NearbyFC->PlayerState));
				}
			}
		}
		else
		{
			if (GetNetMode() != NM_DedicatedServer)
			{
				if (RallyChargingEffectPSC != nullptr)
				{
					// clear it
					RallyChargingEffectPSC->ActivateSystem(false);
					RallyChargingEffectPSC->UnregisterComponent();
					RallyChargingEffectPSC = nullptr;

					// spawn charge stopped
					LosingChargeEffectPSC = UGameplayStatics::SpawnEmitterAtLocation(this, bRedColor ? LosingChargeEffectRed : LosingChargeEffectBlue, GetActorLocation(), GetActorRotation());
					UUTGameplayStatics::UTPlaySound(GetWorld(), RallyBrokenSound, this, SRT_All);
				}
				SetAmbientSound(nullptr, false);
			}
			if ((Role == ROLE_Authority) && ((RallyReadyCountdown  < 2.f) || !TouchingFC || TouchingFC->IsDead()))
			{
				if ((UTGS->CurrentRallyPoint == nullptr) || (UTGS->CurrentRallyPoint == this))
				{
					for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
					{
						AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
						if (PC && PC->UTPlayerState && PC->UTPlayerState->Team && (UTGS->bRedToCap == (PC->UTPlayerState->Team->TeamIndex == 0)))
						{
							PC->UTClientPlaySound(RallyBrokenSound);
						}
					}
				}
			}
		}
	}
	OnAvailableEffectChanged();
	OnRallyStateChanged();
}

void AUTRallyPoint::OnAvailableEffectChanged()
{
	if (bIsEnabled && (GetNetMode() != NM_DedicatedServer))
	{
		if (AvailableEffectPSC == nullptr)
		{
			AvailableEffectPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AvailableEffect, GetActorLocation() - FVector(0.f, 0.f, 64.f), GetActorRotation());
		}
		FVector RingColor(1.f, 1.f, 1.f);
		if (bShowAvailableEffect)
		{
			AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
			bHaveGameState = (UTGS != nullptr);
			if (bHaveGameState)
			{
				RingColor = (UTGS->bRedToCap || !UTGS->HasMatchStarted()) ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 0.f, 1.f);
			}
		}
		else
		{
			SetAmbientSound(PoweringUpSound, true);
		}
		static FName NAME_RingColor(TEXT("RingColor"));
		AvailableEffectPSC->SetVectorParameter(NAME_RingColor, RingColor);
		if (GlowDecalMaterialInstance)
		{
			static FName NAME_Color(TEXT("Color"));
			GlowDecalMaterialInstance->SetVectorParameterValue(NAME_Color, 5.f*RingColor);
			static FName NAME_EmissiveBrightness(TEXT("EmissiveBrightness"));
			GlowDecalMaterialInstance->SetScalarParameterValue(NAME_EmissiveBrightness, 1.f);
		}
	}
}

void AUTRallyPoint::OnRallyTimeRemaining()
{
	RallyTimeRemaining = ReplicatedRallyTimeRemaining;
}

// Blitz game has pointer to active flag, use this to determine distance. base on flag, not carrier
void AUTRallyPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ClientCountdown -= DeltaTime;
	if (bIsEnabled && (Role == ROLE_Authority))
	{
		AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
		if (FlagRunGame && FlagRunGame->ActiveFlag && (RallyPointState != RallyPointStates::Powered))
		{
			FVector FlagLocation = FlagRunGame->ActiveFlag->HoldingPawn ? FlagRunGame->ActiveFlag->HoldingPawn->GetActorLocation() : FlagRunGame->ActiveFlag->GetActorLocation();
			AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
			bool bFlagIsClose = ((FlagLocation - GetActorLocation()).Size() < RallyAvailableDistance) && UTGS && (UTGS->RemainingPickupDelay <= 0);
			if (bShowAvailableEffect != bFlagIsClose)
			{
				FlagNearbyChanged(bFlagIsClose);
			}
		}
	}
	if (bShowAvailableEffect)
	{
		if (Role == ROLE_Authority)
		{
			if (RallyPointState == RallyPointStates::Charging)
			{
				AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
				AUTCharacter* NearbyFC = FlagRunGame && FlagRunGame->ActiveFlag ? FlagRunGame->ActiveFlag->HoldingPawn : nullptr;
				if (!NearbyFC)
				{
					EndRallyCharging();
				}
				else
				{
					LastRallyHot = FMath::Max(LastRallyHot, NearbyFC->LastTargetedTime);
					UpdateRallyReadyCountdown(RallyReadyCountdown - DeltaTime);
					if (RallyReadyCountdown <= 0.f)
					{
						AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
						if (UTGS != nullptr)
						{
							for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
							{
								AUTPlayerState* PS = Cast<AUTPlayerState>(Iterator->IsValid() ? Iterator->Get()->PlayerState : nullptr);
								if (PS != nullptr)
								{
									PS->bRallyActivated = true;
									PS->ForceNetUpdate();
									AUTPlayerController* PC = Cast<AUTPlayerController>(Iterator->Get());
									if (PC != nullptr)
									{
										if (!UTGS->OnSameTeam(NearbyFC, PC))
										{
											PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 28, NearbyFC->PlayerState);
										}
										else
										{

											if (!PC->GetUTCharacter() || PC->GetUTCharacter()->bCanRally || PC->GetUTCharacter()->GetCarriedObject())
											{
												PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), PC->UTPlayerState->CarriedObject ? 22 : 30, NearbyFC->PlayerState);
											}
										}
									}
								}

								// tell bots about FC position
								// technically this should happen only after someone uses the rally point (since that's when the defender HUD shows up)
								// but we don't have a "tell bot we realized they were here X seconds ago" function and anyway in the majority of cases the rally point used is not a surprise
								AUTBot* B = Cast<AUTBot>(Iterator->Get());
								if (B != NULL && !B->IsTeammate(NearbyFC))
								{
									B->UpdateEnemyInfo(NearbyFC, EUT_HeardExact);
								}
							}
						}
						UUTGameplayStatics::UTPlaySound(GetWorld(), ReadyToRallySound, this, SRT_All);
						SetRallyPointState(RallyPointStates::Powered);
						AUTPlayerState* PS = Cast<AUTPlayerState>(NearbyFC->PlayerState);
						if (PS)
						{
							PS->ModifyStatsValue(NAME_RalliesPowered, 1);
						}
						RallyStartTime = GetWorld()->GetTimeSeconds();
						RallyTimeRemaining = MinimumRallyTime;
						ReplicatedRallyTimeRemaining = MinimumRallyTime;
						if (NearbyFC->Health < NearbyFC->HealthMax)
						{
							NearbyFC->Health += FMath::Min(50, NearbyFC->HealthMax - NearbyFC->Health);
							NearbyFC->OnHealthUpdated();
							AUTPlayerController* FCPC = Cast<AUTPlayerController>(NearbyFC->GetController());
							if (FCPC)
							{
								FCPC->ClientReceiveLocalizedMessage(UUTPickupMessage::StaticClass(), 0, NULL, NULL, AUTPickupHealth::StaticClass());
							}
						}
						GetWorldTimerManager().SetTimer(EndRallyHandle, this, &AUTRallyPoint::RallyPoweredComplete, MinimumRallyTime, false);
						OnRallyChargingChanged();
						ChangeAmbientSoundPitch(PoweringUpSound, 1.5f);
					}
					else
					{
						ChangeAmbientSoundPitch(PoweringUpSound, 1.5f - RallyReadyCountdown / RallyReadyDelay);
					}
				}
			}
			else if (RallyPointState == RallyPointStates::Powered)
			{
				if (GetWorld()->GetTimeSeconds() - RallyStartTime < 0.7f)
				{
					AUTFlagRunGame* FlagRunGame = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
					AUTCharacter* NearbyFC = FlagRunGame && FlagRunGame->ActiveFlag ? FlagRunGame->ActiveFlag->HoldingPawn : nullptr;
					if (NearbyFC && (NearbyFC->LastTargetedTime > LastRallyHot))
					{
						LastRallyHot = NearbyFC->LastTargetedTime;
					}
				}
				RallyTimeRemaining = MinimumRallyTime - (GetWorld()->GetTimeSeconds() - RallyStartTime);
				if (int32(RallyTimeRemaining) != int32(RallyTimeRemaining + DeltaTime))
				{
					ReplicatedRallyTimeRemaining = RallyTimeRemaining;
				}
			}
			else
			{
				UpdateRallyReadyCountdown(RallyReadyCountdown + 2.f*DeltaTime);
			}
		}
		else if (!bHaveGameState)
		{
			AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
			bHaveGameState = (UTGS != nullptr);
			if (bHaveGameState && AvailableEffectPSC)
			{
				if (AvailableEffectPSC)
				{
					static FName NAME_RingColor(TEXT("RingColor"));
					AvailableEffectPSC->SetVectorParameter(NAME_RingColor, UTGS->bRedToCap ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 0.f, 1.f));
				}
				if (GlowDecalMaterialInstance)
				{
					static FName NAME_Color(TEXT("Color"));
					GlowDecalMaterialInstance->SetVectorParameterValue(NAME_Color, UTGS && UTGS->bRedToCap ? FVector(5.f, 0.f, 0.f) : FVector(0.f, 0.f, 5.f));
				}
			}
		}
		else if (RallyPointState == RallyPointStates::Charging)
		{
			ChangeAmbientSoundPitch(PoweringUpSound, AmbientSoundPitch + DeltaTime / RallyReadyDelay);
		}
		else if (RallyPointState == RallyPointStates::Powered)
		{
			RallyTimeRemaining -= DeltaTime;
		}
	}
}

void AUTRallyPoint::SetAmbientSound(USoundBase* NewAmbientSound, bool bClear)
{
	if (bClear)
	{
		if (NewAmbientSound == AmbientSound)
		{
			AmbientSound = NULL;
		}
	}
	else
	{
		AmbientSound = NewAmbientSound;
	}
	AmbientSoundUpdated();
}

void AUTRallyPoint::AmbientSoundUpdated()
{
	if (AmbientSound == NULL)
	{
		if (AmbientSoundComp != NULL)
		{
			AmbientSoundComp->Stop();
		}
	}
	else
	{
		if (AmbientSoundComp == NULL)
		{
			AmbientSoundComp = NewObject<UAudioComponent>(this);
			AmbientSoundComp->bAutoDestroy = false;
			AmbientSoundComp->bAutoActivate = false;
			AmbientSoundComp->SetupAttachment(RootComponent);
			AmbientSoundComp->RegisterComponent();
		}
		if (AmbientSoundComp->Sound != AmbientSound)
		{
			// don't attenuate/spatialize sounds made by a local viewtarget
			AmbientSoundComp->bAllowSpatialization = true;
			AmbientSoundComp->SetPitchMultiplier(1.f);

			if (GEngine->GetMainAudioDevice() && !GEngine->GetMainAudioDevice()->IsHRTFEnabledForAll())
			{
				for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
				{
					if (It->PlayerController != NULL && It->PlayerController->GetViewTarget() == this)
					{
						AmbientSoundComp->bAllowSpatialization = false;
						break;
					}
				}
			}

			AmbientSoundComp->SetSound(AmbientSound);
		}
		if (!AmbientSoundComp->IsPlaying())
		{
			AmbientSoundComp->Play();
		}
	}
}

void AUTRallyPoint::ChangeAmbientSoundPitch(USoundBase* InAmbientSound, float NewPitch)
{
	if (AmbientSoundComp && AmbientSound && (AmbientSound == InAmbientSound))
	{
		AmbientSoundPitch = NewPitch;
		AmbientSoundPitchUpdated();
	}
}

void AUTRallyPoint::AmbientSoundPitchUpdated()
{
	if (AmbientSoundComp && AmbientSound)
	{
		AmbientSoundComp->SetPitchMultiplier(AmbientSoundPitch);
	}
}

FVector AUTRallyPoint::GetRallyLocation(AUTCharacter* TestChar)
{
	if (TestChar != nullptr)
	{
		RallyOffset += 3;
		int32 OldRallyOffset = RallyOffset;
		for (int32 i = 0; i < 8 - RallyOffset; i++)
		{
			FVector Adjust(0.f);
			FVector NextLocation = GetActorLocation() + 142.f * FVector(FMath::Sin(2.f*PI*RallyOffset*0.125f), FMath::Cos(2.f*PI*RallyOffset*0.125f), 0.f);
			// check if fits at desired location
			if (!GetWorld()->EncroachingBlockingGeometry(TestChar, NextLocation, TestChar->GetActorRotation(), &Adjust))
			{
				return NextLocation;
			}
			RallyOffset++;
		}
		RallyOffset = 0;
		for (int32 i = 0; i < OldRallyOffset; i++)
		{
			FVector Adjust(0.f);
			FVector NextLocation = GetActorLocation() + 142.f * FVector(FMath::Sin(2.f*PI*RallyOffset*0.125f), FMath::Cos(2.f*PI*RallyOffset*0.125f), 0.f);
			// check if fits at desired location
			if (!GetWorld()->EncroachingBlockingGeometry(TestChar, NextLocation, TestChar->GetActorRotation(), &Adjust))
			{
				return NextLocation;
			}
			RallyOffset++;
		}
	}
	return GetActorLocation();
}

FVector AUTRallyPoint::GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow)
{
	FVector Cross = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
	FVector DrawScreenPosition;
	float ExtraPadding = 0.065f * Canvas->ClipX;
	DrawScreenPosition = Canvas->Project(WorldPosition);
	FVector FlagDir = WorldPosition - ViewPoint;
	if ((ViewDir | FlagDir) < 0.f)
	{
		bool bWasLeft = bBeaconWasLeft;
		bDrawEdgeArrow = true;
		DrawScreenPosition.X = bWasLeft ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
		DrawScreenPosition.Y = 0.5f*Canvas->ClipY;
		DrawScreenPosition.Z = 0.0f;
		return DrawScreenPosition;
	}
	else if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > Canvas->ClipX))
	{
		bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
		float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - Canvas->ClipX;
		bDrawEdgeArrow = true;
		DrawScreenPosition.X = bLeftOfScreen ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
		//Y approaches 0.5*Canvas->ClipY as further off screen
		float MaxOffscreenDistance = Canvas->ClipX;
		DrawScreenPosition.Y = 0.4f*Canvas->ClipY + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) * (DrawScreenPosition.Y - 0.6f*Canvas->ClipY);
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, 0.25f*Canvas->ClipY, 0.75f*Canvas->ClipY);
		bBeaconWasLeft = bLeftOfScreen;
	}
	else
	{
		bBeaconWasLeft = false;
		DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, Edge, Canvas->ClipX - Edge);
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, Edge, Canvas->ClipY - Edge);
		DrawScreenPosition.Z = 0.0f;
	}
	return DrawScreenPosition;
}

void AUTRallyPoint::DrawChargingThermometer(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, bool bFixedPosition)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	if (UTPC && UTPC->MyUTHUD)
	{
		float Width = 150.f;
		float Height = 21.f;
		float WidthScale = Canvas->ClipX / 1920.f;;
		float HeightScale = WidthScale;
		WidthScale *= 0.75f;
		FLinearColor ChargeColor = (RallyPointState == RallyPointStates::Charging) ? FLinearColor::White : REDHUDCOLOR;
		float ChargePct = FMath::Clamp((RallyReadyDelay - ClientCountdown) / RallyReadyDelay, 0.f, 1.f);
		UTexture* Texture = UTPC->MyUTHUD->HUDAtlas;
		float DistanceScaling = 1.f;

		float U = 127.f / Texture->Resource->GetSizeX();
		float V = 641.f / Texture->Resource->GetSizeY();
		float UL = U + (Width*ChargePct / Texture->Resource->GetSizeX());
		float VL = V + (Height / Texture->Resource->GetSizeY());
		FVector2D RenderPos = FVector2D(0.5f*Canvas->ClipX - (0.5f*Width), 0.4f*Canvas->ClipY - (0.5f*Height));
		if (!bFixedPosition)
		{
			// position and scale based on world location
			FVector ViewDir = UTPC->GetControlRotation().Vector();
			float Dist = FMath::Max(1.f,(CameraPosition - GetActorLocation()).Size());
			bool bDrawEdgeArrow = false;
			FVector WorldPosition = GetActorLocation() + FVector(0.f, 0.f, 200.f);
			FVector ScreenPos = GetAdjustedScreenPosition(Canvas, WorldPosition, CameraPosition, ViewDir, Dist, 20.f, bDrawEdgeArrow);
			DistanceScaling = FMath::Min(0.5f, 1000.f/Dist);
			RenderPos.X = ScreenPos.X - 0.5f*Width*DistanceScaling;
			RenderPos.Y = ScreenPos.Y - 8.f;
		}

		FLinearColor DrawColor = FLinearColor::White;
		DrawColor.A = 0.8f;

		FCanvasTileItem ImageItem(RenderPos, Texture->Resource, DistanceScaling*FVector2D(Width*ChargePct, Height), FVector2D(U, V), FVector2D(UL, VL), DrawColor);
		ImageItem.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
		Canvas->DrawItem(ImageItem);

		V = 612.f / Texture->Resource->GetSizeY();
		UL = U + (Width / Texture->Resource->GetSizeX());
		VL = V + (Height / Texture->Resource->GetSizeY());
		FCanvasTileItem ImageItemB(RenderPos, Texture->Resource, DistanceScaling*FVector2D(Width, Height), FVector2D(U, V), FVector2D(UL, VL), DrawColor);
		ImageItemB.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
		Canvas->DrawItem(ImageItemB);
	}
}

void AUTRallyPoint::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
	AUTPlayerState* ViewerPS = PC ? Cast <AUTPlayerState>(PC->PlayerState) : nullptr;
	AUTFlagRunGameState* UTGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	if (!ViewerPS || !UTGS || !ViewerPS->Team)
	{
		return;
	}
	if (ViewerPS->CarriedObject)
	{
		if (!bShowAvailableEffect)
		{
			return;
		}
		if (RallyPointState == RallyPointStates::Charging)
		{
			DrawChargingThermometer(PC, Canvas, CameraPosition, true);
			return;
		}
	}
	else if (RallyPointState != RallyPointStates::Powered)
	{
		if (RallyPointState == RallyPointStates::Charging)
		{
			AUTCharacter* FC = (UTGS && UTGS->GetOffenseFlag()) ? UTGS->GetOffenseFlag()->HoldingPawn : nullptr;
			if (FC && (GetWorld()->GetTimeSeconds() - FC->GetLastRenderTime() < 0.1f) && PC->LineOfSightTo(FC))
			{
				DrawChargingThermometer(PC, Canvas, CameraPosition, false);
			}
		}
		return;
	}
	bool bViewerIsAttacking = (UTGS->bRedToCap == (ViewerPS->Team->TeamIndex == 0));
	if (!UTGS->bEnemyRallyPointIdentified && !bViewerIsAttacking)
	{
		return;
	}

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	const bool bIsViewTarget = (PC->GetViewTarget() == this);
	FVector WorldPosition = GetActorLocation();
	if (UTPC != NULL && !UTGS->IsMatchIntermission() && !UTGS->HasMatchEnded() && ((FVector::DotProduct(CameraDir, (WorldPosition - CameraPosition)) > 0.0f) || !ViewerPS->CarriedObject)
		 && (UTPC->MyUTHUD == nullptr || !UTPC->MyUTHUD->bShowScores))
	{
		float TextXL, YL;
		float Scale = Canvas->ClipX / 1920.f;
		UFont* SmallFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->SmallFont;
		FText RallyText = RallyBeaconText; 
		if ((RallyPointState == RallyPointStates::Powered) && MyGameVolume)
		{
			FFormatNamedArguments Args;
			Args.Add("RallyLoc", LocationText.IsEmpty() ? MyGameVolume->VolumeName : LocationText);
			RallyText = bViewerIsAttacking ? FText::Format(NSLOCTEXT("UTRallyPoint", "RallyAtLoc", " RALLY at {RallyLoc} "), Args) : FText::Format(NSLOCTEXT("UTRallyPoint", "EnemyRallyAtLoc", " ENEMY RALLY at {RallyLoc} "), Args);
		}
		Canvas->TextSize(SmallFont, RallyText.ToString(), TextXL, YL, Scale, Scale);
		FVector ViewDir = UTPC->GetControlRotation().Vector();
		float Dist = (CameraPosition - GetActorLocation()).Size();
		bool bDrawEdgeArrow = false; 
		FVector ScreenPosition = GetAdjustedScreenPosition(Canvas, WorldPosition, CameraPosition, ViewDir, Dist, 20.f, bDrawEdgeArrow);
		float XPos = ScreenPosition.X - 0.5f*TextXL;
		float YPos = ScreenPosition.Y - YL;
		if (XPos < Canvas->ClipX || XPos + TextXL < 0.0f)
		{
			FLinearColor TeamColor = FLinearColor::Yellow;
			float CenterFade = 1.f;
			float PctFromCenter = (ScreenPosition - FVector(0.5f*Canvas->ClipX, 0.5f*Canvas->ClipY, 0.f)).Size() / Canvas->ClipX;
			CenterFade = CenterFade * FMath::Clamp(10.f*PctFromCenter, 0.15f, 1.f);
			TeamColor.A = 0.2f * CenterFade;
			UTexture* BarTexture = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->HUDAtlas;

			Canvas->SetLinearDrawColor(TeamColor);
			float Border = 2.f*Scale;
			float Height = 0.75*YL + 0.7f * YL;
			float Width = TextXL + 2.f*Border;
			float PartialFill = (RallyPointState == RallyPointStates::Powered) ? RallyTimeRemaining/FMath::Max(MinimumRallyTime, 0.1f) : 1.f;
			Canvas->DrawTile(Canvas->DefaultTexture, XPos - Border, YPos - YL - Border, Width * PartialFill, Height + 2.f*Border, 0, 0, 1, 1);
			if (PartialFill < 1.f)
			{
				Canvas->SetLinearDrawColor(FLinearColor(0.3f, 0.3f, 0.3f, TeamColor.A));
				Canvas->DrawTile(Canvas->DefaultTexture, XPos - Border + Width*PartialFill, YPos - YL - Border, Width * (1.f -PartialFill), Height + 2.f*Border, 0, 0, 1, 1);
			}

			FLinearColor BeaconTextColor = FLinearColor::White;
			BeaconTextColor.A = 0.6f * CenterFade;
			FUTCanvasTextItem TextItem(FVector2D(FMath::TruncToFloat(Canvas->OrgX + XPos), FMath::TruncToFloat(Canvas->OrgY + YPos - 1.2f*YL)), RallyText, SmallFont, BeaconTextColor, NULL);
			TextItem.Scale = FVector2D(Scale, Scale);
			TextItem.BlendMode = SE_BLEND_Translucent;
			FLinearColor ShadowColor = FLinearColor::Black;
			ShadowColor.A = BeaconTextColor.A;
			TextItem.EnableShadow(ShadowColor);
			TextItem.FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
			Canvas->DrawItem(TextItem);

			FFormatNamedArguments Args;
			FText NumberText = FText::AsNumber(int32(0.01f*Dist));
			UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
			Canvas->TextSize(TinyFont, TEXT("XX meters"), TextXL, YL, Scale, Scale);
			Args.Add("Dist", NumberText);
			FText DistText = NSLOCTEXT("UTRallyPoint", "DistanceText", "{Dist} meters");
			TextItem.Font = TinyFont;
			TextItem.Text = FText::Format(DistText, Args);
			TextItem.Position.X = ScreenPosition.X - 0.5f*TextXL;
			TextItem.Position.Y += 0.9f*YL;

			Canvas->DrawItem(TextItem);
		}
	}
}

// FIXMESTEVE show distance and triangle


