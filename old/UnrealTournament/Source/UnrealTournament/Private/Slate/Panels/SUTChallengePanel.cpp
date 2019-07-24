
// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTChallengePanel.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "../Widgets/SUTScaleBox.h"
#include "../Widgets/SUTButton.h"
#include "UTChallengeManager.h"
#include "UTAnalytics.h"
#include "../Menus/SUTMainMenu.h"
#include "UTLevelSummary.h"
#include "UTGameEngine.h"
#include "../Widgets/SUTBorder.h"

#if !UE_SERVER
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"

SUTChallengePanel::~SUTChallengePanel()
{
	if (LevelScreenshot != NULL)
	{
		delete LevelScreenshot;
		LevelScreenshot = NULL;
	}
}
void SUTChallengePanel::ConstructPanel(FVector2D ViewportSize)
{
	LevelShot = nullptr;

	SelectedChallenge = NAME_None;
	ChallengeManager = Cast<UUTGameEngine>(GEngine)->GetChallengeManager();

	LastChallengeRevisionNumber = ChallengeManager.IsValid() ? ChallengeManager->RevisionNumber : 0;
	LevelScreenshot = new FSlateDynamicImageBrush(GEngine->DefaultTexture, FVector2D(256.0f, 128.0f), NAME_None);

	TSharedPtr<SUTButton> TabUnstarted;
	TSharedPtr<SUTButton> TabCompleted;
	TSharedPtr<SUTButton> TabExpired;
	TSharedPtr<SUTButton> TabAll;

	this->ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SAssignNew(AnimWidget, SUTBorder).OnAnimEnd(this, &SUTChallengePanel::AnimEnd)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SUTScaleBox)
						.bMaintainAspectRatio(false)
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush("UT.HomePanel.Background"))
						]
					]
				]
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(64.0, 15.0, 64.0, 15.0)
				.FillHeight(1.0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(1792).HeightOverride(860)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f,0.0f,8.0f,0.0f)
							[
								SNew(SBox).WidthOverride(900)
								[
									SNew(SVerticalBox)
	/*
									+SVerticalBox::Slot()
									.Padding(0.0,0.0,0.0,16.0)
									.AutoHeight()
									[
										SNew(SVerticalBox)
										+SVerticalBox::Slot()
										.FillHeight(1.0)
										.VAlign(VAlign_Center)
										[
											SNew(SButton)
											.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
											.OnClicked(this, &SUTChallengePanel::CustomClicked)
											[
												SNew(SBox).HeightOverride(96)
												[
													SNew(SBorder)
													.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
													[
														SNew(SHorizontalBox)
														+SHorizontalBox::Slot()
														.HAlign(HAlign_Center)
														.VAlign(VAlign_Center)
														[
															SNew(STextBlock)
															.Text(NSLOCTEXT("SUTChallengePanel","CustomChallenge","Create your own custom match."))
															.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium")
														]
													]									
												]
											]
										]
									]
									+SVerticalBox::Slot()
									.Padding(0.0,0.0,0.0,16.0)
									.AutoHeight()
									[
										SNew(SBorder)
										.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.HAlign(HAlign_Center)
											[
												SNew(STextBlock)
												.Text(NSLOCTEXT("SUTChallengePanel","AvailableChallenges","AVAILABLE CHALLENGES"))
												.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Bold")
											]
										]
									]
	*/
									+SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SBorder)
										.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
										[
											SNew(SVerticalBox)
											+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(FMargin(0.0f,0.0f,5.0f,0.0f))
											[
												SNew(SHorizontalBox)
												+ SHorizontalBox::Slot()
												.HAlign(HAlign_Center)
												.VAlign(VAlign_Center)
												[
													SNew(STextBlock)
													.Text(NSLOCTEXT("SUTChallengePanel", "AvailableChallenges", "Filter challenges by..."))
													.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
												]
											]
											+ SVerticalBox::Slot()
											.HAlign(HAlign_Fill)
											.AutoHeight()
											[
												SNew(SBox).HeightOverride(48)
												[
													SNew(SHorizontalBox)
													+ SHorizontalBox::Slot()
													.Padding(3.0, 0.0, 3.0, 0.0)
													[
														SNew(SBorder)
														.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
														[
															SAssignNew(TabUnstarted, SUTButton)
															.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
															.OnClicked(this, &SUTChallengePanel::TabChanged, 0)
															[
																SNew(SVerticalBox)
																+ SVerticalBox::Slot()
																.AutoHeight()
																.HAlign(HAlign_Center)
																[
																	SNew(STextBlock)
																	.Text(NSLOCTEXT("SUTChallengePanel", "NewChallenges", "Active"))
																	.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
																	.ColorAndOpacity(this, &SUTChallengePanel::GetTabColor, EChallengeFilterType::Active)
																]
															]
														]
													]
													+ SHorizontalBox::Slot()
													.Padding(3.0, 0.0, 3.0, 0.0)
													[
														SNew(SBorder)
														.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
														[
															SAssignNew(TabCompleted, SUTButton)
															.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
															.OnClicked(this, &SUTChallengePanel::TabChanged, 1)
															[
																SNew(SVerticalBox)
																+ SVerticalBox::Slot()
																.AutoHeight()
																.HAlign(HAlign_Center)
																[
																	SNew(STextBlock)
																	.Text(NSLOCTEXT("SUTChallengePanel", "CompletedChallenges", "Completed"))
																	.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
																	.ColorAndOpacity(this, &SUTChallengePanel::GetTabColor, EChallengeFilterType::Completed)
																]
															]
														]
													]
													+ SHorizontalBox::Slot()
													.Padding(3.0, 0.0, 3.0, 0.0)
													[
														SNew(SBorder)
														.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
														[
															SAssignNew(TabExpired, SUTButton)
															.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
															.OnClicked(this, &SUTChallengePanel::TabChanged, 2)
															[
																SNew(SVerticalBox)
																+ SVerticalBox::Slot()
																.AutoHeight()
																.HAlign(HAlign_Center)
																[
																	SNew(STextBlock)
																	.Text(NSLOCTEXT("SUTChallengePanel", "ExpiredChallenges", "Expired"))
																	.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
																	.ColorAndOpacity(this, &SUTChallengePanel::GetTabColor, EChallengeFilterType::Expired)

																]
															]
														]
													]
													+ SHorizontalBox::Slot()
													.Padding(3.0, 0.0, 3.0, 0.0)
													[
														SNew(SBorder)
														.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
														[
															SAssignNew(TabAll, SUTButton)
															.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
															.OnClicked(this, &SUTChallengePanel::TabChanged, 3)
															[
																SNew(SVerticalBox)
																+ SVerticalBox::Slot()
																.AutoHeight()
																.HAlign(HAlign_Center)
																[
																	SNew(STextBlock)
																	.Text(NSLOCTEXT("SUTChallengePanel", "AllChallenges", "All"))
																	.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
																	.ColorAndOpacity(this, &SUTChallengePanel::GetTabColor, EChallengeFilterType::All)
																]
															]
														]
													]
												]
											]
										]
									]

									+SVerticalBox::Slot()
									.FillHeight(1.0)
									[
										SNew(SBorder)
										.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
										[
											SNew(SScrollBox)
											.Style(SUWindowsStyle::Get(),"UT.ScrollBox.Borderless")
											+SScrollBox::Slot()
											[
												SAssignNew(ChallengeBox,SVerticalBox)
											]
										]
									]


								]
							]

							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(8.0f,0.0f,0.0f,0.0f)
							[
								SNew(SBox).WidthOverride(876)
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot()
									.Padding(0.0,0.0,0.0,16.0)
									.AutoHeight()
									[
										SNew(SBox).HeightOverride(438)
										[
											SNew(SImage)
											.Image(LevelScreenshot)
										]
									]

									+SVerticalBox::Slot()
									.Padding(10.0,0.0,10.0,16.0)
									.FillHeight(1.0)
									[
										SNew(SBorder)
										.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Medium"))
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.FillHeight(1.0)
											[
												SNew(SScrollBox)
												+ SScrollBox::Slot()
												.Padding(FMargin(0.0f, 5.0f, 0.0f, 5.0f))
												[
													SAssignNew(ChallengeDescription, SRichTextBlock)
													.Text(NSLOCTEXT("SUTChallengePanel", "Description", "This is the description"))	// Change me to a delegate call
													.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
													.Justification(ETextJustify::Center)
													.DecoratorStyleSet(&SUTStyle::Get())
													.AutoWrapText(true)
												]
											]
											+SVerticalBox::Slot()
											.AutoHeight()
											[
												SNew(SBox).HeightOverride(64)
												[
													SNew(SVerticalBox)
													+SVerticalBox::Slot()
													.AutoHeight()
													[
														SNew(STextBlock)
														.Text(this,&SUTChallengePanel::GetCurrentScoreText)
														.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Bold")
														.ColorAndOpacity(FLinearColor(0.5f,0.5f,0.5f))

													]
													+SVerticalBox::Slot()
													.AutoHeight()
													[
														SNew(STextBlock)
														.Text(this,&SUTChallengePanel::GetCurrentChallengeData)
														.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small.Bold")
														.ColorAndOpacity(FLinearColor(0.5f,0.5f,0.5f))
													]
											
												]
											]
										]
									]
									+SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SBorder)
										.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
										[
											SAssignNew(GoBox, SVerticalBox)
										]
									]
								]
							]
						]
					]
				]
			]
		]
	];

	ChallengeTabs.Add(TabUnstarted);
	ChallengeTabs.Add(TabCompleted);
	ChallengeTabs.Add(TabExpired);
	ChallengeTabs.Add(TabAll);

	TabChanged(0);
}

