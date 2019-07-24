// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUTTabWidget.h"
#include "../SUWindowsStyle.h"

#if !UE_SERVER
void SUTTabWidget::Construct(const FArguments& InArgs)
{
	OnTabButtonSelectionChanged = InArgs._OnTabButtonSelectionChanged;
	OnTabButtonNumberChanged = InArgs._OnTabButtonNumberChanged;
	TabTextStyle = InArgs._TabTextStyle;

	TabsContainer = SNew(SWrapBox)
		.UseAllottedWidth(true);

	TabSwitcher = SNew(SWidgetSwitcher);

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(0.0f)
				.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
				.Content()
				[
					TabsContainer.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				TabSwitcher.ToSharedRef()
			]
		];
}

void SUTTabWidget::AddTab(FText ButtonLabel, TSharedPtr<SWidget> Widget)
{
	if (!ButtonLabel.IsEmpty() && Widget.IsValid())
	{
		TSharedPtr<SUTTabButton> Button = SNew(SUTTabButton)
			.ContentPadding(FMargin(15.0f, 10.0f, 70.0f, 0.0f))
			.ButtonStyle(SUTStyle::Get(), "UT.TabButton")
			.IsToggleButton(true)
			.TextStyle(TabTextStyle)
			.Text(ButtonLabel)
			.OnClicked(this, &SUTTabWidget::OnButtonClicked, ButtonLabel);
		ButtonLabels.Add(ButtonLabel);
		TabButtons.Add(Button);

		//Add the tab button and the widget to the switcher
		if (TabsContainer.IsValid())
		{
			TabsContainer->AddSlot()
			[
				Button.ToSharedRef()
			];
		}

		if (TabSwitcher.IsValid())
		{
			TabSwitcher->AddSlot()
			[
				Widget.ToSharedRef()
			];
		}
	}
}

void SUTTabWidget::SelectTab(int32 NewTab)
{
	if (ButtonLabels.IsValidIndex(NewTab))
	{
		OnButtonClicked(ButtonLabels[NewTab]);
	}
}

FReply SUTTabWidget::OnButtonClicked(FText InLabel)
{
	if (!CurrentTabButton.EqualTo(InLabel))
	{
		int32 NewIndex = 0;
		CurrentTabButton = InLabel;

		for (int32 i = 0; i < TabButtons.Num(); i++)
		{
			if (ButtonLabels.IsValidIndex(i))
			{
				if (ButtonLabels[i].EqualTo(InLabel))
				{
					TabButtons[i]->BePressed();
					TabSwitcher->SetActiveWidgetIndex(i);
					NewIndex = i;
				}
				else
				{
					TabButtons[i]->UnPressed();
				}
			}
		}

		OnTabButtonSelectionChanged.ExecuteIfBound(CurrentTabButton);
		OnTabButtonNumberChanged.ExecuteIfBound(NewIndex);
	}
	return FReply::Handled();
}


#endif