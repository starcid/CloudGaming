// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SScaleBox.h"
#include "SUTDialogBase.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "Engine/UserInterfaceSettings.h"


#if !UE_SERVER

void SUTDialogBase::Construct(const FArguments& InArgs)
{
	PlayerOwner = InArgs._PlayerOwner;
	checkSlow(PlayerOwner != NULL);

	TabStop = 0;
	bSkipWorldRender = false;

	OnDialogResult = InArgs._OnDialogResult;

	// Calculate the size.  If the size is relative, then scale it against the designed by resolution as the 
	// DPI Scale planel will take care of the rest.
	FVector2D DesignedRez(1920,1080);
	ActualSize = InArgs._DialogSize;
	if (InArgs._bDialogSizeIsRelative)
	{
		ActualSize *= DesignedRez;
	}

	// Now we have to center it.  The tick here is we have to scale the current viewportSize UP by scale used in the DPI panel other
	// we can't position properly.
	
	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);

	ActualPosition = (DesignedRez * InArgs._DialogPosition) - (ActualSize * InArgs._DialogAnchorPoint);

	TSharedPtr<SWidget> FinalContent;

	if ( InArgs._IsScrollable )
	{
		SAssignNew(FinalContent, SScrollBox)
		.Style(SUWindowsStyle::Get(),"UT.ScrollBox.Borderless")
		+ SScrollBox::Slot()

		.Padding(FMargin(0.0f, 15.0f, 0.0f, 15.0f))
		[
			// Add an Overlay
			SAssignNew(DialogContent, SOverlay)
		];
	}
	else
	{
		FinalContent = SAssignNew(DialogContent, SOverlay);
	}

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[

		SNew(SOverlay)
		+SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SImage)
			.Image(InArgs._bShadow ? SUTStyle::Get().GetBrush("UT.HeaderBackground.Shaded") : new FSlateNoResource)
		]
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(Canvas, SCanvas)

			// We use a Canvas Slot to position and size the dialog.  
			+ SCanvas::Slot()
			.Position(ActualPosition)
			.Size(ActualSize)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Left)
			[
				// This is our primary overlay.  It controls all of the various elements of the dialog.  This is not
				// the content overlay.  This comes below.
				SNew(SOverlay)

				// this is the background image
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.DialogBox.Background"))
					]
				]

				// This will define a vertical box that holds the various components of the dialog box.
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)

					// The title bar
					+ SVerticalBox::Slot()
					.Padding(0.0f, 5.0f, 0.0f, 5.0f)
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Fill)
					[
						BuildTitleBar(InArgs._DialogTitle)
					]

					// The content section
					+ SVerticalBox::Slot()
					.FillHeight(1.0)
					.Padding(InArgs._ContentPadding.X, InArgs._ContentPadding.Y, InArgs._ContentPadding.X, InArgs._ContentPadding.Y)
					[
						FinalContent.ToSharedRef()
					]

					// The ButtonBar
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Bottom)
					.Padding(5.0f, 5.0f, 5.0f, 5.0f)
					[
						SNew(SBox)
						.HeightOverride(48)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0)
								[
									SNew(SOverlay)
									+ SOverlay::Slot()
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.AutoHeight()
										.HAlign(HAlign_Right)
										[
											SNew(SImage)
											.Image(SUWindowsStyle::Get().GetBrush("UT.Dialog.RightButtonBackground"))
										]
									]
									+ SOverlay::Slot()
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.AutoHeight()
										.HAlign(HAlign_Right)
										[
											BuildButtonBar(InArgs._ButtonMask)
										]

									]
								]

							]
							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Left)
								.Padding(10.0f, 0.0f, 0.0f, 0.0f)
								[
									BuildCustomButtonBar()
								]
							]
						]
					]
				]
			]
		]
	];
}



void SUTDialogBase::BuildButton(TSharedPtr<SUniformGridPanel> Bar, FText ButtonText, uint16 ButtonID, uint32 &ButtonCount)
{
	TSharedPtr<SButton> Button;
	if (Bar.IsValid())
	{
		Bar->AddSlot(ButtonCount,0)
			.HAlign(HAlign_Fill)
			[
				SNew(SBox)
				.HeightOverride(48)
				[
					SAssignNew(Button, SButton)
					.HAlign(HAlign_Center)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
					.Text(ButtonText)
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					.OnClicked(this, &SUTDialogBase::OnButtonClick, ButtonID)
				]
			];

		ButtonCount++;
		if (Button.IsValid())
		{
			TabTable.AddUnique(Button);
			ButtonMap.Add(ButtonID, Button);
		}
	};
}

