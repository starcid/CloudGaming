// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTGameViewportClient.h"
#include "Dialogs/SUTMessageBoxDialog.h"
#include "Base/SUTDialogBase.h"
#include "Dialogs/SUTInputBoxDialog.h"
#include "SUTGameLayerManager.h"
#include "Engine/GameInstance.h"
#include "UTGameEngine.h"
#include "Engine/Console.h"
#include "UTLocalPlayer.h"
#include "PartyContext.h"
#include "BlueprintContextLibrary.h"
#include "UTOnlineGameSettingsBase.h"

UUTGameViewportClient::UUTGameViewportClient(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReconnectAfterDownloadingMapDelay = 0;
	VerifyFilesToDownloadAndReconnectDelay = 0;
	MaxSplitscreenPlayers = 6;
	bReconnectAtNextTick = false;
	bReconnectAtNextTickNeedSpectator = false;

	SplitscreenInfo.SetNum(10); // we are hijacking entries 8 and 9 for 5 and 6 players
	
	SplitscreenInfo[8].PlayerData.Add(FPerPlayerSplitscreenData(0.33f, 0.5f, 0.0f, 0.0f));
	SplitscreenInfo[8].PlayerData.Add(FPerPlayerSplitscreenData(0.33f, 0.5f, 0.33f, 0.0f));
	SplitscreenInfo[8].PlayerData.Add(FPerPlayerSplitscreenData(0.33f, 0.5f, 0.0f, 0.5f));
	SplitscreenInfo[8].PlayerData.Add(FPerPlayerSplitscreenData(0.33f, 0.5f, 0.33f, 0.5f));
	SplitscreenInfo[8].PlayerData.Add(FPerPlayerSplitscreenData(0.33f, 1.0f, 0.66f, 0.0f));

	const float OneThird = 1.0f / 3.0f;
	const float TwoThirds = 2.0f / 3.0f;
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, 0.0f, 0.0f));
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, OneThird, 0.0f));
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, 0.0f, 0.5f));
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, OneThird, 0.5f));
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, TwoThirds, 0.0f));
	SplitscreenInfo[9].PlayerData.Add(FPerPlayerSplitscreenData(OneThird, 0.5f, TwoThirds, 0.5f));
	KickReason = FText::GetEmpty();
}

void UUTGameViewportClient::AddViewportWidgetContent(TSharedRef<class SWidget> ViewportContent, const int32 ZOrder)
{
#if !UE_SERVER
	if ( !LayerManagerPtr.IsValid() )
	{
		TSharedRef<SUTGameLayerManager> LayerManager = SNew(SUTGameLayerManager);
		Super::AddViewportWidgetContent(LayerManager);

		LayerManagerPtr = LayerManager;
	}

	if (LayerManagerPtr.IsValid())
	{
		LayerManagerPtr.Pin()->AddLayer(ViewportContent, ZOrder);
	}
#endif
}

void UUTGameViewportClient::RemoveViewportWidgetContent(TSharedRef<class SWidget> ViewportContent)
{
#if !UE_SERVER
	if ( LayerManagerPtr.IsValid() )
	{
		LayerManagerPtr.Pin()->RemoveLayer(ViewportContent);
	}
#endif
}

void UUTGameViewportClient::AddViewportWidgetContent_NoAspect(TSharedRef<class SWidget> ViewportContent, const int32 ZOrder)
{
#if !UE_SERVER
	if (!LayerManagerPtr.IsValid())
	{
		TSharedRef<SUTGameLayerManager> LayerManager = SNew(SUTGameLayerManager);
		Super::AddViewportWidgetContent(LayerManager);

		LayerManagerPtr = LayerManager;
	}

	if (LayerManagerPtr.IsValid())
	{
		LayerManagerPtr.Pin()->AddLayer_NoAspect(ViewportContent, ZOrder);
	}
#endif
}

void UUTGameViewportClient::RemoveViewportWidgetContent_NoAspect(TSharedRef<class SWidget> ViewportContent)
{
#if !UE_SERVER
	if (LayerManagerPtr.IsValid())
	{
		LayerManagerPtr.Pin()->RemoveLayer_NoAspect(ViewportContent);
	}
#endif
}

