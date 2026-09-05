// ====== 【修复】必须在所有 include 之前定义 _WIN32_WINNT ======
#define _WIN32_WINNT 0x0602
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "http_server.h"
#include "../config/config.h"
#include "../i18n.h"
#include "network.h"
#include "logger.h"
#include "lunar.h"
#include "lunar_online.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <ctime>

// 在线农历数据（windowproc.cpp 提供）
extern LunarOnlineData g_lunarOnline;

#pragma comment(lib, "ws2_32.lib")

static volatile LONG g_httpRunning = 0;
static SOCKET g_httpSocket = INVALID_SOCKET;
static HANDLE g_httpThread = NULL;

static std::string UrlDecode(const std::string& s)
{
    std::string result;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int hi = s[i+1], lo = s[i+2];
            auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
            };
            result += (char)(hex((char)hi) * 16 + hex((char)lo));
            i += 2;
        } else if (s[i] == '+') {
            result += ' ';
        } else {
            result += s[i];
        }
    }
    return result;
}

static void ApplySetting(const std::string& key, const std::string& value)
{
    AppConfig& cfg = AppConfig::GetInstance();
    if (key == "message")          { cfg.message = value; }
    else if (key == "showClock")   { cfg.showClock = (value == "1"); }
    else if (key == "showCountdown") { cfg.showCountdown = (value == "1"); }
    else if (key == "showMessage") { cfg.showMessage = (value == "1"); }
    else if (key == "showLunar")   { cfg.showLunar = (value == "1"); }
    else if (key == "showWeather") { cfg.showWeather = (value == "1"); }
    else if (key == "weatherCity") { cfg.weatherCity = value; }
    else if (key == "fontSize")    { int sz = atoi(value.c_str()); if (sz > 0) cfg.fontSize = sz; }
    else if (key == "dailyRemark") { cfg.dailyRemark = value; }
    else if (key == "language") {
        cfg.language = (atoi(value.c_str()) == 1) ? AppLanguage::EN_US : AppLanguage::ZH_CN;
        I18nInit(cfg.language == AppLanguage::EN_US ? Language::EN_US : Language::ZH_CN);
    }
    cfg.SaveConfig();
}

