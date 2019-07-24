// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTChatWidget.h"
#include "../SUWindowsStyle.h"

#if WITH_SOCIAL
#include "Social.h"
#endif

#define CHAT_BOX_WIDTH 576.0f
#define CHAT_BOX_HEIGHT 320.0f
#define CHAT_BOX_HEIGHT_FADED 120.0f
#define CHAT_BOX_PADDING 20.0f
#define MAX_CHAT_MESSAGES 10

#if !UE_SERVER

void SUTChatWidget::Construct(const FArguments& InArgs)
{

#if WITH_SOCIAL
	//some constant values
	const int32 PaddingValue = 2;

	TSharedPtr< class SWidget > Chat;
	TSharedRef< IFriendsAndChatManager > Manager = ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true);
	Display = Manager->GenerateChatDisplayService();
	Display->SetMinimizeEnabled(false);
	Settings = Manager->GetChatSettingsService();
	
	//InArgs._FriendStyle, 
	Chat = Manager->GenerateChromeWidget(Display.ToSharedRef(), Settings.ToSharedRef());

	// Initialize Menu
	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Bottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(CHAT_BOX_HEIGHT)
			.WidthOverride(CHAT_BOX_WIDTH)
 			[
				Chat.ToSharedRef()
 			]
		]
	];
#endif
}

void SUTChatWidget::HandleFriendsNetworkChatMessage(const FString& NetworkMessage)
{
	//ViewModel->SetOverrideColorActive(false);
	//Ctx.GetPlayerController()->Say(NetworkMessage);
}

void SUTChatWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	//Always tick the super
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SUTChatWidget::SetFocus()
{
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));

#if WITH_SOCIAL
	Display->SetFocus();
#endif
}

FReply SUTChatWidget::OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent )
{
	return FReply::Handled().ReleaseMouseCapture().LockMouseToWidget( SharedThis( this ) );
}

#endif