void UUTGameViewportClient::PeekTravelFailureMessages(UWorld* InWorld, enum ETravelFailure::Type FailureType, const FString& ErrorString)
{
	Super::PeekTravelFailureMessages(InWorld, FailureType, ErrorString);
#if !UE_SERVER
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);

	if (FailureType == ETravelFailure::PackageMissing)
	{
#if !PLATFORM_MAC
		// Missing map
		if (!ErrorString.IsEmpty() && ErrorString != TEXT("DownloadFiles"))
		{
			bool bAlreadyDownloaded = false;
			int32 SpaceIndex;
			FString URL;
			FString Checksum = TEXT("");
			if (ErrorString.FindChar(TEXT(' '), SpaceIndex))
			{
				URL = ErrorString.Left(SpaceIndex);

				Checksum = ErrorString.RightChop(SpaceIndex + 1);
				FString BaseFilename = FPaths::GetBaseFilename(URL);

				// If it already exists with the correct checksum, just mount it again
				if (UTEngine && UTEngine->DownloadedContentChecksums.Contains(BaseFilename))
				{
					FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"), *BaseFilename) + TEXT(".pak");
					if (UTEngine->DownloadedContentChecksums[BaseFilename] == Checksum)
					{
						if (FCoreDelegates::OnMountPak.IsBound())
						{
							FCoreDelegates::OnMountPak.Execute(Path, 0, nullptr);
							UTEngine->MountedDownloadedContentChecksums.Add(BaseFilename, Checksum);
							bAlreadyDownloaded = true;
						}
					}
					else
					{
						// Delete the original file
						FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
					}
				}
			}
			else
			{
				URL = ErrorString;
			}

			if (GEngine && GEngine->PendingNetGameFromWorld(World))
			{
				LastAttemptedURL = GEngine->PendingNetGameFromWorld(World)->URL;
			}

			if (bAlreadyDownloaded)
			{
				ReconnectAfterDownloadingMapDelay = 0.5f;
				return;
			}
			else if (FPaths::GetExtension(URL) == FString(TEXT("pak")))
			{
				DownloadRedirect(URL,TEXT(""), Checksum);
				return;
			}
		}

		bool bMountedPreviousDownload = false;

		if (UTEngine &&	!UTEngine->ContentDownloadCloudId.IsEmpty() && UTEngine->FilesToDownload.Num() > 0)
		{
			LastAttemptedURL = UTEngine->LastURLFromWorld(World);

			TArray<FString> FileURLs;

			for (auto It = UTEngine->FilesToDownload.CreateConstIterator(); It; ++It)
			{
				bool bNeedsToDownload = true;

				if (UTEngine->MountedDownloadedContentChecksums.Contains(It.Key()))
				{
					FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"), *It.Key()) + TEXT(".pak");

					// Unmount the pak
					if (FCoreDelegates::OnUnmountPak.IsBound())
					{
						FCoreDelegates::OnUnmountPak.Execute(Path);
					}

					// Remove the CRC entry
					UTEngine->MountedDownloadedContentChecksums.Remove(It.Key());
					UTEngine->DownloadedContentChecksums.Remove(It.Key());

					// Delete the original file
					FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
				}
				else if (UTEngine->DownloadedContentChecksums.Contains(It.Key()))
				{
					if (UTEngine->DownloadedContentChecksums[It.Key()] == It.Value())
					{
						FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"), *It.Key()) + TEXT(".pak");

						if (FCoreDelegates::OnMountPak.IsBound())
						{
							FCoreDelegates::OnMountPak.Execute(Path, 0, nullptr);
							UTEngine->MountedDownloadedContentChecksums.Add(It.Key(), It.Value());
							bNeedsToDownload = false;
							bMountedPreviousDownload = true;
						}
					}
				}

				if (bNeedsToDownload)
				{

					FString BaseURL = GetBackendBaseUrl();
					FString CommandURL = TEXT("/api/cloudstorage/user/");

					FileURLs.Add(BaseURL + CommandURL + UTEngine->ContentDownloadCloudId + TEXT("/") + It.Key() + TEXT(".pak"));
				}
			}

			if (FileURLs.Num() > 0)
			{

				for (int32 i = 0; i < FileURLs.Num(); i++)
				{
					DownloadRedirect(FileURLs[i]);
				}
			}
			else if (bMountedPreviousDownload)
			{
				// Assume we mounted all the files that we needed
				VerifyFilesToDownloadAndReconnectDelay = 0.5f;
			}

			return;
		}
#endif
	}

	FText NetworkErrorMessage;

	switch (FailureType)
	{
		case ETravelFailure::PackageMissing: NetworkErrorMessage = NSLOCTEXT("UTGameViewportClient", "TravelErrors_PackageMissing", "We were unable to find all the content necessary to join this server."); break;

		default: NetworkErrorMessage = FText::FromString(ErrorString);
	}

	if (ReconnectDialog.IsValid())
	{
		FirstPlayer->CloseDialog(ReconnectDialog.ToSharedRef());
	}
	if (FailureType == ETravelFailure::ServerTravelFailure || FailureType == ETravelFailure::ClientTravelFailure)
	{
		ReconnectDialog = FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "TravelErrorDialogTitle", "Error"), NetworkErrorMessage, UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
	}
	else
	{
		ReconnectDialog = FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "NetworkErrorDialogTitle", "Network Error"), NetworkErrorMessage, UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_RECONNECT, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
	}

#endif
}

