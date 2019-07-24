// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTPlayerSettingsDialog.h"
#include "SUWindowsStyle.h"
#include "SUTWeaponConfigDialog.h"
#include "SNumericEntryBox.h"
#include "UTCanvasRenderTarget2D.h"
#include "EngineModule.h"
#include "SlateMaterialBrush.h"
#include "UTPlayerCameraManager.h"
#include "UTCharacterContent.h"
#include "UTWeap_ShockRifle.h"
#include "UTWeaponAttachment.h"
#include "UTHUD.h"
#include "Engine/UserInterfaceSettings.h"
#include "UTGameEngine.h"
#include "UTFlagInfo.h"
#include "../SUTStyle.h"
#include "../Widgets/SUTButton.h"

#if !UE_SERVER
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"
#endif

#include "AssetData.h"

#if !UE_SERVER

// scale factor for weapon/view bob sliders (i.e. configurable value between 0 and this)
const float SUTPlayerSettingsDialog::BOB_SCALING_FACTOR = 1.f;

#include "SScaleBox.h"
#include "Widgets/SDragImage.h"

#define LOCTEXT_NAMESPACE "SUTPlayerSettingsDialog"

void SUTPlayerSettingsDialog::Construct(const FArguments& InArgs)
{
	FVector2D ViewportSize;
	InArgs._PlayerOwner->ViewportClient->GetViewportSize(ViewportSize);

	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(NSLOCTEXT("SUTMenuBase", "PlayerSettings", "Player Settings"))
							.DialogSize(FVector2D(1900.0,1060.0))
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(FVector2D(0,0))
							.IsScrollable(false)
							.ButtonMask(UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_CANCEL)
							.OnDialogResult(InArgs._OnDialogResult)
						);

	PlayerPreviewMesh = nullptr;
	PreviewWeapon = nullptr;
	bSpinPlayer = true;
	bLeaderHatSelectedLast = false;
	bSkipPlayingGroupTauntBGMusic = false;

	bSkipWorldRender = true;

	CameraLocations.Add(FVector(-150.0f, -25.0f, -125.0f));
	CameraLocations.Add(FVector(90.0f, -75.0f, -60.0f));
	CameraLocations.Add(FVector(400.0f, -150.0f, -40.0f));
	CurrentCam = 1;
	CamLocation = CameraLocations[1];

	AvatarList.Add(FName("UT.Avatar.0"));
	AvatarList.Add(FName("UT.Avatar.1"));
	AvatarList.Add(FName("UT.Avatar.2"));
	AvatarList.Add(FName("UT.Avatar.3"));
	AvatarList.Add(FName("UT.Avatar.4"));
	AvatarList.Add(FName("UT.Avatar.5"));
	AvatarList.Add(FName("UT.Avatar.6"));

	WeaponConfigDelayFrames = 0;

	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine != nullptr && GetPlayerOwner().IsValid())
	{
		for (auto FlagPair : UTEngine->FlagList)
		{
			if (FlagPair.Value->IsEntitled(GetPlayerOwner()->CommunityRole))
			{
				CountryFlags.Add(FlagPair.Value);
			}
		}

		//Sort the flag list by priority and alphabetically
		CountryFlags.Sort([](const TWeakObjectPtr<UUTFlagInfo>& A, const TWeakObjectPtr<UUTFlagInfo>& B) -> bool
		{
			return A->Priority < B->Priority || (A->Priority == B->Priority && A->GetFriendlyName() < B->GetFriendlyName());
		});
	}

	// allocate a preview scene for rendering
	PlayerPreviewWorld = UWorld::CreateWorld(EWorldType::GamePreview, true);
	PlayerPreviewWorld->bShouldSimulatePhysics = true;
	GEngine->CreateNewWorldContext(EWorldType::GamePreview).SetCurrentWorld(PlayerPreviewWorld);
	PlayerPreviewWorld->InitializeActorsForPlay(FURL(), true);
	ViewState.Allocate();
	{
		UClass* EnvironmentClass = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewEnvironment.PlayerPreviewEnvironment_C"));
		PreviewEnvironment = PlayerPreviewWorld->SpawnActor<AActor>(EnvironmentClass, FVector(500.f, 50.f, 0.f), FRotator(0, 0, 0));
	}
	
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(NULL, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewProxy.PlayerPreviewProxy"));
	if (BaseMat != NULL)
	{
		PlayerPreviewTexture = Cast<UUTCanvasRenderTarget2D>(UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetPlayerOwner().Get(), UUTCanvasRenderTarget2D::StaticClass(), ViewportSize.X, ViewportSize.Y));
		PlayerPreviewTexture->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
		PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.BindSP(this, &SUTPlayerSettingsDialog::UpdatePlayerRender);
		PlayerPreviewMID = UMaterialInstanceDynamic::Create(BaseMat, PlayerPreviewWorld);
		PlayerPreviewMID->SetTextureParameterValue(FName(TEXT("TheTexture")), PlayerPreviewTexture);
		PlayerPreviewBrush = new FSlateMaterialBrush(*PlayerPreviewMID, ViewportSize);
	}
	else
	{
		PlayerPreviewTexture = NULL;
		PlayerPreviewMID = NULL;
		PlayerPreviewBrush = new FSlateMaterialBrush(*UMaterial::GetDefaultMaterial(MD_Surface), ViewportSize);
	}

	PlayerPreviewTexture->TargetGamma = GEngine->GetDisplayGamma();
	PlayerPreviewTexture->InitCustomFormat(ViewportSize.X, ViewportSize.Y, PF_B8G8R8A8, false);
	PlayerPreviewTexture->UpdateResourceImmediate();

	FVector2D ResolutionScale(ViewportSize.X / 1280.0f, ViewportSize.Y / 720.0f);

	UUTGameUserSettings* Settings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	UUTProfileSettings* ProfileSettings = PlayerOwner->GetProfileSettings();
	
	{
		HatList.Add(MakeShareable(new FString(TEXT("No Hat"))));
		HatPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTHat::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTHat::StaticClass()) && !TestClass->IsChildOf(AUTHatLeader::StaticClass()))
				{
					if (!TestClass->GetDefaultObject<AUTHat>()->bOverrideOnly)
					{
						HatList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTHat>()->CosmeticName)));
						HatPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
					}
				}
			}
		}
	}

	{
		LeaderHatList.Add(MakeShareable(new FString(TEXT("No Hat"))));
		LeaderHatPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTHatLeader::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTHatLeader::StaticClass()))
				{
					LeaderHatList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTHatLeader>()->CosmeticName)));
					LeaderHatPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
				}
			}
		}
	}

	HatVariantList.Add(MakeShareable(new FString(TEXT("Default"))));
	EyewearVariantList.Add(MakeShareable(new FString(TEXT("Default"))));

	{
		EyewearList.Add(MakeShareable(new FString(TEXT("No Glasses"))));
		EyewearPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTEyewear::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTEyewear::StaticClass()))
				{
					EyewearList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTEyewear>()->CosmeticName)));
					EyewearPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
				}
			}
		}
	}

	{
		CharacterList.Add(MakeShareable(new FString(TEXT("Taye"))));
		CharacterPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTCharacterContent::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				// TODO: long term should probably delayed load this... but would need some way to look up the loc'ed display name without loading the class (currently impossible...)
				if ( TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTCharacterContent::StaticClass()) &&
					TestClass->GetDefaultObject<AUTCharacterContent>()->GetMesh()->SkeletalMesh != NULL &&
					 !TestClass->GetDefaultObject<AUTCharacterContent>()->bHideInUI )
				{
					CharacterList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTCharacterContent>()->DisplayName.ToString())));
					CharacterPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
				}
			}
		}
	}

	{
		GroupTauntList.Add(MakeShareable(new FString(TEXT("No Taunt"))));
		GroupTauntPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTGroupTaunt::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTGroupTaunt::StaticClass()))
				{
					GroupTauntList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTGroupTaunt>()->DisplayName)));
					GroupTauntPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
				}
			}
		}
	}

	{
		TauntList.Add(MakeShareable(new FString(TEXT("No Taunt"))));
		TauntPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTTaunt::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTTaunt::StaticClass()))
				{
					TauntList.Add(MakeShareable(new FString(TestClass->GetDefaultObject<AUTTaunt>()->DisplayName)));
					TauntPathList.Add(Asset.ObjectPath.ToString() + TEXT("_C"));
				}
			}
		}
	}

	{
		IntroList.Add(MakeShareable(new FString(TEXT("Random Intro"))));
		IntroPathList.Add(TEXT(""));

		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTLineUpZone::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if (ClassPath != NULL)
			{
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTLineUpZone::StaticClass()))
				{
					AUTLineUpZone* LineUpZone = TestClass->GetDefaultObject<AUTLineUpZone>();
					if (LineUpZone)
					{
						for (TSubclassOf<AUTIntro>& IntroClass : LineUpZone->DefaultIntros)
						{
							AUTIntro* Intro = IntroClass->GetDefaultObject<AUTIntro>();
							if (Intro)
							{
								IntroList.Add(MakeShareable(new FString(Intro->DisplayName)));
								IntroPathList.Add(IntroClass->GetPathName());
							}
						}
					}
				}
			}
		}
	}

	SelectedPlayerColor = GetDefault<AUTPlayerController>()->FFAPlayerColor;
	SelectedHatVariantIndex = GetPlayerOwner()->GetHatVariant();
	SelectedEyewearVariantIndex = GetPlayerOwner()->GetEyewearVariant();

	FMargin NameColumnPadding = FMargin(10, 4);
	FMargin ValueColumnPadding = FMargin(0, 4);
	const float NameColumnForcedSizing = 250.f;
	
	float FOVSliderSetting = ((ProfileSettings ? ProfileSettings->PlayerFOV : 100)- FOV_CONFIG_MIN) / (FOV_CONFIG_MAX - FOV_CONFIG_MIN);

	if (GetPlayerOwner().IsValid() && UTEngine)
	{
		SelectedFlag = UTEngine->GetFlag(GetPlayerOwner()->GetCountryFlag());
	}

	if (SelectedFlag == nullptr && CountryFlags.Num() > 0)
	{
		SelectedFlag = CountryFlags[0].Get();
	}

	if (DialogContent.IsValid())
	{




		const float MessageTextPaddingX = 10.0f;
		TSharedPtr<STextBlock> MessageTextBlock;
		DialogContent->AddSlot()
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				[
					SNew(SDragImage)
					.Image(PlayerPreviewBrush)
					.OnDrag(this, &SUTPlayerSettingsDialog::DragPlayerPreview)
					.OnZoom(this, &SUTPlayerSettingsDialog::ZoomPlayerPreview)
				]
			]

			+ SOverlay::Slot()
			[
				SNew(SHorizontalBox)
			
				+ SHorizontalBox::Slot()
				.FillWidth(0.50f)
				[
					SNew(SSpacer)
					.Size(FVector2D(1,1))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.35f)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
				
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("ClanName", "Clan Name"))
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(ClanName, SEditableTextBox)
							.OnTextChanged(this, &SUTPlayerSettingsDialog::OnNameTextChanged)
							.Text(FText::FromString(GetPlayerOwner()->GetClanName()))
							.Style(SUWindowsStyle::Get(), "UT.Common.Editbox.White")
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()

					[
						SNew(SGridPanel)

						// Country Flag
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 0)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("CountryFlag", "Country Flag"))
						]

						+ SGridPanel::Slot(1, 0)
						.Padding(ValueColumnPadding)
						[
							SNew( SBox )
							.WidthOverride(250.f)
							[
								SAssignNew(CountryFlagComboBox, SComboBox< TWeakObjectPtr<UUTFlagInfo> >)
								.InitiallySelectedItem(SelectedFlag)
								.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
								.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
								.OptionsSource(&CountryFlags)
								.OnGenerateWidget(this, &SUTPlayerSettingsDialog::GenerateFlagListWidget)
								.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnFlagSelected)
								.Content()
								[
									SAssignNew(SelectedFlagWidget, SOverlay).Visibility(EVisibility::HitTestInvisible)
								]
							]
						]

						// Hat
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 1)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("HatSelectionLabel", "Hat"))
						]

						+ SGridPanel::Slot(1, 1)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(HatComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&HatList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnHatSelected)
							.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
							.Content()
							[
								SAssignNew(SelectedHat, STextBlock)
								.Text(FText::FromString(TEXT("No Hats Available")))
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
							]
						]
						
						+ SGridPanel::Slot(2, 1)
						.Padding(NameColumnPadding)
						[
							SAssignNew(HatVariantComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&HatVariantList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnHatVariantSelected)
							.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
							.Content()
							[
								SAssignNew(SelectedHatVariant, STextBlock)
								.Text(FText::FromString(TEXT("Default")))
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
							]
						]

						+ SGridPanel::Slot(0, 2)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("LeaderHatSelectionLabel", "Leader Hat"))
						]

						+ SGridPanel::Slot(1, 2)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(LeaderHatComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&LeaderHatList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnLeaderHatSelected)
							.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
							.Content()
							[
								SAssignNew(SelectedLeaderHat, STextBlock)
								.Text(FText::FromString(TEXT("No Leader Hats Available")))
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
							]
						]
						// Eyewear
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 3)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("EyewearSelectionLabel", "Eyewear"))
						]

						+ SGridPanel::Slot(1, 3)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(EyewearComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&EyewearList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnEyewearSelected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedEyewear, STextBlock)
									.Text(FText::FromString(TEXT("No Glasses Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]
						
						+ SGridPanel::Slot(2, 3)
						.Padding(NameColumnPadding)
						[
							SAssignNew(EyewearVariantComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&EyewearVariantList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnEyewearVariantSelected)
							.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
							.Content()
							[
								SAssignNew(SelectedEyewearVariant, STextBlock)
								.Text(FText::FromString(TEXT("Default")))
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
							]
						]
						
						+ SGridPanel::Slot(0, 4)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("GroupTauntSelectionLabel", "Group Taunt"))
						]

						+ SGridPanel::Slot(1, 4)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(GroupTauntComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&GroupTauntList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnGroupTauntSelected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedGroupTaunt, STextBlock)
									.Text(FText::FromString(TEXT("No Taunts Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]

						// Taunt
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 5)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("TauntSelectionLabel", "Taunt"))
						]

						+ SGridPanel::Slot(1, 5)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(TauntComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&TauntList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnTauntSelected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedTaunt, STextBlock)
									.Text(FText::FromString(TEXT("No Taunts Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]
						
						// Taunt 2
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 6)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("Taunt2SelectionLabel", "Taunt 2"))
						]

						+ SGridPanel::Slot(1, 6)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(Taunt2ComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&TauntList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnTaunt2Selected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedTaunt2, STextBlock)
									.Text(FText::FromString(TEXT("No Taunts Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]

						// Character
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 7)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("CharSelectionLabel", "Character"))
						]

						+ SGridPanel::Slot(1, 7)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(CharacterComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&CharacterList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnCharacterSelected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedCharacter, STextBlock)
									.Text(FText::FromString(TEXT("No Characters Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]

						// Line Up Intro
						// ---------------------------------------------------------------------------------
						+SGridPanel::Slot(0, 8)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("IntroSelectionLabel", "Character Introduction"))
						]

						+ SGridPanel::Slot(1, 8)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(IntroComboBox, SComboBox< TSharedPtr<FString> >)
							.InitiallySelectedItem(0)
							.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
							.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
							.OptionsSource(&IntroList)
							.OnGenerateWidget(this, &SUTDialogBase::GenerateStringListWidget)
							.OnSelectionChanged(this, &SUTPlayerSettingsDialog::OnIntroSelected)
							.Content()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(10.0f, 0.0f, 10.0f, 0.0f)
								[
									SAssignNew(SelectedIntro, STextBlock)
									.Text(FText::FromString(TEXT("No Intros Available")))
									.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
								]
							]
						]
					]

					/*
					TODO FIX ME
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("FFAPlayerColor", "Free for All Player Color"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(ValueColumnPadding)
						[
							SNew(SComboButton)
							.Method(EPopupMethod::UseCurrentWindow)
							.MenuPlacement(MenuPlacement_ComboBoxRight)
							.HasDownArrow(false)
							.ContentPadding(0)
							.VAlign(VAlign_Fill)
							.ButtonContent()
							[
								SNew(SColorBlock)
								.Color(this, &SUTPlayerSettingsDialog::GetSelectedPlayerColor)
								.IgnoreAlpha(true)
								.Size(FVector2D(32.0f * ResolutionScale.X, 16.0f * ResolutionScale.Y))
							]
							.MenuContent()
							[
								SNew(SBorder)
								.BorderImage(SUWindowsStyle::Get().GetBrush("UWindows.Standard.Dialog.Background"))
								[
									SNew(SColorPicker)
									.OnColorCommitted(this, &SUTPlayerSettingsDialog::PlayerColorChanged)
									.TargetColorAttribute(SelectedPlayerColor)
								]
							]
						]
					]*/

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSpacer)
						.Size(FVector2D(30, 30))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4)
					[
						SNew(SGridPanel)
						.FillColumn(1, 1)

						// Weapon Bob
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 0)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("WeaponBobScaling", "Weapon Bob"))
						]

						+ SGridPanel::Slot(1, 0)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(WeaponBobScaling, SSlider)
							.IndentHandle(false)
							.Orientation(Orient_Horizontal)
							.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
							.Value(GetDefault<AUTPlayerController>()->WeaponBobGlobalScaling / BOB_SCALING_FACTOR)
						]

						// View Bob
						// ---------------------------------------------------------------------------------
						+ SGridPanel::Slot(0, 1)
						.Padding(NameColumnPadding)
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(LOCTEXT("ViewBobScaling", "View Bob"))
						]

						+ SGridPanel::Slot(1, 1)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(ViewBobScaling, SSlider)
							.IndentHandle(false)
							.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
							.Orientation(Orient_Horizontal)
							.Value(GetDefault<AUTPlayerController>()->EyeOffsetGlobalScaling / BOB_SCALING_FACTOR)
						]
						// FOV
						+ SGridPanel::Slot(0, 2)
						.Padding(NameColumnPadding)
						[
							SNew(SBox)
							.WidthOverride(NameColumnForcedSizing)
							[
								SAssignNew(FOVLabel, STextBlock)
								.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
								.Text(FText::FromString(GetFOVLabelText(FOVSliderSetting)))
							]
						]
						+ SGridPanel::Slot(1, 2)
						.Padding(ValueColumnPadding)
						[
							SAssignNew(FOV, SSlider)
							.IndentHandle(false)
							.Style(SUWindowsStyle::Get(), "UT.Common.Slider")
							.Orientation(Orient_Horizontal)
							.Value(FOVSliderSetting)
							.OnValueChanged(this, &SUTPlayerSettingsDialog::OnFOVChange)
						]
					]


					// ------------------------ Avatar

					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0,20.0)
					[
						SNew(STextBlock)
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
						.Text(LOCTEXT("Avatar", "Select your avatar"))
					]

					+SVerticalBox::Slot()
					.AutoHeight().HAlign(HAlign_Fill)
					.Padding(0.0,5.0,0,0)
					[
						SNew(SBox).HeightOverride(148)
						[
							SNew(SScrollBox).Orientation(EOrientation::Orient_Vertical)
							+SScrollBox::Slot()
							[
								SAssignNew(AvatarGrid, SGridPanel)
							]
						]
					]

					//+ SVerticalBox::Slot()
					//.AutoHeight()
					//[
					//	SNew(SButton)
					//	.HAlign(HAlign_Center)
					//	.ButtonStyle(SUWindowsStyle::Get(), "UWindows.Standard.Button")
					//	.ContentPadding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
					//	.Text(LOCTEXT("WeaponConfig", "Weapon Settings"))
					//	.OnClicked(this, &SUTPlayerSettingsDialog::WeaponConfigClick)
					//]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.15f)
				[
					SNew(SSpacer)
					.Size(FVector2D(1,1))
				]
			]
		];

		SelectedAvatar = GetPlayerOwner()->GetAvatar();
		for (int32 i = 0 ; i <AvatarList.Num(); i++)
		{
			TSharedPtr<SUTButton> Button = AddAvatar(AvatarList[i], i);
			if (AvatarList[i] == SelectedAvatar)
			{
				Button->BePressed();
			}
		}

		bool bFoundSelectedHat = false;
		for (int32 i = 0; i < HatPathList.Num(); i++)
		{
			if (HatPathList[i] == GetPlayerOwner()->GetHatPath())
			{
				HatComboBox->SetSelectedItem(HatList[i]);
				bFoundSelectedHat = true;
				break;
			}
		}
		if (!bFoundSelectedHat && HatPathList.Num() > 0)
		{
			HatComboBox->SetSelectedItem(HatList[0]);
		}
		PopulateHatVariants();
		if (SelectedHatVariantIndex > 0 && SelectedHatVariantIndex < HatVariantList.Num())
		{
			HatVariantComboBox->SetSelectedItem(HatVariantList[SelectedHatVariantIndex]);
		}
		
		bool bFoundSelectedLeaderHat = false;
		for (int32 i = 0; i < LeaderHatPathList.Num(); i++)
		{
			if (LeaderHatPathList[i] == GetPlayerOwner()->GetLeaderHatPath())
			{
				LeaderHatComboBox->SetSelectedItem(LeaderHatList[i]);
				bFoundSelectedLeaderHat = true;
				break;
			}
		}
		if (!bFoundSelectedLeaderHat && LeaderHatPathList.Num() > 0)
		{
			LeaderHatComboBox->SetSelectedItem(LeaderHatList[0]);
		}

		bool bFoundSelectedEyewear = false;
		for (int32 i = 0; i < EyewearPathList.Num(); i++)
		{
			if (EyewearPathList[i] == GetPlayerOwner()->GetEyewearPath())
			{
				EyewearComboBox->SetSelectedItem(EyewearList[i]);
				bFoundSelectedEyewear = true;
				break;
			}
		}
		if (!bFoundSelectedEyewear && EyewearPathList.Num() > 0)
		{
			EyewearComboBox->SetSelectedItem(EyewearList[0]);
		}
		PopulateEyewearVariants();
		if (SelectedEyewearVariantIndex > 0 && SelectedEyewearVariantIndex < EyewearVariantList.Num())
		{
			EyewearVariantComboBox->SetSelectedItem(EyewearVariantList[SelectedEyewearVariantIndex]);
		}

		bool bFoundSelectedGroupTaunt = false;
		for (int32 i = 0; i < GroupTauntPathList.Num(); i++)
		{
			if (GroupTauntPathList[i] == GetPlayerOwner()->GetGroupTauntPath())
			{
				bSkipPlayingGroupTauntBGMusic = true;
				GroupTauntComboBox->SetSelectedItem(GroupTauntList[i]);
				bSkipPlayingGroupTauntBGMusic = false;
				bFoundSelectedGroupTaunt = true;
				break;
			}
		}
		if (!bFoundSelectedGroupTaunt && TauntPathList.Num() > 0)
		{
			bSkipPlayingGroupTauntBGMusic = true;
			GroupTauntComboBox->SetSelectedItem(GroupTauntList[0]);
			bSkipPlayingGroupTauntBGMusic = false;
		}

		bool bFoundSelectedTaunt = false;
		for (int32 i = 0; i < TauntPathList.Num(); i++)
		{
			if (TauntPathList[i] == GetPlayerOwner()->GetTauntPath())
			{
				TauntComboBox->SetSelectedItem(TauntList[i]);
				bFoundSelectedTaunt = true;
				break;
			}
		}
		if (!bFoundSelectedTaunt && TauntPathList.Num() > 0)
		{
			TauntComboBox->SetSelectedItem(TauntList[0]);
		}

		bool bFoundSelectedTaunt2 = false;
		for (int32 i = 0; i < TauntPathList.Num(); i++)
		{
			if (TauntPathList[i] == GetPlayerOwner()->GetTaunt2Path())
			{
				Taunt2ComboBox->SetSelectedItem(TauntList[i]);
				bFoundSelectedTaunt2 = true;
				break;
			}
		}
		if (!bFoundSelectedTaunt2 && TauntPathList.Num() > 0)
		{
			Taunt2ComboBox->SetSelectedItem(TauntList[0]);
		}

		bool bFoundSelectedCharacter = false;
		for (int32 i = 0; i < CharacterPathList.Num(); i++)
		{
			if (CharacterPathList[i] == GetPlayerOwner()->GetCharacterPath())
			{
				CharacterComboBox->SetSelectedItem(CharacterList[i]);
				bFoundSelectedCharacter = true;
				break;
			}
		}
		if (!bFoundSelectedCharacter && CharacterPathList.Num() > 0)
		{
			CharacterComboBox->SetSelectedItem(CharacterList[0]);
		}

		bool bFoundSelectedIntro = false;
		for (int32 i = 0; i < IntroPathList.Num(); ++i)
		{
			if (IntroPathList[i] == GetPlayerOwner()->GetIntroPath())
			{
				IntroComboBox->SetSelectedItem(IntroList[i]);
				bFoundSelectedIntro = true;
				break;
			}
		}
		if (!bFoundSelectedIntro && IntroPathList.Num() > 0)
		{
			IntroComboBox->SetSelectedItem(IntroList[0]);
		}

		if (SelectedFlag != nullptr)
		{
			OnFlagSelected(SelectedFlag, ESelectInfo::Direct);
		}
	}

	// Turn on Screen Space Reflection max quality
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	OldSSRQuality = SSRQualityCVar->GetInt();
	SSRQualityCVar->Set(4, ECVF_SetByCode);
}

