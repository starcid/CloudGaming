#ifndef __FOCUS_TRACE_SYSTEM_H__
#define __FOCUS_TRACE_SYSTEM_H__

#include <stdlib.h>
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <algorithm>
#include "FocusTracer.h"

struct ResParam {
	int width;
	int height;
	bool isAA;
};

class FocusTraceSystem
{
	static FocusTraceSystem* instance;
	FocusTraceSystem();
public:
	static FocusTraceSystem* Instance();
	void Release();

	void Update(float DeltaSeconds);
	void OnDrawHud();

	void Register(FocusTracerBase* tracer);
	void UnRegister(FocusTracerBase* tracer);

	void SetUITracer(FocusUITracerBase* tracer) 
	{
		if (uiTracer != NULL) 
			delete uiTracer;
		uiTracer = tracer;
	}
	void SetDraw(FocusDrawBase* draw) 
	{ 
		if (drawer != NULL)
			delete drawer;
		drawer = draw; 
	}
	void SetCamera(FocusCameraBase* cam) 
	{ 
		if (camera != NULL)
			camera = NULL;
		camera = cam; 
	}
	void SetScreenPerHandle(FocusScreenPercentageBase* screenPer)
	{
		if (screenPercentage != NULL)
			delete screenPercentage;
		screenPercentage = screenPer;
	}
	void SetSceneJumpd() { sceneJumped = true; }
	void SetSender(FocusSocketSenderBase* s) 
	{ 
		if (sender != NULL)
		{
			sender->Disconnect();
			delete sender;
		}
		sender = s; 
	}

	void InitializeCapture();
	void StartCaptureScreen( void* userData );
	void ClearCaptureScreen();
	void SetCaptureInterval(float interval)
	{
		captureInterval = interval;
	}

	void AddRectInfo(int prio, float left, float top, float right, float bottom, float dist = 0.0f);
	std::vector<FocusRectInfo*>* GetRectInfos() { return &rectInfos; }
	bool GetCameraPosition(float* outPos);
	bool GetCameraRotation(float* outRot);	/// stored in degree
	bool IsSceneJumped() { return sceneJumped; }

	void SetScreenPercentage(float per);

private:
	void RetriveAndSendDatas();

	int CheckOverlapped(FocusRectInfo* a, FocusRectInfo* b);
	int CheckAndClipRect(FocusRectInfo* front, FocusRectInfo* back, FocusRectInfo* temp );

private:
	std::vector<FocusTracerBase*> tracers;
	FocusUITracerBase* uiTracer;
	FocusDrawBase* drawer;
	FocusCameraBase* camera;
	bool sceneJumped;

	float captureInterval;
	std::vector<ResParam> resParams;
	std::vector<FocusCaptureScreenBase*> captures;

	std::vector<FocusRectInfo*> rectInfos;

	FocusSocketSenderBase* sender;
	FocusScreenPercentageBase* screenPercentage;

	float timer;
};

#endif	/*__FOCUS_TRACE_SYSTEM_H__*/