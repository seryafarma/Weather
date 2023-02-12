//---------------------------------------------------------------------------------------------------------------------
// weather_reader.hpp
// Read the weather from OpenWeatherMap and then provide the clean information as weather_info.
//---------------------------------------------------------------------------------------------------------------------
#ifndef WEATHER_READER_HPP
#define WEATHER_READER_HPP

#include "weather_info.hpp"

namespace Weather
{
class WeatherReader
{
public:
    WeatherReader(String properties);

    void read();

    WeatherInfo get_current_weather();

private:
    void process_json(String& json_array);

    void get_http_weather(const char* serverName);

    String get_request(const char* server);

    String url;

    WeatherInfo current_weather;
};
} // namespace Weather

#endif