SUTPlayerSettingsDialog::~SUTPlayerSettingsDialog()
{
	// Reset Screen Space Reflection max quality, wish there was a cleaner way to reset the flags
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	EConsoleVariableFlags Flags = SSRQualityCVar->GetFlags();
	Flags = (EConsoleVariableFlags)(((uint32)Flags & ~ECVF_SetByMask) | ECVF_SetByScalability);
	SSRQualityCVar->Set(OldSSRQuality, ECVF_SetByCode);
	SSRQualityCVar->SetFlags(Flags);

	if (!GExitPurge)
	{
		if (PlayerPreviewTexture != NULL)
		{
			PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.Unbind();
			PlayerPreviewTexture = NULL;
		}
		FlushRenderingCommands();
		if (PlayerPreviewBrush != NULL)
		{
			// FIXME: Slate will corrupt memory if this is deleted. Must be referencing it somewhere that doesn't get cleaned up...
			//		for now, we'll take the minor memory leak (the texture still gets GC'ed so it's not too bad)
			//delete PlayerPreviewBrush;
			PlayerPreviewBrush->SetResourceObject(NULL);
			PlayerPreviewBrush = NULL;
		}
		if (PlayerPreviewWorld != NULL)
		{
			PlayerPreviewWorld->DestroyWorld(true);
			GEngine->DestroyWorldContext(PlayerPreviewWorld);
			PlayerPreviewWorld = NULL;
			GetPlayerOwner()->GetWorld()->ForceGarbageCollection(true);
		}
	}
	ViewState.Destroy();
}

void SUTPlayerSettingsDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PlayerPreviewMesh);
	Collector.AddReferencedObject(PreviewWeapon);
	Collector.AddReferencedObject(PreviewEnvironment);
	Collector.AddReferencedObject(PlayerPreviewTexture);
	Collector.AddReferencedObject(PlayerPreviewMID);
	Collector.AddReferencedObject(PlayerPreviewAnimBlueprint);
	Collector.AddReferencedObject(PlayerPreviewWorld);
}

FString SUTPlayerSettingsDialog::GetFOVLabelText(float SliderValue)
{
	int32 FOVAngle = FMath::TruncToInt(SliderValue * (FOV_CONFIG_MAX - FOV_CONFIG_MIN) + FOV_CONFIG_MIN);
	return FText::Format(NSLOCTEXT("SUTPlayerSettingsDialog", "FOV", "Field of View ({0})"), FText::FromString(FString::Printf(TEXT("%i"), FOVAngle))).ToString();
}

void SUTPlayerSettingsDialog::OnFOVChange(float NewValue)
{
	FOVLabel->SetText(GetFOVLabelText(NewValue));
}

void SUTPlayerSettingsDialog::OnNameTextChanged(const FText& NewText)
{
	FString AdjustedText = NewText.ToString();
	FString InvalidNameChars = FString(INVALID_NAME_CHARACTERS) + FString("?#");
	for (int32 i = AdjustedText.Len() - 1; i >= 0; i--)
	{
		if (InvalidNameChars.GetCharArray().Contains(AdjustedText.GetCharArray()[i]))
		{
			AdjustedText.GetCharArray().RemoveAt(i);
		}
	}

	if (AdjustedText.Len() > 8)
	{
		AdjustedText = AdjustedText.Left(8);
	}

	if (AdjustedText != NewText.ToString())
	{
		ClanName->SetText(FText::FromString(AdjustedText));
	}
}

