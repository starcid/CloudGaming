// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUD.h"
#include "UTLocalPlayer.h"
#include "UTPlayerState.h"
#include "UTPlayerController.h"
#include "UTCharacterMovement.h"
#include "ActiveSound.h"
#include "AudioDevice.h"
#include "UTPickup.h"
#include "UTPickupInventory.h"
#include "UTPickupWeapon.h"
#include "UTAnnouncer.h"
#include "UTHUDWidgetMessage.h"
#include "UTPlayerInput.h"
#include "UTPlayerCameraManager.h"
#include "UTCheatManager.h"
#include "UTSpreeMessage.h"
#include "UTCTFGameMessage.h"
#include "UTCTFRewardMessage.h"
#include "UTCountDownMessage.h"
#include "UTDeathMessage.h"
#include "UTPickupMessage.h"
#include "UTMultiKillMessage.h"
#include "UTVictimMessage.h"
#include "UTCTFMajorMessage.h"
#include "UTGameMode.h"
#include "UTWeap_Translocator.h"
#include "UTWeap_Enforcer.h"
#include "UTCTFGameMode.h"
#include "UTCarriedObject.h"
#include "UTCharacterContent.h"
#include "UTImpactEffect.h"
#include "UTPathTestBot.h"
#include "BlueprintContextLibrary.h"
#include "MatchmakingContext.h"
#include "UTProj_Rocket.h"
#include "UTRewardMessage.h"
#include "UTShowdownRewardMessage.h"
#include "UTMcpUtils.h"
#include "UTKillerMessage.h"
#include "UTCTFGameMessage.h"
#include "UTTutorialAnnouncement.h"
#include "UTFlagRunMessage.h"
#include "UTDoorMessage.h"
#include "StatNames.h"

#if WITH_PROFILE
#include "OnlineSubsystemMcp.h"
#include "GameServiceMcp.h"
#endif

UUTCheatManager::UUTCheatManager(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUTCheatManager::WOff(float F)
{
	AUTCharacter* MyPawn = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (MyPawn)
	{
		UE_LOG(UT, Warning, TEXT("OFFSET %f"), F);
		MyPawn->GetWeapon()->LowMeshOffset.Z = F;
	}
}

void UUTCheatManager::Ann(int32 Switch)
{
	// play an announcement for testing
/*	AUTCTFGameMode* CTF = GetWorld()->GetAuthGameMode<AUTCTFGameMode>();
	AUTCarriedObject* Flag = CTF->CTFGameState->FlagBases[0]->GetCarriedObject();
	AUTPlayerState* Holder = Cast<AUTPlayerState>(GetOuterAPlayerController()->PlayerState);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(1, NULL, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	CTF->BroadcastScoreUpdate(Holder, Holder->Team);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(1, NULL, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	CTF->BroadcastScoreUpdate(Holder, Holder->Team);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(1, NULL, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(1, NULL, NULL);
	Flag->SendGameMessage(4, Holder, NULL);
	Flag->SendGameMessage(3, Holder, NULL);
	Flag->SendGameMessage(1, NULL, NULL);*/
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCTFRewardMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	/*GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTSpreeMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTVictimMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTKillerMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCTFGameMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTPickupMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);

		
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTSpreeMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTDeathMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTPickupMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTMultiKillMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTVictimMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCTFRewardMessage::StaticClass(), Switch, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
*/
}

void UUTCheatManager::NoOutline()
{
	for (FConstPawnIterator It = GetOuterAPlayerController()->GetWorld()->GetPawnIterator(); It; ++It)
	{
		AUTCharacter* Char = Cast<AUTCharacter>(*It);
		if (Char)
		{
			Char->bForceNoOutline = true;
		}
	}
}

void UUTCheatManager::HL()
{
	AUTHUD* HLHUD = Cast<AUTHUD>(GetOuterAPlayerController()->MyHUD);
	if (HLHUD)
	{
		GetOuterAUTPlayerController()->UTPlayerState->MatchHighlights[0] = HighlightNames::TopScorer;
		GetOuterAUTPlayerController()->UTPlayerState->MatchHighlights[1] = HighlightNames::MostHeadShots;
		GetOuterAUTPlayerController()->UTPlayerState->MatchHighlights[2] = NAME_AirRox;
		GetOuterAUTPlayerController()->UTPlayerState->MatchHighlights[3] = NAME_MultiKillLevel3;
		HLHUD->OpenMatchSummary();
	}
}

void UUTCheatManager::AnnM(float F)
{
	AnnCount = 0;
	AnnDelay = F;
	NextAnn();
}

void UUTCheatManager::NextAnn()
{
	FTimerHandle TempHandle;
	if (AnnCount == 0)
	{
		GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTMultiKillMessage::StaticClass(), 3, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
		GetOuterAPlayerController()->GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UUTCheatManager::NextAnn, AnnDelay, false);
	}
	else if (AnnCount == 1)
	{
		GetOuterAUTPlayerController()->UTPlayerState->AnnounceStatus(StatusMessage::ImOnDefense);
		GetOuterAPlayerController()->GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UUTCheatManager::NextAnn, AnnDelay, false);
	}
	else if (AnnCount == 2)
	{
		GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTSpreeMessage::StaticClass(), 1, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
		GetOuterAPlayerController()->GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UUTCheatManager::NextAnn, AnnDelay, false);
	}
	else if (AnnCount == 3)
	{
		GetOuterAUTPlayerController()->UTPlayerState->AnnounceStatus(StatusMessage::IGotFlag);
		GetOuterAPlayerController()->GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UUTCheatManager::NextAnn, AnnDelay, false);
	}
	else if (AnnCount == 4)
	{
		GetOuterAPlayerController()->ClientReceiveLocalizedMessage(UUTCTFRewardMessage::StaticClass(), 0, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
		GetOuterAPlayerController()->GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UUTCheatManager::NextAnn, AnnDelay, false);
	}
	else if (AnnCount == 5)
	{
		GetOuterAUTPlayerController()->UTPlayerState->AnnounceStatus(StatusMessage::NeedBackup);
//		GetOuterAPlayerController()->ClientReceiveLocalizedMessage(AUTProj_Rocket::StaticClass()->GetDefaultObject<AUTProj_Rocket>()->AirRocketRewardClass, 0, GetOuterAPlayerController()->PlayerState, GetOuterAPlayerController()->PlayerState, NULL);
	}
	AnnCount++;
}

