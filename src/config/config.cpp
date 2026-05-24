#include "config.h"
#include "../utils/logger.h"
#include "../utils/utils.h"
#include "../utils/network.h"
#include "../main.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>

// 单例实现
AppConfig& AppConfig::GetInstance() {
    static AppConfig instance;
    return instance;
}

AppConfig::AppConfig()
    : message("欢迎使用图形化时钟！")
    , showMessage(true)
    , showClock(true)
    , showCountdown(true)
    , fontSize(20)
    , autoLayout(true)
    , fontName("Arial")
    , fontColor(RGB(0, 0, 0))
    , resizable(true)
    , windowWidth(500)
    , windowHeight(400)
    , dailyRemark("每日倒计时备注")
    , countdownMode(COUNTDOWN_ONCE)
    , countdownTarget({0})
    , useNetworkTime(false)
    , lastNetworkTime({0})
    , localTimeOffset(0)
{
    ZeroMemory(configFile, sizeof(configFile));
    ZeroMemory(specialDaysFile, sizeof(specialDaysFile));
}

void AppConfig::GetCurrentDateTime(SYSTEMTIME& st)
{
    if (useNetworkTime)
    {
        SYSTEMTIME localNow;
        GetLocalTime(&localNow);

        FILETIME ftLocalNow, ftLastNetwork;
        SystemTimeToFileTime(&localNow, &ftLocalNow);
        SystemTimeToFileTime(&lastNetworkTime, &ftLastNetwork);

        ULARGE_INTEGER ulLocal, ulNetwork;
        ulLocal.LowPart = ftLocalNow.dwLowDateTime;
        ulLocal.HighPart = ftLocalNow.dwHighDateTime;
        ulNetwork.LowPart = ftLastNetwork.dwLowDateTime;
        ulNetwork.HighPart = ftLastNetwork.dwHighDateTime;

        ULARGE_INTEGER ulCurrent;
        ulCurrent.QuadPart = ulLocal.QuadPart + localTimeOffset;

        FILETIME ftCurrent;
        ftCurrent.dwLowDateTime = ulCurrent.LowPart;
        ftCurrent.dwHighDateTime = ulCurrent.HighPart;

        SYSTEMTIME utcTime;
        FileTimeToSystemTime(&ftCurrent, &utcTime);

        if (!SystemTimeToTzSpecificLocalTime(NULL, &utcTime, &st)) {
            st = utcTime;
            WriteLog("UTC 到本地时间转换失败，使用 UTC 时间");
        }

        if (st.wYear < 2020 || st.wYear > 2100) {
            WriteLog("计算出的网络时间异常，回退本地时间");
            GetLocalTime(&st);
            useNetworkTime = false;
        }
    }
    else
    {
        GetLocalTime(&st);
    }
}

void AppConfig::SyncNetworkTime()
{
    SYSTEMTIME networkTime;
    if (GetNetworkTime(networkTime))
    {
        SYSTEMTIME localNow;
        GetLocalTime(&localNow);

        FILETIME ftLocal, ftNetwork;
        SystemTimeToFileTime(&localNow, &ftLocal);
        SystemTimeToFileTime(&networkTime, &ftNetwork);

        ULARGE_INTEGER ulLocal, ulNetwork;
        ulLocal.LowPart = ftLocal.dwLowDateTime;
        ulLocal.HighPart = ftLocal.dwHighDateTime;
        ulNetwork.LowPart = ftNetwork.dwLowDateTime;
        ulNetwork.HighPart = ftNetwork.dwHighDateTime;

        LONGLONG newOffset = (LONGLONG)(ulNetwork.QuadPart - ulLocal.QuadPart);

        if (useNetworkTime) {
            // 指数平滑偏移（alpha = 0.3）
            localTimeOffset = (LONGLONG)(localTimeOffset * 0.7 + newOffset * 0.3);
        } else {
            localTimeOffset = newOffset;
        }

        lastNetworkTime = networkTime;
        useNetworkTime = true;
        WriteLog("网络时间同步成功，偏移: " + std::to_string(localTimeOffset) + " 100ns");
    }
    else
    {
        static int failCount = 0;
        failCount++;
        if (failCount >= 3) {
            useNetworkTime = false;
            WriteLog("连续 3 次网络时间同步失败，回退到本地时间");
            failCount = 0;
        } else {
            WriteLog("网络时间同步失败，仍保持当前模式");
        }
    }
}

void AppConfig::InitPaths()
{
    GetModuleFileNameA(NULL, configFile, MAX_PATH);
    RemoveFileNameFromPath(configFile);
    strcpy(specialDaysFile, configFile);
    strcat(configFile, "config.ini");
    strcat(specialDaysFile, "special_days.txt");
    WriteLog("配置文件路径: " + std::string(configFile));
    WriteLog("特殊日子文件路径: " + std::string(specialDaysFile));
}