void SUTPlayerSettingsDialog::PlayerColorChanged(FLinearColor NewValue)
{
	SelectedPlayerColor = NewValue;
	RecreatePlayerPreview();
}

FReply SUTPlayerSettingsDialog::OKClick()
{
	UUTProfileSettings* ProfileSettings = GetPlayerOwner()->GetProfileSettings();

	GetPlayerOwner()->SetClanName(ClanName->GetText().ToString());

	GetPlayerOwner()->SetCountryFlagAndAvatar(SelectedFlag.IsValid() ? SelectedFlag->GetFName() : GetPlayerOwner()->GetCountryFlag(), SelectedAvatar);

	// FOV
	float NewFOV = FMath::TruncToFloat(FOV->GetValue() * (FOV_CONFIG_MAX - FOV_CONFIG_MIN) + FOV_CONFIG_MIN);

	if (ProfileSettings)
	{
		ProfileSettings->WeaponBob = WeaponBobScaling->GetValue() * BOB_SCALING_FACTOR;
		ProfileSettings->ViewBob = ViewBobScaling->GetValue() * BOB_SCALING_FACTOR;
		ProfileSettings->FFAPlayerColor = SelectedPlayerColor;
		ProfileSettings->PlayerFOV = NewFOV;
	}

	AUTPlayerController* UTPlayerController = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);
	if (UTPlayerController != NULL)
	{
		UTPlayerController->WeaponBobGlobalScaling = WeaponBobScaling->GetValue() * BOB_SCALING_FACTOR;
		UTPlayerController->EyeOffsetGlobalScaling = ViewBobScaling->GetValue() * BOB_SCALING_FACTOR;
		UTPlayerController->FFAPlayerColor = SelectedPlayerColor;
		UTPlayerController->SaveConfig();

		if (UTPlayerController->GetUTCharacter())
		{
			UTPlayerController->GetUTCharacter()->NotifyTeamChanged();
		}

		UTPlayerController->FOV(NewFOV);
	}
	else
	{
		AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>()->ConfigDefaultFOV = NewFOV;
		AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>()->SaveConfig();
	}

	int32 Index = HatList.Find(HatComboBox->GetSelectedItem());
	GetPlayerOwner()->SetHatPath(HatPathList.IsValidIndex(Index) ? HatPathList[Index] : FString());
	Index = LeaderHatList.Find(LeaderHatComboBox->GetSelectedItem());
	GetPlayerOwner()->SetLeaderHatPath(LeaderHatPathList.IsValidIndex(Index) ? LeaderHatPathList[Index] : FString());
	Index = EyewearList.Find(EyewearComboBox->GetSelectedItem());
	GetPlayerOwner()->SetEyewearPath(EyewearPathList.IsValidIndex(Index) ? EyewearPathList[Index] : FString());
	Index = GroupTauntList.Find(GroupTauntComboBox->GetSelectedItem());
	GetPlayerOwner()->SetGroupTauntPath(GroupTauntPathList.IsValidIndex(Index) ? GroupTauntPathList[Index] : FString());
	Index = TauntList.Find(TauntComboBox->GetSelectedItem());
	GetPlayerOwner()->SetTauntPath(TauntPathList.IsValidIndex(Index) ? TauntPathList[Index] : FString());
	Index = TauntList.Find(Taunt2ComboBox->GetSelectedItem());
	GetPlayerOwner()->SetTaunt2Path(TauntPathList.IsValidIndex(Index) ? TauntPathList[Index] : FString());
	Index = CharacterList.Find(CharacterComboBox->GetSelectedItem());
	GetPlayerOwner()->SetCharacterPath(CharacterPathList.IsValidIndex(Index) ? CharacterPathList[Index] : FString());
	Index = IntroList.Find(IntroComboBox->GetSelectedItem());
	GetPlayerOwner()->SetIntroPath(IntroPathList.IsValidIndex(Index) ? IntroPathList[Index] : FString(TEXT("")));
	Index = HatVariantList.Find(HatVariantComboBox->GetSelectedItem());
	GetPlayerOwner()->SetHatVariant(Index);
	Index = EyewearVariantList.Find(EyewearVariantComboBox->GetSelectedItem());
	GetPlayerOwner()->SetEyewearVariant(Index);

	GetPlayerOwner()->SaveProfileSettings();
	GetPlayerOwner()->CloseDialog(SharedThis(this));

	return FReply::Handled();
}

