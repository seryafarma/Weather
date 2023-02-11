//---------------------------------------------------------------------------------------------------------------------
// GetWeather.ino
// Get the weather from OpenWeather and show it on the display.
// Style: State Machine
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <StateMachine.h>

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
#define STATE_DELAY_MS 100

//---------------------------------------------------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
String things_to_show = "0";

uint32_t dummy_counter = 0;

StateMachine machine = StateMachine();

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
void setup()
{
    Serial.begin(9600);
    P.begin();

    State* idle = machine.addState(&idle_state);
    State* gather = machine.addState(&gather_state);
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
    Serial.println("[Idle State]");
    if (P.displayAnimate())
    {
        P.displayText(things_to_show.c_str(), PA_LEFT, 100, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_gather()
{
    Serial.println("[Event: Gather]");
    bool return_val = false;

    static const uint32_t ONE_MINUTE = 1000 * 60;
    uint32_t current_millis = millis();

    // A simple timer actually for a minute...
    if (current_millis - previous_millis > ONE_MINUTE)
    {
        // Prepare to go to the next state.
        return_val = true;
        // Save the last time tick.
        previous_millis = current_millis;
    }

    return return_val;
}

//---------------------------------------------------------------------------------------------------------------------
void gather_state()
{
    Serial.println("[Gather State]");
    if (machine.executeOnce) // Execute one time gathering of weather.
    {
        Serial.println("[Gather State, Entering]");
        // Dummy
        dummy_counter += 1;
        things_to_show = String(dummy_counter);
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool ev_idle()
{
    Serial.println("[Event: Idle]");
    // Go to Idle immediately.
    return true;
}