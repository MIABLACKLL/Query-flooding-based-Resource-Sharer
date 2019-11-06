#pragma once
#include<fstream>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include<filesystem>
#include"Config.h"
#include"FileManagement.h"

#pragma comment(lib,"Ws2_32.lib")


constexpr int MAXTRANSFERFILELEGTH = 256;
constexpr int MAXFILESEGEMENT = 1024;
constexpr int MAXLISTENPEER = 10;//最大PEER数量


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
	char FileName[MAXTRANSFERFILELEGTH];
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

	void __bindCommandSocket();
	void __receivFilePacket(SOCKET& vClientSock);
	void __sendTargetFile(SOCKET& vServerSock, SRequestDownloadPacket& vRequestDownloadPacket);
	void __sendFilePacket(SOCKET& vServerSock, SFilePacket& vFilePacket, std::filesystem::directory_entry vDirEntry);

};

CTransfer::CTransfer(CConfig* vConfig, CFileManagement* vFileManagement) :m_pPeerConfig(vConfig), m_pPeerFileSystem(vFileManagement)
{
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);
	m_ServerDataSock = socket(AF_INET, SOCK_STREAM, 0);
	__bindCommandSocket();
}

//*********************************************************************
//FUNCTION:
void CTransfer::__bindCommandSocket()
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
		SocketStatus = listen(m_ServerDataSock, MAXLISTENPEER);//设定接受连接的套接字，以及设定连接队列长度;
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
		SocketStatus = connect(ClientSock, reinterpret_cast<sockaddr*>(&DestinationAddr), sizeof(DestinationAddr));
		if (SocketStatus == SOCKET_ERROR) { return false; }
	}
	char SendBuffer[sizeof(SRequestDownloadPacket)];
	memcpy(SendBuffer, &vRequstDownloadPacket, sizeof(SRequestDownloadPacket));
	send(ClientSock, SendBuffer, sizeof(vRequstDownloadPacket), 0);
	__receivFilePacket(ClientSock);
	closesocket(ClientSock);
	return true;
}

//*********************************************************************
//FUNCTION:
void CTransfer::receiveDownloadRequest()
{
	while (m_ServerDataSock != SOCKET_ERROR)
	{
		sockaddr_in ClientAddr;
		int ClientAddrSize = sizeof(ClientAddr);
		SOCKET ConnectClientSock = accept(m_ServerDataSock, reinterpret_cast<sockaddr*>(&ClientAddr), &ClientAddrSize);
		if (ConnectClientSock != SOCKET_ERROR)
		{
			char RequestBuffer[sizeof(SRequestDownloadPacket)];
			recv(ConnectClientSock, RequestBuffer, sizeof(SRequestDownloadPacket), 0);
			SRequestDownloadPacket RequestDownloadPacket = *reinterpret_cast<SRequestDownloadPacket*>(RequestBuffer);
			__sendTargetFile(ConnectClientSock, RequestDownloadPacket);
		}
		closesocket(ConnectClientSock);
	}
}

//*********************************************************************
//FUNCTION:
void CTransfer::__receivFilePacket(SOCKET& vClientSock)//fixme:这个函数写的太多了，有空封装。
{
	std::string recvRootPacketPath;
	std::string shareRootPath = m_pPeerFileSystem->getSharePath();
	std::ofstream File;
	while (true)
	{
		char FileBuffer[sizeof(SFilePacket)];
		recv(vClientSock, FileBuffer, sizeof(SFilePacket), 0);
		SFilePacket FilePacket = *reinterpret_cast<SFilePacket*>(FileBuffer);
		if (FilePacket.FileEnd) { break; }
		if (FilePacket.CurrentFileNum == 1 && FilePacket.File.IsDir) 
		{ 
			recvRootPacketPath = FilePacket.File.FilePath; 
			recvRootPacketPath = recvRootPacketPath.substr(0, recvRootPacketPath.rfind('\\'));
			m_pPeerFileSystem->createDir(recvRootPacketPath);
			continue;
		}
		if (!FilePacket.File.IsDir&&FilePacket.FileOffset == 0) 
		{
			std::string FilePath;
			if(recvRootPacketPath.empty())
				FilePath = shareRootPath + "\\" + FilePacket.File.FileName;
			else
			{
				std::string recvFilePath = FilePacket.File.FilePath;
				FilePath = shareRootPath + recvFilePath.substr(recvRootPacketPath.size());
			}
			if (File.is_open()) { File.close(); }
			File.open(FilePath, std::ios_base::app | std::ios_base::out | std::ios_base::binary);
		}
		if (!FilePacket.File.IsDir)
		{
			File.write(FilePacket.FileSegement, (FilePacket.File.FileSize > MAXFILESEGEMENT) && (FilePacket.File.FileSize - FilePacket.FileOffset > MAXFILESEGEMENT)
				? MAXFILESEGEMENT : FilePacket.File.FileSize - FilePacket.FileOffset);//Perfect!
			File.seekp(std::ios_base::end);
		}
		if (!FilePacket.File.IsDir&&FilePacket.FileEnd) { File.close(); }
		if (FilePacket.File.IsDir)
		{
			std::string recvFilePath = FilePacket.File.FilePath;
			std::string PacketPath = shareRootPath + recvFilePath.substr(recvRootPacketPath.size());
			m_pPeerFileSystem->createDir(PacketPath);
		}
	}
}

