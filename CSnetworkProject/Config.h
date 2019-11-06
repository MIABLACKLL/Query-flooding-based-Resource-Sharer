#pragma once
#include<cstdio>
#include<fstream>
#include<iostream>
#include<any>
#include<string>
#include<vector>
#include<map>
#include<algorithm>
#include<regex>

constexpr int BufferSize = 1024;
constexpr int MinPort = 1024;
constexpr int MaxPort = 65535;

//fixme:尚有一些地方未做正确性检测，目前只保证在正确配置文件下正常读入，有空完善
//实现读取ini配置文件，并转换为类
class CConfig
{

public:
	CConfig() { __initConfigSet(); }
	~CConfig(void) = default;

	void setFilePath(const std::string vFilePath) { m_FilePath = vFilePath; }

	[[nodiscard]] bool loadConfigFile();

	[[nodiscard]] const std::string getIP();
	[[nodiscard]] const std::string getSelfPeerID();
	[[nodiscard]] const int getDataPort();
	[[nodiscard]] const int getCommandPort();
	[[nodiscard]] const std::vector<std::map<std::string, std::any>> getConnectPeerSocket();

private:
	std::map<std::string, std::map<std::string, std::string>> m_ConfigSet;
	std::vector<std::map<std::string,std::any>> m_PeerConfigSet;
	std::string m_FilePath = "PeerConfig.ini";
	std::ifstream m_FileIn;
	std::string m_RecentName;
	std::string m_ValidIpPattern = "((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]|[1-9][0-9]|[0-9]\\.)";

	void __initConfigSet();

	inline bool __isName(std::string vStrLine);
	inline std::string __splitName(std::string vStrLine);
	inline bool __appendName(std::string vName);
	inline bool __isKeyValuePair(std::string vStrLine);
	inline std::pair<std::string, std::string> __splitKeyValue(std::string vStrLine);
	inline bool __appendKeyValue(std::pair<std::string, std::string> vKeyValue);
	template<typename T> inline std::vector<T> __splitConnectPeer(std::string vStrConnectPeer);
	inline bool __checkPort(int vPort);
	inline bool __checkIP(std::string vIP);
	const std::vector<std::string> __getConnectPeerIP();
	const std::vector<int> __getConnectPeerCommandPort();
	const std::vector<int> __getConnectPeerDataPort();

};

//*********************************************************************
//FUNCTION:
bool CConfig::loadConfigFile()
{
	if (m_FileIn.is_open()) { m_FileIn.close(); }
	m_FileIn.open(m_FilePath);
	if (!m_FileIn) 
	{
		std::cout << "Fail to read config file.Program exit.";
		system("pause");
		exit(0);
	}
	char StrLineBuffer[BufferSize];
	std::string RecentName;
	while (m_FileIn.getline(StrLineBuffer, BufferSize))
	{
		std::string StrLine = StrLineBuffer;
		if (__isName(StrLine)) 
		{
			m_RecentName = __splitName(StrLine);
			__appendName(RecentName);
		}
		else if (__isKeyValuePair(StrLine)) { __appendKeyValue(__splitKeyValue(StrLine)); }
	}
	std::cout << "# Loading PeerConfig.ini successful" << std::endl;
	return true;
}