void UUTGameViewportClient::PeekNetworkFailureMessages(UWorld *InWorld, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	static FName NAME_PendingNetDriver(TEXT("PendingNetDriver"));

	// ignore connection loss on game net driver if we have a pending one; this happens during standard blocking server travel
	if ( NetDriver->NetDriverName == NAME_GameNetDriver && GEngine->GetWorldContextFromWorld(InWorld)->PendingNetGame != NULL &&
		(FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::FailureReceived) )
	{
		return;
	}

	Super::PeekNetworkFailureMessages(InWorld, NetDriver, FailureType, ErrorString);
#if !UE_SERVER

	// Don't care about net drivers that aren't the game net driver, they are probably just beacon net drivers
	if (NetDriver->NetDriverName != NAME_PendingNetDriver && NetDriver->NetDriverName != NAME_GameNetDriver)
	{
		return;
	}

	FText NetworkErrorMessage;
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.

	if (FirstPlayer)
	{
		FirstPlayer->HandleNetworkFailureMessage(FailureType, ErrorString);

		if (NetDriver->NetDriverName == NAME_PendingNetDriver)
		{
			FirstPlayer->CloseConnectingDialog();
		}

		if (FailureType == ENetworkFailure::PendingConnectionFailure)
		{
			if ( ErrorString == TEXT("NEEDPASS") )
			{
				// If we have gotten here, then any password used is invalid.. So remove it.
				if (NetDriver != NULL && NetDriver->ServerConnection != NULL)
				{
					// If we are in a session, we we have already attempted to use a cached password and it would have failed.  So just popup the dialog
					IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
					if (OnlineSub)
					{
						IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
						FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(TEXT("Game"));
						
						if (Settings != nullptr)
						{
							FString ServerGUID;
							Settings->Get(SETTING_SERVERINSTANCEGUID, ServerGUID);
							if (ServerGUID.IsEmpty())
							{
								FirstPlayer->RemoveCachedPassword(ServerGUID);
							}						
						}
					}

					FirstPlayer->OpenDialog(SNew(SUTInputBoxDialog)
						.OnDialogResult( FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::ConnectPasswordResult))
						.PlayerOwner(FirstPlayer)
						.DialogTitle(NSLOCTEXT("UTGameViewportClient", "PasswordRequireTitle", "Password is Required"))
						.MessageText(NSLOCTEXT("UTGameViewportClient", "PasswordRequiredText", "This server requires a password:"))
						);
				}
			}
			else if (ErrorString == TEXT("CANTJOININPROGRESS"))
			{
				FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","PreLoginError","Login Error"), NSLOCTEXT("UTGameViewportClient","CANTJIP","This server does not support join in progress!"), UTDIALOG_BUTTON_OK,FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));	
				FirstPlayer->ShowMenu(TEXT(""));
				return;
			}
			else if (ErrorString == TEXT("TOOWEAK"))
			{
				FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","PreLoginError","Login Error"), NSLOCTEXT("UTGameViewportClient","WEAKMSG","You are not skilled enough to play on this server!"), UTDIALOG_BUTTON_OK,FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));	
				FirstPlayer->ShowMenu(TEXT(""));
				return;
			}
			else if (ErrorString == TEXT("TOOSTRONG"))
			{
				FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","PreLoginError","Login Error"), NSLOCTEXT("UTGameViewportClient","STRONGMSG","Your skill is too high for this server!"), UTDIALOG_BUTTON_OK,FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));	
				FirstPlayer->ShowMenu(TEXT(""));
				return;
			}
			else if (ErrorString == TEXT("NOTLOGGEDIN"))
			{
				// NOTE: It's possible that the player logged in during the connect sequence but after Prelogin was called on the client.  If this is the case, just reconnect.
				if (FirstPlayer->IsLoggedIn())
				{
					FirstPlayer->PlayerController->ConsoleCommand(TEXT("Reconnect"));
					return;
				}
			
				// If we already have a reconnect message, then don't handle this
				if (!ReconnectDialog.IsValid())
				{
					ReconnectDialog = FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","LoginRequiredTitle","Login Required"), 
															   NSLOCTEXT("UTGameViewportClient","LoginRequiredMessage","You need to login to your Epic account before you can play on this server."), UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_RECONNECT, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::LoginFailureDialogResult));
				}
			}
			else if (ErrorString == TEXT("BANNED"))
			{
				FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "BannedFromServerTitle", "IMPORTANT"), NSLOCTEXT("UTGameViewportClient", "BannedFromServerMsg", "You have been banned from this server!"), UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
				FirstPlayer->ShowMenu(TEXT(""));
				return;
			}

			// TODO: Explain to the engine team why you can't localize server error strings :(
			else if (ErrorString == TEXT("Server full."))
			{
				if (!FirstPlayer->QuickMatchCheckFull())
				{
					FirstPlayer->ShowMenu(TEXT(""));
					FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","PreLoginError","Unable to Join"), NSLOCTEXT("UTGameViewportClient","SERVERFULL","The game you are trying to join is full!"), UTDIALOG_BUTTON_OK,FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));	
				}

				return;
			}
			return;
		}
	}

	uint16 DialogButtons = UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_RECONNECT;
	switch (FailureType)
	{
		case ENetworkFailure::FailureReceived:
		case ENetworkFailure::ConnectionLost:
			if (!KickReason.IsEmpty())
			{
				NetworkErrorMessage = KickReason;
				DialogButtons = UTDIALOG_BUTTON_OK;
				KickReason = FText::GetEmpty();
			}
			else
			{
				NetworkErrorMessage = NSLOCTEXT("UTGameViewportClient","NetworkErrors_ConnectionLost","Connection to server Lost!");
			}
			break;
		case ENetworkFailure::ConnectionTimeout:
			NetworkErrorMessage = NSLOCTEXT("UTGameViewportClient","NetworkErrors_ConnectionTimeout","Connection to server timed out!");
			break;
		case ENetworkFailure::OutdatedClient:
			NetworkErrorMessage = NSLOCTEXT("UTGameViewportClient","NetworkErrors_ClientOutdated","Your client is outdated.  Please update your version of UT.  Go to Forums.UnrealTournament.com for more information.");
			break;
		case ENetworkFailure::OutdatedServer:
			NetworkErrorMessage = NSLOCTEXT("UTGameViewportClient","NetworkErrors_ServerOutdated","The server you are connecting to is running a different version of UT.  Make sure you have the latest version of UT.  Go to Forums.UnrealTournament.com for more information.");
			break;
		default:
			NetworkErrorMessage = FText::FromString(ErrorString);
			break;
	}

	if (!FirstPlayer->QuickMatchCheckFull() && !ReconnectDialog.IsValid())
	{
		if (FirstPlayer->GetCurrentMenu().IsValid())
		{
			FirstPlayer->HideMenu();
		}
		ReconnectDialog = FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient","NetworkErrorDialogTitle","Network Error"), NetworkErrorMessage, DialogButtons, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
	}
#endif
}