void SUTChallengePanel::BuildGoBox()
{
	GoBox->ClearChildren();
	
	if (ChallengeManager.IsValid() && ChallengeManager->Challenges.Contains(SelectedChallenge))
	{
		const FUTChallengeInfo Challenge = ChallengeManager->Challenges[SelectedChallenge];

		if (Challenge.bExpiredChallenge)
		{
			GoBox->AddSlot()
			.FillHeight(1.0)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUTChallengePanel", "ExpiredMessage", "This challenge has expired and can no longer be completed.  Please choose a different challenge."))
				.AutoWrapText(true)
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
			];
		}
		else
		{
			GoBox->AddSlot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUTChallengePanel", "DifficultyText", "Select Difficulty"))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
				]
			];

			GoBox->AddSlot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(96)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SNew(SButton)
							.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
							.OnClicked(this, &SUTChallengePanel::StartClicked, 0)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("SUTChallengePanel", "Level_Easy", "Easy"))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")

								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
								]
							]
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(3.0, 0.0, 3.0, 0.0)
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SNew(SButton)
							.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
							.OnClicked(this, &SUTChallengePanel::StartClicked, 1)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("SUTChallengePanel", "Level_Medium", "Medium"))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")

								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)

										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
								]
							]
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SNew(SButton)
							.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
							.OnClicked(this, &SUTChallengePanel::StartClicked, 2)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("SUTChallengePanel", "Level_Hard", "Hard"))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")

								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox).WidthOverride(32).HeightOverride(32)
										[
											SNew(SImage)
											.Image(this, &SUTChallengePanel::GetStarCompletedImage)
											.ColorAndOpacity(this, &SUTChallengePanel::GetSelectMatchColor)
										]
									]
								]
							]
						]
					]
				]
			];
		}
	}
}

