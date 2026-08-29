#ifndef NETWORK_H
#define NETWORK_H

#include <windows.h>
#include <cstdint>

struct NTPQueryResult {
    int64_t  offset100ns;
    int64_t  delay100ns;
    uint64_t serverTxNtp;
    uint8_t  stratum;
    uint8_t  leapIndicator;
    bool     valid;
};

// ======================== Winsock 进程级初始化 ========================
// 修复：原实现中 GetNetworkTime/FetchWeather 各自 WSAStartup/WSACleanup，
// 导致 HTTP/NTP 服务器线程在客户端清理后 socket() 报 WSANOTINITIALISED (10093)。
// 现改为进程级一次性初始化：WinMain 启动时初始化、退出时清理；
// 各客户端/服务端函数仅防御性调用（幂等）。
bool WinsockInit();
void WinsockShutdown();
//   customServer: NULL 使用内置服务器池，否则只用该服务器
bool GetNetworkTime(SYSTEMTIME& st,
                    int64_t* pBestOffset100ns = nullptr,
                    int64_t* pBestDelay100ns  = nullptr,
                    const char* customServer   = nullptr);

bool QuerySingleNtpServer(const char* server, NTPQueryResult& result);

#endif // NETWORK_H