//*********************************************************************
//FUNCTION:
void  CTransfer::__sendTargetFile(SOCKET& vServerSock, SRequestDownloadPacket& vRequestDownloadPacket)//fixme:这个函数写的太多了，有空封装。
{
	auto TargetFile = m_pPeerFileSystem->findFile(vRequestDownloadPacket.FileName);
	if (TargetFile.second)
	{
		SFilePacket FilePacket;
		char SendBuffer[sizeof(SFilePacket)];
		FilePacket.FileNum = 1;
		FilePacket.CurrentFileNum = 1;
		FilePacket.FileOffset = 0;
		FilePacket.FileEnd = false;
		FilePacket.File = TargetFile.first;
		if (TargetFile.first.IsDir)
		{
			for (auto p : std::filesystem::recursive_directory_iterator(TargetFile.first.FilePath))
				FilePacket.FileNum++;
			memcpy(SendBuffer, &FilePacket, sizeof(SFilePacket));
			send(vServerSock, SendBuffer, sizeof(SFilePacket), 0);
			for (auto p : std::filesystem::recursive_directory_iterator(TargetFile.first.FilePath))
			{
				__sendFilePacket(vServerSock, FilePacket, p);
			}
		}
		else
			__sendFilePacket(vServerSock, FilePacket, std::filesystem::directory_entry(TargetFile.first.FilePath));
		FilePacket.FileEnd = true;
		memcpy(SendBuffer, &FilePacket, sizeof(SFilePacket));
		send(vServerSock, SendBuffer, sizeof(SFilePacket), 0);
	}
}
//*********************************************************************
//FUNCTION:
void  CTransfer::__sendFilePacket(SOCKET& vServerSock, SFilePacket& vFilePacket,std::filesystem::directory_entry vDirEntry)
{
	vFilePacket.FileOffset = 0;
	std::ifstream File;
	char SendBuffer[sizeof(SFilePacket)];
	while (true)
	{
		if (vDirEntry.is_directory())
		{
			vFilePacket.File.IsDir = true;
			strcpy_s(vFilePacket.File.FilePath, MAXTRANSFERFILELEGTH, vDirEntry.path().string().c_str());
			vFilePacket.CurrentFileNum++;
			memcpy(SendBuffer, &vFilePacket, sizeof(SFilePacket));
			send(vServerSock, SendBuffer, sizeof(SFilePacket), 0);
			break;
		}
		else
		{
			int ReadSize = (vDirEntry.file_size() > MAXFILESEGEMENT) && (vDirEntry.file_size() - vFilePacket.FileOffset > MAXFILESEGEMENT) ? MAXFILESEGEMENT : vDirEntry.file_size() - vFilePacket.FileOffset;
			if (vFilePacket.FileOffset == 0)
			{
				File.open(vDirEntry.path(), std::ios_base::binary | std::ios_base::in);
				vFilePacket.File.IsDir = false;
				vFilePacket.File.FileSize = vDirEntry.file_size();
				strcpy_s(vFilePacket.File.FileName, MAXTRANSFERFILELEGTH, vDirEntry.path().filename().string().c_str());
				strcpy_s(vFilePacket.File.FilePath, MAXTRANSFERFILELEGTH, vDirEntry.path().string().c_str());
			}
			File.read(vFilePacket.FileSegement, ReadSize);
			memcpy(SendBuffer, &vFilePacket, sizeof(SFilePacket));
			send(vServerSock, SendBuffer, sizeof(SFilePacket), 0);
			if (vFilePacket.FileOffset + ReadSize < vDirEntry.file_size())
			{
				vFilePacket.FileOffset += ReadSize;
				continue;
			}
			break;
		}
	}
}