#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <string>
#include <vector>

enum CountdownMode {
    COUNTDOWN_ONCE,
    COUNTDOWN_DAILY
};

union CountdownTarget {
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

// 单例配置管理类（线程不安全，适用于单线程）
class AppConfig {
public:
    static AppConfig& GetInstance();

    // 禁止拷贝
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // 数据成员（原全局变量）
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
    CountdownMode countdownMode;
    CountdownTarget countdownTarget;
    std::vector<SpecialDay> specialDays;

    bool useNetworkTime;
    SYSTEMTIME lastNetworkTime;
    LONGLONG localTimeOffset;   // 100ns 单位

    // 文件路径
    char configFile[MAX_PATH];
    char specialDaysFile[MAX_PATH];

    // 方法（原全局函数）
    void InitPaths();
    void SaveConfig();
    void LoadConfig();
    std::string GetSpecialDay(int year, int month, int day, int hour, int minute, int second);
    void InitSpecialDays();
    void LoadSpecialDaysFromFile();
    void SaveSpecialDaysToFile();
    void CreateDefaultSpecialDaysFile();
    void GetCurrentDateTime(SYSTEMTIME& st);
    void SyncNetworkTime();

private:
    AppConfig();
    ~AppConfig() = default;
};

// 便捷访问宏
#define g_config AppConfig::GetInstance()

// 变量映射宏（保留兼容旧代码）
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

// 注意：不再提供函数映射宏，避免与成员函数定义冲突

#endif // CONFIG_H
