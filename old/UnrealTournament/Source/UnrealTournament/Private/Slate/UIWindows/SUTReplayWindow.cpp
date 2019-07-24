// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "UTGameViewportClient.h"
#include "UTHUDWidget_SpectatorSlideOut.h"
#include "SUTReplayWindow.h"
#include "../SUWindowsStyle.h"
#include "../Widgets/SUTButton.h"
#include "SNumericEntryBox.h"
#include "UTPlayerState.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/DemoNetDriver.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "UTVideoRecordingFeature.h"
#include "../Dialogs/SUTScreenshotConfigDialog.h"
#include "SceneViewport.h"
#include "../Dialogs/SUTInputBoxDialog.h"

#if !UE_SERVER

void SUTReplayWindow::Construct(const FArguments& InArgs)
{
	SpeedMin = 0.1f;
	SpeedMax = 2.0f;

	RecordTimeStart = -1;
	RecordTimeStop = -1;

	bHideScrubBar = false;

	PlayerOwner = InArgs._PlayerOwner;
	DemoNetDriver = InArgs._DemoNetDriver;

	checkSlow(PlayerOwner != nullptr);
	checkSlow(DemoNetDriver != nullptr);

	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);

	float DPIScale = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));
	ViewportSize = ViewportSize / DPIScale;

	FVector2D TimeSize = FVector2D(0.5f, 0.09f) * ViewportSize;
	FVector2D TimePos(ViewportSize.X * 0.5f - TimeSize.X * 0.5f, ViewportSize.Y * 0.8);

	HideTimeBarTime = 5.0f;
	bDrawTooltip = false;

	bHandledMouseClick = false;

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SAssignNew(MainOverlay, SOverlay)
		.Visibility(this, &SUTReplayWindow::GetVis)
		+ SOverlay::Slot()
		[
			//TimeSlider tooltip
			SNew(SCanvas)
			+ SCanvas::Slot()
			.Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &SUTReplayWindow::GetTimeTooltipPosition)))
			.Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &SUTReplayWindow::GetTimeTooltipSize)))
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Replay.Tooltip.BG"))
					.Content()
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Text(this, &SUTReplayWindow::GetTooltipText)
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Tooltip.Arrow"))
				]
			]
		]
		//Time Controls
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(TimeBar, SBorder)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.ColorAndOpacity(this, &SUTReplayWindow::GetTimeBarColor)
				.BorderBackgroundColor(this, &SUTReplayWindow::GetTimeBarBorderColor)
				.BorderImage(SUWindowsStyle::Get().GetBrush("UT.TopMenu.Shadow"))
				.Content()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					.Padding(10.0f, 0.0f, 10.0f, 10.0f)
					[
						//The Main time slider
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SAssignNew(TimeSlider, SUTProgressSlider)
							.IndentHandle(false)
							.Value(this, &SUTReplayWindow::GetTimeSlider)
							.TotalValue(this, &SUTReplayWindow::GetTimeSliderLength)
							.OnValueChanged(this, &SUTReplayWindow::OnSetTimeSlider)
							.SliderBarColor(FColor(33, 93, 220))
							.SliderBarBGColor(FLinearColor(0.05f, 0.05f, 0.05f))
						]
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(50.0f, 0.0f)
						[
							//The current / total time
							SNew(SUniformGridPanel)
							+ SUniformGridPanel::Slot(0, 0)
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.HAlign(HAlign_Fill)
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(this, &SUTReplayWindow::GetTimeText)
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
								]
								+ SVerticalBox::Slot()
								.HAlign(HAlign_Fill)
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
										MakeMarkRecordStartButton()
									]
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
										MakeMarkRecordStopButton()
									]
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
										MakeRecordButton()
									]
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.HAlign(HAlign_Fill)
										.AutoHeight()
										[
											SNew(SBox)
											.HeightOverride(32)
											.WidthOverride(32)
											[
												SAssignNew(MarkEndButton, SButton)
												.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
												.OnClicked(this, &SUTReplayWindow::OnScreenshotButtonClicked)
												.Content()
												[
													SNew(SImage)
													.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.Screenshot"))
												]
											]
										]
									]
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.HAlign(HAlign_Fill)
										.AutoHeight()
										[
											SNew(SBox)
											.HeightOverride(32)
											.WidthOverride(32)
											[
												SAssignNew(MarkEndButton, SButton)
												.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
												.OnClicked(this, &SUTReplayWindow::OnScreenshotConfigButtonClicked)
												.Content()
												[
													SNew(SImage)
													.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.ScreenshotConfig"))
												]
											]
										]
									]
									/*
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.HAlign(HAlign_Fill)
									.AutoHeight()
									[
									SNew(SBox)
									.HeightOverride(32)
									.WidthOverride(32)
									[
									SAssignNew(MarkEndButton, SButton)
									.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
									.OnClicked(this, &SUTReplayWindow::OnCommentButtonClicked)
									.Content()
									[
									SNew(SImage)
									.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.Comment"))
									]
									]
									]
									]*/
									+SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									.AutoWidth()
									.Padding(10.0f, 0.0f, 10.0f, 0.0f)
									[
										SAssignNew(BookmarksComboBox, SComboBox< TSharedPtr<FString> >)
										.InitiallySelectedItem(0)
										.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
										.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
										.OptionsSource(&BookmarkNameList)
										.OnGenerateWidget(this, &SUTReplayWindow::GenerateStringListWidget)
										.OnSelectionChanged(this, &SUTReplayWindow::OnBookmarkSetSelected)
										.Content()
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.Padding(10.0f, 0.0f, 10.0f, 0.0f)
											[
												SAssignNew(SelectedBookmark, STextBlock)
												.Text(FText::FromString(TEXT("Bookmarks")))
												.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
											]
										]
									]
								]
							]
							//Play / Pause button
							+ SUniformGridPanel::Slot(1, 0)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
								.OnClicked(this, &SUTReplayWindow::OnPlayPauseButtonClicked)
								.ContentPadding(1.0f)
								.Content()
								[
									SNew(SImage)
									.Image(this, &SUTReplayWindow::GetPlayButtonBrush)
								]
							]

							//Speed Control
							+ SUniformGridPanel::Slot(2, 0)
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.WidthOverride(170)
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Center)
								.Content()
								[
									SNew(SNumericEntryBox<float>)
									.AllowSpin(true)
									.MinValue(SpeedMin)
									.MaxValue(SpeedMax)
									.MinSliderValue(SpeedMin)
									.MaxSliderValue(SpeedMax)
									.Value(this, &SUTReplayWindow::GetSpeedSlider)
									.OnValueChanged(this, &SUTReplayWindow::OnSetSpeedSlider)
									.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
								]
							]
						]
					]
				]
			]
		]
	];

	bool bVideoRecorderPresent = false;
	static const FName VideoRecordingFeatureName("VideoRecording");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
	{
		UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
		if (VideoRecorder)
		{
			bVideoRecorderPresent = true;
		}
	}

	if (!bVideoRecorderPresent)
	{
		if (RecordButton.IsValid())
		{
			RecordButton->SetVisibility(EVisibility::Hidden);
		}

		if (MarkStartButton.IsValid())
		{
			MarkStartButton->SetVisibility(EVisibility::Hidden);
		}

		if (MarkEndButton.IsValid())
		{
			MarkEndButton->SetVisibility(EVisibility::Hidden);
		}
	}

	EnumerateEvents();
}

