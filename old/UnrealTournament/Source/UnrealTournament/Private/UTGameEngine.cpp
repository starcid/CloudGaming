// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTGameEngine.h"
#include "UTAnalytics.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "AssetRegistryModule.h"
#include "UTLevelSummary.h"
#include "Engine/Console.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Net/UnrealNetwork.h"
#include "UTConsole.h"
#include "UTFlagInfo.h"
#include "UTLobbyGameMode.h"
#include "UTMenuGameMode.h"
#include "UTGameInstance.h"
#include "IAnalyticsProvider.h"
#include "UTBotCharacter.h"
#include "UTEpicDefaultRulesets.h"
#include "UserActivityTracking.h"

#include "UTPlaylistManager.h"
#if !UE_SERVER
#include "SlateBasics.h"
#include "MoviePlayer.h"
#include "SUWindowsStyle.h"
#endif
#if PLATFORM_WINDOWS
#undef ERROR_SUCCESS
#undef ERROR_IO_PENDING
#undef E_NOTIMPL
#undef E_FAIL
#undef S_OK
#include "WindowsHWrapper.h"
#endif

// prevent setting MipBias to an intentionally broken value to make textures turn solid color
static void MipBiasClamp()
{
	IConsoleVariable* MipBiasVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.MipBias"));
	if (MipBiasVar != NULL)
	{
		TConsoleVariableData<float>* FloatVar = MipBiasVar->AsVariableFloat();
		if (FloatVar->GetValueOnGameThread() > 1.0f)
		{
			MipBiasVar->ClearFlags(ECVF_SetByConsole);
			MipBiasVar->Set(1.0f);
		}
	}
}
FAutoConsoleVariableSink MipBiasSink(FConsoleCommandDelegate::CreateStatic(&MipBiasClamp));

UUTGameEngine::UUTGameEngine(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	TAssetSubclassOf<AUTWeapon> WeaponRef;
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/Enforcer/Enforcer.Enforcer_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/Flak/BP_FlakCannon.BP_FlakCannon_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/ImpactHammer/BP_ImpactHammer.BP_ImpactHammer_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/LinkGun/BP_LinkGun.BP_LinkGun_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/Minigun/BP_Minigun.BP_Minigun_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/RocketLauncher/BP_RocketLauncher.BP_RocketLauncher_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/ShockRifle/ShockRifle.ShockRifle_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/Sniper/BP_Sniper.BP_Sniper_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/LightningRifle/BP_LightningRifle.BP_LightningRifle_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);
	WeaponRef = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/GrenadeLauncher/BP_GrenadeLauncher.BP_GrenadeLauncher_C"));
	AlwaysLoadedWeaponsStringRefs.Add(WeaponRef);

	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultLevelScreenshotObj(TEXT("Texture2D'/Game/RestrictedAssets/Textures/_T_NSA_2_D._T_NSA_2_D'"));
	DefaultLevelScreenshot = DefaultLevelScreenshotObj.Object;

	bFirstRun = true;
	bAllowClientNetProfile = false;
	ReadEULACaption = NSLOCTEXT("UTGameEngine", "ReadEULACaption", "READ ME FIRST");
	ReadEULAText = NSLOCTEXT("UTGameEngine", "ReadEULAText", "EULA TEXT");

	SmoothedDeltaTime = 0.01f;

	HitchTimeThreshold = 0.05f;
	HitchScaleThreshold = 2.f;
	HitchSmoothingRate = 0.5f;
	NormalSmoothingRate = 0.1f;
	MaximumSmoothedTime = 0.04f;

	ServerMaxPredictionPing = 120.f;
	VideoRecorder = nullptr;

#if !UE_SERVER
	ConstructorHelpers::FObjectFinder<UClass> TutorialMenuFinder(TEXT("/Game/RestrictedAssets/Tutorials/Blueprints/TutMainMenuWidget.TutMainMenuWidget_C"));
	TutorialMenuClass = TutorialMenuFinder.Object;
#endif
}

