// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeaponStateZooming.h"

UUTWeaponStateZooming::UUTWeaponStateZooming(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bDrawHeads = true;
	bDrawPingAdjustedTargets = true;
}

void UUTWeaponStateZooming::EndState()
{
	bDelayShot = false;
	ToggleLoopingEffects(false);
	GetOuterAUTWeapon()->GetWorldTimerManager().ClearAllTimersForObject(this);
}

void UUTWeaponStateZooming::PendingFireStarted()
{
	if (GetOuterAUTWeapon()->ZoomState != EZoomState::EZS_NotZoomed)
	{
		GetOuterAUTWeapon()->SetZoomState(EZoomState::EZS_NotZoomed);
	}
	else
	{
		GetOuterAUTWeapon()->SetZoomState(EZoomState::EZS_ZoomingIn);
	}
}

void UUTWeaponStateZooming::PendingFireStopped()
{
	if (GetOuterAUTWeapon()->ZoomState != EZoomState::EZS_NotZoomed)
	{
		GetOuterAUTWeapon()->SetZoomState(EZoomState::EZS_Zoomed);
	}
}

bool UUTWeaponStateZooming::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	// this isn't actually firing so immediately switch to other fire mode
	if (FireModeNum != GetOuterAUTWeapon()->GetCurrentFireMode() && GetOuterAUTWeapon()->FiringState.IsValidIndex(FireModeNum) && GetOuterAUTWeapon()->HasAmmo(FireModeNum))
	{
		GetOuterAUTWeapon()->GotoFireMode(FireModeNum);
	}
	return false;
}

void UUTWeaponStateZooming::EndFiringSequence(uint8 FireModeNum)
{
	GetOuterAUTWeapon()->GotoActiveState();
}

void UUTWeaponStateZooming::WeaponBecameInactive()
{
	if (GetOuterAUTWeapon()->ZoomState != EZoomState::EZS_NotZoomed 
		&& GetUTOwner() != nullptr && GetUTOwner()->Controller != nullptr) //Spectator weapons are alwyas inactive 
	{
		GetOuterAUTWeapon()->SetZoomState(EZoomState::EZS_NotZoomed);
	}
}

