#ifndef WEATHER_INFO_HPP
#define WEATHER_INFO_HPP

#include <WString.h>

namespace Weather
{
struct WeatherInfo
{
    String name;
    String weather;
    String weather_desc;
    String temperature_c;
    String humidity_perc;
    String counter;

    void clear();

    String get_string();
};
} // namespace Weather

#endif