void SUTReplayWindow::EnumerateEvents()
{
	if (DemoNetDriver.IsValid())
	{
		if (DemoNetDriver->ReplayStreamer.IsValid() && DemoNetDriver->ReplayStreamer->IsLive())
		{
			FTimerHandle ThrowawayHandle;
			PlayerOwner->GetWorld()->GetTimerManager().SetTimer(ThrowawayHandle, FTimerDelegate::CreateSP(this, &SUTReplayWindow::EnumerateEvents), 30.0f, false);
		}

		FEnumerateEventsCompleteDelegate EnumKills = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::KillsEnumerated);
		FEnumerateEventsCompleteDelegate EnumFlagCaps = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::FlagCapsEnumerated);
		FEnumerateEventsCompleteDelegate EnumFlagReturns = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::FlagReturnsEnumerated);
		FEnumerateEventsCompleteDelegate EnumFlagDeny = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::FlagDenyEnumerated);
		FEnumerateEventsCompleteDelegate EnumMultiKills = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::MultiKillsEnumerated);
		FEnumerateEventsCompleteDelegate EnumSpreeKills = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::SpreeKillsEnumerated);
		FEnumerateEventsCompleteDelegate EnumComments = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::CommentsEnumerated);
		DemoNetDriver->EnumerateEvents(TEXT("Kills"), EnumKills);
		DemoNetDriver->EnumerateEvents(TEXT("FlagCaps"), EnumFlagCaps);
		DemoNetDriver->EnumerateEvents(TEXT("FlagReturns"), EnumFlagReturns);
		DemoNetDriver->EnumerateEvents(TEXT("FlagDeny"), EnumFlagDeny);
		DemoNetDriver->EnumerateEvents(TEXT("MultiKills"), EnumMultiKills);
		DemoNetDriver->EnumerateEvents(TEXT("SpreeKills"), EnumSpreeKills);
		DemoNetDriver->EnumerateEvents(TEXT("Comments"), EnumComments);
	}
}

void SUTReplayWindow::KillsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		KillEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::FlagCapsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		FlagCapEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::FlagReturnsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		FlagReturnEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::FlagDenyEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		FlagDenyEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::MultiKillsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		MultiKillEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::SpreeKillsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		SpreeKillEvents = ReplayEventList.ReplayEvents;
	}
}

void SUTReplayWindow::CommentsEnumerated(const FReplayEventList& ReplayEventList, bool bSucceeded)
{
	if (bSucceeded)
	{
		CommentEvents = ReplayEventList.ReplayEvents;
	}

	RefreshBookmarksComboBox();
}

FText SUTReplayWindow::GetTimeText() const
{
	if (DemoNetDriver.IsValid())
	{
		FFormatNamedArguments Args;
		Args.Add("TotalTime", FText::AsTimespan(FTimespan(0, 0, static_cast<int32>(DemoNetDriver->DemoTotalTime))));
		Args.Add("CurrentTime", FText::AsTimespan(FTimespan(0, 0, static_cast<int32>(DemoNetDriver->DemoCurrentTime))));
		return FText::Format(FText::FromString(TEXT("{CurrentTime} / {TotalTime}")), Args);
	}
	return FText();
}

