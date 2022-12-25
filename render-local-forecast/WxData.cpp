#include "WxData.h"
#include "nlohmann\json.hpp"
#include <time.h>
#include <fstream>
#include <sstream>

using namespace std;
using json = nlohmann::json;

const wchar_t* TCCEmoji(uint32_t tcc, uint32_t lightning, string precipType, double precipRate)
{
    if (!precipRate && lightning)
        return L"🌩";
    else if (precipRate && lightning)
        return L"⛈";
    else if (precipType != "") {
        if (precipType == "ice")
            return L"🧊";
        else if (precipType == "rain")
            return L"🌧️";
        else if (precipType == "snow")
            return L"🌨️";

        return L"";
    }
    else if (tcc < 5)
        return L"🌞";
    else if (tcc < 33)
        return L"☀️";
    else if (tcc < 66)
        return L"🌤️";
    else if (tcc < 95)
        return L"⛅";
    return L"☁️";
}

WxData GetWxData(wstring pathToForecastJson)
{
    std::ifstream f(pathToForecastJson);
    json forecast = json::parse(f);
    WxData wxData;

    auto locations = forecast["locations"];
    auto forecastLength = forecast["forecastTimes"].size();

    for (auto& forecastTime: forecast["forecastTimes"])
    {
        struct tm time = {};
        sscanf_s(forecastTime.get<string>().c_str(), "%4d-%2d-%2dT%2d", &time.tm_year, &time.tm_mon, &time.tm_mday, &time.tm_hour);
        time.tm_year -= 1900;
        time.tm_mon -= 1;

        wxData.forecastTime.push_back(_mkgmtime(&time));
    }

    for (auto itr = locations.begin(); itr != locations.end(); itr++)
    {
        auto location = itr.value();
        if (!location["isCity"].get<bool>())
            continue;

        LocationData data = {};
        auto wx = location["wx"];
        data.x = location["coords"]["x"].get<uint32_t>();
        data.y = location["coords"]["y"].get<uint32_t>();
        for (auto i = 0; i < forecastLength; i++)
        {
            wstringstream ss;
            auto emoji = TCCEmoji(wx["totalCloudCover"][i].get<int32_t>(), wx["lightning"][i].get<int32_t>(), wx["precipType"][i].get<string>(), wx["precipRate"][i].get<double>());
            ss << wx["temperature"][i].get<int32_t>() << "°F " << emoji;
            data.forecast.push_back(ss.str());
        }

        wxData.locations.push_back(data);
    }

    return wxData;
}