#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include "weather.h"
#include "logger.h"
#include "../config/config.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cctype>
#include "network.h"

#pragma comment(lib, "ws2_32.lib")

// ================================================================
// URL 编码：空格 → +，非 ASCII/特殊字符 → %XX
// ================================================================
static std::string UrlEncode(const std::string& src)
{
    std::string result;
    result.reserve(src.size() * 3);
    for (char c : src) {
        if (c == ' ') {
            result += '+';
        } else if (isalnum((unsigned char)c) ||
                   c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            char hex[4];
            sprintf(hex, "%%%02X", (unsigned char)c);
            result += hex;
        }
    }
    return result;
}

// ================================================================
// UTF-8 → ANSI (CP_ACP) 转换，用于 wttr.in 返回的中文
// ================================================================
static std::string Utf8ToAnsi(const std::string& utf8)
{
    if (utf8.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (wlen <= 0) return utf8;
    std::wstring wide(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wlen);
    int alen = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    if (alen <= 0) return utf8;
    std::string ansi(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &ansi[0], alen, NULL, NULL);
    if (!ansi.empty() && ansi.back() == '\0') ansi.pop_back();
    return ansi;
}

// ================================================================
// 主函数：从 wttr.in 获取天气（支持中/英文）
// ================================================================
WeatherInfo FetchWeather(const std::string& city)
{
    WeatherInfo result = {false, "", "", "", "", ""};

    if (city.empty()) {
        WriteLog("[Weather] 未设置城市");
        return result;
    }

    // ---- 1. 确保 Winsock 已初始化（进程级，幂等） ----
    if (!WinsockInit()) {
        WriteLog("[Weather] Winsock 初始化失败");
        return result;
    }

    // ---- 2. DNS 解析 ----
    struct addrinfo hints = {}, *addrInfo = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("wttr.in", "80", &hints, &addrInfo) != 0) {
        WriteLog("[Weather] DNS 解析 wttr.in 失败");
        return result;
    }

    // ---- 3. 创建套接字并连接 ----
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WriteLog("[Weather] socket() 失败");
        freeaddrinfo(addrInfo);
        return result;
    }

    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    if (connect(sock, addrInfo->ai_addr, (int)addrInfo->ai_addrlen) == SOCKET_ERROR) {
        WriteLog("[Weather] connect() 失败");
        closesocket(sock);
        freeaddrinfo(addrInfo);
        return result;
    }

    freeaddrinfo(addrInfo);

    // ---- 4. 构造 HTTP GET 请求（含语言参数）----
    std::string encodedCity = UrlEncode(city);

    // 根据语言设置决定 lang 参数
    std::string langParam;
    AppConfig& cfg = AppConfig::GetInstance();
    if (cfg.language == AppLanguage::ZH_CN) {
        langParam = "&lang=zh";
    } else {
        langParam = "&lang=en";
    }

    // 构造完整 HTTP 请求
    std::string httpRequest;
    httpRequest += "GET /" + encodedCity + "?format=%C+%t+%h+%w" + langParam + " HTTP/1.0\r\n";
    httpRequest += "Host: wttr.in\r\n";
    httpRequest += "User-Agent: GUIC_2.0/1.0\r\n";
    httpRequest += "Accept-Language: zh-CN,zh;q=0.9\r\n";
    httpRequest += "Connection: close\r\n";
    httpRequest += "\r\n";

    char logBuf[256];
    sprintf(logBuf, "[Weather] 请求: %s (lang=%s)",
            encodedCity.c_str(),
            cfg.language == AppLanguage::ZH_CN ? "zh" : "en");
    WriteLog(logBuf);

    int sent = send(sock, httpRequest.c_str(), (int)httpRequest.size(), 0);
    if (sent <= 0) {
        WriteLog("[Weather] send() 失败");
        closesocket(sock);
        return result;
    }

    // ---- 5. 接收响应 ----
    std::string response;
    char buffer[256];
    int recvLen;
    while ((recvLen = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[recvLen] = '\0';
        response += buffer;
        if (response.size() > 4096) break;
    }

    closesocket(sock);

    if (response.empty()) {
        WriteLog("[Weather] 收到空响应");
        return result;
    }

    // ---- 6. 去掉 HTTP 头 ----
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        response = response.substr(headerEnd + 4);
    }

    // ---- 7. 解析 ----
    return ParseWeatherResponse(response);
}

// ================================================================
// 解析 wttr.in 简单格式返回值
// 格式: "Condition Temp Humidity Wind"
// 示例: "晴 +22°C 58% 15km/h"
// ================================================================
WeatherInfo ParseWeatherResponse(const std::string& response)
{
    WeatherInfo result = {false, "", "", "", "", response};

    if (response.empty()) return result;

    // 去除首尾空白
    std::string s = response;
    while (!s.empty() && (s[0] == ' ' || s[0] == '\n' || s[0] == '\r' || s[0] == '\t'))
        s.erase(0, 1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n' || s.back() == '\r' || s.back() == '\t'))
        s.pop_back();

    if (s.empty()) return result;

    // 按空格分割（最多4段：condition, temp, humidity, wind）
    size_t p1 = s.find(' ');
    if (p1 == std::string::npos) {
        result.condition = Utf8ToAnsi(s);
        result.valid     = true;
    } else {
        size_t p2 = s.find(' ', p1 + 1);
        if (p2 == std::string::npos) {
            result.condition = Utf8ToAnsi(s.substr(0, p1));
            result.temp      = Utf8ToAnsi(s.substr(p1 + 1));
            result.valid     = true;
        } else {
            size_t p3 = s.find(' ', p2 + 1);
            if (p3 == std::string::npos) p3 = s.size();

            result.condition = Utf8ToAnsi(s.substr(0, p1));
            result.temp      = Utf8ToAnsi(s.substr(p1 + 1, p2 - p1 - 1));
            result.humidity  = Utf8ToAnsi(s.substr(p2 + 1, p3 - p2 - 1));
            result.wind      = (p3 < s.size()) ? Utf8ToAnsi(s.substr(p3 + 1)) : "";
            result.valid     = true;
        }
    }

    char buf[256];
    sprintf(buf, "[Weather] 获取成功: %s %s", result.condition.c_str(), result.temp.c_str());
    WriteLog(buf);

    return result;
}
