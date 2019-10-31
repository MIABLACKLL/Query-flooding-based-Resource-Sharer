#pragma once
#include<iostream>
#include<cstring>
#include<string>
#include<future>
#include<ctime>
#include<algorithm>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include"FileManagement.h"
#include"Config.h"

#pragma comment(lib,"Ws2_32.lib")
constexpr int MAXLISTEN = 10;//最大PEER数量
constexpr int MAXIPLENGTH = 16;
constexpr int MAXPORTLENGTH = 6;
constexpr int MAXPEERLENGTH = 64;
constexpr int MAXFILENAME = 256;
constexpr int MAXCOMMANDPACKET = 924;

constexpr int FILLRESULTPACKET = 372;//让查询包和结果包等大，以便接收（有点毒瘤）
constexpr int FILLQUERYPACKET = 4;
enum PacketType { QueryPacket, ResultPacket };

struct SQueryPacket
{
	int Type = QueryPacket;
	char SenderIP[MAXIPLENGTH];
	char PassPeer[MAXLISTEN][MAXPEERLENGTH];//记录传递过程中经过的PEER，前16字节IP，17-22字节命令端口，22字节往后记录PEERID（似乎有些浪费）
	int SenderDataPort;
	int SenderCommandPort;
	char FileName[MAXFILENAME];
	time_t SendTime;
	char EmptyBuffer[FILLQUERYPACKET];
};

struct SResultPacket
{
	int Type = ResultPacket;
	char RecvIP[MAXIPLENGTH];
	int RecvDataPort;
	int RecvCommandPort;
	bool IsExistOnline;
	SFile File;
	time_t SendTime;
	char EmptyBuffer[FILLRESULTPACKET];
};

//fixme:不支持多线程，先完成基本功能，有空写个稍微完备的线程池。
class CQueryFlooding
{
public:
	CQueryFlooding()=default;
	CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement); 
	~CQueryFlooding() { closesocket(m_ServerCommandSock); WSACleanup(); }

	[[nodiscard]]bool listenCommandPort();
	[[nodiscard]] SFile queryFileLocal(std::string vFileName);//收到远端查询请求时，先在本地进行查询，找不到继续发起洪泛。

	void receiveBuffer(std::promise<SResultPacket> &vSResultPacket);//先对接受到的包判断（两种类型），然后再调用不同处理方式
																		//收到查询请求时，交给下层处理。收到查询结果时，返回，主进程需要再次调用。
	void requestQuery(std::string vFileName);
private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	WSADATA m_WSAData;
	SOCKET m_ServerCommandSock;
	std::string m_ResponsePeerID;
	std::pair<std::string, time_t> m_LastRecvFile;//区分是否是一次请求内的多PEER返回，只接受第一次

	void __receiveQueryRequest(SQueryPacket& vQueryPacket);//查到当前PEER有该文件则发生返回信息，没有则继续转发
	bool __acceptClient(SOCKET& vClientSocket ,sockaddr_in& vClientAddr);
	void __bandCommandSocket();
	void __queryFlooding(SQueryPacket& vFilePacket);
	bool __connectPeer(SOCKET &vLocalSock,std::string vIP,int vPort);
	void __sendQuery(SOCKET &vLoackSock, SQueryPacket& vFilePacket);
	void __sendResult(SOCKET &vLoackSock, SResultPacket& vFilePacket);//向查询方返回信息
	void __queryFileOnline(SQueryPacket& vFilePacket);
	bool __checkPass(SQueryPacket& vFilePacket, std::string vIP, int vPort);//检测是否往回路发
	bool __addPass(SQueryPacket& vFilePacket, std::string vIP, int vPort);//添加到转发经过的路径中
};