void SUTReplayWindow::OnSetTimeSlider(float NewValue)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (UTPC != nullptr)
	{
		// Unpause if we're paused
		if (DemoNetDriver.IsValid())
		{
			AWorldSettings* const WorldSettings = UTPC->GetWorldSettings();

			if (WorldSettings->Pauser != nullptr)
			{
				WorldSettings->Pauser = nullptr;
			}
		}

		UTPC->DemoGoTo(DemoNetDriver->DemoTotalTime * NewValue);
	}
}

float SUTReplayWindow::GetTimeSlider() const
{
	float SliderPos = 0.f;
	if (DemoNetDriver.IsValid() && DemoNetDriver->DemoTotalTime > 0.0f)
	{
		SliderPos = DemoNetDriver->DemoCurrentTime / DemoNetDriver->DemoTotalTime;
	}
	return SliderPos;
}

float SUTReplayWindow::GetTimeSliderLength() const
{
	if (DemoNetDriver.IsValid() && DemoNetDriver->DemoTotalTime > 0.0f)
	{
		return DemoNetDriver->DemoTotalTime;
	}

	return 1.0f;
}

void SUTReplayWindow::OnSetSpeedSlider(float NewValue)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (UTPC != nullptr)
	{
		UTPC->DemoSetTimeDilation(NewValue);
	}
}

TOptional<float> SUTReplayWindow::GetSpeedSlider() const
{
	return PlayerOwner->GetWorld()->GetWorldSettings()->DemoPlayTimeDilation;
}

const FSlateBrush* SUTReplayWindow::GetPlayButtonBrush() const
{
	return PlayerOwner->GetWorld()->GetWorldSettings()->Pauser ? SUWindowsStyle::Get().GetBrush("UT.Replay.Button.Play") : SUWindowsStyle::Get().GetBrush("UT.Replay.Button.Pause");
}

FReply SUTReplayWindow::OnPlayPauseButtonClicked()
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (UTPC != nullptr)
	{
		UTPC->UTDemoPause();
	}
	return FReply::Handled();
}

FLinearColor SUTReplayWindow::GetTimeBarColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, FMath::Max(HideTimeBarTime / 2.0f, 0.0f));
}

FSlateColor SUTReplayWindow::GetTimeBarBorderColor() const
{
	return FSlateColor(GetTimeBarColor());
}

void SUTReplayWindow::Tick(const FGeometry & AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( bHideScrubBar )
	{
		if (HideTimeBarTime > 0.0)
		{
			HideTimeBarTime -= InDeltaTime;
		}
	}
	else
	{
		HideTimeBarTime = 2.0f;
	}


	// If focus has changed and we're looking at kill bookmarks, refilter the bookmarks
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (UTPC &&	LastSpectedPlayerID != UTPC->LastSpectatedPlayerId)
	{
		LastSpectedPlayerID = UTPC->LastSpectatedPlayerId;

		if (SelectedBookmark->GetText().ToString() == TEXT("Kills"))
		{
			CurrentBookmarks.Empty();
			OnKillBookmarksSelected();
		}
		else if (SelectedBookmark->GetText().ToString() == TEXT("Multi Kills"))
		{
			CurrentBookmarks.Empty();
			OnMultiKillBookmarksSelected();
		}
		else if (SelectedBookmark->GetText().ToString() == TEXT("Spree Kills"))
		{
			CurrentBookmarks.Empty();
			OnSpreeKillBookmarksSelected();
		}
		else if (SelectedBookmark->GetText().ToString() == TEXT("Flag Returns"))
		{
			CurrentBookmarks.Empty();
			OnFlagReturnsBookmarksSelected();
		}
	}
	
	ToolTipTargetSizeX = 0;
	ToolTipCurrentSizeY = 0;
	if (bDrawTooltip)
	{
		if (TooltipBookmarkText.IsEmpty())
		{
			ToolTipTargetSizeX = 110.0f;
			ToolTipCurrentSizeY = 55.0f;
		}
		else
		{
			ToolTipTargetSizeX = 10.0f + 15.0f * TooltipBookmarkText.Len();
			ToolTipCurrentSizeY = 55.0f;
		}
	}

	ToolTipCurrentSizeX = FMath::FInterpTo(ToolTipCurrentSizeX, ToolTipTargetSizeX, InDeltaTime, 10.0f);

	return SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FReply SUTReplayWindow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GetVis() != EVisibility::Hidden)
	{
		FReply Reply = FReply::Unhandled();
	
		TSet< TSharedRef<SWidget> > TimeBarSet;
		TimeBarSet.Add(TimeBar.ToSharedRef());
		TMap<TSharedRef<SWidget>, FArrangedWidget> TimeBarGeometryResult;

		// FindChildGeometry can't handle the case where a child might not actually be a descendant, must use this overcomplicated version
		bool bFoundTimeBarGeometry = FindChildGeometries(MyGeometry, TimeBarSet, TimeBarGeometryResult);

		if (bFoundTimeBarGeometry && TimeBarGeometryResult[TimeBarSet.Array()[0]].Geometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
		{
			Reply = FReply::Handled();
		}
	
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);
		bHandledMouseClick = Reply.IsEventHandled();
	}
	return FReply::Handled();
}

