#define STRINGIFY_MACRO(x) STR(x)
#define STR(x) #x
#define EXPAND(x) x
#define CONCAT(n1, n2, n3) STRINGIFY_MACRO(EXPAND(n1)EXPAND(n2)EXPAND(n3))
//#include CONCAT(UE_PROJECT_NAME,.,h)
#include "UnrealTournament.h"
#include "UObject/Object.h"
#include "UTFocusTracer.h"
#include "Public/Widgets/SViewport.h"
#include "Public/Slate/SceneViewport.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Public/UTPlayerCameraManager.h"
#include "Public/UTCharacter.h"
#include "Public/SocketSubsystem.h"
#include "Public/IPAddress.h"
#include "Public/UTATypes.h"
#include "HighResScreenshot.h"
//#include "IImageWrapper.h"
//#include "Runtime/ImageWriteQueue/Public/ImageWriteQueue.h"
#include "UniquePtr.h"
#include "Slate/SceneViewport.h"
#include "Slate/WidgetRenderer.h"

#include <stdlib.h>
#include <vector>

UTFocusTracer::UTFocusTracer(AActor* aactor, uint8 p, FString key)
	: actor(aactor)
	, priority(p)
	, keyName(key)
	, localBound(ForceInitToZero)
	, localBoundCalculated(false)
{
	player = NULL;
	primComp = NULL;
	boundsUpdated = UpdateBounds();
}

bool UTFocusTracer::UpdateBounds()
{
	if (primComp == NULL)
	{
		for (const UActorComponent* ActorComponent : actor->GetComponents())
		{
			const UPrimitiveComponent* primitiveComp = Cast<UPrimitiveComponent>(ActorComponent);
			if (primitiveComp)
			{
				if (primitiveComp->IsRegistered() && primitiveComp->GetFName().ToString().Contains(keyName))
				{
					primComp = (UPrimitiveComponent*)primitiveComp;
					break;
				}
			}
		}
	}
	if (primComp == NULL)
	{
		return false;
	}
	
	const USkeletalMeshComponent* sklComp = Cast<const USkeletalMeshComponent>(primComp);
	if (!localBoundCalculated || sklComp != NULL)
	{
		localBound = primComp->CalcBounds(FTransform::Identity);
		localBoundCalculated = true;
	}

	/// calc world bounds
	FTransform toWorldTrans = primComp->GetComponentTransform();
	bounds[0].Set(localBound.Origin.X + localBound.BoxExtent.X, localBound.Origin.Y + localBound.BoxExtent.Y, localBound.Origin.Z + localBound.BoxExtent.Z);
	bounds[1].Set(localBound.Origin.X - localBound.BoxExtent.X, localBound.Origin.Y + localBound.BoxExtent.Y, localBound.Origin.Z + localBound.BoxExtent.Z);
	bounds[2].Set(localBound.Origin.X + localBound.BoxExtent.X, localBound.Origin.Y + localBound.BoxExtent.Y, localBound.Origin.Z - localBound.BoxExtent.Z);
	bounds[3].Set(localBound.Origin.X - localBound.BoxExtent.X, localBound.Origin.Y + localBound.BoxExtent.Y, localBound.Origin.Z - localBound.BoxExtent.Z);
	bounds[4].Set(localBound.Origin.X + localBound.BoxExtent.X, localBound.Origin.Y - localBound.BoxExtent.Y, localBound.Origin.Z + localBound.BoxExtent.Z);
	bounds[5].Set(localBound.Origin.X - localBound.BoxExtent.X, localBound.Origin.Y - localBound.BoxExtent.Y, localBound.Origin.Z + localBound.BoxExtent.Z);
	bounds[6].Set(localBound.Origin.X + localBound.BoxExtent.X, localBound.Origin.Y - localBound.BoxExtent.Y, localBound.Origin.Z - localBound.BoxExtent.Z);
	bounds[7].Set(localBound.Origin.X - localBound.BoxExtent.X, localBound.Origin.Y - localBound.BoxExtent.Y, localBound.Origin.Z - localBound.BoxExtent.Z);
	for (int i = 0; i < 8; i++)
	{
		bounds[i] = toWorldTrans.TransformFVector4(bounds[i]);
	}

	return true;
}


