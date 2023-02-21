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
#include <ezTime.h>

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
// Timezone for the clock.
#define TIMEZONE "Europe/Amsterdam"

//---------------------------------------------------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

StateMachine machine = StateMachine();
String things_to_show = "";
String clock_to_show = "";
WeatherInfo weather_info;
WeatherReader wr(Authentication::API_KEY);

bool time_to_clock = false;
bool pending_clock = false;
bool time_to_gather = false;
bool pending_gather = false;

Timezone amst;

//---------------------------------------------------------------------------------------------------------------------
// Functions Declaration
//---------------------------------------------------------------------------------------------------------------------
void idle_state();
bool ev_gather();
void gather_state();
bool ev_idle();
void ntc_state();
bool ev_ntc();
void clock_state();
bool ev_clock();

//---------------------------------------------------------------------------------------------------------------------
// Setup and Loop
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void connect_wifi()
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
    waitForSync();


    State* gather = machine.addState(&gather_state);
    State* ntc = machine.addState(&ntc_state);
    State* clock = machine.addState(&clock_state);
    State* idle = machine.addState(&idle_state);
    idle->addTransition(&ev_clock, clock);
    gather->addTransition(&ev_ntc, ntc);
    ntc->addTransition(&ev_idle, idle);
    clock->addTransition(&ev_gather, gather);

    // Initialize pseudo state
    wr.read();
    weather_info = wr.get_current_weather();
    things_to_show = String(weather_info.get_string());
    clock_to_show = amst.dateTime("Hi");
    amst.setLocation(TIMEZONE);
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
    static bool once = false;

    if (machine.executeOnce)
    {
        once = false;
        Serial.println("[Idle State, Entering]");
    }

    if (P.displayAnimate())
    {
        if (once == false)
        {
            once = true;
            Serial.println("[Idle State, Show Weather]");
            P.displayText(things_to_show.c_str(), PA_LEFT, 50, 50, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        }
        else
        {
            time_to_clock = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_clock()
{
    if (time_to_clock)
    {
        time_to_clock = false;
        Serial.println("[Event: Clock]");
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void clock_state()
{
    // Stay in this state for a minute.
    static const uint32_t ONE_MINUTE = 60UL * 1000UL;
    static uint32_t previous_clock_millis = 0;
    uint32_t current_clock_millis = millis();

    if (machine.executeOnce)
    {
        Serial.println("[Clock State, Entering]");
    }

    if (P.displayAnimate())
    {
        // Done displaying, do we have a pending request?
        if (pending_gather)
        {
            // Clear the pending flag.
            pending_gather = false;
            // Get ready to show clock.
            time_to_gather = true;
            Serial.println("Time to gather weather data [Event: Gather]");
        }
        else
        {
            // Nothing pending, redraw.
            P.displayText(clock_to_show.c_str(), PA_CENTER, 50, 50, PA_PRINT);
        }
    }

    // A simple timer actually for a minute...
    if (current_clock_millis - previous_clock_millis > ONE_MINUTE)
    {
        // Save the last time tick.
        previous_clock_millis = current_clock_millis;
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

        static const uint32_t FIVE_MINUTE = 5UL * 60UL * 1000UL;
        static uint32_t previous_gather_millis = 0;
        uint32_t current_gather_millis = millis();

        // A simple timer actually for a minute...
        if (current_gather_millis - previous_gather_millis > FIVE_MINUTE)
        {
            // Save the last time tick.
            previous_gather_millis = current_gather_millis;

            Serial.println("[Gather State, Read the weather]");

            wr.read();
            weather_info = wr.get_current_weather();

            things_to_show = String(weather_info.get_string());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_ntc()
{
    Serial.println("[Event: NTC]");
    // Go to NTC immediately.
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void ntc_state()
{
    if (machine.executeOnce) // Execute one time gathering of NTC.
    {
        Serial.println("[NTC State, Entering]");

        clock_to_show = amst.dateTime("H:i");

        Serial.print("[NTC State, Showing Clock]");
        Serial.println(clock_to_show);
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_idle()
{
    Serial.println("[Event: Idle]");
    // Go to Idle immediately.
    return true;
}