static FVector2D PaniniProjection(FVector2D OM, float d, float s)
{
	float PaniniDirectionXZInvLength = 1.0f / FMath::Sqrt(1.0f + OM.X * OM.X);
	float SinPhi = OM.X * PaniniDirectionXZInvLength;
	float TanTheta = OM.Y * PaniniDirectionXZInvLength;
	float CosPhi = FMath::Sqrt(1.0f - SinPhi * SinPhi);
	float S = (d + 1.0f) / (d + CosPhi);

	return S * FVector2D(SinPhi, FMath::Lerp(TanTheta, TanTheta / CosPhi, s));
}

static FName NAME_d(TEXT("d"));
static FName NAME_s(TEXT("s"));
static FName NAME_FOVMulti(TEXT("FOV Multi"));
static FName NAME_Scale(TEXT("Scale"));
FVector UUTGameViewportClient::PaniniProjectLocation(const FSceneView* SceneView, const FVector& WorldLoc, UMaterialInterface* PaniniParamsMat) const
{
	float d = 1.0f;
	float s = 0.0f;
	float FOVMulti = 0.5f;
	float Scale = 1.0f;
	if (PaniniParamsMat != NULL)
	{
		PaniniParamsMat->GetScalarParameterValue(NAME_d, d);
		PaniniParamsMat->GetScalarParameterValue(NAME_s, s);
		PaniniParamsMat->GetScalarParameterValue(NAME_FOVMulti, FOVMulti);
		PaniniParamsMat->GetScalarParameterValue(NAME_Scale, Scale);
	}

	FVector ViewSpaceLoc = SceneView->ViewMatrices.GetViewMatrix().TransformPosition(WorldLoc);
	FVector2D PaniniResult = PaniniProjection(FVector2D(ViewSpaceLoc.X / ViewSpaceLoc.Z, ViewSpaceLoc.Y / ViewSpaceLoc.Z), d, s);

	FMatrix ClipToView = SceneView->ViewMatrices.GetInvProjectionMatrix();
	float ScreenSpaceScaleFactor = (ClipToView.M[0][0] / PaniniProjection(FVector2D(ClipToView.M[0][0], ClipToView.M[1][1]), d, 0.0f).X) * Scale;
	float FOVModifier = ((ClipToView.M[0][0] - 1.0f) * FOVMulti + 1.0f) * ScreenSpaceScaleFactor;

	PaniniResult *= (ViewSpaceLoc.Z * FOVModifier);

	return SceneView->ViewMatrices.GetInvViewMatrix().TransformPosition(FVector(PaniniResult.X, PaniniResult.Y, ViewSpaceLoc.Z));
}

FVector UUTGameViewportClient::PaniniProjectLocationForPlayer(ULocalPlayer* Player, const FVector& WorldLoc, UMaterialInterface* PaniniParamsMat) const
{
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetWorld()->Scene, EngineShowFlags).SetRealtimeUpdate(true));

	FVector ViewLocation;
	FRotator ViewRotation;
	FSceneView* SceneView = Player->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, Viewport);
	return (SceneView != NULL) ? PaniniProjectLocation(SceneView, WorldLoc, PaniniParamsMat) : WorldLoc;
}

void UUTGameViewportClient::Draw(FViewport* InViewport, FCanvas* SceneCanvas)
{
	// apply panini projection to first person weapon

	struct FSavedTransform
	{
		USceneComponent* Component;
		FTransform Transform;
		FSavedTransform(USceneComponent* InComp, const FTransform& InTransform)
			: Component(InComp), Transform(InTransform)
		{}
	};
	TArray<FSavedTransform> SavedTransforms;

	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->PlayerController);
		if (UTPC != NULL && !UTPC->IsBehindView())
		{
			AUTCharacter* UTC = Cast<AUTCharacter>(It->PlayerController->GetViewTarget());
			if (UTC != NULL && UTC->GetWeapon() != NULL && UTC->GetWeapon()->GetMesh() != NULL)
			{
				FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetWorld()->Scene, EngineShowFlags).SetRealtimeUpdate(true));

				FVector ViewLocation;
				FRotator ViewRotation;
				FSceneView* SceneView = It->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, Viewport);
				if (SceneView != NULL)
				{
					TArray<UMeshComponent*> WeaponMeshes = UTC->GetWeapon()->Get1PMeshes();
					TArray<USceneComponent*> Children = UTC->GetWeapon()->GetMesh()->GetAttachChildren(); // make a copy in case something below causes it to change
					for (USceneComponent* Attachment : Children)
					{
						// any additional weapon meshes are assumed to be projected in the shader if desired
						if (!Attachment->IsPendingKill() && !WeaponMeshes.Contains(Attachment) && !SavedTransforms.ContainsByPredicate([Attachment](const FSavedTransform& TestItem) { return TestItem.Component == Attachment; }))
						{
							FVector AdjustedLoc = PaniniProjectLocation(SceneView, Attachment->GetComponentLocation(), UTC->GetWeapon()->GetMesh()->GetMaterial(0));

							new(SavedTransforms) FSavedTransform(Attachment, Attachment->GetComponentTransform());
							Attachment->SetWorldLocation(AdjustedLoc);
							// hacky solution to attached spline mesh beams that need to update their endpoint
							if (Attachment->GetOwner() != nullptr && Attachment->GetOwner() != UTC->GetWeapon())
							{
								Attachment->GetOwner()->Tick(0.0f);
							}
							if (!Attachment->IsPendingKill())
							{
								Attachment->DoDeferredRenderUpdates_Concurrent();
							}
						}
					}
				}
			}
		}
	}

	Super::Draw(InViewport, SceneCanvas);

	// revert components to their normal location
	for (const FSavedTransform& RestoreItem : SavedTransforms)
	{
		RestoreItem.Component->SetWorldTransform(RestoreItem.Transform);
	}
}

