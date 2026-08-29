#ifndef WEATHER_H
#define WEATHER_H

#include <string>

// 天气查询结果
struct WeatherInfo {
    bool    valid;          // 是否获取成功
    std::string condition;  // 天气状况 (如 "晴")
    std::string temp;       // 温度 (如 "+22°C")
    std::string humidity;   // 湿度
    std::string wind;       // 风力
    std::string raw;        // 原始返回文本
};

// 从 wttr.in 获取天气（异步，需在独立线程中调用）
WeatherInfo FetchWeather(const std::string& city);

// 解析 wttr.in 简单格式返回值
WeatherInfo ParseWeatherResponse(const std::string& response);

#endif // WEATHER_H