void UUTGameEngine::Init(IEngineLoop* InEngineLoop)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FCommandLine::Append(TEXT(" -ddc=noshared"));
#endif

	FString Commandline = FCommandLine::Get();
	if (!Commandline.Contains(TEXT("?game=lobby")))
	{
		for (auto WeaponClassRef : AlwaysLoadedWeaponsStringRefs)
		{
			AlwaysLoadedWeapons.Add(Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *WeaponClassRef.ToStringReference().ToString(), NULL, LOAD_NoWarn)));
		}
	}

	// workaround for engine bugs when loading classes that reference UMG on a dedicated server (i.e. mutators)
	FModuleManager::Get().LoadModule("Foliage");
	FModuleManager::Get().LoadModule("BlueprintContext");
	FModuleManager::Get().LoadModule("CinematicCamera");

	if (bFirstRun)
	{
		#if !UE_SERVER
		if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
		{
			// When the movie ends, ask if the user accepts the eula.  
			// Note we cannot prompt while the movie is playing due to threading issues
			GetMoviePlayer()->OnMoviePlaybackFinished().AddUObject(this, &UUTGameEngine::OnLoadingMoviePlaybackFinished);
		}
		else
			#endif
		{
			// If the movie has already ended or was never played, prompt for eula now.
			PromptForEULAAcceptance();
		}
	}

	IndexExpansionContent();

	FUTAnalytics::Initialize();
	if (FUTAnalytics::IsAvailable())
	{
		FUTAnalytics::FireEvent_ApplicationStart();
	}

	Super::Init(InEngineLoop);


	// HACK: UGameUserSettings::ApplyNonResolutionSettings() isn't virtual so we need to force our settings to be applied...
	GetGameUserSettings()->ApplySettings(true);

	UE_LOG(UT, Log, TEXT("Running %d processors (%d logical cores)"), FPlatformMisc::NumberOfCores(), FPlatformMisc::NumberOfCoresIncludingHyperthreads());
	if (FPlatformMisc::NumberOfCoresIncludingHyperthreads() < ParallelRendererProcessorRequirement)
	{
		UE_LOG(UT, Log, TEXT("Enabling r.RHICmdBypass due to not having %d logical cores"), ParallelRendererProcessorRequirement);
		IConsoleVariable* BypassVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHICmdBypass"));
		BypassVar->Set(1);
	}

	FParse::Value(FCommandLine::Get(), TEXT("ClientProcID="), OwningProcessID);

	if (!IsRunningDedicatedServer())
	{
		static const FName VideoRecordingFeatureName("VideoRecording");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
		{
			VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
		}
	}

#if !UE_SERVER
	ChallengeManager = NewObject<UUTChallengeManager>(GetTransientPackage(),UUTChallengeManager::StaticClass());
	ChallengeManager->AddToRoot();
#endif

	IConsoleVariable* TonemapperFilm = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TonemapperFilm"));
	if (TonemapperFilm != NULL)
	{
		TonemapperFilm->Set(0, ECVF_SetByCode);
	}

}

void UUTGameEngine::PreExit()
{

	if (FUTAnalytics::IsAvailable())
	{	
		FUTAnalytics::FireEvent_ApplicationStop();
	}

	Super::PreExit();
	FUTAnalytics::Shutdown();
}

// @TODO FIXMESTEVE - we want open to be relative like it used to be
bool UUTGameEngine::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld *InWorld)
{
	// the netcode adds implicit "?game=" to network URLs that we don't want when going from online game to local game
	FWorldContext* Context = GetWorldContextFromWorld(InWorld);
	if (Context != NULL && !Context->LastURL.IsLocalInternal())
	{
		Context->LastURL.RemoveOption(TEXT("game="));
	}
	return HandleTravelCommand(Cmd, Ar, InWorld);
}

bool UUTGameEngine::GetMonitorRefreshRate(int32& MonitorRefreshRate)
{
#if PLATFORM_WINDOWS && !UE_SERVER
	DEVMODE DeviceMode;
	FMemory::Memzero(DeviceMode);
	DeviceMode.dmSize = sizeof(DEVMODE);

	if (EnumDisplaySettings(NULL, -1, &DeviceMode) != 0)
	{
		MonitorRefreshRate = DeviceMode.dmDisplayFrequency;
		return true;
	}
#endif

	return false;
}

bool UUTGameEngine::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Out)
{
#if UE_BUILD_SHIPPING
	// make these debug commands illegal in shipping builds
	if (FParse::Command(&Cmd, TEXT("SET")) || FParse::Command(&Cmd, TEXT("SETNOPEC")) || 
		FParse::Command(&Cmd, TEXT("DISPLAYALL")) || FParse::Command(&Cmd, TEXT("DISPLAYALLLOCATION")) ||
		FParse::Command(&Cmd, TEXT("GET")) || FParse::Command(&Cmd, TEXT("GETALL")))
	{
		return true;
	}
#endif
	if (FParse::Command(&Cmd, TEXT("START")))
	{
		FWorldContext &WorldContext = GetWorldContextFromWorldChecked(InWorld);
		FURL TestURL(&WorldContext.LastURL, Cmd, TRAVEL_Absolute);
		// make sure the file exists if we are opening a local file
		if (TestURL.IsLocalInternal() && !MakeSureMapNameIsValid(TestURL.Map))
		{
			Out.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
			return true;
		}
		else
		{
			SetClientTravel(InWorld, Cmd, TRAVEL_Absolute);
			return true;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("GAMEVER")) || FParse::Command(&Cmd, TEXT("GAMEVERSION")))
	{
		FString VersionString = FString::Printf(TEXT("GameVersion %s Date: %s Time: %s"),
			BUILD_VERSION, TEXT(__DATE__), TEXT(__TIME__));

		Out.Logf(*VersionString);
		FPlatformMisc::ClipboardCopy(*VersionString);
		return true;
	}
	else if (FParse::Command(&Cmd, TEXT("MONITORREFRESH")))
	{
		FString OutputString;
#if PLATFORM_WINDOWS && !UE_SERVER

		int32 CurrentRefreshRate = 0;
		if (GetMonitorRefreshRate(CurrentRefreshRate))
		{
			OutputString = FString::Printf(TEXT("Monitor frequency is %dhz"), CurrentRefreshRate);
		}
		else
		{
			OutputString = TEXT("Could not detect monitor refresh frequency");
		}
		Out.Logf(*OutputString);

		int32 NewRefreshRate = FCString::Atoi(Cmd);
		if (NewRefreshRate > 0 && NewRefreshRate != CurrentRefreshRate)
		{
			DEVMODE NewSettings;
			FMemory::Memzero(NewSettings);
			NewSettings.dmSize = sizeof(NewSettings);
			NewSettings.dmDisplayFrequency = NewRefreshRate;
			NewSettings.dmFields = DM_DISPLAYFREQUENCY;
			LONG ChangeDisplayReturn = ChangeDisplaySettings(&NewSettings, CDS_GLOBAL | CDS_UPDATEREGISTRY);
			if (ChangeDisplayReturn == DISP_CHANGE_SUCCESSFUL)
			{
				OutputString = FString::Printf(TEXT("Monitor frequency was changed to %dhz"), NewSettings.dmDisplayFrequency);
			}
			else if (ChangeDisplayReturn == DISP_CHANGE_BADMODE)
			{
				OutputString = FString::Printf(TEXT("Your monitor does not support running at %dhz"), NewSettings.dmDisplayFrequency);
			}
			else
			{
				OutputString = TEXT("Error setting monitor refresh frequency");
			}
			Out.Logf(*OutputString);
		}
#else
		OutputString = TEXT("Could not detect monitor refresh frequency");
		Out.Logf(*OutputString);
#endif

		return true;
	}
	else
	{
		return Super::Exec(InWorld, Cmd, Out);
	}

}

void UUTGameEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	// HACK: make sure our default URL options are in all travel URLs since FURL code to do this was removed
	for (int32 WorldIdx = 0; WorldIdx < WorldList.Num(); ++WorldIdx)
	{
		FWorldContext& Context = WorldList[WorldIdx];
		if (!Context.TravelURL.IsEmpty())
		{
			UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetLocalPlayerFromControllerId(GWorld,0));
			if (UTLocalPlayer != NULL)
			{
				//Hide the console when traveling to a new map
				UUTConsole* UTConsole = (UTLocalPlayer->ViewportClient != nullptr) ? Cast<UUTConsole>(UTLocalPlayer->ViewportClient->ViewportConsole) : nullptr;
				if (UTConsole != nullptr)
				{
					UTConsole->ClearReopenMenus();
					UTConsole->FakeGotoState(NAME_None);
				}
			}
		}
	}
	
	Super::Tick(DeltaSeconds, bIdleMode);

	if (OwningProcessID != 0 && !FPlatformProcess::IsApplicationRunning(OwningProcessID))
	{
		FPlatformMisc::RequestExit(false);
	}

#if !UE_SERVER
	if (GWorld->GetNetMode() != NM_DedicatedServer)
	{
		TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetUserFocusedWidget(0);
		TSharedPtr<SViewport> Viewport = FSlateApplication::Get().GetGameViewport();
		if (Widget.IsValid() && Viewport.IsValid() && Widget->GetType() == Viewport->GetType())
		{
			UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetLocalPlayerFromControllerId(GWorld,0));
			if (UTLocalPlayer != NULL)
			{
				UTLocalPlayer->RegainFocus();
			}
		}
	}
#endif
}

EBrowseReturnVal::Type UUTGameEngine::Browse( FWorldContext& WorldContext, FURL URL, FString& Error )
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetLocalPlayerFromControllerId(WorldContext.World(),0));
	if (UTLocalPlayer != NULL)
	{
		FURL DefaultURL;
		DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
		FURL NewURL(&DefaultURL, *URL.ToString(), TRAVEL_Absolute);
		for (int32 i = 0; i < DefaultURL.Op.Num(); i++)
		{
			NewURL.AddOption(*DefaultURL.Op[i]);
		}

		if (NewURL.HasOption(TEXT("Rank")))
		{
			NewURL.RemoveOption(TEXT("Rank"));
		}

		NewURL.AddOption(*FString::Printf(TEXT("Rank=%i"),UTLocalPlayer->GetBaseELORank()));

		UUTProfileSettings* ProfileSettings = UTLocalPlayer->GetProfileSettings();
		if (ProfileSettings && ProfileSettings->bNeedProfileWriteOnLevelChange)
		{
			UTLocalPlayer->SaveProfileSettings();
		}

		UUTProgressionStorage* Storage = UTLocalPlayer->GetProgressionStorage();
		if (Storage && Storage->NeedsToBeUpdate())
		{
			UTLocalPlayer->SaveProgression();
		}

		// clear Team from URL when leaving server (go back to no preference)
		if (URL.IsLocalInternal())
		{
			UTLocalPlayer->SetDefaultURLOption(TEXT("Team"), FString::FromInt(255));
		}

		URL = NewURL;
	}