void UUTCheatManager::Spread(float Scaling)
{
	AUTCharacter* MyPawn = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (!MyPawn)
	{
		return;
	}
	for (TInventoryIterator<AUTWeapon> It(MyPawn); It; ++It)
	{
		for (int32 i = 0; i < It->Spread.Num(); i++)
		{
			It->Spread[i] = It->GetClass()->GetDefaultObject<AUTWeapon>()->Spread[i] * Scaling;
			if (It->Spread[i] != 0.f)
			{
				UE_LOG(UT, Warning, TEXT("%s New Spread %d is %f"), *It->GetName(), i, It->Spread[i]);
			}
		}
	}
}

void UUTCheatManager::AllAmmo()
{
	AUTCharacter* MyPawn = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (MyPawn != NULL)
	{
		MyPawn->AllAmmo();
	}
}

void UUTCheatManager::Gibs()
{
	AUTCharacter* MyPawn = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (MyPawn != NULL)
	{
		if (MyPawn->CharacterData.GetDefaultObject()->GibExplosionEffect != NULL)
		{
			MyPawn->CharacterData.GetDefaultObject()->GibExplosionEffect.GetDefaultObject()->SpawnEffect(MyPawn->GetWorld(), MyPawn->GetRootComponent()->GetComponentTransform(), MyPawn->GetMesh(), MyPawn, NULL, SRT_None);
		}
		for (const FGibSlotInfo& GibInfo : MyPawn->CharacterData.GetDefaultObject()->Gibs)
		{
			MyPawn->SpawnGib(GibInfo, *MyPawn->LastTakeHitInfo.DamageType);
		}
		MyPawn->TeleportTo(MyPawn->GetActorLocation() - 800.f * MyPawn->GetActorRotation().Vector(), MyPawn->GetActorRotation(), true);
	}
}

void UUTCheatManager::UnlimitedAmmo()
{
	AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (Game != NULL)
	{
		Game->bAmmoIsLimited = false;
	}
}

