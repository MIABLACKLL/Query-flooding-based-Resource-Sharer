#pragma once
#include<iostream>
#include<cstring>
#include<string>
#include<future>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include"FileManagement.h"
#include"Config.h"

#pragma comment(lib,"Ws2_32.lib")
constexpr int MAXLISTEN = 10;//最大PEER数量
constexpr int MAXIPLENGTH = 16;
constexpr int MAXPEERLENGTH = 64;
constexpr int MAXFILENAME = 256;
constexpr int MAXCOMMANDPACKET = 924;

constexpr int FILLRESULTPACKET = 364;//让查询包和结果包等大以便接受（有点毒瘤）
constexpr int FILLQUERYPACKET = 4;
enum PacketType { QueryPacket, ResultPacket };

struct SFileQueryPacket
{
	int Type = QueryPacket;
	char SenderIP[MAXIPLENGTH];
	char PassPeer[MAXLISTEN][MAXPEERLENGTH];//记录传递过程中经过的PEER，前16字节IP，17-20字节命令端口，21字节往后记录PEERID（似乎有些浪费）
	int SenderDataPort;
	int SenderCommandPort;
	char FileName[MAXFILENAME];
	char EmptyBuffer[FILLQUERYPACKET];
};

struct SFileResultPacket
{
	int Type = ResultPacket;
	char RecvIP[MAXIPLENGTH];
	int RecvDataPort;
	int RecvCommandPort;
	bool IsExistOnline;
	SFile File;
	char EmptyBuffer[FILLRESULTPACKET];
};

class CQueryFlooding
{
public:
	CQueryFlooding()=default;
	CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement); 
	~CQueryFlooding() { WSACleanup(); }

	[[nodiscard]]bool listenCommandPort();
	void receiveBuffer(std::promise<SFileResultPacket> &vSFileResultPacket);//先对接受到的包判断（两种类型），然后再调用不同处理方式
																			//收到查询请求时，交给下层处理。收到查询结果时，返回，主进程需要再次调用。
	void requestQuery(std::string vFileName);
	
	[[nodiscard]] SFile queryFileLocal(std::string vFileName);//收到远端查询请求时，先在本地进行查询，找不到继续发起洪泛。

private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	int m_TimeOut = 5;
	WSADATA m_WSAData;
	SOCKET m_ServerCommandSock;
	std::string m_ResponsePeerID;

	void __receiveQueryRequest(SFileQueryPacket& vQueryPacket);//查到当前PEER有该文件则发生返回信息，没有则继续转发
	void __bandCommandSocket();
	void __queryFlooding(SFileQueryPacket& vFilePacket);
	bool __connectPeer(SOCKET &vLocalSock,std::string vIP,int vPort);
	void __sendQuery(SOCKET &vLoackSock, SFileQueryPacket& vFilePacket);
	void __sendResult(SOCKET &vLoackSock, SFileResultPacket& vFilePacket);//向查询方返回信息
	void __queryFileOnline(SFileQueryPacket& vFilePacket);

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
void CQueryFlooding::receiveBuffer(std::promise<SFileResultPacket> &vSFileResultPacket)
{
	while (m_ServerCommandSock != SOCKET_ERROR)
	{
		char CommandBuffer[MAXCOMMANDPACKET];
		recv(m_ServerCommandSock, CommandBuffer, MAXCOMMANDPACKET,0);
		if (*reinterpret_cast<int*>(CommandBuffer) == QueryPacket)
		{
			__receiveQueryRequest(*reinterpret_cast<SFileQueryPacket*>(CommandBuffer));
		}
		else if(*reinterpret_cast<int*>(CommandBuffer) == ResultPacket)
		{
			vSFileResultPacket.set_value_at_thread_exit(*reinterpret_cast<SFileResultPacket*>(CommandBuffer));
			return;
		}
		else
		{
			std::cout << "Something error happen..." << std::endl;
		}
	}
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__receiveQueryRequest(SFileQueryPacket& vQueryPacket)
{
	auto File = queryFileLocal(vQueryPacket.FileName);
	if (File.IsExist)
	{
		SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
		if (__connectPeer(LocalSock, vQueryPacket.SenderIP, vQueryPacket.SenderCommandPort))
		{
			SFileResultPacket ResultPacket;
			ResultPacket.File = File;
			strcpy_s(ResultPacket.RecvIP ,m_pPeerConfig->getIP().c_str());
			ResultPacket.RecvCommandPort = m_pPeerConfig->getCommandPort();
			ResultPacket.RecvDataPort = m_pPeerConfig->getDataPort();
			__sendResult(LocalSock, ResultPacket);
		}
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
	SFileQueryPacket FilePacket;
	strcpy_s(FilePacket.FileName,vFileName.c_str());
	strcpy_s(FilePacket.SenderIP, m_pPeerConfig->getIP().c_str());
	FilePacket.SenderCommandPort = m_pPeerConfig->getCommandPort();
	FilePacket.SenderDataPort = m_pPeerConfig->getDataPort();
	__queryFileOnline(FilePacket);
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__queryFileOnline(SFileQueryPacket& vFilePacket)
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
void CQueryFlooding::__queryFlooding(SFileQueryPacket& vFilePacket)//fixme:尚未检测包来的路径（会往回发），有空写
{
	auto ConnectPeerSocket = m_pPeerConfig->getConnectPeerSocket();
	for (auto Peer : ConnectPeerSocket)
	{
		SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
		if (__connectPeer(LocalSock, std::any_cast<std::string>(Peer["IP"]), std::any_cast<int>(Peer["COMMANDPORT"])))
		{
			//vFilePacket.PassPeer[m_pPeerConfig->getSelfPeerID()] = std::make_pair(std::any_cast<std::string>(Peer["IP"]), std::any_cast<int>(Peer["COMMANDPORT"]));
			__sendQuery(LocalSock, vFilePacket);
		}
		else
		{
			std::cout << "Failed to connect " << std::any_cast<std::string>(Peer["IP"]) << " " << std::any_cast<int>(Peer["COMMANDPORT"]) << std::endl;
		}
		closesocket(LocalSock);
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
		SocketStatus = connect(vLocalSock, (sockaddr*)&NearPeerAddr, sizeof(NearPeerAddr));//成功返回0。否则返回SOCKET_ERROR
		if (SocketStatus == SOCKET_ERROR) { return false; }
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__sendQuery(SOCKET& vLoackSock, SFileQueryPacket& vFilePacket)
{
	send(vLoackSock, reinterpret_cast<char*>(&vFilePacket), sizeof(vFilePacket), 0);
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__sendResult(SOCKET &vLoackSock, SFileResultPacket& vFilePacket)
{
	send(vLoackSock, reinterpret_cast<char*>(&vFilePacket), sizeof(vFilePacket), 0);
}