#if !UE_SERVER && !UE_EDITOR
	if (URL.Valid && URL.HasOption(TEXT("downloadfiles")))
	{
		WorldContext.TravelURL = TEXT("");

		// Need to download files, load default map
		if (WorldContext.PendingNetGame)
		{
			CancelPending(WorldContext);
		}
		// Handle failure URL.
		UE_LOG(LogNet, Log, TEXT("%s"), TEXT("Returning to Entry"));
		if (WorldContext.World() != NULL)
		{
			ResetLoaders(WorldContext.World()->GetOuter());
		}

		const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
		bool LoadSuccess = LoadMap(WorldContext, FURL(&URL, *(GameMapsSettings->GetGameDefaultMap() + GameMapsSettings->LocalMapOptions), TRAVEL_Partial), NULL, Error);
		if (LoadSuccess == false)
		{
			UE_LOG(LogNet, Fatal, TEXT("Failed to load default map (%s). Error: (%s)"), *(GameMapsSettings->GetGameDefaultMap() + GameMapsSettings->LocalMapOptions), *Error);
		}

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		WorldContext.LastURL = URL;
		// now remove "downloadfiles" options from LastURL so it doesn't get copied on to future URLs
		WorldContext.LastURL.RemoveOption(TEXT("downloadfiles"));
		
		BroadcastTravelFailure(WorldContext.World(), ETravelFailure::PackageMissing, TEXT("DownloadFiles"));

		return (LoadSuccess ? EBrowseReturnVal::Success : EBrowseReturnVal::Failure);
	}
#endif

	return Super::Browse(WorldContext, URL, Error);
}

static TAutoConsoleVariable<int32> CVarUnsteadyFPS(
	TEXT("ut.UnsteadyFPS"), 0,
	TEXT("Causes FPS to bounce around randomly in the 85-120 range."));

static TAutoConsoleVariable<int32> CVarSmoothFrameRate(
	TEXT("ut.SmoothFrameRate"), 1,
	TEXT("Enable frame rate smoothing."));

float UUTGameEngine::GetMaxTickRate(float DeltaTime, bool bAllowFrameRateSmoothing) const
{
	float MaxTickRate = 0;
	UWorld* World = NULL;

	for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
	{
		if (WorldList[WorldIndex].WorldType == EWorldType::Game)
		{
			World = WorldList[WorldIndex].World();
			break;
		}
	}

	// Don't smooth here if we're a dedicated server
	if (IsRunningDedicatedServer())
	{
		if (World)
		{
			UNetDriver* NetDriver = World->GetNetDriver();
			// In network games, limit framerate to not saturate bandwidth.
			if (NetDriver && (NetDriver->GetNetMode() == NM_DedicatedServer || (NetDriver->GetNetMode() == NM_ListenServer && NetDriver->bClampListenServerTickRate)))
			{
				// We're a dedicated server, use the LAN or Net tick rate.
				MaxTickRate = FMath::Clamp(NetDriver->NetServerMaxTickRate, 10, 120);

				// Allow hubs to override the tick rate
				AUTLobbyGameMode* LobbyGame = World->GetAuthGameMode<AUTLobbyGameMode>();
				if (LobbyGame)
				{
					MaxTickRate = LobbyGame->LobbyMaxTickRate;
				}
			}
		}
		return MaxTickRate;
	}

	if (VideoRecorder)
	{
		uint32 VideoRecorderTickRate = 0;
		if (VideoRecorder->IsRecording(VideoRecorderTickRate))
		{
			return VideoRecorderTickRate;
		}
	}

	if (CVarUnsteadyFPS.GetValueOnGameThread())
	{
		float RandDelta = 0.f;
		// random variation in frame time because of effects, etc.
		if (FMath::FRand() < 0.5f)
		{
			RandDelta = (1.f+FMath::FRand()) * DeltaTime;
		}
		// occasional large hitches
		if (FMath::FRand() < 0.002f)
		{
			RandDelta += 0.1f;
		}

		DeltaTime += RandDelta;
		MaxTickRate = 1.f / DeltaTime;
		//UE_LOG(UT, Warning, TEXT("FORCING UNSTEADY FRAME RATE desired delta %f adding %f maxtickrate %f"),  DeltaTime, RandDelta, MaxTickRate);
	}

	if (bSmoothFrameRate && bAllowFrameRateSmoothing && CVarSmoothFrameRate.GetValueOnGameThread())
	{
		MaxTickRate = 1.f / SmoothedDeltaTime;
	}

	// clamp frame rate based on negotiated net speed
	if (World)
	{
		UNetDriver* NetDriver = World->GetNetDriver();
		if (NetDriver && NetDriver->GetNetMode() == NM_Client)
		{
			UPlayer * LocalPlayer = World->GetFirstLocalPlayerFromController();
			if (LocalPlayer)
			{
				float NetRateClamp = float(LocalPlayer->CurrentNetSpeed) / 100.f;
				MaxTickRate = MaxTickRate > 0 ? FMath::Min(MaxTickRate, NetRateClamp) : NetRateClamp;
			}
		}
	}

	AUTMenuGameMode* MenuGame = Cast<AUTMenuGameMode>(World->GetAuthGameMode());
	if (MenuGame)
	{
		const float MaxMenuTickRate = 160.0f;
		if (MaxTickRate > 0)
		{
			MaxTickRate = FMath::Min(MaxMenuTickRate, MaxTickRate);
		}
		else
		{
			MaxTickRate = MaxMenuTickRate;
		}
	}

	// Hard cap at frame rate cap
	if (FrameRateCap > 0)
	{
		if (MaxTickRate > 0)
		{
			MaxTickRate = FMath::Min(FMath::Max(30.f, FrameRateCap), MaxTickRate);
		}
		else
		{
			MaxTickRate = FMath::Max(30.f, FrameRateCap);
		}
	}
	
	return MaxTickRate;
}

