#include "weather_reader.hpp"

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

WiFiClient wifiClient;

namespace Weather
{
//---------------------------------------------------------------------------------------------------------------------
WeatherReader::WeatherReader(String properties)
{
    url = "http://api.openweathermap.org/data/2.5/weather?" + properties;
}

//---------------------------------------------------------------------------------------------------------------------
void WeatherReader::read()
{
    get_http_weather(url.c_str());
}

//---------------------------------------------------------------------------------------------------------------------
WeatherInfo WeatherReader::get_current_weather()
{
    return current_weather;
}

//---------------------------------------------------------------------------------------------------------------------
void WeatherReader::process_json(String& json_array)
{
    static int mycounter = 0;

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json_array);

    {
        const char* weather = NULL;
        const char* weather_desc = NULL;
        const char* name = NULL;
        double temp = 0.0;
        long hum = 0;

        weather = doc["weather"][0]["main"];
        current_weather.weather = String(weather);
        weather_desc = doc["weather"][0]["description"];
        current_weather.weather_desc = String(weather_desc);

        temp = doc["main"]["temp"];
        temp -= 273.15;
        current_weather.temperature_c = String(temp);

        hum = doc["main"]["humidity"];
        current_weather.humidity_perc = String(hum);

        name = doc["name"];
        current_weather.name = String(name);

        mycounter++;
        current_weather.counter = String(mycounter);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void WeatherReader::get_http_weather(const char* serverName)
{
    current_weather.clear();
    if (WiFi.status() == WL_CONNECTED)
    {
        String json_array = get_request(serverName);

        Serial.println(json_array);

        process_json(json_array);
    }
}

//---------------------------------------------------------------------------------------------------------------------
String WeatherReader::get_request(const char* server)
{
    HTTPClient http;
    http.begin(wifiClient, server);
    int httpResponseCode = http.GET();

    String payload = "{}";

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();

    return payload;
}
} // namespace Weather