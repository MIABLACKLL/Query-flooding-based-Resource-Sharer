#pragma once
#include<cstdio>
#include<fstream>
#include<iostream>
#include<string>
#include<set>
#include<map>
#include<algorithm>

constexpr int BufferSize = 1024;
constexpr int MinPort = 1024;
constexpr int MaxPort = 65535;

using namespace std;
class CConfig
{

public:
	CConfig() { __initConfigSet(); }
	~CConfig(void) = default;

	void setFilePath(const string vFilePath) { m_FilePath = vFilePath; }

	[[nodiscard]] bool openConfigFile();
	[[nodiscard]] bool readConfigFile();
	[[nodiscard]] const string const getIP();
	[[nodiscard]] const string const getSelfPeerID();
	[[nodiscard]] const int const getPort();
	[[nodiscard]] const set<string> const getConnectPeerID();

private:
	map<string, map<string, string>> m_ConfigSet;
	string m_FilePath = "PeerConfig.ini";
	ifstream m_FileIn;
	string m_RecentName;

	void __initConfigSet();

	inline bool __isName(string vStrLine);
	inline string __splitName(string vStrLine);
	inline bool __appendName(string vName);
	inline bool __isKeyValuePair(string vStrLine);
	inline pair<string, string> __splitKeyValue(string vStrLine);
	inline bool __appendKeyValue(pair<string, string> vKeyValue);
	inline set<string> __splitConnectPeerID(string vStrConnectPeerID);
	inline bool __checkPort(int vPort);
	inline bool __checkIP(string vIP);
};

//*********************************************************************
//FUNCTION:
bool CConfig::openConfigFile()
{
	if (m_FileIn.is_open()) { m_FileIn.close(); }
	m_FileIn.open(m_FilePath);
	if (m_FileIn) { return true; }
	cout << "Fail to read config file.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
bool CConfig::readConfigFile()
{
	if (!m_FileIn) { return false; }
	char StrLineBuffer[BufferSize];
	string RecentName;
	while (m_FileIn.getline(StrLineBuffer, BufferSize))
	{
		string StrLine = StrLineBuffer;
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
const string const CConfig::getIP()
{
	string IP = m_ConfigSet["peer"]["IP"];
	if (__checkIP(IP)) { return IP; }
	cout << "Fail to load config because of invalid IP.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const int const CConfig::getPort()
{
	int Port = stoi(m_ConfigSet["peer"]["PORT"]);
	if (__checkPort(Port)) { return Port; }
	cout << "Fail to load config because of invalid port.Program exit.";
	system("pause");
	exit(0);
}

//*********************************************************************
//FUNCTION:
const string const CConfig::getSelfPeerID()
{
	return m_ConfigSet["peer"]["PEERID"];
}

//*********************************************************************
//FUNCTION:
const set<string> const CConfig::getConnectPeerID()
{
	set<string> ConnectPeerIDSet = __splitConnectPeerID(m_ConfigSet["peer"]["CONNECTPEER"]);
	ConnectPeerIDSet.erase(m_ConfigSet["peer"]["PEERID"]);
	return ConnectPeerIDSet;
}

//*********************************************************************
//FUNCTION:
void CConfig::__initConfigSet()
{
	m_ConfigSet.insert(make_pair("peer", map<string, string>()));
	m_ConfigSet["peer"].insert(make_pair("IP",""));
	m_ConfigSet["peer"].insert(make_pair("PORT", ""));
	m_ConfigSet["peer"].insert(make_pair("PEERID", ""));
	m_ConfigSet["peer"].insert(make_pair("CONNECTPEER", ""));
}

//*********************************************************************
//FUNCTION:
bool CConfig::__isName(string vStrLine)
{
	if (vStrLine.size() > 2 && vStrLine.front() == '['&&vStrLine.back() == ']') { return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
string CConfig::__splitName(string vStrLine)
{
	return vStrLine.substr(1, vStrLine.size() - 2);
}

//*********************************************************************
//FUNCTION:
bool CConfig::__appendName(string vName)
{
	transform(vName.begin(), vName.end(), vName.begin(), ::tolower);
	if (m_ConfigSet.find(vName) == m_ConfigSet.end())
	{
		m_ConfigSet.insert(make_pair(vName, map<string, string>()));
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__isKeyValuePair(string vStrLine)
{
	int EqualIndex = vStrLine.find('=');
	if (EqualIndex != 0 && EqualIndex != vStrLine.size() - 1 && EqualIndex != vStrLine.npos&&vStrLine.find_first_not_of('=') == vStrLine.find_last_of('=')) { return true; }
	return false;
}

//*********************************************************************
//FUNCTION:
pair<string, string> CConfig::__splitKeyValue(string vStrLine)
{
	int EqualIndex = vStrLine.find('=');
	pair<string, string> KeyValue = make_pair(vStrLine.substr(0, EqualIndex),vStrLine.substr(EqualIndex+1,vStrLine.size()- EqualIndex-1));
	return KeyValue;
}

//*********************************************************************
//FUNCTION:
bool CConfig::__appendKeyValue(pair<string, string> vKeyValue)
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
set<string> CConfig::__splitConnectPeerID(string vStrConnectPeerID)
{
	set<string> ConnectPeerIDList;
	int BeginIndex = 0, SpacingIndex = 0;
	for (auto ch : vStrConnectPeerID)
	{
		if (ch == ' ')
		{
			string StrPeerID = vStrConnectPeerID.substr(BeginIndex, SpacingIndex - BeginIndex);
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
bool CConfig::__checkIP(string vIP)
{
	int IPNum[4];
	if (sscanf_s(vIP.c_str(), "%d.%d.%d.%d", &IPNum[0], &IPNum[1], &IPNum[2], &IPNum[3]) &&
		(IPNum[0] >= 0 && IPNum[0] <= 255) && (IPNum[1] >= 0 && IPNum[1] <= 255) && (IPNum[2] >= 0 && IPNum[2] <= 255) && (IPNum[3] >= 0 && IPNum[3] <= 255))
	{
		return true;
	}
	return false;
}