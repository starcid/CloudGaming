// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 

#include "UTHUDWidget_SpectatorSlideOut.generated.h"

/**
*	Holds the bounds and simulated key of a hud element when clicked
*/
USTRUCT()
struct FClickElement
{
	GENERATED_USTRUCT_BODY()

	//Command when the element is clicked
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoreboard")
	FString Command;

	// Holds the X1/Y1/X2/Y2 bounds of this element.  
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoreboard")
	FVector4 Bounds;

	// optionally holds selected playerstate
	UPROPERTY()
		AUTPlayerState* SelectedPlayer;

	FClickElement()
	{
		Bounds = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	FClickElement(const FString InCommand, const FVector4& InBounds, AUTPlayerState* InPlayer=NULL)
	{
		Command = InCommand;
		Bounds = InBounds;
		SelectedPlayer = InPlayer;
	}
};

UCLASS()
class UNREALTOURNAMENT_API UUTHUDWidget_SpectatorSlideOut : public UUTHUDWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Draw_Implementation(float DeltaTime);
	virtual bool ShouldDraw_Implementation(bool bShowScores);
	virtual void DrawPlayerHeader(float RenderDelta, float XOffset, float YOffset);
	virtual void DrawPlayer(int32 Index, AUTPlayerState* PlayerState, float RenderDelta, float XOffset, float YOffset);
	virtual void DrawFlag(FString FlagCommand, FString FlagName, AUTCarriedObject* Flag, float RenderDelta, float XOffset, float YOffset);
	virtual void DrawCamBind(FString CamCommand, FString ProjName, float RenderDelta, float XOffset, float YOffset, float Width, bool bCamSelected);
	virtual void DrawPowerup(class AUTPickup* Pickup, float XOffset, float YOffset);
	virtual void DrawSelector(FString Command, bool bPointRight, float XOffset, float YOffset);

	// The total Height of a given cell
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
		float CellHeight;

	UPROPERTY()
		UFont* SlideOutFont;

	/** Current slide offset. */
	float SlideIn;

	/** How fast to slide menu in/out */
	UPROPERTY()
		float SlideSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
		float ArrowSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
		float CamTypeButtonStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
		float CamTypeButtonWidth;

	// Where to draw the flags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
		float FlagX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ColumnHeaderPlayerX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ColumnHeaderScoreX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ColumnHeaderArmor;

	// The offset of text data within the cell
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ColumnY;

	/** How long after last action to highlight active players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ActionHighlightTime;

	/** How long after last action to highlight active players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		FText TerminatedNotification;


	UPROPERTY(EditDefaultsOnly, Category = "RenderObject")
		FCanvasIcon UDamageHUDIcon;

	UPROPERTY(EditDefaultsOnly, Category = "RenderObject")
		FCanvasIcon HealthIcon;

	UPROPERTY(EditDefaultsOnly, Category = "RenderObject")
		FCanvasIcon ArmorIcon;

	UPROPERTY(EditDefaultsOnly, Category = "RenderObject")
		FCanvasIcon FlagIcon;

	/** Cached spectator bindings. */
	UPROPERTY()
	FName DemoRestartBind;
	
	UPROPERTY()
	FName DemoLiveBind;

	UPROPERTY()
	FName DemoRewindBind;
	
	UPROPERTY()
	FName DemoFastForwardBind;

	UPROPERTY()
	FName DemoPauseBind;

	UPROPERTY()
	FString CameraString[10];

	UPROPERTY()
	int32 NumCameras;

	UPROPERTY()
		int32 NumCamBinds;

	UPROPERTY()
		int32 DrawnCamBinds;

	UPROPERTY()
		bool bBalanceCamBinds;

	UPROPERTY()
	bool bCamerasInitialized;

	UPROPERTY()
		float MouseOverOpacity;

	UPROPERTY()
		float SelectedOpacity;

	UPROPERTY()
		float CameraBindWidth;

	UPROPERTY()
		float PowerupWidth;

	UPROPERTY()
		AUTPlayerState* SelectedPlayer;

	UPROPERTY()
		bool bShowingStats;

	/**Called from Slate to set the mouse position*/
	virtual void TrackMouseMovement(FVector2D InMousePosition) { MousePosition = InMousePosition; }
	/**Called from Slate when the mouse has clicked*/
	virtual bool MouseClick(FVector2D InMousePosition);

	virtual void SetMouseInteractive(bool bNewInteractive) { bIsInteractive = bNewInteractive; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
	UTexture2D* TextureAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
	UTexture2D* FlagAtlas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideOut")
	UTexture2D* WeaponAtlas;

	UPROPERTY()
	TArray<AUTPickup*> PowerupList;

	UPROPERTY()
	bool bPowerupListInitialized;

	virtual void InitPowerupList();
	virtual float GetDrawScaleOverride();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float KillsColumn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float DeathsColumn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float ShotsColumn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
		float AccuracyColumn;

	/** List of default weapons to display stats for. */
	UPROPERTY()
		TArray<AUTWeapon *> StatsWeapons;

	/** Index of current top weapon (in kills). */
	UPROPERTY()
		int32 BestWeaponIndex;

	virtual void ToggleStats();

	virtual void ShowSelectedPlayerStats(AUTPlayerState* PlayerState, float RenderDelta, float XOffset, float YOffset);

	virtual void DrawWeaponStatsLine(FText StatsName, int32 StatValue, int32 ScoreValue, int32 Shots, float Accuracy, float DeltaTime, float XOffset, float& YPos, const FStatsFontInfo& StatsFontInfo, float ScoreWidth, bool bIsBestWeapon);

	virtual void DrawWeaponStats(AUTPlayerState* PS, float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float MaxHeight, const FStatsFontInfo& StatsFontInfo);

private:
	FVector2D MousePosition;

	/**Whether the mouse can interact with hud elements*/
	bool bIsInteractive;

	/** Holds a list of hud elements that have keybinds associated with them.  NOTE: this is regenerated every frame*/
	TArray<FClickElement> ClickElementStack;

	/**Returns the index of the FClickElement the mouse is under. -1 for none*/
	int32 MouseHitTest(FVector2D Position);

	void UpdateCameraBindOffset(float& DrawOffset, float& XOffset, bool& bOverflow, float StartOffset, float& EndCamOffset);
};
