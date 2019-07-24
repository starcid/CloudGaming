// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTVideoRecordingFeature.h"
#include "UTChallengeManager.h"
#include "../../Engine/Source/Runtime/AssetRegistry/Public/AssetData.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineTitleFileInterface.h"
#include "UTGameEngine.generated.h"


UCLASS()
class UNREALTOURNAMENT_API UUTGameEngine : public UGameEngine
{
	GENERATED_UCLASS_BODY()

private:
	TArray< TAssetSubclassOf<AUTWeapon> > AlwaysLoadedWeaponsStringRefs;

	/** used to have the standard weapon set always loaded for UI performance and load times */
	UPROPERTY()
	TArray< TSubclassOf<AUTWeapon> > AlwaysLoadedWeapons;

public:
	/** cached list of UTBotCharacter assets from the asset registry, so we don't need to query the registry every time we add a bot */
	TArray<FAssetData> BotAssets;

	virtual class UUTBotCharacter* FindBotAsset(const FString& BotName);

	/** default screenshot used for levels when none provided in the level itself */
	UPROPERTY()
	UTexture2D* DefaultLevelScreenshot;

	/** tutorial menu class, here for cooking */
	UPROPERTY()
	TSubclassOf<class UUserWidget> TutorialMenuClass;

	UPROPERTY()
	FText ReadEULACaption;
	UPROPERTY()
	FText ReadEULAText;

	/** used to display EULA info on first run */
	UPROPERTY(globalconfig)
	bool bFirstRun;

	UPROPERTY(config)
	int32 ParallelRendererProcessorRequirement;

	//==================================
	// Frame Rate Smoothing
	
	/** Current smoothed delta time. */
	UPROPERTY()
	float SmoothedDeltaTime;

	/** Frame time (in seconds) longer than this is considered a hitch. */
	UPROPERTY(config)
	float HitchTimeThreshold;

	/** Frame time longer than SmoothedDeltaTime*HitchScaleThreshold is considered a hitch. */
	UPROPERTY(config)
	float HitchScaleThreshold;

	/** How fast to smooth up from a hitch frame. */
	UPROPERTY(config)
	float HitchSmoothingRate;

	/** How fast to smooth between normal frames. */
	UPROPERTY(config)
	float NormalSmoothingRate;

	/** Never return a smoothed time larger than this. */
	UPROPERTY(config)
	float MaximumSmoothedTime;
	
	//==================================

	/* Set true to allow clients to toggle netprofiling using the NP console command. @TODO FIXMESTEVE temp until we have adminlogin/admin server console command executing */
	UPROPERTY(config)
	bool bAllowClientNetProfile;

	/* Frame rate cap */
	UPROPERTY(config)
	float FrameRateCap;

	UPROPERTY(config)
	FString RconPassword;

	/** Max prediction ping (used when negotiating with clients) */
	float ServerMaxPredictionPing;

	/** set to process ID of owning game client when running a "listen" server (which is really dedicated + client on same machine) */
	uint32 OwningProcessID;
	
	UTVideoRecordingFeature* VideoRecorder;

	TMap<FString, FString> DownloadedContentChecksums;
	TMap<FString, FString> MountedDownloadedContentChecksums;
	TMap<FString, FString> LocalContentChecksums;
	TMap<FString, FString> CloudContentChecksums;

	FString ContentDownloadCloudId;
	TMap<FString, FString> FilesToDownload;

	virtual void Init(IEngineLoop* InEngineLoop);
	virtual void PreExit();
	virtual bool HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld *InWorld) override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Out = *GLog) override;
	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;
	virtual float GetMaxTickRate(float DeltaTime, bool bAllowFrameRateSmoothing) const override;
	virtual void UpdateRunningAverageDeltaTime(float DeltaTime, bool bAllowFrameRateSmoothing = true) override;
	virtual void IndexExpansionContent();
	virtual void AddAssetRegistry(const FString& PakFilename);

	// return whether the given pak with the given checksum is among the downloaded content list
	// can pass in empty string for checksum to match by filename only
	bool HasContentWithChecksum(const FString& PakBaseFilename, const FString& Checksum) const
	{
		for (int32 i = 0; i < 2; i++)
		{
			const FString* FoundChecksum = ((i == 0) ? DownloadedContentChecksums : MountedDownloadedContentChecksums).Find(PakBaseFilename);
			if (FoundChecksum != NULL && (Checksum.IsEmpty() || *FoundChecksum == Checksum))
			{
				return true;
			}
		}
		return false;
	}

	virtual EBrowseReturnVal::Type Browse(FWorldContext& WorldContext, FURL URL, FString& Error) override;

	FString MD5Sum(const TArray<uint8>& Data);
	bool IsCloudAndLocalContentInSync();

	virtual void SetupLoadingScreen();