const FSlateBrush* SUTChallengePanel::GetStarImage() const
{
	return SUTStyle::Get().GetBrush(SelectedStarStyle);
}

const FSlateBrush* SUTChallengePanel::GetStarCompletedImage() const

{
	return SUTStyle::Get().GetBrush(SelectedStarStyle_Completed);
}

void SUTChallengePanel::GenerateChallengeList()
{
	ChallengeBox->ClearChildren();
	LastReward = NAME_REWARD_None;

	RewardStars.Empty();

	if (ChallengeManager.IsValid())
	{
		if (PlayerOwner.IsValid())
		{
			if (PlayerOwner->ChallengeRevisionNumber < ChallengeManager->RevisionNumber)
			{
				PlayerOwner->ChallengeRevisionNumber = ChallengeManager->RevisionNumber;
				PlayerOwner->SaveConfig();
			}
		}

		TArray<const FUTChallengeInfo*> Challenges;

		// First count the stars
		ChallengeManager->GetChallenges(Challenges, EChallengeFilterType::All, PlayerOwner->GetProgressionStorage());
		for (int32 i = 0 ; i < Challenges.Num(); i++)
		{
			const FUTChallengeInfo* Challenge = Challenges[i];

			// Track it's overall star count....

			int32 Stars = PlayerOwner->GetChallengeStars(Challenge->Tag);
			if (RewardStars.Contains(Challenge->RewardTag))
			{
				RewardStars[Challenge->RewardTag] = RewardStars[Challenge->RewardTag] + Stars;
			}
			else
			{
				RewardStars.Add(Challenge->RewardTag, Stars);
			}
		}

		// Now build the list.
		Challenges.Empty();
		ChallengeManager->GetChallenges(Challenges, ChallengeFilter, PlayerOwner->GetProgressionStorage());


		// Now create the buttons
		for (int32 i = 0 ; i < Challenges.Num(); i++)
		{
			const FUTChallengeInfo* Challenge = Challenges[i];
			AddChallengeButton(Challenges[i]->Tag, *Challenges[i]);
		}
		
		if (Challenges.Num() > 0)
		{
			ChallengeClicked(Challenges[0]->Tag);
		}
		else
		{
			ChallengeClicked(NAME_None);
		}
	}
}

