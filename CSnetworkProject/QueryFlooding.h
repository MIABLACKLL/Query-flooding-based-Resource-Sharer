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

	void __queryFloodingSend();//��ʵ��ʱ�������ڱ�PEER�����Ƿ����Ŀ���ļ�/�ļ��У��������ڣ���������PEER���к鷺��ѯ��
};

CQueryFlooding::CQueryFlooding()
{
	m_PeerConfig.openConfigFile();
	m_PeerConfig.readConfigFile();
}