FocusRectInfo* UTFocusTracer::UpdateRectInfo()
{
	if (actor->GetRootComponent()->Mobility != EComponentMobility::Static)
	{
		boundsUpdated = UpdateBounds();
	}
	
	if (!boundsUpdated)
		return NULL;

	if (actor->GetWorld())
	{
		player = UGameplayStatics::GetPlayerController(actor->GetWorld(), 0);
	}
	
	FVector2D screenPos;
	FVector worldPos = actor->GetActorTransform().GetLocation();
	if(player && UGameplayStatics::ProjectWorldToScreen(player, worldPos, screenPos) )
	{
		FVector viewWorldPos;
		FRotator viewWorldRot;
		player->GetPlayerViewPoint(viewWorldPos, viewWorldRot);
		FocusRectInfo* rectInfo = new FocusRectInfo();
		rectInfo->distToCam = FVector::Dist(worldPos, viewWorldPos);
		rectInfo->priority = priority;
		rectInfo->left = FLT_MAX;
		rectInfo->right = -FLT_MAX;
		rectInfo->top = FLT_MAX;
		rectInfo->bottom = -FLT_MAX;
		for (int i = 0; i < 8; i++)
		{
			if (UGameplayStatics::ProjectWorldToScreen(player, bounds[i], screenPos))
			{
				if (screenPos.X <= rectInfo->left)
					rectInfo->left = screenPos.X;
				if (screenPos.X >= rectInfo->right)
					rectInfo->right = screenPos.X;
				if (screenPos.Y <= rectInfo->top)
					rectInfo->top = screenPos.Y;
				if (screenPos.Y >= rectInfo->bottom)
					rectInfo->bottom = screenPos.Y;
			}
		}

		return rectInfo;
	}

	return NULL;
}

float UTFocusUITracer::GetTextOpacity(STextBlock* text)
{
	FWidgetStyle& inWidgetStyle = text->GetInWidgetStyle();
	return inWidgetStyle.GetColorAndOpacityTint().A * text->GetColorAndOpacityPub().GetColor(inWidgetStyle).A;
}

float UTFocusUITracer::GetImageOpacity(SImage* image)
{
	return image->GetFinalOpacity();
}

void UTFocusUITracer::GoThroughChildren(SWidget* parent, std::vector<FocusRectInfo*>& rectInfos, FVector2D& offset, int level)
{
	static STextBlock* text;
	static SImage* image;
	FChildren* children = parent->GetChildren();
	for (int i = 0; i < children->Num(); i++)
	{
		SWidget* child = &(children->GetChildAt(i)).Get();
		FString name = child->GetTypeAsString();

		if (name.Contains("SLevelEditorViewport") || name.Contains("SEditorViewport")|| name.Contains("SLevelViewport"))	/// or check frrom SGameLayerManager children
			continue;
		
		if (child->GetVisibility() == EVisibility::Visible && child->IsEnabled() && /*child->GetRenderOpacity() > 0.0f &&*/ /// no self opacity in the previous ue4 version 
			(name =="STextBlock"|| name == "SImage")
			)
		{
			bool ok = true;
			if (name == "STextBlock")
			{
				text = (STextBlock*)child;
				///UE_LOG(LogTemp, Warning, TEXT("Text color specified is %d"), text->GetColorAndOpacityPub().IsColorSpecified());
				if (text->GetText().IsEmpty() || GetTextOpacity(text) == 0.0f)
				{
					///UE_LOG(LogTemp, Warning, TEXT("Text is %s"), *text->GetText().ToString());
					ok = false;
				}
			}
			else if (name == "SImage")
			{
				image = (SImage*)child;
				const FSlateBrush* ImageBrush = image->GetImagePub().Get();
				///UE_LOG(LogTemp, Warning, TEXT("Image color specified is %d"), image->GetColorAndOpacityPub().IsColorSpecified());
				if (ImageBrush == NULL || ImageBrush->DrawAs == ESlateBrushDrawType::NoDrawType || GetImageOpacity(image) == 0.0f)
				{
					ok = false;
				}
			}

			if (ok)
			{
				FGeometry geo = child->GetCachedGeometry();
				FVector2D size = geo.GetLocalSize();
				FVector2D screenPos = geo.LocalToAbsolute(FVector2D(0, 0));
				FVector2D screenPosDown = geo.LocalToAbsolute(size);

				if (screenPosDown.X > screenPos.X && screenPosDown.Y > screenPos.Y)
				{
					FocusRectInfo* rectInfo = new FocusRectInfo();
					rectInfo->distToCam = 0;
					rectInfo->priority = 128;
					rectInfo->left = screenPos.X - offset.X;
					rectInfo->right = screenPosDown.X - offset.X;
					rectInfo->top = screenPos.Y - offset.Y;
					rectInfo->bottom = screenPosDown.Y - offset.Y;
					rectInfos.push_back(rectInfo);
				}
			}
		}
		GoThroughChildren(child, rectInfos, offset, level + 1);
	}
}

