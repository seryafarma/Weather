# Weather ⛅
Let's display the current weather and how is the forecast for the next few days? 

![display](https://github.com/seryafarma/Weather/blob/main/display.jpg)

Yeah it is mostly cloudy around here.

## Hardware
The hardware utilizes a simple Wemos D1 board and 4x8x8 dot matrix module. It is simply using one SPI connection from the Wemos D1 to the module. The board needs to be connected to the internet to fetch the data.

## Software
* A simple states approach with Arduino state machine library from https://github.com/jrullan/StateMachine.
* Weather information from https://openweathermap.org/
* NTC time from `ezTime` https://github.com/ropg/ezTime

It serves as a clock display, where every 5 minutes it will ask for the weather data and display it.

![github_state_diagram](https://github.com/seryafarma/Weather/blob/main/state_diagram.png)

## Note
We are using OpenWeatherMap API

https://openweathermap.org/price

> Free
> 60 calls/minute
> 1,000,000 calls/month

If we call 1 call a minute = 1440 calls a day = 43200 calls a month.
However since the weather doesn't change that much, this can be lowered to a call every 5 minutes.

This project calls once every 5 minutes for the current weather.
