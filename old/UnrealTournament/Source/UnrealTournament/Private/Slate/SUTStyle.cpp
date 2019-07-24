// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "Slate/SlateBrushAsset.h"
#include "SUTStyle.h"

#if WITH_SOCIAL
#include "../../UTSocialStyle.h"
#include "Social.h"
#endif

#if !UE_SERVER
TSharedPtr<FSlateStyleSet> SUTStyle::UWindowsStyleInstance = NULL;

void SUTStyle::Initialize()
{
	if (!UWindowsStyleInstance.IsValid())
	{
		UWindowsStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *UWindowsStyleInstance);
	}
}

void SUTStyle::Shutdown()
{
}

FName SUTStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SUTStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... )	FSlateImageBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) 		FSlateBoxBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )

#define BORDER_BRUSH( RelativePath, ... ) 	FSlateBorderBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) 		FSlateFontInfo( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) 		FSlateFontInfo( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".otf"), __VA_ARGS__ )

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

const int32 FONT_SIZE_Teenie = 9;
const int32 FONT_SIZE_Tiny = 12;
const int32 FONT_SIZE_Small = 14;
const int32 FONT_SIZE_Tween = 19;
const int32 FONT_SIZE_Medium = 24;
const int32 FONT_SIZE_Large = 32;
const int32 FONT_SIZE_Huge = 64;

const int32 FONT_SIZE_Notice = 20;
const int32 FONT_SIZE_Browser = 16;

const FColor SuperDark(1,1,1,255);

const FColor SuperDarkListA(2, 2, 2, 255);
const FColor SuperDarkListB(3, 3, 3, 255);

const FColor Dark(4,4,4,255);
const FColor Navy(0,0,4,255);
const FColor DeepRed(6,0,0,255);
const FColor Medium(10,10,10,255);
const FColor Light(14,14,14,255);
const FColor SuperLight(32,32,32,255);
const FColor UltraBright(61,135,255,255);
const FColor Bright(128,128,128,255);
const FColor WhiteBright(255,255,255,255);
const FColor GoldBright(255,255,171,255);
const FColor Disabled(189,189,189,255);
const FColor Shaded(4,4,4,200);


const FColor SelectedColor(64,62,48,255);

const FColor TestRed(128,0,0,255);
const FColor TestBlue(0,0,128,255);
const FColor TestGreen(0,128,0,255);
const FColor TestYellow(128,128,0,255);
const FColor TestPurple(128,0,128,255);
const FColor TestAqua(0,128,128,255);

const FColor TabSelected(128,128,128,255);


const FColor Pressed(250,250,250,255);
const FColor Hovered(200,200,200,255);

FSlateSound SUTStyle::ButtonPressSound;
FSlateSound SUTStyle::ButtonHoverSound;
FSlateSound SUTStyle::MessageSound;
FSlateSound SUTStyle::PauseSound;

FSlateColor SUTStyle::DefaultForeground;

TSharedRef<FSlateStyleSet> SUTStyle::Create()
{
	TSharedRef<FSlateStyleSet> StyleRef = MakeShareable(new FSlateStyleSet(SUTStyle::GetStyleSetName()));

	StyleRef->SetContentRoot(FPaths::GameContentDir() / TEXT("RestrictedAssets/Slate"));
	StyleRef->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));


	FSlateStyleSet& Style = StyleRef.Get();

	ButtonHoverSound = FSlateSound::FromName_DEPRECATED(FName("SoundCue'/Game/RestrictedAssets/UI/UT99UI_LittleSelect_Cue.UT99UI_LittleSelect_Cue'"));
	ButtonPressSound = FSlateSound::FromName_DEPRECATED(FName("SoundCue'/Game/RestrictedAssets/UI/UT99UI_BigSelect_Cue.UT99UI_BigSelect_Cue'"));
	MessageSound = FSlateSound::FromName_DEPRECATED(FName("SoundCue'/Game/RestrictedAssets/Audio/UI/A_UI_Attention02_Cue.A_UI_Attention02_Cue'"));
	PauseSound = FSlateSound::FromName_DEPRECATED(FName("SoundCue'/Game/RestrictedAssets/Audio/UI/A_UI_Pause01_Cue.A_UI_Pause01_Cue'"));


	SetFonts(StyleRef);
	SetIcons(StyleRef);
	SetCommonStyle(StyleRef);
	SetAvatars(StyleRef);
	SetRankBadges(StyleRef);
	SetChallengeBadges(StyleRef);
	SetContextMenus(StyleRef);
	SetServerBrowser(StyleRef);
	SetLoginStyle(StyleRef);

	#if WITH_SOCIAL
	USocialStyleAsset* SocialAsset = LoadObject<USocialStyleAsset>(NULL, TEXT("/Game/RestrictedAssets/UI/UTSocialStyle.UTSocialStyle"), NULL, LOAD_None, NULL);
	SocialAsset->AddToRoot();
	ISocialModule::Get().GetFriendsAndChatManager(TEXT(""), true)->InitializeSocialStyle(&SocialAsset->Style);
	#endif

	return StyleRef;
}

void SUTStyle::SetFonts(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("UT.Font.NormalText.Teenie", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Teenie)).SetColorAndOpacity(FLinearColor::White));

	Style.Set("UT.Font.NormalText.Tiny", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Tiny)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Tiny.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Tiny)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Tiny.Bold.Gold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Tiny)).SetColorAndOpacity(FLinearColor::Yellow));

	Style.Set("UT.Font.NormalText.Small", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Small.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Small.Gray", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor::Gray));

	Style.Set("UT.Font.Chat.Text", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor(0.7,0.7,0.7,1.0)));
	Style.Set("UT.Font.Chat.Text.Whisper", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-LightItalic", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor(0.7,0.7,0.7,1.0)));
	Style.Set("UT.Font.Chat.Name", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor::White));

	Style.Set("UT.Font.NormalText.Tween", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Tween)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Tween.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Tween)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Tween.Bold.SkyBlue", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Tween)).SetColorAndOpacity(FLinearColor(0.30f, 0.77f,0.94f,1.0f)));

	Style.Set("UT.Font.NormalText.Medium", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Medium.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Medium.Gray", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor::Gray));
	Style.Set("UT.Font.NormalText.Medium.Error", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor::Red));
	Style.Set("UT.Font.NormalText.Medium.Link", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor(0.0f,0.0f,0.6f,1.0f)));
	Style.Set("UT.Font.NormalText.Medium.Orange", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor(1.0f, 0.412f, 0.027f, 1.0f)));

	Style.Set("UT.Font.NormalText.Large", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Large)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Large.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Large)).SetColorAndOpacity(FLinearColor::White));
	
	Style.Set("UT.Font.TeamScore.Red", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Large)).SetColorAndOpacity(FLinearColor::Red));
	Style.Set("UT.Font.TeamScore.Blue", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Large)).SetColorAndOpacity(FLinearColor::Blue));

	Style.Set("UT.Font.NormalText.Huge", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Huge)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.NormalText.Huge.Bold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Huge)).SetColorAndOpacity(FLinearColor::White));

	Style.Set("UT.Font.Notice", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Notice)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.Notice.Gold", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Notice)).SetColorAndOpacity(FLinearColor(255.0, 255.0, 96 / 255.0 ,1.0)));
	Style.Set("UT.Font.Notice.Blue", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Notice)).SetColorAndOpacity(FLinearColor(25.0/255.0,48.0 / 255.0,180.0 / 255, 1.0)));

	Style.Set("UT.Font.MenuBarText", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Medium)).SetColorAndOpacity(FLinearColor::White));

	Style.Set("UT.Font.ServerBrowser.List.Header", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Browser)).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.ServerBrowser.List.Normal", TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Browser));
	Style.Set("UT.Font.ServerBrowser.List.Bold", TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", FONT_SIZE_Browser));

	SetTextChatStyle(StyleRef);
}


