#ifndef __UT_FOCUS_TRACER_H__
#define	__UT_FOCUS_TRACER_H__

#include "../../FocusTrace/FocusTracer.h"
#include "GameFramework/Actor.h"
#include "Public/Sockets.h"

class UTFocusTracer : public FocusTracerBase
{
public:
	UTFocusTracer(AActor* aactor, uint8 p, FString key);
	virtual ~UTFocusTracer() {}

	virtual FocusRectInfo* UpdateRectInfo();

	static void UpdateUIRect(std::vector<FocusTracerBase*>& rectInfos);

private:
	bool UpdateBounds();

	AActor* actor;
	uint8 priority;
	FString keyName;
	APlayerController* player;

	UPrimitiveComponent* primComp;
	FBoxSphereBounds localBound;
	bool localBoundCalculated;

	FVector bounds[8];
	bool boundsUpdated;
};

class UTFocusUITracer : public FocusUITracerBase
{
public:
	UTFocusUITracer() {}
	virtual ~UTFocusUITracer() {}

	void UpdateUIRectInfo(std::vector<FocusRectInfo*>& rectInfos);

private:
	void GoThroughChildren(SWidget* parent, std::vector<FocusRectInfo*>& rectInfos, FVector2D& offset, int level);
	float GetTextOpacity(STextBlock* text);
	float GetImageOpacity(SImage* image);
};

class UTFocusDraw : public FocusDrawBase
{
public:
	UTFocusDraw();
	virtual ~UTFocusDraw() {}

	virtual void DrawRect(float left, float right, float top, float bottom, uint8 prio);
};

class UTFocusCamera : public FocusCameraBase
{
public:
	UTFocusCamera() {}
	virtual ~UTFocusCamera() {}

	virtual bool GetPosition(float* outPos);
	virtual bool GetRotation(float* outRot);
};

class UTFocusSocketSender : public FocusSocketSenderBase
{
public:
	UTFocusSocketSender() { socket = NULL; }
	virtual ~UTFocusSocketSender();

	virtual bool Connect();
	virtual bool Connect(const char* ipAddr, int port);
	virtual bool IsConnected();
	virtual bool Send(unsigned char* buf, unsigned int size);
	virtual void Recv(std::vector<Packet>& packets);
	virtual void Disconnect();

private:
	/// recv param
	unsigned int offset;
	unsigned char* outBuf;
	unsigned int totalSize;
	unsigned char header[4];

	FSocket* socket;
};

class UTFocusCaptureScreen : public FocusCaptureScreenBase
{
public:
	UTFocusCaptureScreen(int width, int height, bool isAA, void* userData);
	virtual ~UTFocusCaptureScreen();

	virtual void Update();

	virtual unsigned char* CaptureScreenToMemory(unsigned int& size);
	virtual bool CaptureScreenToDisk(const char* path);
	virtual bool CaptureUIToDisk(const char* path);

private:
	int capWidth;
	int capHeight;
	bool isAntiAliasing;

	USceneCaptureComponent2D *capture;
	UTextureRenderTarget2D *renderTarget;

	UCameraComponent* camera;
};

class UTFocusScreenPercentage : public FocusScreenPercentageBase
{
public:
	UTFocusScreenPercentage() {}
	virtual ~UTFocusScreenPercentage() {}

	virtual void SetScreenPercentage(float percentage);
};

#endif	/*__UT_FOCUS_TRACER_H__*/