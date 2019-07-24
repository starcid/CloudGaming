// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObjectMessage.h"
#include "UTCTFMajorMessage.h"
#include "UTAnnouncer.h"
#include "UTRallyPoint.h"

UUTCTFMajorMessage::UUTCTFMajorMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Lifetime = 3.0f;
	MessageArea = FName(TEXT("Announcements"));
	MessageSlot = FName(TEXT("MajorRewardMessage"));
	HalftimeMessage = NSLOCTEXT("CTFGameMessage", "Halftime", "");
	OvertimeMessage = NSLOCTEXT("CTFGameMessage", "Overtime", "OVERTIME!");
	FlagReadyMessage = NSLOCTEXT("CTFGameMessage", "FlagReadyMessage", "Attacker flag activated!");
	FlagRallyMessage = NSLOCTEXT("CTFGameMessage", "FlagRallyMessage", "RALLY NOW!");
	RallyReadyMessage = NSLOCTEXT("CTFGameMessage", "RallyReadyMessage", "Rally Started!");
	EnemyRallyMessage = NSLOCTEXT("CTFGameMessage", "EnemyRallyMessage", "Enemy Rally Activated!");
	EnemyRallyPrefix = NSLOCTEXT("CTFGameMessage", "EnemyRallyPrefix", "Enemy Rally at ");
	EnemyRallyPostfix = NSLOCTEXT("CTFGameMessage", "EnemyRallyPostfix", "");
	TeamRallyMessage = NSLOCTEXT("CTFGameMessage", "TeamRallyMessage", "");
	RallyCompleteMessage = NSLOCTEXT("CTFGameMessage", "RallyCompleteMessage", "Rally Ended!");
	RallyClearMessage = NSLOCTEXT("CTFGameMessage", "RallyClearMessage", "   ");
	PressToRallyPrefix = NSLOCTEXT("CTFGameMessage", "PressToRallyPrefix", "Press ");
	PressToRallyPostfix = NSLOCTEXT("CTFGameMessage", "PressToRallyPostfix", " to Rally Now!");
	bIsStatusAnnouncement = true;
	bIsPartiallyUnique = true;
	ScaleInSize = 3.f;
	RallyCompleteName = TEXT("RallyComplete");
	FontSizeIndex = 1;

	static ConstructorHelpers::FObjectFinder<USoundBase> FlagWarningSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/FlagUp_stereo.FlagUp_stereo'"));
	FlagWarningSound = FlagWarningSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> FlagRallySoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyCall.RallyCall'"));
	FlagRallySound = FlagRallySoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> RallyReadySoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyAvailable.RallyAvailable'"));
	RallyReadySound = RallyReadySoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> EnemyRallySoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/EnemyRally.EnemyRally'"));
	EnemyRallySound = EnemyRallySoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> RallyFinalSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyFinal.RallyFinal'"));
	RallyFinalSound = RallyFinalSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> RallyCompleteSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyComplete.RallyComplete'"));
	RallyCompleteSound = RallyCompleteSoundFinder.Object;
}

void UUTCTFMajorMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	Super::ClientReceive(ClientData);
	AUTPlayerController* PC = Cast<AUTPlayerController>(ClientData.LocalPC);
	if (PC)
	{
		if (ClientData.MessageIndex == 21)
		{
			PC->UTClientPlaySound(FlagWarningSound);
		}
		else if (ClientData.MessageIndex == 22)
		{
			PC->UTClientPlaySound(FlagRallySound);
			PC->bNeedsRallyNotify = true;
		}
		else if ((ClientData.MessageIndex == 23) || (ClientData.MessageIndex == 28) || (ClientData.MessageIndex == 30))
		{
			PC->UTClientPlaySound(RallyReadySound);
			PC->bNeedsRallyNotify = (ClientData.MessageIndex != 28);
		}
		else if (ClientData.MessageIndex == 24)
		{
			PC->UTClientPlaySound(EnemyRallySound);
		}
		else if (ClientData.MessageIndex == 25)
		{
			PC->UTClientPlaySound(RallyFinalSound);
		}
		else if (ClientData.MessageIndex == 27)
		{
			PC->UTClientPlaySound(RallyCompleteSound);
		}
	}
}

bool UUTCTFMajorMessage::ShouldDrawMessage(int32 MessageIndex, AUTPlayerController* PC, bool bIsAtIntermission, bool bNoLivePawnTarget) const
{
	if ((MessageIndex == 23) || (MessageIndex == 30))
	{
		// only draw if can still rally
		if (!PC->CanPerformRally())
		{
			return false;
		}
	}
		
	return Super::ShouldDrawMessage(MessageIndex, PC, bIsAtIntermission, bNoLivePawnTarget);
}

FLinearColor UUTCTFMajorMessage::GetMessageColor_Implementation(int32 MessageIndex) const
{
	return FLinearColor::White;
}