FReply SUTReplayWindow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GetVis() != EVisibility::Hidden)
	{
		FVector2D MousePosition = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).IntPoint();
		if (MouseClickHUD(MousePosition))
		{
		}
		else
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
			if (!PC->bSpectatorMouseChangesView)
			{
				PC->SetSpectatorMouseChangesView(true);
			}
		}

		//If mouse down was handled, set to the current position so it doesn't jump around
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
		if (bHandledMouseClick && UTPC != nullptr)
		{
			UTPC->SavedMouseCursorLocation = FSlateApplication::Get().GetCursorPos();
		}
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);
	}
	return FReply::Handled();
}

bool SUTReplayWindow::MouseClickHUD(const FVector2D& MousePosition)
{
	if (GetVis() != EVisibility::Hidden)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
		if (PC && PC->MyUTHUD)
		{
			UUTHUDWidget_SpectatorSlideOut* SpectatorWidget = PC->MyUTHUD->GetSpectatorSlideOut();
			if (SpectatorWidget)
			{
				SpectatorWidget->SetMouseInteractive(true);
				return SpectatorWidget->MouseClick(MousePosition);
			}
		}
	}
	return false;
}

FReply SUTReplayWindow::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{

	FVector2D MousePosition = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).IntPoint();
	FVector2D MenuSize = MyGeometry.GetDrawSize();

	bHideScrubBar = (MousePosition.Y < MenuSize.Y * 0.8f);

	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		UUTHUDWidget_SpectatorSlideOut* SpectatorWidget = PC->MyUTHUD->GetSpectatorSlideOut();
		if (SpectatorWidget)
		{
			SpectatorWidget->SetMouseInteractive(true);
			SpectatorWidget->TrackMouseMovement(MousePosition);
		}
	}

	//Update the tooltip if the mouse is over the slider
	if (GetVis() != EVisibility::Collapsed && GetVis() != EVisibility::Hidden)
	{
		const FGeometry TimeSliderGeometry = FindChildGeometry(MyGeometry, TimeSlider.ToSharedRef());
		bDrawTooltip = TimeSliderGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
		if (bDrawTooltip)
		{
			float Alpha = TimeSlider->PositionToValue(TimeSliderGeometry, MouseEvent.GetScreenSpacePosition());
			if (DemoNetDriver.IsValid())
			{
				TooltipTime = DemoNetDriver->DemoTotalTime * Alpha;
			}

			float BestBookmarkDist = 1.0f;
			FString BestBookmarkID;
			for (int32 BookmarkIdx = 0; BookmarkIdx < CurrentBookmarks.Num(); BookmarkIdx++)
			{
				float BookmarkDist = FMath::Abs(Alpha - (CurrentBookmarks[BookmarkIdx].Time / DemoNetDriver->DemoTotalTime));
				if (BookmarkDist < 0.01)
				{
					if (!EventDataInfo.Contains(CurrentBookmarks[BookmarkIdx].ID))
					{
						if (!EventDataRequests.Contains(CurrentBookmarks[BookmarkIdx].ID))
						{
							EventDataRequests.Add(CurrentBookmarks[BookmarkIdx].ID);
							FOnRequestEventDataComplete RequestEventDataDelegate = FOnRequestEventDataComplete::CreateSP(this, &SUTReplayWindow::BookmarkDataReady, CurrentBookmarks[BookmarkIdx].ID, SelectedBookmark->GetText().ToString());
							DemoNetDriver->RequestEventData(CurrentBookmarks[BookmarkIdx].ID, RequestEventDataDelegate);
						}
					}

					if (BookmarkDist < BestBookmarkDist)
					{
						BestBookmarkDist = BookmarkDist;
						BestBookmarkID = CurrentBookmarks[BookmarkIdx].ID;
					}
				}
			}

			if (!BestBookmarkID.IsEmpty() && EventDataInfo.Contains(BestBookmarkID))
			{
				TooltipBookmarkText = EventDataInfo[BestBookmarkID];
			}
			else
			{
				TooltipBookmarkText.Empty();
			}

			//need to scale the position like we are doing for the whole widget
			float Scale = MyGeometry.Size.X / 1920.0f;
			ToolTipPos.X = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X / Scale;
			ToolTipPos.Y = MyGeometry.AbsoluteToLocal(TimeSliderGeometry.LocalToAbsolute(FVector2D(0.0f, 0.0f))).Y / Scale;
		}
	}

	return FReply::Handled(); //FReply::Unhandled();
}

FReply SUTReplayWindow::OnMarkRecordStartClicked()
{
	if (DemoNetDriver.IsValid())
	{
		RecordTimeStart = DemoNetDriver->DemoCurrentTime;
		TimeSlider->SetMarkStart(TimeSlider->GetValue());
		// Verify that time stop is in a valid spot
		if (RecordTimeStop > 0 && RecordTimeStop < RecordTimeStart)
		{
			RecordTimeStop = -1;
			TimeSlider->SetMarkEnd(-1);
		}
	}

	return FReply::Handled();
}

