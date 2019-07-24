// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTATypes.h"
#include "UTRconAdminInfo.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTRconAdminInfo : public AInfo
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Replicated)
	TArray<FRconPlayerData> PlayerData;

	UPROPERTY(Replicated)
	TArray<FBanInfo> BanData;

	virtual void BeginPlay();
	virtual void NoLongerNeeded();

protected:

	FTimerHandle UpdateTimerHandle;
	void UpdateList();

	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerNoLongerNeeded();
};