void UUTGameViewportClient::FinalizeViews(FSceneViewFamily* ViewFamily, const TMap<ULocalPlayer*, FSceneView*>& PlayerViewMap)
{
	Super::FinalizeViews(ViewFamily, PlayerViewMap);
	
	// set special show flags when using the casting guide
	if (GameInstance != NULL)
	{
		const TArray<ULocalPlayer*> GamePlayers = GameInstance->GetLocalPlayers();
		if (GamePlayers.Num() > 0 && GamePlayers[0] != NULL)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(GamePlayers[0]->PlayerController);
			if (PC != NULL && PC->bCastingGuide)
			{
				ViewFamily->EngineShowFlags.PostProcessing = 0;
				ViewFamily->EngineShowFlags.AtmosphericFog = 0;
				ViewFamily->EngineShowFlags.DynamicShadows = 0;
				ViewFamily->EngineShowFlags.LightFunctions = 0;
				ViewFamily->EngineShowFlags.ScreenSpaceReflections = 0;
			}
		}
	}
}

void UUTGameViewportClient::UpdateActiveSplitscreenType()
{
	int32 NumPlayers = GEngine->GetNumGamePlayers(GetWorld());
	if (NumPlayers <= 4)
	{
		Super::UpdateActiveSplitscreenType();
	}
	else
	{
		ActiveSplitscreenType = ESplitScreenType::Type(7 + (NumPlayers - 4));
	}
}

void UUTGameViewportClient::PostRender(UCanvas* Canvas)
{
#if WITH_EDITORONLY_DATA
	if (!GIsEditor)
#endif
	{
		// work around bug where we can end up with no PlayerController during initial connection (or when losing connection) by rendering an overlay to explain to the user what's going on
		TArray<ULocalPlayer*> GamePlayers;
		if (GameInstance != NULL)
		{
			GamePlayers = GameInstance->GetLocalPlayers();
			// remove players that aren't currently set up for proper rendering (no PlayerController or currently using temp proxy while waiting for server)
			for (int32 i = GamePlayers.Num() - 1; i >= 0; i--)
			{
				if (GamePlayers[i]->PlayerController == NULL || GamePlayers[i]->PlayerController->Player == NULL)
				{
					GamePlayers.RemoveAt(i);
				}
			}
		}
		if (GamePlayers.Num() > 0)
		{
			Super::PostRender(Canvas);
		}
		else
		{
			UFont* Font = GetDefault<AUTHUD>()->MediumFont;
			FText Message = NSLOCTEXT("UTHUD", "WaitingForServer", "Waiting for server to respond...");
			float XL, YL;
			Canvas->SetLinearDrawColor(FLinearColor::White);
			Canvas->TextSize(Font, Message.ToString(), XL, YL);
			Canvas->DrawText(Font, Message, (Canvas->ClipX - XL) * 0.5f, (Canvas->ClipY - YL) * 0.5f);
		}
	}
}

void UUTGameViewportClient::LoginFailureDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		if (FirstPlayer->IsLoggedIn())
		{
			// We are already Logged in.. Reconnect;
			FirstPlayer->PlayerController->ConsoleCommand(TEXT("Reconnect"));
		}
		else
		{
			FirstPlayer->LoginOnline(TEXT(""),TEXT(""),false, false);
		}
	}
	else
	{
		FirstPlayer->PlayerController->ConsoleCommand(TEXT("Reconnect"));
	}
}

void UUTGameViewportClient::RankDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	ReconnectDialog.Reset();
}


void UUTGameViewportClient::NetworkFailureDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_RECONNECT)
	{
		UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));
		FirstPlayer->PlayerController->ConsoleCommand(TEXT("Reconnect"));
	}
	else
	{
		UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));
		if (!FirstPlayer->IsPartyLeader())
		{
			UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UPartyContext::StaticClass()));
			if (PartyContext)
			{
				PartyContext->LeaveParty();
			}
		}
	}

	ReconnectDialog.Reset();
}

void UUTGameViewportClient::ConnectPasswordResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
#if !UE_SERVER
	if (ButtonID != UTDIALOG_BUTTON_CANCEL)
	{
		TSharedPtr<SUTInputBoxDialog> Box = StaticCastSharedPtr<SUTInputBoxDialog>(Widget);
		if (Box.IsValid())
		{
			FString InputText = Box->GetInputText();

			UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
			if (!InputText.IsEmpty() && FirstPlayer != NULL)
			{
				if (FirstPlayer->LastSession.IsValid())
				{
					FString ServerGUID;
					FirstPlayer->LastSession.Session.SessionSettings.Get(SETTING_SERVERINSTANCEGUID, ServerGUID);
					FirstPlayer->CachePassword(ServerGUID, InputText);
				}						
				else
				{
					FirstPlayer->CachePassword(FirstPlayer->LastConnectToIP, InputText);
				}

				FirstPlayer->Reconnect(false);
			}
		}
	}
#endif
}

void UUTGameViewportClient::ReconnectAfterDownloadingContent()
{
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer != nullptr)
	{
		FirstPlayer->Reconnect(false);
	}
}

