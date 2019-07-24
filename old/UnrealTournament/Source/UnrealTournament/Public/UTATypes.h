// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTATypes.generated.h"

// Const defines for Dialogs
const uint16 UTDIALOG_BUTTON_OK = 0x0001;			
const uint16 UTDIALOG_BUTTON_CANCEL = 0x0002;
const uint16 UTDIALOG_BUTTON_YES = 0x0004;
const uint16 UTDIALOG_BUTTON_NO = 0x0008;
const uint16 UTDIALOG_BUTTON_HELP = 0x0010;
const uint16 UTDIALOG_BUTTON_RECONNECT = 0x0020;
const uint16 UTDIALOG_BUTTON_EXIT = 0x0040;
const uint16 UTDIALOG_BUTTON_QUIT = 0x0080;
const uint16 UTDIALOG_BUTTON_VIEW = 0x0100;
const uint16 UTDIALOG_BUTTON_YESCLEAR = 0x0200;
const uint16 UTDIALOG_BUTTON_PLAY = 0x0400;
const uint16 UTDIALOG_BUTTON_LAN = 0x0800;
const uint16 UTDIALOG_BUTTON_CLOSE = 0x1000;
const uint16 UTDIALOG_BUTTON_APPLY = 0x2000;

const uint16 TUTORIAL_Movement = 0x0001;
const uint16 TUTOIRAL_Weapon = 0x0002;
const uint16 TUTORIAL_Pickups = 0x0004;
const uint16 TUTORIAL_SkillMoves = TUTORIAL_Movement | TUTOIRAL_Weapon | TUTORIAL_Pickups;

const uint16 TUTORIAL_DM = 0x0008;
const uint16 TUTORIAL_TDM = 0x0010;
const uint16 TUTORIAL_CTF = 0x0020;
const uint16 TUTORIAL_Duel = 0x0040;
const uint16 TUTORIAL_FlagRun = 0x0080;
const uint16 TUTORIAL_Showdown = 0x0100;
const uint16 TUTORIAL_Gameplay = TUTORIAL_DM | TUTORIAL_FlagRun | TUTORIAL_Showdown;
const uint16 TUTORIAL_Hardcore = TUTORIAL_CTF | TUTORIAL_TDM | TUTORIAL_Duel;

const uint16 TUTORIAL_All = TUTORIAL_SkillMoves | TUTORIAL_Gameplay | TUTORIAL_Hardcore;

const int32 DEFAULT_RANK_CHECK = 0;
const int32 NEW_USER_ELO = 1000;
const int32 NUMBER_RANK_LEVELS = 9;
const int32 STARTER_RANK_LEVEL = 4;
const int32 MAXENTRYROUTES = 5;
const int32 MAX_CHAT_TEXT_SIZE = 384;

const float RALLY_ANIMATION_TIME = 1.2;

const FLinearColor REDHUDCOLOR = FLinearColor(1.0f, 0.05f, 0.0f, 1.0f);
const FLinearColor BLUEHUDCOLOR = FLinearColor(0.1f, 0.1f, 1.0f, 1.0f);
const FLinearColor GOLDCOLOR = FLinearColor(1.f, 0.9f, 0.15f);
const FLinearColor SILVERCOLOR = FLinearColor(0.5f, 0.5f, 0.75f);
const FLinearColor BRONZECOLOR = FLinearColor(0.48f, 0.25f, 0.18f);

const uint16 GAME_OPTION_FLAGS_All = 0xFFFF;
const uint16 GAME_OPTION_FLAGS_RequireFull = 0x0001;
const uint16 GAME_OPTION_FLAGS_AllowBots = 0x0002;
const uint16 GAME_OPTION_FLAGS_BotSkill = 0x0004;

UENUM()
namespace EGameStage
{
	enum Type
	{
		Initializing,
		PreGame, 
		GameInProgress,
		GameOver,
		MAX,
	};
}

UENUM()
namespace ETextHorzPos
{
	enum Type
	{
		Left,
		Center, 
		Right,
		MAX,
	};
}

UENUM()
namespace ETextVertPos
{
	enum Type
	{
		Top,
		Center,
		Bottom,
		MAX,
	};
}

static const FName NAME_FlagCaptures(TEXT("FlagCaptures"));
static const FName NAME_FlagReturns(TEXT("FlagReturns"));
static const FName NAME_FlagAssists(TEXT("FlagAssists"));
static const FName NAME_FlagHeldDeny(TEXT("FlagHeldDeny"));
static const FName NAME_FlagHeldDenyTime(TEXT("FlagHeldDenyTime"));
static const FName NAME_FlagHeldTime(TEXT("FlagHeldTime"));
static const FName NAME_FlagReturnPoints(TEXT("FlagReturnPoints"));
static const FName NAME_CarryAssist(TEXT("CarryAssist"));
static const FName NAME_CarryAssistPoints(TEXT("CarryAssistPoints"));
static const FName NAME_FlagCapPoints(TEXT("FlagCapPoints"));
static const FName NAME_DefendAssist(TEXT("DefendAssist"));
static const FName NAME_DefendAssistPoints(TEXT("DefendAssistPoints"));
static const FName NAME_ReturnAssist(TEXT("ReturnAssist"));
static const FName NAME_ReturnAssistPoints(TEXT("ReturnAssistPoints"));
static const FName NAME_TeamCapPoints(TEXT("TeamCapPoints"));
static const FName NAME_EnemyFCDamage(TEXT("EnemyFCDamage"));
static const FName NAME_FCKills(TEXT("FCKills"));
static const FName NAME_FCKillPoints(TEXT("FCKillPoints"));
static const FName NAME_FlagSupportKills(TEXT("FlagSupportKills"));
static const FName NAME_FlagSupportKillPoints(TEXT("FlagSupportKillPoints"));
static const FName NAME_RegularKillPoints(TEXT("RegularKillPoints"));
static const FName NAME_FlagGrabs(TEXT("FlagGrabs"));
static const FName NAME_TeamFlagGrabs(TEXT("TeamFlagGrabs"));
static const FName NAME_TeamFlagHeldTime(TEXT("TeamFlagHeldTime"));
static const FName NAME_RalliesPowered(TEXT("RalliesPowered"));
static const FName NAME_Rallies(TEXT("Rallies"));
static const FName NAME_FlagDenials(TEXT("FlagDenials"));
static const FName NAME_RedeemerRejected(TEXT("RedeemerRejected"));


const FName NAME_Custom = FName(TEXT("Custom"));
const FName NAME_RedCountryFlag = FName(TEXT("Red.Team"));
const FName NAME_BlueCountryFlag = FName(TEXT("Blue.Team"));
const FName NAME_Epic = FName(TEXT("Epic"));
namespace GameVolumeSpeechType
{
	const FName GV_Bridge = FName(TEXT("GV_Bridge"));
	const FName GV_River = FName(TEXT("GV_River"));
	const FName GV_Antechamber = FName(TEXT("GV_Antechamber"));
	const FName GV_ThroneRoom = FName(TEXT("GV_ThroneRoom"));
	const FName GV_Courtyard = FName(TEXT("GV_Courtyard"));
	const FName GV_Stables = FName(TEXT("GV_Stables"));
	const FName GV_AntechamberHigh = FName(TEXT("GV_AntechamberHigh"));
	const FName GV_Tower = FName(TEXT("GV_Tower"));
	const FName GV_Creek = FName(TEXT("GV_Creek"));
	const FName GV_Temple = FName(TEXT("GV_Temple"));
	const FName GV_Cave = FName(TEXT("GV_Cave"));
	const FName GV_BaseCamp = FName(TEXT("GV_BaseCamp"));
	const FName GV_Sniper = FName(TEXT("GV_Sniper"));
	const FName GV_Arena = FName(TEXT("GV_Arena"));
	const FName GV_Bonsaii = FName(TEXT("GV_Bonsaii"));
	const FName GV_Cliffs = FName(TEXT("GV_Cliffs"));
	const FName GV_Core = FName(TEXT("GV_Core"));
	const FName GV_Crossroads = FName(TEXT("GV_Crossroads"));
	const FName GV_Vents = FName(TEXT("GV_Vents"));
	const FName GV_Pipes = FName(TEXT("GV_Pipes"));
	const FName GV_Ramp = FName(TEXT("GV_Ramp"));
	const FName GV_Hinge = FName(TEXT("GV_Hinge"));
	const FName GV_Tree = FName(TEXT("GV_Tree"));
	const FName GV_Tunnel = FName(TEXT("GV_Tunnel"));
	const FName GV_Falls = FName(TEXT("GV_Falls"));
	const FName GV_Fort = FName(TEXT("GV_Fort"));
	const FName GV_Fountain = FName(TEXT("GV_Fountain"));
	const FName GV_GateHouse = FName(TEXT("GV_GateHouse"));
	const FName GV_Overlook = FName(TEXT("GV_Overlook"));
	const FName GV_Ruins = FName(TEXT("GV_Ruins"));
	const FName GV_SniperTower = FName(TEXT("GV_SniperTower"));
	const FName GV_Flak = FName(TEXT("GV_Flak"));
	const FName GV_Waterfall = FName(TEXT("GV_Waterfall"));
	const FName GV_Shrine = FName(TEXT("GV_Shrine"));
	const FName GV_Stinger = FName(TEXT("GV_Stinger"));
	const FName GV_Checkpoint = FName(TEXT("GV_Checkpoint"));
}

namespace PickupSpeechType
{
	const FName RedeemerPickup = FName(TEXT("RedeemerPickup"));
	const FName UDamagePickup = FName(TEXT("UDamagePickup"));
	const FName ShieldbeltPickup = FName(TEXT("ShieldbeltPickup"));
}

namespace CarriedObjectState
{
	const FName Home = FName(TEXT("Home"));
	const FName Held = FName(TEXT("Held"));
	const FName Dropped = FName(TEXT("Dropped"));
	const FName Delivered = FName(TEXT("Delivered"));
}

namespace RallyPointStates
{
	const FName Off = FName(TEXT("Off"));
	const FName Charging = FName(TEXT("Charging"));
	const FName Powered = FName(TEXT("Powered"));
}

namespace InventoryEventName
{
	const FName Landed = FName(TEXT("Landed"));
	const FName LandedWater = FName(TEXT("LandedWater"));
	const FName FiredWeapon = FName(TEXT("FiredWeapon"));
	const FName Jump = FName(TEXT("Jump"));
	const FName MultiJump = FName(TEXT("MultiJump"));
	const FName Dodge = FName(TEXT("Dodge"));
}

