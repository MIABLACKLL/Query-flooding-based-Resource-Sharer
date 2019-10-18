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

	void listenCommandPort();
	void listenDataPort();

	[[nodiscard]] std::string queryLocalFile(std::string vFileName);
	
private:
	CConfig m_PeerConfig;
	CFileManagement m_PeerFileSystem;
	std::string m_ResponsePeerID;

	void __queryFloodingSend();//在实现时，优先在本PEER查找是否存在目标文件/文件夹，若不存在，则向所连PEER进行洪泛查询。
};

CQueryFlooding::CQueryFlooding()
{
	m_PeerConfig.openConfigFile();
	m_PeerConfig.readConfigFile();
}