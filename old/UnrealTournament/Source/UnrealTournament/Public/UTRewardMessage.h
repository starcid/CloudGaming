// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTAnnouncer.h"
#include "UTRewardMessage.generated.h"

UCLASS(CustomConstructor, Abstract)
class UNREALTOURNAMENT_API UUTRewardMessage : public UUTLocalMessage
{
	GENERATED_UCLASS_BODY()

	UUTRewardMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{
		MessageArea = FName(TEXT("Announcements"));
		MessageSlot = FName(TEXT("MajorRewardMessage"));
		bIsUnique = true;
		bIsConsoleMessage = false;
		Lifetime = 1.2f;
		AnnouncementHS = FName(TEXT("RW_HolyShit"));
		bWantsBotReaction = true;
		ScaleInSize = 3.f;
		AnnouncementDelay = 0.3f;
		ScaleInTime = 0.15f;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText MessageText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FName Announcement;

	/** Announcement on every other message. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
		FName AnnouncementAlt;

	/** Announcement on fifth message. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
		FName AnnouncementFive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
		FName AnnouncementHS;

	virtual FLinearColor GetMessageColor_Implementation(int32 MessageIndex) const override
	{
		return REDHUDCOLOR;
	}

	virtual bool ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
	{
		return true;
	}

	virtual bool InterruptAnnouncement(const FAnnouncementInfo AnnouncementInfo, const FAnnouncementInfo OtherAnnouncementInfo) const
	{
		return Cast<UUTLocalMessage>(OtherAnnouncementInfo.MessageClass->GetDefaultObject())->IsOptionalSpoken(OtherAnnouncementInfo.Switch);
	}

	virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const override
	{
		if (Switch == 100)
		{
			return AnnouncementHS;
		}
		if ((Switch > 0) && (Switch % 5 == 0) && (AnnouncementFive != NAME_None))
		{
			return AnnouncementFive;
		}
		if ((Switch % 2 == 1) && (AnnouncementAlt != NAME_None))
		{
			return AnnouncementAlt;
		}
		return Announcement;
	}

	virtual FText GetText(int32 Switch, bool bTargetsPlayerState1, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const override
	{
		return MessageText;
	}

	virtual void PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const override
	{
		Announcer->PrecacheAnnouncement(Announcement);
		Announcer->PrecacheAnnouncement(AnnouncementAlt);
		Announcer->PrecacheAnnouncement(AnnouncementFive);
		Announcer->PrecacheAnnouncement(AnnouncementHS);
	}
};

