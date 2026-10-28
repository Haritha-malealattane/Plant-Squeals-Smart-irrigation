# Wiring guide

The pins below come from the report's Pico sketch (`firmware/pico_soil_moisture_pump`) and its architecture diagram. Rows marked **new** were added with the extended sketches and haven't been tested on the prototype yet, so check them against your own board.

## Raspberry Pi Pico

| Pico pin | Connects to | Source |
|---|---|---|
| GP26 | DHT11 data pin | Original sketch |
| GP27 (A1) | Soil-moisture sensor, analog output (AO) | Original sketch |
| GP28 (A2) | Sound sensor, analog output (AO) | **New** |
| GP15 | Pump driver input (relay module or L293D input) | Original sketch. HIGH when the soil reading is 45 or below |
| GP16 (TX) | NodeMCU D5 (RX) | Original sketch (serial link to the NodeMCU) |
| GP17 (RX) | NodeMCU D6 (TX) | Original sketch (unused so far) |
| 3V3 (OUT) | VCC of the DHT11, soil sensor and sound sensor | See notes |
| GND | GND of the NodeMCU, sensors and pump driver | Common ground is required |

## NodeMCU (ESP8266)

| NodeMCU pin | Connects to |
|---|---|
| D5 | Pico GP16 (TX) |
| D6 | Pico GP17 (RX) |
| GND | Pico GND |
| USB | Power, and the serial monitor at 115200 baud |

Both boards use 3.3 V logic, so the serial lines can be connected directly.

## Notes

- **Power the sensors from 3V3, not 5V.** The Pico's pins only tolerate 3.3 V, and a sensor powered from 5V can send a 5V signal into them. The DHT11 works from 3.3 V.
- **Never power the pump from a Pico pin.** GP15 only signals the relay module or the L293D, and the pump needs its own supply. Some relay boards switch on when the input is LOW, so if the pump behaves backwards, swap the two `digitalWrite` values in the sketch.
- **Mains side:** the report's power supply uses a 230 V to 12 V transformer, bridge rectifier, filter and 7805 regulator. Only build or touch the mains side if you know how to do it safely, or use a ready-made enclosed 5 V / 12 V adapter instead.
- **Sound sensor:** the module in this prototype (microphone with an LM393 comparator) measures audible sound level. Detecting true plant ultrasonic emissions needs an ultrasonic microphone and a much higher sampling rate than this setup, so the SOUND values on the dashboard are a general sound-level reading.
- **Soil sensor calibration:** the threshold of 45 in the sketch was chosen for the prototype's sensor and soil. Print the readings in dry and wet soil and adjust `SOIL_THRESHOLD` for your own setup.
