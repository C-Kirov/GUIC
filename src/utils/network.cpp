#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "network.h"
#include "logger.h"

#pragma comment(lib, "ws2_32.lib")

// ======================== Winsock 进程级初始化 ========================
// 原问题：GetNetworkTime / FetchWeather 每次调用各自 WSAStartup/WSACleanup，
// 而 HTTP/NTP 服务器线程从不初始化 Winsock；客户端一清理，服务器线程的
// socket() 就报 WSANOTINITIALISED (10093)（见 build/log.txt 历史日志）。
// 修复：进程级引用计数初始化 —— WinMain 启动时初始化、退出时清理；
// 各网络函数只做防御性 WinsockInit()（幂等），不再各自清理。

static CRITICAL_SECTION g_wsaCs;
static INIT_ONCE        g_wsaCsInit = INIT_ONCE_STATIC_INIT;
static volatile LONG    g_wsaRefCount = 0;

static BOOL CALLBACK InitWsaCsOnce(PINIT_ONCE /*once*/, PVOID /*param*/, PVOID* /*ctx*/)
{
    InitializeCriticalSection(&g_wsaCs);
    return TRUE;
}

static void WsaEnter()
{
    InitOnceExecuteOnce(&g_wsaCsInit, InitWsaCsOnce, NULL, NULL);
    EnterCriticalSection(&g_wsaCs);
}

static void WsaLeave()
{
    LeaveCriticalSection(&g_wsaCs);
}

// 幂等：进程内任意时刻调用均安全；返回 false 表示 Winsock 初始化失败
bool WinsockInit()
{
    WsaEnter();
    if (g_wsaRefCount == 0) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            WsaLeave();
            return false;
        }
    }
    g_wsaRefCount++;
    WsaLeave();
    return true;
}

// 进程退出时调用（引用计数归零才真正 WSACleanup）
void WinsockShutdown()
{
    WsaEnter();
    if (g_wsaRefCount > 0 && --g_wsaRefCount == 0)
        WSACleanup();

    WsaLeave();
}

// ======================== 常量 ========================
#define NTP_PORT            123
#define NTP_TIMEOUT_MS      3000
#define MAX_SAMPLE_COUNT    1           // 每服务器重试次数

// ======================== NTP 服务器池 ========================
static const std::vector<const char*> NTP_SERVERS = {
    "ntp.aliyun.com",       // 阿里云 NTP（国内推荐）
    "ntp.tencent.com",      // 腾讯云 NTP（国内推荐）
    "cn.pool.ntp.org",      // 中国 NTP 池
    "pool.ntp.org",         // 全球 NTP 池
    "time.windows.com",     // 微软默认
    "time.google.com"       // Google
};

// ======================== NTP 协议常量 ========================

// 1900-01-01 (NTP epoch) 与 1601-01-01 (FILETIME epoch) 之差，单位秒
static const uint64_t NTP_TO_FILETIME_SECONDS = 9435484800ULL;

// 上述差值对应的 100ns 单位数
static const uint64_t NTP_TO_FILETIME_100NS = NTP_TO_FILETIME_SECONDS * 10000000ULL;

// 最大可接受延迟（100ns 单位）：5 秒
static const int64_t  MAX_DELAY_100NS = 50000000LL;

// 最大可接受偏移（100ns 单位）：3600 秒（1 小时）
static const int64_t  MAX_OFFSET_100NS = 3600LL * 10000000LL;

// ======================== NTP 数据包（RFC 5905, 48 字节） ========================
#pragma pack(push, 1)
struct NTPPacket
{
    uint8_t  li_vn_mode;        // [2bit leap] [3bit version] [3bit mode]
    uint8_t  stratum;           // 层级
    int8_t   poll;              // 轮询间隔（log2 秒）
    int8_t   precision;         // 时钟精度（log2 秒）

    uint32_t rootDelay;         // 根延迟（NTP 短格式，16.16 定点数）
    uint32_t rootDispersion;    // 根色散（NTP 短格式）

    uint32_t refId;             // 参考源标识（stratum 1: ASCII, 其它: IPv4）

    uint64_t refTimestamp;      // 参考时间戳（上次同步时间）
    uint64_t origTimestamp;     // 原始时间戳（客户端发出 = t1）
    uint64_t recvTimestamp;     // 接收时间戳（服务器收到 = t2）
    uint64_t xmitTimestamp;     // 发送时间戳（服务器发出 = t3）
};
#pragma pack(pop)