FReply SUTPlayerSettingsDialog::CancelClick()
{
	GetPlayerOwner()->CloseDialog(SharedThis(this));
	return FReply::Handled();
}

FReply SUTPlayerSettingsDialog::OnButtonClick(uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK) OKClick();
	else if (ButtonID == UTDIALOG_BUTTON_CANCEL) CancelClick();
	return FReply::Handled();
}

FReply SUTPlayerSettingsDialog::WeaponConfigClick()
{
	WeaponConfigDelayFrames = 3;
	GetPlayerOwner()->ShowContentLoadingMessage();
	return FReply::Handled();
}

void SUTPlayerSettingsDialog::OnEmote1Committed(int32 NewValue, ETextCommit::Type CommitInfo)
{
	Emote1Index = NewValue;
}

void SUTPlayerSettingsDialog::OnEmote2Committed(int32 NewValue, ETextCommit::Type CommitInfo)
{
	Emote2Index = NewValue;
}

void SUTPlayerSettingsDialog::OnEmote3Committed(int32 NewValue, ETextCommit::Type CommitInfo)
{
	Emote3Index = NewValue;
}

TOptional<int32> SUTPlayerSettingsDialog::GetEmote1Value() const
{
	return Emote1Index;
}

TOptional<int32> SUTPlayerSettingsDialog::GetEmote2Value() const
{
	return Emote2Index;
}

TOptional<int32> SUTPlayerSettingsDialog::GetEmote3Value() const
{
	return Emote3Index;
}

void SUTPlayerSettingsDialog::OnHatSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		bLeaderHatSelectedLast = false;
		SelectedHat->SetText(*NewSelection.Get());
		PopulateHatVariants();
		RecreatePlayerPreview();
	}
}

void SUTPlayerSettingsDialog::OnLeaderHatSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		if (*NewSelection.Get() == FString(TEXT("No Hat")))
		{
			bLeaderHatSelectedLast = false;
		}
		else
		{
			bLeaderHatSelectedLast = true;
		}

		SelectedLeaderHat->SetText(*NewSelection.Get());
		RecreatePlayerPreview();
	}
}