namespace StatusMessage
{
	const FName Taunt = FName(TEXT("Taunt"));
	const FName NeedBackup = FName(TEXT("NeedBackup"));
	const FName EnemyFCHere = FName(TEXT("EnemyFCHere"));
	const FName AreaSecure = FName(TEXT("AreaSecure"));
	const FName IGotFlag = FName(TEXT("IGotFlag"));
	const FName DefendFlag = FName(TEXT("DefendFlag"));
	const FName DefendFC = FName(TEXT("DefendFC"));
	const FName GetFlagBack = FName(TEXT("GetFlagBack"));
	const FName ImGoingIn = FName(TEXT("ImGoingIn"));
	const FName ImOnDefense = FName(TEXT("ImOnDefense"));
	const FName ImOnOffense = FName(TEXT("ImOnOffense"));
	const FName SpreadOut = FName(TEXT("SpreadOut"));
	const FName BaseUnderAttack = FName(TEXT("BaseUnderAttack"));
	const FName Incoming = FName(TEXT("Incoming"));
	const FName Affirmative = FName(TEXT("Affirmative"));
	const FName Negative = FName(TEXT("Negative"));
	const FName EnemyRally = FName(TEXT("EnemyRally"));
	const FName RallyNow = FName(TEXT("RallyNow"));
	const FName FindFC = FName(TEXT("FindFC"));
	const FName LastLife = FName(TEXT("LastLife"));
	const FName EnemyLowLives = FName(TEXT("EnemyLowLives"));
	const FName EnemyThreePlayers = FName(TEXT("EnemyThreePlayers"));
	const FName NeedRally = FName(TEXT("NeedRally"));
	const FName NeedHealth = FName(TEXT("NeedHealth"));
	const FName BehindYou = FName(TEXT("BehindYou"));
	const FName RedeemerKills = FName(TEXT("RedeemerKills"));
	const FName RedeemerSpotted = FName(TEXT("RedeemerSpotted"));
	const FName GetTheFlag = FName(TEXT("GetTheFlag"));
	const FName DoorRally = FName(TEXT("DoorRally"));
	const FName SniperSpotted = FName(TEXT("SniperSpotted"));
	const FName RallyHot = FName(TEXT("RallyHot"));
}

namespace HighlightNames
{
	const FName TopScorer = FName(TEXT("TopScorer"));
	const FName MostKills = FName(TEXT("MostKills"));
	const FName LeastDeaths = FName(TEXT("LeastDeaths"));
	const FName BestKD = FName(TEXT("BestKD"));
	const FName MostWeaponKills = FName(TEXT("MostWeaponKills"));
	const FName BestCombo = FName(TEXT("BestCombo"));
	const FName MostHeadShots = FName(TEXT("MostHeadShots"));
	const FName MostAirRockets = FName(TEXT("MostAirRockets"));

	const FName TopScorerRed = FName(TEXT("TopScorerRed"));
	const FName TopScorerBlue = FName(TEXT("TopScorerBlue"));
	const FName TopFlagCapturesRed = FName(TEXT("TopFlagCapturesRed"));
	const FName TopFlagCapturesBlue = FName(TEXT("TopFlagCapturesBlue"));
	const FName FlagCaptures = FName(TEXT("FlagCaptures"));
	const FName TopAssistsRed = FName(TEXT("TopAssistsRed"));
	const FName TopAssistsBlue = FName(TEXT("TopAssistsBlue"));
	const FName Assists = FName(TEXT("Assists"));
	const FName TopFlagReturnsRed = FName(TEXT("TopFlagReturnsRed"));
	const FName TopFlagReturnsBlue = FName(TEXT("TopFlagReturnsBlue"));
	const FName FlagReturns = FName(TEXT("FlagReturns"));
	const FName ParticipationAward = FName(TEXT("ParticipationAward"));
	const FName KillsAward = FName(TEXT("KillsAward"));
	const FName DamageAward = FName(TEXT("DamageAward"));

	const FName MostKillsTeam = FName(TEXT("MostKillsTeam"));
	const FName RedeemerRejection = FName(TEXT("RedeemerRejection"));
	const FName FlagDenials = FName(TEXT("FlagDenials"));
	const FName WeaponKills = FName(TEXT("WeaponKills"));
	const FName KillingBlowsAward = FName(TEXT("KillingBlowsAward"));
	const FName MostKillingBlowsAward = FName(TEXT("MostKillingBlowsAward"));
	const FName CoupDeGrace = FName(TEXT("Coup de Grace"));
	const FName FlagCap = FName(TEXT("FlagCap"));

	const FName BadMF = FName(TEXT("BadMF"));
	const FName BadAss = FName(TEXT("BadAss"));
	const FName LikeABoss = FName(TEXT("LikeABoss"));
	const FName DeathIncarnate = FName(TEXT("DeathIncarnate"));
	const FName NaturalBornKiller = FName(TEXT("NaturalBornKiller"));
	const FName SpecialForces = FName(TEXT("SpecialForces"));
	const FName HiredGun = FName(TEXT("HiredGun"));
	const FName CoolBeans = FName(TEXT("CoolBeans"));
	const FName BobLife = FName(TEXT("BobLife"));
	const FName HappyToBeHere = FName(TEXT("HappyToBeHere"));
	const FName HardToKill = FName(TEXT("HardToKill"));
	const FName Rallies = FName(TEXT("Rallies"));
	const FName RallyPointPowered = FName(TEXT("RallyPointPowered"));
	const FName HatTrick = FName(TEXT("HatTrick"));
	const FName NotSureIfSerious = FName(TEXT("NotSureIfSerious"));
	const FName ComeAtMeBro = FName(TEXT("ComeAtMeBro"));
	const FName ThisIsSparta = FName(TEXT("ThisIsSparta"));
	const FName AllOutOfBubbleGum = FName(TEXT("AllOutOfBubbleGum"));
	const FName GameOver = FName(TEXT("GameOver"));
	const FName LikeTheWind = FName(TEXT("LikeTheWind"));
	const FName DeliveryBoy = FName(TEXT("DeliveryBoy"));
	const FName MoreThanAHandful = FName(TEXT("MoreThanAHandful"));
	const FName ToughGuy = FName(TEXT("ToughGuy"));
	const FName LargerThanLife = FName(TEXT("LargerThanLife"));
	const FName AssKicker = FName(TEXT("AssKicker"));
	const FName Destroyer = FName(TEXT("Destroyer"));
	const FName LockedAndLoaded = FName(TEXT("LockedAndLoaded"));
}

namespace ArmorTypeName
{
	const FName ShieldBelt = FName(TEXT("ShieldBelt"));
	const FName ThighPads = FName(TEXT("ThighPads"));
	const FName FlakVest = FName(TEXT("FlakVest"));
	const FName SmallArmor = FName(TEXT("SmallArmor"));
}

namespace ChatDestinations
{
	// You can chat with your friends from anywhere
	const FName Friends = FName(TEXT("CHAT_Friends"));			// The chat should go to anyone on the server

	// These are lobby chat types
	const FName Global = FName(TEXT("CHAT_Global"));			// The chat should route to everyone on the server
	const FName Match = FName(TEXT("CHAT_Match"));				// The chat should route to everyone currently in my match lobby

	// these are general game chating
	const FName Lobby = FName(TEXT("CHAT_Lobby"));				// The chat came in from a hub lobby and needs to go directly to a player
	const FName Local = FName(TEXT("CHAT_Local"));				// The chat is local to everyone on that server
	const FName Team = FName(TEXT("CHAT_Team"));				// The chat is for anyone with the same team num
	const FName Whisper = FName(TEXT("CHAT_Whisper"));			// The chat is only for the person specified

	const FName System = FName(TEXT("CHAT_System"));			// This chat message is a system message
	const FName MOTD = FName(TEXT("CHAT_MOTD"));				// This chat message is a message of the day

	const FName Instance = FName(TEXT("CHAT_Instance"));		// This is chat message from a player in an instance to everyone in the lobby

}

// Our Dialog results delegate.  It passes in a reference to the dialog triggering it, as well as the button id 
DECLARE_DELEGATE_TwoParams(FDialogResultDelegate, TSharedPtr<SCompoundWidget>, uint16);

USTRUCT()
struct FTextureUVs
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TextureUVs")
	float U;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TextureUVs")
	float V;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TextureUVs")
	float UL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TextureUVs")
	float VL;

	FTextureUVs()
		: U(0.0f)
		, V(0.0f)
		, UL(0.0f)
		, VL(0.0f)
	{};

	FTextureUVs(float inU, float inV, float inUL, float inVL)
	{
		U = inU; V = inV; UL = inUL;  VL = inVL;
	}

};

USTRUCT(BlueprintType)
struct FAnnouncementInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite, Category = Announcement)
		TSubclassOf<class UUTLocalMessage> MessageClass;
	UPROPERTY(BlueprintReadWrite, Category = Announcement)
		int32 Switch;
	UPROPERTY(BlueprintReadWrite, Category = Announcement)
		const class APlayerState* RelatedPlayerState_1;
	UPROPERTY(BlueprintReadWrite, Category = Announcement)
		const class APlayerState* RelatedPlayerState_2;
	UPROPERTY(BlueprintReadWrite, Category = Announcement)
		const UObject* OptionalObject;
	UPROPERTY(BlueprintReadWrite, Category = Announcement)
		float QueueTime;

	FAnnouncementInfo()
		: MessageClass(NULL), Switch(0), RelatedPlayerState_1(NULL), RelatedPlayerState_2(NULL), OptionalObject(NULL), QueueTime(0.f)
	{}
	FAnnouncementInfo(TSubclassOf<UUTLocalMessage> InMessageClass, int32 InSwitch, const class APlayerState* InRelatedPlayerState_1, const class APlayerState* InRelatedPlayerState_2, const UObject* InOptionalObject, float InQueueTime)
		: MessageClass(InMessageClass), Switch(InSwitch), RelatedPlayerState_1(InRelatedPlayerState_1), RelatedPlayerState_2(InRelatedPlayerState_2), OptionalObject(InOptionalObject), QueueTime(InQueueTime)
	{}
};

USTRUCT(BlueprintType)
struct UNREALTOURNAMENT_API FLocalizedMessageData
{
	GENERATED_USTRUCT_BODY()

		// These members are static and set only upon construction

		// A cached reference to the class of this message.
		UPROPERTY(BlueprintReadOnly, Category = HUD)
		TSubclassOf<class UUTLocalMessage> MessageClass;

	// The index.
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		int32 MessageIndex;

	// The text of this message.  We build this once so we don't have to process the string each frame
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		FText Text;

	// which section of text should have emphasis effects
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		FText EmphasisText;

	// which section of text should have emphasis effects
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		FText PrefixText;

	// which section of text should have emphasis effects
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		FText PostfixText;

	// How much time does this message have left
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float LifeLeft;

	// How long total this message has in its life
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float LifeSpan;

	// How long to scale in this message
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float ScaleInTime;