bool UUTWeaponStateZooming::DrawHUD(UUTHUDWidget* WeaponHudWidget)
{
	if (GetOuterAUTWeapon()->ZoomState != EZoomState::EZS_NotZoomed && OverlayMat != NULL)
	{
		UCanvas* C = WeaponHudWidget->GetCanvas();
		AUTPlayerController* UTPC = GetUTOwner()->GetLocalViewer();
		if (UTPC && UTPC->IsBehindView())
		{
			// no zoom overlay in 3rd person
			return true;
		}
		if (OverlayMI == NULL)
		{
			OverlayMI = UMaterialInstanceDynamic::Create(OverlayMat, this);
		}
		FCanvasTileItem Item(FVector2D(0.0f, 0.0f), OverlayMI->GetRenderProxy(false), FVector2D(C->ClipX, C->ClipY));
		// expand X axis size to be widest supported aspect ratio (16:9)
		float OrigSizeX = Item.Size.X;
		Item.Size.X = FMath::Max<float>(Item.Size.X, Item.Size.Y * 16.0f / 9.0f);
		Item.Position.X -= (Item.Size.X - OrigSizeX) * 0.5f;
		Item.UV0 = FVector2D(0.0f, 0.0f);
		Item.UV1 = FVector2D(1.0f, 1.0f);
		C->DrawItem(Item);

		if (bDrawHeads && TargetIndicator != NULL)
		{
			float HeadScale = GetOuterAUTWeapon()->GetHeadshotScale(nullptr);
			if (HeadScale > 0.0f)
			{
				AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
				APlayerState* OwnerState = GetUTOwner()->PlayerState;
				float WorldTime = GetWorld()->TimeSeconds;
				FVector FireStart = GetOuterAUTWeapon()->GetFireStartLoc();
				for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
				{
					AUTCharacter* EnemyChar = Cast<AUTCharacter>(*It);
					if (EnemyChar != NULL && !EnemyChar->IsDead() && !EnemyChar->IsInvisible() && !EnemyChar->IsFeigningDeath() && (EnemyChar->GetMesh()->LastRenderTime > WorldTime - 0.25f) && EnemyChar != GetUTOwner() && (GS == NULL || !GS->OnSameTeam(EnemyChar, GetUTOwner())))
					{
						FVector HeadLoc = EnemyChar->GetHeadLocation();
						static FName NAME_SniperZoom(TEXT("SniperZoom"));
						if (!GetWorld()->LineTraceTestByChannel(FireStart, HeadLoc, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionQueryParams(NAME_SniperZoom, true, GetUTOwner())))
						{
							bool bDrawPingAdjust = bDrawPingAdjustedTargets && OwnerState != NULL && !EnemyChar->GetVelocity().IsZero();
							float NetPing = 0.0f;
							if (bDrawPingAdjust)
							{
								NetPing = 0.001f * (OwnerState->ExactPing - (UTPC ? UTPC->MaxPredictionPing : 0.f));
								bDrawPingAdjust = NetPing > 0.f;
							}
							float HeadRadius = EnemyChar->HeadRadius * EnemyChar->HeadScale * HeadScale;
							if (EnemyChar->UTCharacterMovement && EnemyChar->UTCharacterMovement->bIsFloorSliding)
							{
								HeadRadius = EnemyChar->HeadRadius * EnemyChar->HeadScale;
							}
							for (int32 i = 0; i < (bDrawPingAdjust ? 2 : 1); i++)
							{
								FVector Perpendicular = (HeadLoc - FireStart).GetSafeNormal() ^ FVector(0.0f, 0.0f, 1.0f);
								FVector PointA = C->Project(HeadLoc + Perpendicular * HeadRadius);
								FVector PointB = C->Project(HeadLoc - Perpendicular * HeadRadius);
								FVector2D UpperLeft(FMath::Min<float>(PointA.X, PointB.X), FMath::Min<float>(PointA.Y, PointB.Y));
								FVector2D BottomRight(FMath::Max<float>(PointA.X, PointB.X), FMath::Max<float>(PointA.Y, PointB.Y));
								float MidY = (UpperLeft.Y + BottomRight.Y) * 0.5f;

								// skip drawing if too off-center
								if ((FMath::Abs(MidY - 0.5f*C->SizeY) < 0.11f*C->SizeY) && (FMath::Abs(0.5f*(UpperLeft.X + BottomRight.X) - 0.5f*C->SizeX) < 0.11f*C->SizeX))
								{
									// square-ify
									float SizeY = FMath::Max<float>(MidY - UpperLeft.Y, (BottomRight.X - UpperLeft.X) * 0.5f);
									UpperLeft.Y = MidY - SizeY;
									BottomRight.Y = MidY + SizeY;
									FLinearColor TargetColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.5f);
									FCanvasTileItem HeadCircleItem(UpperLeft, TargetIndicator->Resource, BottomRight - UpperLeft, (i == 0) ? TargetColor : FLinearColor(0.7f, 0.7f, 0.7f, 0.5f));
									HeadCircleItem.BlendMode = SE_BLEND_Translucent;
									C->DrawItem(HeadCircleItem);
								}
								if (OwnerState != NULL)
								{
									HeadLoc += EnemyChar->GetVelocity() * NetPing;
								}
							}
						}
					}
				}
			}
		}

		// temp until there's a decent crosshair in the material
		return true;
	}
	else
	{
		return true;
	}
}

void UUTWeaponStateZooming::OnZoomingFinished()
{
	ToggleZoomInSound(false);
}

void UUTWeaponStateZooming::ToggleZoomInSound(bool bNowOn)
{
	if (GetOuterAUTWeapon()->GetNetMode() != NM_DedicatedServer)
	{
		if (ZoomLoopSound != NULL)
		{
			if (ZoomLoopComp == NULL)
			{
				ZoomLoopComp = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass());
				ZoomLoopComp->bAutoDestroy = false;
				ZoomLoopComp->bAutoActivate = false;
				ZoomLoopComp->Sound = ZoomLoopSound;
				ZoomLoopComp->bAllowSpatialization = false;
			}
			if (bNowOn)
			{
				ZoomLoopComp->RegisterComponent();
				// note we don't need to attach to anything because we disabled spatialization
				ZoomLoopComp->Play();
			}
			else if (ZoomLoopComp->IsRegistered())
			{
				ZoomLoopComp->Stop();
				ZoomLoopComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				ZoomLoopComp->UnregisterComponent();
			}
		}
	}
}

bool UUTWeaponStateZooming::WillSpawnShot(float DeltaTime)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetOuterAUTWeapon()->GetUTOwner()->GetController());
	return UTPC && UTPC->HasDeferredFireInputs();
}
