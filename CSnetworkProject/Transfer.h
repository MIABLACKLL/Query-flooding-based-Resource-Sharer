#pragma once
#include<fstream>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include<filesystem>
#include"Config.h"
#include"FileManagement.h"


#pragma comment(lib,"Ws2_32.lib")

constexpr int MAXFILELEGTH = 256;
constexpr int MAXFILESEGEMENT = 1024;
constexpr int MAXLISTEN = 10;//最大PEER数量

struct SFilePacket
{
	int FileNum;
	int CurrentFileNum;
	bool FileEnd;
	int FileOffset;
	SFile File;
	char FileSegement[MAXFILESEGEMENT];
};

struct SRequestDownloadPacket
{
	char FileName[MAXFILELEGTH];
	bool IsDir;
};

class CTransfer
{
public:
	CTransfer() = default;
	CTransfer(CConfig* vConfig, CFileManagement* vFileManagement);

	[[nodiscard]]bool listenDataPort();
	[[nodiscard]]bool sendDownloadRequest(SRequestDownloadPacket& vRequstDownloadPacket,std::string vDestinationIP,int vDestinationPort);

	void receiveDownloadRequest();

private:
	CFileManagement* m_pPeerFileSystem;
	CConfig* m_pPeerConfig;
	WSADATA m_WSAData;
	SOCKET m_ServerDataSock;
	std::string m_ResponsePeerID;

	void __bandCommandSocket();
	void __receivFilePacket(SOCKET& vClientSock);
	void __sendFilePacket(SOCKET& vServerSock, SRequestDownloadPacket& vRequestDownloadPacket);
	void __makeFilePacket();
	void __splitFile();
	void __saveFile();
	void __readFile();

};

CTransfer::CTransfer(CConfig* vConfig, CFileManagement* vFileManagement) :m_pPeerConfig(vConfig), m_pPeerFileSystem(vFileManagement)
{
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);
	m_ServerDataSock = socket(AF_INET, SOCK_STREAM, 0);
	__bandCommandSocket();
}

//*********************************************************************
//FUNCTION:
void CTransfer::__bandCommandSocket()
{
	sockaddr_in SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET, m_pPeerConfig->getIP().c_str(), &SockAddr.sin_addr);
	SockAddr.sin_port = htons(m_pPeerConfig->getDataPort());
	bind(m_ServerDataSock, reinterpret_cast<sockaddr*>(&SockAddr), sizeof(SockAddr));
}

//*********************************************************************
//FUNCTION:
bool CTransfer::listenDataPort()
{
	int SocketStatus = SOCKET_ERROR;
	if (m_ServerDataSock != SOCKET_ERROR)
	{
		SocketStatus = listen(m_ServerDataSock, MAXLISTEN);//设定接受连接的套接字，以及设定连接队列长度;
	}
	if (SocketStatus == SOCKET_ERROR) { return false; }
	return true;
}

//*********************************************************************
//FUNCTION:
bool CTransfer::sendDownloadRequest(SRequestDownloadPacket& vRequstDownloadPacket, std::string vDestinationIP, int vDestinationPort)
{
	int SocketStatus = SOCKET_ERROR;
	SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0);
	if (ClientSock != SOCKET_ERROR)
	{
		sockaddr_in DestinationAddr;
		memset(&DestinationAddr, 0, sizeof(DestinationAddr));
		DestinationAddr.sin_family = AF_INET;
		DestinationAddr.sin_port = htons(vDestinationPort);
		InetPton(AF_INET, vDestinationIP.c_str(), &DestinationAddr.sin_addr);
		SocketStatus = connect(ClientSock, (sockaddr*)&DestinationAddr, sizeof(DestinationAddr));
		if (SocketStatus == SOCKET_ERROR) { return false; }
	}
	send(ClientSock, reinterpret_cast<char*>(&vRequstDownloadPacket), sizeof(vRequstDownloadPacket), 0);
	__receivFilePacket(ClientSock);
	return true;
}

//*********************************************************************
//FUNCTION:
void CTransfer::receiveDownloadRequest()
{
	sockaddr_in ClientAddr;
	int ClientAddrSize = sizeof(ClientAddr);
	SOCKET ConnectClientSock = accept(m_ServerDataSock, reinterpret_cast<sockaddr*>(&ClientAddr), &ClientAddrSize);
	if (ConnectClientSock != SOCKET_ERROR)
	{
		char RequestBuffer[sizeof(SRequestDownloadPacket)];
		recv(ConnectClientSock, RequestBuffer, sizeof(SRequestDownloadPacket), 0);
		SRequestDownloadPacket RequestDownloadPacket = *reinterpret_cast<SRequestDownloadPacket*>(RequestBuffer);
		__sendFilePacket(ConnectClientSock, RequestDownloadPacket);
	}
	closesocket(ConnectClientSock);
}

//*********************************************************************
//FUNCTION:
void CTransfer::__receivFilePacket(SOCKET& vClientSock)
{
	std::string recvRootPath;
	std::ofstream File;
	while (true)
	{
		char FileBuffer[sizeof(SFilePacket)];
		recv(vClientSock, FileBuffer, sizeof(SFilePacket), 0);
		SFilePacket FilePacket = *reinterpret_cast<SFilePacket*>(FileBuffer);
		if (FilePacket.CurrentFileNum == 1 && FilePacket.File.IsDir) { recvRootPath = FilePacket.File.FileName; }

		if (!FilePacket.File.IsDir&&FilePacket.FileOffset == 0) { File.open(FilePacket.File.FileName, std::ios_base::app | std::ios_base::out | std::ios_base::binary); }
		if (!FilePacket.File.IsDir)
		{
			File.write(FilePacket.FileSegement, (FilePacket.File.FileSize > MAXFILESEGEMENT) && (FilePacket.File.FileSize - FilePacket.FileOffset > MAXFILESEGEMENT)
				? MAXFILESEGEMENT : FilePacket.File.FileSize - FilePacket.FileOffset);//Perfect!
			File.seekp(std::ios_base::end);
		}
		if (!FilePacket.File.IsDir&&FilePacket.FileEnd) { File.close(); }

		if (FilePacket.File.IsDir)
		{

		}
		if (FilePacket.FileNum == FilePacket.CurrentFileNum&&FilePacket.FileEnd) { break; }
	}
}