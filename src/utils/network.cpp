#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include "network.h"
#include "logger.h"

#pragma comment(lib, "ws2_32.lib")

#define NTP_PORT           123
#define NTP_TIMEOUT_MS     3000
#define MAX_SAMPLE_COUNT   1

static const std::vector<std::string> NTP_SERVERS = {
    "ntp.aliyun.com",
    "ntp.tencent.com",
    "cn.pool.ntp.org",
    "pool.ntp.org",
    "time.windows.com",
    "time.google.com"
};

#pragma pack(push, 1)
struct NTPPacket {
    uint8_t  li_vn_mode;
    uint8_t  stratum;
    uint8_t  poll;
    uint8_t  precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refId;
    uint64_t refTm;
    uint64_t origTm;
    uint64_t rxTm;
    uint64_t txTm;
};
#pragma pack(pop)

static inline uint64_t ntoh64(uint64_t x) {
    union { uint64_t u64; uint32_t u32[2]; } u;
    u.u64 = x;
    uint32_t tmp = ntohl(u.u32[0]);
    u.u32[0] = ntohl(u.u32[1]);
    u.u32[1] = tmp;
    return u.u64;
}

static bool NtpTimestampToSystemTime(uint64_t ntpTimestamp, SYSTEMTIME& st)
{
    // NTP 时间戳前 32 位为秒数（1900-01-01 起算）
    uint32_t seconds = (uint32_t)(ntpTimestamp >> 32);

    // 1601-01-01 到 1900-01-01 相差 9435484800 秒
    // 转换为 100 纳秒间隔：9435484800 * 10000000 = 94354848000000000
    const ULONGLONG FILETIME_EPOCH_DIFF = 9435484800ULL * 10000000ULL;
    ULONGLONG timeIn100ns = FILETIME_EPOCH_DIFF + (ULONGLONG)seconds * 10000000ULL;

    FILETIME ft;
    ft.dwLowDateTime  = (DWORD)(timeIn100ns & 0xFFFFFFFF);
    ft.dwHighDateTime = (DWORD)(timeIn100ns >> 32);

    if (!FileTimeToSystemTime(&ft, &st))
        return false;

    // 合理性检查
    if (st.wYear < 2024 || st.wYear > 2036) return false;
    if (st.wMonth < 1 || st.wMonth > 12 || st.wDay < 1 || st.wDay > 31) return false;
    if (st.wHour > 23 || st.wMinute > 59 || st.wSecond > 59) return false;
    return true;
}

static bool ResolveNtpServer(const char* hostname, struct sockaddr_in& addr)
{
    struct addrinfo hints = {0}, *result = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(hostname, "123", &hints, &result) != 0) {
        WriteLog("DNS 解析失败: " + std::string(hostname));
        return false;
    }
    if (!result) return false;
    addr = *(struct sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);
    return true;
}

static bool QueryNtpServer(const char* server, uint64_t& outTxTimestamp, int64_t& outDelayUs)
{
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        int err = WSAGetLastError();
        char buf[128];
        sprintf(buf, "创建 socket 失败，错误码: %d [%s]", err, server);
        WriteLog(buf);
        return false;
    }

    int timeout = NTP_TIMEOUT_MS;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    struct sockaddr_in serverAddr;
    if (!ResolveNtpServer(server, serverAddr)) {
        closesocket(sock);
        return false;
    }
    serverAddr.sin_port = htons(NTP_PORT);

    NTPPacket request = {0};
    request.li_vn_mode = (0 << 6) | (4 << 3) | 3;

    FILETIME ftBefore;
    GetSystemTimePreciseAsFileTime(&ftBefore);

    int sentBytes = sendto(sock, (char*)&request, sizeof(request), 0,
                           (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes != sizeof(request)) {
        int err = WSAGetLastError();
        char buf[128];
        sprintf(buf, "发送 NTP 请求失败，错误码: %d [%s]", err, server);
        WriteLog(buf);
        closesocket(sock);
        return false;
    }

    NTPPacket response;
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    int recvLen = recvfrom(sock, (char*)&response, sizeof(response), 0,
                           (struct sockaddr*)&fromAddr, &fromLen);

    FILETIME ftAfter;
    GetSystemTimePreciseAsFileTime(&ftAfter);
    closesocket(sock);

    if (recvLen <= 0) {
        int err = WSAGetLastError();
        if (err == 0) err = WSAETIMEDOUT;
        char buf[128];
        sprintf(buf, "接收 NTP 响应失败，错误码: %d (recvLen=%d) [%s]", err, recvLen, server);
        WriteLog(buf);
        return false;
    }
    if (recvLen != sizeof(response)) {
        char buf[128];
        sprintf(buf, "接收 NTP 响应长度异常 (期望 %d, 实际 %d) [%s]", (int)sizeof(response), recvLen, server);
        WriteLog(buf);
        return false;
    }

    ULARGE_INTEGER ulBefore, ulAfter;
    ulBefore.LowPart = ftBefore.dwLowDateTime;
    ulBefore.HighPart = ftBefore.dwHighDateTime;
    ulAfter.LowPart = ftAfter.dwLowDateTime;
    ulAfter.HighPart = ftAfter.dwHighDateTime;
    int64_t diff100ns = (int64_t)(ulAfter.QuadPart - ulBefore.QuadPart);
    outDelayUs = diff100ns / 10;

    uint64_t rawTx = ntoh64(response.txTm);
    if (rawTx == 0) {
        WriteLog(std::string("NTP 服务器返回时间戳为零 [") + server + "]");
        return false;
    }

    outTxTimestamp = rawTx;
    return true;
}

bool GetNetworkTime(SYSTEMTIME& st)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        WriteLog("WSAStartup 失败");
        return false;
    }

    struct Sample { uint64_t txTimestamp; int64_t delayUs; std::string server; };
    std::vector<Sample> allSamples;

    for (const auto& server : NTP_SERVERS) {
        WriteLog("尝试从 NTP 服务器获取时间: " + server);
        for (int attempt = 0; attempt < MAX_SAMPLE_COUNT; ++attempt) {
            uint64_t txTs; int64_t delay;
            if (QueryNtpServer(server.c_str(), txTs, delay)) {
                allSamples.push_back({txTs, delay, server});
                break;
            }
            if (attempt + 1 < MAX_SAMPLE_COUNT)
                Sleep(200);
        }
    }

    WSACleanup();

    if (allSamples.empty()) {
        WriteLog("所有 NTP 服务器均未能成功获取时间");
        return false;
    }

    auto best = std::min_element(allSamples.begin(), allSamples.end(),
        [](const Sample& a, const Sample& b) { return a.delayUs < b.delayUs; });

    SYSTEMTIME tempSt;
    if (!NtpTimestampToSystemTime(best->txTimestamp, tempSt)) {
        WriteLog("NTP 时间戳转换失败 (" + best->server + ")");
        return false;
    }

    st = tempSt;
    char logBuf[256];
    sprintf(logBuf, "网络时间同步成功 - 服务器: %s, 延迟: %lld us, UTC: %04d-%02d-%02d %02d:%02d:%02d",
            best->server.c_str(), best->delayUs,
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    WriteLog(logBuf);
    return true;
}
