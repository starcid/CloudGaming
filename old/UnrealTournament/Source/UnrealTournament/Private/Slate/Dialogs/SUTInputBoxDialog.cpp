// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTInputBoxDialog.h"
#include "../SUWindowsStyle.h"

#if !UE_SERVER

void SUTInputBoxDialog::Construct(const FArguments& InArgs)
{
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(InArgs._DialogTitle)
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(InArgs._ButtonMask)
							.OnDialogResult(InArgs._OnDialogResult)
							.IsScrollable(InArgs._IsScrollable)
						);
	TextFilter = InArgs._TextFilter;
	IsPassword = InArgs._IsPassword;
	MaxInputLength = InArgs._MaxInputLength;
	ButtonMask = InArgs._ButtonMask;

	if (DialogContent.IsValid())
	{
		const float MessageTextPaddingX = 10.0f;
		TSharedPtr<STextBlock> MessageTextBlock;
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.HAlign(HAlign_Center)
			.Padding(FMargin(10.0f,5.0f,10.0f,5.0f))
			[ 
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().FillWidth(1.0)
				[
					SNew(STextBlock)
					.Text(InArgs._MessageText)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Fill)
			.AutoHeight()
			.Padding(FMargin(10.0f, 0.0f, 10.0f, 5.0f))
			[
				SAssignNew(EditBox, SEditableTextBox)
				.Style(SUWindowsStyle::Get(), "UT.Common.Editbox.White")
				.OnTextChanged(this, &SUTInputBoxDialog::OnTextChanged)
				.OnTextCommitted(this, &SUTInputBoxDialog::OnTextCommited)
				.IsPassword(IsPassword)
				.MinDesiredWidth(300.0f)
				.Padding(FMargin(5.0f, 12.0f, 5.0f,10.0f))
				.Text(FText::FromString(InArgs._DefaultInput))
			]
		];
	}

}

void SUTInputBoxDialog::OnDialogOpened()
{
	SUTDialogBase::OnDialogOpened();
	// start with the editbox focused
	PlayerOwner->FocusWidget(EditBox.ToSharedRef());
}


void SUTInputBoxDialog::OnTextChanged(const FText& NewText)
{
	FString Result = NewText.ToString().Left(MaxInputLength);
	if (TextFilter.IsBound())
	{
		for (int32 i = Result.Len() - 1; i >= 0; i--)
		{
			if (!TextFilter.Execute(Result.GetCharArray()[i]))
			{
				Result.GetCharArray().RemoveAt(i);
			}
		}
	}
	EditBox->SetText(FText::FromString(Result));
}

void SUTInputBoxDialog::OnTextCommited(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		if (ButtonMask & UTDIALOG_BUTTON_OK)
		{
			OnButtonClick(UTDIALOG_BUTTON_OK);
		}
		else if (ButtonMask & UTDIALOG_BUTTON_YES)
		{
			OnButtonClick(UTDIALOG_BUTTON_YES);
		}
	}
}

FString SUTInputBoxDialog::GetInputText()
{
	if (EditBox.IsValid())
	{
		return EditBox->GetText().ToString();
	}
	return TEXT("");
}

TSharedPtr<SWidget> SUTInputBoxDialog::GetBestWidgetToFocus()
{
	return EditBox;
}



#endif