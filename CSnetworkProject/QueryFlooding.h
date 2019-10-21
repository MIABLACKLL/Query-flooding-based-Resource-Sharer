#pragma once
#include<iostream>
#include<cstring>
#include<string>
#include<thread>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include"FileManagement.h"
#include"Config.h"

#pragma comment(lib,"Ws2_32.lib")
constexpr int MAXLISTEN = 20;
class CQueryFlooding
{
public:
	CQueryFlooding()=default;
	CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement); 
	~CQueryFlooding() { WSACleanup(); }

	bool listenCommandPort();


	[[nodiscard]] SFile queryFile(std::string vFileName);//在实现时，优先在本PEER查找是否存在目标文件/文件夹，若不存在，则向所连PEER进行洪泛查询。
	
private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	int m_TimeOut = 5;
	WSADATA m_WSAData;
	SOCKET m_ServerCommandSock;
	std::string m_ResponsePeerID;

	void __bandCommandSocket();
	SFile __queryFlooding(std::string vFileName);
	bool __connectNearlyPeer(SOCKET &vLocalSock,std::string vIP,int vPort);
	void __sendQuery(SOCKET &vLoackSock,std::string vFileName);

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
	int nRet = -1;
	if (m_ServerCommandSock != -1)
	{
		nRet = listen(m_ServerCommandSock, MAXLISTEN);//设定接受连接的套接字，以及设定连接队列长度;成功返回0，失败返回-1
	}
	if (nRet == SOCKET_ERROR) { return false; }
	return true;
}

//*********************************************************************
//FUNCTION:
SFile CQueryFlooding::queryFile(std::string vFileName)
{
	auto voFileQueryResult = m_pPeerFileSystem->findFile(vFileName);
	if (voFileQueryResult.second)
	{
		std::cout << "Find file in local. " << voFileQueryResult.first.FilePath << std::endl;
		return voFileQueryResult.first;
	}
	auto FileFloodingResult = __queryFlooding(vFileName);
	return FileFloodingResult;
}

//*********************************************************************
//FUNCTION:
SFile CQueryFlooding::__queryFlooding(std::string vFileName)
{
	auto ConnectPeerSocket = m_pPeerConfig->getConnectPeerSocket();
	for (auto Peer : ConnectPeerSocket)
	{
		SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
		if (__connectNearlyPeer(LocalSock, std::any_cast<std::string>(Peer["IP"]), std::any_cast<int>(Peer["COMMANDPORT"])))
		{
			__sendQuery(LocalSock,vFileName);
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