void UUTCheatManager::Loaded()
{
	AUTCharacter* MyPawn = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (MyPawn != NULL)
	{
		// grant all weapons that are in memory
		for (TObjectIterator<UClass> It; It; ++It)
		{
			// make sure we don't use abstract, deprecated, or blueprint skeleton classes
			if (It->IsChildOf(AUTWeapon::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) && !It->GetName().StartsWith(TEXT("SKEL_")) && !It->IsChildOf(AUTWeap_Translocator::StaticClass()))
			{
				UClass* TestClass = *It;
				if (!MyPawn->FindInventoryType(TSubclassOf<AUTInventory>(*It), true))
				{
					MyPawn->AddInventory(MyPawn->GetWorld()->SpawnActor<AUTInventory>(*It, FVector(0.0f), FRotator(0, 0, 0)), true);
				}
			}
		}
		AUTWeap_Enforcer* Enforcer = Cast<AUTWeap_Enforcer>(MyPawn->FindInventoryType(TSubclassOf<AUTInventory>(AUTWeap_Enforcer::StaticClass()), false));
		if (Enforcer)
		{
			Enforcer->BecomeDual();
		}
		MyPawn->AllAmmo();
	}
}

void UUTCheatManager::ua()
{
	UnlimitedAmmo();
}

void UUTCheatManager::SetHat(const FString& Hat)
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(GetOuterAPlayerController()->PlayerState);
	if (PS)
	{
		FString HatPackageName;
		if (FPackageName::SearchForPackageOnDisk(Hat, &HatPackageName))
		{
			HatPackageName += TEXT(".") + Hat + TEXT("_C");
			PS->ServerReceiveHatClass(HatPackageName);
		}
	}
}

void UUTCheatManager::ImpartHats(const FString& Hat)
{
	AUTGameState* GS = GetOuterAPlayerController()->GetWorld()->GetGameState<AUTGameState>();
	for (int32 i = 0; i < GS->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
		if (PS)
		{
			FString HatPackageName;
			if (FPackageName::SearchForPackageOnDisk(Hat, &HatPackageName))
			{
				HatPackageName += TEXT(".") + Hat + TEXT("_C");
				PS->ServerReceiveHatClass(HatPackageName);
			}
		}
	}
}

void UUTCheatManager::ImpartWeaponSkin(const FString& Skin)
{
	FString SkinPackageName;
	if (FPackageName::SearchForPackageOnDisk(Skin, &SkinPackageName))
	{
		AUTGameState* GS = GetOuterAPlayerController()->GetWorld()->GetGameState<AUTGameState>();
		for (int32 i = 0; i < GS->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
			if (PS)
			{
				PS->ServerReceiveWeaponSkin(SkinPackageName);
			}
		}
	}
}

void UUTCheatManager::SetEyewear(const FString& Eyewear)
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(GetOuterAPlayerController()->PlayerState);
	if (PS)
	{
		FString EyewearPackageName;
		if (FPackageName::SearchForPackageOnDisk(Eyewear, &EyewearPackageName))
		{
			EyewearPackageName += TEXT(".") + Eyewear + TEXT("_C");
			PS->ServerReceiveEyewearClass(EyewearPackageName);
		}
	}
}

void UUTCheatManager::ImpartEyewear(const FString& Eyewear)
{
	AUTGameState* GS = GetOuterAPlayerController()->GetWorld()->GetGameState<AUTGameState>();
	for (int32 i = 0; i < GS->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
		if (PS)
		{
			FString EyewearPackageName;
			if (FPackageName::SearchForPackageOnDisk(Eyewear, &EyewearPackageName))
			{
				EyewearPackageName += TEXT(".") + Eyewear + TEXT("_C");
				PS->ServerReceiveEyewearClass(EyewearPackageName);
			}
		}
	}
}

void UUTCheatManager::BugItWorker(FVector TheLocation, FRotator TheRotation)
{
	Super::BugItWorker(TheLocation, TheRotation);

	// The teleport doesn't play nice with our physics, so just go to walk mode rather than fall through the world
	Walk();
}

