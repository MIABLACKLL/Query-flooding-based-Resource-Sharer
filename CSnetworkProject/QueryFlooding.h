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

struct SFileQueryPacket
{
	char SenderIP[16];
	int SenderDataPort;
	int SenderCommandPort;
	SFile File;
};

class CQueryFlooding
{
public:
	CQueryFlooding()=default;
	CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement); 
	~CQueryFlooding() { WSACleanup(); }

	[[nodiscard]]bool listenCommandPort();
	[[nodiscard]]bool recevieQueryRequest();//fixme:û��þ���ʵ��


	[[nodiscard]] bool  requestQuery(std::string vFileName);
	[[nodiscard]] SFile queryFileOnline(SFileQueryPacket& vFilePacket);
	[[nodiscard]] SFile queryFileLocal(std::string vFileName);//�յ�Զ�˲�ѯ����ʱ�����ڱ��ؽ��в�ѯ���Ҳ�����������鷺��

private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	int m_TimeOut = 5;
	WSADATA m_WSAData;
	SOCKET m_ServerCommandSock;
	std::string m_ResponsePeerID;

	void __bandCommandSocket();
	SFile __queryFlooding(SFileQueryPacket& vFilePacket);
	bool __connectNearlyPeer(SOCKET &vLocalSock,std::string vIP,int vPort);
	void __sendQuery(SOCKET &vLoackSock, SFileQueryPacket& vFilePacket);

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
		nRet = listen(m_ServerCommandSock, MAXLISTEN);//�趨�������ӵ��׽��֣��Լ��趨���Ӷ��г���;
	}
	if (nRet == SOCKET_ERROR) { return false; }
	return true;
}

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::requestQuery(std::string vFileName)
{
	SFileQueryPacket FilePacket;
	strcpy_s(FilePacket.File.FileName,vFileName.c_str());
	strcpy_s(FilePacket.SenderIP, m_pPeerConfig->getIP().c_str());
	FilePacket.SenderCommandPort = m_pPeerConfig->getCommandPort();
	FilePacket.SenderDataPort = m_pPeerConfig->getDataPort();
}

//*********************************************************************
//FUNCTION:
SFile CQueryFlooding::queryFileOnline(SFileQueryPacket& vFilePacket)
{
	auto voFileQueryResult = m_pPeerFileSystem->findFile(vFilePacket.File.FileName);
	if (!voFileQueryResult.second)
	{
		__queryFlooding(vFilePacket);
	}
	return voFileQueryResult.first;
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
	//}//���ţ�����CUI�
	return voFileQueryResult.first;
}

//*********************************************************************
//FUNCTION:
SFile CQueryFlooding::__queryFlooding(SFileQueryPacket& vFilePacket)//fixme:��δ��������·���������ط������п�д
{
	auto ConnectPeerSocket = m_pPeerConfig->getConnectPeerSocket();
	for (auto Peer : ConnectPeerSocket)
	{
		SOCKET LocalSock = socket(AF_INET, SOCK_STREAM, 0);
		if (__connectNearlyPeer(LocalSock, std::any_cast<std::string>(Peer["IP"]), std::any_cast<int>(Peer["COMMANDPORT"])))
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
bool CQueryFlooding::__connectNearlyPeer(SOCKET &vLocalSock, std::string vIP, int vPort)
{
	int nRet = SOCKET_ERROR;
	if (vLocalSock != -1)
	{
		sockaddr_in NearPeerAddr;
		memset(&NearPeerAddr, 0, sizeof(NearPeerAddr));
		NearPeerAddr.sin_family = AF_INET;
		NearPeerAddr.sin_port = htons(vPort);
		InetPton(AF_INET, vIP.c_str(), &NearPeerAddr.sin_addr);
		nRet = connect(vLocalSock, (sockaddr*)&NearPeerAddr, sizeof(NearPeerAddr));//�ɹ�����0�����򷵻�SOCKET_ERROR
		if (nRet == SOCKET_ERROR) { return false; }
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
