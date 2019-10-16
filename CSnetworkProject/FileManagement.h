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

	[[nodiscard]] std::string getCurrentPath();
	[[nodiscard]] bool changeCurrentPath(std::string vDir);
	[[nodiscard]] bool createDir(std::string vDir);

	void listCurrenPathFileAndDir();

private:
	std::filesystem::path m_RootPath;
	std::filesystem::path m_CurrentPath;
	std::filesystem::path m_ShareFilePath;
	const std::string m_InvalidCharPattern = "[^\\?\"<>\\*\\|:\\.]+";

	[[nodiscard]]bool __isOutRoot(std::string vDir);
	[[nodiscard]]bool __isPathValid(std::string vDir);
	void __initPath();
};


//*********************************************************************
//FUNCTION:
std::string CFileManagement::getCurrentPath()
{
	return m_CurrentPath.string();
}

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
}

//*********************************************************************
//FUNCTION:
bool CFileManagement::__isPathValid(std::string vDir)
{
	return std::regex_match(vDir,std::regex(m_InvalidCharPattern));
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