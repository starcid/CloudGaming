#ifndef __FOCUS_TRACER_H__
#define __FOCUS_TRACER_H__

#include "../Server/FocusData.h"

class FocusTracerBase
{
public:
	FocusTracerBase() {}
	virtual ~FocusTracerBase() {}

	virtual FocusRectInfo* UpdateRectInfo() = 0;
};

class FocusUITracerBase
{
public:
	FocusUITracerBase() {}
	virtual ~FocusUITracerBase() {}

	virtual void UpdateUIRectInfo(std::vector<FocusRectInfo*>& rectInfos) = 0;
};

class FocusDrawBase
{
public:
	FocusDrawBase() { isDisplay = false; }
	virtual ~FocusDrawBase() {}

	virtual void DrawRect(float left, float right, float top, float bottom, uint8 priority) = 0;

protected:
	bool isDisplay;
};

class FocusCameraBase
{
public:
	FocusCameraBase() {}
	virtual ~FocusCameraBase() {}

	virtual bool GetPosition(float* outPos) = 0;
	virtual bool GetRotation(float* outRot) = 0;
};

class FocusSocketSenderBase
{
public:
	FocusSocketSenderBase() {}
	virtual ~FocusSocketSenderBase() {}

	virtual bool Connect() = 0;
	virtual bool Connect(const char* ipAddr, int port) = 0;
	virtual bool IsConnected() = 0;
	virtual bool Send(unsigned char* buf, unsigned int size) = 0;
	virtual void Disconnect() = 0;
};

class FocusCaptureScreenBase
{
public:
	FocusCaptureScreenBase(int width, int height, void* userData) {}
	virtual ~FocusCaptureScreenBase() {}

	virtual void Update() = 0;

	virtual unsigned char* CaptureScreenToMemory( unsigned int& size ) = 0;
	virtual bool CaptureScreenToDisk(const char* path) = 0;
	virtual bool CaptureUIToDisk(const char* path) = 0;
};

#endif/*__FOCUS_TRACER_H__*/