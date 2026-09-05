#include "lunar_online.h"
#include "logger.h"
#include <winhttp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#pragma comment(lib, "winhttp.lib")

// ================================================================
// 说明
// ================================================================
// 在线农历来源：香港天文台（HKO）公历与农历日期对照表（政府官方发布，免密钥、长期稳定）。
// 按年下载文本文件，例如 2026 年：
//   https://www.hko.gov.hk/en/gts/time/calendar/text/files/T2026e.txt
// 文件格式（每行）：
//   2023/1/22         1st Lunar Month         Sunday
//   2023/1/23         2
//     ...
//   “Nth Lunar Month” 表示该日为第 N 个月初一；纯数字表示当日是该月第几天。
//   闰月表示为同一月号重复出现（例如 2023 年 3/22 再次出现 “2nd Lunar Month” = 闰二月初一）。
//   月号下降（12 月 → 1 月）表示进入新的农历年。
// ================================================================

// ================================================================
// HTTPS GET（WinHTTP，系统 DLL，支持 TLS）
// ================================================================
static bool HttpGet(const wchar_t* host, const wchar_t* path, std::string& body)
{
    HINTERNET hSession = WinHttpOpen(L"GUIC_2.0/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        WriteLog("[Lunar] WinHttpOpen 失败");
        return false;
    }

    DWORD timeout = 10000;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WriteLog("[Lunar] WinHttpConnect 失败");
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WriteLog("[Lunar] WinHttpOpenRequest 失败");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bool ok = (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                  WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE);
    if (ok) ok = (WinHttpReceiveResponse(hRequest, NULL) != FALSE);

    body.clear();
    if (ok) {
        DWORD avail = 0;
        do {
            avail = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &avail)) { ok = false; break; }
            if (avail == 0) break;
            std::string chunk;
            chunk.resize(avail);
            DWORD read = 0;
            if (!WinHttpReadData(hRequest, &chunk[0], avail, &read)) { ok = false; break; }
            body.append(chunk, 0, read);
        } while (avail > 0);
    }

    if (!ok) WriteLog("[Lunar] WinHttp 请求/接收失败");
    if (ok && body.empty()) { ok = false; WriteLog("[Lunar] 响应体为空"); }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

// ================================================================
// 工具：抽取行中农历标记
// ================================================================

// 解析 “1st”..“12th” 月头序号；纯数字（日）返回 false
static bool ParseOrdinal(const std::string& token, int& outMonth)
{
    size_t i = 0;
    while (i < token.size() && isdigit((unsigned char)token[i])) i++;
    if (i == 0 || i >= token.size()) return false;

    std::string suffix = token.substr(i);
    if (suffix != "st" && suffix != "nd" && suffix != "rd" && suffix != "th")
        return false;

    outMonth = atoi(token.substr(0, i).c_str());
    return outMonth >= 1 && outMonth <= 12;
}

// 解析一行：日期 "Y/M/D" 与后续农历标记（数字日 或 “Nth Lunar Month”）
struct HkoLine {
    bool     ok;
    int      y, m, d;
    int      monthHeader;   // >0 表示该行是第 monthHeader 个月初一
    int      day;           // >0 且 monthHeader==0 表示该行是当月第 day 天
};

static HkoLine ParseHkoLine(const std::string& line)
{
    HkoLine out = { false, 0, 0, 0, 0, 0 };

    // 跳过空白，读取第一个日期 token "Y/M/D"
    size_t p = 0;
    while (p < line.size() && (line[p] == ' ' || line[p] == '\t')) p++;
    size_t start = p;
    while (p < line.size() && line[p] != ' ' && line[p] != '\t') p++;
    std::string dateTok = line.substr(start, p - start);
    if (dateTok.empty()) return out;

    int y = 0, m = 0, d = 0;
    size_t s1 = dateTok.find('/');
    if (s1 == std::string::npos) return out;
    size_t s2 = dateTok.find('/', s1 + 1);
    if (s2 == std::string::npos) return out;
    y = atoi(dateTok.substr(0, s1).c_str());
    m = atoi(dateTok.substr(s1 + 1, s2 - s1 - 1).c_str());
    d = atoi(dateTok.substr(s2 + 1).c_str());
    if (y < 1900 || m < 1 || m > 12 || d < 1 || d > 31) return out;

    // 读取下一个 token（农历标记）
    while (p < line.size() && line[p] == ' ') p++;
    start = p;
    while (p < line.size() && line[p] != ' ' && line[p] != '\t') p++;
    std::string tok = line.substr(start, p - start);

    int hMonth = 0;
    if (ParseOrdinal(tok, hMonth)) {
        out.ok = true; out.y = y; out.m = m; out.d = d;
        out.monthHeader = hMonth;
        return out;
    }

    if (!tok.empty() && isdigit((unsigned char)tok[0])) {
        int day = atoi(tok.c_str());
        if (day < 1 || day > 30) return out;
        out.ok = true; out.y = y; out.m = m; out.d = d;
        out.day = day;
        return out;
    }

    return out;
}