void UTFocusUITracer::UpdateUIRectInfo(std::vector<FocusRectInfo*>& rectInfos)
{
	/// test coed
	TSharedPtr<SViewport> GameViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
	check(GameViewportWidget.IsValid());
	TSharedPtr<SWidget> spSWidget = GameViewportWidget->GetContent();
	FVector2D offset = GEngine->GameViewport->GetGameViewport()->GetCachedGeometry().LocalToAbsolute(FVector2D(0, 0));

	int level = 0;
	GoThroughChildren(spSWidget.Get(), rectInfos, offset, level);
}
	

UTFocusDraw::UTFocusDraw()
{
	const TCHAR* CmdLineParam = FCommandLine::Get();
	FString param(CmdLineParam);
	if (param.Contains("-showrect"))
	{
		isDisplay = true;
	}
}

void UTFocusDraw::DrawRect(float left, float right, float top, float bottom, uint8 prio)
{
	if (isDisplay && GWorld != NULL)
	{
		APlayerController* player = UGameplayStatics::GetPlayerController(GWorld, 0);
		if (player != NULL)
		{
			AHUD* hud = player->GetHUD();
			if (hud != NULL)
			{
				FLinearColor color;
				if (prio == 255)
					color = FLinearColor::Red;
				else if(prio == 128)
					color = FLinearColor::Yellow;
				else
				{
					if (prio == 5)
						color = FLinearColor::Gray;
					else if (prio == 10)
						color = FLinearColor::Blue;
					else
						color = FLinearColor::Green;
				}
				hud->DrawLine(left, top, left, bottom, color, 2.0f);
				hud->DrawLine(left, bottom, right, bottom, color, 2.0f);
				hud->DrawLine(right, bottom, right, top, color, 2.0f);
				hud->DrawLine(right, top, left, top, color, 2.0f);
			}
		}
	}
}

bool UTFocusCamera::GetPosition(float* outPos)
{
	if (GWorld != NULL)
	{
		APlayerController* player = UGameplayStatics::GetPlayerController(GWorld, 0);
///		ASPlayerCameraManager* CameraManager = Cast<ASPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GWorld, 0));
		if (player != NULL)
		{
			FVector viewWorldPos;
			FRotator viewWorldRot;
			player->GetPlayerViewPoint(viewWorldPos, viewWorldRot);
			outPos[0] = viewWorldPos[0];
			outPos[1] = viewWorldPos[1];
			outPos[2] = viewWorldPos[2];
			return true;
		}
	}
	return false;
}

bool UTFocusCamera::GetRotation(float* outRot)
{
	if (GWorld != NULL)
	{
		APlayerController* player = UGameplayStatics::GetPlayerController(GWorld, 0);
		if (player != NULL)
		{
			FVector viewWorldPos;
			FRotator viewWorldRot;
			player->GetPlayerViewPoint(viewWorldPos, viewWorldRot);
			outRot[0] = viewWorldRot.Roll;
			outRot[1] = viewWorldRot.Pitch;
			outRot[2] = viewWorldRot.Yaw;
			return true;
		}
	}
	return false;
}

bool UTFocusSocketSender::Connect()
{
	const TCHAR* CmdLineParam = FCommandLine::Get();

	FString ipParam;
	if (!FParse::Value(CmdLineParam, TEXT("-cloudip="), ipParam))
	{
		return false;
	}

	FString portParam;
	if (!FParse::Value(CmdLineParam, TEXT("-cloudport="), portParam))
	{
		portParam = TEXT("8888");
	}

	return Connect(TCHAR_TO_ANSI(*ipParam), FCString::Atoi(*portParam));
}

bool UTFocusSocketSender::Connect(const char* ipAddr, int port)
{
	//socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("FocusTrace"), TEXT("default"), false);
	socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);

	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	FString fIpAddr(ipAddr);
	bool isValid;
	const TCHAR* tIpAddr = fIpAddr.GetCharArray().GetData();
	addr->SetIp(tIpAddr, isValid);
	addr->SetPort(port);

	offset = 0;
	outBuf = NULL;
	totalSize = 0;

	return isValid && socket->Connect(*addr);;
}