void AppConfig::SaveConfig()
{
    if (strlen(configFile) == 0) InitPaths();
    WriteLog("保存配置到: " + std::string(configFile));
    char buf[32];
    sprintf(buf, "%d", windowWidth);
    WritePrivateProfileString("Window", "Width", buf, configFile);
    sprintf(buf, "%d", windowHeight);
    WritePrivateProfileString("Window", "Height", buf, configFile);
    WritePrivateProfileString("Window", "Resizable", resizable ? "1" : "0", configFile);
    sprintf(buf, "%d", fontSize);
    WritePrivateProfileString("Font", "Size", buf, configFile);
    WritePrivateProfileString("Font", "Name", fontName.c_str(), configFile);
    sprintf(buf, "%d", GetRValue(fontColor));
    WritePrivateProfileString("Color", "R", buf, configFile);
    sprintf(buf, "%d", GetGValue(fontColor));
    WritePrivateProfileString("Color", "G", buf, configFile);
    sprintf(buf, "%d", GetBValue(fontColor));
    WritePrivateProfileString("Color", "B", buf, configFile);
    char timeStr[32];
    sprintf(timeStr, "%02d:%02d:%02d",
            countdownTarget.dailyTime.wHour,
            countdownTarget.dailyTime.wMinute,
            countdownTarget.dailyTime.wSecond);
    WritePrivateProfileString("Countdown", "DailyTime", timeStr, configFile);
    WritePrivateProfileString("Countdown", "Remark", dailyRemark.c_str(), configFile);
    sprintf(buf, "%d", countdownMode);
    WritePrivateProfileString("Countdown", "Mode", buf, configFile);
    WritePrivateProfileString("Message", "Content", message.c_str(), configFile);
    WritePrivateProfileString("Display", "ShowClock", showClock ? "1" : "0", configFile);
    WritePrivateProfileString("Display", "ShowCountdown", showCountdown ? "1" : "0", configFile);
    WritePrivateProfileString("Display", "ShowMessage", showMessage ? "1" : "0", configFile);
    WritePrivateProfileString("Display", "AutoLayout", autoLayout ? "1" : "0", configFile);
    WriteLog("配置已保存到config.ini");
}

