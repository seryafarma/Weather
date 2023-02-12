//---------------------------------------------------------------------------------------------------------------------
// main.cpp
// Get the weather from OpenWeather and show it on the display.
// Style: State Machine
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <StateMachine.h>

//---------------------------------------------------------------------------------------------------------------------
// Local Includes
//---------------------------------------------------------------------------------------------------------------------
#include "auth.hpp"
#include "weather_info.hpp"
#include "weather_reader.hpp"

using Weather::WeatherInfo;
using Weather::WeatherReader;

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

//---------------------------------------------------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

StateMachine machine = StateMachine();
String things_to_show = "";
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

    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
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