// ================================================================
// 解析 HKO 年度文本文件
// ================================================================
bool ParseHkoCalendarText(int textYear, int queryYear, int queryMonth, int queryDay,
                          const std::string& text, LunarOnlineData& out)
{
    out.valid = false;
    out.dataYear = -1;
    out.lunarYear = 0;
    out.lunarMonth = 0;
    out.lunarDay = 0;
    out.isLeapMonth = false;

    // ---- 1. 校验文件头年份 ----
    // 首行形如: "Gregorian-Lunar Calendar Conversion Table of 2026 (Ding-wu? - year of the Horse)"
    const std::string marker = "Conversion Table of ";
    size_t hPos = text.find(marker);
    if (hPos == std::string::npos) return false;
    int headYear = atoi(text.c_str() + hPos + marker.size());
    if (headYear != textYear) return false;

    // ---- 2. 预扫描：找到文件内第一个月头标记（用于确定年初所在农历月）----
    // 每年 1 月初的月份属于上一农历年（十一月或十二月）。
    int firstHeaderN = 0;
    {
        size_t pos = 0;
        while (true) {
            size_t eol = text.find('\n', pos);
            if (eol == std::string::npos) eol = text.size();
            std::string line = text.substr(pos, eol - pos);
            pos = eol + 1;
            if (pos > text.size()) break;

            HkoLine hl = ParseHkoLine(line);
            if (!hl.ok) continue;
            if (hl.monthHeader > 0) { firstHeaderN = hl.monthHeader; break; }
        }
    }
    if (firstHeaderN == 0) return false;

    // ---- 3. 顺序扫描，状态机 ----
    int  curYear   = queryYear - 1;                 // 年初尚在上一农历年
    int  curMonth  = (firstHeaderN == 1) ? 12 : firstHeaderN - 1;
    bool curLeap   = false;

    int queryKey = queryYear * 10000 + queryMonth * 100 + queryDay;
    bool found = false;
    LunarOnlineData hit;
    hit.valid = true;

    size_t pos = 0;
    while (!found) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        std::string line = text.substr(pos, eol - pos);
        pos = eol + 1;
        if (pos > text.size() + 1) break;

        HkoLine hl = ParseHkoLine(line);
        if (!hl.ok) continue;
        if (hl.y != queryYear) continue;

        if (hl.monthHeader > 0) {
            int n = hl.monthHeader;
            if (n == 1 && curMonth == 12) {
                curYear++; curMonth = 1; curLeap = false;    // 新年
            } else if (n == curMonth) {
                curLeap = true;                              // 闰月（月号重复）
            } else if (n == curMonth + 1) {
                curMonth = n; curLeap = false;               // 普通月份
            } else {
                return false;                                // 数据异常
            }
            if (hl.y * 10000 + hl.m * 100 + hl.d == queryKey) {
                hit.dataYear = textYear;
                hit.lunarYear = curYear;
                hit.lunarMonth = curMonth;
                hit.lunarDay = 1;
                hit.isLeapMonth = curLeap;
                found = true;
            }
        } else {
            if (hl.y * 10000 + hl.m * 100 + hl.d == queryKey) {
                hit.dataYear = textYear;
                hit.lunarYear = curYear;
                hit.lunarMonth = curMonth;
                hit.lunarDay = hl.day;
                hit.isLeapMonth = curLeap;
                found = true;
            }
        }
    }

    if (!found) return false;

    memset(&hit.fetchTime, 0, sizeof(hit.fetchTime));
    out = hit;
    return true;
}

// ================================================================
// 在线获取（HTTPS 下载当年对照表并解析当天）
// ================================================================
bool FetchOnlineLunar(const SYSTEMTIME& st, LunarOnlineData& out)
{
    out.valid = false;
    out.dataYear = -1;
    out.lunarYear = 0;
    out.lunarMonth = 0;
    out.lunarDay = 0;
    out.isLeapMonth = false;
    memset(&out.fetchTime, 0, sizeof(out.fetchTime));

    char pathBuf[96];
    sprintf(pathBuf, "/en/gts/time/calendar/text/files/T%04de.txt", st.wYear);

    wchar_t wpath[128];
    MultiByteToWideChar(CP_ACP, 0, pathBuf, -1, wpath, 128);

    std::string body;
    if (!HttpGet(L"www.hko.gov.hk", wpath, body))
        return false;

    if (!ParseHkoCalendarText(st.wYear, st.wYear, st.wMonth, st.wDay, body, out))
        return false;

    out.fetchTime = st;

    char buf[160];
    sprintf(buf, "[Lunar] 在线获取成功 %04d-%02d-%02d = 农历%d年%s%d月%d日",
            st.wYear, st.wMonth, st.wDay,
            out.lunarYear,
            out.isLeapMonth ? "闰" : "",
            out.lunarMonth,
            out.lunarDay);
    WriteLog(buf);

    return true;
}
