#include "UnrealTournament.h"
#include "FocusTraceSystem.h"
#include "../Implement/UE4/UTFocusTracer.h"

FocusTraceSystem* FocusTraceSystem::instance = NULL;

FocusTraceSystem* FocusTraceSystem::Instance()
{
	if (instance == NULL)
	{
		instance = new FocusTraceSystem();
	}
	return instance;
}

void FocusTraceSystem::Release()
{
	if (uiTracer != NULL)
		delete uiTracer;
	if (drawer != NULL)
		delete drawer;
	if (camera != NULL)
		delete camera;
	if (sender != NULL)
	{
		sender->Disconnect();
		delete sender;
	}
	if (screenPercentage != NULL)
		delete screenPercentage;
	uiTracer = NULL;
	drawer = NULL;
	camera = NULL;
	sender = NULL;
	screenPercentage = NULL;

	std::vector<FocusRectInfo*>::iterator rectInfoIter;
	for (rectInfoIter = rectInfos.begin(); rectInfoIter != rectInfos.end(); rectInfoIter++)
	{
		delete (*rectInfoIter);
	}
	rectInfos.clear();

	std::vector<FocusTracerBase*>::iterator iter;
	for (iter = tracers.begin(); iter != tracers.end(); iter++)
	{
		delete (*iter);
	}
	tracers.clear();

	ClearCaptureScreen();
}

FocusTraceSystem::FocusTraceSystem()
{
	uiTracer = NULL;
	drawer = NULL;
	camera = NULL;
	sceneJumped = false;
	sender = NULL;
	screenPercentage = NULL;
	timer = 0;
}

void FocusTraceSystem::SetScreenPercentage(float per)
{
	if (screenPercentage != NULL)
	{
		screenPercentage->SetScreenPercentage(per);
	}
}

static bool compareRectDist(FocusRectInfo* i1, FocusRectInfo* i2)
{
	return (i1->distToCam < i2->distToCam);
}

void FocusTraceSystem::Update(float DeltaSeconds)
{
	timer += DeltaSeconds;

	/// collect valid rect info
	std::vector<FocusTracerBase*>::iterator iter;
	for (iter = tracers.begin(); iter != tracers.end(); iter++)
	{
		FocusRectInfo* pRectInfo = (*iter)->UpdateRectInfo();
		if (pRectInfo != NULL)
		{
			rectInfos.push_back(pRectInfo);
		}
	}

	/// capture screen
	std::vector<FocusCaptureScreenBase*>::iterator capIter;
	for (capIter = captures.begin(); capIter != captures.end(); capIter++)
	{
		(*capIter)->Update();
	}
	if (timer >= captureInterval)
	{
		for (capIter = captures.begin(); capIter != captures.end(); capIter++)
		{
			(*capIter)->CaptureScreenToDisk("D:\\Screen");
			///(*capIter)->CaptureUIToDisk("D:\\Screen");
		}
		timer = 0.0f;
	}

	/// collect ui info
	if (uiTracer)
	{
		uiTracer->UpdateUIRectInfo(rectInfos);
	}

	/// process datas
	RetriveAndSendDatas();

	/// restore scene jumped
	sceneJumped = false;
}

void FocusTraceSystem::OnDrawHud()
{
	/// draw in hud
	if (drawer != NULL && rectInfos.size() > 0)
	{
		for (std::vector<FocusRectInfo*>::iterator rectInfoIter = rectInfos.begin(); rectInfoIter != rectInfos.end(); rectInfoIter++)
		{
			FocusRectInfo* info = *rectInfoIter;
			drawer->DrawRect(info->left, info->right, info->top, info->bottom, info->priority);
		}
	}

	/// clear
	std::vector<FocusRectInfo*>::iterator rectInfoIter;
	for (rectInfoIter = rectInfos.begin(); rectInfoIter != rectInfos.end(); rectInfoIter++)
	{
		delete (*rectInfoIter);
	}
	rectInfos.clear();
}

void FocusTraceSystem::AddRectInfo(int prio, float left, float top, float right, float bottom, float dist)
{
	FocusRectInfo* rectInfo = new FocusRectInfo();
	rectInfo->distToCam = dist;
	rectInfo->priority = prio;
	rectInfo->left = left;
	rectInfo->right = right;
	rectInfo->top = top;
	rectInfo->bottom = bottom;
	rectInfos.push_back(rectInfo);
}

bool FocusTraceSystem::GetCameraPosition(float* outPos)
{
	if (camera != NULL)
	{
		return camera->GetPosition(outPos);
	}
	return false;
}

bool FocusTraceSystem::GetCameraRotation(float* outRot)
{
	if (camera != NULL)
	{
		return camera->GetRotation(outRot);
	}
	return false;
}