void UUTCheatManager::PlayTaunt(const FString& TauntClass)
{
	AUTCharacter* UTChar = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (UTChar != NULL)
	{
		TSubclassOf<AUTTaunt> TauntClassInstance = nullptr;

		FString TauntPackageName;
		if (FPackageName::SearchForPackageOnDisk(TauntClass, &TauntPackageName))
		{
			TauntPackageName += TEXT(".") + TauntClass + TEXT("_C");
			TauntClassInstance = LoadClass<AUTTaunt>(NULL, *TauntPackageName, NULL, LOAD_None, NULL);
		}

		if (!TauntClassInstance)
		{
			FString MoreSpecificTauntClass = TEXT("/Game/RestrictedAssets/Blueprints/Taunts/") + TauntClass;
			if (FPackageName::SearchForPackageOnDisk(MoreSpecificTauntClass, &TauntPackageName))
			{
				TauntPackageName += TEXT(".") + TauntClass + TEXT("_C");
				TauntClassInstance = LoadClass<AUTTaunt>(NULL, *TauntPackageName, NULL, LOAD_None, NULL);
			}
		}

		if (TauntClassInstance)
		{
			UTChar->PlayTauntByClass(TauntClassInstance);
		}
	}
}

void UUTCheatManager::SetChar(const FString& NewChar)
{
	AUTGameState* GS = GetOuterAPlayerController()->GetWorld()->GetGameState<AUTGameState>();
	for (int32 i = 0; i < GS->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
		if (PS)
		{
			bool bFoundCharacterContent = false;
			FString NewCharPackageName;
			if (FPackageName::SearchForPackageOnDisk(NewChar, &NewCharPackageName))
			{
				NewCharPackageName += TEXT(".") + NewChar + TEXT("_C");
				if (LoadClass<AUTCharacterContent>(NULL, *NewCharPackageName, NULL, LOAD_None, NULL))
				{
					bFoundCharacterContent = true;
				}
			}

			FString NewCharAlt = NewChar + TEXT("CharacterContent");
			if (!bFoundCharacterContent && FPackageName::SearchForPackageOnDisk(NewCharAlt, &NewCharPackageName))
			{
				NewCharPackageName += TEXT(".") + NewCharAlt + TEXT("_C");
				if (LoadClass<AUTCharacterContent>(NULL, *NewCharPackageName, NULL, LOAD_None, NULL))
				{
					bFoundCharacterContent = true;
				}
			}

			FString NewCharAlt2 = NewChar + TEXT("Character");
			if (!bFoundCharacterContent && FPackageName::SearchForPackageOnDisk(NewCharAlt2, &NewCharPackageName))
			{
				NewCharPackageName += TEXT(".") + NewCharAlt2 + TEXT("_C");
				if (LoadClass<AUTCharacterContent>(NULL, *NewCharPackageName, NULL, LOAD_None, NULL))
				{
					bFoundCharacterContent = true;
				}
			}

			if (bFoundCharacterContent)
			{
				PS->ServerSetCharacter(NewCharPackageName);
			}
		}
	}
}

void UUTCheatManager::God()
{
	AUTCharacter* UTChar = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (UTChar != NULL)
	{
		if (UTChar->bDamageHurtsHealth)
		{
			UTChar->bDamageHurtsHealth = false;
			GetOuterAPlayerController()->ClientMessage(TEXT("God mode on"));
		}
		else
		{
			UTChar->bDamageHurtsHealth = true;
			GetOuterAPlayerController()->ClientMessage(TEXT("God Mode off"));
		}
	}
	else
	{
		Super::God();
	}
}

void UUTCheatManager::UnlimitedHealth()
{
	AUTCharacter* UTChar = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (UTChar != NULL)
	{
		if (UTChar->bUnlimitedHealth)
		{
			UTChar->bUnlimitedHealth = false;
			GetOuterAPlayerController()->ClientMessage(TEXT("Unlimited health off"));
		}
		else
		{
			UTChar->bUnlimitedHealth = true;
			GetOuterAPlayerController()->ClientMessage(TEXT("Unlimited health on"));
		}
	}
}