	// Starting scale of message
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float ScaleInSize;

	// The related playerstates from the localized message
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		APlayerState* RelatedPlayerState_1;
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		APlayerState* RelatedPlayerState_2;

	// The optional object for this class.  
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		UObject* OptionalObject;

	// DrawColor will get set to the base color upon creation.  You can manually apply any 
	// palette/alpha shifts safely during render.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
		FLinearColor DrawColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
		FLinearColor EmphasisColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
		UFont* DisplayFont;

	// These members are setup at first render.
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float TextWidth;

	UPROPERTY(BlueprintReadOnly, Category = HUD)
		float TextHeight;

	UPROPERTY(BlueprintReadOnly, Category = HUD)
		bool bHasBeenRendered;

	// Count is tracked differently.  It's incremented when the same message arrives
	UPROPERTY(BlueprintReadOnly, Category = HUD)
		int32 MessageCount;

	UPROPERTY(BlueprintReadOnly, Category = HUD)
		int32 RequestedSlot;

	UPROPERTY()
		FVector2D ShadowDirection;

	UPROPERTY()
		TWeakObjectPtr<class UUTUMGHudWidget> UMGWidget;

	UPROPERTY()
		FAnnouncementInfo AnnouncementInfo;

	bool ShouldDraw_Implementation(bool bShowScores)
	{
		return bShowScores;
	}

	FLocalizedMessageData()
		: MessageClass(NULL)
		, MessageIndex(0)
		, LifeLeft(0)
		, LifeSpan(0)
		, RelatedPlayerState_1(nullptr)
		, RelatedPlayerState_2(nullptr)
		, OptionalObject(NULL)
		, DrawColor(ForceInit)
		, DisplayFont(NULL)
		, TextWidth(0)
		, TextHeight(0)
		, bHasBeenRendered(false)
		, MessageCount(0)
	{
		UMGWidget.Reset();
	}
};

USTRUCT(BlueprintType)
struct FHUDRenderObject
{
	GENERATED_USTRUCT_BODY()

	// Set to true to make this renderobject hidden
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bHidden;

	// The depth priority.  Higher means rendered later.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	float RenderPriority;

	// Where (in unscaled pixels) should this HUDObject be displayed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FVector2D Position;

	// How big (in unscaled pixels) is this HUDObject.  NOTE: the HUD object will be scaled to fit the size.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FVector2D Size;

	// The Text Color to display this in.  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FLinearColor RenderColor;

	// An override for the opacity of this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	float RenderOpacity;

	// Additional Scaler to apply when rendering
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	float RenderScale;

	FHUDRenderObject()
	{
		RenderScale = 1.0f;
		RenderPriority = 0.0f;
		RenderColor = FLinearColor::White;
		RenderOpacity = 1.0f;
	};

	virtual ~FHUDRenderObject()
	{
	}

public:
	virtual float GetWidth() { return Size.X * RenderScale; }
	virtual float GetHeight() { return Size.Y * RenderScale; }
};


USTRUCT(BlueprintType)
struct FHUDRenderObject_Texture : public FHUDRenderObject
{
	GENERATED_USTRUCT_BODY()

	// The texture to draw
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	UTexture* Atlas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FTextureUVs UVs;

	// If true, this texture object will pickup the team color of the owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bUseTeamColors;

	// The team colors to select from.  If this array is empty, the base HUD team colors will be used.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TArray<FLinearColor> TeamColorOverrides;

	// If true, this is a background element and should take the HUDWidgetBorderOpacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bIsBorderElement;

	// If true, this is a background element and should take the HUDWidgetBorderOpacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bIsSlateElement;


	// The offset to be applied to the position.  They are normalized to the width and height of the image being draw.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FVector2D RenderOffset;

	// The rotation angle to render with
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	float Rotation;

	// The point at which within the image that the rotation will be around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FVector2D RotPivot;

	FHUDRenderObject_Texture() : FHUDRenderObject()
	{
		Atlas = NULL;
		bUseTeamColors = false;
		bIsBorderElement = false;
		Rotation = 0.0f;
	}

	virtual ~FHUDRenderObject_Texture()
	{
	}

public:
	virtual float GetWidth()
	{
		return ((Size.X <= 0) ? UVs.UL : Size.X) * RenderScale;
	}

	virtual float GetHeight()
	{
		return ((Size.Y <= 0) ? UVs.VL : Size.Y) * RenderScale;
	}

	virtual void QuickSet(UTexture* inAtlas, FTextureUVs inTextureUVs, FVector2D inRenderOffset = FVector2D(0.5f, 0.5f), FVector2D inRotation = FVector2D(0.0f, 0.0f), float inRenderOpacity = 1.0f, FLinearColor inRenderColor = FLinearColor::White)
	{
		Atlas = inAtlas;
		UVs = inTextureUVs;
		RenderOffset = inRenderOffset;
		RenderOpacity = inRenderOpacity;
		RenderColor = inRenderColor;
		RotPivot = FVector2D(0.5f, 0.5f);
	}

};

// This is a simple delegate that returns an FTEXT value for rendering things in HUD render widgets
DECLARE_DELEGATE_RetVal(FText, FUTGetTextDelegate)

USTRUCT(BlueprintType)
struct FHUDRenderObject_Text : public FHUDRenderObject
{
	GENERATED_USTRUCT_BODY()

	// If this delegate is set, then Text is ignore and this function is called each frame.
	FUTGetTextDelegate GetTextDelegate;

	// The text to be displayed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FText Text;

	// The font to render with
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	UFont* Font;

	// Additional scaling applied to the font.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	float TextScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bDrawShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FVector2D ShadowDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FLinearColor ShadowColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	bool bDrawOutline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FLinearColor OutlineColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TEnumAsByte<ETextHorzPos::Type> HorzPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TEnumAsByte<ETextVertPos::Type> VertPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FLinearColor BackgroundColor;

	FHUDRenderObject_Text() : FHUDRenderObject()
	{
		Font = NULL;
		TextScale = 1.0f;
		bDrawShadow = false;
		ShadowColor = FLinearColor::White;
		bDrawOutline = false;
		OutlineColor = FLinearColor::Black;
		HorzPosition = ETextHorzPos::Left;
		VertPosition = ETextVertPos::Top;
		BackgroundColor = FLinearColor(0.0f,0.0f,0.0f,0.0f);
	}

	virtual ~FHUDRenderObject_Text()
	{
	}

public:
	FVector2D GetSize()
	{
		if (Font)
		{
			FText TextToRender = Text;
			if (GetTextDelegate.IsBound())
			{
				TextToRender = GetTextDelegate.Execute();
			}

			int32 Width = 0;
			int32 Height = 0;
			Font->GetStringHeightAndWidth(TextToRender.ToString(), Height, Width);
			return FVector2D(Width * TextScale * RenderScale , Height * TextScale * RenderScale);
		}
	
		return FVector2D(0,0);
	}
};

DECLARE_DELEGATE(FGameOptionChangedDelegate);

// These are attribute tags that can be used to look up data in the MatchAttributesDatastore
namespace EMatchAttributeTags
{
	const FName GameMode = FName(TEXT("GameMode"));
	const FName GameName = FName(TEXT("GameName"));
	const FName Map = FName(TEXT("Map"));
	const FName Options = FName(TEXT("Options"));
	const FName Stats = FName(TEXT("Stats"));
	const FName Host = FName(TEXT("Host"));
	const FName PlayTime = FName(TEXT("PlayTime"));
	const FName RedScore = FName(TEXT("RedScore"));
	const FName BlueScore = FName(TEXT("BlueScore"));
	const FName PlayerCount = FName(TEXT("PlayerCount"));
}

namespace ELobbyMatchState
{
	const FName Dead = TEXT("Dead");
	const FName Initializing = TEXT("Initializing");
	const FName Setup = TEXT("Setup");
	const FName WaitingForPlayers = TEXT("WaitingForPlayers");
	const FName Launching = TEXT("Launching");
	const FName Aborting = TEXT("Aborting");
	const FName InProgress = TEXT("InProgress");
	const FName Completed = TEXT("Completed");
	const FName Recycling = TEXT("Recycling");
	const FName Returning = TEXT("Returning");
}

class FSimpleListData
{
public: 
	FString DisplayText;
	FLinearColor DisplayColor;

	FSimpleListData(FString inDisplayText, FLinearColor inDisplayColor)
		: DisplayText(inDisplayText)
		, DisplayColor(inDisplayColor)
	{
	};

	static TSharedRef<FSimpleListData> Make( FString inDisplayText, FLinearColor inDisplayColor)
	{
		return MakeShareable( new FSimpleListData( inDisplayText, inDisplayColor ) );
	}
};

const FString HUBSessionIdKey = "HUBSessionId";

namespace FFriendsStatus
{
	const FName IsBot = FName(TEXT("IsBot"));
	const FName IsYou = FName(TEXT("IsYou"));
	const FName NotAFriend = FName(TEXT("NotAFriend"));
	const FName FriendRequestPending = FName(TEXT("FriendRequestPending"));
	const FName Friend = FName(TEXT("Friend"));
}


UENUM()
namespace ERedirectStatus
{
	enum Type
	{
		Pending,
		InProgress,
		Completed, 
		Failed,
		Cancelled,
		MAX,
	};
}

/*
	Describes a package that might be needed
*/
USTRUCT()
struct FPackageRedirectReference
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Ruleset)
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = Ruleset)
	FString PackageURLProtocol;

	UPROPERTY(BlueprintReadOnly, Category = Ruleset)
	FString PackageURL;

	UPROPERTY(BlueprintReadOnly, Category = Ruleset)
	FString PackageChecksum;

	FPackageRedirectReference()
		: PackageName(TEXT("")), PackageURLProtocol(TEXT("")), PackageURL(TEXT("")), PackageChecksum(TEXT(""))
	{}

	FPackageRedirectReference(FString inPackageName, FString inPackageURLProtocol, FString inPackageURL, FString inPackageChecksum)
		: PackageName(inPackageName), PackageURLProtocol(inPackageURLProtocol), PackageURL(inPackageURL), PackageChecksum(inPackageChecksum)
	{}

	FPackageRedirectReference(FPackageRedirectReference* OtherReference)
	{
		PackageName = OtherReference->PackageName;
		PackageURLProtocol = OtherReference->PackageURLProtocol;
		PackageURL = OtherReference->PackageURL;
		PackageChecksum = OtherReference->PackageChecksum;
	}

	// Converts the redirect to a download URL
	FString ToString() const
	{
		return PackageURLProtocol + TEXT("://") + PackageURL + TEXT(" ");
	}

	bool operator==(const FPackageRedirectReference& Other) const
	{
		return PackageName == Other.PackageName && PackageURLProtocol == Other.PackageURLProtocol && PackageURL == Other.PackageURL && PackageChecksum == Other.PackageChecksum;
	}
};