bool UTFocusSocketSender::IsConnected()
{
	return socket && ( socket->GetConnectionState() == ESocketConnectionState::SCS_Connected);
}

bool UTFocusSocketSender::Send(unsigned char* buf, unsigned int size)
{
	if (!socket)
		return false;

	/// rebuild packet with header
	unsigned char* realBuf = new unsigned char[size + 4];
	*((unsigned int*)realBuf) = size;
	memcpy(realBuf + 4, buf, size);
	size += 4;

	/// send
	int sent = 0;
	int left = size;
	while (socket->Send(realBuf, left, sent))
	{
		if (sent == left)
		{
			delete[] realBuf;
			return true;
		}

		left -= sent;
		buf += sent;
	}
	return false;
}

static unsigned int MakePackets(unsigned char* buf, unsigned int buf_size, unsigned int offset, std::vector<Packet>* out_packets, unsigned char*& out_buf, unsigned int& total_size, unsigned char* header)
{
	if (buf_size == 0)
		return offset;

	if (buf_size + offset <= 4)
	{
		memcpy(header + offset, buf, buf_size);
	}
	else
	{
		if (offset <= 4)
		{
			memcpy(header + offset, buf, 4 - offset);
			total_size = *((unsigned int*)header);
			out_buf = new unsigned char[total_size];
			if (buf_size + offset < total_size + 4)
			{
				memcpy(out_buf, buf + 4 - offset, buf_size + offset - 4);
				offset += buf_size;
			}
			else
			{
				memcpy(out_buf, buf + 4 - offset, total_size);
				buf += (total_size + 4 - offset);
				buf_size -= (total_size + 4 - offset);
				out_packets->push_back(Packet(out_buf, total_size));
				offset = 0;
				out_buf = NULL;
				offset = MakePackets(buf, buf_size, offset, out_packets, out_buf, total_size, header);
			}
		}
		else
		{
			if (buf_size + offset < total_size + 4)
			{
				memcpy(out_buf + offset - 4, buf, buf_size);
				offset += buf_size;
			}
			else
			{
				memcpy(out_buf + offset - 4, buf, total_size + 4 - offset);
				buf += (total_size + 4 - offset);
				buf_size -= (total_size + 4 - offset);
				out_packets->push_back(Packet(out_buf, total_size));
				offset = 0;
				out_buf = NULL;
				offset = MakePackets(buf, buf_size, offset, out_packets, out_buf, total_size, header);
			}
		}
	}
	///printf("offset:%d, iResult:%d, totalSize:%d\n", offset, iResult, totalSize);
	return offset;
}

void UTFocusSocketSender::Recv(std::vector<Packet>& packets)
{
	if (!socket)
		return;

	int32 iResult;
	uint8 recvbuf[512];

	if (socket->Recv(recvbuf, 512, iResult))
	{
		offset = MakePackets((unsigned char*)recvbuf, iResult, offset, &packets, outBuf, totalSize, header);
	}
}

void UTFocusSocketSender::Disconnect()
{
	if (socket != NULL)
	{
		socket->Close();
	}
	if (outBuf != NULL)
	{
		delete[] outBuf;
		outBuf = NULL;
	}
}

UTFocusSocketSender::~UTFocusSocketSender()
{
	Disconnect();
}