void SUTStyle::SetIcons(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();
	Style.Set("UT.Icon.Lock.Small", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Lock.Small", FVector2D(18,18), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.Lan.Small", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Lan.Small", FVector2D(18,18), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.Friends.Small", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Friends.Small", FVector2D(18,18), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set( "UT.Icon.Friends.Small.Inline", FInlineTextImageStyle()
		.SetImage( IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Friends.Small", FVector2D(18,18), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)) )
		.SetBaseline( 0 )
	);

	Style.Set("UT.Icon.Friends.Small", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Friends.Small", FVector2D(18,18), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.Checkmark", new IMAGE_BRUSH( "/UTStyle/Icons/UT.Icon.Checkmark", FVector2D(64,64), FLinearColor(1.0f, 1.0f, 0.0f, 1.0f) ));
	Style.Set("UT.Icon.PlayerCard", new IMAGE_BRUSH("/UTStyle/Icons/UT.Icon.PlayCard", FVector2D(48,48)));

	Style.Set("UT.Icon.BackArrow", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.BackArrow", FVector2D(12,8), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.ComboTick", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.ComboTick", FVector2D(8,8), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortDown", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortDown", FVector2D(8,4), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortDownX2", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortDownX2", FVector2D(16,4), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortUp", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortUp", FVector2D(8,4), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortUpX2", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortUpX2", FVector2D(16,4), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.SortTop", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortTop", FVector2D(8, 8), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortBottom", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortBottom", FVector2D(8, 8), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.SortUp.Big", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortUp48x48", FVector2D(48, 48), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortDown.Big", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortDown48x48", FVector2D(48, 48), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortTop.Big", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortTop48x48", FVector2D(48, 48), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.SortBottom.Big", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SortBottom48x48", FVector2D(48, 48), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.Alert", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Alert", FVector2D(64,64), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.Server.Epic", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Server.Epic", FVector2D(54, 54), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.Server.Trusted", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Server.Trusted", FVector2D(54, 54), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Icon.Server.Untrusted", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Server.Untrusted", FVector2D(54, 54), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.Icon.Star.24x24", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Star.24x24", FVector2D(24, 24), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	
	Style.Set("UT.Icon.Back", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Back", FVector2D(48, 48)));
	Style.Set("UT.Icon.Forward", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Forward", FVector2D(48, 48)));
	Style.Set("UT.Icon.About", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.About", FVector2D(48, 48)));
	Style.Set("UT.Icon.Online", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Online", FVector2D(48, 48)));
	Style.Set("UT.Icon.Settings", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Settings", FVector2D(48, 48)));
	Style.Set("UT.Icon.Exit", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Exit", FVector2D(48, 48)));
	Style.Set("UT.Icon.Stats", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Stats", FVector2D(48, 48)));
	Style.Set("UT.Icon.Chat36", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Chat36", FVector2D(36, 36)));
	Style.Set("UT.Icon.Chat.Inverted", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Chat.Inverted", FVector2D(36, 36)));
	
	Style.Set("UT.Icon.Lan.Big", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Lan.Big", FVector2D(90, 64)));


	Style.Set("UT.Icon.Browser", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Browser", FVector2D(48, 48)));
	Style.Set("UT.Icon.SocialBang", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SocialBang", FVector2D(12, 12)));

	Style.Set("UT.Icon.Minimize", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Minimize", FVector2D(48, 48)));
	Style.Set("UT.Icon.Fullscreen", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Fullscreen", FVector2D(48, 48)));
	Style.Set("UT.Icon.Windowed", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Windowed", FVector2D(48, 48)));


	Style.Set("UT.Icon.SignOut", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SignOut", FVector2D(48, 48)));
	Style.Set("UT.Icon.SignIn",  new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.SignIn", FVector2D(48, 48)));

	Style.Set("UT.Icon.ChangeTeam", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.ChangeTeam", FVector2D(48, 48)));
	Style.Set("UT.Icon.ChangeTeam.Red", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.ChangeTeam.Red", FVector2D(48, 48)));
	Style.Set("UT.Icon.ChangeTeam.Blue", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.ChangeTeam.Blue", FVector2D(48, 48)));

	Style.Set("UT.Icon.PartyMember", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.PartyMember", FVector2D(48, 48)));
	Style.Set("UT.Icon.PartyMember.Empty", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.PartyMember.Empty", FVector2D(48, 48)));
	Style.Set("UT.Icon.PartyLeader", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.PartyLeader", FVector2D(48, 48)));

	Style.Set("UT.Icon.LockedContent", new IMAGE_BRUSH( "UTStyle/Icons/UT.Icon.LockedContent", FVector2D(128.0f, 128.0f)));

	Style.Set("UT.Icon.Pumpkin", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Pumpkin", FVector2D(64.0, 64.0)));
	Style.Set("UT.Icon.Skull.128x128", new IMAGE_BRUSH("UTStyle/Icons/UT.Icon.Skull.128x128", FVector2D(128.0, 128.0)));
}


void SUTStyle::SetCommonStyle(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();


	Style.Set("UT.Background.Shadow", new FSlateColorBrush(FLinearColor(0, 0, 0, .8f)));

	Style.Set("UT.SimpleButton", FButtonStyle()
		.SetNormal( FSlateColorBrush(FColor(25,48,180,255)) )
		.SetHovered( FSlateColorBrush(FColor(67,128,224,255)) )
		.SetPressed( FSlateColorBrush(FColor(32,32,32,255)) )
		.SetDisabled( FSlateColorBrush(FColor(1,1,1,255)) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.ClearButton", FButtonStyle()
		.SetNormal( FSlateNoResource(FVector2D(128.0f, 128.0f)) )
		.SetHovered( FSlateNoResource(FVector2D(128.0f, 128.0f)) )
		.SetPressed( FSlateNoResource(FVector2D(128.0f, 128.0f)) )
		.SetDisabled( FSlateNoResource(FVector2D(128.0f, 128.0f)) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.SimpleButton.Dark", FButtonStyle()
		.SetNormal( FSlateColorBrush(Dark) )
		.SetHovered( FSlateColorBrush(Hovered) )
		.SetPressed( FSlateColorBrush(Pressed) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.SimpleButton.SuperDark", FButtonStyle()
		.SetNormal( FSlateColorBrush(FLinearColor::Black) )
		.SetPressed( FSlateColorBrush(Pressed) )
		.SetHovered( FSlateColorBrush(Hovered) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);


	Style.Set("UT.SimpleButton.Medium", FButtonStyle()
		.SetNormal( FSlateColorBrush(Medium) )
		.SetHovered( FSlateColorBrush(Hovered) )
		.SetPressed( FSlateColorBrush(Pressed) )
		.SetDisabled( FSlateColorBrush(SuperDark) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.SimpleButton.Bright", FButtonStyle()
		.SetNormal( FSlateColorBrush(SuperLight) )
		.SetHovered( FSlateColorBrush(UltraBright) )
		.SetPressed( FSlateColorBrush(Light) )
		.SetDisabled( FSlateColorBrush(SuperDark) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.SimpleButton.Keybind", FButtonStyle()
		.SetNormal( FSlateColorBrush(Bright) )
		.SetHovered( FSlateColorBrush(WhiteBright) )
		.SetPressed( FSlateColorBrush(GoldBright) )
		.SetDisabled( FSlateColorBrush(SuperDark) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);


	Style.Set("UT.SimpleButton.SuperDark", FButtonStyle()
		.SetNormal( FSlateColorBrush(Dark) )
		.SetHovered( FSlateColorBrush(Medium) )
		.SetPressed( FSlateColorBrush(SuperDark) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.Button.Clear", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Button.Clear", FVector2D(36, 24)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Button.Clear.Hover", FVector2D(36, 24)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Button.Clear.Press", FVector2D(36, 24)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.Button.Abort", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Button.Abort", FVector2D(36, 24)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Button.Abort.Hover", FVector2D(36, 24)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Button.Abort.Press", FVector2D(36, 24)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.ArrowButton.Left", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Left", FVector2D(256, 256)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Left.Hover", FVector2D(256, 256)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Left.Press", FVector2D(256, 256)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.ArrowButton.Right", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Right", FVector2D(256, 256)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Right.Hover", FVector2D(256, 256)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Button.Arrow.Right.Press", FVector2D(256, 256)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.WeaponWheelButton", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Button.WeaponWheel", FVector2D(189, 113)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Button.WeaponWheel.Hover", FVector2D(189, 113)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Button.WeaponWheel.Press", FVector2D(189, 113)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.Button.Soft", FButtonStyle()
		.SetNormal(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f), FLinearColor(0.8f, 0.8f, 0.8f, 1.0f)))
		.SetHovered(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White.Highlighted", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f), FLinearColor(0.9f, 0.9f, 0.9f, 1.0f)))
		.SetPressed(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White.Pressed", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.Button.Soft.Gold", FButtonStyle()
		.SetNormal(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f), FLinearColor(1.0f, 1.0f, 0.0f, 1.0f)))
		.SetHovered(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White.Highlighted", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f), FLinearColor(1.0f, 1.0f, 0.33f, 1.0f)))
		.SetPressed(BOX_BRUSH("UTStyle/Buttons/Button.Soft.White.Pressed", FVector2D(64,32), FMargin(12.0f /64.0f, 12.0f/32.0f),FLinearColor(1.0f, 1.0f, 0.6f, 1.0f)))
		.SetDisabled(FSlateColorBrush(Disabled))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);


	//new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f / 32.0f), FLinearColor(1, 1, 1, 0.5)));


	Style.Set("UT.NoStyle", new FSlateNoResource(FVector2D(128.0f, 128.0f)));

	Style.Set("UT.TeamColor.Red", new FSlateColorBrush(FColor(255,12,0,255)));
	Style.Set("UT.TeamColor.Blue", new FSlateColorBrush(FColor(12,12,255,255)));
	Style.Set("UT.TeamColor.Spectator", new FSlateColorBrush(FColor(200,200,200,255)));

	Style.Set("UT.ListBackground.Even", new FSlateColorBrush(FColor(5,5,5,255)));
	Style.Set("UT.ListBackground.Odd", new FSlateColorBrush(FColor(7,7,7,255)));

	Style.Set("UT.HeaderBackground.SuperDark", new FSlateColorBrush(SuperDark));

	Style.Set("UT.HeaderBackground.SuperDark.ListA", new FSlateColorBrush(SuperDarkListA));
	Style.Set("UT.HeaderBackground.SuperDark.ListB", new FSlateColorBrush(SuperDarkListB));

	Style.Set("UT.HeaderBackground.Dark", new FSlateColorBrush(Dark));
	Style.Set("UT.HeaderBackground.Navy", new FSlateColorBrush(Navy));
	Style.Set("UT.HeaderBackground.DeepRed", new FSlateColorBrush(DeepRed));
	Style.Set("UT.HeaderBackground.Medium", new FSlateColorBrush(Medium));
	Style.Set("UT.HeaderBackground.Light", new FSlateColorBrush(Light));
	Style.Set("UT.HeaderBackground.SuperLight", new FSlateColorBrush(SuperLight));
	Style.Set("UT.HeaderBackground.Shaded", new FSlateColorBrush(Shaded));

	Style.Set("UT.HeaderBackground.White", new FSlateColorBrush(WhiteBright));


	Style.Set("UT.Box", new FSlateColorBrush(FColor(13,13,13,153)));
	Style.Set("UT.Divider", new FSlateColorBrush(FColor(25, 25, 25, 255)));
	Style.Set("UT.Divider.Black", new FSlateColorBrush(FColor(0, 0, 0, 255)));

	Style.Set("UT.Star", new IMAGE_BRUSH( "Star24x24", FVector2D(24.0f,24.0f) ));
	Style.Set("UT.Star.Outline", new IMAGE_BRUSH( "StarOutline24x24", FVector2D(24.0f,24.0f) ));

	Style.Set("UT.ScaryStar", new IMAGE_BRUSH( "/UTStyle/ChallengeBadges/PumpkinA", FVector2D(24.0f,24.0f) ));
	Style.Set("UT.ScaryStar.Completed", new IMAGE_BRUSH( "/UTStyle/ChallengeBadges/PumpkinB", FVector2D(24.0f,24.0f) ));

	Style.Set("UT.Logo.Small", new IMAGE_BRUSH( "/UTStyle/Logos/UT.Logo.Small", FVector2D(301,98) ));
	Style.Set("UT.Logo.Loading", new IMAGE_BRUSH( "/UTStyle/Logos/UT.Logo.Loading", FVector2D(810.0f,290.0f) ));
	Style.Set("UT.Logo.Community", new IMAGE_BRUSH( "/UTStyle/Logos/UT.Logo.Community", FVector2D(188.0f,124.0f) ));

	Style.Set("UT.TabButton", FButtonStyle()
		.SetNormal( FSlateColorBrush(Dark) )
		.SetHovered( FSlateColorBrush(SuperDarkListA) )
		.SetPressed( FSlateColorBrush(TabSelected) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound) 
		);

	Style.Set("UT.OptionButton", FButtonStyle()
		.SetNormal( FSlateColorBrush(Dark) )
		.SetHovered( FSlateColorBrush(SuperDarkListA) )
		.SetPressed( FSlateColorBrush(SelectedColor) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound) 
		);

	Style.Set("UT.EditBox", FEditableTextBoxStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small))
		.SetForegroundColor(FLinearColor(0.75f,0.75f,0.75f,1.0f))
		.SetBackgroundImageNormal( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageHovered( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageFocused( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageReadOnly( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		);

	Style.Set("UT.EditBox.Boxed", FEditableTextBoxStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small))
		.SetForegroundColor(FLinearColor(0.75f,0.75f,0.75f,1.0f))
		.SetBackgroundImageNormal( FSlateColorBrush(SuperLight))
		.SetBackgroundImageHovered( FSlateColorBrush(UltraBright))
		.SetBackgroundImageFocused( FSlateColorBrush(UltraBright))
		.SetBackgroundImageReadOnly( FSlateColorBrush(Medium))
		);

	Style.Set("UT.EditBox.Boxed.Medium", FEditableTextBoxStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Medium))
		.SetForegroundColor(FLinearColor(0.75f,0.75f,0.75f,1.0f))
		.SetBackgroundImageNormal( FSlateColorBrush(Dark))
		.SetBackgroundImageHovered( FSlateColorBrush(SuperDark))
		.SetBackgroundImageFocused( FSlateColorBrush(SuperDarkListA))
		.SetBackgroundImageReadOnly( FSlateColorBrush(SuperDarkListB))
		);


	Style.Set("UT.ChatEditBox", FEditableTextBoxStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small))
		.SetForegroundColor(FLinearColor(0.75f,0.75f,0.75f,1.0f))
		.SetBackgroundImageNormal( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageHovered( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageFocused( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageReadOnly( FSlateNoResource(FVector2D(128.0f, 128.0f)))
		);

	Style.Set("UT.List.Background.Even", new FSlateColorBrush(FColor(27,27,27,255)));
	Style.Set("UT.List.Background.Odd", new FSlateColorBrush(FColor(30,30,30,255)));


	Style.Set("UT.CheckBox", FCheckBoxStyle()
		.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
		.SetUncheckedImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.UnChecked.Normal", FVector2D(32,32)))
		.SetUncheckedHoveredImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.UnChecked.Hovered", FVector2D(32,32)))
		.SetUncheckedPressedImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.UnChecked.Pressed", FVector2D(32,32)))
		.SetCheckedImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.Checked.Normal", FVector2D(32,32)))
		.SetCheckedHoveredImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.Checked.Normal", FVector2D(32,32)))
		.SetCheckedPressedImage(IMAGE_BRUSH("UTStyle/CommonControls/CheckBox/UT.CheckBox.Checked.Normal", FVector2D(32,32)))
		.SetUndeterminedImage(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetUndeterminedHoveredImage(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetUndeterminedPressedImage(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		);

	Style.Set("UT.MatchBadge.Circle", new IMAGE_BRUSH( "UTStyle/MatchBadges/UT.MatchBadge.Circle", FVector2D(78.0f, 78.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.MatchBadge.Circle.Thin", new IMAGE_BRUSH( "UTStyle/MatchBadges/UT.MatchBadge.Circle.Thin", FVector2D(78.0f, 78.0f), FLinearColor(0.3f, 0.3f, 0.3f, 1.0f) ));
	Style.Set("UT.MatchBadge.Circle.Tight", new IMAGE_BRUSH( "UTStyle/MatchBadges/UT.MatchBadge.Circle.Tight", FVector2D(78.0f, 78.0f), FLinearColor(0.3f, 0.3f, 0.3f, 1.0f) ));

	Style.Set("UT.List.Row", FTableRowStyle()
		.SetEvenRowBackgroundBrush(FSlateColorBrush(FColor(4,4,4,255)))
		.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FColor(5,5,5,255)))
		.SetOddRowBackgroundBrush(FSlateColorBrush(FColor(6,6,6,255)))
		.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FColor(7,7,7,255)))
		.SetSelectorFocusedBrush(FSlateColorBrush(FColor(32,32,32,255)))
		.SetActiveBrush(FSlateColorBrush(FColor(32,32,32,255)))
		.SetActiveHoveredBrush(FSlateColorBrush(FColor(32,32,32,255)))
		.SetInactiveBrush(FSlateColorBrush(FColor(32,32,32,255)))
		.SetInactiveHoveredBrush(FSlateColorBrush(FColor(32,32,32,255)))
		.SetTextColor(FLinearColor::White)
		.SetSelectedTextColor(FLinearColor::Black)
		);

	Style.Set("UT.PlayerList.Row", FTableRowStyle()
		.SetEvenRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FColor(5,5,5,255)))
		.SetOddRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FColor(7,7,7,255)))
		.SetSelectorFocusedBrush(FSlateColorBrush(FColor(9,9,9,255)))
		.SetActiveBrush(FSlateColorBrush(FColor(9,9,9,255)))
		.SetActiveHoveredBrush(FSlateColorBrush(FColor(9,9,9,255)))
		.SetInactiveBrush(FSlateColorBrush(FColor(9,9,9,255)))
		.SetInactiveHoveredBrush(FSlateColorBrush(FColor(9,9,9,255)))
		.SetTextColor(FLinearColor::White)
		.SetSelectedTextColor(FLinearColor::Black)
		);

	Style.Set("UT.PlayerList.Row.Red", FTableRowStyle()
		.SetEvenRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FColor(5,2,2,255)))
		.SetOddRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FColor(7,3,3,255)))
		.SetSelectorFocusedBrush(FSlateColorBrush(FColor(9,2,2,255)))
		.SetActiveBrush(FSlateColorBrush(FColor(9,2,2,255)))
		.SetActiveHoveredBrush(FSlateColorBrush(FColor(9,2,2,255)))
		.SetInactiveBrush(FSlateColorBrush(FColor(9,2,2,255)))
		.SetInactiveHoveredBrush(FSlateColorBrush(FColor(9,2,2,255)))
		.SetTextColor(FLinearColor::White)
		.SetSelectedTextColor(FLinearColor::Black)
		);


	Style.Set("UT.PlayerList.Row.Blue", FTableRowStyle()
		.SetEvenRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FColor(2,2,5,255)))
		.SetOddRowBackgroundBrush(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FColor(2,2,7,255)))
		.SetSelectorFocusedBrush(FSlateColorBrush(FColor(2,2,9,255)))
		.SetActiveBrush(FSlateColorBrush(FColor(2,2,9,255)))
		.SetActiveHoveredBrush(FSlateColorBrush(FColor(2,2,9,255)))
		.SetInactiveBrush(FSlateColorBrush(FColor(2,2,9,255)))
		.SetInactiveHoveredBrush(FSlateColorBrush(FColor(2,2,9,255)))
		.SetTextColor(FLinearColor::White)
		.SetSelectedTextColor(FLinearColor::Black)
		);





	FStringAssetReference T_TUT_ULogo_Shadow(TEXT("/Game/RestrictedAssets/SlateLargeImages/T_TUT_ULogo_Shadow_Brush.T_TUT_ULogo_Shadow_Brush"));
	USlateBrushAsset* T_TUT_ULogo_ShadowBrushAsset = Cast<USlateBrushAsset>(T_TUT_ULogo_Shadow.TryLoad());
	if (T_TUT_ULogo_ShadowBrushAsset)
	{
		T_TUT_ULogo_ShadowBrushAsset->AddToRoot();
		Style.Set("UT.HomePanel.TutorialLogo", &T_TUT_ULogo_ShadowBrushAsset->Brush);
	}

	FStringAssetReference Background(TEXT("/Game/RestrictedAssets/SlateLargeImages/Background_Brush.Background_Brush"));
	USlateBrushAsset* BackgroundBrushAsset = Cast<USlateBrushAsset>(Background.TryLoad());
	if (BackgroundBrushAsset)
	{
		BackgroundBrushAsset->AddToRoot();
		Style.Set("UT.HomePanel.Background", &BackgroundBrushAsset->Brush);
	}

	FStringAssetReference IABadge(TEXT("/Game/RestrictedAssets/SlateLargeImages/IABadge_Brush.IABadge_Brush"));
	USlateBrushAsset* IABadgeBrushAsset = Cast<USlateBrushAsset>(IABadge.TryLoad());
	if (IABadgeBrushAsset)
	{
		IABadgeBrushAsset->AddToRoot();
		Style.Set("UT.HomePanel.IABadge", &IABadgeBrushAsset->Brush);
	}

	FStringAssetReference NewChallenge(TEXT("/Game/RestrictedAssets/SlateLargeImages/NewChallenge_Brush.NewChallenge_Brush"));
	USlateBrushAsset* NewChallengeBrushAsset = Cast<USlateBrushAsset>(NewChallenge.TryLoad());
	if (NewChallengeBrushAsset)
	{
		NewChallengeBrushAsset->AddToRoot();
		Style.Set("UT.HomePanel.NewChallenge", &NewChallengeBrushAsset->Brush);
	}

	Style.Set("UT.HomePanel.NewFragCenter", new IMAGE_BRUSH("UTStyle/MainPanel/NewFragCenter", FVector2D(180,180), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.HomePanel.NewFragCenter.Transparent", new IMAGE_BRUSH("UTStyle/MainPanel/FragCenterEmblemTransparent", FVector2D(644,644), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.HomePanel.FragCenterLogo", new IMAGE_BRUSH("UTStyle/MainPanel/FragCenterEmblem", FVector2D(644, 644), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.HomePanel.Flak", new IMAGE_BRUSH( "UTStyle/MainPanel/Flak", FVector2D(180,180), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.FMBadge", new IMAGE_BRUSH( "UTStyle/MainPanel/FMBadge", FVector2D(380,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.DMBadge", new IMAGE_BRUSH( "UTStyle/MainPanel/DMBadge", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.CTFBadge", new IMAGE_BRUSH( "UTStyle/MainPanel/CTFBadge", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.TeamShowdownBadge", new IMAGE_BRUSH( "UTStyle/MainPanel/TeamShowdownBadge", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.Replays", new IMAGE_BRUSH( "UTStyle/MainPanel/Replays", FVector2D(180,180), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.Live", new IMAGE_BRUSH( "UTStyle/MainPanel/Live", FVector2D(180,180), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.BasicTraining", new IMAGE_BRUSH( "UTStyle/MainPanel/BasicTraining", FVector2D(800,220), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.ChallengesNewIcon", new IMAGE_BRUSH( "UTStyle/MainPanel/NewChallenge72x72", FVector2D(72,72), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));

	Style.Set("UT.HomePanel.BasicTraining", new IMAGE_BRUSH( "UTStyle/MainPanel/NewBadge4", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.Challenges", new IMAGE_BRUSH( "UTStyle/MainPanel/NewBadge2", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.HomePanel.vsBots", new IMAGE_BRUSH( "UTStyle/MainPanel/NewBadge3", FVector2D(250,270), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));


	Style.Set("UT.HomePanel.Button", FButtonStyle()
		.SetNormal( FSlateNoResource(FVector2D(256.0f, 256.0f) ))
		.SetHovered( BOX_BRUSH("UTStyle/MainPanel/Highlight", FVector2D(256,256), FMargin(16.0f / 256.0f, 16.0f/256.0f, 16.0f / 256.0f, 16.0f/256.0f), FLinearColor(200.0/255.0, 200.0/255.0, 200.0 / 255.0, 1.0) ))
		.SetPressed( BOX_BRUSH("UTStyle/MainPanel/Highlight", FVector2D(256,256), FMargin(16.0f / 256.0f, 16.0f/256.0f, 16.0f / 256.0f, 16.0f/256.0f), FLinearColor(1.0, 1.0, 1.0, 1.0) ))
		.SetDisabled( FSlateNoResource(FVector2D(256.0f, 256.0f) ))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.WeaponConfig.Button", FButtonStyle()
		.SetNormal(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetHovered(BOX_BRUSH("UTStyle/MainPanel/Highlight", FVector2D(256, 256), FMargin(16.0f / 256.0f, 16.0f / 256.0f, 16.0f / 256.0f, 16.0f / 256.0f), FLinearColor(200.0 / 255.0, 200.0 / 255.0, 200.0 / 255.0, 1.0)))
		.SetPressed(BOX_BRUSH("UTStyle/MainPanel/Pressed", FVector2D(256, 256), FMargin(16.0f / 256.0f, 16.0f / 256.0f, 16.0f / 256.0f, 16.0f / 256.0f), FLinearColor(1.0, 1.0, 1.0, 1.0)))
		.SetDisabled(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);

	Style.Set("UT.WeaponConfig.Use.Button", FButtonStyle()
		.SetNormal(IMAGE_BRUSH("UTStyle/Buttons/Use.Normal", FVector2D(192, 192)))
		.SetHovered(IMAGE_BRUSH("UTStyle/Buttons/Use.Hovered", FVector2D(192, 192)))
		.SetPressed(IMAGE_BRUSH("UTStyle/Buttons/Use.Pressed", FVector2D(192, 192)))
		.SetDisabled(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);


	Style.Set("UT.Button.MenuBar", FButtonStyle()
		.SetNormal( FSlateNoResource(FVector2D(256.0f, 256.0f)) )
		.SetPressed( FSlateColorBrush(Pressed) )
		.SetHovered( FSlateColorBrush(Hovered) )
		.SetDisabled( FSlateColorBrush(Disabled) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
	);


	Style.Set( "UT.ProgressBar", FProgressBarStyle()
		.SetBackgroundImage( FSlateColorBrush(FColor(4,4,4,255)) )
		.SetFillImage( FSlateColorBrush(FColor(200,200,200,255)) )
		.SetMarqueeImage( FSlateColorBrush(FColor(255,255,255,255)) )
		);

	Style.Set( "UT.ProgressBar.XP", FProgressBarStyle()
		.SetBackgroundImage( FSlateColorBrush(FColor(0,0,0,255)) )
		.SetFillImage( FSlateColorBrush(FColor(200,200,0,255)) )
		.SetMarqueeImage( FSlateColorBrush(FColor(255,255,0,255)) )
		);



	FComboButtonStyle ComboButton = FComboButtonStyle()
		.SetButtonStyle(Style.GetWidgetStyle<FButtonStyle>("UT.Button.MenuBar"))
		.SetDownArrowImage(IMAGE_BRUSH("UWindows.ComboBox.TickMark", FVector2D(8.0,4.0)))
		.SetMenuBorderBrush(FSlateColorBrush(Medium))
		.SetMenuBorderPadding(FMargin(5.0f, 0.05, 5.0f, 0.0f));
	Style.Set("UT.ComboButton", ComboButton);

	Style.Set("UT.ComboBox", FComboBoxStyle()
		.SetComboButtonStyle(ComboButton)
		);

	FStringAssetReference LoadingScreen(TEXT("/Game/RestrictedAssets/SlateLargeImages/LoadingScreen_Brush.LoadingScreen_Brush"));
	USlateBrushAsset* LoadingScreenBrushAsset = Cast<USlateBrushAsset>(LoadingScreen.TryLoad());
	if (LoadingScreenBrushAsset)
	{
		LoadingScreenBrushAsset->AddToRoot();
		Style.Set("UT.LoadingScreen", &LoadingScreenBrushAsset->Brush);
	}


	Style.Set("UT.Slider", FSliderStyle()
		.SetNormalBarImage(FSlateColorBrush(FColor::White))
		.SetDisabledBarImage(FSlateColorBrush(FLinearColor::Gray))
		.SetNormalThumbImage(IMAGE_BRUSH("UTCommon/UT.SliderHandle.Normal", FVector2D(32,32)))
		.SetDisabledThumbImage(IMAGE_BRUSH("UTCommon/UT.SliderHandle.Disabled", FVector2D(32,32)))
		);

	// Map Overlays
	Style.Set("UT.MapOverlay.Epic.WIP", new IMAGE_BRUSH( "UTStyle/MapOverlays/UT.MapOverlay.Epic.WIP", FVector2D(1024.0f, 512.0f)));
	Style.Set("UT.MapOverlay.Community", new IMAGE_BRUSH( "UTStyle/MapOverlays/UT.MapOverlay.Community", FVector2D(1024.0f, 512.0f)));
	Style.Set("UT.MapOverlay.Community.WIP", new IMAGE_BRUSH( "UTStyle/MapOverlays/UT.MapOverlay.Community.WIP", FVector2D(1024.0f, 512.0f)));

}

void SUTStyle::SetAvatars(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("UT.Avatar.0", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.0", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.1", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.1", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.2", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.2", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.3", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.3", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.4", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.4", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.5", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.5", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.Avatar.6", new IMAGE_BRUSH( "UTStyle/Avatars/UT.Avatar.6", FVector2D(50.0f, 50.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
}

void SUTStyle::SetRankBadges(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("UT.RankBadge.0", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Beginner.77x77", FVector2D(77.0f, 77.0f), FLinearColor(0.36f, 0.8f, 0.34f, 1.0f) ));
	Style.Set("UT.RankBadge.0.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Beginner.48x48", FVector2D(48.0f, 48.0f), FLinearColor(0.36f, 0.8f, 0.34f, 1.0f) ));

	Style.Set("UT.RankBadge.1", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Steel.77x77", FVector2D(77.0f, 77.0f), FLinearColor(0.4f, 0.235f, 0.07f, 1.0f) ));
	Style.Set("UT.RankBadge.1.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Steel.48x48", FVector2D(48.0f, 48.0f), FLinearColor(0.4f, 0.235f, 0.07f, 1.0f) ));

	Style.Set("UT.RankBadge.2", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Gold.77x77", FVector2D(77.0f, 77.0f), FLinearColor(0.96f, 0.96f, 0.96f, 1.0f) ));
	Style.Set("UT.RankBadge.2.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Gold.48x48", FVector2D(48.0f, 48.0f), FLinearColor(0.96f, 0.96f, 0.96f, 1.0f) ));

	Style.Set("UT.RankBadge.3", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Tarydium.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankBadge.3.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankBadge.Tarydium.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.0", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.One.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.0.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.One.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.0.Tiny", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.One.32x32", FVector2D(32.0f, 32.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.1", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Two.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.1.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Two.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.1.Tiny", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Two.32x32", FVector2D(32.0f, 32.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.2", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Three.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.2.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Three.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.2.Tiny", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Three.32x32", FVector2D(32.0f, 32.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.3", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Four.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.3.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Four.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.3.Tiny", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Four.32x32", FVector2D(32.0f, 32.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.4", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Five.77x77", FVector2D(77.0f, 77.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.4.Small", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Five.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));
	Style.Set("UT.RankStar.4.Tiny", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Five.32x32", FVector2D(32.0f, 32.0f), FLinearColor(1.0f, 0.95f, 0.42f, 1.0f) ));

	Style.Set("UT.RankStar.Empty", new IMAGE_BRUSH( "UTStyle/RankBadges/UT.RankStar.Five.48x48", FVector2D(48.0f, 48.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.0f) ));

	Style.Set("UT.RankedBronze", new IMAGE_BRUSH("UTStyle/RankBadges/UT.RankedBronze.128x128", FVector2D(128.0f, 128.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.RankedSilver", new IMAGE_BRUSH("UTStyle/RankBadges/UT.RankedSilver.128x128", FVector2D(128.0f, 128.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.RankedGold", new IMAGE_BRUSH("UTStyle/RankBadges/UT.RankedGold.128x128", FVector2D(128.0f, 128.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.RankedPlatinum", new IMAGE_BRUSH("UTStyle/RankBadges/UT.RankedPlatinum.128x128", FVector2D(128.0f, 128.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.RankedMaster", new IMAGE_BRUSH("UTStyle/RankBadges/UT.RankedMaster.128x128", FVector2D(128.0f, 128.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	
}


void SUTStyle::SetChallengeBadges(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();
	Style.Set("UT.ChallengeBadges.DM", new IMAGE_BRUSH( "UTStyle/ChallengeBadges/DeathmatchChallenge", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.ChallengeBadges.CTF", new IMAGE_BRUSH( "UTStyle/ChallengeBadges/CaptureTheFlagChallenge", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) ));
	Style.Set("UT.ChallengeBadges.CTF_Face", new IMAGE_BRUSH("UTStyle/ChallengeBadges/CaptureTheFlagChallenge_FacingWorlds", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.CTF_Pistola", new IMAGE_BRUSH("UTStyle/ChallengeBadges/CaptureTheFlagChallenge_Pistola", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.CTF_Titan", new IMAGE_BRUSH("UTStyle/ChallengeBadges/CaptureTheFlagChallenge_TitanPass", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.DM_Lea", new IMAGE_BRUSH("UTStyle/ChallengeBadges/DeathmatchChallenge_Lea", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.DM_OP23", new IMAGE_BRUSH("UTStyle/ChallengeBadges/DeathmatchChallenge_OP23", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.ChallengeBadges.GenericA", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeA", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericB", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeB", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericC", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeC", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericD", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeD", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericE", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeE", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericF", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeF", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericG", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeG", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.GenericH", new IMAGE_BRUSH("UTStyle/ChallengeBadges/GenericChallengeH", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.ChallengeBadges.SpookyA", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenA", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.SpookyB", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenB", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.SpookyC", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenC", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.SpookyD", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenD", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.SpookyE", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenE", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.SpookyF", new IMAGE_BRUSH("UTStyle/ChallengeBadges/HalloweenF", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

	Style.Set("UT.ChallengeBadges.UnderlandChallenge_SD", new IMAGE_BRUSH("UTStyle/ChallengeBadges/UnderlandChallenge_SD", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.UnderlandChallenge_FFA", new IMAGE_BRUSH("UTStyle/ChallengeBadges/UnderlandChallenge_FFA", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.UnderlandChallenge_TDM", new IMAGE_BRUSH("UTStyle/ChallengeBadges/UnderlandChallenge_TDM", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.ChillChallenge_SD", new IMAGE_BRUSH("UTStyle/ChallengeBadges/ChillChallenge_SD", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.ChillChallenge_FFA", new IMAGE_BRUSH("UTStyle/ChallengeBadges/ChillChallenge_FFA", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.ChallengeBadges.ChillChallenge_TDM", new IMAGE_BRUSH("UTStyle/ChallengeBadges/ChillChallenge_TDM", FVector2D(880.0f, 96.0f), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
}

void SUTStyle::SetContextMenus(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("UT.ContextMenu.Item", FButtonStyle()
		.SetNormal ( FSlateColorBrush(Medium) )
		.SetHovered( FSlateColorBrush(Hovered) )
		.SetPressed( FSlateColorBrush(Pressed) )
		.SetDisabled( FSlateColorBrush(SuperDark) )
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
		);

	Style.Set("UT.ContextMenu.Fill", new FSlateColorBrush(Medium));
	Style.Set("UT.ContextMenu.Item.Spacer", new FSlateColorBrush(SuperDark));
	Style.Set("UT.Font.ContextMenuItem", FTextBlockStyle().SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", FONT_SIZE_Small)).SetColorAndOpacity(FLinearColor::White));

}

void SUTStyle::SetServerBrowser(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	const FTableColumnHeaderStyle TableColumnHeaderStyle = FTableColumnHeaderStyle()
		.SetSortPrimaryAscendingImage(*Style.GetBrush("UT.Icon.SortUp"))
		.SetSortPrimaryDescendingImage(*Style.GetBrush("UT.Icon.SortDown"))
		.SetSortSecondaryAscendingImage(*Style.GetBrush("UT.Icon.SortUpX2"))
		.SetSortSecondaryDescendingImage(*Style.GetBrush("UT.Icon.SortDownX2"))
		.SetNormalBrush(FSlateColorBrush(Light))
		.SetHoveredBrush(FSlateColorBrush(Hovered))
		.SetMenuDropdownImage(*Style.GetBrush("UT.Icon.ComboTick"))
		.SetMenuDropdownNormalBorderBrush(FSlateColorBrush(Dark))
		.SetMenuDropdownHoveredBorderBrush(FSlateColorBrush(Dark));

	Style.Set("UT.List.Header.Column", TableColumnHeaderStyle);

	const FSplitterStyle TableHeaderSplitterStyle = FSplitterStyle()
		.SetHandleNormalBrush(FSlateColorBrush(SuperDark))
		.SetHandleHighlightBrush(FSlateColorBrush(Dark));

	Style.Set("UT.List.Header", FHeaderRowStyle()
		.SetColumnStyle(TableColumnHeaderStyle)
		.SetLastColumnStyle(TableColumnHeaderStyle)
		.SetColumnSplitterStyle(TableHeaderSplitterStyle)
		.SetBackgroundBrush(FSlateColorBrush(Dark))
		.SetForegroundColor(FLinearColor(SuperDark))
		);

}


void SUTStyle::SetLoginStyle(TSharedRef<FSlateStyleSet> StyleRef)
{
	FSlateStyleSet& Style = StyleRef.Get();

	Style.Set("UT.Login.Dialog.Background", new BOX_BRUSH("Login/UT.Login.Dialog.Background", FMargin(8.0f / 256.0f, 8.0f / 256.0f, 8.0f / 256.0f, 8.0f / 256.0f)));
	Style.Set("UT.Login.EpicLogo", new IMAGE_BRUSH("Login/UT.Login.EpicLogo", FVector2D(110, 126), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	Style.Set("UT.Login.Editbox.Background", new BOX_BRUSH("Login/UT.Login.Editbox.Normal", FMargin(4.0f / 338.0f, 4.0f / 62.0f, 4.0f / 338.0f, 4.0f / 62.0f)));

	Style.Set("UT.Login.Editbox", FEditableTextBoxStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Regular", 20))
		.SetForegroundColor(FLinearColor::Black)
		.SetBackgroundImageNormal(FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageHovered(FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageFocused(FSlateNoResource(FVector2D(128.0f, 128.0f)))
		.SetBackgroundImageReadOnly(FSlateNoResource(FVector2D(128.0f, 128.0f)))
		);

	Style.Set("UT.Login.Error.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 16))
		.SetColorAndOpacity(FLinearColor(FColor(126, 7, 13, 255))));

	Style.Set("UT.Login.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 20))
		.SetColorAndOpacity(FLinearColor(FColor(78, 78, 78, 255))));

	Style.Set("UT.Login.Label.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 20))
		.SetColorAndOpacity(FLinearColor(FColor(148, 148, 148, 255))));

	Style.Set("UT.Login.Button", FButtonStyle()
		.SetNormal(BOX_BRUSH("Login/UT.Login.Button.Normal", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetHovered(BOX_BRUSH("Login/UT.Login.Button.Hovered", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetPressed(BOX_BRUSH("Login/UT.Login.Button.Pressed", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetDisabled(BOX_BRUSH("Login/UT.Login.Button.Normal", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
		);

	Style.Set("UT.Login.Button.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 32))
		.SetColorAndOpacity(FLinearColor::White));

	Style.Set("UT.Login.EmptyButton", FButtonStyle()
		.SetNormal(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetHovered(BOX_BRUSH("Login/UT.Login.EmptyButton.Hovered", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetPressed(BOX_BRUSH("Login/UT.Login.EmptyButton.Hovered", FMargin(4.0f / 298.0f, 4.0f / 44.0f, 4.0f / 298.0f, 4.0f / 44.0f)))
		.SetDisabled(FSlateNoResource(FVector2D(256.0f, 256.0f)))
		.SetHoveredSound(ButtonHoverSound)
		.SetPressedSound(ButtonPressSound)
		);

	Style.Set("UT.Login.EmptyButton.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 20))
		.SetColorAndOpacity(FLinearColor(FColor(33, 93, 220, 255))));

	Style.Set("UT.Login.Offline.TextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("/UTStyle/Fonts/Lato/Lato-Bold", 22))
		.SetColorAndOpacity(FLinearColor::White));
		//.SetColorAndOpacity(FLinearColor(FColor(220, 93, 33, 255))));


}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT


void SUTStyle::Reload()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& SUTStyle::Get()
{
	return * UWindowsStyleInstance;
}

const FSlateColor SUTStyle::GetSlateColor( const FName PropertyName, const ANSICHAR* Specifier)
{
	if (PropertyName == FName(TEXT("NormalTextColor"))) return FSlateColor(FLinearColor(1.0,1.0,1.0,1.0));
	else if (PropertyName == FName(TEXT("FocusTextColor"))) return FSlateColor(FLinearColor(1.0,1.0,1.0,1.0));
	else if (PropertyName == FName(TEXT("HoverTextColor"))) return FSlateColor(FLinearColor(0.0,0.0,0.0,1.0));
	else if (PropertyName == FName(TEXT("PressedTextColor"))) return FSlateColor(FLinearColor(0.0,0.0,0.0,1.0));
	else if (PropertyName == FName(TEXT("DisabledTextColor"))) return FSlateColor(FLinearColor(0.0,0.0,0.0,1.0));

	else if (PropertyName == FName(TEXT("TabNormalTextColor"))) return FSlateColor(FLinearColor(1.0,1.0,1.0,1.0));
	else if (PropertyName == FName(TEXT("TabFocusTextColor"))) return FSlateColor(FLinearColor(1.0,1.0,1.0,1.0));
	else if (PropertyName == FName(TEXT("TabHoverTextColor"))) return FSlateColor(FLinearColor(1.0,1.0,1.0,1.0));
	else if (PropertyName == FName(TEXT("TabPressedTextColor"))) return FSlateColor(FLinearColor(0.0,0.0,0.0,1.0));
	else if (PropertyName == FName(TEXT("TabDisabledTextColor"))) return FSlateColor(FLinearColor(0.0,0.0,0.0,1.0));
	
	return FSlateColor(FLinearColor(0.6,0.6,0.6,1.0));
}


void SUTStyle::SetTextChatStyle(TSharedRef<FSlateStyleSet> StyleRef)
{	
	// Text Chat styles

	FSlateStyleSet& Style = StyleRef.Get();
	FTextBlockStyle NormalChatStyle = Style.GetWidgetStyle<FTextBlockStyle>("UT.Font.Chat.Text");

	Style.Set("UT.Font.Chat.Text.Global",		FTextBlockStyle(NormalChatStyle));
	Style.Set("UT.Font.Chat.Text.Lobby",		FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor::White));
	Style.Set("UT.Font.Chat.Text.Match",		FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor::Yellow));
	Style.Set("UT.Font.Chat.Text.Friends",		FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor::Green));
	Style.Set("UT.Font.Chat.Text.Team",			FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor::Yellow));
	Style.Set("UT.Font.Chat.Text.Team.Red",		FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor(1.0f, 0.25f, 0.25f, 1.0f)));
	Style.Set("UT.Font.Chat.Text.Team.Blue",	FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor(0.25, 0.25, 1.0, 1.0)));
	Style.Set("UT.Font.Chat.Text.Local",		FTextBlockStyle(NormalChatStyle));
	Style.Set("UT.Font.Chat.Text.Admin",		FTextBlockStyle(NormalChatStyle).SetColorAndOpacity(FLinearColor::Yellow));
}

#endif