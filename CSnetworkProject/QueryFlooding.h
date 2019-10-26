#pragma once
#include<iostream>
#include<cstring>
#include<string>
#include<future>
#include<ctime>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include"FileManagement.h"
#include"Config.h"

#pragma comment(lib,"Ws2_32.lib")
constexpr int MAXLISTEN = 10;//���PEER����
constexpr int MAXIPLENGTH = 16;
constexpr int MAXPORTLENGTH = 4;
constexpr int MAXPEERLENGTH = 64;
constexpr int MAXFILENAME = 256;
constexpr int MAXCOMMANDPACKET = 924;

constexpr int FILLRESULTPACKET = 372;//�ò�ѯ���ͽ�����ȴ��Ա���գ��е㶾����
constexpr int FILLQUERYPACKET = 4;
enum PacketType { QueryPacket, ResultPacket };

struct SFileQueryPacket
{
	int Type = QueryPacket;
	char SenderIP[MAXIPLENGTH];
	char PassPeer[MAXLISTEN][MAXPEERLENGTH];//��¼���ݹ����о�����PEER��ǰ16�ֽ�IP��17-20�ֽ�����˿ڣ�21�ֽ������¼PEERID���ƺ���Щ�˷ѣ�
	int SenderDataPort;
	int SenderCommandPort;
	char FileName[MAXFILENAME];
	time_t SendTime;
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
	time_t SendTime;
	char EmptyBuffer[FILLRESULTPACKET];
};

class CQueryFlooding
{
public:
	CQueryFlooding()=default;
	CQueryFlooding(CConfig* vConfig, CFileManagement* vFileManagement); 
	~CQueryFlooding() { WSACleanup(); }

	[[nodiscard]]bool listenCommandPort();
	void receiveBuffer(std::promise<SFileResultPacket> &vSFileResultPacket);//�ȶԽ��ܵ��İ��жϣ��������ͣ���Ȼ���ٵ��ò�ͬ����ʽ
																			//�յ���ѯ����ʱ�������²㴦���յ���ѯ���ʱ�����أ���������Ҫ�ٴε��á�
	void requestQuery(std::string vFileName);
	
	[[nodiscard]] SFile queryFileLocal(std::string vFileName);//�յ�Զ�˲�ѯ����ʱ�����ڱ��ؽ��в�ѯ���Ҳ�����������鷺��

private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	int m_TimeOut = 5;
	WSADATA m_WSAData;
	SOCKET m_ServerCommandSock;
	std::string m_ResponsePeerID;
	std::pair<std::string, time_t> m_LastRecvFile;//�����Ƿ���һ�������ڵĶ�PEER���أ�ֻ���ܵ�һ��

	void __receiveQueryRequest(SFileQueryPacket& vQueryPacket);//�鵽��ǰPEER�и��ļ�����������Ϣ��û�������ת��
	void __bandCommandSocket();
	void __queryFlooding(SFileQueryPacket& vFilePacket);
	bool __connectPeer(SOCKET &vLocalSock,std::string vIP,int vPort);
	void __sendQuery(SOCKET &vLoackSock, SFileQueryPacket& vFilePacket);
	void __sendResult(SOCKET &vLoackSock, SFileResultPacket& vFilePacket);//���ѯ��������Ϣ
	void __queryFileOnline(SFileQueryPacket& vFilePacket);
	bool __checkPass(SFileQueryPacket& vFilePacket, std::string vIP, int vPort);//����Ƿ�����·��
	bool __addPass(SFileQueryPacket& vFilePacket, std::string vIP, int vPort);//��ӵ�ת��������·����
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
		SocketStatus = listen(m_ServerCommandSock, MAXLISTEN);//�趨�������ӵ��׽��֣��Լ��趨���Ӷ��г���;
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
		auto PacketType = *reinterpret_cast<int*>(CommandBuffer);
		if (PacketType == QueryPacket)
		{
			__receiveQueryRequest(*reinterpret_cast<SFileQueryPacket*>(CommandBuffer));
		}
		else if(PacketType == ResultPacket)
		{
			auto ResultPacket = *reinterpret_cast<SFileResultPacket*>(CommandBuffer);
			if (strcmp(ResultPacket.File.FileName, m_LastRecvFile.first.c_str()) && ResultPacket.SendTime == m_LastRecvFile.second);
			else
			{
				m_LastRecvFile.first = ResultPacket.File.FileName;
				m_LastRecvFile.second = ResultPacket.SendTime;
				vSFileResultPacket.set_value_at_thread_exit(ResultPacket);
				return;
			}
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
			ResultPacket.SendTime = vQueryPacket.SendTime;
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
	FilePacket.SendTime = time(NULL);
	memset(FilePacket.PassPeer, 0, MAXLISTEN*MAXPEERLENGTH);
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
	//}//���ţ�����CUI�
	return voFileQueryResult.first;
}

//*********************************************************************
//FUNCTION:
void CQueryFlooding::__queryFlooding(SFileQueryPacket& vFilePacket)//fixme:��δ��������·���������ط������п�д
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
		SocketStatus = connect(vLocalSock, (sockaddr*)&NearPeerAddr, sizeof(NearPeerAddr));//�ɹ�����0�����򷵻�SOCKET_ERROR
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

//*********************************************************************
//FUNCTION:
bool CQueryFlooding::__checkPass(SFileQueryPacket& vFilePacket, std::string vIP, int vPort)//fixme:portת��д����
{
	int PassPeerNum = 0;
	while (strlen(vFilePacket.PassPeer[PassPeerNum])>0)
	{
		char IP[MAXIPLENGTH];
		char StrPort[MAXPORTLENGTH];
		strncpy_s(IP, vFilePacket.PassPeer[PassPeerNum], MAXIPLENGTH);
		strncpy_s(StrPort, &(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH+1]), MAXPORTLENGTH);
		int Port = *reinterpret_cast<int*>(StrPort);
		if (strcmp(IP, vIP.c_str()) == 0 && vPort == Port)
			return false;
		PassPeerNum++;
	}
	return true;
}
bool CQueryFlooding::__addPass(SFileQueryPacket& vFilePacket, std::string vIP, int vPort)//fixme:portת��д���ˣ��ȿ��������пո�
{
	int PassPeerNum = 0;
	while (strlen(vFilePacket.PassPeer[PassPeerNum]) > 0)
	{
		PassPeerNum++;
	}
	char StrPort[MAXPORTLENGTH];
	strcpy_s(vFilePacket.PassPeer[PassPeerNum], vIP.c_str());
	strcpy_s(&(vFilePacket.PassPeer[PassPeerNum][MAXIPLENGTH + 1]),MAXPORTLENGTH,itoa(vPort, StrPort));
	return false;


}