void SUTPlayerSettingsDialog::OnHatVariantSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		// last minute prevent selecting "locked" entries, since Slate won't let us disable the menu items
		bool bForcedNewSelection = false;
		{
			int32 Index = HatList.Find(HatComboBox->GetSelectedItem());
			if (HatPathList.IsValidIndex(Index))
			{
				if (!GetPlayerOwner()->OwnsItemFor(HatPathList[Index], HatVariantList.Find(NewSelection)))
				{
					for (int32 i = 0; i < HatVariantList.Num(); i++)
					{
						if (GetPlayerOwner()->OwnsItemFor(HatPathList[Index], i))
						{
							HatVariantComboBox->SetSelectedItem(HatVariantList[i]);
							bForcedNewSelection = true;
							break;
						}
					}
				}
			}
		}
		if (!bForcedNewSelection)
		{
			SelectedHatVariant->SetText(*NewSelection.Get());
			RecreatePlayerPreview();
		}
	}
}

void SUTPlayerSettingsDialog::OnEyewearSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedEyewear->SetText(*NewSelection.Get());
		PopulateEyewearVariants();
		RecreatePlayerPreview();
	}
}

void SUTPlayerSettingsDialog::OnEyewearVariantSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		// last minute prevent selecting "locked" entries, since Slate won't let us disable the menu items
		bool bForcedNewSelection = false;
		{
			int32 Index = EyewearList.Find(EyewearComboBox->GetSelectedItem());
			if (EyewearPathList.IsValidIndex(Index))
			{
				if (!GetPlayerOwner()->OwnsItemFor(EyewearPathList[Index], EyewearVariantList.Find(NewSelection)))
				{
					for (int32 i = 0; i < EyewearVariantList.Num(); i++)
					{
						if (GetPlayerOwner()->OwnsItemFor(EyewearPathList[Index], i))
						{
							EyewearVariantComboBox->SetSelectedItem(EyewearVariantList[i]);
							bForcedNewSelection = true;
							break;
						}
					}
				}
			}
		}
		if (!bForcedNewSelection)
		{
			SelectedEyewearVariant->SetText(*NewSelection.Get());
			RecreatePlayerPreview();
		}
	}
}

void SUTPlayerSettingsDialog::OnCharacterSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedCharacter->SetText(*NewSelection.Get());
		RecreatePlayerPreview();
	}
}

void SUTPlayerSettingsDialog::OnGroupTauntSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedGroupTaunt->SetText(*NewSelection.Get());
		if (GroupTauntAudio.IsValid())
		{
			GroupTauntAudio->Stop();
		}

		if (PlayerPreviewMesh != nullptr)
		{
			int32 TauntIndex = GroupTauntList.Find(NewSelection);
			UClass* TauntClass = LoadObject<UClass>(NULL, *GroupTauntPathList[TauntIndex]);
			if (TauntClass)
			{
				PlayerPreviewMesh->PlayGroupTaunt(TSubclassOf<AUTGroupTaunt>(TauntClass));
				AUTGroupTaunt* GroupTaunt = TauntClass->GetDefaultObject<AUTGroupTaunt>();
				if (GroupTaunt != nullptr && !bSkipPlayingGroupTauntBGMusic && GroupTaunt->BGMusic)
				{
					GroupTauntAudio = UGameplayStatics::SpawnSound2D(PlayerPreviewMesh->GetWorld(), GroupTaunt->BGMusic);
				}
			}
		}
	}
}

void SUTPlayerSettingsDialog::OnTauntSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedTaunt->SetText(*NewSelection.Get());

		if (PlayerPreviewMesh != nullptr)
		{
			int32 TauntIndex = TauntList.Find(NewSelection);
			UClass* TauntClass = LoadObject<UClass>(NULL, *TauntPathList[TauntIndex]);
			if (TauntClass)
			{
				PlayerPreviewMesh->PlayTauntByClass(TSubclassOf<AUTTaunt>(TauntClass));
				//PlayerPreviewMesh->GetMesh()->PlayAnimation(TauntClass->GetDefaultObject<AUTTaunt>()->TauntMontage, true);
			}
		}
	}
}

void SUTPlayerSettingsDialog::OnTaunt2Selected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedTaunt2->SetText(*NewSelection.Get());
		if (PlayerPreviewMesh != nullptr)
		{
			int32 TauntIndex = TauntList.Find(NewSelection);
			UClass* TauntClass = LoadObject<UClass>(NULL, *TauntPathList[TauntIndex]);
			if (TauntClass)
			{
				PlayerPreviewMesh->PlayTauntByClass(TSubclassOf<AUTTaunt>(TauntClass));
				//PlayerPreviewMesh->GetMesh()->PlayAnimation(TauntClass->GetDefaultObject<AUTTaunt>()->TauntMontage, true);
			}
		}
	}
}

void SUTPlayerSettingsDialog::OnIntroSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedIntro->SetText(*NewSelection.Get());
	}
}

void SUTPlayerSettingsDialog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SUTDialogBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (PlayerPreviewWorld != nullptr)
	{
		PlayerPreviewWorld->Tick(LEVELTICK_All, InDeltaTime);
	}

	if ( PlayerPreviewTexture != nullptr )
	{
		PlayerPreviewTexture->FastUpdateResource();
	}

	// Force the preview mesh to put the highest mips into memory
	if (PlayerPreviewMesh != nullptr)
	{
		PlayerPreviewMesh->PrestreamTextures(1, true);
		if (PlayerPreviewMesh->Hat)
		{
			PlayerPreviewMesh->Hat->PrestreamTextures(1, true);
		}
	}
	if (PreviewWeapon)
	{
		PreviewWeapon->PrestreamTextures(1, true);
	}

	if (WeaponConfigDelayFrames > 0)
	{
		WeaponConfigDelayFrames--;
		if (WeaponConfigDelayFrames == 0)
		{
			GetPlayerOwner()->HideContentLoadingMessage();
			GetPlayerOwner()->OpenDialog(SNew(SUTWeaponConfigDialog).PlayerOwner(GetPlayerOwner()));
		}
	}

	if ( bSpinPlayer )
	{
		if ( PlayerPreviewWorld != nullptr )
		{
			const float SpinRate = 15.0f * InDeltaTime;
			PlayerPreviewMesh->AddActorWorldRotation(FRotator(0, SpinRate, 0.0f));
		}
	}
}