/**
 *	Holds information about a map that can be set via config.  This will be used to build the FMapListInfo objects in various places but contains
 *  a cut down copy of the content to make life easier to manage.
 **/
USTRUCT()
struct FConfigMapInfo
{
	GENERATED_USTRUCT_BODY()

	// NOTE: this can be the long or short name for the map and will be validated when the maps are loaded
	UPROPERTY(Config)
	FString MapName;						

	// The Redirect for this map.
	UPROPERTY(Config)
	FPackageRedirectReference Redirect;		

	FConfigMapInfo()
	{
		MapName = TEXT("");
		Redirect.PackageName = TEXT("");
		Redirect.PackageURL = TEXT("");
		Redirect.PackageURLProtocol = TEXT("");
		Redirect.PackageChecksum = TEXT("");
	}

	FConfigMapInfo(const FString& inMapName)
	{
		MapName = inMapName;
		Redirect.PackageName = TEXT("");
		Redirect.PackageURL = TEXT("");
		Redirect.PackageURLProtocol = TEXT("");
		Redirect.PackageChecksum = TEXT("");
	}

	FConfigMapInfo(const FConfigMapInfo& ExistingRuleMapInfo)
	{
		MapName = ExistingRuleMapInfo.MapName;
		Redirect = ExistingRuleMapInfo.Redirect;
	}

	FConfigMapInfo(const FString& inMapName, const FString& inPackageName, const FString& inPackageURL, const FString& inPackageChecksum)
	{
		MapName = inMapName;
		Redirect.PackageName = inPackageName;
		Redirect.PackageURL = inPackageURL;
		Redirect.PackageURLProtocol = TEXT("http");
		Redirect.PackageChecksum = inPackageChecksum;
	}
};



UENUM()
namespace EGameDataType
{
	enum Type
	{
		GameMode,
		Map,
		Mutator, 
		MAX,
	};
}

USTRUCT()
struct FAllowedData
{
	GENERATED_USTRUCT_BODY()

	// What type of data is this.
	UPROPERTY()
	TEnumAsByte<EGameDataType::Type> DataType;

	// The package name of this content
	UPROPERTY()
	FString PackageName;

	FAllowedData()
		: DataType(EGameDataType::GameMode)
		, PackageName(TEXT(""))
	{}

	FAllowedData(EGameDataType::Type inDataType, const FString& inPackageName)
		: DataType(inDataType)
		, PackageName(inPackageName)
	{}

};

UENUM()
namespace EUnrealRoles
{
	enum Type
	{
		Gamer,
		Developer,
		Concepter,
		Contributor, 
		Marketplace,
		Prototyper,
		Ambassador,
		MAX,
	};
}

USTRUCT()
struct FFlagInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	int32 Id;

	FFlagInfo()
		: Title(FString())
		, Id(0)
	{
	}

	FFlagInfo(const FString inTitle, int32 inId)
		: Title(inTitle)
		, Id(inId)
	{
	}

	static TSharedRef<FFlagInfo> Make(const FFlagInfo& OtherFlag)
	{
		return MakeShareable( new FFlagInfo( OtherFlag.Title, OtherFlag.Id) );
	}

	static TSharedRef<FFlagInfo> Make(const FString inTitle, int32 inId)
	{
		return MakeShareable( new FFlagInfo( inTitle, inId) );
	}

};

static FName NAME_MapInfo_Title(TEXT("Title"));
static FName NAME_MapInfo_Author(TEXT("Author"));
static FName NAME_MapInfo_Description(TEXT("Description"));
static FName NAME_MapInfo_OptimalPlayerCount(TEXT("OptimalPlayerCount"));
static FName NAME_MapInfo_OptimalTeamPlayerCount(TEXT("OptimalTeamPlayerCount"));
static FName NAME_MapInfo_ScreenshotReference(TEXT("ScreenshotReference"));

// Called upon completion of a redirect transfer.  
DECLARE_MULTICAST_DELEGATE_ThreeParams(FContentDownloadComplete, class UUTGameViewportClient*, ERedirectStatus::Type, const FString&);



USTRUCT()
struct FMatchPlayerListStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString PlayerName;

	UPROPERTY()
	FString PlayerId;

	UPROPERTY()
	FString PlayerScore;

	UPROPERTY()
	uint32 TeamNum;

	FMatchPlayerListStruct()
		: PlayerName(TEXT(""))
		, PlayerId(TEXT(""))
		, PlayerScore(TEXT(""))
		, TeamNum(255)
	{
	}

	FMatchPlayerListStruct(const FString& inPlayerName, const FString& inPlayerId, const FString& inPlayerScore, uint32 inTeamNum)
		: PlayerName(inPlayerName)
		, PlayerId(inPlayerId)
		, PlayerScore(inPlayerScore)
		, TeamNum(inTeamNum)
	{
	}
};


struct FMatchPlayerListCompare
{
	FORCEINLINE bool operator()( const FMatchPlayerListStruct A, const FMatchPlayerListStruct B ) const 
	{
		return A.PlayerName < B.PlayerName;
	}
};


USTRUCT()
struct FMatchUpdate
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float TimeLimit;

	UPROPERTY()
	int32 GoalScore;

	// The current game time of this last update
	UPROPERTY()
	float GameTime;

	// # of players in this match
	UPROPERTY()
	int32 NumPlayers;

	// # of spectators in this match
	UPROPERTY()
	int32 NumSpectators;

	// Team Scores.. non-team games will be 0 entries
	UPROPERTY()
	TArray<int32> TeamScores;

	// The current state of the match
	UPROPERTY()
	FName MatchState;

	UPROPERTY()
	bool bMatchHasBegun;

	UPROPERTY()
	bool bMatchHasEnded;

	UPROPERTY()
	int32 RankCheck;

	FMatchUpdate()
	{
		GameTime = 0.0f;
		NumPlayers = 0;
		NumSpectators = 0;
		TeamScores.Empty();
	}

};

USTRUCT()
struct FServerInstanceData 
{
	GENERATED_USTRUCT_BODY()

	// The Unique GUID that describes this instance.  It's sent to the MCP in the SETTING_SERVERINSTANCEGUID.
	UPROPERTY()
	FGuid InstanceId;

	// The human readable name for the ruleset in use.  
	UPROPERTY()
	FString RulesTitle;

	// The actual rules tag.  We can't use the rules title because of custom rules and this is typically used
	// during the quickmatch portion.
	UPROPERTY()
	FString RulesTag;

	// The actual game mode that this match is running.
	UPROPERTY()
	FString GameModeClass;

	// The name of the map being played.  It will be the friendly name if possible
	UPROPERTY()
	FString MapName;
	
	// Max # of players desired for this instance
	UPROPERTY()
	int32 MaxPlayers;

	// A Collection of flags that describe this instance.  See UTOnlineGameSettingsBase.h for a list
	UPROPERTY()
	uint32 Flags;

	// The rank check value for this match
	UPROPERTY()
	int32 RankCheck;
	
	// Will be true if this is a team game
	UPROPERTY()
	bool bTeamGame;

	// Will be true if this instance is joinable as a player.  Equates to UTLobbyMatchInfo.bJoinAnytime
	UPROPERTY()
	bool bJoinableAsPlayer;

	// Will be true if this instance is joinable as a spectator.  Equates to UTLobbyMatchInfo.bSpectatable
	UPROPERTY()
	bool bJoinableAsSpectator;

	// Holds a list of mutators running on this instance.
	UPROPERTY()
	FString MutatorList;

	// Holds the most up to date match data regarding this instance
	UPROPERTY()
	FMatchUpdate MatchData;

	// Holds the list of players in this instance
	UPROPERTY(NotReplicated)
	TArray<FMatchPlayerListStruct> Players;

	UPROPERTY()
	FString CustomGameName;

	FServerInstanceData()
		: RulesTitle(TEXT(""))
		, RulesTag(TEXT(""))
		, GameModeClass(TEXT(""))
		, MapName(TEXT(""))
		, MaxPlayers(0)
		, Flags(0x00)
		, RankCheck(DEFAULT_RANK_CHECK)
		, bTeamGame(false)
		, bJoinableAsPlayer(false)
		, bJoinableAsSpectator(false)
		, MutatorList(TEXT(""))
		, CustomGameName(TEXT(""))
	{
		MatchData = FMatchUpdate();
	}

	FServerInstanceData(FGuid inInstanceId, const FString& inRulesTitle, const FString& inRulesTag, const FString&  inGameModeClass, const FString& inMapName, int32 inMaxPlayers, uint32 inFlags, int32 inRankCheck, bool inbTeamGame, bool inbJoinableAsPlayer, bool inbJoinableAsSpectator, const FString& inMutatorList, const FString& inCustomGameName)
		: InstanceId(inInstanceId)
		, RulesTitle(inRulesTitle)
		, RulesTag(inRulesTag)
		, GameModeClass(inGameModeClass)
		, MapName(inMapName)
		, MaxPlayers(inMaxPlayers)
		, Flags(inFlags)
		, RankCheck(inRankCheck)
		, bTeamGame(inbTeamGame)
		, bJoinableAsPlayer(inbJoinableAsPlayer)
		, bJoinableAsSpectator(inbJoinableAsSpectator)
		, MutatorList(inMutatorList)
		, CustomGameName(inCustomGameName)
	{
		MatchData = FMatchUpdate();
	}

	int32 NumPlayers()		{ return MatchData.NumPlayers; }
	int32 NumSpectators()	{ return MatchData.NumSpectators; }

	void SetNumPlayers(int32 NewNumPlayers)			{ MatchData.NumPlayers = NewNumPlayers; }
	void SetNumSpectators(int32 NewNumSpectators)	{ MatchData.NumSpectators = NewNumSpectators; }

	int32 GetNumPlayersOnTeam(int32 TeamNum)
	{
		int32 Count = 0;
		for (int i=0; i < Players.Num(); i++)
		{
			if (Players[i].TeamNum == TeamNum)
			{
				Count++;
			}
		}
		return Count;
	}

	static TSharedRef<FServerInstanceData> Make(FGuid inInstanceId, const FString& inRulesTitle, const FString& inRulesTag, const FString& inGameModeClass, const FString& inMapName, int32 inMaxPlayers, uint32 inFlags, int32 inRankCheck, bool inbTeamGame, bool inbJoinableAsPlayer, bool inbJoinableAsSpectator, const FString& inMutatorList, const FString& inCustomGameName)
	{
		return MakeShareable(new FServerInstanceData(inInstanceId, inRulesTitle, inRulesTag, inGameModeClass, inMapName, inMaxPlayers, inFlags, inRankCheck, inbTeamGame, inbJoinableAsPlayer, inbJoinableAsSpectator, inMutatorList, inCustomGameName));
	}
	static TSharedRef<FServerInstanceData> Make(const FServerInstanceData& Other)
	{
		return MakeShareable(new FServerInstanceData(Other));
	}

};