FReply SUTReplayWindow::OnMarkRecordStopClicked()
{
	if (DemoNetDriver.IsValid())
	{
		// Don't allow demo clips longer than 90 seconds right now, video temp files get really close to 4 gigs at that size
		if (RecordTimeStart > 0 && DemoNetDriver->DemoCurrentTime > RecordTimeStart && DemoNetDriver->DemoCurrentTime - RecordTimeStart < 90)
		{
			RecordTimeStop = DemoNetDriver->DemoCurrentTime;
			TimeSlider->SetMarkEnd(TimeSlider->GetValue());
		}
	}

	return FReply::Handled();
}

FReply SUTReplayWindow::OnRecordButtonClicked()
{
	if (DemoNetDriver.IsValid())
	{
		if (RecordTimeStart > 0 && RecordTimeStop > 0)
		{
			AWorldSettings* const WorldSettings = PlayerOwner->GetWorld()->GetWorldSettings();
			WorldSettings->Pauser = nullptr;

			PlayerOwner->GetWorld()->GetWorldSettings()->DemoPlayTimeDilation = 1.0f;
			DemoNetDriver->GotoTimeInSeconds(RecordTimeStart, FOnGotoTimeDelegate::CreateSP(this, &SUTReplayWindow::RecordSeekCompleted));
		}
	}

	return FReply::Handled();
}

FReply SUTReplayWindow::OnScreenshotButtonClicked()
{
	UUTProfileSettings* ProfileSettings = GetPlayerOwner()->GetProfileSettings();
	if (ProfileSettings)
	{
		GScreenshotResolutionX = ProfileSettings->ReplayScreenshotResX;
		GScreenshotResolutionY = ProfileSettings->ReplayScreenshotResY;
	}

	if (GScreenshotResolutionX <= 0)
	{
		// 4k resolution isn't actually >= 4000
		GScreenshotResolutionX = 3840;
		GScreenshotResolutionY = 2160;
	}

	PlayerOwner->ViewportClient->GetGameViewport()->TakeHighResScreenShot();

	return FReply::Handled();
}

FReply SUTReplayWindow::OnCommentButtonClicked()
{
	PlayerOwner->OpenDialog(SNew(SUTInputBoxDialog)
		.OnDialogResult(FDialogResultDelegate::CreateSP(this, &SUTReplayWindow::CommentDialogResult))
		.PlayerOwner(PlayerOwner)
		.DialogSize(FVector2D(700, 400))
		.bDialogSizeIsRelative(false)
		.DefaultInput(TEXT(""))
		.DialogTitle(NSLOCTEXT("ReplayWindow", "CommentTitle", "Enter Your Comment"))
		.MessageText(NSLOCTEXT("ReplayWindow", "Comment", "Your comment will be viewable by everyone, your player id is saved along with the comment.\nInappropriate comments will result in your account being closed. Please be nice!"))
		.ButtonMask(UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_CANCEL)
		);

	return FReply::Handled();
}

void SUTReplayWindow::CommentDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		TSharedPtr<SUTInputBoxDialog> Box = StaticCastSharedPtr<SUTInputBoxDialog>(Widget);
		if (Box.IsValid())
		{
			FString CommentText = Box->GetInputText();
			
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			if (OnlineSubsystem)
			{
				IOnlineIdentityPtr OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
				if (OnlineIdentityInterface.IsValid())
				{
					FString PlayerId = OnlineIdentityInterface->GetUniquePlayerId(0)->ToString();
					if (!PlayerId.IsEmpty() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
					{
						FString CommentEvent = FString::Printf(TEXT("%s: %s"), *PlayerOwner->PlayerController->PlayerState->PlayerName, *CommentText);

						TArray<uint8> Data;
						FMemoryWriter MemoryWriter(Data);
						MemoryWriter.Serialize(TCHAR_TO_ANSI(*CommentEvent), CommentEvent.Len() + 1);

						DemoNetDriver->AddEvent(TEXT("Comments"), PlayerId, Data);

						FEnumerateEventsCompleteDelegate EnumComments = FEnumerateEventsCompleteDelegate::CreateSP(this, &SUTReplayWindow::CommentsEnumerated);
						DemoNetDriver->EnumerateEvents(TEXT("Comments"), EnumComments);
					}
				}
			}
		}
	}
}

FReply SUTReplayWindow::OnScreenshotConfigButtonClicked()
{
	FDialogResultDelegate Callback;
	Callback.BindRaw(this, &SUTReplayWindow::ScreenshotConfigResult);

	TSharedPtr<SUTDialogBase> NewDialog; // important to make sure the ref count stays until OpenDialog()
	SAssignNew(NewDialog, SUTScreenshotConfigDialog)
		.PlayerOwner(PlayerOwner)
		.ButtonMask(UTDIALOG_BUTTON_OK)
		.OnDialogResult(Callback);

	PlayerOwner->OpenDialog(NewDialog.ToSharedRef());

	return FReply::Handled();
}

void SUTReplayWindow::ScreenshotConfigResult(TSharedPtr<SCompoundWidget> Dialog, uint16 ButtonID)
{
}

void SUTReplayWindow::RecordSeekCompleted(bool bSucceeded)
{
	if (bSucceeded)
	{
		PlayerOwner->RecordReplay(RecordTimeStop - RecordTimeStart);
	}
	else
	{
		RecordTimeStart = -1;
		RecordTimeStop = -1;
		TimeSlider->SetMarkStart(-1);
		TimeSlider->SetMarkEnd(-1);
	}
}

