<h1 align="center">Wetterstation</h1>

## Description

This is a project to build a weather station with an Arduino Uno communicating with an ESP01 via UDP.

## Hardware

- Arduino Uno
- ESP01
- DHT22
- BMP180

## What it can do

- Measure temperature, humidity and pressure
- Send data to a server via UDP
- Display data on a 16x2 LCD
- It can steer the hot water boiler in my house

## What I want to add

- A 3D printed case
- A battery
- Connect it to a web application to display the data

## Bugs

- The ESP01 is not always able to connect to the WiFi network
- The ESP01 is not always able to send the data to the server
- Some packages are lost