namespace EQuickMatchResults
{
	const FName JoinTimeout = FName(TEXT("JoinTimeout"));
	const FName CantJoin = FName(TEXT("CantJoin"));
	const FName WaitingForStart = FName(TEXT("WaitingForStart"));
	const FName WaitingForStartNew = FName(TEXT("WaitingForStartNew"));
	const FName Join = FName(TEXT("Join"));
}

/**
 *	NOTE: For each Epic Default Ruletags, there has to be an associated rule setup in UTEpicDefaultRulesets
 **/
namespace EEpicDefaultRuleTags
{
	const FString Deathmatch = TEXT("DEATHMATCH");
	const FString TDM = TEXT("TDM");
	const FString DUEL = TEXT("DUEL");
	const FString SHOWDOWN = TEXT("SHOWDOWN");
	const FString TEAMSHOWDOWN = TEXT("TEAMSHOWDOWN");
	const FString CTF = TEXT("CTF");
	const FString BIGCTF = TEXT("BIGCTF");
	const FString COMPCTF = TEXT("CompCTF");
	const FString iDM = TEXT("iDM");
	const FString iTDM = TEXT("iTDM");
	const FString iCTF = TEXT("iCTF");
	const FString iCTFT = TEXT("iCTF+T");
	const FString FlagRun = TEXT("FlagRun");
	const FString FlagRunVSAI = TEXT("FlagRunVSAI");
	const FString Siege = TEXT("Siege");

	// Quick play Rules

	const FString QuickPlay_Deathmatch = TEXT("QuickPlay_DEATHMATCH");
	const FString QuickPlay_FlagRun = TEXT("QuickPlay_FlagRun");
	const FString QuickPlay_FlagRunVSAINormal = TEXT("QuickPlay_FlagRunVSAINormal");
	const FString QuickPlay_FlagRunVSAIHard = TEXT("QuickPlay_FlagRunVSAIHard");

	// Ranked Rules
	const FString Ranked_DUEL = TEXT("Ranked_DUEL");
	const FString Ranked_TEAMSHOWDOWN = TEXT("Ranked_TEAMSHOWDOWN");
	const FString Ranked_CTF = TEXT("Ranked_CTF");
	const FString Ranked_FlagRun = TEXT("Ranked_FlagRun");
}

namespace EPlayerListContentCommand
{
	const FName PlayerCard = FName(TEXT("PlayerCard"));
	const FName ServerKick = FName(TEXT("ServerKick"));
	const FName ServerBan = FName(TEXT("ServerBan"));
	const FName SendMessage = FName(TEXT("SendMessage"));
}

UENUM()
namespace EInstanceJoinResult
{
	enum Type
	{
		MatchNoLongerExists,
		MatchLocked,
		MatchRankFail,
		JoinViaLobby,
		JoinDirectly,
		MAX,
	};
}

UENUM()
namespace EChallengeFilterType
{
	enum Type
	{
		Active,
		Completed,
		Expired,
		All,
		DailyUnlocked,
		DailyLocked,
		MAX,
	};
}


USTRUCT()
struct FUTDailyChallengeUnlock
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName Tag;

	UPROPERTY()
	FDateTime UnlockTime;

	FUTDailyChallengeUnlock()
		: Tag(NAME_None)
	{
	}

	FUTDailyChallengeUnlock(FName inTag)
		: Tag(inTag)
		, UnlockTime(FDateTime::Now())
	{
	}
};


USTRUCT()
struct FUTChallengeResult
{
	GENERATED_USTRUCT_BODY()

	// The Challenge tag this result is for
	UPROPERTY()
	FName Tag;

	// The number of stars received for this challenge
	UPROPERTY()
	int32 Stars;

	// when was this challenge completed
	UPROPERTY()
	FDateTime LastUpdate;

	FUTChallengeResult()
		: Tag(NAME_None)
		, Stars(0)
		, LastUpdate(FDateTime::UtcNow())
	{}

	FUTChallengeResult(FName inTag, int32 inStars)
		: Tag(inTag)
		, Stars(inStars)
		, LastUpdate(FDateTime::UtcNow())
	{
	}

	void Update(int32 NewStars)
	{
		Stars = NewStars;
		LastUpdate = FDateTime::Now();
	}
};

USTRUCT()
struct FTeamRoster
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FText TeamName;

	UPROPERTY()
	TArray<FName> Roster;

	FTeamRoster()
		: TeamName(FText::GetEmpty())
		, Roster()
	{
	}

	FTeamRoster(FText inTeamName)
		: TeamName(inTeamName)
	{
	}
};

USTRUCT()
struct FUTRewardInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FLinearColor StarColor;

	UPROPERTY()
	FName StarEmptyStyleTag;

	UPROPERTY()
	FName StarCompletedStyleTag;

	FUTRewardInfo()
		: StarColor(FLinearColor::White)
		, StarEmptyStyleTag(FName(TEXT("UT.Star.Outline")))
		, StarCompletedStyleTag(FName(TEXT("UT.Star")))
	{
	}

	FUTRewardInfo(FLinearColor inColor, FName inEmptyStyle, FName inCompletedStyle)
		: StarColor(inColor)
		, StarEmptyStyleTag(inEmptyStyle)
		, StarCompletedStyleTag(inCompletedStyle)
	{
	}

};

USTRUCT()
struct FUTChallengeInfo
{
	GENERATED_USTRUCT_BODY()

	// NOTE: The tag will be fixed up at load time and shouldn't be included in the MCP data as it will be overwritten
	UPROPERTY()
	FName Tag;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Map;

	UPROPERTY()
	FString GameURL;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	int32 PlayerTeamSize;

	UPROPERTY()
	int32 EnemyTeamSize;

	UPROPERTY()
	FName EnemyTeamName[3];

	UPROPERTY()
	FName SlateUIImageName;

	UPROPERTY()
	FName RewardTag;

	UPROPERTY()
	bool bDailyChallenge;

	UPROPERTY()
	bool bExpiredChallenge;

	UPROPERTY()
	bool bHighlighted;

	UPROPERTY()
	bool ShadowOpacity;


	FUTChallengeInfo()
		: Title(TEXT(""))
		, Map(TEXT(""))
		, GameURL(TEXT(""))
		, Description(TEXT(""))
		, PlayerTeamSize(0)
		, EnemyTeamSize(0)
		, EnemyTeamName()
		, SlateUIImageName(NAME_None)
		, RewardTag(NAME_None)
	{
		EnemyTeamName[0] = NAME_None;
		EnemyTeamName[1] = NAME_None;
		EnemyTeamName[2] = NAME_None;
		bExpiredChallenge = false;
		bDailyChallenge = false;
		bHighlighted = false;
		ShadowOpacity = 1.0f;
	}

	FUTChallengeInfo(FName inTag, const FUTChallengeInfo& inInfo)
		: Tag(inTag)
		, Title(inInfo.Title)
		, Map(inInfo.Map)
		, GameURL(inInfo.GameURL)
		, Description(inInfo.Description)
		, PlayerTeamSize(inInfo.PlayerTeamSize)
		, EnemyTeamSize(inInfo.EnemyTeamSize)
		, SlateUIImageName(inInfo.SlateUIImageName)
		, RewardTag(inInfo.RewardTag)
		, bDailyChallenge(inInfo.bDailyChallenge)
		, bExpiredChallenge(inInfo.bExpiredChallenge)
		, bHighlighted(inInfo.bHighlighted)
		, ShadowOpacity(inInfo.ShadowOpacity)
	{
		EnemyTeamName[0] = inInfo.EnemyTeamName[0];
		EnemyTeamName[1] = inInfo.EnemyTeamName[1];
		EnemyTeamName[2] = inInfo.EnemyTeamName[2];
	}

	FUTChallengeInfo(FName inTag, FString inTitle, FString inMap, FString inGameURL, FString inDescription, int32 inPlayerTeamSize, int32 inEnemyTeamSize, FName EasyEnemyTeam, FName MediumEnemyTeam, FName HardEnemyTeam, FName inSlateUIImageName, FName inRewardTag)
		: Tag(inTag)
		, Title(inTitle)
		, Map(inMap)
		, GameURL(inGameURL)
		, Description(inDescription)
		, PlayerTeamSize(inPlayerTeamSize)
		, EnemyTeamSize(inEnemyTeamSize)
		, SlateUIImageName(inSlateUIImageName)
		, RewardTag(inRewardTag)
	{
		EnemyTeamName[0] = EasyEnemyTeam;
		EnemyTeamName[1] = MediumEnemyTeam;
		EnemyTeamName[2] = HardEnemyTeam;
		bExpiredChallenge = false;
		bDailyChallenge = false;
		bHighlighted = false;
		ShadowOpacity = 1.0f;
	}
};

USTRUCT()
struct FStoredUTChallengeInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName ChallengeName;

	UPROPERTY()
	FUTChallengeInfo Challenge;

	FStoredUTChallengeInfo()
	{
		ChallengeName = NAME_None;
	}

	FStoredUTChallengeInfo(FName inChallengeName, FUTChallengeInfo inChallenge)
		: ChallengeName(inChallengeName)
		, Challenge(inChallenge)
	{
	}

};

USTRUCT()
struct FMCPPulledData
{
	GENERATED_USTRUCT_BODY()

	bool bValid;

	// Holds the current "version" so to speak.  Just increment it each time we push
	// a new update.  
	UPROPERTY()
	int32 ChallengeRevisionNumber;

	// Holds a list of reward categories
	UPROPERTY()
	TArray<FName> RewardTags;

	// Holds a list of challenges 
	UPROPERTY()
	TArray<FStoredUTChallengeInfo> Challenges;

	UPROPERTY()
	int32 FragCenterCounter;

	UPROPERTY()
	int32 CurrentVersionNumber;

	UPROPERTY()
	FString BuildNotesURL;

	FMCPPulledData()
	{
		Challenges.Empty();
		BuildNotesURL = TEXT("");
		FragCenterCounter=0;
		CurrentVersionNumber=0;
	}
};

USTRUCT()
struct FUTOnlineSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<int32> ActiveRankedPlaylists;
	
	UPROPERTY()
	int32 RankedEloRange;

	UPROPERTY()
	int32 RankedMinEloRangeBeforeHosting;

	UPROPERTY()
	int32 RankedMinEloSearchStep;

	UPROPERTY()
	int32 QMEloRange;

	UPROPERTY()
	int32 QMMinEloRangeBeforeHosting;

	UPROPERTY()
	int32 QMMinEloSearchStep;

	FUTOnlineSettings()
		: RankedEloRange(20), RankedMinEloRangeBeforeHosting(100), RankedMinEloSearchStep(20), QMEloRange(20), QMMinEloRangeBeforeHosting(100), QMMinEloSearchStep(20)
	{}
};