//*********************************************************************
//FUNCTION:
const std::string CConfig::getIP()
{
	std::string voIP = m_ConfigSet["peer"]["IP"];
	if (__checkIP(voIP)) { return voIP; }
	std::cout << "Fail to load config because of invalid IP.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const int CConfig::getDataPort()
{
	int voPort = stoi(m_ConfigSet["peer"]["DATAPORT"]);
	if (__checkPort(voPort)) { return voPort; }
	std::cout << "Fail to load config because of invalid port.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const int CConfig::getCommandPort()
{
	int voPort = stoi(m_ConfigSet["peer"]["COMMANDPORT"]);
	if (__checkPort(voPort)) { return voPort; }
	std::cout << "Fail to load config because of invalid port.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const std::string CConfig::getSelfPeerID()
{
	return m_ConfigSet["peer"]["PEERID"];
}

//*********************************************************************
//FUNCTION:
const std::vector<std::map<std::string, std::any>> CConfig::getConnectPeerSocket()
{
	std::vector<std::map<std::string, std::any>> voConnectPeerSocketSet;
	auto ConnectPeerIP = __getConnectPeerIP();
	auto ConnectPeerCommandPort = __getConnectPeerCommandPort();
	auto ConnectPeerDataPort = __getConnectPeerDataPort();
	for (size_t i = 0; i < ConnectPeerIP.size(); i++)
	{
		std::map<std::string, std::any> SocketSet;
		try
		{
			SocketSet.insert(std::make_pair("IP", ConnectPeerIP[i]));
			SocketSet.insert(std::make_pair("COMMANDPORT", ConnectPeerCommandPort[i]));
			SocketSet.insert(std::make_pair("DATAPORT", ConnectPeerDataPort[i]));
		}
		catch (...)
		{
			std::cout << "Peer socket data unequal.Program exit.";
			system("pause");
			exit(0);
		}
		voConnectPeerSocketSet.push_back(SocketSet);
	}
	return voConnectPeerSocketSet;
}

//*********************************************************************
//FUNCTION:
void CConfig::__initConfigSet()
{
	m_ConfigSet.insert(std::make_pair("peer", std::map<std::string, std::string>()));
	m_ConfigSet["peer"].insert(std::make_pair("IP",""));
	m_ConfigSet["peer"].insert(std::make_pair("DATAPORT", ""));
	m_ConfigSet["peer"].insert(std::make_pair("COMMANDPORT", ""));
	m_ConfigSet["peer"].insert(std::make_pair("PEERID", ""));
	m_ConfigSet["peer"].insert(std::make_pair("CONNECTPEER_IP", ""));
	m_ConfigSet["peer"].insert(std::make_pair("CONNECTPEER_COMMANDPORT", ""));
	m_ConfigSet["peer"].insert(std::make_pair("CONNECTPEER_DATAPORT", ""));
}

//*********************************************************************
//FUNCTION:
bool CConfig::__isName(std::string vStrLine)
{
	if (vStrLine.size() > 2 && vStrLine.front() == '['&&vStrLine.back() == ']') { return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
std::string CConfig::__splitName(std::string vStrLine)
{
	return vStrLine.substr(1, vStrLine.size() - 2);
}

//*********************************************************************
//FUNCTION:
bool CConfig::__appendName(std::string vName)
{
	transform(vName.begin(), vName.end(), vName.begin(), ::tolower);
	if (m_ConfigSet.find(vName) == m_ConfigSet.end())
	{
		m_ConfigSet.insert(std::make_pair(vName, std::map<std::string, std::string>()));
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__isKeyValuePair(std::string vStrLine)
{
	int EqualIndex = vStrLine.find('=');
	if (EqualIndex != 0 && EqualIndex != vStrLine.size() - 1 && EqualIndex != vStrLine.npos&&vStrLine.find_first_of('=') == vStrLine.find_last_of('=')) {  return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
std::pair<std::string, std::string> CConfig::__splitKeyValue(std::string vStrLine)
{
	int EqualIndex = vStrLine.find('=');
	std::pair<std::string, std::string> voKeyValue = std::make_pair(vStrLine.substr(0, EqualIndex),vStrLine.substr(EqualIndex+1,vStrLine.size()- EqualIndex-1));
	return voKeyValue;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__appendKeyValue(std::pair<std::string, std::string> vKeyValue)
{
	auto Config = m_ConfigSet.find(m_RecentName);
	if (Config != m_ConfigSet.end())
	{
		transform(vKeyValue.first.begin(), vKeyValue.first.end(), vKeyValue.first.begin(), ::toupper);
		Config->second[vKeyValue.first] = vKeyValue.second;
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
template<typename T>
std::vector<T> CConfig::__splitConnectPeer(std::string vStrConnectPeer)
{
	std::vector<T> voConnectPeerIDList;
	int BeginIndex = 0, SpacingIndex = 0;
	for (auto ch : vStrConnectPeer)
	{
		if (ch == ' ' || (SpacingIndex == vStrConnectPeer.size() - 1 && SpacingIndex++))
		{
			std::string StrPeer = vStrConnectPeer.substr(BeginIndex, SpacingIndex - BeginIndex);
			if (!StrPeer.empty())
			{
				if (std::is_same<T, int>::value)
				{
					T voPort = std::any_cast<T>(stoi(StrPeer));
					if (__checkPort(std::any_cast<int>(voPort))) { voConnectPeerIDList.push_back(voPort); }
					else
					{
						std::cout << "Fail to load config because of invalid port.Program exit.";
						system("pause");
						exit(0);
					}
				}
				else
					voConnectPeerIDList.push_back(std::any_cast<T>(StrPeer));
			}
			BeginIndex = SpacingIndex+1;
		}
		SpacingIndex++;
	}
	return voConnectPeerIDList;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__checkPort(int vPort)
{
	if (vPort >= MinPort && vPort <= MaxPort) { return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__checkIP(std::string vIP)
{
	return std::regex_match(vIP,std::regex(m_ValidIpPattern));
}

//*********************************************************************
//FUNCTION:
const std::vector<std::string> CConfig::__getConnectPeerIP()
{
	std::vector<std::string> voConnectPeerIPList = __splitConnectPeer<std::string>(m_ConfigSet["peer"]["CONNECTPEER_IP"]);
	return voConnectPeerIPList;
}

//*********************************************************************
//FUNCTION:
const std::vector<int> CConfig::__getConnectPeerCommandPort()
{
	std::vector<int> voConnectPeerCommandPortList = __splitConnectPeer<int>(m_ConfigSet["peer"]["CONNECTPEER_COMMANDPORT"]);
	return voConnectPeerCommandPortList;
}

//*********************************************************************
//FUNCTION:
const std::vector<int> CConfig::__getConnectPeerDataPort()
{
	std::vector<int> voConnectPeerPeerPortList = __splitConnectPeer<int>(m_ConfigSet["peer"]["CONNECTPEER_DATAPORT"]);
	return voConnectPeerPeerPortList;
}