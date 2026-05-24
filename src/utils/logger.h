#ifndef LOGGER_H
#define LOGGER_H

#include <string>

void WriteLog(const std::string& message);
void WriteSystemInfoIfNeeded();
std::string GetSystemInfo();
std::string GetCurrentDateTimeString();

#endif // LOGGER_H