void UUTGameEngine::UpdateRunningAverageDeltaTime(float DeltaTime, bool bAllowFrameRateSmoothing)
{
	if (DeltaTime > SmoothedDeltaTime)
	{
		// can't slow down drop
		SmoothedDeltaTime = DeltaTime;
		// return 0.f; // @TODO FIXMESTEVE - not doing this so unsteady FPS is valid
	}
	else if (SmoothedDeltaTime > FMath::Max(HitchTimeThreshold, HitchScaleThreshold * DeltaTime))
	{
		// fast return from hitch - smoothing them makes game crappy a long time
		HitchSmoothingRate = FMath::Clamp(HitchSmoothingRate, 0.f, 1.f);
		SmoothedDeltaTime = (1.f - HitchSmoothingRate)*SmoothedDeltaTime + HitchSmoothingRate*DeltaTime;
	}
	else
	{
		// simply smooth the trajectory back up to limit mouse input variations because of difference in sampling frame time and this frame time
		NormalSmoothingRate = FMath::Clamp(NormalSmoothingRate, 0.f, 1.f);

		SmoothedDeltaTime = (1.f - NormalSmoothingRate)*SmoothedDeltaTime + NormalSmoothingRate*DeltaTime;
	}

	// Make sure that we don't try to smooth below a certain minimum
	SmoothedDeltaTime = FMath::Clamp(SmoothedDeltaTime, 0.001f, MaximumSmoothedTime);

	//UE_LOG(UT, Warning, TEXT("SMOOTHED TO %f"), SmoothedDeltaTime);
}