void FocusTraceSystem::InitializeCapture()
{
	const TCHAR* CmdLineParam = FCommandLine::Get();
	FString resParam;
	resParams.clear();
	if (!FParse::Value(CmdLineParam, TEXT("-capres="), resParam))
	{
		return;
	}
	FString left, right;
	FString width, height;
	FString isAAStr, resStr;
	bool isAA;
	while (resParam.Split(TEXT("|"), &left, &right))
	{
		isAA = false;
		if (left.Split(TEXT("_"), &resStr, &isAAStr))
		{
			left = resStr;
			if (isAAStr == TEXT("AA"))
			{
				isAA = true;
			}
		}
		if (left.Split(TEXT("*"), &width, &height))
		{
			ResParam p;
			p.width = FCString::Atoi(*width);
			p.height = FCString::Atoi(*height);
			p.isAA = isAA;
			resParams.push_back(p);
		}
		resParam = right;
	}
	isAA = false;
	if (left.Split(TEXT("_"), &resStr, &isAAStr))
	{
		left = resStr;
		if (isAAStr == TEXT("AA"))
		{
			isAA = true;
		}
	}
	if (resParam.Split(TEXT("*"), &width, &height))
	{
		ResParam p;
		p.width = FCString::Atoi(*width);
		p.height = FCString::Atoi(*height);
		p.isAA = isAA;
		resParams.push_back(p);
	}

	FString intervalParam;
	if (!FParse::Value(CmdLineParam, TEXT("-capinterval="), intervalParam))
	{
		intervalParam = TEXT("5");
	}
	SetCaptureInterval(FCString::Atoi(*intervalParam));
}

void FocusTraceSystem::StartCaptureScreen( void* userData )
{
	for (int i = 0; i < resParams.size(); i++)
	{
		captures.push_back(new UTFocusCaptureScreen(resParams[i].width, resParams[i].height, resParams[i].isAA, userData));
	}
}

void FocusTraceSystem::ClearCaptureScreen()
{
	std::vector<FocusCaptureScreenBase*>::iterator capIter;
	for (capIter = captures.begin(); capIter != captures.end(); capIter++)
	{
		delete (*capIter);
	}
	captures.clear();
}

void FocusTraceSystem::RetriveAndSendDatas()
{
	if (sender != NULL && sender->IsConnected())
	{
		unsigned int size = 0;
		float camPos[3];
		float camRot[3];
		GetCameraPosition(camPos);
		GetCameraRotation(camRot);
		unsigned char* buf = Serialize(GetRectInfos(), camPos, camRot, IsSceneJumped(), &size);
		if (buf != NULL && size > 0)
		{
			sender->Send(buf, size);
			delete[] buf;
		}
	}
}

int FocusTraceSystem::CheckOverlapped(FocusRectInfo* a, FocusRectInfo* b)
{
	return !(a->left >= b->right || a->right <= b->left || a->top >= b->bottom || a->bottom <= b->top);
}

int FocusTraceSystem::CheckAndClipRect(FocusRectInfo* front, FocusRectInfo* back, FocusRectInfo* temp)
{
	int ret = -1;
	if (CheckOverlapped(front, back) == 0)
		return ret;

	/// overlapped rect
	ret = 0;
	float left, top, right, bottom;
	left = fmaxf(back->left, front->left);
	top = fmaxf(back->top, front->top);
	right = fminf(back->right, front->right);
	bottom = fminf(back->bottom, front->bottom);

	/// clip along each edge
	float remainLeft = back->left;
	float remainRight = back->right;
	float remainTop = back->top;
	float remainBottom = back->bottom;
	/// top
	if (top < remainBottom && top > remainTop)
	{
		temp[ret].top = remainTop;
		temp[ret].bottom = top;
		remainTop = top;
		temp[ret].left = remainLeft;
		temp[ret].right = remainRight;
		temp[ret].distToCam = back->distToCam;
		ret++;
	}
	/// bottom
	if (bottom < remainBottom && bottom > remainTop)
	{
		temp[ret].top = bottom;
		temp[ret].bottom = remainBottom;
		remainBottom = bottom;
		temp[ret].left = remainLeft;
		temp[ret].right = remainRight;
		temp[ret].distToCam = back->distToCam;
		ret++;
	}
	/// left
	if (left < remainRight && left > remainLeft)
	{
		temp[ret].left = remainLeft;
		temp[ret].right = left;
		remainLeft = left;
		temp[ret].top = remainTop;
		temp[ret].bottom = remainBottom;
		temp[ret].distToCam = back->distToCam;
		ret++;
	}
	/// right
	if (right < remainRight && right > remainLeft)
	{
		temp[ret].left = right;
		temp[ret].right = remainRight;
		remainRight = right;
		temp[ret].top = remainTop;
		temp[ret].bottom = remainBottom;
		temp[ret].distToCam = back->distToCam;
		ret++;
	}

	return ret;
}

void FocusTraceSystem::Register(FocusTracerBase* tracer)
{
	tracers.push_back(tracer);
}

void FocusTraceSystem::UnRegister(FocusTracerBase* tracer)
{
	std::vector<FocusTracerBase*>::iterator iter;
	for (iter = tracers.begin(); iter != tracers.end(); iter++)
	{
		if (*iter == tracer)
		{
			tracers.erase(iter);
			delete tracer;
			break;
		}
	}
}