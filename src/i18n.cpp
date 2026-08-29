#include "i18n.h"

static const char* g_stringsZH[] = {
    "图形化时钟",                          // STR_WINDOW_TITLE
    "设置显示消息",                        // STR_SET_MESSAGE
    "设置一次性倒计时",                    // STR_SET_COUNTDOWN_ONCE
    "设置每日倒计时",                      // STR_SET_COUNTDOWN_DAILY
    "消息",                                // STR_SHOW_MESSAGE
    "时钟",                                // STR_SHOW_CLOCK
    "倒计时",                              // STR_SHOW_COUNTDOWN
    "自动布局",                            // STR_SHOW_LAYOUT
    "农历",                                // STR_SHOW_LUNAR
    "天气",                                // STR_SHOW_WEATHER
    "无边框",                              // STR_SHOW_BORDERLESS
    "窗口设置",                            // STR_WINDOW_SETTINGS
    "字体设置",                            // STR_FONT_SETTINGS
    "颜色设置",                            // STR_COLOR_SETTINGS
    "天气城市设置",                        // STR_WEATHER_CITY_SETTINGS
    "NTP服务器设置",                       // STR_NTP_SERVER_SETTINGS
    "添加特殊日期",                        // STR_ADD_SPECIAL_DAY
    "世界时钟",                            // STR_WORLD_CLOCK
    "模拟时钟",                            // STR_ANALOG_CLOCK
    "菜单",                                // STR_MENU
    "语言",                                // STR_LANGUAGE
    "简体中文",                            // STR_LANGUAGE_ZH
    "English",                             // STR_LANGUAGE_EN
    "启动NTP服务器",                       // STR_NTP_SERVER_START
    "停止NTP服务器",                       // STR_NTP_SERVER_STOP
    "启动Web控制面板",                     // STR_HTTP_SERVER_START
    "停止Web控制面板",                     // STR_HTTP_SERVER_STOP
    "关于",                                // STR_ABOUT
    "欢迎使用图形化时钟！",                // STR_WELCOME_MESSAGE
    "每日倒计时备注",                      // STR_DAILY_REMARK
    "倒计时已到!",                         // STR_COUNTDOWN_ENDED
    "倒计时未设置",                        // STR_COUNTDOWN_NOT_SET
    "倒计时 %lld天 %lld小时%lld分%lld秒",  // STR_COUNTDOWN_FMT
    "天气加载中...",                       // STR_WEATHER_LOADING
    "天气获取失败",                        // STR_WEATHER_FAILED
    "湿度:",                               // STR_HUMIDITY
    "风:",                                 // STR_WIND
    "农历",                                // STR_LUNAR
    "确定",                                // STR_OK
    "取消",                                // STR_CANCEL
    "错误",                                // STR_ERROR
    "提示",                                // STR_PROMPT
    "确认",                                // STR_CONFIRM
    "提示",                                // STR_TIP
    "无边框模式将在下次启动时生效",        // STR_BORDERLESS_NOTE
    "语言已切换，重启后完全生效",          // STR_LANGUAGE_SWITCHED
    "NTP服务器已启动，端口 %d",            // STR_NTP_SERVER_STARTED_FMT
    "NTP服务器已停止",                     // STR_NTP_SERVER_STOPPED
    "Web控制面板已启动\nhttp://localhost:%d", // STR_HTTP_SERVER_STARTED_FMT
    "Web控制面板已停止",                   // STR_HTTP_SERVER_STOPPED
    "请输入显示消息：",                    // STR_MSG_INPUT_LABEL
    "请输入目标时间(YYYY-MM-DD HH:MM:SS)：", // STR_COUNTDOWN_ONCE_INPUT_LABEL
    "请输入有效的时间格式！\n格式：YYYY-MM-DD HH:MM:SS", // STR_COUNTDOWN_ONCE_INVALID
    "一次性倒计时设置成功！",              // STR_COUNTDOWN_ONCE_SUCCESS
    "请输入每日时间(HH:MM:SS):",           // STR_COUNTDOWN_DAILY_INPUT_LABEL
    "备注:",                               // STR_COUNTDOWN_REMARK_LABEL
    "请输入有效的时间格式！\n格式：HH:MM:SS", // STR_COUNTDOWN_DAILY_INVALID
    "每日倒计时设置成功！",                // STR_COUNTDOWN_DAILY_SUCCESS
    "允许调整窗口大小",                    // STR_WINDOW_RESIZABLE
    "窗口大小:",                           // STR_WINDOW_SIZE_LABEL
    "宽度:",                               // STR_WINDOW_WIDTH_LABEL
    "高度:",                               // STR_WINDOW_HEIGHT_LABEL
    "范围: 宽度300-2000, 高度200-1500",    // STR_WINDOW_RANGE_LABEL
    "请输入有效的窗口大小。\n宽度: 300-2000, 高度: 200-1500", // STR_WINDOW_SIZE_INVALID
    "字体大小:",                           // STR_FONT_SIZE_LABEL
    "字体名称:",                           // STR_FONT_NAME_LABEL
    "请选择字体颜色:",                     // STR_COLOR_LABEL
    "选择颜色",                            // STR_COLOR_BUTTON
    "名称:",                               // STR_SPECIAL_NAME_LABEL
    "月份:",                               // STR_SPECIAL_MONTH_LABEL
    "日期:",                               // STR_SPECIAL_DAY_LABEL
    "年份:",                               // STR_SPECIAL_YEAR_LABEL
    "时间:",                               // STR_SPECIAL_TIME_LABEL
    "每年重复",                            // STR_SPECIAL_ANNUAL_CHECK
    "节气",                                // STR_SPECIAL_SOLAR_TERM_CHECK
    "请输入名称！",                        // STR_SPECIAL_NAME_REQUIRED
    "月份/日期无效",                       // STR_SPECIAL_MONTH_DAY_INVALID
    "年份无效",                            // STR_SPECIAL_YEAR_INVALID
    "添加成功",                            // STR_SPECIAL_ADD_SUCCESS
    "关闭",                                // STR_CLOSE
    "城市名 (如 Beijing, Shanghai, Tokyo):", // STR_WEATHER_CITY_LABEL
    "自定义NTP服务器 (留空使用默认):",     // STR_NTP_SERVER_LABEL
    "关于 GUIC 2.0",                       // STR_ABOUT_TITLE
    "GUIC 2.0 - 图形化时钟",               // STR_ABOUT_DESC
    "版本 %d.%d.%d.%d",                   // STR_ABOUT_VERSION
    "功能: 时钟/倒计时/农历/天气/世界时钟", // STR_ABOUT_FEATURES
    "NTP校时/局域网NTP服务器/Web控制面板",  // STR_ABOUT_NETWORK
    "编译: MinGW-w64 + C++14",             // STR_ABOUT_COMPILE
    "北京 (UTC+8)",                        // STR_TZ_BEIJING
    "东京 (UTC+9)",                        // STR_TZ_TOKYO
    "纽约 (UTC-5)",                        // STR_TZ_NEW_YORK
    "伦敦 (UTC+0)",                        // STR_TZ_LONDON
    "悉尼 (UTC+11)",                       // STR_TZ_SYDNEY
    "莫斯科 (UTC+3)",                      // STR_TZ_MOSCOW
    "巴黎 (UTC+1)",                        // STR_TZ_PARIS
    "洛杉矶 (UTC-8)",                      // STR_TZ_LOS_ANGELES
    "GUIC 2.0 远程控制",                   // STR_WEB_TITLE
    "时钟:",                               // STR_WEB_CLOCK
    "倒计时:",                             // STR_WEB_COUNTDOWN
    "消息:",                               // STR_WEB_MESSAGE
    "农历:",                               // STR_WEB_LUNAR
    "天气:",                               // STR_WEB_WEATHER
    "开",                                  // STR_WEB_ON
    "关",                                  // STR_WEB_OFF
    "显示消息:",                           // STR_WEB_MSG_LABEL
    "天气城市:",                           // STR_WEB_CITY_LABEL
    "字体大小:",                           // STR_WEB_FONT_LABEL
    "每日备注:",                           // STR_WEB_REMARK_LABEL
    "设置",                                // STR_WEB_SET
    "刷新",                                // STR_WEB_REFRESH
    "设置",                                // STR_SUBMENU_SETTINGS
    "显示",                                // STR_SUBMENU_VIEW
    "服务器",                              // STR_SUBMENU_SERVERS
    "时钟",                                // STR_SUBMENU_CLOCK
    "局域网NTP服务器",                     // STR_SERVER_NTP
    "Web控制面板",                         // STR_SERVER_HTTP
    "服务器设置",                          // STR_SERVER_SETTINGS
    "NTP服务器端口:",                      // STR_NTP_PORT_LABEL
    "Web控制面板端口:",                    // STR_HTTP_PORT_LABEL
    "范围: 1-65535（NTP 默认 123，被占用时可修改）", // STR_PORT_RANGE_LABEL
    "端口无效，请输入 1-65535 之间的数字。", // STR_PORT_INVALID
    "服务器端口已更新，运行中的服务器已按新端口重启。", // STR_SERVER_PORTS_UPDATED
};

