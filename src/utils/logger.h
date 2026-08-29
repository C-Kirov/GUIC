#ifndef LOGGER_H
#define LOGGER_H

#include <string>

void WriteLog(const std::string& message);
void WriteSystemInfoIfNeeded();
std::string GetSystemInfo();
std::string GetCurrentDateTimeString();

//   新增：记录 Win32 API 错误到日志
void LogWin32Error(const char* context);

#endif // LOGGER_H
