#include "weather_info.hpp"

namespace Weather
{
//---------------------------------------------------------------------------------------------------------------------
void WeatherInfo::clear()
{
    name = weather = weather_desc = temperature_c = humidity_perc = counter = "";
}

//---------------------------------------------------------------------------------------------------------------------
String WeatherInfo::get_string()
{
    String n = name + ", " + weather + ": " + weather_desc + ". T: " + temperature_c + " C, H: " + humidity_perc +
               "% ==== [Counter " + counter + "]";
    return n;
}
} // namespace Weather