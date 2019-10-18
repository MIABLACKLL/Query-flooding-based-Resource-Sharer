#pragma once
#include<iostream>
#include<iomanip>
#include<string>
#include<filesystem>
#include<regex>

class CFileManagement
{
public:
	CFileManagement() { __initPath(); }
	~CFileManagement() = default;

	[[nodiscard]] inline std::string getCurrentPath() { return m_CurrentPath.string(); }

	[[nodiscard]] bool changeCurrentPath(std::string vDir);
	[[nodiscard]] bool createDir(std::string vDir);
	[[nodiscard]] bool setShareDir(std::string vDir);
	[[nodiscard]] std::string findFile(std::string vFileName);
	[[nodiscard]] bool findDir(std::string vFileName);//fixme:正在想怎么实现...

	void listCurrenPathFileAndDir();

private:
	std::filesystem::path m_RootPath;
	std::filesystem::path m_CurrentPath;
	std::filesystem::path m_ShareFilePath;
	const std::string m_InvalidCharPattern = "[^\\?\"<>\\*\\|:\\.]+";

	[[nodiscard]] bool __isOutRoot(std::string vDir);
	[[nodiscard]] inline bool __isPathValid(std::string vDir) { return std::regex_match(vDir, std::regex(m_InvalidCharPattern)); }
	void __initPath();
};

//*********************************************************************
//FUNCTION:
bool CFileManagement::changeCurrentPath(std::string vDir)
{
	if (!__isPathValid(vDir)||__isOutRoot(vDir))
		return false;
	auto NewPath = std::filesystem::path(vDir);
	try { NewPath = std::filesystem::absolute(NewPath); }
	catch (...) { return false; }
	if (std::filesystem::exists(NewPath) && std::filesystem::is_directory(NewPath))
	{
		m_CurrentPath = NewPath;
		std::filesystem::current_path(m_CurrentPath);
		return true;
	}
	return false;
}

//*********************************************************************
//FUNCTION:
bool CFileManagement::createDir(std::string vDir)
{
	if (!__isPathValid(vDir)||__isOutRoot(vDir))
		return false;
	auto CreatePath = std::filesystem::path(vDir);
	return std::filesystem::create_directory(CreatePath);
}

//*********************************************************************
//FUNCTION:
bool CFileManagement::setShareDir(std::string vDir)
{
	if (!__isPathValid(vDir) || __isOutRoot(vDir))
		return false;
	m_ShareFilePath = m_ShareFilePath = std::filesystem::absolute(std::filesystem::path(vDir));
}

//*********************************************************************
//FUNCTION:
std::string CFileManagement::findFile(std::string vFileName)//fixme:暂时无法解决不同目录同名文件的问题
{
	for (auto p : std::filesystem::recursive_directory_iterator(m_ShareFilePath))
	{
		if (p.path().filename().string() == vFileName)
			return std::filesystem::absolute(p.path()).string();
	}
	return std::string("");
}

//*********************************************************************
//FUNCTION:
void CFileManagement::listCurrenPathFileAndDir()
{
	for (auto p : std::filesystem::directory_iterator(m_CurrentPath))
	{
		if (p.is_directory()) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); }
		else { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); }
		std::cout << p.path().filename() << "\t";
	}
	std::cout << std::endl;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

//*********************************************************************
//FUNCTION:
void CFileManagement::__initPath()
{
	m_RootPath = std::filesystem::current_path();
	m_CurrentPath = m_RootPath;
	m_ShareFilePath = std::filesystem::absolute(std::filesystem::path("share"));
	if (!std::filesystem::exists(m_ShareFilePath)) { std::filesystem::create_directory(m_ShareFilePath); }
}

//*********************************************************************
//FUNCTION:
bool CFileManagement::__isOutRoot(std::string vDir)
{
	std::filesystem::path Path = std::filesystem::absolute(std::filesystem::path(vDir));
	if (Path.string().find(m_RootPath.string()) != Path.string().npos)
		return false;
	return true;
}
