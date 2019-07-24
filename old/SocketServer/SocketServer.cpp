// SocketServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

/*
	Bind socket to port 8888 on localhost
*/

#include<io.h>
#include<stdio.h>
#include<winsock2.h>
#include <vector>
#include <time.h>

#include "../SurvivalGame 4.22/Source/SurvivalGame/ThirdParty/CloudImp/Server/FocusData.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

struct Packet 
{
	Packet(unsigned char* b, unsigned int s)
	{
		buf = b;
		size = s;
	}
	unsigned char* buf;
	unsigned int size;
};

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

int main(int argc, char *argv[])
{
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int c;

	int port = 8888;
	int interval = 5;
	bool autoChangePercentage = false;
	if (argc == 2)
	{
		port = atoi(argv[1]);
	}
	else if (argc == 3)
	{
		port = atoi(argv[1]);
		interval = atoi(argv[2]);
	}
	else if (argc == 4)
	{
		port = atoi(argv[1]);
		interval = atoi(argv[2]);
		if (argv[3] == "true")
		{
			autoChangePercentage = true;
		}
	}

	printf("Initialising focus trace server with port:%d\n", port);
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed.Error Code : %d" , WSAGetLastError());
		return 1;
	}

	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return 1;
	}

	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
	}

	puts("Bind done");

	//Listen to incoming connections
	listen(s, 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");

	c = sizeof(struct sockaddr_in);
	new_socket = accept(s, (struct sockaddr *)&client, &c);
	if (new_socket == INVALID_SOCKET)
	{
		printf("accept failed with error code : %d", WSAGetLastError());
	}

	puts("Connection accepted");

	int iResult;
	char recvbuf[512];
	unsigned int offset = 0;
	unsigned char* outBuf = NULL;
	unsigned int totalSize = 0;
	unsigned char header[4];
	time_t now = time(0);
	int count = 0;
	/// receiving data
	do {
		iResult = recv(new_socket, recvbuf, 512, 0);
		if (iResult > 0)
		{
			std::vector<Packet> outPackets;
			offset = MakePackets((unsigned char*)recvbuf, iResult, offset, &outPackets, outBuf, totalSize, header);
			if (outPackets.size() > 0)
			{
				for (int i = 0; i < outPackets.size(); i++)
				{
					FocusInfo info;
					Deserialize(outPackets[i].buf, outPackets[i].size, &info);

					/// record data
					time_t new_time = time(0);
					if (new_time - now > interval)
					{
						for (int i = 0; i < info.rectInfos.size(); i++)
						{
							FocusRectInfo* rectInfo = &info.rectInfos[i];
							printf("Rectangle[%d]: Priority(%d) left:(%.1f) right:(%.1f) top:(%.1f) bottom:(%.1f) distance:(%.1f)\n", i, rectInfo->priority, rectInfo->left, rectInfo->right, rectInfo->top, rectInfo->bottom, rectInfo->distToCam);
						}
						printf("Camera Position:%.1f %.1f %.1f\n", info.camPos[0], info.camPos[1], info.camPos[2]);
						printf("Camera Rotation:%.1f %.1f %.1f\n", info.camRot[0], info.camRot[1], info.camRot[2]);
						printf("Scene Jumped:%d\n", info.sceneJumped);

						now = new_time;

						if (autoChangePercentage)
						{
							unsigned int sendData[2];
							sendData[0] = 4;	/// data size 4
							float* percentage = (float*)sendData[1];
							*percentage = 100.0f - 10.0f * (count % 6);
							send(new_socket, (char*)sendData, 8, 0);
							count++;
						}
					}

					delete[] outPackets[i].buf;
				}
			}
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed: %d\n", WSAGetLastError());

	} while (iResult > 0);

	if (outBuf != NULL)
	{
		delete[] outBuf;
	}
	closesocket(s);
	WSACleanup();

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
