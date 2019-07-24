// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 

/**
 *
 **/

#include "UTHUDWidget_Spectator.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTHUDWidget_Spectator : public UUTHUDWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Draw_Implementation(float DeltaTime);
	virtual bool ShouldDraw_Implementation(bool bShowScores);

	/** Draw the bar and the spectator message.  If ShortMessage, don't use entire bottom of screen. */
	virtual void DrawSimpleMessage(FText SimpleMessage, float DeltaTime, FText ShortMessage);

	virtual FText GetSpectatorMessageText(FText& ShortMessage);

	virtual FLinearColor GetMessageColor() const
	{
		return FLinearColor::White;
	}
protected:
	virtual float GetDrawScaleOverride();

	/** Last viewed player. */
	UPROPERTY()
	class AUTPlayerState* LastViewedPS;

	/** Last time viewed player changed. */
	float ViewCharChangeTime;

private:
};