CQueryFlooding::CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement) :m_pPeerConfig(vConfig), m_pPeerFileSystem(vFileManagement)
{
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);
	m_ServerCommandSock = socket(AF_INET, SOCK_STREAM, 0);
	__bandCommandSocket();
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__bandCommandSocket()
{
	sockaddr_in SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET, m_pPeerConfig->getIP().c_str(), &SockAddr.sin_addr);
	SockAddr.sin_port = htons(m_pPeerConfig->getCommandPort());
	bind(m_ServerCommandSock, reinterpret_cast<sockaddr*>(&SockAddr), sizeof(SockAddr));
}

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::listenCommandPort()
{
	int SocketStatus = SOCKET_ERROR;
	if (m_ServerCommandSock != SOCKET_ERROR)
	{
		SocketStatus = listen(m_ServerCommandSock, MAXLISTEN);//设定接受连接的套接字，以及设定连接队列长度;
	}
	if (SocketStatus == SOCKET_ERROR) { return false; }
	return true;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::receiveBuffer(std::promise<SResultPacket> &vSResultPacket)
{
	while (m_ServerCommandSock != SOCKET_ERROR)
	{
		char CommandBuffer[MAXCOMMANDPACKET];
		SOCKET ClientSocket = 0;
		sockaddr_in ClientAddr;
		if (__acceptClient(ClientSocket, ClientAddr))
		{
			recv(ClientSocket, CommandBuffer, MAXCOMMANDPACKET, 0);
			closesocket(ClientSocket);
			auto PacketType = *reinterpret_cast<int*>(CommandBuffer);
			if (PacketType == QueryPacket)
			{
				__receiveQueryRequest(*reinterpret_cast<SQueryPacket*>(CommandBuffer));
			}
			else if (PacketType == ResultPacket)
			{
				auto ResultPacket = *reinterpret_cast<SResultPacket*>(CommandBuffer);
				if (strcmp(ResultPacket.File.FileName, m_LastRecvFile.first.c_str()) && ResultPacket.SendTime == m_LastRecvFile.second);
				else
				{
					m_LastRecvFile.first = ResultPacket.File.FileName;
					m_LastRecvFile.second = ResultPacket.SendTime;
					vSResultPacket.set_value_at_thread_exit(ResultPacket);
					return;
				}
			}
			else
			{
				std::cout << "Something error happen..." << std::endl;
			}
		}
	}
}

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::__acceptClient(SOCKET& vClientSocket, sockaddr_in& vClientAddr)
{
	int ClientAddrSize = sizeof(vClientAddr);
	vClientSocket = accept(m_ServerCommandSock, reinterpret_cast<sockaddr*>(&vClientAddr), &ClientAddrSize);//接受客户端连接，阻塞状态;失败返回-1
	if (vClientSocket == SOCKET_ERROR)
	{
		std::cout << "Failed to connect client." << std::endl;
		return false;
	}
	return true;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__receiveQueryRequest(SQueryPacket& vQueryPacket)
{
	auto File = queryFileLocal(vQueryPacket.FileName);
	if (File.IsExist)
	{
		SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
		if (__connectPeer(LocalSock, vQueryPacket.SenderIP, vQueryPacket.SenderCommandPort))
		{
			SResultPacket ResultPacket;
			ResultPacket.File = File;
			strcpy_s(ResultPacket.RecvIP ,m_pPeerConfig->getIP().c_str());
			ResultPacket.RecvCommandPort = m_pPeerConfig->getCommandPort();
			ResultPacket.RecvDataPort = m_pPeerConfig->getDataPort();
			ResultPacket.SendTime = vQueryPacket.SendTime;
			__sendResult(LocalSock, ResultPacket);
		}
		closesocket(LocalSock);
	}
	else
	{
		__queryFlooding(vQueryPacket);
	}
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::requestQuery(std::string vFileName)
{
	SQueryPacket FilePacket;
	strcpy_s(FilePacket.FileName,vFileName.c_str());
	strcpy_s(FilePacket.SenderIP, m_pPeerConfig->getIP().c_str());
	FilePacket.SenderCommandPort = m_pPeerConfig->getCommandPort();
	FilePacket.SenderDataPort = m_pPeerConfig->getDataPort();
	FilePacket.SendTime = time(NULL);
	memset(FilePacket.PassPeer, 0, MAXLISTEN*MAXPEERLENGTH);
	__queryFileOnline(FilePacket);
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__queryFileOnline(SQueryPacket& vFilePacket)
{
	__queryFlooding(vFilePacket);
}

//*********************************************************************
//FUNCTION:
SFile CQueryFlooding::queryFileLocal(std::string vFileName)
{
	auto voFileQueryResult = m_pPeerFileSystem->findFile(vFileName);
	//if (voFileQueryResult.second)
	//{
	//	std::cout << "Find file in local. " << voFileQueryResult.first.FilePath << std::endl;
	//}
	//else
	//{
	//	std::cout << "File not exist in local. " << voFileQueryResult.first.FilePath << std::endl;
	//}//留着，放在CUI里。
	return voFileQueryResult.first;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__queryFlooding(SQueryPacket& vFilePacket)
{
	auto ConnectPeerSocket = m_pPeerConfig->getConnectPeerSocket();
	for (auto Peer : ConnectPeerSocket)
	{
		std::string DestinationIP = std::any_cast<std::string>(Peer["IP"]);
		int DestinationPort = std::any_cast<int>(Peer["COMMANDPORT"]);
		if (__checkPass(vFilePacket, DestinationIP, DestinationPort))
		{
			SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
			if (__connectPeer(LocalSock,DestinationIP, DestinationPort))
			{
				__sendQuery(LocalSock, vFilePacket);
				_ASSERTE(__addPass(vFilePacket, DestinationIP, DestinationPort));
			}
			else
			{
				std::cout << "Failed to connect " << DestinationIP << " " << DestinationPort << std::endl;
			}
			closesocket(LocalSock);
		}
	}
}

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::__connectPeer(SOCKET &vLocalSock, std::string vIP, int vPort)
{
	int SocketStatus = SOCKET_ERROR;
	if (vLocalSock != SOCKET_ERROR)
	{
		sockaddr_in NearPeerAddr;
		memset(&NearPeerAddr, 0, sizeof(NearPeerAddr));
		NearPeerAddr.sin_family = AF_INET;
		NearPeerAddr.sin_port = htons(vPort);
		InetPton(AF_INET, vIP.c_str(), &NearPeerAddr.sin_addr);
		SocketStatus = connect(vLocalSock, reinterpret_cast<sockaddr*>(&NearPeerAddr), sizeof(NearPeerAddr));
		if (SocketStatus == SOCKET_ERROR) { return false; }
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__sendQuery(SOCKET& vLoackSock, SQueryPacket& vFilePacket)
{
	char SendBuffer[sizeof(SQueryPacket)];
	memcpy(SendBuffer, &vFilePacket, sizeof(SQueryPacket));
	send(vLoackSock, SendBuffer, sizeof(SQueryPacket), 0);
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__sendResult(SOCKET &vLoackSock, SResultPacket& vFilePacket)
{
	char SendBuffer[sizeof(SResultPacket)];
	memcpy(SendBuffer, &vFilePacket, sizeof(SResultPacket));
	send(vLoackSock, SendBuffer, sizeof(SResultPacket), 0);
}

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::__checkPass(SQueryPacket& vFilePacket, std::string vIP, int vPort)
{
	int PassPeerNum = 0;
	while (strlen(vFilePacket.PassPeer[PassPeerNum])>0)
	{
		std::string IP;
		std::string StrPort;
		IP.resize(MAXIPLENGTH);
		StrPort.resize(MAXPORTLENGTH);
		std::copy(vFilePacket.PassPeer[PassPeerNum], &(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH]), IP.begin());
		std::copy(&(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH + 1]), &(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH + MAXPORTLENGTH]), StrPort.begin());
		int Port = atoi(StrPort.c_str());
		if (IP == vIP && vPort == Port)
			return false;
		PassPeerNum++;
	}
	return true;
}
bool CQueryFlooding::__addPass(SQueryPacket& vFilePacket, std::string vIP, int vPort)
{
	int PassPeerNum = 0;
	while (strlen(vFilePacket.PassPeer[PassPeerNum]) > 0)
	{
		PassPeerNum++;
	}
	char StrPort[MAXPORTLENGTH];
	_itoa_s(vPort, StrPort, 10);
	std::copy(vIP.begin(), vIP.end(), vFilePacket.PassPeer[PassPeerNum]);
	std::copy(StrPort, StrPort + MAXPORTLENGTH, &(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH + 1]));
	return false;
}