void SUTPlayerSettingsDialog::RecreatePlayerPreview()
{
	FRotator ActorRotation = FRotator(0.0f, 180.0f, 0.0f);

	if (PlayerPreviewMesh != nullptr)
	{
		ActorRotation = PlayerPreviewMesh->GetActorRotation();
		PlayerPreviewMesh->Destroy();
	}

	if ( PreviewWeapon != nullptr )
	{
		PreviewWeapon->Destroy();
	}

	TSubclassOf<class APawn> DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *GetDefault<AUTGameMode>()->PlayerPawnObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));
	PlayerPreviewMesh = PlayerPreviewWorld->SpawnActor<AUTCharacter>(DefaultPawnClass, FVector(300.0f, 0.f, 4.f), ActorRotation);

	if (PlayerPreviewMesh == nullptr)
	{
		// everything beyond here needs the preview mesh, bail out
		return;
	}

	// set character mesh
	// NOTE: important this is first since it may affect the following items (socket locations, etc)
	int32 Index = CharacterList.Find(CharacterComboBox->GetSelectedItem());
	FString NewCharPath = CharacterPathList.IsValidIndex(Index) ? CharacterPathList[Index] : FString();
	bool bFoundCharacterClass = false;
	TSubclassOf<AUTCharacterContent> CharacterClass;
	if (NewCharPath.Len() > 0)
	{
		CharacterClass = LoadClass<AUTCharacterContent>(NULL, *NewCharPath, NULL, LOAD_None, NULL);
	}
	else
	{
		CharacterClass = GetDefault<AUTCharacter>()->CharacterData;
	}

	if (CharacterClass != NULL)
	{
		PlayerPreviewMesh->ApplyCharacterData(CharacterClass);

		bFoundCharacterClass = true;
		if (CharacterClass.GetDefaultObject()->bIsFemale)
		{
			PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_Female_PlayerPreview.ABP_Female_PlayerPreview_C"));
		}
		else
		{
			PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_PlayerPreview.ABP_PlayerPreview_C"));
		}
	}

	if (!bFoundCharacterClass)
	{
		PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_PlayerPreview.ABP_PlayerPreview_C"));
	}

	if (PlayerPreviewAnimBlueprint)
	{
		PlayerPreviewMesh->GetMesh()->SetAnimInstanceClass(PlayerPreviewAnimBlueprint);
	}

	if (bLeaderHatSelectedLast)
	{
		// set leader hat
		Index = LeaderHatList.Find(LeaderHatComboBox->GetSelectedItem());
		FString NewHatPath = LeaderHatPathList.IsValidIndex(Index) ? LeaderHatPathList[Index] : FString();
		if (NewHatPath.Len() > 0)
		{
			TSubclassOf<AUTHatLeader> HatClass = LoadClass<AUTHatLeader>(NULL, *NewHatPath, NULL, LOAD_None, NULL);
			if (HatClass != NULL)
			{
				PlayerPreviewMesh->SetHatClass(HatClass);
			}
		}

	}
	else
	{
		// set hat
		Index = HatList.Find(HatComboBox->GetSelectedItem());
		FString NewHatPath = HatPathList.IsValidIndex(Index) ? HatPathList[Index] : FString();
		if (NewHatPath.Len() > 0)
		{
			TSubclassOf<AUTHat> HatClass = LoadClass<AUTHat>(NULL, *NewHatPath, NULL, LOAD_None, NULL);
			if (HatClass != NULL)
			{
				PlayerPreviewMesh->HatVariant = HatVariantList.Find(HatVariantComboBox->GetSelectedItem());
				PlayerPreviewMesh->SetHatClass(HatClass);
			}
		}
	}

	// set eyewear
	Index = EyewearList.Find(EyewearComboBox->GetSelectedItem());
	FString NewEyewearPath = EyewearPathList.IsValidIndex(Index) ? EyewearPathList[Index] : FString();
	if (NewEyewearPath.Len() > 0)
	{
		TSubclassOf<AUTEyewear> EyewearClass = LoadClass<AUTEyewear>(NULL, *NewEyewearPath, NULL, LOAD_None, NULL);
		if (EyewearClass != NULL)
		{
			PlayerPreviewMesh->EyewearVariant = EyewearVariantList.Find(EyewearVariantComboBox->GetSelectedItem());
			PlayerPreviewMesh->SetEyewearClass(EyewearClass);
		}
	}
	
	UClass* PreviewAttachmentType = LoadClass<AUTWeaponAttachment>(NULL, TEXT("/Game/RestrictedAssets/Weapons/ShockRifle/ShockAttachment.ShockAttachment_C"), NULL, LOAD_None, NULL);
	if (PreviewAttachmentType != NULL)
	{
		PreviewWeapon = PlayerPreviewWorld->SpawnActor<AUTWeaponAttachment>(PreviewAttachmentType, FVector(0, 0, 0), FRotator(0, 0, 0));
		PreviewWeapon->Instigator = PlayerPreviewMesh;
	}

	// Tick the world to make sure the animation is up to date.
	if ( PlayerPreviewWorld != nullptr )
	{
		PlayerPreviewWorld->Tick(LEVELTICK_All, 0.0);
	}
	
	PlayerPreviewMesh->BeginPlay();

	if ( PreviewWeapon )
	{
		PreviewWeapon->BeginPlay();
		PreviewWeapon->AttachToOwner();
	}
}

void SUTPlayerSettingsDialog::PopulateHatVariants()
{
	HatVariantList.Empty();
	TSharedPtr<FString> DefaultVariant = MakeShareable(new FString(TEXT("Default")));
	HatVariantList.Add(DefaultVariant);
	HatVariantComboBox->SetSelectedItem(DefaultVariant);

	int32 Index = HatList.Find(HatComboBox->GetSelectedItem());
	FString NewHatPath = HatPathList.IsValidIndex(Index) ? HatPathList[Index] : FString();
	if (NewHatPath.Len() > 0)
	{
		TSubclassOf<AUTCosmetic> HatClass = LoadClass<AUTCosmetic>(NULL, *NewHatPath, NULL, LOAD_None, NULL);
		if (HatClass != NULL)
		{
			for (int32 i = 0; i < HatClass->GetDefaultObject<AUTCosmetic>()->VariantNames.Num(); i++)
			{
				TSharedPtr<FString> Variant = MakeShareable(new FString());
				if (!HatClass->GetDefaultObject<AUTCosmetic>()->bRequiresItem || GetPlayerOwner()->OwnsItemFor(NewHatPath, i))
				{
					*Variant.Get() = HatClass->GetDefaultObject<AUTCosmetic>()->VariantNames[i].ToString();
				}
				else
				{
					*Variant.Get() = NSLOCTEXT("SUTPlayerSettingsDialog", "LockedVariant", "--LOCKED--").ToString();
				}
				HatVariantList.Add(Variant);
			}
		}
	}

	if (HatVariantList.Num() == 1)
	{
		HatVariantComboBox->SetVisibility(EVisibility::Hidden);
	}
	else
	{
		HatVariantComboBox->SetVisibility(EVisibility::Visible);
	}

	HatVariantComboBox->RefreshOptions();
}