void UUTCheatManager::Teleport()
{
	AUTCharacter* UTChar = Cast<AUTCharacter>(GetOuterAPlayerController()->GetPawn());
	if (UTChar != NULL)
	{
		FHitResult Hit(1.f);
		ECollisionChannel TraceChannel = COLLISION_TRACE_WEAPONNOCHARACTER;
		FCollisionQueryParams QueryParams(GetClass()->GetFName(), true, UTChar);
		FVector StartLocation(0.f);
		FRotator SpawnRotation(0.f);
		UTChar->GetActorEyesViewPoint(StartLocation, SpawnRotation);
		const FVector EndTrace = StartLocation + SpawnRotation.Vector() * 20000.f;

		if (!GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndTrace, TraceChannel, QueryParams))
		{
			Hit.Location = EndTrace;
		}
		UTChar->TeleportTo(Hit.Location, SpawnRotation);
	}
}

void UUTCheatManager::McpGrantItem(const FString& ItemId)
{
	/*
	if (GetOuterAUTPlayerController()->McpProfile)
	{
		TArray<FString> ItemList;
		ItemList.Push(ItemId);
		FDevCheatUrlContext Context(FMcpQueryComplete::CreateUObject(this, &UUTCheatManager::LogWebResponse));
		GetOuterAUTPlayerController()->McpProfile->GrantItems(ItemList, 1, Context);
	}*/
}

void UUTCheatManager::McpDestroyItem(const FString& ItemId)
{
	/*
	if (GetOuterAUTPlayerController()->McpProfile)
	{
		FDevCheatUrlContext Context(FMcpQueryComplete::CreateUObject(this, &UUTCheatManager::LogWebResponse));
		GetOuterAUTPlayerController()->McpProfile->DestroyItems(ItemId, 1, Context);
	}*/
}

#if WITH_PROFILE

void UUTCheatManager::LogWebResponse(const FMcpQueryResult& Response)
{
	/*
	if (!Response.bSucceeded)
	{
		GetOuterAPlayerController()->ClientMessage(TEXT("Cheat failed"));
		UE_LOG(UT, Warning, TEXT("%s"), *Response.ErrorMessage.ToString());
	}
	else
	{
		GetOuterAPlayerController()->ClientMessage(TEXT("Cheat succeeded"));
	}*/
}

#endif

void UUTCheatManager::McpCheat()
{
#if WITH_PROFILE
	AUTPlayerController* const MyPC = GetOuterAUTPlayerController();
	UUtMcpProfile* McpProfile = MyPC->GetMcpProfile();
	if (McpProfile)
	{
		FOnlineSubsystemMcp* OnlineSubMcp = McpProfile->GetOnlineSubMcp();
		if (OnlineSubMcp && OnlineSubMcp->GetMcpGameService().IsValid())
		{
			FString ServiceUrl = OnlineSubMcp->GetMcpGameService()->GetBaseUrl();

			// this is a bit of a hack, but it's for a cheat...
			ServiceUrl.ReplaceInline(TEXT("-public-service"), TEXT("-admin-service"));
			ServiceUrl.ReplaceInline(TEXT(".epicgames.com/"), TEXT(".epicgames.net/"));
			ServiceUrl += TEXT("/admin#");
			ServiceUrl += McpProfile->GetProfileGroup().GetGameAccountId().ToString();

			// open the url in the browser
			FPlatformProcess::LaunchURL(*ServiceUrl, nullptr, nullptr);
		}
	}
#endif
}

void UUTCheatManager::McpRefreshProfile()
{
#if WITH_PROFILE
	AUTPlayerController* const MyPC = GetOuterAUTPlayerController();
	UUtMcpProfile* McpProfile = MyPC->GetMcpProfile();
	if (McpProfile)
	{
		McpProfile->ForceQueryProfile(FMcpQueryComplete());
	}
#endif
}

void UUTCheatManager::MatchmakeMyParty(int32 PlaylistId)
{
	UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UMatchmakingContext::StaticClass()));
	if (MatchmakingContext)
	{
		MatchmakingContext->StartMatchmaking(PlaylistId);
	}
}