FVector2D SUTReplayWindow::GetTimeTooltipPosition() const
{
	return ToolTipPos;
}

FVector2D SUTReplayWindow::GetTimeTooltipSize() const
{
	return FVector2D(ToolTipCurrentSizeX, ToolTipCurrentSizeY);
}

FText SUTReplayWindow::GetTooltipText() const
{
	if (TooltipBookmarkText.IsEmpty())
	{
		return FText::AsTimespan(FTimespan(0, 0, static_cast<int32>(TooltipTime)));
	}

	return FText::FromString(TooltipBookmarkText);
}

EVisibility SUTReplayWindow::GetVis() const
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		if (!PC->MyUTHUD->bShowHUD)
		{
			return EVisibility::Collapsed;
		}
	}

	return PlayerOwner->AreMenusOpen() ? EVisibility::Collapsed : EVisibility::Visible;
}

void SUTReplayWindow::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	const EVisibility ChildVisibility = ChildSlot.GetWidget()->GetVisibility();
	if (ArrangedChildren.Accepts(ChildVisibility))
	{
		FVector2D DesiredDrawSize = FVector2D(1920.0f, 1080.0f);
		FVector2D ActualGeometrySize = AllottedGeometry.Size * AllottedGeometry.Scale;
		float Scale = 1.0f;
		if (AllottedGeometry.Size != DesiredDrawSize)
		{
			//Scale to fit the width of the screen
			Scale = AllottedGeometry.Size.X / DesiredDrawSize.X;
			DesiredDrawSize = ActualGeometrySize / Scale;
		}
		ArrangedChildren.AddWidget(ChildVisibility, AllottedGeometry.MakeChild(ChildSlot.GetWidget(), FVector2D(0.0f, 0.0f), DesiredDrawSize, Scale));
	}
}

void SUTReplayWindow::OnKillBookmarksSelected()
{
	if (KillEvents.Num() == 0)
	{
		TimeSlider->SetBookmarks(CurrentBookmarks);
		return;
	}

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	LastSpectedPlayerID = UTPC->LastSpectatedPlayerId;
	
	FString FilterID = GetSpectatedPlayerID();

	FBookmarkTimeAndColor TimeAndColor;
	TimeAndColor.Color = FLinearColor::Red;
	for (int32 EventsIdx = 0; EventsIdx < KillEvents.Num(); EventsIdx++)
	{
		if (!FilterID.IsEmpty() && FilterID != KillEvents[EventsIdx].Metadata)
		{
			continue;
		}

		TimeAndColor.ID = KillEvents[EventsIdx].ID;
		TimeAndColor.Time = KillEvents[EventsIdx].Time1 / 1000.0f;
		CurrentBookmarks.Add(TimeAndColor);
	}
	TimeSlider->SetBookmarks(CurrentBookmarks);
}

void SUTReplayWindow::OnMultiKillBookmarksSelected()
{
	if (MultiKillEvents.Num() == 0)
	{
		TimeSlider->SetBookmarks(CurrentBookmarks);
		return;
	}

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	LastSpectedPlayerID = UTPC->LastSpectatedPlayerId;
	
	FString FilterID = GetSpectatedPlayerID();

	FBookmarkTimeAndColor TimeAndColor;
	TimeAndColor.Color = FLinearColor::Yellow;
	for (int32 EventsIdx = 0; EventsIdx < MultiKillEvents.Num(); EventsIdx++)
	{
		if (!FilterID.IsEmpty() && FilterID != MultiKillEvents[EventsIdx].Metadata)
		{
			continue;
		}

		TimeAndColor.ID = MultiKillEvents[EventsIdx].ID;
		TimeAndColor.Time = MultiKillEvents[EventsIdx].Time1 / 1000.0f;
		CurrentBookmarks.Add(TimeAndColor);
	}
	TimeSlider->SetBookmarks(CurrentBookmarks);
}

FString SUTReplayWindow::GetSpectatedPlayerID()
{
	if (LastSpectedPlayerID > 0)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
		if (UTPC->GetWorld()->GetGameState())
		{
			const TArray<APlayerState*>& PlayerArray = UTPC->GetWorld()->GetGameState()->PlayerArray;
			for (int32 i = 0; i < PlayerArray.Num(); i++)
			{
				AUTPlayerState* UTPS = Cast<AUTPlayerState>(PlayerArray[i]);
				if (UTPS)
				{
					if (UTPS->SpectatingID == LastSpectedPlayerID)
					{
						// bots don't have StatsID, they just use playername right now
						if (!UTPS->StatsID.IsEmpty())
						{
							return UTPS->StatsID;
						}
						else
						{
							return UTPS->PlayerName;
						}
					}
				}
			}
		}
	}

	return TEXT("");
}

