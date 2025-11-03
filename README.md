Overview

The Smart Plant Monitoring System is an IoT-based project that helps monitor and maintain the health of plants automatically. Using sensors connected to an ESP32, the system tracks environmental conditions like temperature, humidity, soil moisture, and motion, while enabling remote control via the Blynk mobile app.

⚙️ Features

🌡️ Temperature & Humidity Monitoring using DHT11

💧 Soil Moisture Detection to monitor plant watering needs

🚶‍♂️ Motion Detection using PIR sensor (can trigger alerts or lights)

🔌 Automated Watering System using Relay Module

📱 Blynk App Integration for real-time data monitoring and control

☁️ IoT Connectivity using ESP32 Wi-Fi capabilities

🧠 Components Used
Component	Quantity	Description

ESP32	1	Main microcontroller with Wi-Fi
DHT11 Sensor	1	Measures temperature & humidity
Soil Moisture Sensor	1	Detects soil water level
PIR Sensor	1	Detects human/animal motion
Relay Module	1	Controls water pump or motor
Water Pump (optional)	1	Used for automatic watering
Jumper Wires	-	For connections
Breadboard / PCB	1	For circuit assembly

Working Principle

The DHT11 sensor continuously measures temperature and humidity.

The Soil Moisture Sensor checks if the soil is dry.

When the soil moisture drops below a threshold, the Relay turns ON, activating the water pump automatically.

The PIR Sensor detects motion and can send alerts via the Blynk app.

All sensor readings are sent to Blynk Cloud, allowing users to monitor data in real time from their phone.