bool UUTGameEngine::CheckVersionOfPakFile(const FString& PakFilename) const
{
	bool bValidPak = false;

	if (!PakFilename.StartsWith(TEXT("UnrealTournament-"), ESearchCase::IgnoreCase))
	{
		int32 DashPosition = PakFilename.Find(TEXT("-"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (DashPosition != -1)
		{
			FString DashlessPakFilename = PakFilename.Left(DashPosition);

			FString VersionFilename = DashlessPakFilename + TEXT("-version.txt");
			FString VersionString;
			if (FFileHelper::LoadFileToString(VersionString, *(FPaths::GameDir() / VersionFilename)))
			{
				VersionString = VersionString.LeftChop(2);
				FNetworkVersion::bHasCachedNetworkChecksum = false;
				FString CompiledVersionString = FString::FromInt(FNetworkVersion::GetLocalNetworkVersion());

				if (VersionString == CompiledVersionString)
				{
					bValidPak = true;
				}
				else
				{
					UE_LOG(UT, Warning, TEXT("%s is version %s, but needs to be version %s"), *DashlessPakFilename, *VersionString, *CompiledVersionString);
				}
			}
			else
			{
				UE_LOG(UT, Warning, TEXT("%s had no version file"), *DashlessPakFilename);
			}
		}
	}
	else
	{
		// Assume the stock pak is good
		bValidPak = true;
	}

	return bValidPak;
}

void UUTGameEngine::AddAssetRegistry(const FString& PakFilename)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	int32 DashPosition = PakFilename.Find(TEXT("-"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (DashPosition != -1)
	{
		FString DashlessPakFilename = PakFilename.Left(DashPosition);
		FString AssetRegistryName = DashlessPakFilename + TEXT("-AssetRegistry.bin");
		FArrayReader SerializedAssetData;
		if (FFileHelper::LoadFileToArray(SerializedAssetData, *(FPaths::GameDir() / AssetRegistryName)))
		{
			// serialize the data with the memory reader (will convert FStrings to FNames, etc)
			AssetRegistryModule.Get().Serialize(SerializedAssetData);
			UE_LOG(UT, Log, TEXT("%s merged into the asset registry"), *AssetRegistryName);
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("%s could not be found"), *AssetRegistryName);
		}
	}
}

void UUTGameEngine::IndexExpansionContent()
{
	double IndexExpansionContentStartTime = FPlatformTime::Seconds();

	// Plugin manager should handle this instead of us, but we're not using plugin-based dlc just yet
	if (FPlatformProperties::RequiresCookedData())
	{
		// Helper class to find all pak files.
		class FPakFileSearchVisitor : public IPlatformFile::FDirectoryVisitor
		{
			TArray<FString>& FoundFiles;
		public:
			FPakFileSearchVisitor(TArray<FString>& InFoundFiles)
				: FoundFiles(InFoundFiles)
			{}
			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
			{
				if (bIsDirectory == false)
				{
					FString Filename(FilenameOrDirectory);
					if (Filename.MatchesWildcard(TEXT("*.pak")))
					{
						FoundFiles.Add(Filename);
					}
				}
				return true;
			}
		};
		
		// Search for pak files that were downloaded through redirects
		const bool bAltPaks = FParse::Param(FCommandLine::Get(), TEXT("altpaks"));
		TArray<FString>	FoundPaks;
		FPakFileSearchVisitor PakVisitor(FoundPaks);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FoundPaks.Empty();

		FString PakFolder = bAltPaks
			? FPaths::Combine(FPlatformProcess::UserDir(), FApp::GetGameName(), TEXT("Saved"), TEXT("Paks"), TEXT("DownloadedPaks"))
			: FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"));

		PlatformFile.IterateDirectoryRecursively(*PakFolder, PakVisitor);
		for (const auto& PakPath : FoundPaks)
		{
			FString PakFilename = FPaths::GetBaseFilename(PakPath);

			//Log user activity so Crash Reporter knows if we crash during PakLoading
			FUserActivityTracking::SetActivity(FUserActivity(FString::Printf(TEXT("LoadingCustomContentPAK:_%s"),*PakFilename)));

			// If dedicated server, mount the pak
			if (bAltPaks && FCoreDelegates::OnMountPak.IsBound())
			{
				FCoreDelegates::OnMountPak.Execute(PakPath, 0, nullptr);
			}

			bool bValidPak = CheckVersionOfPakFile(PakFilename);

			if (bValidPak)
			{
				TArray<uint8> Data;
				if (FFileHelper::LoadFileToArray(Data, *PakPath))
				{
					FString MD5 = MD5Sum(Data);
					DownloadedContentChecksums.Add(PakFilename, MD5);
					MountedDownloadedContentChecksums.Add(PakFilename, MD5);
				}

				AddAssetRegistry(PakFilename);
			}
			else
			{
				// Unmount the pak
				if (FCoreDelegates::OnUnmountPak.IsBound())
				{
					FCoreDelegates::OnUnmountPak.Execute(PakPath);
					UE_LOG(UT, Warning, TEXT("Unmounted %s"), *PakPath);
				}
				PaksThatFailedToLoad.Add(PakFilename);
			}
		}

		// Only add MyContent pak files to LocalContentChecksums
		FoundPaks.Empty();
		PakFolder = bAltPaks
			? FPaths::Combine(FPlatformProcess::UserDir(), FApp::GetGameName(), TEXT("Saved"), TEXT("Paks"), TEXT("MyContent"))
			: FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("MyContent"));
		PlatformFile.IterateDirectoryRecursively(*PakFolder, PakVisitor);
		for (const auto& PakPath : FoundPaks)
		{
			FString PakFilename = FPaths::GetBaseFilename(PakPath);

			// If dedicated server, mount the pak
			if (bAltPaks && FCoreDelegates::OnMountPak.IsBound())
			{
				FCoreDelegates::OnMountPak.Execute(PakPath, 0, nullptr);
			}
			
			bool bValidPak = CheckVersionOfPakFile(PakFilename);

			if (bValidPak)
			{
				TArray<uint8> Data;
				if (FFileHelper::LoadFileToArray(Data, *PakPath))
				{
					FString MD5 = MD5Sum(Data);
					LocalContentChecksums.Add(PakFilename, MD5);
				}

				AddAssetRegistry(PakFilename);
			}
			else
			{
				// Unmount the pak
				if (FCoreDelegates::OnUnmountPak.IsBound())
				{
					FCoreDelegates::OnUnmountPak.Execute(PakPath);
					UE_LOG(UT, Warning, TEXT("Unmounted %s"), *PakPath);
				}
				PaksThatFailedToLoad.Add(PakFilename);
			}
		}

		// Load asset registries from pak files that aren't the default one
		FoundPaks.Empty();
		PlatformFile.IterateDirectoryRecursively(*FPaths::Combine(*FPaths::GameContentDir(), TEXT("Paks")), PakVisitor);
		for (const auto& PakPath : FoundPaks)
		{
			bool bValidPak = false;

			FString PakFilename = FPaths::GetBaseFilename(PakPath);
			if (!PakFilename.StartsWith(TEXT("UnrealTournament-"), ESearchCase::IgnoreCase))
			{
				bValidPak = CheckVersionOfPakFile(PakFilename);
				if (bValidPak)
				{
					// TODO: temporary workaround until we have smarter checksum handling for hub instances is to use the .ini value without checking if it is set
					AUTBaseGameMode* DefGame = GetMutableDefault<AUTBaseGameMode>();
					FPackageRedirectReference Redirect;
					if (DefGame->FindRedirect(PakFilename, Redirect) && !Redirect.PackageChecksum.IsEmpty())
					{
						DownloadedContentChecksums.Add(PakFilename, Redirect.PackageChecksum);
						MountedDownloadedContentChecksums.Add(PakFilename, Redirect.PackageChecksum);
					}
					else
					{
						TArray<uint8> Data;
						if (FFileHelper::LoadFileToArray(Data, *PakPath))
						{
							FString MD5 = MD5Sum(Data);
							DownloadedContentChecksums.Add(PakFilename, MD5);
							MountedDownloadedContentChecksums.Add(PakFilename, MD5);
						}
					}
					AddAssetRegistry(PakFilename);
				}
				else
				{
					PaksThatFailedToLoad.Add(PakFilename);
				}
			}
			else
			{
				// Assume the stock pak is good
				bValidPak = true;
			}

			if (!bValidPak)
			{
				// Unmount the pak
				if (FCoreDelegates::OnUnmountPak.IsBound())
				{
					FCoreDelegates::OnUnmountPak.Execute(PakPath);
					UE_LOG(UT, Warning, TEXT("Unmounted %s"), *PakPath);
				}
			}
		}
	}
	
	double IndexExpansionContentDuration = FPlatformTime::Seconds() - IndexExpansionContentStartTime;
	UE_LOG(LogInit, Display, TEXT("IndexExpansionContent took %f seconds"), IndexExpansionContentDuration);
}

void UUTGameEngine::SetupLoadingScreen()
{
#if !UE_SERVER
/*
	if (IsMoviePlayerEnabled())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		// TODO: probably need to do something to handle aspect ratio
		//LoadingScreen.WidgetLoadingScreen = SNew(SImage).Image(&LoadingScreenImage);
		LoadingScreen.WidgetLoadingScreen = SNew(SImage).Image(SUWindowsStyle::Get().GetBrush("LoadingScreen"));
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		FCoreUObjectDelegates::PreLoadMap.Broadcast(TEXT(""));
	}
*/
#endif
}

static FName UT_DEFAULT_LOADING(TEXT("UT.LoadingScreen"));


bool UUTGameEngine::IsCloudAndLocalContentInSync()
{
	bool bFoundFileInCloud = false;

	for (auto LocalIt = LocalContentChecksums.CreateConstIterator(); LocalIt; ++LocalIt)
	{
		bFoundFileInCloud = false;
		for (auto CloudIt = CloudContentChecksums.CreateConstIterator(); CloudIt; ++CloudIt)
		{
			if (CloudIt.Key() == LocalIt.Key())
			{
				if (CloudIt.Value() == LocalIt.Value())
				{
					bFoundFileInCloud = true;
					UE_LOG(UT, Log, TEXT("%s is properly synced in the cloud"), *LocalIt.Key());
				}
				else
				{
					UE_LOG(UT, Log, TEXT("%s is on the cloud but out of date"), *LocalIt.Key());
				}
			}
		}

		if (!bFoundFileInCloud)
		{
			UE_LOG(UT, Log, TEXT("%s is not synced on the cloud"), *LocalIt.Key());
			return false;
		}
	}

	return true;
}

FString UUTGameEngine::MD5Sum(const TArray<uint8>& Data)
{
	uint8 Digest[16];

	FMD5 Md5Gen;

	Md5Gen.Update((uint8*)Data.GetData(), Data.Num());
	Md5Gen.Final(Digest);

	FString MD5;
	for (int32 i = 0; i < 16; i++)
	{
		MD5 += FString::Printf(TEXT("%02x"), Digest[i]);
	}

	return MD5;
}

void UUTGameEngine::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// ignore connection loss on game net driver if we have a pending one; this happens during standard blocking server travel
	if (NetDriver == nullptr || NetDriver->NetDriverName != NAME_GameNetDriver || GEngine->GetWorldContextFromWorld(World)->PendingNetGame == NULL ||
		(FailureType != ENetworkFailure::ConnectionLost && FailureType != ENetworkFailure::FailureReceived) )
	{
		Super::HandleNetworkFailure(World, NetDriver, FailureType, ErrorString);
	}
}

