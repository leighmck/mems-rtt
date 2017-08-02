# mems-rtt
Example project to demonstrate tracing MEM data using Segger RTT.
Convert RTT events to MQTT to leverage pub/sub & trace visualisation tools.
Debug server to facilitate streaming or injection of data or events.

Features:
 - X-CUBE-MEMS1 demonstration code
 - Segger RTT

Roadmap:
 - Target
   - FreeRTOS
   - Logging
   - MQTT style trace output
   - Debug server
   
 - Host 
   - Python RTT telnet app
   - Local MQTT broker
   - RTT to MQTT messaging
   - Display MQTT messages using MQTT-SPY

Hardware:
STM32L073ZI Nucleo board
STM X-NUCLEO-IKS01A2 MEMS expansion board featuring:
 - LSM6DSL MEMS 3D accelerometer (±2/±4/±8/±16 g) and 3D gyroscope (±125/±245/±500/±1000/±2000 dps)
 - LSM303AGR MEMS 3D accelerometer (±2/±4/±8/±16 g) and MEMS3D magnetometer (±50 gauss)
 - LPS22HB MEMS pressure sensor, 260-1260 hPa absolute digital output barometer
 - HTS221: capacitive digital relative humidity and temperature

Toolchain:
ARM GCC Compiler
Eclipse Neon.3 IDE, CDT managed build
MCU GNU Eclipse plugin
Segger J-Link Debugger


