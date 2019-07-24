// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTimerMessage.generated.h"

UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTTimerMessage : public UUTLocalMessage
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Message)
	TArray<FText> CountDownText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Message)
	FText LeadingText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Message)
	TArray<FName> CountDownAnnouncements;

	UUTTimerMessage(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		MessageArea = FName(TEXT("Announcements"));
		MessageSlot = FName(TEXT("CountdownMessages"));
		bIsUnique = true;
		Lifetime = 2.0f;
		bIsStatusAnnouncement = true;
		bOptionalSpoken = true; 
		bPlayDuringInstantReplay = false;

		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text1",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text2",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text3",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text4",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text5",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text6",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text7",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text8",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text9",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text10",""));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text30Secs","30 seconds left! {0}"));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text1Min","One minute remains! {0}"));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text3Min","Three minutes remain! {0}"));
		CountDownText.Add( NSLOCTEXT("UTTimerMessage","Text5Min","Five minutes remain! {0}"));

		LeadingText = NSLOCTEXT("UTTimerMessage", "LeadingText", "{0} leads!");

		CountDownAnnouncements.Add(TEXT("CD1"));
		CountDownAnnouncements.Add(TEXT("CD2"));
		CountDownAnnouncements.Add(TEXT("CD3"));
		CountDownAnnouncements.Add(TEXT("CD4"));
		CountDownAnnouncements.Add(TEXT("CD5"));
		CountDownAnnouncements.Add(TEXT("CD6"));
		CountDownAnnouncements.Add(TEXT("CD7"));
		CountDownAnnouncements.Add(TEXT("CD8"));
		CountDownAnnouncements.Add(TEXT("CD9"));
		CountDownAnnouncements.Add(TEXT("CD10"));
		CountDownAnnouncements.Add(TEXT("ThirtySecondsRemain"));
		CountDownAnnouncements.Add(TEXT("1MinRemains"));
		CountDownAnnouncements.Add(TEXT("3MinRemains"));
		CountDownAnnouncements.Add(TEXT("FiveMinuteWarning"));
	}

	virtual bool IsOptionalSpoken(int32 MessageIndex) const override
	{
		return bOptionalSpoken && (MessageIndex < 11);
	}

	virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const override
	{
		return (Switch >= 0 && Switch < CountDownAnnouncements.Num() ? CountDownAnnouncements[Switch] : NAME_None);
	}

	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const override
	{
		FText Result = (Switch >= 0 && Switch < CountDownText.Num() ? CountDownText[Switch] : FText::GetEmpty());
		FText CurrentLeader;
		APlayerState* PS = Cast<APlayerState>(OptionalObject);
		if (PS != NULL)
		{
			CurrentLeader = FText::FromString(PS->PlayerName);
		}
		else
		{
			AUTTeamInfo* Team = Cast<AUTTeamInfo>(OptionalObject);
			if (Team != NULL)
			{
				CurrentLeader = Team->TeamName;
			}
		}
		Result = FText::Format(Result, CurrentLeader.IsEmpty() ? CurrentLeader : FText::Format(LeadingText, CurrentLeader));
		return Result;
	}
};