// ======================== 工具函数：字节序 ========================
static inline uint64_t ntoh64(uint64_t x)
{
    union { uint64_t u64; uint32_t u32[2]; } u;
    u.u64 = x;
    uint32_t tmp = ntohl(u.u32[0]);
    u.u32[0] = ntohl(u.u32[1]);
    u.u32[1] = tmp;
    return u.u64;
}

// ======================== 工具函数：NTP <-> FILETIME 互转 ========================

/**
 * NTP 时间戳 → FILETIME（100ns 单位）
 *
 * NTP 格式: 高 32 位 = 自 1900-01-01 的整数秒数
 *           低 32 位 = 秒的小数部分（1/232 秒精度 ≈ 232 皮秒）
 */
static uint64_t NtpToFileTime100ns(uint64_t ntpTimestamp)
{
    uint32_t seconds  = (uint32_t)(ntpTimestamp >> 32);
    uint32_t fraction = (uint32_t)(ntpTimestamp & 0xFFFFFFFFULL);

    // 整数部分转换
    uint64_t ft = NTP_TO_FILETIME_100NS + (uint64_t)seconds * 10000000ULL;

    // 小数部分转换（fraction * 1e7 / 232）
    // 使用 64 位乘法避免溢出
    ft += ((uint64_t)fraction * 10000000ULL) >> 32;

    return ft;
}

/**
 * FILETIME（100ns 单位） → NTP 时间戳
 */
static uint64_t FileTime100nsToNtp(uint64_t ft100ns)
{
    if (ft100ns < NTP_TO_FILETIME_100NS)
        return 0;

    uint64_t diff = ft100ns - NTP_TO_FILETIME_100NS;
    uint32_t seconds  = (uint32_t)(diff / 10000000ULL);
    uint32_t fraction = (uint32_t)(((diff % 10000000ULL) << 32) / 10000000ULL);

    return ((uint64_t)seconds << 32) | fraction;
}

/**
 * FILETIME 结构 → NTP 时间戳
 */
static uint64_t FileTimeToNtp(const FILETIME& ft)
{
    ULARGE_INTEGER ul;
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return FileTime100nsToNtp(ul.QuadPart);
}

/**
 * NTP 时间戳 → SYSTEMTIME（UTC）
 */
static bool NtpToSystemTime(uint64_t ntpTimestamp, SYSTEMTIME& st)
{
    uint64_t ft100ns = NtpToFileTime100ns(ntpTimestamp);

    FILETIME ft;
    ft.dwLowDateTime  = (DWORD)(ft100ns & 0xFFFFFFFFULL);
    ft.dwHighDateTime = (DWORD)(ft100ns >> 32);

    if (!FileTimeToSystemTime(&ft, &st))
        return false;

    // 年代合理性检查
    if (st.wYear < 2020 || st.wYear > 2036) return false;
    if (st.wMonth < 1 || st.wMonth > 12)     return false;
    if (st.wDay < 1 || st.wDay > 31)         return false;
    if (st.wHour > 23 || st.wMinute > 59 || st.wSecond > 59) return false;

    return true;
}

// ======================== DNS 解析 ========================
static bool ResolveNtpServer(const char* hostname, struct sockaddr_in& addr)
{
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(hostname, "123", &hints, &result) != 0)
    {
        char buf[256];
        sprintf(buf, "[NTP] DNS 解析失败: %s", hostname);
        WriteLog(buf);
        return false;
    }
    if (!result) return false;

    addr = *(struct sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);
    return true;
}

// ======================== 单服务器 SNTPv4 完整查询 ========================

/**
 * 对单台 NTP 服务器进行完整 SNTPv4 查询
 *
 * 四时间戳模型:
 *   t1 — 客户端发送请求（本地时钟）
 *   t2 — 服务器收到请求（服务器时钟）
 *   t3 — 服务器发送响应（服务器时钟）
 *   t4 — 客户端收到响应（本地时钟）
 *
 * 偏移: θ = ((t2 - t1) + (t3 - t4)) / 2
 * 延迟: δ = (t4 - t1) - (t3 - t2)
 */