void SUTReplayWindow::OnSpreeKillBookmarksSelected()
{
	if (SpreeKillEvents.Num() == 0)
	{
		TimeSlider->SetBookmarks(CurrentBookmarks);
		return;
	}

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	LastSpectedPlayerID = UTPC->LastSpectatedPlayerId;

	FString FilterID = GetSpectatedPlayerID();

	FBookmarkTimeAndColor TimeAndColor;
	TimeAndColor.Color = FLinearColor::Yellow;
	for (int32 EventsIdx = 0; EventsIdx < SpreeKillEvents.Num(); EventsIdx++)
	{
		if (!FilterID.IsEmpty() && FilterID != SpreeKillEvents[EventsIdx].Metadata)
		{
			continue;
		}

		TimeAndColor.ID = SpreeKillEvents[EventsIdx].ID;
		TimeAndColor.Time = SpreeKillEvents[EventsIdx].Time1 / 1000.0f;
		CurrentBookmarks.Add(TimeAndColor);
	}
	TimeSlider->SetBookmarks(CurrentBookmarks);
}

void SUTReplayWindow::OnFlagReturnsBookmarksSelected()
{
	if (FlagReturnEvents.Num() == 0)
	{
		TimeSlider->SetBookmarks(CurrentBookmarks);
		return;
	}

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	LastSpectedPlayerID = UTPC->LastSpectatedPlayerId;

	FString FilterID = GetSpectatedPlayerID();

	FBookmarkTimeAndColor TimeAndColor;
	TimeAndColor.Color = FLinearColor::Yellow;
	for (int32 EventsIdx = 0; EventsIdx < FlagReturnEvents.Num(); EventsIdx++)
	{
		if (!FilterID.IsEmpty() && FilterID != FlagReturnEvents[EventsIdx].Metadata)
		{
			continue;
		}

		TimeAndColor.ID = FlagReturnEvents[EventsIdx].ID;
		TimeAndColor.Time = FlagReturnEvents[EventsIdx].Time1 / 1000.0f;
		CurrentBookmarks.Add(TimeAndColor);
	}
	TimeSlider->SetBookmarks(CurrentBookmarks);
}

void SUTReplayWindow::OnBookmarkSetSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	CurrentBookmarks.Empty();
	FBookmarkTimeAndColor TimeAndColor;

	if (!NewSelection.IsValid())
	{
		return;
	}

	SelectedBookmark->SetText(FText::FromString(*NewSelection));
	if (*NewSelection == TEXT("Bookmarks"))
	{
		// clear bookmarks
		TimeSlider->SetBookmarks(CurrentBookmarks);
	}
	else if (*NewSelection == TEXT("Kills"))
	{
		OnKillBookmarksSelected();
	}
	else if (*NewSelection == TEXT("Flag Captures"))
	{
		for (int32 EventsIdx = 0; EventsIdx < FlagCapEvents.Num(); EventsIdx++)
		{
			if (FlagCapEvents[EventsIdx].Metadata == TEXT("0"))
			{
				TimeAndColor.Color = FLinearColor::Red;
			}
			else
			{
				TimeAndColor.Color = FLinearColor::Blue;
			}
			TimeAndColor.ID = FlagCapEvents[EventsIdx].ID;
			TimeAndColor.Time = FlagCapEvents[EventsIdx].Time1 / 1000.0f;
			CurrentBookmarks.Add(TimeAndColor);
		}
		TimeSlider->SetBookmarks(CurrentBookmarks);
	}
	else if (*NewSelection == TEXT("Flag Denies"))
	{
		for (int32 EventsIdx = 0; EventsIdx < FlagDenyEvents.Num(); EventsIdx++)
		{
			if (FlagDenyEvents[EventsIdx].Metadata == TEXT("0"))
			{
				TimeAndColor.Color = FLinearColor::Red;
			}
			else
			{
				TimeAndColor.Color = FLinearColor::Blue;
			}
			TimeAndColor.ID = FlagDenyEvents[EventsIdx].ID;
			TimeAndColor.Time = FlagDenyEvents[EventsIdx].Time1 / 1000.0f;
			CurrentBookmarks.Add(TimeAndColor);
		}
		TimeSlider->SetBookmarks(CurrentBookmarks);
	}
	else if (*NewSelection == TEXT("Flag Returns"))
	{
		OnFlagReturnsBookmarksSelected();
	}
	else if (*NewSelection == TEXT("Multi Kills"))
	{
		OnMultiKillBookmarksSelected();
	}
	else if (*NewSelection == TEXT("Spree Kills"))
	{
		OnSpreeKillBookmarksSelected();
	}
}

