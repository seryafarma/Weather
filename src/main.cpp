//---------------------------------------------------------------------------------------------------------------------
// main.cpp
// Get the weather from OpenWeather and show it on the display.
// Style: State Machine
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <StateMachine.h>
#include <WiFiClient.h>

WiFiClient wifiClient;

//---------------------------------------------------------------------------------------------------------------------
// Local Includes
//---------------------------------------------------------------------------------------------------------------------
#include "auth.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------------------------------------------------

// Displays.
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4
#define CLK_PIN       5
#define DATA_PIN      7
#define CS_PIN        4
// State Machine.
#define STATE_DELAY_MS 50

//---------------------------------------------------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------------------------------------------------

struct WeatherInfo
{
    String name;
    String weather;
    String weather_desc;
    String temperature_c;
    String humidity_perc;
    String counter;

    void clear()
    {
        weather = weather_desc = temperature_c = humidity_perc = "";
    }

    String get_string()
    {
        String n = name + ", " + weather + ": " + weather_desc + ". T: " + temperature_c + " C, H: " + humidity_perc +
                   "% ==== [Counter " + counter + "]";
        return n;
    }
};

class WeatherReader
{
public:
    WeatherReader(String properties)
    {
        url = "http://api.openweathermap.org/data/2.5/weather?" + properties;
    }

    void read()
    {
        get_http_weather(url.c_str());
    }

    WeatherInfo get_current_weather()
    {
        return current_weather;
    }

private:
    void get_http_weather(const char* serverName)
    {
        static int mycounter = 0;

        current_weather.clear();
        if (WiFi.status() == WL_CONNECTED)
        {
            String json_array = get_request(serverName);

            Serial.println(json_array);

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
    }

    String get_request(const char* server)
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

    String url;
    WeatherInfo current_weather;
};

//---------------------------------------------------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
String things_to_show = "";

StateMachine machine = StateMachine();

WeatherInfo weather_info;
WeatherReader wr(Authentication::API_KEY);

bool time_to_gather = false;
bool pending_gather = false;
uint32_t previous_millis = 0;
//---------------------------------------------------------------------------------------------------------------------
// Functions Declaration
//---------------------------------------------------------------------------------------------------------------------
void idle_state();
bool ev_gather();
void gather_state();
bool ev_idle();

//---------------------------------------------------------------------------------------------------------------------
// Setup and Loop
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void connectWifi()
{
    WiFi.begin(Authentication::WIFI_SSID, Authentication::WIFI_PASSWORD);
    Serial.print("Connecting to ");
    Serial.println(Authentication::WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.println(WiFi.localIP());
    Serial.println();
}

//---------------------------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(9600);
    P.begin();
    delay(1500);
    connectWifi();

    State* gather = machine.addState(&gather_state);
    State* idle = machine.addState(&idle_state);
    idle->addTransition(&ev_gather, gather);
    gather->addTransition(&ev_idle, idle);
}

//---------------------------------------------------------------------------------------------------------------------
void loop()
{
    machine.run();
    delay(STATE_DELAY_MS);
}

//---------------------------------------------------------------------------------------------------------------------
// Functions Definition
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void idle_state()
{
    if (machine.executeOnce)
    {
        Serial.println("[Idle State, Entering]");
    }

    if (P.displayAnimate())
    {
        // Done displaying, do we have a pending request?
        if (pending_gather)
        {
            // Clear the pending flag.
            pending_gather = false;
            // Get ready to gather.
            time_to_gather = true;
            Serial.println("Time to gather [Event: Gather]");
        }
        else
        {
            // Nothing pending, redraw.
            P.displayText(things_to_show.c_str(), PA_LEFT, 50, 50, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        }
    }

    static const uint32_t ONE_MINUTE = 1000 * 60;
    uint32_t current_millis = millis();

    // A simple timer actually for a minute...
    if (current_millis - previous_millis > ONE_MINUTE)
    {
        // Save the last time tick.
        previous_millis = current_millis;
        // Add a pending request.
        pending_gather = true;
        // Pending flag.
        Serial.println("Pending [Event: Gather]");
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_gather()
{
    if (time_to_gather)
    {
        time_to_gather = false;
        Serial.println("[Event: Gather]");
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void gather_state()
{
    if (machine.executeOnce) // Execute one time gathering of weather.
    {
        Serial.println("[Gather State, Entering]");

        wr.read();
        weather_info = wr.get_current_weather();

        things_to_show = String(weather_info.get_string());
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_idle()
{
    Serial.println("[Event: Idle]");
    // Go to Idle immediately.
    return true;
}