bool QuerySingleNtpServer(const char* server, NTPQueryResult& result)
{
    result = {};   // 清零

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        char buf[128];
        sprintf(buf, "[NTP] socket() 失败, 错误码: %d [%s]", err, server);
        WriteLog(buf);
        return false;
    }

    // 设置接收超时
    int timeout = NTP_TIMEOUT_MS;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    // DNS 解析
    struct sockaddr_in serverAddr;
    if (!ResolveNtpServer(server, serverAddr))
    {
        closesocket(sock);
        return false;
    }
    serverAddr.sin_port = htons(NTP_PORT);

    // ---- 构建 NTPv4 请求 ----
    NTPPacket request = {};
    request.li_vn_mode = (0 << 6) | (4 << 3) | 3;   // Leap=0, Version=4, Mode=3(Client)

    // ---- 记录 t1（发送前的高精度时刻）----
    FILETIME ftT1;
    GetSystemTimePreciseAsFileTime(&ftT1);

    // ---- 发送请求 ----
    int sentBytes = sendto(sock, (char*)&request, sizeof(request), 0,
                           (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes != sizeof(request))
    {
        int err = WSAGetLastError();
        char buf[128];
        sprintf(buf, "[NTP] sendto() 失败, 错误码: %d [%s]", err, server);
        WriteLog(buf);
        closesocket(sock);
        return false;
    }

    // ---- 接收响应 + 记录 t4 ----
    NTPPacket response;
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    int recvLen = recvfrom(sock, (char*)&response, sizeof(response), 0,
                           (struct sockaddr*)&fromAddr, &fromLen);

    FILETIME ftT4;
    GetSystemTimePreciseAsFileTime(&ftT4);

    closesocket(sock);

    // ---- 校验响应 ----
    if (recvLen <= 0)
    {
        int err = WSAGetLastError();
        if (err == 0) err = WSAETIMEDOUT;
        char buf[128];
        sprintf(buf, "[NTP] recvfrom() 失败, 错误码: %d (recvLen=%d) [%s]",
                err, recvLen, server);
        WriteLog(buf);
        return false;
    }
    if (recvLen != sizeof(response))
    {
        char buf[128];
        sprintf(buf, "[NTP] 响应长度异常 (期望 %d, 实际 %d) [%s]",
                (int)sizeof(response), recvLen, server);
        WriteLog(buf);
        return false;
    }

    // ---- 提取响应字段 ----
    uint8_t  li_vn_mode = response.li_vn_mode;
    uint8_t  stratum    = response.stratum;
    uint8_t  leap       = (li_vn_mode >> 6) & 0x03;

    uint64_t rx_orig = ntoh64(response.origTimestamp);   // 客户端发出的 t1（服务器回显）
    uint64_t rx_recv = ntoh64(response.recvTimestamp);   // 服务器收到 = t2
    uint64_t rx_xmit = ntoh64(response.xmitTimestamp);   // 服务器发出 = t3

    // ---- 基本有效性检查 ----
    if (stratum == 0 || stratum >= 16)
    {
        char buf[128];
        sprintf(buf, "[NTP] 服务器层级无效 (stratum=%d) [%s]", stratum, server);
        WriteLog(buf);
        return false;
    }

    if (leap == 3)
    {
        char buf[128];
        sprintf(buf, "[NTP] 服务器未同步 (leap=3) [%s]", server);
        WriteLog(buf);
        return false;
    }

    if (rx_xmit == 0)
    {
        char buf[128];
        sprintf(buf, "[NTP] 服务器发送时间戳为零 [%s]", server);
        WriteLog(buf);
        return false;
    }

    // ---- 转换为 NTP 时间戳 ----
    uint64_t t1_ntp = FileTimeToNtp(ftT1);
    uint64_t t4_ntp = FileTimeToNtp(ftT4);

    // 优先使用服务器返回的 t2 (recvTimestamp), t3 (xmitTimestamp)
    // 某些服务器可能不填 t2，此时退化为仅用 t3
    uint64_t t2_ntp = rx_recv;
    uint64_t t3_ntp = rx_xmit;

    int64_t offsetNtp;   // NTP 格式的偏移（含小数部分）
    int64_t delayNtp;    // NTP 格式的延迟

    if (t2_ntp != 0 && t2_ntp >= t1_ntp)
    {
        // 完整四时间戳计算（RFC 5905）
        // θ = ((t2 - t1) + (t3 - t4)) / 2
        // δ = (t4 - t1) - (t3 - t2)
        int64_t t21 = (int64_t)(t2_ntp - t1_ntp);
        int64_t t34 = (int64_t)(t3_ntp - t4_ntp);
        int64_t t41 = (int64_t)(t4_ntp - t1_ntp);
        int64_t t32 = (int64_t)(t3_ntp - t2_ntp);

        offsetNtp = (t21 + t34) / 2;
        delayNtp  = t41 - t32;
    }
    else
    {
        // 退化：t2 不可用，使用 t3 和 (t1+t4)/2
        // θ = t3 - (t1 + t4) / 2
        int64_t t41 = (int64_t)(t4_ntp - t1_ntp);
        offsetNtp = (int64_t)(t3_ntp - t1_ntp) - t41 / 2;
        delayNtp  = t41;
    }

    // ---- 转换为 100ns 单位 ----
    // NTP 时间戳: 高 32 位 = 秒, 低 32 位 = 1/232 秒
    // 100ns 单位 = 10?? 秒
    auto NtpDiffTo100ns = [](int64_t ntpDiff) -> int64_t {
        bool neg = (ntpDiff < 0);
        uint64_t absVal = (uint64_t)(neg ? -ntpDiff : ntpDiff);

        uint64_t seconds  = absVal >> 32;
        uint64_t fraction = absVal & 0xFFFFFFFFULL;

        uint64_t result = seconds * 10000000ULL;
        result += (fraction * 10000000ULL) >> 32;

        return neg ? -(int64_t)result : (int64_t)result;
    };

    int64_t offset100ns = NtpDiffTo100ns(offsetNtp);
    int64_t delay100ns  = NtpDiffTo100ns(delayNtp);

    // ---- 合理性过滤 ----
    if (delay100ns < 0)
    {
        // 延迟不可能为负（可能是 t2 回显异常），取绝对值做保守估计
        delay100ns = -delay100ns;
    }

    if (delay100ns > MAX_DELAY_100NS)
    {
        char buf[128];
        sprintf(buf, "[NTP] 延迟过大 (%lld ms), 丢弃 [%s]",
                delay100ns / 10000, server);
        WriteLog(buf);
        return false;
    }

    if (offset100ns > MAX_OFFSET_100NS || offset100ns < -MAX_OFFSET_100NS)
    {
        char buf[128];
        sprintf(buf, "[NTP] 偏移异常 (%lld ms), 丢弃 [%s]",
                offset100ns / 10000, server);
        WriteLog(buf);
        return false;
    }

    // ---- 填充结果 ----
    result.offset100ns  = offset100ns;
    result.delay100ns   = delay100ns;
    result.serverTxNtp  = t3_ntp;
    result.stratum      = stratum;
    result.leapIndicator = leap;
    result.valid        = true;

    char buf[256];
    sprintf(buf,
            "[NTP] 查询成功 [%s] "
            "stratum=%d offset=%.3fms delay=%.3fms",
            server,
            stratum,
            offset100ns / 10000.0,
            delay100ns / 10000.0);
    WriteLog(buf);

    return true;
}