void AppConfig::LoadConfig()
{
    if (strlen(configFile) == 0) InitPaths();
    WriteLog("尝试从配置文件加载设置: " + std::string(configFile));
    char buffer[256];
    DWORD fileAttributes = GetFileAttributesA(configFile);
    bool configFileExists = (fileAttributes != INVALID_FILE_ATTRIBUTES &&
                            !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    if (!configFileExists) {
        WriteLog("配置文件不存在，创建默认配置");
        countdownTarget.dailyTime.wHour = 20;
        countdownTarget.dailyTime.wMinute = 10;
        countdownTarget.dailyTime.wSecond = 0;
        dailyRemark = "每日倒计时备注";
        SaveConfig();
        return;
    }
    WriteLog("配置文件存在，开始加载配置");
    GetPrivateProfileString("Window", "Width", "500", buffer, sizeof(buffer), configFile);
    windowWidth = atoi(buffer);
    GetPrivateProfileString("Window", "Height", "400", buffer, sizeof(buffer), configFile);
    windowHeight = atoi(buffer);
    GetPrivateProfileString("Window", "Resizable", "1", buffer, sizeof(buffer), configFile);
    resizable = (atoi(buffer) == 1);
    GetPrivateProfileString("Font", "Size", "20", buffer, sizeof(buffer), configFile);
    fontSize = atoi(buffer);
    GetPrivateProfileString("Font", "Name", "Arial", buffer, sizeof(buffer), configFile);
    fontName = buffer;
    GetPrivateProfileString("Color", "R", "0", buffer, sizeof(buffer), configFile);
    int r = atoi(buffer);
    GetPrivateProfileString("Color", "G", "0", buffer, sizeof(buffer), configFile);
    int g = atoi(buffer);
    GetPrivateProfileString("Color", "B", "0", buffer, sizeof(buffer), configFile);
    int b = atoi(buffer);
    fontColor = RGB(r, g, b);
    GetPrivateProfileString("Countdown", "DailyTime", "20:10:00", buffer, sizeof(buffer), configFile);
    int hour, minute, second;
    if (sscanf(buffer, "%d:%d:%d", &hour, &minute, &second) == 3) {
        countdownTarget.dailyTime.wHour = (WORD)hour;
        countdownTarget.dailyTime.wMinute = (WORD)minute;
        countdownTarget.dailyTime.wSecond = (WORD)second;
    } else {
        countdownTarget.dailyTime.wHour = 20;
        countdownTarget.dailyTime.wMinute = 10;
        countdownTarget.dailyTime.wSecond = 0;
    }
    GetPrivateProfileString("Countdown", "Remark", "每日倒计时备注", buffer, sizeof(buffer), configFile);
    dailyRemark = buffer;
    GetPrivateProfileString("Countdown", "Mode", "1", buffer, sizeof(buffer), configFile);
    countdownMode = (CountdownMode)atoi(buffer);
    GetPrivateProfileString("Message", "Content", "欢迎使用图形化时钟！", buffer, sizeof(buffer), configFile);
    message = buffer;
    GetPrivateProfileString("Display", "ShowClock", "1", buffer, sizeof(buffer), configFile);
    showClock = (atoi(buffer) == 1);
    GetPrivateProfileString("Display", "ShowCountdown", "1", buffer, sizeof(buffer), configFile);
    showCountdown = (atoi(buffer) == 1);
    GetPrivateProfileString("Display", "ShowMessage", "1", buffer, sizeof(buffer), configFile);
    showMessage = (atoi(buffer) == 1);
    GetPrivateProfileString("Display", "AutoLayout", "1", buffer, sizeof(buffer), configFile);
    autoLayout = (atoi(buffer) == 1);
    WriteLog("配置加载完成");
}

std::string AppConfig::GetSpecialDay(int year, int month, int day, int hour, int minute, int second)
{
    std::string result;
    for (const auto& sd : specialDays) {
        if (sd.month == month && sd.day == day) {
            if (sd.isAnnual || (!sd.isAnnual && sd.year == year)) {
                if (!result.empty()) result += " ";
                result += sd.name;
                if (sd.isSolarTerm && sd.hour >= 0 && sd.minute >= 0) {
                    char timeStr[32];
                    sprintf(timeStr, " (%02d:%02d)", sd.hour, sd.minute);
                    result += timeStr;
                }
            }
        }
    }
    return result;
}

void AppConfig::InitSpecialDays()
{
    specialDays.clear();
    WriteLog("初始化特殊日子数据");
    LoadSpecialDaysFromFile();
}

void AppConfig::LoadSpecialDaysFromFile()
{
    if (strlen(specialDaysFile) == 0) InitPaths();
    std::ifstream file(specialDaysFile);
    if (!file) {
        WriteLog("special_days.txt 文件不存在，创建默认文件");
        CreateDefaultSpecialDaysFile();
        return;
    }
    std::string line;
    int customCount = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '/') continue;
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos) continue;
        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos) continue;
        size_t pos3 = line.find('|', pos2 + 1);
        if (pos3 == std::string::npos) continue;
        size_t pos4 = line.find('|', pos3 + 1);
        if (pos4 == std::string::npos) continue;
        size_t pos5 = line.find('|', pos4 + 1);
        if (pos5 == std::string::npos) continue;
        size_t pos6 = line.find('|', pos5 + 1);
        if (pos6 == std::string::npos) continue;
        size_t pos7 = line.find('|', pos6 + 1);
        if (pos7 == std::string::npos) continue;
        SpecialDay sd;
        sd.name = line.substr(0, pos1);
        sd.month = atoi(line.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
        sd.day = atoi(line.substr(pos2 + 1, pos3 - pos2 - 1).c_str());
        sd.hour = atoi(line.substr(pos3 + 1, pos4 - pos3 - 1).c_str());
        sd.minute = atoi(line.substr(pos4 + 1, pos5 - pos4 - 1).c_str());
        sd.isAnnual = (atoi(line.substr(pos5 + 1, pos6 - pos5 - 1).c_str()) == 1);
        sd.year = atoi(line.substr(pos6 + 1, pos7 - pos6 - 1).c_str());
        sd.isSolarTerm = (atoi(line.substr(pos7 + 1).c_str()) == 1);
        specialDays.push_back(sd);
        customCount++;
    }
    file.close();
    WriteLog("从 special_days.txt 加载了 " + std::to_string(customCount) + " 条特殊日子");
}

void AppConfig::SaveSpecialDaysToFile()
{
    if (strlen(specialDaysFile) == 0) InitPaths();
    std::ofstream file(specialDaysFile);
    if (file.is_open()) {
        file << "# 特殊日子配置文件" << std::endl;
        file << "# 格式：名称|月份|日期|小时|分钟|是否每年重复(1/0)|年份(如果是0则忽略)|是否节气(1/0)" << std::endl;
        file << "# 示例：" << std::endl;
        for (const auto& sd : specialDays) {
            file << sd.name << "|" << sd.month << "|" << sd.day << "|"
                 << sd.hour << "|" << sd.minute << "|"
                 << (sd.isAnnual ? "1" : "0") << "|" << sd.year << "|"
                 << (sd.isSolarTerm ? "1" : "0") << std::endl;
        }
        file.close();
        WriteLog("特殊日子已保存到 special_days.txt");
    }
}

