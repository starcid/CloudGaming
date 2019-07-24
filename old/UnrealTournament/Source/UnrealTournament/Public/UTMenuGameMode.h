// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPlayerController.h"
#include "UTBaseGameMode.h"
#include "UTMenuGameMode.generated.h"

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTMenuGameMode : public AUTBaseGameMode
{
	GENERATED_UCLASS_BODY()

public:
	virtual void RestartGame();
	virtual void BeginGame();
	virtual void RestartPlayer(AController* aPlayer);
	virtual TSubclassOf<AGameMode> SetGameMode(const FString& MapName, const FString& Options, const FString& Portal); // FIXME: waiting on engine: override
	void Logout( AController* Exiting );
	
	virtual void PostInitializeComponents() override;
	virtual void OnLoadingMovieEnd() override;

	virtual void ShowMenu(AUTBasePlayerController* PC);

	UPROPERTY(config)
	FString MenuMusicAssetName;

	UPROPERTY()
	USoundBase* MenuMusic;

	UPROPERTY()
	UAudioComponent* MenuMusicAC;

protected:

	/** default music options for cooking */
	UPROPERTY()
	TArray<USoundBase*> DefaultMusics;
};





