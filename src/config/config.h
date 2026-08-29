#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <string>
#include <vector>

enum CountdownMode {
    COUNTDOWN_ONCE,
    COUNTDOWN_DAILY
};

struct CountdownTarget {
    SYSTEMTIME onceTime;
    SYSTEMTIME dailyTime;
};

struct SpecialDay {
    std::string name;
    int month;
    int day;
    int hour;
    int minute;
    bool isAnnual;
    int year;
    bool isSolarTerm;
};

enum class AppLanguage {
    ZH_CN,
    EN_US
};

class AppConfig {
public:
    static AppConfig& GetInstance();

    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    class ScopedLock {
        AppConfig& m_cfg;
    public:
        explicit ScopedLock(AppConfig& cfg) : m_cfg(cfg) {
            EnterCriticalSection(&m_cfg.m_cs);
        }
        ~ScopedLock() {
            LeaveCriticalSection(&m_cfg.m_cs);
        }
    };

    // 显示配置
    std::string message;
    bool showMessage;
    bool showClock;
    bool showCountdown;
    int  fontSize;
    bool autoLayout;
    std::string fontName;
    COLORREF fontColor;
    bool resizable;
    int  windowWidth;
    int  windowHeight;
    std::string dailyRemark;
    CountdownMode   countdownMode;
    CountdownTarget countdownTarget;
    std::vector<SpecialDay> specialDays;

    // 网络时间
    bool      useNetworkTime;
    SYSTEMTIME lastNetworkTime;
    LONGLONG   localTimeOffset;

    // 网络与扩展
    std::string  ntpCustomServer;
    std::string  weatherCity;
    bool         showWeather;
    bool         showLunar;
    AppLanguage  language;
    bool         borderless;

    // 内建服务器
    bool         ntpServerEnabled;
    int          ntpServerPort;
    bool         httpServerEnabled;
    int          httpServerPort;

    // 文件路径
    char configFile[MAX_PATH];
    char specialDaysFile[MAX_PATH];

    // 方法
    void InitPaths();
    void SaveConfig();
    void LoadConfig();
    std::string GetSpecialDay(int year, int month, int day,
                              int hour, int minute, int second);
    void InitSpecialDays();
    void LoadSpecialDaysFromFile();
    void SaveSpecialDaysToFile();
    void CreateDefaultSpecialDaysFile();
    void GetCurrentDateTime(SYSTEMTIME& st);
    void SyncNetworkTime();

private:
    AppConfig();
    ~AppConfig();

    CRITICAL_SECTION m_cs;
    int              m_ntpFailCount;
};

#define g_config AppConfig::GetInstance()

#define g_message           g_config.message
#define g_showMessage       g_config.showMessage
#define g_showClock         g_config.showClock
#define g_showCountdown     g_config.showCountdown
#define g_fontSize          g_config.fontSize
#define g_autoLayout        g_config.autoLayout
#define g_fontName          g_config.fontName
#define g_fontColor         g_config.fontColor
#define g_resizable         g_config.resizable
#define g_windowWidth       g_config.windowWidth
#define g_windowHeight      g_config.windowHeight
#define g_dailyRemark       g_config.dailyRemark
#define g_countdownMode     g_config.countdownMode
#define g_countdownTarget   g_config.countdownTarget
#define g_specialDays       g_config.specialDays
#define g_useNetworkTime    g_config.useNetworkTime
#define g_lastNetworkTime   g_config.lastNetworkTime
#define g_localTimeOffset   g_config.localTimeOffset
#define CONFIG_FILE         g_config.configFile
#define SPECIAL_DAYS_FILE   g_config.specialDaysFile

#endif // CONFIG_H