void UUTCheatManager::ViewBot()
{
	if (GetOuterAUTPlayerController()->Role == ROLE_Authority)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS->PlayerArray.Num() > 0)
		{
			int32 CurrentIndex = -1;
			APawn* PTarget = Cast<APawn>(GetOuterAUTPlayerController()->GetViewTarget());
			if (PTarget != nullptr && PTarget->PlayerState != nullptr)
			{
				CurrentIndex = GS->PlayerArray.Find(PTarget->PlayerState);
			}
			int32 Start = CurrentIndex;
			bool bFoundBot = false;
			do
			{
				CurrentIndex++;
				if (CurrentIndex >= GS->PlayerArray.Num())
				{
					CurrentIndex = 0;
				}
				bFoundBot = GS->PlayerArray[CurrentIndex] != nullptr && Cast<AUTBot>(GS->PlayerArray[CurrentIndex]->GetOwner()) != nullptr;
			} while (CurrentIndex != Start && !bFoundBot);
			
			if (bFoundBot)
			{
				GetOuterAUTPlayerController()->BehindView(GetOuterAUTPlayerController()->bSpectateBehindView);
				GetOuterAUTPlayerController()->SetViewTarget(GS->PlayerArray[CurrentIndex]);
			}
			else
			{
				ViewSelf();
			}
		}
	}
}

void UUTCheatManager::TestPaths(bool bHighJumps, bool bWallDodges, bool bLifts, bool bLiftJumps)
{
	AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (Game != NULL)
	{
		Game->KillBots();
		AUTPathTestBot* NewBot = GetWorld()->SpawnActor<AUTPathTestBot>();
		if (NewBot != NULL)
		{
			static int32 NameCount = 0;
			NewBot->PlayerState->SetPlayerName(FString(TEXT("PathTestBot")) + ((NameCount > 0) ? FString::Printf(TEXT("_%i"), NameCount) : TEXT("")));
			NameCount++;
			Game->NumBots++;
			Game->BotFillCount = Game->NumPlayers + Game->NumBots;
			Game->ChangeTeam(NewBot, 0);
			Game->GenericPlayerInitialization(NewBot);

			NewBot->GatherTestList(bHighJumps, bWallDodges, bLifts, bLiftJumps);

			GetOuterAPlayerController()->SetViewTarget(NewBot->PlayerState);
		}
	}
}

void UUTCheatManager::McpGetVersion()
{
#if WITH_PROFILE
	UMcpProfileGroup* McpProfileGroup = GetOuterAUTPlayerController()->GetMcpProfileManager()->GetMcpProfileGroup();
	if (McpProfileGroup)
	{
		FString VersionString = McpProfileGroup->GetMcpVersion();
		UE_LOG(UT, Display, TEXT("MCP-Version = %s"), *VersionString);
		FPlatformMisc::ClipboardCopy(*VersionString);
	}
	else
	{
		UE_LOG(UT, Display, TEXT("No MCP version available."));
	}
#endif
}

void UUTCheatManager::CheatShowRankedReconnectDialog()
{
#if !UE_BUILD_SHIPPING
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GetOuterAPlayerController()->Player);
	if (LP)
	{
		LP->LastRankedMatchTimeString = FDateTime::Now().ToString();
		LP->ShowRankedReconnectDialog(LP->LastRankedMatchSessionId);
	}
#endif
}

void UUTCheatManager::DebugAchievement(FString AchievementName)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GetOuterAPlayerController()->Player);
	if (LP)
	{
		LP->AwardAchievement(FName(*AchievementName));
		LP->SaveProgression();
	}
#endif
}

void UUTCheatManager::UnlimitedPowerupUses()
{
	AUTPlayerController* UTPlayerController = Cast<AUTPlayerController>(GetOuterAPlayerController());
	if (UTPlayerController && UTPlayerController->UTPlayerState)
	{
		UTPlayerController->UTPlayerState->SetRemainingBoosts(255);
	}
}

void UUTCheatManager::ReportWaitTime(FString RatingType, int32 Seconds)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), TSharedPtr<const FUniqueNetId>());
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to report wait time."));
		return;
	}

	// tell MCP about the match to update players' MMRs
	McpUtils->ReportWaitTime(RatingType, Seconds, [this, RatingType](const FOnlineError& Result) {
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
			UE_LOG(UT, Warning, TEXT("Failed to report wait time to the server. (%d) %s %s"), Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			UE_LOG(UT, Log, TEXT("Wait time reported to backend"));
		}
	});