// ======================== Marzullo 算法 ========================

/**
 * Marzullo 算法：从多个时间区间中找出最多服务器共识的真实时间
 *
 * 输入: 一组 interval = [offset - delay/2, offset + delay/2]
 * 输出: 共识偏移量 consensusOffset
 *
 * 返回被最多区间覆盖的区间的中点。
 * 要求共识数 >= 总区间数的一半（多数原则）。
 */
static bool MarzulloSelect(
    const std::vector<NTPQueryResult>& samples,
    int64_t& consensusOffset,
    int64_t& consensusDelay,
    int&     agreeingServers)
{
    if (samples.empty()) return false;

    // ---- 1. 为每个样本构造可信区间 ----
    struct Interval {
        int64_t lo, hi;       // [offset - delay/2, offset + delay/2]
    };
    std::vector<Interval> intervals;
    intervals.reserve(samples.size());

    for (const auto& s : samples)
    {
        if (!s.valid) continue;
        int64_t halfDelay = s.delay100ns / 2;
        intervals.push_back({ s.offset100ns - halfDelay,
                              s.offset100ns + halfDelay });
    }

    if (intervals.empty()) return false;
    size_t n = intervals.size();

    // ---- 2. 收集所有端点并排序 ----
    struct Endpoint {
        int64_t val;
        int     type;     // +1 = 区间起点, -1 = 区间终点
    };
    std::vector<Endpoint> pts;
    pts.reserve(n * 2);
    for (const auto& iv : intervals)
    {
        pts.push_back({ iv.lo, +1 });
        pts.push_back({ iv.hi, -1 });
    }

    std::sort(pts.begin(), pts.end(),
        [](const Endpoint& a, const Endpoint& b) {
            if (a.val != b.val)
                return a.val < b.val;
            // 同值：起点 (+1) 先于终点 (-1)，保证相邻区间正确重叠
            return a.type > b.type;
        });

    // ---- 3. 扫描找出最大覆盖数区间 ----
    int     cnt       = 0;
    int     bestCnt   = 0;
    int64_t bestLo    = 0;
    int64_t bestHi    = 0;
    int64_t bestWidth = INT64_MAX;

    for (size_t i = 0; i < pts.size(); i++)
    {
        cnt += pts[i].type;   // 进入区间 +1, 离开区间 -1

        if (cnt > bestCnt)
        {
            // 发现新的最大覆盖数
            bestCnt = cnt;
            bestLo  = pts[i].val;

            // 向前搜索：找到覆盖数首次回落到 bestCnt 以下的终点
            int tempCnt = cnt;
            for (size_t j = i + 1; j < pts.size(); j++)
            {
                tempCnt += pts[j].type;
                if (tempCnt < bestCnt)
                {
                    bestHi    = pts[j].val;
                    bestWidth = bestHi - bestLo;
                    break;
                }
                if (j == pts.size() - 1)
                {
                    // 直到末尾仍未回落，使用最后一个端点
                    bestHi    = pts[j].val;
                    bestWidth = bestHi - bestLo;
                }
            }
        }
        else if (cnt == bestCnt)
        {
            // 与当前最佳覆盖数相同的其他区间（可能是不连通的）
            // 检查这个新区间是否比已记录的最佳区间更窄
            int64_t candidateLo = pts[i].val;
            int tempCnt = cnt;
            for (size_t j = i + 1; j < pts.size(); j++)
            {
                tempCnt += pts[j].type;
                if (tempCnt < bestCnt)
                {
                    int64_t candidateHi = pts[j].val;
                    int64_t candidateWidth = candidateHi - candidateLo;
                    if (candidateWidth < bestWidth)
                    {
                        bestLo    = candidateLo;
                        bestHi    = candidateHi;
                        bestWidth = candidateWidth;
                    }
                    break;
                }
                if (j == pts.size() - 1)
                {
                    int64_t candidateHi = pts[j].val;
                    int64_t candidateWidth = candidateHi - candidateLo;
                    if (candidateWidth < bestWidth)
                    {
                        bestLo    = candidateLo;
                        bestHi    = candidateHi;
                        bestWidth = candidateWidth;
                    }
                }
            }
        }
    }

    // ---- 4. 多数原则检查 ----
    size_t majority = (n + 1) / 2;   // 超过半数
    if (bestCnt < (int)majority)
    {
        // 达不到多数共识，退化为中位数法
        WriteLog("[NTP Marzullo] 未达多数共识，改用中位数法");

        std::vector<int64_t> offsets;
        for (const auto& s : samples)
            if (s.valid) offsets.push_back(s.offset100ns);

        if (offsets.empty()) return false;

        std::sort(offsets.begin(), offsets.end());
        size_t mid = offsets.size() / 2;
        consensusOffset = offsets[mid];
        consensusDelay  = 0;   // 无法可靠估计

        // 取中位数附近的样本估算延迟
        int64_t delaySum = 0;
        int count = 0;
        for (const auto& s : samples)
        {
            if (s.valid)
            {
                delaySum += s.delay100ns;
                count++;
            }
        }
        if (count > 0) consensusDelay = delaySum / count;

        agreeingServers = (int)offsets.size();
    }
    else
    {
        // Marzullo 成功
        consensusOffset = (bestLo + bestHi) / 2;
        consensusDelay  = bestWidth;   // 共识区间的宽度视为不确定性

        agreeingServers = bestCnt;

        char buf[200];
        sprintf(buf,
                "[NTP Marzullo] 共识: %d/%d 服务器一致, "
                "offset=%.3fms, uncertainty=%.3fms",
                bestCnt, (int)n,
                consensusOffset / 10000.0,
                bestWidth / 10000.0);
        WriteLog(buf);
    }

    return true;
}