void SUTChallengePanel::AddChallengeButton(FName ChallengeTag, const FUTChallengeInfo& Challenge)
{
	TSharedPtr<SUTButton> Button;

	if (Challenge.RewardTag != LastReward && ChallengeManager->RewardCaptions.Contains(Challenge.RewardTag))
	{
		LastReward = Challenge.RewardTag;
		int32 StarCount = RewardStars[Challenge.RewardTag];

		FText Caption = FText::Format(ChallengeManager->RewardCaptions[LastReward], FText::AsNumber(StarCount));

		ChallengeBox->AddSlot()
		.Padding(10.0,48.0,10.0,15.0)
		.AutoHeight()
		.HAlign(HAlign_Right)
		[
			SNew(SBox).HeightOverride(42)
			[
				SNew(STextBlock)
				.Text(Caption)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Bold")
				.ShadowOffset(FVector2D(2.0f,2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.5))
				
			]
		];
	}

	FLinearColor StarColor = ChallengeManager->RewardInfo.Contains(LastReward) ? ChallengeManager->RewardInfo[LastReward].StarColor : FLinearColor(1.0,1.0,0.0,1.0);
	FName StarStyle = ChallengeManager->RewardInfo.Contains(LastReward) ? ChallengeManager->RewardInfo[LastReward].StarEmptyStyleTag: NAME_REWARDSTYLE_STAR;
	FName CompletedStarStyle = ChallengeManager->RewardInfo.Contains(LastReward) ? ChallengeManager->RewardInfo[LastReward].StarCompletedStyleTag: NAME_REWARDSTYLE_STAR_COMPLETED;

	ChallengeBox->AddSlot()
	.Padding(10.0,6.0,10.0,6.0)
	.AutoHeight()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(SBox).WidthOverride(860).HeightOverride(96)
			[
				SAssignNew(Button, SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
				.IsToggleButton(true)
				.OnClicked(this, &SUTChallengePanel::ChallengeClicked, ChallengeTag)
				.UTOnMouseOver(this, &SUTChallengePanel::OnUTMouseEnter)
				.UTOnMouseExit(this, &SUTChallengePanel::OnUTMouseExit)
				.WidgetNameTag(ChallengeTag)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush(Challenge.SlateUIImageName))
					]
					+SOverlay::Slot()
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush("UT.Background.Shadow"))
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SUTChallengePanel::GetShadowVis, ChallengeTag)))
					]

					+SOverlay::Slot()
					[
						CreateCheck(ChallengeTag)
					]
					+SOverlay::Slot()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(70.0,0.0,0.0,0.0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Challenge.Title))
							.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Bold")
							.ShadowOffset(FVector2D(2.0f,2.0f))
							.ShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.5))
						]
					]
					+SOverlay::Slot()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.HAlign(HAlign_Right)
						[
							CreateStars(ChallengeTag,StarColor, StarStyle, CompletedStarStyle)
						]
					]
				]
			]
		]
	];

	ButtonMap.Add(ChallengeTag, Button);



}

