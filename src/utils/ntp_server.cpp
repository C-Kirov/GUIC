// ====== 【修复】必须在所有 include 之前定义 _WIN32_WINNT ======
#define _WIN32_WINNT 0x0602
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdint>

#include "ntp_server.h"
#include "network.h"
#include "../config/config.h"
#include "logger.h"
#include <cstdio>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct NtpServerPacket {
    uint8_t  li_vn_mode;
    uint8_t  stratum;
    int8_t   poll;
    int8_t   precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refId;
    uint64_t refTimestamp;
    uint64_t origTimestamp;
    uint64_t recvTimestamp;
    uint64_t xmitTimestamp;
};
#pragma pack(pop)

static volatile LONG g_ntpServerRunning = 0;
static SOCKET g_ntpSocket = INVALID_SOCKET;
static HANDLE g_ntpThread = NULL;

static uint64_t SystemTimeToNtp64(const SYSTEMTIME& st)
{
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    const uint64_t NTP_EPOCH_OFFSET = 9435484800ULL * 10000000ULL;
    if (ul.QuadPart < NTP_EPOCH_OFFSET) return 0;
    uint64_t diff = ul.QuadPart - NTP_EPOCH_OFFSET;
    uint32_t seconds  = (uint32_t)(diff / 10000000ULL);
    uint32_t fraction = (uint32_t)(((diff % 10000000ULL) << 32) / 10000000ULL);
    return ((uint64_t)seconds << 32) | fraction;
}

static uint64_t FileTimeToNtp64(const FILETIME& ft)
{
    ULARGE_INTEGER ul;
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    const uint64_t NTP_EPOCH_OFFSET = 9435484800ULL * 10000000ULL;
    if (ul.QuadPart < NTP_EPOCH_OFFSET) return 0;
    uint64_t diff = ul.QuadPart - NTP_EPOCH_OFFSET;
    uint32_t seconds  = (uint32_t)(diff / 10000000ULL);
    uint32_t fraction = (uint32_t)(((diff % 10000000ULL) << 32) / 10000000ULL);
    return ((uint64_t)seconds << 32) | fraction;
}

static uint64_t hton64_local(uint64_t x)
{
    union { uint64_t u64; uint32_t u32[2]; } u;
    u.u64 = x;
    uint32_t tmp = htonl(u.u32[0]);
    u.u32[0] = htonl(u.u32[1]);
    u.u32[1] = tmp;
    return u.u64;
}

static DWORD WINAPI NtpServerThread(LPVOID param)
{
    unsigned short port = (unsigned short)(uintptr_t)param;
    char buf[128];
    sprintf(buf, "[NTP Server] 监听端口 %d", port);
    WriteLog(buf);

    // 【修复】防御性确保 Winsock 已初始化（进程级）
    WinsockInit();

    g_ntpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_ntpSocket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        sprintf(buf, "[NTP Server] socket() 失败, 错误码: %d", err);
        WriteLog(buf);
        InterlockedExchange(&g_ntpServerRunning, 0);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(g_ntpSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        sprintf(buf, "[NTP Server] bind() 失败, 错误码: %d (端口 %d 可能被占用或需管理员权限)",
                err, port);
        WriteLog(buf);
        closesocket(g_ntpSocket);
        g_ntpSocket = INVALID_SOCKET;
        InterlockedExchange(&g_ntpServerRunning, 0);
        return 1;
    }

    int timeout = 1000;
    setsockopt(g_ntpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    WriteLog("[NTP Server] 开始监听");

    while (InterlockedCompareExchange(&g_ntpServerRunning, 1, 1) == 1)
    {
        NtpServerPacket request;
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);

        int len = recvfrom(g_ntpSocket, (char*)&request, sizeof(request), 0,
                           (struct sockaddr*)&clientAddr, &clientLen);
        if (len <= 0) continue;
        if (len != sizeof(request)) continue;

        // ====== 【修复】使用 GetSystemTimeAsFileTime 替代 GetSystemTimePreciseAsFileTime ======
        // GetSystemTimePreciseAsFileTime 需要 Win8+ (_WIN32_WINNT >= 0x0602)
        // 对 NTP 服务器而言，毫秒级精度已足够（NTP 本身 ms 级别精度）
        FILETIME ftRecv;
        GetSystemTimeAsFileTime(&ftRecv);

        NtpServerPacket response;
        memset(&response, 0, sizeof(response));

        response.li_vn_mode = (0 << 6) | (4 << 3) | 4;  // Leap=0, Version=4, Mode=4(Server)
        response.stratum    = 2;
        response.precision  = -20;

        uint32_t refId = 0x7F000001;
        memcpy(&response.refId, &refId, 4);

        response.origTimestamp = request.xmitTimestamp;

        uint64_t recvNtp = FileTimeToNtp64(ftRecv);
        response.recvTimestamp = hton64_local(recvNtp);

        FILETIME ftXmit;
        GetSystemTimeAsFileTime(&ftXmit);
        uint64_t xmitNtp = FileTimeToNtp64(ftXmit);
        response.xmitTimestamp = hton64_local(xmitNtp);

        sendto(g_ntpSocket, (char*)&response, sizeof(response), 0,
               (struct sockaddr*)&clientAddr, clientLen);
    }

    closesocket(g_ntpSocket);
    g_ntpSocket = INVALID_SOCKET;
    WriteLog("[NTP Server] 已停止");
    return 0;
}

HANDLE StartNtpServer(unsigned short port)
{
    if (InterlockedCompareExchange(&g_ntpServerRunning, 1, 0) != 0)
        return NULL;

    HANDLE hThread = CreateThread(NULL, 0, NtpServerThread,
                                  (LPVOID)(uintptr_t)port, 0, NULL);
    if (!hThread) {
        InterlockedExchange(&g_ntpServerRunning, 0);
        return NULL;
    }
    g_ntpThread = hThread;
    return hThread;
}

void StopNtpServer()
{
    InterlockedExchange(&g_ntpServerRunning, 0);
    if (g_ntpThread) {
        WaitForSingleObject(g_ntpThread, 3000);
        CloseHandle(g_ntpThread);
        g_ntpThread = NULL;
    }
}

bool IsNtpServerRunning()
{
    return InterlockedCompareExchange(&g_ntpServerRunning, 1, 1) == 1;
}
