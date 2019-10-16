#pragma once
#include<iostream>
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

	
	void listAllFileAndDir();

private:
	std::filesystem::path m_RootPath;
	std::filesystem::path m_CurrentPath;
	std::filesystem::path m_ShareFilePath;
	const std::string m_InvalidCharPattern = "[\?\"\\<>\*\|:\.]";

	bool __isPathValid(std::string vPath);
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
	auto CreatePath = std::filesystem::path(vDir);
	return true;
}

//*********************************************************************
//FUNCTION:
void CFileManagement::listAllFileAndDir()
{

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
bool CFileManagement::__isPathValid(std::string vPath)
{

}