// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTMutator.h"

AUTMutator::AUTMutator(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.DoNotCreateDefaultSubobject("Sprite"))
{
}

void AUTMutator::Init_Implementation(const FString& Options)
{
}

void AUTMutator::Mutate_Implementation(const FString& MutateString, APlayerController* Sender)
{
	if (NextMutator != NULL)
	{
		NextMutator->Mutate(MutateString, Sender);
	}
}

void AUTMutator::ModifyLogin_Implementation(FString& Portal, FString& Options)
{
	if (NextMutator != NULL)
	{
		NextMutator->ModifyLogin(Portal, Options);
	}
}

void AUTMutator::PostPlayerInit_Implementation(AController* C)
{
	if (NextMutator != NULL)
	{
		NextMutator->PostPlayerInit(C);
	}
}

void AUTMutator::NotifyLogout_Implementation(AController* C)
{
	if (NextMutator != NULL)
	{
		NextMutator->NotifyLogout(C);
	}
}

void AUTMutator::TriggerBoost_Implementation(AUTPlayerState* Who)
{
	if (NextMutator != NULL)
	{
		NextMutator->TriggerBoost(Who);
	}
}

bool AUTMutator::AlwaysKeep_Implementation(AActor* Other, bool& bPreventModify)
{
	return (NextMutator != NULL && NextMutator->AlwaysKeep(Other, bPreventModify));
}

bool AUTMutator::CheckRelevance_Implementation(AActor* Other)
{
	return (NextMutator == NULL || NextMutator->CheckRelevance(Other));
}

void AUTMutator::ModifyPlayer_Implementation(APawn* Other, bool bIsNewSpawn)
{
	if (NextMutator != NULL)
	{
		NextMutator->ModifyPlayer(Other, bIsNewSpawn);
	}
}

void AUTMutator::ModifyInventory_Implementation(AUTInventory* NewInv, AUTCharacter* NewOwner)
{
	if (NextMutator != NULL)
	{
		NextMutator->ModifyInventory(NewInv, NewOwner);
	}
}

bool AUTMutator::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
	if (NextMutator != NULL)
	{
		NextMutator->ModifyDamage(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
	}
	return true;
}

bool AUTMutator::PreventDeath_Implementation(APawn* KilledPawn, AController* Killer, TSubclassOf<UDamageType> DamageType, const FHitResult& HitInfo)
{
	return (NextMutator != NULL && NextMutator->PreventDeath(KilledPawn, Killer, DamageType, HitInfo));
}

void AUTMutator::ScoreKill_Implementation(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	if (NextMutator != NULL)
	{
		NextMutator->ScoreKill(Killer, Other, DamageType);
	}
}

void AUTMutator::ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason)
{
	if (NextMutator != NULL)
	{
		NextMutator->ScoreObject(GameObject, HolderPawn, Holder, Reason);
	}
}

void AUTMutator::ScoreDamage_Implementation(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker)
{
	if (NextMutator != NULL)
	{
		NextMutator->ScoreDamage(DamageAmount, Victim, Attacker);
	}
}

void AUTMutator::NotifyMatchStateChange_Implementation(FName NewState)
{
	if (NextMutator != NULL)
	{
		NextMutator->NotifyMatchStateChange(NewState);
	}
}

bool AUTMutator::OverridePickupQuery_Implementation(APawn* Other, TSubclassOf<AUTInventory> ItemClass, AActor* Pickup, bool& bAllowPickup)
{
	return (NextMutator != NULL && NextMutator->OverridePickupQuery(Other, ItemClass, Pickup, bAllowPickup));
}

//TODO: Maybe we should just create a static object for parsing game options.  Since this is currently at the engine level I'll
// just forward for now, but something to think about.
FString AUTMutator::ParseOption( const FString& Options, const FString& InKey )
{
	return UGameplayStatics::ParseOption(Options, InKey);
}

//TODO: Maybe we should just create a static object for parsing game options.  Since this is currently at the engine level I'll
// just forward for now, but something to think about.
bool AUTMutator::HasOption( const FString& Options, const FString& InKey )
{
	return UGameplayStatics::HasOption(Options, InKey);
}

//TODO: Maybe we should just create a static object for parsing game options.  Since this is currently at the engine level I'll
// just forward for now, but something to think about.
int32 AUTMutator::GetIntOption( const FString& Options, const FString& ParseString, int32 CurrentValue)
{
	return UGameplayStatics::GetIntOption(Options, ParseString, CurrentValue);
}

void AUTMutator::AddDefaultInventory(TSubclassOf<AUTInventory> InventoryClass)
{
	AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (GameMode != NULL)
	{
		GameMode->DefaultInventory.AddUnique(InventoryClass);
	}
}

// By default we do nothing.
void AUTMutator::GetGameURLOptions_Implementation(TArray<FString>& OptionsList) const
{
}

bool AUTMutator::AllowTextMessage_Implementation(FString& Msg, bool bIsTeamMessage, AUTBasePlayerController* Sender)
{
	if (NextMutator) return NextMutator->AllowTextMessage(Msg, bIsTeamMessage, Sender);
	return true;
}