UTFocusCaptureScreen::UTFocusCaptureScreen(int width, int height, bool isAA, void* userData)
	:FocusCaptureScreenBase(width, height, isAA, userData)
	,capWidth(width)
	,capHeight(height)
	,isAntiAliasing(isAA)
{
	AUTCharacter* chara = (AUTCharacter*)userData;
	UCameraComponent* cam = chara->GetCameraComponent();
	if (cam != NULL)
	{
		capture = NewObject<USceneCaptureComponent2D>();
		renderTarget = NewObject<UTextureRenderTarget2D>();

		renderTarget->CompressionSettings = TextureCompressionSettings::TC_Default;
		renderTarget->SRGB = false;
		renderTarget->bAutoGenerateMips = false;
		renderTarget->AddressX = TextureAddress::TA_Clamp;
		renderTarget->AddressY = TextureAddress::TA_Clamp;

		capture->AntiAliasing = isAntiAliasing;
		capture->FOVAngle = 100;
		capture->bCaptureEveryFrame = true;
		capture->bCaptureOnMovement = true;
		capture->AttachToComponent(cam, FAttachmentTransformRules::KeepRelativeTransform);
		capture->SetRelativeLocation(FVector::ZeroVector);
		capture->ShowFlags.EnableAdvancedFeatures();
		capture->PostProcessBlendWeight = 1.0f;
		capture->PostProcessSettings.SetBaseValues();
		capture->HideActorComponents(chara);
		/*
		capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Basic;
		capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
		capture->PostProcessSettings.AutoExposureMinBrightness = 0.03f;
		capture->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		capture->PostProcessSettings.AutoExposureMaxBrightness = 2.0f;
		capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		capture->PostProcessSettings.AutoExposureSpeedUp = 3.0f;
		capture->PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
		capture->PostProcessSettings.AutoExposureSpeedDown = 1.0f;
		capture->PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
		capture->PostProcessSettings.AutoExposureBias = 0.0f;
		capture->PostProcessSettings.bOverride_AutoExposureBias = true;
		capture->PostProcessSettings.AutoExposureLowPercent = 80;
		capture->PostProcessSettings.bOverride_AutoExposureLowPercent = true;
		capture->PostProcessSettings.AutoExposureHighPercent = 98;
		capture->PostProcessSettings.bOverride_AutoExposureHighPercent = true;
		capture->PostProcessSettings.HistogramLogMin = -8;
		capture->PostProcessSettings.bOverride_HistogramLogMin = true;
		capture->PostProcessSettings.HistogramLogMax = 4;
		capture->PostProcessSettings.bOverride_HistogramLogMax = true;
		capture->PostProcessSettings.AutoExposureCalibrationConstant = 16;
		capture->PostProcessSettings.bOverride_AutoExposureCalibrationConstant = true;
		*/
		renderTarget->InitCustomFormat(width, height, EPixelFormat::PF_FloatRGBA, true);
		renderTarget->TargetGamma = GEngine->GetDisplayGamma();

		capture->TextureTarget = renderTarget;
		capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

		capture->RegisterComponentWithWorld(cam->GetWorld());
	}
	camera = cam;
}

UTFocusCaptureScreen::~UTFocusCaptureScreen()
{
	capture = NULL;
	renderTarget = NULL;
}

void UTFocusCaptureScreen::Update()
{
	if (!capture)
		return;

	//capture->UpdateContent();
	//capture->CaptureScene();
}

static bool WritePixelsToArray(UTextureRenderTarget2D &RenderTarget, TArray<FColor> &BitMap)
{
	check(IsInGameThread());
	FTextureRenderTargetResource *RTResource = RenderTarget.GameThread_GetRenderTargetResource();
	if (RTResource == nullptr)
	{
		return false;
	}
	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(false);
	return RTResource->ReadPixels(BitMap, ReadPixelFlags);
}

/*
static TUniquePtr<TImagePixelData<FColor>> DumpPixels(UTextureRenderTarget2D &RenderTarget)
{
	const FIntPoint DestSize(RenderTarget.GetSurfaceWidth(), RenderTarget.GetSurfaceHeight());
	TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(DestSize);
	if (!WritePixelsToArray(RenderTarget, PixelData->Pixels))
	{
		return nullptr;
	}
	return PixelData;
}
*/

struct LockTexture
{
	LockTexture(FRHITexture2D *InTexture, uint32 &Stride)
		: Texture(InTexture),
		Source(reinterpret_cast<const uint8 *>(
			RHILockTexture2D(Texture, 0, RLM_ReadOnly, Stride, false))) {}

	~LockTexture()
	{
		RHIUnlockTexture2D(Texture, 0, false);
	}

	FRHITexture2D *Texture;

	const uint8 *Source;
};

unsigned char* UTFocusCaptureScreen::CaptureScreenToMemory(unsigned int& size)
{
	size = 0;
	if (!capture)
		return NULL;

	capture->CaptureScene();

	check(IsInRenderingThread());

	FRHITexture2D *Texture = renderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	checkf(Texture != nullptr, TEXT("UTextureRenderTarget2D missing render target texture"));

	const uint32 BytesPerPixel = 4u; // PF_R8G8B8A8
	const uint32 Width = Texture->GetSizeX();
	const uint32 Height = Texture->GetSizeY();
	const uint32 ExpectedStride = Width * BytesPerPixel;

	uint32 SrcStride;
	LockTexture Lock(Texture, SrcStride);

	size = ExpectedStride * Height;
	unsigned char* ret = new unsigned char[ExpectedStride * Height];
	if (IsD3DPlatform(GMaxRHIShaderPlatform, false) && (ExpectedStride != SrcStride))
	{
		auto DstRow = ret;
		const uint8 *SrcRow = Lock.Source;
		for (uint32 Row = 0u; Row < Height; ++Row)
		{
			FMemory::Memcpy(DstRow, SrcRow, ExpectedStride);
			DstRow += ExpectedStride;
			SrcRow += SrcStride;
		}
	}
	else
	{
		check(ExpectedStride == SrcStride);
		const uint8 *Source = Lock.Source;
		FMemory::Memcpy(ret, Source, ExpectedStride * Height);
	}
	return ret;
}

