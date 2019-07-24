// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTFriendsPopupWindow.h"
#include "../Widgets/SUTChatWidget.h"
#include "../Widgets/SUTFriendsWidget.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"

#if !UE_SERVER

void SUTFriendsPopupWindow::Construct(const FArguments& InArgs)
{
	PlayerOwner = InArgs._PlayerOwner;
	checkSlow(PlayerOwner != NULL);
	ChildSlot
		.Padding(0.0f,0.0f,0.0f,0.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(SUTStyle::Get().GetBrush("UT.Background.Shadow"))
				.ColorAndOpacity(FLinearColor(1.0f,1.0f,1.0f,0.5f))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(CloseButton, SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(SUTChatWidget)
				]
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(80.0f, 46.0f))
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(SUTFriendsWidget)
/*					.FriendStyle(&SocialAsset->Style)*/
				]
			]

		];
}

void SUTFriendsPopupWindow::SetOnCloseClicked(FOnClicked InOnClicked)
{
	CloseButton->SetOnClicked(InOnClicked);
}

/******************** ALL OF THE HACKS NEEDED TO MAINTAIN WINDOW FOCUS *********************************/
/*
bool SUTFriendsPopupWindow::SupportsKeyboardFocus() const
{
	return true;
}

FReply SUTFriendsPopupWindow::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InKeyboardFocusEvent)
{
	return FReply::Handled()
		.ReleaseMouseCapture()
		.LockMouseToWidget(SharedThis(this));

}

FReply SUTFriendsPopupWindow::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return FReply::Unhandled();
}

FReply SUTFriendsPopupWindow::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}


FReply SUTFriendsPopupWindow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SUTFriendsPopupWindow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}


FReply SUTFriendsPopupWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return FReply::Handled();
}
*/

#endif