static const char* g_stringsEN[] = {
    "GUI Clock",                            // STR_WINDOW_TITLE
    "Set Display Message",                  // STR_SET_MESSAGE
    "Set One-Time Countdown",               // STR_SET_COUNTDOWN_ONCE
    "Set Daily Countdown",                  // STR_SET_COUNTDOWN_DAILY
    "Message",                              // STR_SHOW_MESSAGE
    "Clock",                                // STR_SHOW_CLOCK
    "Countdown",                            // STR_SHOW_COUNTDOWN
    "Auto Layout",                          // STR_SHOW_LAYOUT
    "Lunar",                                // STR_SHOW_LUNAR
    "Weather",                              // STR_SHOW_WEATHER
    "Borderless",                           // STR_SHOW_BORDERLESS
    "Window Settings",                      // STR_WINDOW_SETTINGS
    "Font Settings",                        // STR_FONT_SETTINGS
    "Color Settings",                       // STR_COLOR_SETTINGS
    "Weather City Settings",                // STR_WEATHER_CITY_SETTINGS
    "NTP Server Settings",                  // STR_NTP_SERVER_SETTINGS
    "Add Special Day",                      // STR_ADD_SPECIAL_DAY
    "World Clock",                          // STR_WORLD_CLOCK
    "Analog Clock",                         // STR_ANALOG_CLOCK
    "Menu",                                 // STR_MENU
    "Language",                             // STR_LANGUAGE
    "Chinese (Simplified)",                 // STR_LANGUAGE_ZH
    "English",                              // STR_LANGUAGE_EN
    "Start NTP Server",                     // STR_NTP_SERVER_START
    "Stop NTP Server",                      // STR_NTP_SERVER_STOP
    "Start Web Control Panel",              // STR_HTTP_SERVER_START
    "Stop Web Control Panel",               // STR_HTTP_SERVER_STOP
    "About",                                // STR_ABOUT
    "Welcome to GUI Clock!",                // STR_WELCOME_MESSAGE
    "Daily Countdown Remark",               // STR_DAILY_REMARK
    "Countdown Ended!",                     // STR_COUNTDOWN_ENDED
    "Countdown Not Set",                    // STR_COUNTDOWN_NOT_SET
    "Countdown: %lldd %lldh %lldm %llds",   // STR_COUNTDOWN_FMT
    "Loading weather...",                   // STR_WEATHER_LOADING
    "Weather fetch failed",                 // STR_WEATHER_FAILED
    "Humidity:",                            // STR_HUMIDITY
    "Wind:",                                // STR_WIND
    "Lunar",                                // STR_LUNAR
    "OK",                                   // STR_OK
    "Cancel",                               // STR_CANCEL
    "Error",                                // STR_ERROR
    "Prompt",                               // STR_PROMPT
    "Confirm",                              // STR_CONFIRM
    "Info",                                 // STR_TIP
    "Borderless mode takes effect on next launch",  // STR_BORDERLESS_NOTE
    "Language switched. Restart to take full effect.", // STR_LANGUAGE_SWITCHED
    "NTP server started on port %d",        // STR_NTP_SERVER_STARTED_FMT
    "NTP server stopped",                   // STR_NTP_SERVER_STOPPED
    "Web control panel started\nhttp://localhost:%d", // STR_HTTP_SERVER_STARTED_FMT
    "Web control panel stopped",            // STR_HTTP_SERVER_STOPPED
    "Enter display message:",               // STR_MSG_INPUT_LABEL
    "Enter target time (YYYY-MM-DD HH:MM:SS):", // STR_COUNTDOWN_ONCE_INPUT_LABEL
    "Invalid time format!\nExpected: YYYY-MM-DD HH:MM:SS", // STR_COUNTDOWN_ONCE_INVALID
    "One-time countdown set successfully!", // STR_COUNTDOWN_ONCE_SUCCESS
    "Enter daily time (HH:MM:SS):",         // STR_COUNTDOWN_DAILY_INPUT_LABEL
    "Remark:",                              // STR_COUNTDOWN_REMARK_LABEL
    "Invalid time format!\nExpected: HH:MM:SS", // STR_COUNTDOWN_DAILY_INVALID
    "Daily countdown set successfully!",    // STR_COUNTDOWN_DAILY_SUCCESS
    "Allow resizing",                       // STR_WINDOW_RESIZABLE
    "Window size:",                         // STR_WINDOW_SIZE_LABEL
    "Width:",                               // STR_WINDOW_WIDTH_LABEL
    "Height:",                              // STR_WINDOW_HEIGHT_LABEL
    "Range: width 300-2000, height 200-1500", // STR_WINDOW_RANGE_LABEL
    "Invalid window size.\nWidth: 300-2000, Height: 200-1500", // STR_WINDOW_SIZE_INVALID
    "Font size:",                           // STR_FONT_SIZE_LABEL
    "Font name:",                           // STR_FONT_NAME_LABEL
    "Select font color:",                   // STR_COLOR_LABEL
    "Choose Color",                         // STR_COLOR_BUTTON
    "Name:",                                // STR_SPECIAL_NAME_LABEL
    "Month:",                               // STR_SPECIAL_MONTH_LABEL
    "Day:",                                 // STR_SPECIAL_DAY_LABEL
    "Year:",                                // STR_SPECIAL_YEAR_LABEL
    "Time:",                                // STR_SPECIAL_TIME_LABEL
    "Repeat every year",                    // STR_SPECIAL_ANNUAL_CHECK
    "Solar term",                           // STR_SPECIAL_SOLAR_TERM_CHECK
    "Please enter a name!",                 // STR_SPECIAL_NAME_REQUIRED
    "Invalid month/day",                    // STR_SPECIAL_MONTH_DAY_INVALID
    "Invalid year",                         // STR_SPECIAL_YEAR_INVALID
    "Added successfully",                   // STR_SPECIAL_ADD_SUCCESS
    "Close",                                // STR_CLOSE
    "City name (e.g. Beijing, Shanghai, Tokyo):", // STR_WEATHER_CITY_LABEL
    "Custom NTP server (empty for default):", // STR_NTP_SERVER_LABEL
    "About GUIC 2.0",                       // STR_ABOUT_TITLE
    "GUIC 2.0 - GUI Clock",                 // STR_ABOUT_DESC
    "Version %d.%d.%d.%d",                 // STR_ABOUT_VERSION
    "Features: clock/countdown/lunar/weather/world clock", // STR_ABOUT_FEATURES
    "NTP sync / LAN NTP server / Web control panel", // STR_ABOUT_NETWORK
    "Built with: MinGW-w64 + C++14",        // STR_ABOUT_COMPILE
    "Beijing (UTC+8)",                      // STR_TZ_BEIJING
    "Tokyo (UTC+9)",                        // STR_TZ_TOKYO
    "New York (UTC-5)",                     // STR_TZ_NEW_YORK
    "London (UTC+0)",                       // STR_TZ_LONDON
    "Sydney (UTC+11)",                      // STR_TZ_SYDNEY
    "Moscow (UTC+3)",                       // STR_TZ_MOSCOW
    "Paris (UTC+1)",                        // STR_TZ_PARIS
    "Los Angeles (UTC-8)",                  // STR_TZ_LOS_ANGELES
    "GUIC 2.0 Remote Control",              // STR_WEB_TITLE
    "Clock:",                               // STR_WEB_CLOCK
    "Countdown:",                           // STR_WEB_COUNTDOWN
    "Message:",                             // STR_WEB_MESSAGE
    "Lunar:",                               // STR_WEB_LUNAR
    "Weather:",                             // STR_WEB_WEATHER
    "On",                                   // STR_WEB_ON
    "Off",                                  // STR_WEB_OFF
    "Display message:",                     // STR_WEB_MSG_LABEL
    "Weather city:",                        // STR_WEB_CITY_LABEL
    "Font size:",                           // STR_WEB_FONT_LABEL
    "Daily remark:",                        // STR_WEB_REMARK_LABEL
    "Set",                                  // STR_WEB_SET
    "Refresh",                              // STR_WEB_REFRESH
    "Settings",                             // STR_SUBMENU_SETTINGS
    "View",                                 // STR_SUBMENU_VIEW
    "Servers",                              // STR_SUBMENU_SERVERS
    "Clock",                                // STR_SUBMENU_CLOCK
    "LAN NTP Server",                       // STR_SERVER_NTP
    "Web Control Panel",                    // STR_SERVER_HTTP
    "Server Settings",                      // STR_SERVER_SETTINGS
    "NTP server port:",                     // STR_NTP_PORT_LABEL
    "Web control panel port:",              // STR_HTTP_PORT_LABEL
    "Range: 1-65535 (NTP defaults to 123; change if occupied)", // STR_PORT_RANGE_LABEL
    "Invalid port. Enter a number between 1 and 65535.", // STR_PORT_INVALID
    "Server ports updated. Running servers restarted with the new ports.", // STR_SERVER_PORTS_UPDATED
};

static const char** g_currentStrings = g_stringsZH;

void I18nInit(Language lang)
{
    if (lang == Language::EN_US)
        g_currentStrings = g_stringsEN;
    else
        g_currentStrings = g_stringsZH;
}

const char* i18nGet(StringID id)
{
    if (id < 0 || id >= STR_COUNT) return "";
    return g_currentStrings[id];
}
