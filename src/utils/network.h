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

// ======================== 本机 NTP 同步状态（局域网 NTP 服务器引用：stratum-2 根信息） ========================
extern uint32_t g_ntpRootDelayFix;     // 16.16 定点：到上游参考时钟的网络延迟
extern uint32_t g_ntpRootDispersion;   // 16.16 定点：同步不确定性
extern uint64_t g_ntpRefTimestamp;     // 最近成功同步时刻（NTP 时间戳，网络字节序）

#endif // NETWORK_H