#define UT_LOADING_SCREEN_HOOK SetupLoadingScreen();
#if CPP
#include "UTLoadMap.h"
#endif
	UT_LOADMAP_DEFINITION()
#undef UT_LOADING_SCREEN_HOOK

	virtual void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;

	/** load the level summary out of a map package */
	static class UUTLevelSummary* LoadLevelSummary(const FString& MapName);

	bool GetMonitorRefreshRate(int32& MonitorRefreshRate);

	bool CheckVersionOfPakFile(const FString& PakFilename) const;

	UPROPERTY()
	TArray<FString>	PaksThatFailedToLoad;

protected:
	virtual bool ShouldShutdownWorldNetDriver() override;
	void OnLoadingMoviePlaybackFinished();
	void PromptForEULAAcceptance();

public:
	static FText ConvertTime(FText Prefix, FText Suffix, int32 Seconds, bool bForceHours = true, bool bForceMinutes = true, bool bForceTwoDigits = true)
	{
		int32 Hours = Seconds / 3600;
		Seconds -= Hours * 3600;
		int32 Mins = Seconds / 60;
		Seconds -= Mins * 60;
		bool bDisplayHours = bForceHours || Hours > 0;
		bool bDisplayMinutes = bDisplayHours || bForceMinutes || Mins > 0;

		FFormatNamedArguments Args;
		FNumberFormattingOptions Options;

		Options.MinimumIntegralDigits = 2;
		Options.MaximumIntegralDigits = 2;

		Args.Add(TEXT("Hours"), FText::AsNumber(Hours, bForceTwoDigits ? &Options : NULL));
		Args.Add(TEXT("Minutes"), FText::AsNumber(Mins, (bDisplayHours || bForceTwoDigits) ? &Options : NULL));
		Args.Add(TEXT("Seconds"), FText::AsNumber(Seconds, (bDisplayMinutes || bForceTwoDigits) ? &Options : NULL));
		Args.Add(TEXT("Prefix"), Prefix);
		Args.Add(TEXT("Suffix"), Suffix);

		if (bDisplayHours)
		{
			return FText::Format(NSLOCTEXT("UTGameEngine", "ConvertTimeHours", "{Prefix}{Hours}:{Minutes}:{Seconds}{Suffix}"), Args);
		}
		else if (bDisplayMinutes)
		{
			return FText::Format(NSLOCTEXT("UTGameEngine", "ConvertTimeMinutes", "{Prefix}{Minutes}:{Seconds}{Suffix}"), Args);
		}
		else
		{
			return FText::Format(NSLOCTEXT("UTGameEngine", "ConvertTime", "{Prefix}{Seconds}{Suffix}"), Args);
		}
	
	}

public:

	UPROPERTY()
	TMap<FName, class UUTFlagInfo*> FlagList;

	class UUTFlagInfo* GetFlag(FName FlagName);

	virtual void InitializeObjectReferences() override;


protected:

	// Holds the reference to this challenge manager.
	UUTChallengeManager* ChallengeManager;

public:

	// returns the Challenge manager.
	TWeakObjectPtr<UUTChallengeManager> GetChallengeManager()
	{
		return ChallengeManager;
	}

	bool HandleReconnectCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld *InWorld );

	// Users in this array are not saved.  It's used when kicking a player from an instance.  They don't get to come back.
	UPROPERTY()
	TArray<FBanInfo> InstanceBannedUsers;

	// Holds a list of the unique ids of the non-idle players in the last match
	UPROPERTY()
	TArray<FString> PlayerReservations;

};