void UUTGameViewportClient::VerifyFilesToDownloadAndReconnect()
{
#if !UE_SERVER
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer != nullptr)
	{
		if (UTEngine)
		{
			for (auto It = UTEngine->FilesToDownload.CreateConstIterator(); It; ++It)
			{
				if (!UTEngine->MountedDownloadedContentChecksums.Contains(It.Key()))
				{
					// File failed to download at all.
					FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "DownloadFail", "Download Failed"), NSLOCTEXT("UTGameViewportClient", "DownloadFailMsg", "The download of cloud content failed."), UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
					return;
				}

				if (UTEngine->MountedDownloadedContentChecksums[It.Key()] != It.Value())
				{
					// File was the wrong checksum.
					FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "WrongChecksum", "Checksum failed"), NSLOCTEXT("UTGameViewportClient", "WrongChecksumMsg", "The files downloaded from the cloud do not match the files the server is using."), UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
					return;
				}
			}
		}

		ReconnectAfterDownloadingContent();
	}
#endif
}

void UUTGameViewportClient::CloudRedirectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
#if !UE_SERVER

	if (ButtonID != UTDIALOG_BUTTON_CANCEL)
	{
		VerifyFilesToDownloadAndReconnect();
	}
	else
	{
		UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
		if (FirstPlayer != nullptr)
		{
			FirstPlayer->ShowMessage(NSLOCTEXT("UTGameViewportClient", "UnableToGetCustomContent", "Server running custom content"), NSLOCTEXT("UTGameViewportClient", "UnableToGetCustomContentMsg", "The server is running custom content and your client was unable to download it."), UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTGameViewportClient::NetworkFailureDialogResult));
		}
	}
#endif
}

void UUTGameViewportClient::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ReconnectAfterDownloadingMapDelay > 0)
	{
		ReconnectAfterDownloadingMapDelay -= DeltaSeconds;
		if (ReconnectAfterDownloadingMapDelay <= 0)
		{
			ReconnectAfterDownloadingContent();
		}
	}

	if (VerifyFilesToDownloadAndReconnectDelay > 0)
	{
		VerifyFilesToDownloadAndReconnectDelay -= DeltaSeconds;
		if (VerifyFilesToDownloadAndReconnectDelay <= 0)
		{
			VerifyFilesToDownloadAndReconnect();
		}
	}

	UpdateRedirects(DeltaSeconds);

	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer && FirstPlayer->SkipWorldRender())
	{
		GEngine->GameViewport->bDisableWorldRendering = true;
	}
	else
	{
		GEngine->GameViewport->bDisableWorldRendering = false;
	}

	if (bReconnectAtNextTick && FirstPlayer)
	{
		FirstPlayer->Reconnect(bReconnectAtNextTickNeedSpectator);
		bReconnectAtNextTick = false;
	}
}

void UUTGameViewportClient::UpdateRedirects(float DeltaTime)
{
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer == nullptr || FirstPlayer->IsShowingDLCWarning()) return;

	if (PendingDownloads.Num() >0)
	{
		FirstPlayer->ShowRedirectDownload();
		if (PendingDownloads[0].Status == ERedirectStatus::Pending)
		{
			PendingDownloads[0].HttpRequest = FHttpModule::Get().CreateRequest();
			if (PendingDownloads[0].HttpRequest.IsValid())
			{
				PendingDownloads[0].Status = ERedirectStatus::InProgress;
				PendingDownloads[0].HttpRequest->SetURL(PendingDownloads[0].FileURL);
				PendingDownloads[0].HttpRequest->OnProcessRequestComplete().BindUObject(this, &UUTGameViewportClient::HttpRequestComplete);
				PendingDownloads[0].HttpRequest->OnRequestProgress().BindUObject(this, &UUTGameViewportClient::HttpRequestProgress);

				if ( PendingDownloads[0].FileURL.Contains(TEXT("epicgames")) ) 				
				{
					IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
					if (OnlineSubsystem)
					{
						IOnlineIdentityPtr OnlineIdentityInterface;
						OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
						if (OnlineIdentityInterface.IsValid())
						{
							FString AuthToken = OnlineIdentityInterface->GetAuthToken(0);
							PendingDownloads[0].HttpRequest->SetHeader(TEXT("Authorization"), FString(TEXT("bearer ")) + AuthToken);
						}
					}
				}

				PendingDownloads[0].HttpRequest->SetVerb("GET");
				if ( !PendingDownloads[0].HttpRequest->ProcessRequest() )
				{
					// Failed too early, clean me up
					ContentDownloadComplete.Broadcast(this, ERedirectStatus::Failed, PendingDownloads[0].FileURL);
					PendingDownloads.RemoveAt(0);
					return;
				}
				else
				{
					HttpRequestProgress(PendingDownloads[0].HttpRequest, 0, 0);
				}
			}
		}

		if ( PendingDownloads[0].HttpRequest.IsValid() )
		{
			if (PendingDownloads[0].HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
			{
				// Update the Requester
				PendingDownloads[0].HttpRequest->Tick(DeltaTime);
			}
			else if (PendingDownloads[0].HttpRequest->GetStatus() == EHttpRequestStatus::Failed)
			{
				// clean me up
				ContentDownloadComplete.Broadcast(this, ERedirectStatus::Failed, PendingDownloads[0].FileURL);
				PendingDownloads.RemoveAt(0);
			}
		}
	}
	else
	{
		FirstPlayer->HideRedirectDownload();
		ContentDownloadComplete.Broadcast(this, ERedirectStatus::Completed, TEXT(""));
	}
}

bool UUTGameViewportClient::IsDownloadInProgress()
{
	return PendingDownloads.Num() > 0;
}

