#ifndef RESOURCE_H
#define RESOURCE_H

#define IDI_ICON1 101

// 对话框资源 ID
#define IDD_MESSAGE_DIALOG           2001
#define IDD_COUNTDOWN_ONCE_DIALOG    2002
#define IDD_COUNTDOWN_DAILY_DIALOG   2003
#define IDD_WINDOW_SETTINGS_DIALOG   2004
#define IDD_FONT_SETTINGS_DIALOG     2005
#define IDD_COLOR_SETTINGS_DIALOG    2006
#define IDD_SPECIAL_DAY_DIALOG       2007
#define IDD_WORLD_CLOCK_DIALOG       2009
#define IDD_ANALOG_CLOCK_DIALOG      2010
#define IDD_WEATHER_CITY_DIALOG      2011
#define IDD_NTP_SERVER_DIALOG        2012
#define IDD_ABOUT_DIALOG             2013
#define IDD_SERVER_SETTINGS_DIALOG   2014

// 菜单项 ID
#define ID_MENU_SET_MESSAGE          40001
#define ID_MENU_SET_COUNTDOWN_ONCE   40002
#define ID_MENU_SET_COUNTDOWN_DAILY  40003
#define ID_MENU_TOGGLE_CLOCK         40004
#define ID_MENU_TOGGLE_COUNTDOWN     40005
#define ID_MENU_TOGGLE_LAYOUT        40006
#define ID_MENU_SETTINGS_WINDOW      40007
#define ID_MENU_SETTINGS_FONT        40008
#define ID_MENU_SETTINGS_COLOR       40009
#define ID_MENU_ADD_SPECIAL_DAY      40010
#define ID_MENU_WORLD_CLOCK          40012
#define ID_MENU_ANALOG_CLOCK         40013
#define ID_MENU_TOGGLE_MESSAGE       40014
#define ID_MENU_TOGGLE_BORDERLESS    40015
#define ID_MENU_TOGGLE_LUNAR         40016
#define ID_MENU_TOGGLE_WEATHER       40017
#define ID_MENU_SETTINGS_WEATHER_CITY 40018
#define ID_MENU_SETTINGS_NTP         40019
#define ID_MENU_LANGUAGE_ZH          40020
#define ID_MENU_LANGUAGE_EN          40021
#define ID_MENU_TOGGLE_NTP_SERVER    40022
#define ID_MENU_TOGGLE_HTTP_SERVER   40024
#define ID_MENU_ABOUT                40026
#define ID_MENU_SETTINGS_SERVER      40027

// 控件 ID
#define IDC_EDIT_MESSAGE                1000
#define IDC_EDIT_ONCE_TIME              1001
#define IDC_EDIT_DAILY_TIME             1002
#define IDC_STATIC                      1003
#define IDC_EDIT_DAILY_REMARK           1004
#define IDC_CHECK_RESIZABLE             1005
#define IDC_EDIT_WINDOW_WIDTH           1006
#define IDC_EDIT_WINDOW_HEIGHT          1007
#define IDC_EDIT_FONT_SIZE              1008
#define IDC_EDIT_FONT_NAME              1009
#define IDC_BUTTON_COLOR                1010
#define IDC_EDIT_NAME                   1011
#define IDC_EDIT_MONTH                  1012
#define IDC_EDIT_DAY                    1013
#define IDC_CHECK_ANNUAL                1014
#define IDC_EDIT_YEAR                   1015
#define IDC_CHECK_SOLAR_TERM            1016
#define IDC_EDIT_HOUR                   1017
#define IDC_EDIT_MINUTE                 1018
#define IDC_LIST_WORLD_CLOCKS           1020
#define IDC_STATIC_WORLD_CLOCK_DISPLAY  1021
#define IDC_STATIC_ANALOG_CLOCK         1022
#define IDC_EDIT_WEATHER_CITY           1023
#define IDC_EDIT_NTP_SERVER             1024
#define IDC_STATIC_ABOUT                1025

// 静态文本控件 ID（国际化：对话框标签唯一 ID，可在代码中逐个覆写文本）
#define IDC_STATIC_MSG_INPUT             1030
#define IDC_STATIC_ONCE_INPUT            1031
#define IDC_STATIC_DAILY_INPUT           1032
#define IDC_STATIC_DAILY_REMARK          1033
#define IDC_STATIC_WINDOW_SIZE           1034
#define IDC_STATIC_WIDTH                 1035
#define IDC_STATIC_HEIGHT                1036
#define IDC_STATIC_WINDOW_RANGE          1037
#define IDC_STATIC_FONT_SIZE             1038
#define IDC_STATIC_FONT_NAME             1039
#define IDC_STATIC_COLOR_LABEL           1040
#define IDC_STATIC_SPECIAL_NAME          1041
#define IDC_STATIC_SPECIAL_MONTH         1042
#define IDC_STATIC_SPECIAL_DAY           1043
#define IDC_STATIC_SPECIAL_TIME          1044
#define IDC_STATIC_SPECIAL_YEAR          1045
#define IDC_STATIC_WEATHER_LABEL         1046
#define IDC_STATIC_NTP_LABEL             1047
#define IDC_STATIC_ABOUT_TITLE           1048
#define IDC_STATIC_ABOUT_VER             1049
#define IDC_STATIC_ABOUT_FEATURES        1050
#define IDC_STATIC_ABOUT_NETWORK         1051
#define IDC_STATIC_ABOUT_COMPILE         1052
#define IDC_STATIC_NTP_PORT_LABEL        1054
#define IDC_EDIT_NTP_PORT                1055
#define IDC_STATIC_HTTP_PORT_LABEL       1056
#define IDC_EDIT_HTTP_PORT               1057
#define IDC_STATIC_PORT_RANGE            1058

#endif