TSharedRef<class SWidget> SUTDialogBase::BuildButtonBar(uint16 ButtonMask)
{
	uint32 ButtonCount = 0;

	ButtonMap.Empty();

	SAssignNew(ButtonBar,SUniformGridPanel)
		.SlotPadding(FMargin(0.0f,0.0f, 10.0f, 10.0f));

	if ( ButtonBar.IsValid() )
	{

		AddButtonsToLeftOfButtonBar(ButtonCount);

		if (ButtonMask & UTDIALOG_BUTTON_APPLY)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","ApplyButton","APPLY"),			UTDIALOG_BUTTON_APPLY,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_OK)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","OKButton","OK"),					UTDIALOG_BUTTON_OK,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_PLAY)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","PlayButton","PLAY"),				UTDIALOG_BUTTON_PLAY,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_LAN)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","LanButton","START LAN GAME"),		UTDIALOG_BUTTON_LAN,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_CANCEL)	BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","CancelButton","CANCEL"),			UTDIALOG_BUTTON_CANCEL,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_YES)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","YesButton", "YES"),				UTDIALOG_BUTTON_YES, ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_NO)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","NoButton","NO"),					UTDIALOG_BUTTON_NO,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_YESCLEAR)	BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","YesClearButton","CLEAR DUPES"),	UTDIALOG_BUTTON_YESCLEAR, ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_HELP)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","HelpButton","HELP"),				UTDIALOG_BUTTON_HELP,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_RECONNECT) BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","ReconnectButton","RECONNECT"),	UTDIALOG_BUTTON_RECONNECT,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_EXIT)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","ExitButton","EXIT"),				UTDIALOG_BUTTON_EXIT,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_QUIT)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","QuitButton","QUIT"),				UTDIALOG_BUTTON_QUIT,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_VIEW)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","ViewButton","VIEW"),				UTDIALOG_BUTTON_VIEW,ButtonCount);
		if (ButtonMask & UTDIALOG_BUTTON_CLOSE)		BuildButton(ButtonBar, NSLOCTEXT("SUTDialogBase","CloseButton","CLOSE"),			UTDIALOG_BUTTON_CLOSE,ButtonCount);
	}

	return ButtonBar.ToSharedRef();
}

TSharedRef<class SWidget> SUTDialogBase::BuildCustomButtonBar()
{
	TSharedPtr<SCanvas> C;
	SAssignNew(C,SCanvas);
	return C.ToSharedRef();
}

FReply SUTDialogBase::OnButtonClick(uint16 ButtonID)
{
	OnDialogResult.ExecuteIfBound(SharedThis(this), ButtonID);
	GetPlayerOwner()->CloseDialog(SharedThis(this));
	return FReply::Handled();
}


/******************** ALL OF THE HACKS NEEDED TO MAINTAIN WINDOW FOCUS *********************************/

void SUTDialogBase::OnDialogOpened()
{
	GameViewportWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);
}

void SUTDialogBase::OnDialogClosed()
{
	FSlateApplication::Get().ClearKeyboardFocus();
	FSlateApplication::Get().ClearUserFocus(0);
}

bool SUTDialogBase::SupportsKeyboardFocus() const
{
	return true;
}

FReply SUTDialogBase::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InKeyboardFocusEvent)
{
	return FReply::Handled()
		.ReleaseMouseCapture();
}

FReply SUTDialogBase::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnButtonClick(UTDIALOG_BUTTON_CANCEL);
	}

	return FReply::Unhandled();
}

FReply SUTDialogBase::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}


FReply SUTDialogBase::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SUTDialogBase::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

TSharedRef<SWidget> SUTDialogBase::GenerateStringListWidget(TSharedPtr<FString> InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*InItem.Get()))
			.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
		];
}

FReply SUTDialogBase::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if ((InKeyEvent.GetKey() == EKeys::Tab) && (TabTable.Num() > 0))
	{
		int32 NewStop = (InKeyEvent.IsLeftShiftDown() || InKeyEvent.IsRightShiftDown()) ? -1 : 1;
		if (TabStop + NewStop < 0)
		{
			TabStop = TabTable.Num() - 1;
		}
		else
		{
			TabStop = (TabStop + NewStop) % TabTable.Num();
		}

		FSlateApplication::Get().SetKeyboardFocus(TabTable[TabStop], EKeyboardFocusCause::Keyboard);	
	}
	return FReply::Handled();


}

void SUTDialogBase::EnableButton(uint16 ButtonID)
{
	const TSharedPtr<SButton>* Button;
	Button = ButtonMap.Find(ButtonID);
	if (Button != nullptr && Button->IsValid())
	{
		Button->Get()->SetEnabled(true);
	}
}

void SUTDialogBase::DisableButton(uint16 ButtonID)
{
	const TSharedPtr<SButton>* Button;
	Button = ButtonMap.Find(ButtonID);
	if (Button != nullptr && Button->IsValid())
	{
		Button->Get()->SetEnabled(false);
	}

}

TSharedRef<class SWidget> SUTDialogBase::BuildTitleBar(FText InDialogTitle)
{
	return 	SNew(SBox)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SAssignNew(DialogTitle, STextBlock)
			.Text(InDialogTitle)
			.TextStyle(SUWindowsStyle::Get(), "UT.Dialog.TitleTextStyle")
		];
}

TSharedPtr<SWidget> SUTDialogBase::GetBestWidgetToFocus()
{
	return PlayerOwner->ViewportClient->GetGameViewportWidget();
}

#endif