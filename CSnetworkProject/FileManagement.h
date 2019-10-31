#pragma once
#include<iostream>
#include<iomanip>
#include<cstring>
#include<string>
#include<filesystem>
#include<regex>


constexpr int MAXFILELEGTH = 256;
//便于文件在网络中传输，不带含有指针的类std::string
struct SFile
{
	char FilePath[MAXFILELEGTH];
	char FileName[MAXFILELEGTH];
	uintmax_t FileSize;
	bool IsDir = false;
	bool IsExist = false;
};

class CFileManagement
{
public:
	CFileManagement() { __initPath(); }
	~CFileManagement() = default;

	[[nodiscard]] inline std::string getCurrentPath() { return m_CurrentPath.string(); }
	[[nodiscard]] inline std::string getSharePath() { return m_ShareFilePath.string(); }

	[[nodiscard]] bool changeCurrentPath(std::string vPath);
	[[nodiscard]] bool createDir(std::string vPath);
	[[nodiscard]] bool setShareDir(std::string vPath);
	[[nodiscard]] std::pair<SFile, bool> findFile(std::string vFileName);//bool记录文件是否文件是否存在

	void listCurrenPathFileAndDir();

private:
	std::filesystem::path m_RootPath;
	std::filesystem::path m_CurrentPath;
	std::filesystem::path m_ShareFilePath;
	const std::string m_InvalidCharPattern = "[^\\?\"<>\\*\\|:\\.]+";

	[[nodiscard]] bool __isOutRoot(std::string vPath);
	[[nodiscard]] inline bool __isPathValid(std::string vPath) { return true; }//return std::regex_match(vPath, std::regex(m_InvalidCharPattern));
	void __initPath();
};

//*********************************************************************
//FUNCTION:
bool CFileManagement::changeCurrentPath(std::string vPath)
{
	if (!__isPathValid(vPath)||__isOutRoot(vPath))
		return false;
	auto NewPath = std::filesystem::path(vPath);
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
bool CFileManagement::createDir(std::string vPath)
{
	if (!__isPathValid(vPath)||__isOutRoot(vPath))
		return false;
	auto CreatePath = std::filesystem::path(vPath);
	return std::filesystem::create_directory(CreatePath);
}

//*********************************************************************
//FUNCTION:
bool CFileManagement::setShareDir(std::string vPath)
{
	if (!__isPathValid(vPath) || __isOutRoot(vPath))
		return false;
	m_ShareFilePath = m_ShareFilePath = std::filesystem::absolute(std::filesystem::path(vPath));
	return true;
}

//*********************************************************************
//FUNCTION:
std::pair<SFile, bool> CFileManagement::findFile(std::string vFileName)//从根目录开始查找。fixme:暂时无法解决不同目录同名文件/文件夹的问题
{
	auto voFile = std::make_pair(SFile(), false);
	for (auto p : std::filesystem::recursive_directory_iterator(m_RootPath))
	{
		if (p.path().filename().string() == vFileName)
		{
			voFile.second = true;
			voFile.first.IsExist = true;
			strcpy_s(voFile.first.FilePath,std::filesystem::absolute(p.path()).string().c_str());
			strcpy_s(voFile.first.FileName,vFileName.c_str());
			voFile.first.FileSize = std::filesystem::file_size(voFile.first.FileName);
			if (std::filesystem::is_directory(p.path()))
				voFile.first.IsDir = true;
			return voFile;
		}
	}
	return voFile;
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
bool CFileManagement::__isOutRoot(std::string vPath)
{
	std::filesystem::path Path = std::filesystem::absolute(std::filesystem::path(vPath));
	if (Path.string().find(m_RootPath.string()) != Path.string().npos)
		return false;
	return true;
}
