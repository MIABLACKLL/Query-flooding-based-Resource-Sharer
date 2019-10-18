#pragma once
#include<iostream>
#include<cstring>
#include<string>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include"FileManagement.h"
#include"Config.h"
#pragma comment(lib,"Ws2_32.lib")

class CQueryFlooding
{
public:
	CQueryFlooding();
	~CQueryFlooding() = default;
	
	void queryOnlineFile(std::string vFileName);//fixme:暂时没想好返回什么...

	void listenCommandPort();
	void listenDataPort();

	[[nodiscard]] std::string queryLocalFile(std::string vFileName);
	
private:
	CConfig m_PeerConfig;
	CFileManagement m_PeerFileSystem;

	void __queryFloodingSend();
};

CQueryFlooding::CQueryFlooding()
{
	m_PeerConfig.openConfigFile();
	m_PeerConfig.readConfigFile();
}