bool UUTGameEngine::ShouldShutdownWorldNetDriver()
{
	return false;
}

void UUTGameEngine::OnLoadingMoviePlaybackFinished()
{
	PromptForEULAAcceptance();
}

void UUTGameEngine::PromptForEULAAcceptance()
{
#if PLATFORM_DESKTOP
	if (bFirstRun)
	{
		FString PasswordStr;
		FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), PasswordStr);

		// PasswordStr will be empty if we were not run from the launcher
		if (PasswordStr.IsEmpty())
		{
#if !UE_SERVER
			if (FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo, *ReadEULAText.ToString(), *ReadEULACaption.ToString()) != EAppReturnType::Yes)
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
#else
			UE_LOG(LogExit, Warning, TEXT("Please accept the EULA before running the dedicated server!"));
			FPlatformMisc::RequestExit(false);
#endif
		}

		bFirstRun = false;
		SaveConfig();
		GConfig->Flush(false);
	}
#endif
}

UUTLevelSummary* UUTGameEngine::LoadLevelSummary(const FString& MapName)
{
	UUTLevelSummary* Summary = NULL;
	FString MapFullName;
	// querying the asset registry and iterating its results is actually faster than SearchForPackageOnDisk() assuming the registry has been initialized already
	TArray<FAssetData> AssetList;
	GetAllAssetData(UWorld::StaticClass(), AssetList, false);
	FName MapFName(*MapName);
	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetName == MapFName)
		{
			MapFullName = Asset.PackageName.ToString();
			break;
		}
	}
	if (MapFullName.Len() > 0 || FPackageName::SearchForPackageOnDisk(MapName + FPackageName::GetMapPackageExtension(), &MapFullName))
	{
		static FName NAME_LevelSummary(TEXT("LevelSummary"));

		UPackage* Pkg = CreatePackage(NULL, *MapFullName);
		Summary = FindObject<UUTLevelSummary>(Pkg, *NAME_LevelSummary.ToString());
		if (Summary == NULL)
		{
			// LoadObject() actually forces the whole package to be loaded for some reason so we need to take the long way around
			BeginLoad();
			FLinkerLoad* Linker = GetPackageLinker(Pkg, NULL, LOAD_NoWarn | LOAD_Quiet, NULL, NULL);
			if (Linker != NULL)
			{
				//UUTLevelSummary* Summary = Cast<UUTLevelSummary>(Linker->Create(UUTLevelSummary::StaticClass(), FName(TEXT("LevelSummary")), Pkg, LOAD_NoWarn | LOAD_Quiet, false));
				// ULinkerLoad::Create() not exported... even more hard way :(
				FPackageIndex SummaryClassIndex, SummaryPackageIndex;
				if (Linker->FindImportClassAndPackage(FName(TEXT("UTLevelSummary")), SummaryClassIndex, SummaryPackageIndex) && SummaryPackageIndex.IsImport() && Linker->ImportMap[SummaryPackageIndex.ToImport()].ObjectName == AUTGameMode::StaticClass()->GetOutermost()->GetFName())
				{
					for (int32 Index = 0; Index < Linker->ExportMap.Num(); Index++)
					{
						FObjectExport& Export = Linker->ExportMap[Index];
						if (Export.ObjectName == NAME_LevelSummary && Export.ClassIndex == SummaryClassIndex)
						{
							Export.Object = NewObject<UUTLevelSummary>(Linker->LinkerRoot, UUTLevelSummary::StaticClass(), Export.ObjectName, EObjectFlags(Export.ObjectFlags | RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects | RF_WasLoaded));
							Export.Object->SetLinker(Linker, Index);
							//GObjLoaded.Add(Export.Object);
							Linker->Preload(Export.Object);
							Export.Object->ConditionalPostLoad();
							Summary = Cast<UUTLevelSummary>(Export.Object);
							break;
						}
					}
				}
			}
			EndLoad();
		}
	}
	return Summary;
}