#endif
}

void UUTCheatManager::EstimateWaitTimes()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), TSharedPtr<const FUniqueNetId>());
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to report wait time."));
		return;
	}

	McpUtils->GetEstimatedWaitTimes([this](const FOnlineError& Result, const FEstimatedWaitTimeInfo& EstimateWaitTimeInfo) {
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
			UE_LOG(UT, Warning, TEXT("Failed to get estimated wait times from the server. (%d) %s %s"), Result.HttpResult, *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			for (int32 i = 0; i < EstimateWaitTimeInfo.WaitTimes.Num(); i++)
			{
				UE_LOG(UT, Log, TEXT("%s %f"), *EstimateWaitTimeInfo.WaitTimes[i].RatingType, EstimateWaitTimeInfo.WaitTimes[i].AverageWaitTimeSecs);
			}
		}
	});
#endif

}

void UUTCheatManager::UnlockTutorials()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GetOuterAPlayerController()->Player);
	if (LP != nullptr)
	{
		LP->SetTutorialMask(TUTORIAL_All);
	}
#endif
}

void UUTCheatManager::TestAMDAllocation()
{
#if WITH_EDITOR
	FTexture2DRHIRef ReadbackTextures[2];
	int32 VideoWidth = 1920;
	int32 VideoHeight = 1080;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FWebMRecordCreateBufers,
		int32, InVideoWidth, VideoWidth,
		int32, InVideoHeight, VideoHeight,
		FTexture2DRHIRef*, InReadbackTextures, ReadbackTextures,
		{
			for (int32 TextureIndex = 0; TextureIndex < 2; ++TextureIndex)
			{
				FRHIResourceCreateInfo CreateInfo;
				InReadbackTextures[TextureIndex] = RHICreateTexture2D(
					InVideoWidth,
					InVideoHeight,
					PF_B8G8R8A8,
					1,
					1,
					TexCreate_CPUReadback,
					CreateInfo
					);
			}
		});
	FlushRenderingCommands();
	
	UE_LOG(UT, Warning, TEXT("Allocated readback textures"));

	FIntPoint ResizeTo(1920, 1080);
	static const FName RendererModuleName("Renderer");
	IRendererModule& RendererModuleRef = FModuleManager::GetModuleChecked<IRendererModule>(RendererModuleName);
	IRendererModule* RendererModule = &RendererModuleRef;

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		ReadSurfaceCommand,
		FTexture2DRHIRef*, InReadbackTextures, ReadbackTextures,
		IRendererModule*, InRenderModule, RendererModule,
		FIntPoint, InResizeTo, ResizeTo,
		{
			FPooledRenderTargetDesc OutputDesc(FPooledRenderTargetDesc::Create2DDesc(InResizeTo, PF_B8G8R8A8, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable, false));
	
			TRefCountPtr<IPooledRenderTarget> ResampleTexturePooledRenderTarget;
			InRenderModule->RenderTargetPoolFindFreeElement(RHICmdList, OutputDesc, ResampleTexturePooledRenderTarget, TEXT("ResampleTexture"));
			check(ResampleTexturePooledRenderTarget);

			const FSceneRenderTargetItem& DestRenderTarget = ResampleTexturePooledRenderTarget->GetRenderTargetItem();

			// Asynchronously copy render target from GPU to CPU
			const bool bKeepOriginalSurface = false;
			RHICmdList.CopyToResolveTarget(
				DestRenderTarget.TargetableTexture,
				InReadbackTextures[0],
				bKeepOriginalSurface,
				FResolveParams());
		});
	FlushRenderingCommands();
	UE_LOG(UT, Warning, TEXT("Copied a frame GPU to CPU"));

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ReadbackFromStagingBuffer,
		FTexture2DRHIRef*, InReadbackTextures, ReadbackTextures,
		{
			RHICmdList.UnmapStagingSurface(InReadbackTextures[0]);
			RHICmdList.UnmapStagingSurface(InReadbackTextures[1]);
		});
	FlushRenderingCommands();
	UE_LOG(UT, Warning, TEXT("Unmapped readback textures"));
#endif
}