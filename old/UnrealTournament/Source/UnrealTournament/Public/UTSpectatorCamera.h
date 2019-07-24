// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTSpectatorCamera.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTSpectatorCamera : public ACameraActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = "SpectatorCamera")
	FString CamLocationName;
	
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = "SpectatorCamera")
	bool bLoadingCamera;
};