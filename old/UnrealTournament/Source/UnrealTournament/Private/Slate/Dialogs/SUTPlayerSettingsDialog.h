// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"
#include "UTFlagInfo.h"

#if !UE_SERVER

class SUTButton;

class UNREALTOURNAMENT_API SUTPlayerSettingsDialog : public SUTDialogBase, public FGCObject
{
public:

	SLATE_BEGIN_ARGS(SUTPlayerSettingsDialog)
	: _DialogSize(FVector2D(1000,900))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)												
	SLATE_ARGUMENT(FVector2D, DialogSize)										
	SLATE_ARGUMENT(bool, bDialogSizeIsRelative)									
	SLATE_ARGUMENT(FVector2D, DialogPosition)									
	SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)								
	SLATE_ARGUMENT(FVector2D, ContentPadding)									
	SLATE_EVENT(FDialogResultDelegate, OnDialogResult)							
	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs);
	virtual ~SUTPlayerSettingsDialog();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	static const float BOB_SCALING_FACTOR;

	/** world for rendering the player preview */
	class UWorld* PlayerPreviewWorld;
	/** view state for player preview (needed for LastRenderTime to work) */
	FSceneViewStateReference ViewState;
	/** preview actors */
	class AUTCharacter* PlayerPreviewMesh;
	UClass* PlayerPreviewAnimBlueprint;
	/** preview weapon */
	AUTWeaponAttachment* PreviewWeapon;
	/** render target for player mesh and cosmetic items */
	class UUTCanvasRenderTarget2D* PlayerPreviewTexture;
	/** material for the player preview since Slate doesn't support rendering the target directly */
	class UMaterialInstanceDynamic* PlayerPreviewMID;
	/** Slate brush to render the preview */
	FSlateBrush* PlayerPreviewBrush;
	/** Do you want the player model to spin? */
	bool bSpinPlayer;

	bool bLeaderHatSelectedLast;

	/**Preset cameraloctions when zooming in/out*/
	TArray<FVector> CameraLocations;
	int32 CurrentCam;
	FVector CamLocation;

	/** counter for displaying weapon dialog since we need to display the "Loading Content" message first */
	int32 WeaponConfigDelayFrames;

	int32 OldSSRQuality;

	AActor* PreviewEnvironment;
	
	TWeakObjectPtr<UAudioComponent> GroupTauntAudio;

	TSharedPtr<SEditableTextBox> ClanName;
	TSharedPtr<SSlider> WeaponBobScaling, ViewBobScaling;
	FLinearColor SelectedPlayerColor;

	TArray<TSharedPtr<FString>> HatList;
	TArray<FString> HatPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > HatComboBox;
	TSharedPtr<STextBlock> SelectedHat;
	void OnHatSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> LeaderHatList;
	TArray<FString> LeaderHatPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > LeaderHatComboBox;
	TSharedPtr<STextBlock> SelectedLeaderHat;
	void OnLeaderHatSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TSharedPtr< SComboBox< TSharedPtr<FString> > > HatVariantComboBox;
	TArray<TSharedPtr<FString>> HatVariantList;
	TSharedPtr<STextBlock> SelectedHatVariant;
	int32 SelectedHatVariantIndex;
	void OnHatVariantSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void PopulateHatVariants();

	TArray<TSharedPtr<FString>> EyewearList;
	TArray<FString> EyewearPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > EyewearComboBox;
	TSharedPtr<STextBlock> SelectedEyewear;
	void OnEyewearSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TSharedPtr< SComboBox< TSharedPtr<FString> > > EyewearVariantComboBox;
	TArray<TSharedPtr<FString>> EyewearVariantList;
	TSharedPtr<STextBlock> SelectedEyewearVariant;
	int32 SelectedEyewearVariantIndex;
	void OnEyewearVariantSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void PopulateEyewearVariants();

	bool bSkipPlayingGroupTauntBGMusic;
	TArray<TSharedPtr<FString>> GroupTauntList;
	TArray<FString> GroupTauntPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > GroupTauntComboBox;
	TSharedPtr<STextBlock> SelectedGroupTaunt;
	void OnGroupTauntSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> TauntList;
	TArray<FString> TauntPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > TauntComboBox;
	TSharedPtr<STextBlock> SelectedTaunt;
	void OnTauntSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> Taunt2List;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > Taunt2ComboBox;
	TSharedPtr<STextBlock> SelectedTaunt2;
	void OnTaunt2Selected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> CharacterList;
	TArray<FString> CharacterPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > CharacterComboBox;
	TSharedPtr<STextBlock> SelectedCharacter;
	void OnCharacterSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> IntroList;
	TArray<FString> IntroPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > IntroComboBox;
	TSharedPtr<STextBlock> SelectedIntro;
	void OnIntroSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TWeakObjectPtr<class UUTFlagInfo> > CountryFlags;
	TWeakObjectPtr<class UUTFlagInfo> SelectedFlag;
	TSharedPtr<SOverlay> SelectedFlagWidget;
	TSharedPtr< SComboBox< TWeakObjectPtr<class UUTFlagInfo> > > CountryFlagComboBox;
	void OnFlagSelected(TWeakObjectPtr<class UUTFlagInfo> NewSelection, ESelectInfo::Type SelectInfo);
	virtual TSharedRef<SWidget> GenerateFlagListWidget(TWeakObjectPtr<class UUTFlagInfo> InItem);
	virtual TSharedRef<SWidget> GenerateSelectedFlagWidget();

	int32 Emote1Index;
	int32 Emote2Index;
	int32 Emote3Index;

	void OnEmote1Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	void OnEmote2Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	void OnEmote3Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	TOptional<int32> GetEmote1Value() const;
	TOptional<int32> GetEmote2Value() const;
	TOptional<int32> GetEmote3Value() const;

	virtual FReply OnButtonClick(uint16 ButtonID);

	FReply OKClick();
	FReply CancelClick();
	FReply WeaponConfigClick();
	FLinearColor GetSelectedPlayerColor() const
	{
		return SelectedPlayerColor;
	}
	void PlayerColorChanged(FLinearColor NewValue);
	void OnNameTextChanged(const FText& NewText);

	TSharedPtr<SSlider> FOV;
	TSharedPtr<STextBlock> FOVLabel;

	void OnFOVChange(float NewValue);
	FString GetFOVLabelText(float SliderValue);

	virtual void DragPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual void ZoomPlayerPreview(float WheelDelta);
	virtual void RecreatePlayerPreview();
	virtual void UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	TSharedPtr<SGridPanel> AvatarGrid;
	TArray<TSharedPtr<SUTButton>> AvatarButtons;

	TSharedPtr<SUTButton> AddAvatar(FName AvatarStyleReference, int32 Index);
	FReply SelectAvatar(int32 Index, FName Avatar);
	FName SelectedAvatar;
	TArray<FName> AvatarList;
};
#endif