void SUTReplayWindow::RefreshBookmarksComboBox()
{
	TSharedPtr<FString> SelectedItem = BookmarksComboBox->GetSelectedItem();

	BookmarkNameList.Empty();
	TSharedPtr<FString> DefaultVariant = MakeShareable(new FString(TEXT("Bookmarks")));
	BookmarkNameList.Add(DefaultVariant);

	if (KillEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Kills")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}
	if (FlagCapEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Flag Captures")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}
	if (FlagDenyEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Flag Denies")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}
	if (FlagReturnEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Flag Returns")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}
	if (MultiKillEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Multi Kills")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}
	if (SpreeKillEvents.Num())
	{
		TSharedPtr<FString> BookmarkName = MakeShareable(new FString(TEXT("Spree Kills")));
		BookmarkNameList.Add(BookmarkName);

		if (SelectedItem.IsValid() && *BookmarkName == *SelectedItem)
		{
			BookmarksComboBox->SetSelectedItem(BookmarkName);
		}
	}

	if (!SelectedItem.IsValid())
	{
		BookmarksComboBox->SetSelectedItem(DefaultVariant);
	}
}

TSharedRef<SWidget> SUTReplayWindow::GenerateStringListWidget(TSharedPtr<FString> InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*InItem.Get()))
			.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
		];
}

void SUTReplayWindow::BookmarkDataReady(const TArray<uint8>& Data, bool bSucceeded, FString EventID, FString EventType)
{
	EventDataRequests.Remove(EventID);

	if (bSucceeded)
	{
		if (Data.Num() > 0 && Data[Data.Num() - 1] == 0)
		{
			FString StringData(ANSI_TO_TCHAR((char*)Data.GetData()));

			if (EventType == TEXT("Kills"))
			{
				TArray<FString> Parsed;
				StringData.ParseIntoArray(Parsed, TEXT(" "), true);
				if (Parsed.Num() >= 2)
				{
					Parsed[0].ReplaceInline(TEXT("%20"), TEXT(" "));
					Parsed[1].ReplaceInline(TEXT("%20"), TEXT(" "));

					StringData = Parsed[0] + TEXT(" killed ") + Parsed[1];
				}
			}
			else if (EventType == TEXT("Multi Kills"))
			{
				TArray<FString> Parsed;
				StringData.ParseIntoArray(Parsed, TEXT(" "), true);
				if (Parsed.Num() >= 1)
				{
					Parsed[0].ReplaceInline(TEXT("%20"), TEXT(" "));
					StringData = Parsed[0];
				}
				
				if (Parsed.Num() >= 2)
				{
					Parsed[0].ReplaceInline(TEXT("%20"), TEXT(" "));

					if (Parsed[1] == TEXT("0"))
					{
						StringData = TEXT("Double Kill: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("1"))
					{
						StringData = TEXT("Multi Kill: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("2"))
					{
						StringData = TEXT("Ultra Kill: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("3"))
					{
						StringData = TEXT("Monster Kill: ") + Parsed[0];
					}
				}
				
			}
			else if (EventType == TEXT("Spree Kills"))
			{
				TArray<FString> Parsed;
				StringData.ParseIntoArray(Parsed, TEXT(" "), true);
				if (Parsed.Num() >= 2)
				{
					Parsed[0].ReplaceInline(TEXT("%20"), TEXT(" "));

					if (Parsed[1] == TEXT("1"))
					{
						StringData = TEXT("Killing Spree: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("2"))
					{
						StringData = TEXT("Rampage: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("3"))
					{
						StringData = TEXT("Dominating: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("4"))
					{
						StringData = TEXT("Unstoppable: ") + Parsed[0];
					}
					else if (Parsed[1] == TEXT("5"))
					{
						StringData = TEXT("Godlike: ") + Parsed[0];
					}
					else
					{
						StringData = Parsed[0] + TEXT(" with ") + Parsed[1];
					}
				}
			}

			EventDataInfo.Add(EventID, StringData);
		}
		else
		{
			FString EmptyStringData;
			EventDataInfo.Add(EventID, EmptyStringData);
		}
	}
}

TSharedRef<SWidget> SUTReplayWindow::MakeMarkRecordStartButton()
{
	if (DemoNetDriver.IsValid() && DemoNetDriver->ReplayStreamer.IsValid() && DemoNetDriver->ReplayStreamer->IsLive())
	{
		return SNew(SVerticalBox);
	}

	return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(32)
				.WidthOverride(32)
				[
					SAssignNew(RecordButton, SButton)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.OnClicked(this, &SUTReplayWindow::OnMarkRecordStartClicked)
					.Content()
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.MarkStart"))
					]
				]
			];
}

TSharedRef<SWidget> SUTReplayWindow::MakeMarkRecordStopButton()
{
	if (DemoNetDriver.IsValid() && DemoNetDriver->ReplayStreamer.IsValid() && DemoNetDriver->ReplayStreamer->IsLive())
	{
		return SNew(SVerticalBox);
	}

	return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(32)
				.WidthOverride(32)
				[
					SAssignNew(MarkStartButton, SButton)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.OnClicked(this, &SUTReplayWindow::OnMarkRecordStopClicked)
					.Content()
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.MarkEnd"))
					]
				]
			];
}

TSharedRef<SWidget> SUTReplayWindow::MakeRecordButton()
{
	if (DemoNetDriver.IsValid() && DemoNetDriver->ReplayStreamer.IsValid() && DemoNetDriver->ReplayStreamer->IsLive())
	{
		return SNew(SVerticalBox);
	}
	
	return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(32)
				.WidthOverride(32)
				[
					SAssignNew(MarkEndButton, SButton)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.OnClicked(this, &SUTReplayWindow::OnRecordButtonClicked)
					.Content()
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Replay.Button.Record"))
					]
				]
			];
}

FReply SUTReplayWindow::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && PlayerOwner.IsValid())
	{
		PlayerOwner->ShowMenu(TEXT(""));
		return FReply::Handled();
	}
	else if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->InputKey(InKeyEvent.GetKey(), EInputEvent::IE_Released, 1.f, false))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SUTReplayWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->InputKey(InKeyEvent.GetKey(), EInputEvent::IE_Pressed, 1.f, false))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool SUTReplayWindow::SupportsKeyboardFocus() const
{
	return true;
}

void SUTReplayWindow::GrabKeyboardFocus()
{
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);
}


#endif