TSharedRef<SWidget> SUTChallengePanel::CreateCheck(FName ChallengeTag)
{
	int32 NoStars = PlayerOwner->GetChallengeStars(ChallengeTag);
	if (NoStars > 0)
	{
		return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0.0,0.0,2.0,0.0)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).WidthOverride(64).HeightOverride(64)
				[
					SNew(SImage)
					.Image(SUTStyle::Get().GetBrush("UT.Icon.Checkmark"))
				]
			]
		];
	}

	return SNew(SCanvas);
}

TSharedRef<SWidget> SUTChallengePanel::CreateStars(FName ChallengeTag, FLinearColor StarColor, FName StarStyle, FName CompletedStarStyle)
{
	int32 NoStars = PlayerOwner->GetChallengeStars(ChallengeTag);
	TSharedPtr<SHorizontalBox> Box;
	SAssignNew(Box, SHorizontalBox);

	for (int32 i=0; i < 3; i++)
	{
		if (i < NoStars)
		{
			Box->AddSlot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(0.0,0.0,2.0,0.0)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox).WidthOverride(32).HeightOverride(32)
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush(CompletedStarStyle))
						.ColorAndOpacity(FSlateColor(StarColor))
					]
				]
			];
		}
		else
		{
			Box->AddSlot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(0.0,0.0,2.0,0.0)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox).WidthOverride(32).HeightOverride(32)
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush(StarStyle))
						.ColorAndOpacity(FSlateColor(StarColor))
					]
				]
			];
		}
	}

	return Box.ToSharedRef();
}

FText SUTChallengePanel::GetYourScoreText() const
{
	return FText::Format(NSLOCTEXT("SUTChallengePanel", "StarTotalFormat", "You have earned {0} gold challenge stars"), FText::AsNumber(PlayerOwner->GetChallengeStars(NAME_REWARD_GoldStars)));
}

FText SUTChallengePanel::GetCurrentScoreText() const
{
	if (SelectedChallenge == NAME_None)
	{
		return FText::GetEmpty();
	}

	return FText::Format(NSLOCTEXT("SUTChallengePanel","StarsForChallengeFormat","You have earned {0} stars for this challenge"), FText::AsNumber(PlayerOwner->GetChallengeStars(SelectedChallenge)));
}

FText SUTChallengePanel::GetCurrentChallengeData() const
{
	if (SelectedChallenge == NAME_None)
	{
		return FText::GetEmpty();
	}
	else if (!ChallengeManager->Challenges.Contains(SelectedChallenge))
	{
		UE_LOG(UT, Warning, TEXT("SelectedChallenge not in list of Challenges"));
	}
	else if (ChallengeManager->Challenges[SelectedChallenge].bDailyChallenge)
	{
		int32 HoursLeft = ChallengeManager->TimeUntilExpiration(SelectedChallenge, PlayerOwner->GetProgressionStorage());
		return FText::Format(NSLOCTEXT("SUTChallengePanel","DateForChallengeFormatDaily","Last Completed: {0}  --  Expires in {1} hours"), FText::FromString(PlayerOwner->GetChallengeDate(SelectedChallenge)), FText::AsNumber(HoursLeft));
	}

	return FText::Format(NSLOCTEXT("SUTChallengePanel","DateForChallengeFormat","Last Completed: {0}"), FText::FromString(PlayerOwner->GetChallengeDate(SelectedChallenge)));
}