void AppConfig::CreateDefaultSpecialDaysFile()
{
    if (strlen(specialDaysFile) == 0) InitPaths();
    std::ofstream file(specialDaysFile);
    if (file.is_open()) {
        file << "# 特殊日子配置文件" << std::endl;
        file << "# 格式：名称|月份|日期|小时|分钟|是否每年重复(1/0)|年份(如果是0则忽略)|是否节气(1/0)" << std::endl;
        file << "# 示例：" << std::endl;
        // 一般节日
        file << "元旦|1|1|0|0|1|0|0" << std::endl;
        file << "春节|2|1|0|0|1|0|0" << std::endl;
        file << "元宵节|2|15|0|0|1|0|0" << std::endl;
        file << "情人节|2|14|0|0|1|0|0" << std::endl;
        file << "妇女节|3|8|0|0|1|0|0" << std::endl;
        file << "植树节|3|12|0|0|1|0|0" << std::endl;
        file << "清明节|4|4|0|0|1|0|0" << std::endl;
        file << "劳动节|5|1|0|0|1|0|0" << std::endl;
        file << "青年节|5|4|0|0|1|0|0" << std::endl;
        file << "儿童节|6|1|0|0|1|0|0" << std::endl;
        file << "建党节|7|1|0|0|1|0|0" << std::endl;
        file << "建军节|8|1|0|0|1|0|0" << std::endl;
        file << "教师节|9|10|0|0|1|0|0" << std::endl;
        file << "国庆节|10|1|0|0|1|0|0" << std::endl;
        file << "平安夜|12|24|0|0|1|0|0" << std::endl;
        file << "圣诞节|12|25|0|0|1|0|0" << std::endl;
        // 国际节日
        file << "世界地球日|4|22|0|0|1|0|0" << std::endl;
        file << "世界环境日|6|5|0|0|1|0|0" << std::endl;
        file << "世界人口日|7|11|0|0|1|0|0" << std::endl;
        file << "世界粮食日|10|16|0|0|1|0|0" << std::endl;
        file << "世界艾滋病日|12|1|0|0|1|0|0" << std::endl;
        // 纪念日
        file << "                          |6|4|0|0|1|0|0" << std::endl;
        file << "七七事变纪念日|7|7|0|0|1|0|0" << std::endl;
        file << "九一八事变纪念日|9|18|0|0|1|0|0" << std::endl;
        file << "十月革命胜利纪念日|11|7|0|0|1|0|0" << std::endl;
        file << "南京大屠杀纪念日|12|13|0|0|1|0|0" << std::endl;
        file << "苏联成立纪念日|12|30|0|0|1|0|0" << std::endl;
        // 节气
        file << "立春|2|4|12|30|1|0|1" << std::endl;
        file << "雨水|2|19|1|45|1|0|1" << std::endl;
        file << "惊蛰|3|5|8|20|1|0|1" << std::endl;
        file << "春分|3|20|20|15|1|0|1" << std::endl;
        file << "清明|4|4|15|30|1|0|1" << std::endl;
        file << "谷雨|4|20|7|10|1|0|1" << std::endl;
        file << "立夏|5|5|18|45|1|0|1" << std::endl;
        file << "小满|5|21|10|30|1|0|1" << std::endl;
        file << "芒种|6|5|22|15|1|0|1" << std::endl;
        file << "夏至|6|21|14|20|1|0|1" << std::endl;
        file << "小暑|7|7|6|50|1|0|1" << std::endl;
        file << "大暑|7|22|19|30|1|0|1" << std::endl;
        file << "立秋|8|7|11|15|1|0|1" << std::endl;
        file << "处暑|8|23|3|40|1|0|1" << std::endl;
        file << "白露|9|7|16|25|1|0|1" << std::endl;
        file << "秋分|9|23|8|55|1|0|1" << std::endl;
        file << "寒露|10|8|1|30|1|0|1" << std::endl;
        file << "霜降|10|23|14|5|1|0|1" << std::endl;
        file << "立冬|11|7|6|35|1|0|1" << std::endl;
        file << "小雪|11|22|19|10|1|0|1" << std::endl;
        file << "大雪|12|7|11|45|1|0|1" << std::endl;
        file << "冬至|12|21|23|20|1|0|1" << std::endl;
        file << "小寒|1|5|15|50|1|0|1" << std::endl;
        file << "大寒|1|20|7|25|1|0|1" << std::endl;
        file.close();
        WriteLog("创建默认 special_days.txt 文件");
    }
}
