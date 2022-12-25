#pragma once
#include <string>
#include <vector>

struct LocationData {
    uint32_t x, y;
    std::vector<std::wstring> forecast;
};

struct WxData {
    std::vector<time_t> forecastTime;
    std::vector<LocationData> locations;
};

WxData GetWxData(std::wstring pathToForecastJson);