void UUTCTFMajorMessage::GetEmphasisText(FText& PrefixText, FText& EmphasisText, FText& PostfixText, FLinearColor& EmphasisColor, int32 Switch, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const
{
	if (Switch == 24)
	{
		AUTGameVolume* GV = Cast<AUTGameVolume>(OptionalObject);
		if (GV)
		{
			PrefixText = EnemyRallyPrefix;
			EmphasisText = GV->VolumeName;
			PostfixText = EnemyRallyPostfix;
			AUTPlayerState* PS = Cast<AUTPlayerState>(RelatedPlayerState_1);
			EmphasisColor = (PS && PS->Team) ? PS->Team->TeamColor : REDHUDCOLOR;
		}
		else if (Cast<AUTRallyPoint>(OptionalObject))
		{
			PrefixText = EnemyRallyPrefix;
			EmphasisText = Cast<AUTRallyPoint>(OptionalObject)->LocationText;
			PostfixText = EnemyRallyPostfix;
			AUTPlayerState* PS = Cast<AUTPlayerState>(RelatedPlayerState_1);
			EmphasisColor = (PS && PS->Team) ? PS->Team->TeamColor : REDHUDCOLOR;
		}
		else
		{
			AUTCarriedObject* Flag = Cast<AUTCarriedObject>(OptionalObject);
			if (Flag)
			{
				PrefixText = EnemyRallyPrefix;
				EmphasisText = Flag->GetHUDStatusMessage(nullptr);
				PostfixText = EnemyRallyPostfix;
				AUTPlayerState* PS = Cast<AUTPlayerState>(RelatedPlayerState_1);
				EmphasisColor = (PS && PS->Team) ? PS->Team->TeamColor : REDHUDCOLOR;
			}
			else
			{
				PrefixText = EnemyRallyMessage;
				EmphasisText = FText::GetEmpty();
				PostfixText = FText::GetEmpty();
			}
		}
		return;
	}
	else if (Switch == 30)
	{
		PrefixText = PressToRallyPrefix;
		PostfixText = PressToRallyPostfix;
		EmphasisColor = FLinearColor::Yellow;
		EmphasisText = FText::GetEmpty(); 
		if (RelatedPlayerState_1 && RelatedPlayerState_1->GetWorld())
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(GEngine->GetFirstLocalPlayerController(RelatedPlayerState_1->GetWorld()));
			if (PC && PC->MyUTHUD)
			{
				EmphasisText = PC->MyUTHUD->RallyLabel;
			}
		}
		return;
	}

	Super::GetEmphasisText(PrefixText, EmphasisText, PostfixText, EmphasisColor, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

FText UUTCTFMajorMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	switch (Switch)
	{
	case 11: return HalftimeMessage; break;
	case 12: return OvertimeMessage; break;
	case 21: return FlagReadyMessage; break;
	case 22: return FlagRallyMessage; break;
	case 23: return RallyReadyMessage; break;
	case 24: return BuildEmphasisText(Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject); break;
	case 25: return RallyCompleteMessage; break;
	case 26: return RallyClearMessage; break;
	case 27: return TeamRallyMessage; break;
	case 28: return EnemyRallyMessage; break;
	case 30: return BuildEmphasisText(Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject); break;
	}
	return FText::GetEmpty();
}

float UUTCTFMajorMessage::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
	return ((AnnouncementInfo.Switch <13) || (AnnouncementInfo.Switch == 30)) ? 1.f : 0.5f;
}

bool UUTCTFMajorMessage::InterruptAnnouncement(const FAnnouncementInfo AnnouncementInfo, const FAnnouncementInfo OtherAnnouncementInfo) const
{
	if (OtherAnnouncementInfo.MessageClass->GetDefaultObject<UUTLocalMessage>()->IsOptionalSpoken(OtherAnnouncementInfo.Switch))
	{
		return true;
	}
	if (AnnouncementInfo.MessageClass == OtherAnnouncementInfo.MessageClass)
	{
		if (OtherAnnouncementInfo.Switch == 26)
		{
			return true;
		}
		return ((AnnouncementInfo.Switch == 25) || (AnnouncementInfo.Switch == 26)) && (OtherAnnouncementInfo.Switch == 30);
	}
	if (GetAnnouncementPriority(AnnouncementInfo) > OtherAnnouncementInfo.MessageClass->GetDefaultObject<UUTLocalMessage>()->GetAnnouncementPriority(OtherAnnouncementInfo))
	{
		return true;
	}
	return false;
}

float UUTCTFMajorMessage::GetAnnouncementSpacing_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	return 0.05f;
}

bool UUTCTFMajorMessage::CancelByAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const
{
	return false;
}

void UUTCTFMajorMessage::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
	Announcer->PrecacheAnnouncement(GetTeamAnnouncement(11, 0));
	Announcer->PrecacheAnnouncement(GetTeamAnnouncement(12, 0));
	Announcer->PrecacheAnnouncement(GetTeamAnnouncement(21, 0));
}

FName UUTCTFMajorMessage::GetTeamAnnouncement(int32 Switch, uint8 TeamNum) const
{
	switch (Switch)
	{
	case 11: return TEXT("HalfTime"); break;
	case 12: return TEXT("OverTime"); break;
	case 21: return TEXT("FlagCanBePickedUp"); break;
	//case 25: return RallyCompleteName; break;
	}
	return NAME_None;
}

FName UUTCTFMajorMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	const AUTTeamInfo* TeamInfo = Cast<AUTTeamInfo>(OptionalObject);
	uint8 TeamNum = (TeamInfo != NULL) ? TeamInfo->GetTeamNum() : 0;
	return GetTeamAnnouncement(Switch, TeamNum);
}

