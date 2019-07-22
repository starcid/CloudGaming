#include "UnrealTournament.h"
#include "FocusData.h"
#include "assert.h"

#define LENGTH_INCREMENT(len, offset) \
	len = len + offset;				  \
	assert( len <= MAX_BUGGER_LENGTH );

#define LENGTH_CHECK(len, offset, max) \
	len = len + offset;				   \
	if( len > max )					   \
	{								   \
		return false;				   \
	}

#define MAX_BUGGER_LENGTH (2*1024*1024)

unsigned char* Serialize(std::vector<FocusRectInfo*>* rectInfos, float* camPos, float* camRot, bool sceneJumped, unsigned  int* outSize)
{
	unsigned char* buffer = new unsigned char[MAX_BUGGER_LENGTH];
	unsigned char* buf = buffer;
	unsigned int totalLength = 0;

	*((int *)buf) = rectInfos->size();
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	std::vector<FocusRectInfo*>::iterator iter;
	for (iter = rectInfos->begin(); iter != rectInfos->end(); iter++)
	{
		FocusRectInfo* rectInfo = *iter;
		*((int *)buf) = rectInfo->priority;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
		*((float *)buf) = rectInfo->left;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
		*((float *)buf) = rectInfo->top;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
		*((float *)buf) = rectInfo->right;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
		*((float *)buf) = rectInfo->bottom;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
		*((float *)buf) = rectInfo->distToCam;
		buf += 4;
		LENGTH_INCREMENT(totalLength, 4);
	}
	*((float *)buf) = camPos[0];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((float *)buf) = camPos[1];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((float *)buf) = camPos[2];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((float *)buf) = camRot[0];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((float *)buf) = camRot[1];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((float *)buf) = camRot[2];
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);
	*((bool *)buf) = sceneJumped;
	buf += 4;
	LENGTH_INCREMENT(totalLength, 4);

	*outSize = totalLength;
	unsigned char* ret = new unsigned char[totalLength];
	memcpy(ret, buffer, totalLength);
	delete[] buffer;

	return ret;
}

bool Deserialize(unsigned char* buf, unsigned int size, FocusInfo* outInfo)
{
	outInfo->rectInfos.clear();
	unsigned int totalLength = 0;
	int rectInfosSize = *((int *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	for (int i = 0; i < rectInfosSize; i++)
	{
		FocusRectInfo rectInfo;
		rectInfo.priority = *((int *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		rectInfo.left = *((float *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		rectInfo.top = *((float *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		rectInfo.right = *((float *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		rectInfo.bottom = *((float *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		rectInfo.distToCam = *((float *)buf);
		buf += 4;
		LENGTH_CHECK(totalLength, 4, size);
		outInfo->rectInfos.push_back(rectInfo);
	}
	outInfo->camPos[0] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->camPos[1] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->camPos[2] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->camRot[0] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->camRot[1] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->camRot[2] = *((float *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);
	outInfo->sceneJumped = *((bool *)buf);
	buf += 4;
	LENGTH_CHECK(totalLength, 4, size);

	return true;
}