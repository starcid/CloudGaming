#ifndef __FOCUS_DATA_H__
#define __FOCUS_DATA_H__

#include <stdlib.h>
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <algorithm>

struct FocusRectInfo
{
	int priority;	// 255 dynamic, 128 ui, 0 --- 127 static
	float left;
	float top;
	float right;
	float bottom;
	float distToCam;
};

struct FocusInfo
{
	std::vector<FocusRectInfo> rectInfos;
	float camPos[3];
	float camRot[3];
	bool sceneJumped;
};

extern unsigned char* Serialize(std::vector<FocusRectInfo*>* rectInfos, float* camPos, float* camRot, bool sceneJumped, unsigned  int* outSize);	/// use delete[] to free buffer memory
extern bool Deserialize(unsigned char* buf, unsigned int size, FocusInfo* outInfo);

#endif // !__FOCUS_DATA_H__
