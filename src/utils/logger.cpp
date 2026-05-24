#include "logger.h"
#include "utils.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <ctime>

std::string GetCurrentDateTimeString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeStr[256];
    sprintf(timeStr, "%02d-%02d-%02d,%02d:%02d:%02d",
            st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(timeStr);
}

void WriteLog(const std::string& message)
{
    char logFilePath[MAX_PATH];
    GetModuleFileNameA(NULL, logFilePath, MAX_PATH);
    RemoveFileNameFromPath(logFilePath);
    strcat(logFilePath, "log.txt");
    std::ofstream logFile(logFilePath, std::ios::app);
    if (logFile.is_open()) {
        logFile << GetCurrentDateTimeString() << " - " << message << std::endl;
        logFile.close();
    }
}

std::string GetSystemInfo()
{
    std::stringstream ss;
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    char computerName[256];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);
    char userName[256];
    DWORD userSize = sizeof(userName);
    GetUserNameA(userName, &userSize);
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    ss << "系统信息: " << std::endl;
    ss << "  计算机名: " << computerName << std::endl;
    ss << "  用户名: " << userName << std::endl;
    ss << "  操作系统: Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << std::endl;
    ss << "  处理器数量: " << sysInfo.dwNumberOfProcessors << std::endl;
    ss << "  处理器类型: " << sysInfo.dwProcessorType << std::endl;
    ss << "  内存页面大小: " << sysInfo.dwPageSize << " 字节" << std::endl;
    return ss.str();
}

void WriteSystemInfoIfNeeded()
{
    char logFilePath[MAX_PATH];
    GetModuleFileNameA(NULL, logFilePath, MAX_PATH);
    RemoveFileNameFromPath(logFilePath);
    strcat(logFilePath, "log.txt");
    std::ifstream logFile(logFilePath);
    std::string line;
    std::string today = GetCurrentDateTimeString().substr(0, 8);
    bool foundTodayInfo = false;
    while (std::getline(logFile, line)) {
        if (line.find(today) != std::string::npos && line.find("系统信息") != std::string::npos) {
            foundTodayInfo = true;
            break;
        }
    }
    logFile.close();
    if (!foundTodayInfo) {
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << GetSystemInfo() << std::endl << std::endl;
            logFile.close();
        }
    }
}
