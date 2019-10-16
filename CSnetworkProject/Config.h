#pragma once
#include<cstdio>
#include<fstream>
#include<iostream>
#include<string>
#include<set>
#include<map>
#include<algorithm>
#include<regex>

constexpr int BufferSize = 1024;
constexpr int MinPort = 1024;
constexpr int MaxPort = 65535;


class CConfig
{

public:
	CConfig() { __initConfigSet(); }
	~CConfig(void) = default;

	void setFilePath(const std::string vFilePath) { m_FilePath = vFilePath; }

	[[nodiscard]] bool openConfigFile();
	[[nodiscard]] bool readConfigFile();
	[[nodiscard]] const std::string const getIP();
	[[nodiscard]] const std::string const getSelfPeerID();
	[[nodiscard]] const int const getPort();
	[[nodiscard]] const std::set<std::string> const getConnectPeerID();

private:
	std::map<std::string, std::map<std::string, std::string>> m_ConfigSet;
	std::string m_FilePath = "PeerConfig.ini";
	std::ifstream m_FileIn;
	std::string m_RecentName;
	std::string m_ValidIpPattern = "((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])";

	void __initConfigSet();

	inline bool __isName(std::string vStrLine);
	inline std::string __splitName(std::string vStrLine);
	inline bool __appendName(std::string vName);
	inline bool __isKeyValuePair(std::string vStrLine);
	inline std::pair<std::string, std::string> __splitKeyValue(std::string vStrLine);
	inline bool __appendKeyValue(std::pair<std::string, std::string> vKeyValue);
	inline std::set<std::string> __splitConnectPeerID(std::string vStrConnectPeerID);
	inline bool __checkPort(int vPort);
	inline bool __checkIP(std::string vIP);
};

//*********************************************************************
//FUNCTION:
bool CConfig::openConfigFile()
{
	if (m_FileIn.is_open()) { m_FileIn.close(); }
	m_FileIn.open(m_FilePath);
	if (m_FileIn) { return true; }
	std::cout << "Fail to read config file.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
bool CConfig::readConfigFile()
{
	if (!m_FileIn) { return false; }
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
	return true;
}

//*********************************************************************
//FUNCTION:
const std::string const CConfig::getIP()
{
	std::string IP = m_ConfigSet["peer"]["IP"];
	if (__checkIP(IP)) { return IP; }
	std::cout << "Fail to load config because of invalid IP.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const int const CConfig::getPort()
{
	int Port = stoi(m_ConfigSet["peer"]["PORT"]);
	if (__checkPort(Port)) { return Port; }
	std::cout << "Fail to load config because of invalid port.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const std::string const CConfig::getSelfPeerID()
{
	return m_ConfigSet["peer"]["PEERID"];
}

//*********************************************************************
//FUNCTION:
const std::set<std::string> const CConfig::getConnectPeerID()
{
	std::set<std::string> ConnectPeerIDSet = __splitConnectPeerID(m_ConfigSet["peer"]["CONNECTPEER"]);
	ConnectPeerIDSet.erase(m_ConfigSet["peer"]["PEERID"]);
	return ConnectPeerIDSet;
}

//*********************************************************************
//FUNCTION:
void CConfig::__initConfigSet()
{
	m_ConfigSet.insert(std::make_pair("peer", std::map<std::string, std::string>()));
	m_ConfigSet["peer"].insert(std::make_pair("IP",""));
	m_ConfigSet["peer"].insert(std::make_pair("PORT", ""));
	m_ConfigSet["peer"].insert(std::make_pair("PEERID", ""));
	m_ConfigSet["peer"].insert(std::make_pair("CONNECTPEER", ""));
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
	if (EqualIndex != 0 && EqualIndex != vStrLine.size() - 1 && EqualIndex != vStrLine.npos&&vStrLine.find_first_not_of('=') == vStrLine.find_last_of('=')) { return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
std::pair<std::string, std::string> CConfig::__splitKeyValue(std::string vStrLine)
{
	int EqualIndex = vStrLine.find('=');
	std::pair<std::string, std::string> KeyValue = std::make_pair(vStrLine.substr(0, EqualIndex),vStrLine.substr(EqualIndex+1,vStrLine.size()- EqualIndex-1));
	return KeyValue;
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
std::set<std::string> CConfig::__splitConnectPeerID(std::string vStrConnectPeerID)
{
	std::set<std::string> ConnectPeerIDList;
	int BeginIndex = 0, SpacingIndex = 0;
	for (auto ch : vStrConnectPeerID)
	{
		if (ch == ' ')
		{
			std::string StrPeerID = vStrConnectPeerID.substr(BeginIndex, SpacingIndex - BeginIndex);
			if (!StrPeerID.empty()) { ConnectPeerIDList.insert(StrPeerID); }
			BeginIndex = SpacingIndex+1;
		}
		else
			SpacingIndex++;
	}
	return ConnectPeerIDList;
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