bool UUTGameViewportClient::CheckIfRedirectExists(const FPackageRedirectReference& Redirect)
{
	FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"), *Redirect.PackageName) + TEXT(".pak");
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		if (UTEngine->LocalContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->LocalContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				return true;
			}
			else
			{
				// Local content has a non-matching md5, not sure if we should try to unmount/delete it
				return false;
			}
		}

		if (UTEngine->MountedDownloadedContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->MountedDownloadedContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				return true;
			}
			else
			{
				// Unmount the pak
				if (FCoreDelegates::OnUnmountPak.IsBound())
				{
					FCoreDelegates::OnUnmountPak.Execute(Path);
				}

				// Remove the CRC entry
				UTEngine->MountedDownloadedContentChecksums.Remove(Redirect.PackageName);
				UTEngine->DownloadedContentChecksums.Remove(Redirect.PackageName);

				// Delete the original file
				FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
			}
		}

		if (UTEngine->DownloadedContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->DownloadedContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				// Mount the pak
				if (FCoreDelegates::OnMountPak.IsBound())
				{
					FCoreDelegates::OnMountPak.Execute(Path, 0, nullptr);
					UTEngine->MountedDownloadedContentChecksums.Add(Redirect.PackageName, Redirect.PackageChecksum);
				}

				return true;
			}
			else
			{
				UTEngine->DownloadedContentChecksums.Remove(Redirect.PackageName);

				// Delete the original file
				FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
			}
		}
	}

	return false;
}

void UUTGameViewportClient::DownloadRedirect(const FString& URL, const FString& OptionalPakName, const FString& OptionalChecksum)
{
	FString WebProtocol = TEXT("");
	FString WebAddress = TEXT("");
	int32 Pos = URL.Find(TEXT("://"));
	if (Pos != INDEX_NONE)
	{
		WebProtocol = URL.Left(Pos);
		WebAddress = URL.RightChop(Pos + 3);
	}

	DownloadRedirect(FPackageRedirectReference(OptionalPakName, WebProtocol, WebAddress, OptionalChecksum));
}

void UUTGameViewportClient::DownloadRedirect(FPackageRedirectReference Redirect)
{
	if (!CheckIfRedirectExists(Redirect))
	{
		for (int32 i = 0; i < PendingDownloads.Num(); i++)
		{
			// Look to see if the file is already pending and if it is, don't add it again.
			if (PendingDownloads[i].FileURL.Equals(Redirect.ToString(), ESearchCase::IgnoreCase))
			{
				return;
			}
		}

		PendingDownloads.Add(FPendingRedirect(Redirect.ToString().TrimTrailing(), Redirect.PackageChecksum));

		UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	
		if (FirstPlayer && FirstPlayer->RequiresDLCWarning() && !FirstPlayer->IsShowingDLCWarning())
		{
			FirstPlayer->ShowDLCWarning();
		}

		// NOTE: The next tick will start the download process...	
	}
}

void UUTGameViewportClient::CancelRedirect(FPackageRedirectReference Redirect)
{
	for (int32 i=0; i < PendingDownloads.Num(); i++)
	{
		if ( PendingDownloads[i].FileURL.Equals(Redirect.ToString(), ESearchCase::IgnoreCase) )	
		{
			// Broadcast that this has been cancelled.

			ContentDownloadComplete.Broadcast(this, ERedirectStatus::Cancelled, PendingDownloads[i].FileURL);

			// Found it.  If we are in progress.. stop us
			if (PendingDownloads[i].HttpRequest.IsValid() && PendingDownloads[i].HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
			{
				PendingDownloads[i].HttpRequest->CancelRequest();
			}
			else
			{
				// We haven't started yet so just remove it.
				PendingDownloads.RemoveAt(i,1);			
			}
			
			return;
		}
	}
}

void UUTGameViewportClient::CancelAllRedirectDownloads()
{
	for (int32 i=0; i < PendingDownloads.Num(); i++)
	{
		ContentDownloadComplete.Broadcast(this, ERedirectStatus::Cancelled, PendingDownloads[i].FileURL);

		// Found it.  If we are in progress.. stop us
		if (PendingDownloads[i].HttpRequest.IsValid() && PendingDownloads[i].HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
		{
			PendingDownloads[i].HttpRequest->CancelRequest();
		}
	}
	PendingDownloads.Empty();
}

void UUTGameViewportClient::HttpRequestProgress(FHttpRequestPtr HttpRequest, int32 NumBytesSent, int32 NumBytesRecv)
{
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	
	if (FirstPlayer && PendingDownloads.Num() > 0)
	{
		int32 ContentLength = HttpRequest->GetResponse()->GetContentLength();

		float Perc = ContentLength > 0 ? (NumBytesRecv / float(ContentLength)) : 0.0f;

		// Create all of the status update strings
		FString DownloadFileName = FPaths::GetBaseFilename(PendingDownloads[0].FileURL);
		DownloadStatusText = FText::Format(NSLOCTEXT("UTLocalPlayer","DownloadStatusFormat","Downloading {0} Files: {1} ({2} / {3}) ...."), FText::AsNumber(PendingDownloads.Num()), FText::FromString(DownloadFileName), FText::AsNumber(NumBytesRecv), FText::AsPercent(Perc));
		Download_NumBytes = NumBytesRecv;
		Download_CurrentFile = DownloadFileName;
		Download_Percentage = Perc;
		Download_NumFilesLeft = PendingDownloads.Num();
	}
}

void UUTGameViewportClient::HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	//If the download was successful save it to disk
	if (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok && PendingDownloads.Num() > 0)
	{
		if (HttpResponse->GetContent().Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		
			FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"));
			if (!PlatformFile.DirectoryExists(*Path))
			{
				PlatformFile.CreateDirectoryTree(*Path);
			}

			FString FileURL = PendingDownloads[0].FileURL;
			FString FullFilePath = FPaths::Combine(*Path, *FPaths::GetCleanFilename(FileURL));

			UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
			if (UTEngine)
			{
				FString ActualMD5 = UTEngine->MD5Sum(HttpResponse->GetContent());
				if (PendingDownloads.Num() > 0 && ActualMD5 == PendingDownloads[0].RequiredMD5 )
				{
					bSucceeded = FFileHelper::SaveArrayToFile(HttpResponse->GetContent(), *FullFilePath);
					if (bSucceeded)
					{
						FString BaseFilename = FPaths::GetBaseFilename(FileURL);
						UTEngine->DownloadedContentChecksums.Add(BaseFilename, ActualMD5);

						if (FCoreDelegates::OnMountPak.IsBound())
						{
							FCoreDelegates::OnMountPak.Execute(FullFilePath, 0, nullptr);
							UTEngine->MountedDownloadedContentChecksums.Add(BaseFilename, ActualMD5);
							UTEngine->AddAssetRegistry(BaseFilename);
						}
					}
					else
					{
						UE_LOG(UT, Warning, TEXT("Downloaded file could not be saved."));

					}
				}
				else
				{
					UE_LOG(UT, Warning, TEXT("Downloaded file %s failed MD5 check (%s != %s)"), *FullFilePath, *ActualMD5, *PendingDownloads[0].RequiredMD5 );
					bSucceeded = false;
				}
			}
		}
	}
	else
	{
		if (HttpResponse.IsValid())
		{
			UE_LOG(UT, Warning, TEXT("HTTP Error: %d"), HttpResponse->GetResponseCode());
			FString ErrorContent = HttpResponse->GetContentAsString();
			UE_LOG(UT, Log, TEXT("%s"), *ErrorContent);
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("HTTP Error"));
		}
	}

	// Remove this one.
	if (PendingDownloads.Num() > 0)
	{
		PendingDownloads.RemoveAt(0,1);
	}
}