bool UTFocusCaptureScreen::CaptureScreenToDisk(const char* path)
{
	if (!capture)
		return false;

	capture->CaptureScene();

	FString FilePath(path);
	if( !isAntiAliasing )
		FilePath = FString::Printf(TEXT("%d_%d_%d.bmp"), capWidth, capHeight, GFrameNumber);
	else
		FilePath = FString::Printf(TEXT("%d_%d_%d_TAA.bmp"), capWidth, capHeight, GFrameNumber);
/*
	TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
	TUniquePtr<TImagePixelData<FColor>> PixelData = DumpPixels(*renderTarget);
	ImageTask->PixelData = MoveTemp(PixelData);
	ImageTask->Filename = FilePath;
	ImageTask->Format = EImageFormat::BMP;
	ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Uncompressed;
	ImageTask->bOverwriteFile = true;
	ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
*/
	TArray<FColor> OutBMP;
	WritePixelsToArray(*renderTarget, OutBMP);
	for (FColor& color : OutBMP)
	{
		color.A = 255;
	}
	FIntRect SourceRect;
	FIntPoint DestSize(renderTarget->GetSurfaceWidth(), renderTarget->GetSurfaceHeight());

	FString ResultPath;
	FHighResScreenshotConfig &HighResScreenshotConfig = GetHighResScreenshotConfig();
	HighResScreenshotConfig.SetHDRCapture(false);
	return HighResScreenshotConfig.SaveImage(FilePath, OutBMP, DestSize, &ResultPath, true);
	//return (HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask))).Get();
}

bool UTFocusCaptureScreen::CaptureUIToDisk(const char* path)
{
	TSharedPtr<SViewport> GameViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
	check(GameViewportWidget.IsValid());
	TSharedPtr<SWidget> spSWidget = GameViewportWidget->GetContent();
	check(spSWidget.IsValid());
	TSharedPtr<FWidgetRenderer> WidgetRenderer = MakeShareable(new FWidgetRenderer(true));
	check(WidgetRenderer.IsValid());
	FVector2D offset = GEngine->GameViewport->GetGameViewport()->GetCachedGeometry().LocalToAbsolute(FVector2D(0, 0));

	UTextureRenderTarget2D *TextureRenderTarget = WidgetRenderer->DrawWidget(spSWidget.ToSharedRef(), FVector2D(capWidth, capHeight));

	FString FilePath(path);
	FilePath = FString::Printf(TEXT("%d_%d_%d_UI.bmp"), capWidth, capHeight, GFrameNumber);
/*
	TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
	TUniquePtr<TImagePixelData<FColor>> PixelData = DumpPixels(*TextureRenderTarget);
	ImageTask->PixelData = MoveTemp(PixelData);
	ImageTask->Filename = FilePath;
	ImageTask->Format = EImageFormat::BMP;
	ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Uncompressed;
	ImageTask->bOverwriteFile = true;
	ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
*/
	TArray<FColor> OutBMP;
	WritePixelsToArray(*TextureRenderTarget, OutBMP);
	for (FColor& color : OutBMP)
	{
		color.A = 255;
	}
	FIntRect SourceRect;
	FIntPoint DestSize(TextureRenderTarget->GetSurfaceWidth(), TextureRenderTarget->GetSurfaceHeight());

	TextureRenderTarget->ConditionalBeginDestroy();
	WidgetRenderer.Reset();

	FString ResultPath;
	FHighResScreenshotConfig &HighResScreenshotConfig = GetHighResScreenshotConfig();
	HighResScreenshotConfig.SetHDRCapture(false);
	return HighResScreenshotConfig.SaveImage(FilePath, OutBMP, DestSize, &ResultPath, true);
	//return (HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask))).Get();
}

void UTFocusScreenPercentage::SetScreenPercentage(float percentage)
{
	auto ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	ScreenPercentageCVar->Set(percentage);
}