void SUTPlayerSettingsDialog::PopulateEyewearVariants()
{
	EyewearVariantList.Empty();
	TSharedPtr<FString> DefaultVariant = MakeShareable(new FString(TEXT("Default")));
	EyewearVariantList.Add(DefaultVariant);
	EyewearVariantComboBox->SetSelectedItem(DefaultVariant);

	int32 Index = EyewearList.Find(EyewearComboBox->GetSelectedItem());
	FString NewEyewearPath = EyewearPathList.IsValidIndex(Index) ? EyewearPathList[Index] : FString();
	if (NewEyewearPath.Len() > 0)
	{
		TSubclassOf<AUTCosmetic> EyewearClass = LoadClass<AUTCosmetic>(NULL, *NewEyewearPath, NULL, LOAD_None, NULL);
		if (EyewearClass != NULL)
		{
			for (int32 i = 0; i < EyewearClass->GetDefaultObject<AUTCosmetic>()->VariantNames.Num(); i++)
			{
				TSharedPtr<FString> Variant = MakeShareable(new FString());
				if (!EyewearClass->GetDefaultObject<AUTCosmetic>()->bRequiresItem || GetPlayerOwner()->OwnsItemFor(NewEyewearPath, i))
				{
					*Variant.Get() = EyewearClass->GetDefaultObject<AUTCosmetic>()->VariantNames[i].ToString();
				}
				else
				{
					*Variant.Get() = NSLOCTEXT("SUTPlayerSettingsDialog", "LockedVariant", "--LOCKED--").ToString();
				}
				EyewearVariantList.Add(Variant);
			}
		}
	}

	if (EyewearVariantList.Num() == 1)
	{
		EyewearVariantComboBox->SetVisibility(EVisibility::Hidden);
	}
	else
	{
		EyewearVariantComboBox->SetVisibility(EVisibility::Visible);
	}
}

void SUTPlayerSettingsDialog::UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height)
{
	FEngineShowFlags ShowFlags(ESFIM_Game);
	//ShowFlags.SetLighting(false); // FIXME: create some proxy light and use lit mode
	ShowFlags.SetMotionBlur(false);
	ShowFlags.SetGrain(false);
	//ShowFlags.SetPostProcessing(false);
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(PlayerPreviewTexture->GameThread_GetRenderTargetResource(), PlayerPreviewWorld->Scene, ShowFlags).SetRealtimeUpdate(true));

//	EngineShowFlagOverride(ESFIM_Game, VMI_Lit, ViewFamily.EngineShowFlags, NAME_None, false);
	if (PlayerPreviewWorld)
	{
		CamLocation = FMath::VInterpTo(CamLocation, CameraLocations[CurrentCam], PlayerPreviewWorld->DeltaTimeSeconds, 10.0f);
	}

	const float PreviewFOV = 45;
	const float AspectRatio = Width / (float)Height;

	FSceneViewInitOptions PlayerPreviewInitOptions;
	PlayerPreviewInitOptions.SetViewRectangle(FIntRect(0, 0, C->SizeX, C->SizeY));
	PlayerPreviewInitOptions.ViewOrigin = -CamLocation;
	PlayerPreviewInitOptions.ViewRotationMatrix = FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0), FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));
	PlayerPreviewInitOptions.ProjectionMatrix = 
		FReversedZPerspectiveMatrix(
			FMath::Max(0.001f, PreviewFOV) * (float)PI / 360.0f,
			AspectRatio,
			1.0f,
			GNearClippingPlane );
	PlayerPreviewInitOptions.ViewFamily = &ViewFamily;
	PlayerPreviewInitOptions.SceneViewStateInterface = ViewState.GetReference();
	PlayerPreviewInitOptions.BackgroundColor = FLinearColor::Black;
	PlayerPreviewInitOptions.WorldToMetersScale = GetPlayerOwner()->GetWorld()->GetWorldSettings()->WorldToMeters;
	PlayerPreviewInitOptions.CursorPos = FIntPoint(-1, -1);
	
	ViewFamily.bUseSeparateRenderTarget = true;

	FSceneView* View = new FSceneView(PlayerPreviewInitOptions); // note: renderer gets ownership
	View->ViewLocation = FVector::ZeroVector;
	View->ViewRotation = FRotator::ZeroRotator;
	FPostProcessSettings PPSettings = GetDefault<AUTPlayerCameraManager>()->DefaultPPSettings;



	ViewFamily.Views.Add(View);

	View->StartFinalPostprocessSettings(CamLocation);

	//View->OverridePostProcessSettings(PPSettings, 1.0f);

	View->EndFinalPostprocessSettings(PlayerPreviewInitOptions);
	View->ViewRect = View->UnscaledViewRect;

	// workaround for hacky renderer code that uses GFrameNumber to decide whether to resize render targets
	--GFrameNumber;
	GetRendererModule().BeginRenderingViewFamily(C->Canvas, &ViewFamily);
}

void SUTPlayerSettingsDialog::DragPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (PlayerPreviewMesh != nullptr)
	{
		bSpinPlayer = false;
		PlayerPreviewMesh->SetActorRotation(PlayerPreviewMesh->GetActorRotation() + FRotator(0, 0.2f * -MouseEvent.GetCursorDelta().X, 0.0f));
	}
}

void SUTPlayerSettingsDialog::ZoomPlayerPreview(float WheelDelta)
{
	CurrentCam = FMath::Clamp(CurrentCam + (WheelDelta > 0.0f ? -1 : 1), 0, CameraLocations.Num() - 1);
}

void SUTPlayerSettingsDialog::OnFlagSelected(TWeakObjectPtr<UUTFlagInfo> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedFlag = NewSelection;

	SelectedFlagWidget->ClearChildren();

	if (SelectedFlag.IsValid())
	{
		SelectedFlagWidget->AddSlot()
		[
			GenerateSelectedFlagWidget()
		];
	}
}


TSharedRef<SWidget> SUTPlayerSettingsDialog::GenerateFlagListWidget(TWeakObjectPtr<UUTFlagInfo> InItem)
{
	return 	SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(36.0f)
				.HeightOverride(26.0f)
				.MaxDesiredWidth(36.0f)
				.MaxDesiredHeight(26.0f)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush(InItem->GetSlatePropertyName()))
					.Visibility(EVisibility::HitTestInvisible)
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetFriendlyName()))
				.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
				.Visibility(EVisibility::HitTestInvisible)
			];
}

TSharedRef<SWidget> SUTPlayerSettingsDialog::GenerateSelectedFlagWidget()
{
	return 	SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(36.0f)
				.HeightOverride(26.0f)
				.MaxDesiredWidth(36.0f)
				.MaxDesiredHeight(26.0f)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush(SelectedFlag->GetSlatePropertyName()))
					.Visibility(EVisibility::HitTestInvisible)

				]
			]
			+ SHorizontalBox::Slot()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(SelectedFlag->GetFriendlyName()))
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.Black")
				.Visibility(EVisibility::HitTestInvisible)

			];
}


TSharedPtr<SUTButton> SUTPlayerSettingsDialog::AddAvatar(FName Avatar, int32 Index)
{
	int32 Col = Index % 8;
	int32 Row = Index / 8;

	TSharedPtr<SUTButton> Button;
	AvatarGrid->AddSlot(Col, Row)
	.Padding(5.0,0.0,0.0,5.0)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(64).HeightOverride(64)
				[
					SAssignNew(Button, SUTButton)
					.IsToggleButton(true)
					.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton.Dark")
					.OnClicked(this, &SUTPlayerSettingsDialog::SelectAvatar, Index, Avatar)
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush(Avatar))
					]
				]
			]
		]
	];

	AvatarButtons.Add(Button);
	return Button;
}

FReply SUTPlayerSettingsDialog::SelectAvatar(int32 Index, FName Avatar)
{
	SelectedAvatar = Avatar;
	for (int32 i = 0; i < AvatarButtons.Num(); i++ )
	{
		if (i == Index)
		{
			AvatarButtons[i]->BePressed();
		}
		else
		{
			AvatarButtons[i]->UnPressed();
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

#endif