// ======================== 公开接口：多服务器获取网络时间 ========================

bool GetNetworkTime(SYSTEMTIME& st,
                    int64_t* pBestOffset100ns,
                    int64_t* pBestDelay100ns,
                    const char* customServer)
{
    // ---- 1. 确保 Winsock 已初始化（进程级，幂等；不再各自 WSACleanup） ----
    if (!WinsockInit())
    {
        WriteLog("[NTP] Winsock 初始化失败");
        return false;
    }

    // ---- 2. 查询服务器 ----
    std::vector<NTPQueryResult> allResults;

    //   自定义服务器优先
    if (customServer && strlen(customServer) > 0)
    {
        char logBuf[256];
        sprintf(logBuf, "[NTP] 使用自定义服务器: %s", customServer);
        WriteLog(logBuf);

        for (int attempt = 0; attempt < 3; ++attempt)
        {
            NTPQueryResult r;
            if (QuerySingleNtpServer(customServer, r) && r.valid)
            {
                allResults.push_back(r);
                WriteLog("[NTP] 自定义服务器查询成功");
                break;
            }
            if (attempt + 1 < 3)
                Sleep(200);
        }

        if (allResults.empty())
        {
            char logBuf[256];
            sprintf(logBuf, "[NTP] 自定义服务器 '%s' 全部尝试失败, 回退到内置服务器池", customServer);
            WriteLog(logBuf);
        }
    }

    // 如果自定义服务器失败或未设置，使用内置服务器池
    if (allResults.empty())
    {
        for (const char* server : NTP_SERVERS)
        {
            for (int attempt = 0; attempt < MAX_SAMPLE_COUNT; ++attempt)
            {
                NTPQueryResult r;
                if (QuerySingleNtpServer(server, r) && r.valid)
                {
                    allResults.push_back(r);
                    break;   // 该服务器成功，不再重试
                }
                if (attempt + 1 < MAX_SAMPLE_COUNT)
                    Sleep(200);
            }
        }
    }



    if (allResults.empty())
    {
        WriteLog("[NTP] 所有服务器查询均失败");
        return false;
    }

    // ---- 3. Marzullo 算法选出共识偏移 ----
    int64_t bestOffset = 0;
    int64_t bestDelay  = 0;
    int     agreeCount = 0;

    if (!MarzulloSelect(allResults, bestOffset, bestDelay, agreeCount))
    {
        WriteLog("[NTP] Marzullo 算法无法选出有效偏移");
        return false;
    }

    // ---- 4. 基于共识偏移量计算当前 UTC 时间 ----
    // 取当前高精度本地时间，加上共识偏移得到 UTC
    FILETIME ftLocal;
    GetSystemTimePreciseAsFileTime(&ftLocal);

    ULARGE_INTEGER ulLocal;
    ulLocal.LowPart  = ftLocal.dwLowDateTime;
    ulLocal.HighPart = ftLocal.dwHighDateTime;

    // 加上偏移（100ns 单位）
    ULARGE_INTEGER ulUtc;
    ulUtc.QuadPart = (ULONGLONG)((LONGLONG)ulLocal.QuadPart + bestOffset);

    FILETIME ftUtc;
    ftUtc.dwLowDateTime  = ulUtc.LowPart;
    ftUtc.dwHighDateTime = ulUtc.HighPart;

    if (!FileTimeToSystemTime(&ftUtc, &st))
    {
        WriteLog("[NTP] FILETIME→SYSTEMTIME 转换失败");
        return false;
    }

    // 年代合理性检查
    if (st.wYear < 2020 || st.wYear > 2036)
    {
        char buf[128];
        sprintf(buf, "[NTP] 计算出的年份异常 (%d), 丢弃结果", st.wYear);
        WriteLog(buf);
        return false;
    }

    // ---- 5. 输出参数 ----
    if (pBestOffset100ns) *pBestOffset100ns = bestOffset;
    if (pBestDelay100ns)  *pBestDelay100ns  = bestDelay;

    char buf[256];
    sprintf(buf,
            "[NTP] 最终结果: UTC=%04d-%02d-%02d %02d:%02d:%02d, "
            "offset=%.3fms, delay=%.3fms, servers=%d/%d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond,
            bestOffset / 10000.0,
            bestDelay / 10000.0,
            agreeCount, (int)allResults.size());
    WriteLog(buf);

    return true;
}