USTRUCT(BlueprintType)
struct FBloodDecalInfo
{
	GENERATED_USTRUCT_BODY()

		/** material to use for the decal */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
		UMaterialInterface* Material;
	/** Base scale of decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
		FVector2D BaseScale;
	/** range of random scaling applied (always uniform) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
		FVector2D ScaleMultRange;

	FBloodDecalInfo()
		: Material(NULL), BaseScale(32.0f, 32.0f), ScaleMultRange(0.8f, 1.2f)
	{}
};

USTRUCT()
struct FRconPlayerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString PlayerName;

	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	FString PlayerIP;

	UPROPERTY()
	int32 ReportedRank;

	UPROPERTY()
	bool bInInstance;

	UPROPERTY()
	FString InstanceGuid;

	UPROPERTY()
	bool bSpectator;

	bool bPendingDelete;

	FRconPlayerData()
		: PlayerName(TEXT(""))
		, PlayerID(TEXT(""))
		, PlayerIP(TEXT(""))
		, ReportedRank(0)
		, bInInstance(false)
	{
		bPendingDelete = false;
	}

	FRconPlayerData(FString inPlayerName, FString inPlayerID, FString inPlayerIP, int32 inRank, bool inbSpectator)
		: PlayerName(inPlayerName)
		, PlayerID(inPlayerID)
		, PlayerIP(inPlayerIP)
		, ReportedRank(inRank)
		, bInInstance(false)
		, bSpectator(inbSpectator)
	{
		bPendingDelete = false;
	}

	FRconPlayerData(FString inPlayerName, FString inPlayerID, FString inPlayerIP, int32 inRank, FString inInstanceGuid, bool inbSpectator)
		: PlayerName(inPlayerName)
		, PlayerID(inPlayerID)
		, PlayerIP(inPlayerIP)
		, ReportedRank(inRank)
		, InstanceGuid(inInstanceGuid)
		, bSpectator(inbSpectator)
	{
		bInInstance = InstanceGuid != TEXT("");
		bPendingDelete = false;
	}

	static TSharedRef<FRconPlayerData> Make(const FRconPlayerData& Original)
	{
		return MakeShareable( new FRconPlayerData(Original.PlayerName, Original.PlayerID, Original.PlayerIP, Original.ReportedRank, Original.InstanceGuid, Original.bSpectator));
	}

};


UENUM()
namespace EUIWindowState
{
	enum Type
	{
		Initializing,
		Opening,
		Active,
		Closing,
		Closed,
		MAX,
	};
}

namespace MatchSummaryViewState
{
	const FName ViewingTeam = FName(TEXT("Team"));
	const FName ViewingSingle = FName(TEXT("Single"));
}

namespace AchievementIDs
{
	const FName TutorialComplete(TEXT("TutorialComplete"));
	const FName ChallengeStars5(TEXT("ChallengeStars5"));
	const FName ChallengeStars15(TEXT("ChallengeStars15"));
	const FName ChallengeStars25(TEXT("ChallengeStars25"));
	const FName ChallengeStars35(TEXT("ChallengeStars35"));
	const FName ChallengeStars45(TEXT("ChallengeStars45"));
	const FName PumpkinHead2015Level1(TEXT("PumpkinHead2015Level1"));
	const FName PumpkinHead2015Level2(TEXT("PumpkinHead2015Level2"));
	const FName PumpkinHead2015Level3(TEXT("PumpkinHead2015Level3"));
	const FName ChallengePumpkins5(TEXT("ChallengePumpkins5"));
	const FName ChallengePumpkins10(TEXT("ChallengePumpkins10"));
	const FName ChallengePumpkins15(TEXT("ChallengePumpkins15"));
	const FName FacePumpkins(TEXT("FacePumpkins"));
};

namespace EQuickStatsLayouts
{
	const FName Arc = FName(TEXT("Arc"));
	const FName Bunch = FName(TEXT("Bunch"));
}

/** controls location and orientation of first person weapon */
UENUM()
enum class EWeaponHand : uint8
{
	HAND_Right,
	HAND_Left,
	HAND_Center,
	HAND_Hidden,
};

namespace CommandTags
{
	const FName Intent = FName(TEXT("Intent"));
	const FName Defend = FName(TEXT("Defend"));
	const FName Distress = FName(TEXT("Distress"));
	const FName Attack = FName(TEXT("Attack"));
	const FName DropFlag = FName(TEXT("DropFlag"));
	const FName Yes = FName(TEXT("Yes"));
	const FName No = FName(TEXT("No"));
}

USTRUCT()
struct FComMenuCommand
{
	GENERATED_USTRUCT_BODY()

public:
	FText MenuText;
	FName CommandTag;

	FComMenuCommand()
		: MenuText(FText::GetEmpty())
		, CommandTag(NAME_None)
	{
	}

	FComMenuCommand(FText inMenuText, FName inCommandTag)
		: MenuText(inMenuText)
		, CommandTag(inCommandTag)
	{
	}
};

USTRUCT()
struct FComMenuCommandList
{
	GENERATED_USTRUCT_BODY()

public:
	FComMenuCommand Intent;
	FComMenuCommand Attack;
	FComMenuCommand Defend;
	FComMenuCommand Distress;
	FComMenuCommand DropFlag;
};

USTRUCT()
struct FScoreboardContextMenuItem
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ContextItem")
	FText MenuText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ContextItem")
	int32 Id;

	FScoreboardContextMenuItem()
		: MenuText(FText::GetEmpty())
		, Id(0)
	{
	}

	FScoreboardContextMenuItem(FText inMenuText, uint8 inId)
		: MenuText(inMenuText)
		, Id(inId)
	{
	}

};


namespace DefaultWeaponCrosshairs
{
const FName Dot = FName(TEXT("CrossDot"));
const FName Bracket1 = FName(TEXT("CrossBracket1"));
const FName Bracket2 = FName(TEXT("CrossBracket2"));
const FName Bracket3 = FName(TEXT("CrossBracket3"));
const FName Bracket4 = FName(TEXT("CrossBracket4"));
const FName Bracket5 = FName(TEXT("CrossBracket5"));
const FName Circle1 = FName(TEXT("CrossCircle1"));
const FName Circle2 = FName(TEXT("CrossCircle2"));
const FName Circle3 = FName(TEXT("CrossCircle3"));
const FName Circle4 = FName(TEXT("CrossCircle4"));
const FName Circle5 = FName(TEXT("CrossCircle5"));
const FName Cross1 = FName(TEXT("CrossCross1"));
const FName Cross2 = FName(TEXT("CrossCross2"));
const FName Cross3 = FName(TEXT("CrossCross3"));
const FName Cross4 = FName(TEXT("CrossCross4"));
const FName Cross5 = FName(TEXT("CrossCross5"));
const FName Cross6 = FName(TEXT("CrossCross6"));
const FName Pointer = FName(TEXT("CrossPointer"));
const FName Triad1 = FName(TEXT("CrossTriad1"));
const FName Triad2 = FName(TEXT("CrossTriad2"));
const FName Triad3 = FName(TEXT("CrossTriad3"));
const FName Sniper = FName(TEXT("Sniper"));
};

namespace EpicWeaponSkinCustomizationTags
{
	const FName BioRifle = FName(TEXT("BioRifle_Skins"));
	const FName ImpactHammer = FName(TEXT("ImpactHammer_Skins"));
	const FName Enforcer = FName(TEXT("Enforcer_Skins"));
	const FName LinkGun = FName(TEXT("LinkGun_Skins"));
	const FName Minigun = FName(TEXT("Minigun_Skins"));
	const FName Translocator = FName(TEXT("Translocator_Skins"));
	const FName FlakCannon = FName(TEXT("FlakCannon_Skins"));
	const FName Redeemer = FName(TEXT("Redeemer_Skins"));
	const FName RocketLauncher = FName(TEXT("RocketLauncher_Skins"));
	const FName ShockRifle = FName(TEXT("ShockRifle_Skins"));
	const FName IGShockRifle = FName(TEXT("IGShockRifle_Skins"));
	const FName Sniper = FName(TEXT("Sniper_Skins"));
};


namespace EpicWeaponCustomizationTags
{
	const FName BioRifle		= FName(TEXT("BioRifle_Settings"));
	const FName ImpactHammer	= FName(TEXT("ImpactHammer_Settings"));
	const FName Enforcer		= FName(TEXT("Enforcer_Settings"));
	const FName LinkGun			= FName(TEXT("LinkGun_Settings"));
	const FName Minigun			= FName(TEXT("Minigun_Settings"));
	const FName Translocator	= FName(TEXT("Translocator_Settings"));
	const FName FlakCannon		= FName(TEXT("FlakCannon_Settings"));
	const FName Redeemer		= FName(TEXT("Redeemer_Settings"));
	const FName RocketLauncher	= FName(TEXT("RocketLauncher_Settings"));
	const FName ShockRifle		= FName(TEXT("ShockRifle_Settings"));
	const FName IGShockRifle	= FName(TEXT("IGShockRifle_Settings"));
	const FName Sniper			= FName(TEXT("Sniper_Settings"));
	const FName GrenadeLauncher = FName(TEXT("GrenadeLauncher_Settings"));
	const FName LightningRifle	= FName(TEXT("LightningRifle_Settings"));
};

USTRUCT(BlueprintType)
struct FWeaponCustomizationInfo
{
	GENERATED_USTRUCT_BODY()

	// This is the main lookup tag for this customization info.  Multiple weapons can share the same
	// tag so that children, mods and experimental weapons can inheirt the same config data.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponInfo)
	FName WeaponCustomizationTag;

	// This is the weapon group for all weapons of this config type.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponInfo)
	int32 WeaponGroup;

