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
WeatherInfo weather_info;
WeatherReader wr(Authentication::API_KEY);

bool time_to_gather = false;
bool pending_gather = false;
uint32_t previous_millis = 0;

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

void scan_lines();

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
    waitForSync();

    State* gather = machine.addState(&gather_state);
    State* ntc = machine.addState(&ntc_state);
    State* clock = machine.addState(&clock_state);
    State* idle = machine.addState(&idle_state);
    idle->addTransition(&ev_clock, clock);
    gather->addTransition(&ev_ntc, ntc);
    ntc->addTransition(&ev_idle, idle);
    clock->addTransition(&ev_gather, gather);
}

//---------------------------------------------------------------------------------------------------------------------
void loop()
{
    String hm = amst.dateTime("Hi"); // move this to statemachine.

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

    static const uint32_t FIVE_MINUTE = 5UL * 60UL * 1000UL;
    uint32_t current_millis = millis();

    // A simple timer actually for a minute...
    if (current_millis - previous_millis > FIVE_MINUTE)
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

//---------------------------------------------------------------------------------------------------------------------
void scanlines()
{
    mx.clear();
    for (uint8_t row = 0; row < ROW_SIZE; row++)
    {
        mx.setRow(row, 0xff);
        delay(300);
        mx.setRow(row, 0x00);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void printTime(String* time)
{
    char clockchar[5];
    time->toCharArray(clockchar, 5);

    mx.clear();
    mx.update(MD_MAX72XX::OFF);

    mx.setRow(2, 2, 0x01);
    mx.setRow(2, 4, 0x01);
    mx.setChar(1 * COL_SIZE - 2, clockchar[3]);
    mx.setChar(2 * COL_SIZE - 2, clockchar[2]);
    mx.setChar(3 * COL_SIZE - 1, clockchar[1]);
    mx.setChar(4 * COL_SIZE - 1, clockchar[0]);

    mx.update();
    mx.update(MD_MAX72XX::ON);
}