static std::string BuildHtmlPage()
{
    AppConfig& cfg = AppConfig::GetInstance();

    SYSTEMTIME st;
    cfg.GetCurrentDateTime(st);

    // 农历（在线同步优先，失败回退本地计算）
    std::string tdgz, sx, lunarStr;
    if (g_lunarOnline.valid &&
        g_lunarOnline.fetchTime.wYear == st.wYear &&
        g_lunarOnline.fetchTime.wMonth == st.wMonth &&
        g_lunarOnline.fetchTime.wDay == st.wDay) {
        LunarDate ld = { g_lunarOnline.lunarYear, g_lunarOnline.lunarMonth,
                         g_lunarOnline.lunarDay, g_lunarOnline.isLeapMonth };
        tdgz = GetTianGanDiZhi(ld.year);
        sx = GetShengXiao(ld.year);
        lunarStr = LunarToString(ld);
    } else {
        SolarDate sd = { (int)st.wYear, (int)st.wMonth, (int)st.wDay };
        LunarDate ld = SolarToLunar(sd);
        tdgz = GetTianGanDiZhi(ld.year);
        sx = GetShengXiao(ld.year);
        lunarStr = LunarToString(ld);
    }

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head><meta charset=\"gbk\">"
         << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
         << "<title>GUIC 2.0</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;max-width:600px;margin:0 auto;padding:20px;background:#f5f5f5;}"
         << "h1{color:#333;border-bottom:2px solid #4a90d9;padding-bottom:10px;}"
         << ".card{background:#fff;border-radius:8px;padding:15px;margin:10px 0;box-shadow:0 1px 3px rgba(0,0,0,.1);}"
         << ".time{font-size:2em;font-weight:bold;color:#222;}"
         << ".label{color:#888;font-size:.85em;}"
         << ".val{color:#333;font-weight:bold;}"
         << "form{margin:5px 0;}"
         << "input[type=text],select{padding:6px;border:1px solid #ccc;border-radius:4px;width:200px;}"
         << "input[type=submit]{padding:6px 16px;background:#4a90d9;color:#fff;border:none;border-radius:4px;cursor:pointer;}"
         << ".toggle{display:inline-block;padding:4px 12px;border-radius:4px;text-decoration:none;color:#fff;margin:2px;}"
         << ".on{background:#4caf50;} .off{background:#e74c3c;}"
         << "</style></head><body>"
         << "<h1>" << _S(STR_WEB_TITLE) << "</h1>"

         << "<div class=\"card\">"
         << "<div class=\"time\">"
         << st.wYear << "-" << (st.wMonth<10?"0":"") << st.wMonth << "-"
         << (st.wDay<10?"0":"") << st.wDay << " "
         << (st.wHour<10?"0":"") << st.wHour << ":"
         << (st.wMinute<10?"0":"") << st.wMinute << ":"
         << (st.wSecond<10?"0":"") << st.wSecond
         << "</div>"
         << "<div class=\"label\">" << tdgz << sx
         << "年 " << lunarStr << "</div>"
         << "</div>"

         << "<div class=\"card\">"
         << "<span class=\"label\">" << _S(STR_WEB_CLOCK) << "</span> "
         << "<a class=\"toggle " << (cfg.showClock?"on":"off") << "\" href=\"/set?showClock="
         << (cfg.showClock?"0":"1") << "\">" << (cfg.showClock?_S(STR_WEB_ON):_S(STR_WEB_OFF)) << "</a> "
         << "<span class=\"label\">" << _S(STR_WEB_COUNTDOWN) << "</span> "
         << "<a class=\"toggle " << (cfg.showCountdown?"on":"off") << "\" href=\"/set?showCountdown="
         << (cfg.showCountdown?"0":"1") << "\">" << (cfg.showCountdown?_S(STR_WEB_ON):_S(STR_WEB_OFF)) << "</a> "
         << "<span class=\"label\">" << _S(STR_WEB_MESSAGE) << "</span> "
         << "<a class=\"toggle " << (cfg.showMessage?"on":"off") << "\" href=\"/set?showMessage="
         << (cfg.showMessage?"0":"1") << "\">" << (cfg.showMessage?_S(STR_WEB_ON):_S(STR_WEB_OFF)) << "</a> "
         << "<span class=\"label\">" << _S(STR_WEB_LUNAR) << "</span> "
         << "<a class=\"toggle " << (cfg.showLunar?"on":"off") << "\" href=\"/set?showLunar="
         << (cfg.showLunar?"0":"1") << "\">" << (cfg.showLunar?_S(STR_WEB_ON):_S(STR_WEB_OFF)) << "</a> "
         << "<span class=\"label\">" << _S(STR_WEB_WEATHER) << "</span> "
         << "<a class=\"toggle " << (cfg.showWeather?"on":"off") << "\" href=\"/set?showWeather="
         << (cfg.showWeather?"0":"1") << "\">" << (cfg.showWeather?_S(STR_WEB_ON):_S(STR_WEB_OFF)) << "</a> "
         << "</div>"

         << "<div class=\"card\">"
         << "<form action=\"/set\" method=\"get\">"
         << "<span class=\"label\">" << _S(STR_WEB_MSG_LABEL) << "</span><br>"
         << "<input type=\"text\" name=\"message\" value=\"" << cfg.message << "\">"
         << "<input type=\"submit\" value=\"" << _S(STR_WEB_SET) << "\">"
         << "</form>"
         << "<form action=\"/set\" method=\"get\" style=\"margin-top:8px;\">"
         << "<span class=\"label\">" << _S(STR_WEB_CITY_LABEL) << "</span><br>"
         << "<input type=\"text\" name=\"weatherCity\" value=\"" << cfg.weatherCity << "\">"
         << "<input type=\"submit\" value=\"" << _S(STR_WEB_SET) << "\">"
         << "</form>"
         << "<form action=\"/set\" method=\"get\" style=\"margin-top:8px;\">"
         << "<span class=\"label\">" << _S(STR_WEB_FONT_LABEL) << "</span><br>"
         << "<input type=\"text\" name=\"fontSize\" value=\"" << cfg.fontSize << "\">"
         << "<input type=\"submit\" value=\"" << _S(STR_WEB_SET) << "\">"
         << "</form>"
         << "<form action=\"/set\" method=\"get\" style=\"margin-top:8px;\">"
         << "<span class=\"label\">" << _S(STR_WEB_REMARK_LABEL) << "</span><br>"
         << "<input type=\"text\" name=\"dailyRemark\" value=\"" << cfg.dailyRemark << "\">"
         << "<input type=\"submit\" value=\"" << _S(STR_WEB_SET) << "\">"
         << "</form>"
         << "</div>"

         << "<div class=\"card\">"
         << "<span class=\"label\">" << _S(STR_LANGUAGE) << ":</span> "
         << "<a class=\"toggle on\" href=\"/set?language=0\">" << _S(STR_LANGUAGE_ZH) << "</a> "
         << "<a class=\"toggle off\" href=\"/set?language=1\">" << _S(STR_LANGUAGE_EN) << "</a> "
         << "</div>"

         << "<div class=\"card\">"
         << "<a href=\"/\" style=\"color:#4a90d9;\">" << _S(STR_WEB_REFRESH) << "</a>"
         << "</div>"
         << "</body></html>";
    return html.str();
}

