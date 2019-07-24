// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"

#if !UE_SERVER

struct TAttributeStat
{
	typedef float(*StatValueFunc)(const AUTPlayerState*, const TAttributeStat*);
	typedef FText(*StatValueTextFunc)(const AUTPlayerState*, const TAttributeStat*);

	TAttributeStat(AUTPlayerState* InPlayerState, FName InStatsName, StatValueFunc InValueFunc = nullptr, StatValueTextFunc InTextFunc = nullptr)
		: StatName(InStatsName), PlayerState(InPlayerState), ValueFunc(InValueFunc), TextFunc(InTextFunc)
	{
		checkSlow(PlayerState.IsValid());
	}
	virtual ~TAttributeStat()
	{}

	virtual float GetValue() const
	{
		if (PlayerState.IsValid())
		{
			return (ValueFunc != nullptr) ? ValueFunc(PlayerState.Get(), this) : PlayerState->GetStatsValue(StatName);
		}
		return 0.0f;
	}
	virtual FText GetValueText() const
	{
		if (PlayerState.IsValid())
		{
			return (TextFunc != nullptr) ? TextFunc(PlayerState.Get(), this) : FText::FromString(FString::FromInt((int32)GetValue()));
		}
		return FText();
	}

	FName StatName;
	TWeakObjectPtr<AUTPlayerState> PlayerState;
	StatValueFunc ValueFunc;
	StatValueTextFunc TextFunc;
};

struct TAttributeStatWeapon : public TAttributeStat
{
	enum EWeaponStat
	{
		WS_KillStat,
		WS_DeathStat,
		WS_AccuracyStat,
	};

	TAttributeStatWeapon(AUTPlayerState* InPlayerState, AUTWeapon* InWeapon, EWeaponStat InStatType)
		: TAttributeStat(InPlayerState, NAME_Name, nullptr, nullptr), StatType(InStatType), Weapon(InWeapon)
	{
		checkSlow(PlayerState.IsValid());
	}

	virtual float GetValue() const override
	{
		if (PlayerState.IsValid() && Weapon.IsValid())
		{
			switch (StatType)
			{
			case WS_KillStat:		return Weapon->GetWeaponKillStats(PlayerState.Get());
			case WS_DeathStat:		return Weapon->GetWeaponDeathStats(PlayerState.Get());
			case WS_AccuracyStat:
				int32 Kills = Weapon->GetWeaponKillStats(PlayerState.Get());
				float Shots = Weapon->GetWeaponShotsStats(PlayerState.Get());
				return (Shots > 0) ? 100.f * Weapon->GetWeaponHitsStats(PlayerState.Get()) / Shots : 0.f;
			};
		}
		return 0.0f;
	}

	virtual FText GetValueText() const
	{
		if (PlayerState.IsValid() && StatType == WS_AccuracyStat)
		{
			return FText::FromString(FString::Printf(TEXT("%8.1f%%"), GetValue()));
		}
		return TAttributeStat::GetValueText();
	}

	EWeaponStat StatType;
	TWeakObjectPtr<AUTWeapon> Weapon;
};


class UNREALTOURNAMENT_API SUTPlayerInfoDialog : public SUTDialogBase, public FGCObject
{
public:

	SLATE_BEGIN_ARGS(SUTPlayerInfoDialog)
	: _DialogSize(FVector2D(1920.0f, 1080))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f, 0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _bAllowLogout(false)
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)												
	SLATE_ARGUMENT(FString, TargetUniqueId)
	SLATE_ARGUMENT(FString, TargetName)
	SLATE_ARGUMENT(FVector2D, DialogSize)										
	SLATE_ARGUMENT(bool, bDialogSizeIsRelative)									
	SLATE_ARGUMENT(FVector2D, DialogPosition)									
	SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)								
	SLATE_ARGUMENT(FVector2D, ContentPadding)									
	SLATE_EVENT(FDialogResultDelegate, OnDialogResult)
	SLATE_ARGUMENT(bool, bAllowLogout)
	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs);
	virtual ~SUTPlayerInfoDialog();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TSharedRef<class SWidget> BuildTitleBar(FText InDialogTitle) override;

	virtual void OnUniqueIdChanged();

protected:

	// This is the target id of who is currently being designed
	FString TargetUniqueId;

	// Holds a reference to a live player state if there is one.  This way we can pull
	// proper team information/etc.

	TWeakObjectPtr<AUTPlayerState> TargetPlayerState;

	// These are the various character customization settings that we need.  We either pull them from the player state if it's available,
	// or they are pulled from that player's profile.

	TSubclassOf<class AUTCharacterContent> CharacterClass;
	TSubclassOf<AUTHat> HatClass;
	int32 HatVariant;
	TSubclassOf<AUTEyewear> EyewearClass;
	int32 EyewearVariant;

	/** world for rendering the player preview */
	class UWorld* PlayerPreviewWorld;

	/** view state for player preview (needed for LastRenderTime to work) */
	FSceneViewStateReference ViewState;

	/** preview actors */
	class AUTCharacter* PlayerPreviewMesh;

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

	/** The Zoom offset to apply to the camera. */
	float ZoomOffset;

	AActor* PreviewEnvironment;
	UClass* PlayerPreviewAnimBlueprint;
	UClass* PlayerPreviewAnimFemaleBlueprint;

	int32 OldSSRQuality;

	virtual FReply OnButtonClick(uint16 ButtonID);

	virtual void DragPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual void ZoomPlayerPreview(float WheelDelta);
	virtual void RecreatePlayerPreview();
	virtual void UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height);
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// Friends...

	TSharedPtr<class SOverlay> InfoPanel;
	TSharedPtr<class SHorizontalBox> FriendPanel;
	FName FriendStatus;

	FText GetFunnyText();
	virtual void BuildFriendPanel();
	virtual FReply OnSendFriendRequest();

	//Holds references to all of the stat attributes that are created
	TArray<TSharedPtr<TAttributeStat> > StatList;
	TSharedPtr<class SUTTabWidget> TabWidget;

	virtual void OnTabButtonSelectionChanged(const FText& NewText);

	FReply NextPlayer();
	FReply PreviousPlayer();
	AUTPlayerState* GetNextPlayerState(int32 dir);
	FText CurrentTab; //Store the current tab so we can go back to it when switching players

	void UpdatePlayerCustomization();

	void CreatePlayerTab();

private:
	void UpdatePlayerStateInReplays();
	void UpdatePlayerCharacterPreviewInReplays();
	void UpdatePlayerStateRankingStatsFromLocalPlayer(int32 NewDuelRank, int32 NewCTFRank, int32 NewTDMRank, int32 NewDMRank, int32 NewShowdownRank, int32 NewFlagRunRank, int32 TotalStars, uint8 DuelMatchesPlayed, uint8 CTFMatchesPlayed, uint8 TDMMatchesPlayed, uint8 DMMatchesPlayed, uint8 ShowdownMatchesPlayed, uint8 FlagRunMatchesPlayed);

protected:
	FDelegateHandle OnReadUserFileCompleteDelegate;
	virtual void OnReadUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName);

	TSharedPtr<SVerticalBox> PlayerCardBox;
	TSharedPtr<SUTWebBrowserPanel> PlayerCardWebBrowser;

	UFUNCTION()
	void OnPlayerCardLoadCompleted();

	UFUNCTION()
	void OnPlayerCardLoadError();

	TSharedPtr<SVerticalBox> ModelBox;

};
#endif