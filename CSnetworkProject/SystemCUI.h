#pragma once
#include<iostream>
#include<thread>
#include<future>
#include<chrono>
#include"FileManagement.h"
#include"Config.h"
#include"QueryFlooding.h"
#include"Transfer.h"

constexpr int MAXINPUTBUFFER = 256;

class CSystemCUI 
{
public:
	CSystemCUI();
	~CSystemCUI() = default;
	void runSystem();
	
private:
	void __displayMenu();
	void __processInput();//ls,cd,find,queryonline,exit
	void __queryOnline(std::string vFile);
	CConfig* m_pPeerConfig;
	CFileManagement* m_pFileManagement;
	CQueryFlooding* m_pQueryFlooding;
	CTransfer* m_pTransfer;
	std::future<SResultPacket> m_ReusltFuture;
	std::promise<SResultPacket> m_PromiseResult;
	std::thread* recvQueryFlooding;
	std::thread* recvTransfer;
};

CSystemCUI::CSystemCUI()
{
	m_pPeerConfig = new CConfig();
	m_pPeerConfig->loadConfigFile();

	m_pFileManagement = new CFileManagement();
	m_pQueryFlooding = new CQueryFlooding(m_pPeerConfig, m_pFileManagement);
	m_pTransfer = new CTransfer(m_pPeerConfig, m_pFileManagement);
}

void CSystemCUI::runSystem()
{
	m_pQueryFlooding->listenCommandPort();
	m_pTransfer->listenDataPort();

	m_PromiseResult = std::promise<SResultPacket>();
	m_ReusltFuture = m_PromiseResult.get_future();
	recvQueryFlooding = new std::thread(&CQueryFlooding::receiveBuffer, m_pQueryFlooding, std::ref(m_PromiseResult));
	recvQueryFlooding->detach();
	recvTransfer = new std::thread(&CTransfer::receiveDownloadRequest, m_pTransfer);

	__displayMenu();
	__processInput();

	exit(0);
}

void CSystemCUI::__displayMenu()
{
	std::cout << "# Flooding query is running." << std::endl;
	std::cout << "# Welcome," << m_pPeerConfig->getSelfPeerID()<<"."<<std::endl;
	std::cout << "# Your IP: " << m_pPeerConfig->getIP() << "" << std::endl;
	std::cout << "# Your CommandPort: " << m_pPeerConfig->getCommandPort() << "" << std::endl;
	std::cout << "# Your DataPort: " << m_pPeerConfig->getDataPort() << "" << std::endl;
	std::cout << "# Your SharePath: " << m_pFileManagement->getSharePath() << "" << std::endl;
	std::cout << std::endl;
}

void CSystemCUI::__processInput()//if else实现选择- -不忍直视
{
	while (true)
	{
		std::string InputCommand;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
		std::cout << m_pFileManagement->getCurrentPath() << std::endl;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		std::cout << "λ ";
		char InputBuffer[MAXINPUTBUFFER];
		std::cin.getline(InputBuffer, MAXINPUTBUFFER);
		InputCommand = InputBuffer;

		if (InputCommand == "ls")
		{
			m_pFileManagement->listCurrenPathFileAndDir();
		}
		else if (InputCommand.substr(0, 2) == "cd")
		{
			if (!m_pFileManagement->changeCurrentPath(InputCommand.substr(3)))
				std::cout << "Failed: System could not find the specified path." << std::endl;
		}
		else if (InputCommand.substr(0, 4) == "find")
		{
			if (InputCommand.size() <= 5)
			{
				std::cout << "You must enter the file name.find [filename]" << std::endl;
			}
			else
			{
				auto ResultFile = m_pFileManagement->findFile(InputCommand.substr(5));
				if (ResultFile.second)
				{
					std::cout << "Find " << ResultFile.first.FileName << " in local path: " << ResultFile.first.FilePath << std::endl;
				}
				else
				{
					std::cout << "Failed: " << ResultFile.first.FileName << " not in  local system." << std::endl;
				}
			}
		}
		else if (InputCommand.substr(0, 11) == "queryonline")
		{
			if (InputCommand.size() <= 12)
			{
				std::cout << "You must enter the file name.find [filename]" << std::endl;
			}
			else
			{
				__queryOnline(InputCommand.substr(12));
			}
		}
		else if (InputCommand == "exit")
		{
			std::cout << "Bye." << std::endl;
			system("pause");
			break;
		}
		else if (InputCommand == "help")
		{
			std::cout << std::setw(25) << "ALL COMMAND" << std::endl;
			std::cout << std::setw(25) << "cd [path]\t" << std::setw(70) << "Modify the current working directory to the specified [path]" << std::endl;
			std::cout << std::setw(25) << "exit\t" << std::setw(70) << "Exit the program" << std::endl;
			std::cout << std::setw(25) << "find [filename]\t" << std::setw(70) << "Query whether the file named [filename] is in local" << std::endl;
			std::cout << std::setw(25) << "ls\t" << std::setw(70) << "Displays all folders and files in the current directory" << std::endl;
			std::cout << std::setw(25) << "queryonline [filename]\t" << std::setw(70) << "Query [filename] on line" << std::endl;
		}
		else
		{
			std::cout << "\"" << InputCommand << "\"" << " is not an internal or external command," << std::endl << " nor is it a runnable program or batch files." << std::endl;
		}
		std::cout << std::endl;
	}
}

void CSystemCUI::__queryOnline(std::string vFile)
{
	m_pQueryFlooding->requestQuery(vFile);
	auto FutrueStatus = m_ReusltFuture.wait_for(std::chrono::seconds(2));
	SResultPacket QueryResult;
	QueryResult.IsExistOnline = false;
	if (FutrueStatus == std::future_status::ready)
	{
		QueryResult = m_ReusltFuture.get();
		m_PromiseResult = std::promise<SResultPacket>();
		m_ReusltFuture = m_PromiseResult.get_future();
		recvQueryFlooding = new std::thread(&CQueryFlooding::receiveBuffer, m_pQueryFlooding, std::ref(m_PromiseResult));
		recvQueryFlooding->detach();
	}
	else
	{
		std::cout << "Not find " << vFile << " online." << std::endl;
	}
	if (QueryResult.IsExistOnline)
	{
		std::cout << "Find file \"" << QueryResult.File.FileName << "\" online." << std::endl;
		std::cout << "From Source Peer: " << std::endl << "IP: " << QueryResult.RecvIP << std::endl << "Command Port: " << QueryResult.RecvCommandPort
			<< std::endl << "Data Port: " << QueryResult.RecvDataPort << std::endl;
		std::cout << "Download this file(" << QueryResult.File.FileSize << "Bytes)?(Y/N)";
		std::string Choose;
		std::cin >> Choose;
		if (Choose == "Y" || Choose == "y")
		{
			SRequestDownloadPacket RequestDownloadPacket;
			strcpy_s(RequestDownloadPacket.FileName, QueryResult.File.FileName);
			RequestDownloadPacket.IsDir = QueryResult.File.IsDir;
			std::cout << "Download Begin..."<<std::endl;
			m_pTransfer->sendDownloadRequest(RequestDownloadPacket, QueryResult.RecvIP, QueryResult.RecvDataPort);
			std::cout << "Download completed! Download in " << m_pFileManagement->getSharePath() << std::endl;
		}
		else
		{
			std::cout << "Cancel download or error command." << std::endl;
		}
		std::cin.clear();
		std::cin.ignore();
	}
}