FReply SUTChallengePanel::ChallengeClicked(FName ChallengeTag)
{

	UE_LOG(UT,Log,TEXT("ChallengeTag %s"), *ChallengeTag.ToString());

	if (ChallengeTag == NAME_None)
	{
		SelectedChallenge = ChallengeTag;
		// Generate empty challenges..

		if (ChallengeFilter == EChallengeFilterType::Active)
		{
			// This is the active tab.  

			LevelShot = LoadObject<UTexture2D>(nullptr, TEXT("/Game/RestrictedAssets/SlateLargeImages/TayeApprovedBanner.TayeApprovedBanner"));
			if (LevelShot)
			{
				*LevelScreenshot = FSlateDynamicImageBrush(LevelShot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
			}

			ChallengeDescription->SetText(NSLOCTEXT("SUTChallengePanel","AllChallengesCompleted","Congratulations\n\nYou have completed all of the available challenges.  Keep checking back as new challenges will be made available for you or why not try to improve your score on one of the challenges you have already played."));
			if (GoBox.IsValid())
			{
				GoBox->ClearChildren();
			}
			return FReply::Handled();
		}
		else
		{
			// This is the active tab.  

			*LevelScreenshot = FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
			ChallengeDescription->SetText(NSLOCTEXT("SUTChallengePanel", "NoChallenges", "There are no challenges available in this group."));
			if (GoBox.IsValid())
			{
				GoBox->ClearChildren();
			}
			return FReply::Handled();
		}
	}

	if (SelectedChallenge == ChallengeTag) return FReply::Handled();

	if (ChallengeManager.IsValid() && ChallengeManager->Challenges.Contains(ChallengeTag))
	{
		FString Description = ChallengeManager->Challenges[ChallengeTag].Description;
		FString Map = ChallengeManager->Challenges[ChallengeTag].Map;

		FName RewardTag = ChallengeManager->Challenges[ChallengeTag].RewardTag;
		
		SelectedStarStyle = ChallengeManager->RewardInfo.Contains(RewardTag) ? ChallengeManager->RewardInfo[RewardTag].StarEmptyStyleTag : NAME_REWARDSTYLE_STAR;
		SelectedStarStyle_Completed  = ChallengeManager->RewardInfo.Contains(RewardTag) ? ChallengeManager->RewardInfo[RewardTag].StarCompletedStyleTag : NAME_REWARDSTYLE_STAR_COMPLETED;

		SelectedChallenge = ChallengeTag;

		BuildGoBox();

		for (auto It = ButtonMap.CreateConstIterator(); It; ++It)
		{
			TSharedPtr<SUTButton> Button = It.Value();
			FName Key = It.Key();
			if (Key == ChallengeTag)
			{
				Button->BePressed();
			}
			else
			{
				Button->UnPressed();
			}
		}

		bool bReset = true;

		TArray<FAssetData> MapAssets;
		GetAllAssetData(UWorld::StaticClass(), MapAssets, false);
		for (const FAssetData& Asset : MapAssets)
		{
			FString MapPackageName = Asset.PackageName.ToString();
			if (MapPackageName == Map)
			{
				const FString* Screenshot = Asset.TagsAndValues.Find(NAME_MapInfo_ScreenshotReference);
				const FString* MapDescription = Asset.TagsAndValues.Find(NAME_MapInfo_Description);

				FText Parsed = FText::GetEmpty();
				if (MapDescription != nullptr)
				{
					FTextStringHelper::ReadFromString(**MapDescription, Parsed);
				}

				if (MapDescription != NULL)
				{
					Description = Description + TEXT("\n\n<UT.Font.NormalText.Small.Bold>") + (Parsed.IsEmpty() ? *MapDescription : Parsed.ToString()) + TEXT("</>\n");
				}

				if (Screenshot != NULL)
				{
					LevelShot = LoadObject<UTexture2D>(nullptr, **Screenshot);
					if (LevelShot)
					{
						*LevelScreenshot = FSlateDynamicImageBrush(LevelShot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
					}
					else
					{
						*LevelScreenshot = FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
					}
					bReset = false;
					break;
				}
			}
		}

		if (bReset )
		{
			*LevelScreenshot = FSlateDynamicImageBrush(Cast<UUTGameEngine>(GEngine)->DefaultLevelScreenshot, LevelScreenshot->ImageSize, LevelScreenshot->GetResourceName());
		}

		ChallengeDescription->SetText(NSLOCTEXT("SUTChallengePanel", "NoChallenges", " Unlock upgraded teammates for every 5 gold stars earned.\n\n Unlock special rewards at 5, 15, 25, 40, and 60 gold stars earned.\n\n You will also earn XP for each match, allowing you to unlock other rewards as you level up."));
	}

	return FReply::Handled();
}

FReply SUTChallengePanel::StartClicked(int32 Difficulty)
{

	if (PlayerOwner->IsLoggedIn())
	{
		if (Difficulty > 0)
		{
			int32 NumStars = PlayerOwner->GetTotalChallengeStars();
			if (NumStars < 10)
			{
				const FUTChallengeInfo Challenge = ChallengeManager->Challenges[SelectedChallenge];
				if ((Challenge.PlayerTeamSize > 0) && (Challenge.RewardTag == NAME_REWARD_GoldStars))
				{
					PendingDifficulty = Difficulty;
					PlayerOwner->ShowMessage(NSLOCTEXT("SUTChallengePanel", "WeakRosterWarningTitle", "WARNING"), NSLOCTEXT("SUTChallengePanel", "WeakRosterWarning", "Your current roster of teammates may not be strong enough to take on this challenge at this skill level.  You will unlock roster upgrades for every 5 stars earned.  Do you wish to continue?"), UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateSP(this, &SUTChallengePanel::WarningResult));
					return FReply::Handled();
				}
			}
		}
		StartChallenge(Difficulty);
	}
	else
	{
		PendingDifficulty = Difficulty;
		PlayerOwner->ShowMessage(NSLOCTEXT("SUTChallengePanel", "NotLoggedInWarningTitle", "WARNING"), NSLOCTEXT("SUTChallengePanel", "NotLoggedInWarning", "You are not logged in.  Any progress you make on this challenge will not be saved.  Do you wish to continue?"), UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateSP(this, &SUTChallengePanel::WarningResult));
	}

	return FReply::Handled();
}

void SUTChallengePanel::WarningResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		StartChallenge(PendingDifficulty);
	}
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName StartChallenge
*
* @Trigger Fires when starting a challenge
*
* @Type Sent by the client
*
* @EventParam Challenge string Name of the challenge started
* @EventParam Difficulty int32 value representing the difficulty of the challenge 
*
* @Comments
*/
void SUTChallengePanel::StartChallenge(int32 Difficulty)
{
	if (ChallengeManager.IsValid() && ChallengeManager->Challenges.Contains(SelectedChallenge))
	{
		const FUTChallengeInfo Challenge = ChallengeManager->Challenges[SelectedChallenge];

		// Kill any existing Dedicated servers
		if (PlayerOwner->DedicatedServerProcessHandle.IsValid())
		{
			FPlatformProcess::TerminateProc(PlayerOwner->DedicatedServerProcessHandle,true);
			PlayerOwner->DedicatedServerProcessHandle.Reset();
		}

		FString Options = FString::Printf(TEXT("%s%s?Challenge=%s?ChallengeDiff=%i"), *Challenge.Map, *Challenge.GameURL, *SelectedChallenge.ToString(), Difficulty);

		if (FUTAnalytics::IsAvailable())
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Challenge"), SelectedChallenge.ToString()));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Difficulty"), Difficulty));
			FUTAnalytics::GetProvider().RecordEvent( TEXT("StartChallenge"), ParamArray );
			FUTAnalytics::SetClientInitialParameters(Cast<AUTPlayerController>(PlayerOwner->PlayerController), ParamArray, false);

			FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(PlayerOwner->PlayerController), "Challenge");

			Options += FUTAnalytics::AnalyticsLoggedGameOptionTrue;

			PlayerOwner->CheckLoadingMovie(Challenge.GameURL);

		}

		ConsoleCommand(TEXT("Open ") + Options);
	}	
}

