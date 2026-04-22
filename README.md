# 💧 Smart Water Quality Monitoring System (IoT) 

A real-time embedded system built with **ESP32** to monitor water quality parameters (pH, TDS, and Temperature). The system features local alerts (LCD, LEDs, Buzzer), dual-cloud synchronization (HiveMQ & ThingSpeak), and a modern web-based interactive dashboard.

## 🚀 Features
- **Real-time Monitoring**: Continuous tracking of pH, TDS (Total Dissolved Solids), and Temperature.
- **Local Alerts**: 16x2 LCD display, color-coded LEDs, and a continuous buzzer alarm for unsafe conditions.
- **Dual Cloud Integration**:
  - **HiveMQ**: Low-latency MQTT for the real-time web dashboard.
  - **ThingSpeak**: Long-term data logging and visualization.
- **Interactive Dashboard**: A glassmorphism-styled web interface with live charts and remote control capabilities (adjusting thresholds, toggling alerts).
- **Simulated Hardware**: Fully compatible with the **Wokwi** simulator for easy prototyping.

## 🛠️ Hardware Setup (Simulated)
- **Microcontroller**: ESP32 DevKit V4
- **Sensors**: 
  - DS18B20 (Temperature)
  - Simulated pH & TDS via Potentiometers
- **Actuators**: Buzzer, Red/Green LEDs
- **Display**: I2C 16x2 LCD

## 💻 Tech Stack
- **Firmware**: C++ (Arduino Framework / PlatformIO)
- **Frontend**: HTML5, Vanilla CSS, JavaScript (MQTT.js, Chart.js)
- **Connectivity**: MQTT (HiveMQ), HTTP/MQTT (ThingSpeak)

## 📂 Project Structure
- `src/main.cpp`: Core firmware logic.
- `dashboard/`: Web-based dashboard files.
- `diagram.json`: Wokwi simulation circuit layout.
- `platformio.ini`: Project dependencies and configuration.

## 🚀 How to Run
1. **Simulation**: Open `diagram.json` in the Wokwi extension or website.
2. **Dashboard**: 
   - Navigate to the `dashboard/` folder.
   - Run a local server (e.g., `python3 -m http.server 8080`).
   - Open `localhost:8080` in your browser.
3. **MQTT**: The system uses `broker.hivemq.com`.

---
Developed by **Group 8** 💀🔥