	// This is the auto-switch priority for weapons of this config type
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponInfo)
	float WeaponAutoSwitchPriority;

	// This tag is used to look up the UTCrosshair for this config.  
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CrosshairInfo)
	FName DefaultCrosshairTag;

	// This tag is used to look up the UTCrosshair for this config when using custom crosshairs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CrosshairInfo)
	FName CrosshairTag;

	// The scale for the crosshair
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CrosshairInfo)
	float CrosshairScaleOverride;

	// The color override for the crosshair
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CrosshairInfo)
	FLinearColor CrosshairColorOverride;

	FWeaponCustomizationInfo()
		: WeaponCustomizationTag(NAME_None)
		, WeaponGroup(-1)
		, WeaponAutoSwitchPriority(-1.0f)
		, DefaultCrosshairTag(NAME_None)
		, CrosshairTag(NAME_None)
		, CrosshairScaleOverride(1.0)
		, CrosshairColorOverride(FLinearColor::White)
	{
	}

	FWeaponCustomizationInfo(FName inWeaponCustomizationTag, int32 inWeaponGroup, float inWeaponAutoSwitchPriority, 
				FName inCrosshairTag, FLinearColor inCrosshairColorOverride, float inCrosshairScaleOverride)
		: WeaponCustomizationTag(inWeaponCustomizationTag)
		, WeaponGroup(inWeaponGroup)
		, WeaponAutoSwitchPriority(inWeaponAutoSwitchPriority)
		, DefaultCrosshairTag(inCrosshairTag)
		, CrosshairTag(inCrosshairTag)
		, CrosshairScaleOverride(inCrosshairScaleOverride)
		, CrosshairColorOverride(inCrosshairColorOverride)
	{
	}

	FWeaponCustomizationInfo(const FWeaponCustomizationInfo& Source)
	{
		Copy(Source);
	}

	void Copy(const FWeaponCustomizationInfo& Source)
	{
		WeaponCustomizationTag = Source.WeaponCustomizationTag;
		WeaponGroup = Source.WeaponGroup;
		WeaponAutoSwitchPriority = Source.WeaponAutoSwitchPriority;
		DefaultCrosshairTag = Source.DefaultCrosshairTag;
		CrosshairTag = Source.CrosshairTag;
		CrosshairColorOverride = Source.CrosshairColorOverride;
		CrosshairScaleOverride = Source.CrosshairScaleOverride;
	}

};

struct FUTMath
{
	static float LerpOvershoot(float Start, float End, float Alpha, float BounceAmount, float Decay)
	{
		float AV = BounceAmount * PI * 2;
		return End + ((End - Start) / 0.1f) * (FMath::Sin((Alpha - 0.1) * AV) / FMath::Exp(Decay * Alpha) / AV);
	}

	static void ReturnToZero(float& Value, float Speed)
	{
		Value = (Value < 0) ? FMath::Clamp<float>(Value + Speed, Value, 0) : FMath::Clamp<float>(Value - Speed, 0, Value);
	}

	static void ReturnVectorToZero(FVector& Value, float Speed)
	{
		FUTMath::ReturnToZero(Value.X, Speed);
		FUTMath::ReturnToZero(Value.Y, Speed);
		FUTMath::ReturnToZero(Value.Z, Speed);
	}
};

USTRUCT()
struct FCustomKeyBinding
{
	GENERATED_USTRUCT_BODY()

	FCustomKeyBinding() : KeyName(FName(TEXT(""))), EventType(IE_Pressed), Command(FString("")) {};

	FCustomKeyBinding(FName InKeyName, TEnumAsByte<EInputEvent> InEventType, FString InCommand) : KeyName(InKeyName), EventType(InEventType), Command(InCommand) {};

	UPROPERTY()
	FName KeyName;
	UPROPERTY()
	TEnumAsByte<EInputEvent> EventType;
	UPROPERTY()
	FString Command;
	UPROPERTY()
	FString FriendlyName;
};

UENUM()
namespace EControlCategory
{
	enum Type
	{
		Movement,
		Combat,
		Weapon,
		Taunts,
		UI,
		Misc,
		MAX,
	};
}

/**
 *	Holds the configuration info for a given key.,
 **/
USTRUCT(BlueprintType)
struct FKeyConfigurationInfo
{
	GENERATED_USTRUCT_BODY()

public:

	// This is the unique tag that describes this action.  
	UPROPERTY(BlueprintReadOnly,Category = INPUT)
	FName GameActionTag;

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	TEnumAsByte<EControlCategory::Type> Category;

	// This is the name that is displayed in the UI via the Controls menu
	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	FText MenuText;

	// This is the primary key that triggers this action
	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	FKey PrimaryKey;

	// This is the secondy key that can trigger this action
	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	FKey SecondaryKey;

	// This is the key for a game pad that can this action.  For now these are hard coded
	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	FKey GamepadKey;

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	uint32 bShowInUI : 1;

	// If true, then this keybind is optional
	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	uint32 bOptional : 1;

	// These values are the meat of the key bind system.  Any GameAction can be used to build multiple Action/Axis/Custom mappings.  
	// The game will clear and rebuild the key table each time it's loaded.  INI's are no longer used.

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	TArray<FInputActionKeyMapping> ActionMappings;

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	TArray<FInputAxisKeyMapping> AxisMappings;

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	TArray<FCustomKeyBinding>  CustomBindings;

	UPROPERTY(BlueprintReadOnly, Category = INPUT)
	TArray<FCustomKeyBinding> SpectatorBindings;

public:

	FKeyConfigurationInfo()
	{
		bShowInUI = true;
	}

	FKeyConfigurationInfo(const FName& inGameActionTag, EControlCategory::Type inCategory, FKey inDefaultPrimaryKey, FKey inDefaultSecondayKey, FKey inGamepadKey, const FText& inMenuText, bool inbOptional)
		: GameActionTag(inGameActionTag)
		, Category(inCategory)
		, MenuText(inMenuText)
		, PrimaryKey(inDefaultPrimaryKey)
		, SecondaryKey(inDefaultSecondayKey)
		, GamepadKey(inGamepadKey)
		, bOptional(inbOptional)
	{
		bShowInUI = true;
	}

	void AddActionMapping(const FName& inActionName)
	{
		ActionMappings.Add(FInputActionKeyMapping(inActionName, EKeys::Invalid, false, false, false, false));
	}

	void AddAxisMapping(const FName& inAxisName, float Scale)
	{
		AxisMappings.Add(FInputAxisKeyMapping(inAxisName, EKeys::Invalid, Scale));
	}

	void AddCustomBinding(const FString& inCommand, TEnumAsByte<EInputEvent> inEvent = EInputEvent::IE_Pressed)
	{
		CustomBindings.Add(FCustomKeyBinding(NAME_None, inEvent, inCommand));
	}

	void AddSpectatorBinding(const FString& inCommand, TEnumAsByte<EInputEvent> inEvent = EInputEvent::IE_Pressed)
	{
		SpectatorBindings.Add(FCustomKeyBinding(NAME_None, inEvent, inCommand));
	}

};

USTRUCT()
struct FKeyConfigurationImportExportObject
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName GameActionTag;

	UPROPERTY()
	FKey PrimaryKey;

	UPROPERTY()
	FKey SecondaryKey;

	UPROPERTY()
	FKey GamepadKey;

	FKeyConfigurationImportExportObject()
		: GameActionTag(NAME_None), PrimaryKey(EKeys::Invalid), SecondaryKey(EKeys::Invalid), GamepadKey(EKeys::Invalid)
	{
	}

	FKeyConfigurationImportExportObject(FName inGameActionTag, FKey inPrimaryKey, FKey inSecondaryKey, FKey inGamepadKey)
		: GameActionTag(inGameActionTag)
		, PrimaryKey(inPrimaryKey)
		, SecondaryKey(inSecondaryKey)
		, GamepadKey(inGamepadKey)
	{}
};

USTRUCT()
struct FKeyConfigurationImportExport
{
	GENERATED_USTRUCT_BODY()
	
public:

	UPROPERTY()
	TArray<FKeyConfigurationImportExportObject> GameActions;

	FKeyConfigurationImportExport()
	{
		GameActions.Empty();
	}
};

UENUM()
namespace ELoginPhase
{
	enum Type
	{
		NotLoggedIn,
		Offline,
		InDialog,
		Auth,
		GettingProfile,
		GettingProgression,		
		GettingMMR,
		GettingTitleUpdate,
		LoggedIn,
		MAX,
	};
}

USTRUCT()
struct FMapVignetteInfo
{
	GENERATED_USTRUCT_BODY()

public:
	// The filename (relative to the Movie folder) of the movie file to play for this vignette
	UPROPERTY(EditInstanceOnly, AssetRegistrySearchable, Category = LevelSummary)
	FString MovieFilename;

	UPROPERTY(EditInstanceOnly, AssetRegistrySearchable, Category = LevelSummary)
	FText Description;

	FMapVignetteInfo()
		: MovieFilename(TEXT(""))
		, Description(FText::GetEmpty())
	{
	}

	FMapVignetteInfo(const FString& inMovieFilename, const FText& InDescription)
		: MovieFilename(inMovieFilename)
		, Description(InDescription)
	{
	}

};

static FName NAME_Vignettes(TEXT("Vignettes"));

namespace EMenuCommand
{
	const FName MC_QuickPlayDM = FName(TEXT("MenuOption_QuickPlayDM"));
	const FName MC_QuickPlayCTF = FName(TEXT("MenuOption_QuickPlayCTF"));
	const FName MC_QuickPlayFlagrun = FName(TEXT("MenuOption_QuickPlayFlagrun"));
	const FName MC_QuickPlayShowdown = FName(TEXT("MenuOption_QuickPlayShowdown"));
	const FName MC_Challenges = FName(TEXT("MenuOption_QuickPlayChallenges"));
	const FName MC_Tutorial = FName(TEXT("MenuOption_Tutorial"));
	const FName MC_InstantAction = FName(TEXT("MenuOption_InstantAction"));
	const FName MC_FindAMatch = FName(TEXT("MenuOption_FindAMatch"));
}

namespace ETutorialTags
{
	const FName TUTTAG_Movement = FName(TEXT("MovementTutorial"));
	const FName TUTTAG_Weapons = FName(TEXT("WeaponsTutorial"));
	const FName TUTTAG_Pickups = FName(TEXT("PickupTutorial"));
	const FName TUTTAG_DM = FName(TEXT("DMTutorial"));
	const FName TUTTAG_Flagrun = FName(TEXT("FlagRunTutorial"));
	const FName TUTTAG_Showdown = FName(TEXT("ShowdownTutorial"));
	const FName TUTTAG_TDM = FName(TEXT("TDMTutorial"));
	const FName TUTTAG_CTF = FName(TEXT("CTFTutorial"));
	const FName TUTTAG_Duel = FName(TEXT("DuelTutorial"));

	// This tag is used to force the game to play the next tutorial in the progression based on what the
	// player has already played.  See UUTLocalPlayer::LaunchTutorial
	const FName TUTTAG_Progress = FName(TEXT("NextTutorialProgression"));

	// Used to set what tutorial should be auto-launched during new player onboarding.
	const FName TUTTAG_NewPlayerLaunchTutorial = TUTTAG_Flagrun;
}

UENUM()
namespace ETutorialSections
{
	enum Type
	{
		SkillMoves,
		Gameplay,
		Hardcore,
		MAX,
	};
}