static DWORD WINAPI HttpServerThread(LPVOID param)
{
    unsigned short port = (unsigned short)(uintptr_t)param;
    char buf[128];
    sprintf(buf, "[HTTP] 监听端口 %d", port);
    WriteLog(buf);

    // 【修复】服务器线程不负责全局初始化，但防御性确保 Winsock 已初始化
    WinsockInit();

    g_httpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_httpSocket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        sprintf(buf, "[HTTP] socket() 失败, 错误码: %d", err);
        WriteLog(buf);
        InterlockedExchange(&g_httpRunning, 0);
        return 1;
    }

    int opt = 1;
    setsockopt(g_httpSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(g_httpSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        sprintf(buf, "[HTTP] bind() 失败, 错误码: %d (端口 %d 可能被占用)", err, port);
        WriteLog(buf);
        closesocket(g_httpSocket);
        g_httpSocket = INVALID_SOCKET;
        InterlockedExchange(&g_httpRunning, 0);
        return 1;
    }

    listen(g_httpSocket, 5);

    int timeout = 1000;
    setsockopt(g_httpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    WriteLog("[HTTP] 开始监听");

    while (InterlockedCompareExchange(&g_httpRunning, 1, 1) == 1)
    {
        // select 超时轮询：停止时线程 1 秒内退出（accept 无法用 SO_RCVTIMEO 打断）
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_httpSocket, &fds);
        struct timeval tv;
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        int selRet = select(0, &fds, NULL, NULL, &tv);
        if (selRet <= 0) continue;

        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET client = accept(g_httpSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (client == INVALID_SOCKET) continue;

        char reqBuf[4096];
        int reqLen = recv(client, reqBuf, sizeof(reqBuf) - 1, 0);
        if (reqLen > 0) {
            reqBuf[reqLen] = '\0';
            std::string request(reqBuf);

            std::string response;
            std::string line1 = request.substr(0, request.find("\r\n"));

            if (line1.find("GET /set?") != std::string::npos) {
                size_t qs = request.find('?') + 1;
                size_t qe = request.find(' ', qs);
                std::string query = request.substr(qs, qe - qs);
                size_t eq = query.find('=');
                if (eq != std::string::npos) {
                    std::string key = query.substr(0, eq);
                    std::string val = UrlDecode(query.substr(eq + 1));
                    ApplySetting(key, val);
                }
                std::string body = BuildHtmlPage();
                response = "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=gbk\r\n"
                           "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
            } else if (line1.find("GET / ") == 0 || line1.find("GET / HTTP") == 0 ||
                       line1.find("GET /?") == 0 || line1.find("GET /favicon") != std::string::npos) {
                std::string body = BuildHtmlPage();
                response = "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=gbk\r\n"
                           "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
            } else {
                std::string body = "<h1>404 Not Found</h1>";
                response = "HTTP/1.0 404 Not Found\r\nContent-Length: "
                           + std::to_string(body.size()) + "\r\n\r\n" + body;
            }
            send(client, response.c_str(), (int)response.size(), 0);
        }
        closesocket(client);
    }

    closesocket(g_httpSocket);
    g_httpSocket = INVALID_SOCKET;
    WriteLog("[HTTP] 已停止");
    return 0;
}

HANDLE StartHttpServer(unsigned short port)
{
    if (InterlockedCompareExchange(&g_httpRunning, 1, 0) != 0)
        return NULL;
    HANDLE hThread = CreateThread(NULL, 0, HttpServerThread,
                                  (LPVOID)(uintptr_t)port, 0, NULL);
    if (!hThread) {
        InterlockedExchange(&g_httpRunning, 0);
        return NULL;
    }
    g_httpThread = hThread;
    return hThread;
}

void StopHttpServer()
{
    InterlockedExchange(&g_httpRunning, 0);
    if (g_httpThread) {
        WaitForSingleObject(g_httpThread, 3000);
        CloseHandle(g_httpThread);
        g_httpThread = NULL;
    }
}

bool IsHttpServerRunning()
{
    return InterlockedCompareExchange(&g_httpRunning, 1, 1) == 1;
}
