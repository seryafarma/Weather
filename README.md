# Weather ⛅
Let's display the current weather and how is the forecast for the next few days? 

![display](https://github.com/seryafarma/Weather/blob/main/display.jpg)

Yeah it is mostly cloudy around here.

## Hardware
The hardware utilizes a simple Wemos D1 board and 4x8x8 dot matrix module. It is simply using one SPI connection from the Wemos D1 to the module. The board needs to be connected to the internet to fetch the data.

## Software
* A simple two states approach with Arduino state machine library from https://github.com/jrullan/StateMachine.
* Weather information from https://openweathermap.org/

![github_state_diagram](https://github.com/seryafarma/Weather/blob/main/state_diagram.png)

## Note
We are using OpenWeatherMap API

https://openweathermap.org/price

> Free
> 60 calls/minute
> 1,000,000 calls/month

We call 1 call a minute = 1440 calls a day = 43200 calls a month.

However since the weather doesn't change that much, this can be lowered to a call every 5 minutes.