USTRUCT()
struct FTutorialData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(Config)
	FName Tag;

	UPROPERTY(Config)
	uint16 Mask;

	UPROPERTY(Config)
	FString Map;

	UPROPERTY(Config)
	FString LaunchArgs;

	UPROPERTY(Config)
	FString LoadingMovie;

	UPROPERTY(Config)
	FString LoadingText;

	FTutorialData()
		: Tag(NAME_None)
		, Mask(0x00)
		, Map(TEXT(""))
		, LaunchArgs(TEXT(""))
		, LoadingMovie(TEXT(""))
	{
	}

	FTutorialData(const FName& inTag, uint16 inMask,const FString& inMap, const FString& inLaunchArgs, const FString& inLoadingMovie, const FString& inLoadingText)	
		: Tag(inTag)
		, Mask(inMask)
		, Map(inMap)
		, LaunchArgs(inLaunchArgs)
		, LoadingMovie(inLoadingMovie)
		, LoadingText(inLoadingText)
	{
	}

};

USTRUCT()
struct FHUDandUMGParticleSystemTracker
{

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY()
	FVector LocationModifier;

	UPROPERTY()
	FRotator DirectionModifier;

	UPROPERTY()
	FVector2D ScreenLocation;

	FHUDandUMGParticleSystemTracker()
		: ParticleSystemComponent(nullptr)
	{
	}


	FHUDandUMGParticleSystemTracker(UParticleSystemComponent* inParticleSystemComponent, FVector inLocationModifier, FRotator inDirectionModifier, FVector2D inScreenLocation)
	{
		ParticleSystemComponent = inParticleSystemComponent;	
		LocationModifier = inLocationModifier;
		DirectionModifier = inDirectionModifier;
		ScreenLocation = inScreenLocation;
	}

};

UENUM()
namespace ECreateInstanceTypes
{
	enum Type
	{
		Lobby,
		Standalone,
		LAN,
		MAX,
	};
}

USTRUCT()
struct FBanInfo 
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString UserName;

	UPROPERTY()
		FString UniqueID;

	FBanInfo()
		: UserName(TEXT(""))
		, UniqueID(TEXT(""))
	{
	}

	FBanInfo(const FString& inUserName, const FString& inUniqueID)
		: UserName(inUserName)
		, UniqueID(inUniqueID)
	{
	}

	FText GetUserName() const
	{
		return FText::FromString(UserName);
	}

	FText GetUniqueID() const
	{
		return FText::FromString(UniqueID);
	}

	static TSharedRef<FBanInfo> Make(const FBanInfo& Original)
	{
		return MakeShareable( new FBanInfo(Original.UserName, Original.UniqueID));
	}

};

USTRUCT()
struct FMCPAnnouncement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FDateTime StartDate;
	
	UPROPERTY()
	FDateTime EndDate;

	UPROPERTY()
	FString AnnouncementURL;

	UPROPERTY()
	float MinHeight;

	UPROPERTY()
	bool bHasAudio;

	FMCPAnnouncement()
		: Title(TEXT(""))
		, StartDate(FDateTime::Now())
		, EndDate(FDateTime::Now())
		, AnnouncementURL(TEXT(""))
		, MinHeight(240)
		, bHasAudio(false)
	{
	}

	FMCPAnnouncement(FDateTime inStartDate, FDateTime inEndDate, const FString& inTitle, const FString& inURL, float inMinHeight=240, bool inbHasAudio=false)
		: Title(inTitle)
		, StartDate(inStartDate)
		, EndDate(inEndDate)
		, AnnouncementURL(inURL)
		, MinHeight(inMinHeight)
		, bHasAudio(inbHasAudio)
	{
	}

};

USTRUCT()
struct FMCPAnnouncementBlob
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FMCPAnnouncement> Announcements;	
};


USTRUCT()
struct UNREALTOURNAMENT_API FUTGameRuleset
{
	GENERATED_USTRUCT_BODY()
public:
	// Holds the name of this rule set.  NOTE it must be unique on this server.  
	UPROPERTY()
	FString UniqueTag;

	// Holds a list of Ruleset Categories this rule set should show up in.  This determines which tabs this rule is sorted in to
	UPROPERTY()
	TArray<FName> Categories;

	// Holds the of this rule set.  It will be displayed over the badge
	UPROPERTY()
	FString Title;

	// Holds the description for this Ruleset.  It will be displayed above the rules selection window.
	UPROPERTY()
	FString Tooltip;

	// Holds the description for this Ruleset.  It will be displayed above the rules selection window.
	UPROPERTY()
	FString Description;

	UPROPERTY()
	TArray<FString> MapPrefixes;

	// Holds the Max # of maps available in this maplist or 0 for no maximum
	UPROPERTY()
	int32 MaxMapsInList;
	
	// Which epic maps to include.  This can't be adjusted via the ini and will be added
	// to the map list before the custom maps.
	UPROPERTY()
	FString EpicMaps;	

	// The default map to use
	UPROPERTY()
	FString DefaultMap;

	UPROPERTY(Config)
	TArray<FString> CustomMapList;

	// The number of players allowed in this match.  NOTE: it must be duplicated in the GameOptions string.
	UPROPERTY()
	int32 MaxPlayers;

	// The Max # of teams available with this ruleset
	UPROPERTY()
	int32 MaxTeamCount;

	// The # of players per team max
	UPROPERTY()
	int32 MaxTeamSize;

	// The max # of players allowed in a single party
	UPROPERTY()
	int32 MaxPartySize;

	// Holds a string reference to the material to display that represents this rule
	UPROPERTY()
	FString DisplayTexture;

	// Not displayed, this holds the game type that will be passed to the server via the URL.  
	UPROPERTY()
	FString GameMode;
	
	// Hold the ?xxxx options that will be used to start the server.  NOTE: this set of options will be parsed for display.
	UPROPERTY()
	FString GameOptions;
	
	UPROPERTY()
	TArray<FString> RequiredPackages;

	UPROPERTY()
	bool bTeamGame;

	/** If competitive, no join in progress allowed, and all players must ready up for match to start. */
	UPROPERTY()
	bool bCompetitiveMatch;

	UPROPERTY()
	uint32 OptionFlags;

	/** If true, this ruleset will not be shown in the UI.  This this is used for rulesets associated with quick play. */
	UPROPERTY()
	bool bHideFromUI;

	/** This will allow Epic to override any UI visibility of a rule.  If it's < 0 then the rule will be removed.  If it's > 0 then it will be displayed.  Leave at 0 to use the original settings. */
	UPROPERTY()
	int32 EpicForceUIVisibility;
	
	FUTGameRuleset()
		: UniqueTag(TEXT(""))
		, Title(TEXT(""))
		, Tooltip(TEXT(""))
		, Description(TEXT(""))
		, EpicMaps(TEXT(""))
		, DefaultMap(TEXT(""))
		, MaxPlayers(0)
		, MaxTeamCount(0)
		, MaxTeamSize(0)
		, MaxPartySize(0)
		, DisplayTexture(TEXT(""))
		, GameMode(TEXT(""))
		, GameOptions(TEXT(""))
		, bTeamGame(false)
		, bCompetitiveMatch(false)
		, OptionFlags(GAME_OPTION_FLAGS_All)
		, bHideFromUI(false)
		, EpicForceUIVisibility(0)
	{
		Categories.Empty();
		MapPrefixes.Empty();
		CustomMapList.Empty();
		RequiredPackages.Empty();
	}

	bool operator == (const FUTGameRuleset& Other) const 
	{ 
		return UniqueTag.Compare(Other.UniqueTag, ESearchCase::IgnoreCase) == 0; 
	}

	void GetCompleteMapList(TArray<FString>& OutMapList, bool bInsureNew = false)
	{
		if (bInsureNew) OutMapList.Empty();

		if (!EpicMaps.IsEmpty())
		{
			EpicMaps.ParseIntoArray(OutMapList, TEXT(","), true);
		}

		for (int32 i=0; i < CustomMapList.Num(); i++)
		{
			OutMapList.Add(CustomMapList[i]);
		}
	}

	FString GenerateURL(const FString& StartingMap, bool bAllowBots, int32 BotDifficulty, bool bRequireFilled)
	{
		FString URL = StartingMap;
		URL += FString::Printf(TEXT("?Game=%s"), *GameMode);
		URL += FString::Printf(TEXT("?MaxPlayers=%i"), MaxPlayers);
		URL += GameOptions;
		if (URL.Find(TEXT("difficulty="),ESearchCase::IgnoreCase) == INDEX_NONE)
		{
			URL += bAllowBots ? FString::Printf(TEXT("?Difficulty=%i"), FMath::Clamp<int32>(BotDifficulty, 0, 7)) : TEXT("?ForceNoBots=1");
		}

		if (bRequireFilled) URL += TEXT("?RequireFull=1");
		if (bCompetitiveMatch) URL += TEXT("?NoJIP");

		// Let the game know what ruleset this is.
		URL += FString::Printf(TEXT("?ART=%s"), *UniqueTag );
		return URL;	
	}
};


USTRUCT()
struct UNREALTOURNAMENT_API FUTGameRulesetStorage
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY()
	TArray<FUTGameRuleset> Rules;

	FUTGameRulesetStorage()
	{
	}
};

USTRUCT()
struct UNREALTOURNAMENT_API FUTGameModeCountStorage
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY()
	FString GameModeClass;

	UPROPERTY()
	int32 PlayCount;


	FUTGameModeCountStorage()
	{
		GameModeClass = TEXT("");
		PlayCount = 0;
	}

	FUTGameModeCountStorage(FString inGameModeClass)
	{
		GameModeClass = inGameModeClass;
		PlayCount = 1;
	}

};

USTRUCT()
struct FPlaylistItem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 PlaylistId;

	UPROPERTY()
	bool bRanked;

	UPROPERTY()
	bool bSkipEloChecks;

	UPROPERTY()
	FString TeamEloRating;

	UPROPERTY()
	bool bAllowBots;

	UPROPERTY()
	int32 BotDifficulty;

	// NOTE: We want the ability to have a play list show a different slate badge then
	// the ruleset in the menus.  So allow it to be set here.
	UPROPERTY()
	FString SlateBadgeName;

	// When the play list is updated, it will be sorted based on this...
	UPROPERTY()
	float SortWeight;

	// The Tag of the ruleset that this play list uses.
	UPROPERTY()
	FString RulesetTag;

	UPROPERTY()
	bool bHideInUI;

	FPlaylistItem()
		: bAllowBots(true)
		, BotDifficulty(3)
		, SortWeight(0.0f)
		, bHideInUI(false)
	{
	}

};

UENUM()
namespace EComFilter
{
	enum Type
	{
		AllComs,
		TeamComs, 
		FriendComs,
		NoComs,
		MAX,
	};
}