FReply SUTChallengePanel::CustomClicked()
{
	TSharedPtr<SUTMainMenu> MainMenu = StaticCastSharedPtr<SUTMainMenu>(PlayerOwner->GetCurrentMenu());
	if (MainMenu.IsValid())
	{
		MainMenu->ShowCustomGamePanel();
	}

	return FReply::Handled();
}

void SUTChallengePanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// If the challenge revision has changed, rebuild the list.
	if (ChallengeManager.IsValid() && ChallengeManager->RevisionNumber != LastChallengeRevisionNumber)
	{
		LastChallengeRevisionNumber = ChallengeManager->RevisionNumber;
		GenerateChallengeList();
	}
}

FSlateColor SUTChallengePanel::GetSelectMatchColor() const
{
	if (SelectedChallenge != NAME_None)
	{
		const FUTChallengeInfo Challenge = ChallengeManager->Challenges[SelectedChallenge];
		FLinearColor StarColor = ChallengeManager->RewardInfo.Contains(Challenge.RewardTag) ? ChallengeManager->RewardInfo[Challenge.RewardTag].StarColor : FLinearColor(1.0,1.0,0.0,1.0);
		return FSlateColor(StarColor);
	}
	return FSlateColor(FLinearColor::White);
}

FReply SUTChallengePanel::TabChanged(int32 Index)
{
	switch (Index)
	{
		case 0 : ChallengeFilter = EChallengeFilterType::Active; break;
		case 1 : ChallengeFilter = EChallengeFilterType::Completed; break;
		case 2 : ChallengeFilter = EChallengeFilterType::Expired; break;
		default: ChallengeFilter = EChallengeFilterType::All; break;
	}

	for (int32 i=0; i< ChallengeTabs.Num(); i++)
	{
		if (i == Index)
		{
			ChallengeTabs[i]->BePressed();
		}
		else
		{
			ChallengeTabs[i]->UnPressed();
		}
	}

	GenerateChallengeList();
	return FReply::Handled();
}