FDelegateHandle UUTGameViewportClient::RegisterContentDownloadCompleteDelegate(const FContentDownloadComplete::FDelegate& NewDelegate)
{
	return ContentDownloadComplete.Add(NewDelegate);
}

void UUTGameViewportClient::RemoveContentDownloadCompleteDelegate(FDelegateHandle DelegateHandle)
{
	ContentDownloadComplete.Remove(DelegateHandle);
}

bool UUTGameViewportClient::HideCursorDuringCapture()
{
	// workaround for Slate bug where toggling the pointer visibility on/off in the PlayerController while this is enabled can cause the pointer to get locked hidden
	if (!Super::HideCursorDuringCapture())
	{
		return false;
	}
	else if (GameInstance == NULL)
	{
		return true;
	}
	else
	{
		const TArray<ULocalPlayer*> GamePlayers = GameInstance->GetLocalPlayers();
		return (GamePlayers.Num() == 0 || GamePlayers[0] == NULL || GamePlayers[0]->PlayerController == NULL || !GamePlayers[0]->PlayerController->ShouldShowMouseCursor());
	}
}

void UUTGameViewportClient::ClientConnectedToServer()
{
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer)
	{
		FirstPlayer->CloseConnectingDialog();
	}
}


ULocalPlayer* UUTGameViewportClient::SetupInitialLocalPlayer(FString& OutError)
{
	// Work around shipping not having a console
#if UE_BUILD_SHIPPING
	// Create the viewport's console.
	ViewportConsole = NewObject<UConsole>(this, GetOuterUEngine()->ConsoleClass);
	// register console to get all log messages
	GLog->AddOutputDevice(ViewportConsole);
#endif // !UE_BUILD_SHIPPING

	return Super::SetupInitialLocalPlayer(OutError);
}

void UUTGameViewportClient::SetActiveWorldOverride(UWorld* WorldOverride)
{
	ActiveWorldOverride = WorldOverride;
	SetActiveLocalPlayerControllers();
}

void UUTGameViewportClient::ClearActiveWorldOverride()
{
	ActiveWorldOverride.Reset();
	SetActiveLocalPlayerControllers();
}

void UUTGameViewportClient::SetActiveLocalPlayerControllers()
{
	// Switch the local player's controller to the controller in the active world.
	// Not calling UPlayer::SwitchController because we don't want to null out the APlayerController::Player pointer here.
	if (GetWorld() != nullptr && GetGameInstance() != nullptr)
	{
		for (ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
		{
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Controller = It->Get();
				if (Controller->Player == LocalPlayer)
				{
					LocalPlayer->PlayerController = Controller;
				}
			}
		}
	}
}

UWorld* UUTGameViewportClient::GetWorld() const
{
	if (ActiveWorldOverride.IsValid())
	{
		return ActiveWorldOverride.Get();
	}

	return Super::GetWorld();
}

void UUTGameViewportClient::ReceivedFocus(FViewport* InViewport)
{
	Super::ReceivedFocus(InViewport);
	UUTLocalPlayer* FirstPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(this, 0));	// Grab the first local player.
	if (FirstPlayer && FirstPlayer->PlayerController)
	{
#if !UE_SERVER
		AUTBasePlayerController* UTBasePlayerController = Cast<AUTBasePlayerController>(FirstPlayer->PlayerController);
		if (UTBasePlayerController)
		{
			UTBasePlayerController->UpdateInputMode(true);
		}
#endif

	}
}