void UUTGameEngine::InitializeObjectReferences()
{
	Super::InitializeObjectReferences();

	//Load all the flags from the config
	TArray<FString> FlagClassNames;
	GConfig->GetPerObjectConfigSections(GetConfigFilename(UUTFlagInfo::StaticClass()->GetDefaultObject()), TEXT("UTFlagInfo"), FlagClassNames);

	for (FString& Class : FlagClassNames)
	{
		UUTFlagInfo* NewFlag = NewObject<UUTFlagInfo>(this, FName(*Class.Replace(TEXT(" UTFlagInfo"), TEXT(""))));
		if (NewFlag != nullptr)
		{
			FlagList.Add(NewFlag->GetFName(), NewFlag);
		}
	}
}

UUTFlagInfo* UUTGameEngine::GetFlag(FName FlagName)
{
	static const FName DefaultFlag(TEXT("Unreal"));
	if (FlagName == NAME_None)
	{
		FlagName = DefaultFlag;
	}
	UUTFlagInfo** Flag = FlagList.Find(FlagName);

	if (Flag != nullptr)
	{
		return *Flag;
	}
	UE_LOG(UT, Warning, TEXT("UUTGameEngine::GetFlag() Couldn't find flag for '%s'"), *FlagName.ToString());
	return nullptr;
}

bool UUTGameEngine::HandleReconnectCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld *InWorld )
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetLocalPlayerFromControllerId(InWorld,0));
	if (UTLocalPlayer)
	{
		UTLocalPlayer->Reconnect(false);
		return true;
	}
	else
	{
		return Super::HandleReconnectCommand(Cmd, Ar, InWorld );
	}
}

UUTBotCharacter* UUTGameEngine::FindBotAsset(const FString& BotName)
{
	if (BotAssets.Num() == 0)
	{
		GetAllAssetData(UUTBotCharacter::StaticClass(), BotAssets);
	}

	UUTBotCharacter* BotData = NULL;
	for (const FAssetData& Asset : BotAssets)
	{
		if (Asset.AssetName.ToString() == BotName)
		{
			BotData = Cast<UUTBotCharacter>(Asset.GetAsset());
			if (BotData != NULL)
			{
				break;
			}
		}
	}
	return BotData;
}