FSlateColor SUTChallengePanel::GetTabColor(EChallengeFilterType::Type TargetFilter) const
{
	if (TargetFilter == ChallengeFilter) return FSlateColor(FLinearColor::Yellow);
	return FSlateColor(FLinearColor::White);
}

void SUTChallengePanel::OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow)
{
	SUTPanelBase::OnShowPanel(inParentWindow);

	if (AnimWidget.IsValid())
	{
		AnimWidget->Animate(FVector2D(200, 0), FVector2D(0, 0), 0.0f, 1.0f, 0.3f);
	}
}

void SUTChallengePanel::OnHidePanel()
{
	bClosing = true;
	if (AnimWidget.IsValid())
	{
		AnimWidget->Animate(FVector2D(0, 0), FVector2D(200, 0), 1.0f, 0.0f, 0.3f);
	}
	else
	{
		SUTPanelBase::OnHidePanel();
	}
}


void SUTChallengePanel::AnimEnd()
{
	if (bClosing)
	{
		bClosing = false;
		TSharedPtr<SWidget> Panel = this->AsShared();
		ParentWindow->PanelHidden(Panel);
		ParentWindow.Reset();
	}
}


void SUTChallengePanel::OnUTMouseEnter(const TSharedPtr<SUTButton> Target)
{
	if (ChallengeManager->Challenges.Contains(Target->WidgetNameTag))
	{
		ChallengeManager->Challenges[Target->WidgetNameTag].bHighlighted = true;
	}
}

void SUTChallengePanel::OnUTMouseExit(const TSharedPtr<SUTButton> Target)
{
	if (ChallengeManager->Challenges.Contains(Target->WidgetNameTag))
	{
		ChallengeManager->Challenges[Target->WidgetNameTag].bHighlighted = false;
	}
}

EVisibility SUTChallengePanel::GetShadowVis(FName ChallengeTag)
{
	if (ChallengeTag == SelectedChallenge) return EVisibility::Collapsed;

	if (ChallengeManager->Challenges.Contains(ChallengeTag))
	{
		return !ChallengeManager->Challenges[ChallengeTag].bHighlighted ? EVisibility::Visible : EVisibility::Collapsed;

	}

	return EVisibility::Visible;
}

#endif