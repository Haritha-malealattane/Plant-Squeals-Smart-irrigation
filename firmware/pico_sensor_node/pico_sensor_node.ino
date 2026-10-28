/*
 * Extended sensor node for the Raspberry Pi Pico / Pico W
 *
 * ADDED AFTER THE ORIGINAL SUBMISSION. The report's code listing did not read
 * the sound sensor and sent only the soil value to the NodeMCU. This sketch keeps
 * the same soil-moisture pump logic as pico_soil_moisture_pump.ino and adds:
 *   - a sound-level reading (peak-to-peak amplitude over a short window)
 *   - one complete data line per second for the NodeMCU:
 *         S,<soil>,<temperature>,<humidity>,<sound>\n
 *     A value of -999.0 means "reading failed" (for example a DHT11 error).
 *
 * Not yet run on the prototype hardware: check the pins in docs/wiring.md first.
 */
#include <SoftwareSerial.h>
#include "DHT.h"

#define rxPin 17              // software serial RX (from NodeMCU, unused for now)
#define txPin 16              // software serial TX (to NodeMCU)
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);

#define DHTPIN 26             // DHT11 data
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_PIN A1           // soil-moisture sensor, analog output (GP27)
#define SOUND_PIN A2          // sound sensor, analog output (GP28)
#define PUMP_PIN 15           // pump control output
#define SOIL_THRESHOLD 45     // same threshold as the original sketch
#define SOUND_WINDOW_MS 50    // how long to sample the sound sensor
#define NOT_AVAILABLE -999.0

// Peak-to-peak amplitude (0-1023) of the sound sensor over a short window.
// A single analogRead() of a microphone signal says little, so we measure how
// far the signal swings while we listen.
int readSoundLevel()
{
  int lo = 1023;
  int hi = 0;
  unsigned long start = millis();
  while (millis() - start < SOUND_WINDOW_MS)
  {
    int v = analogRead(SOUND_PIN);
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  return hi - lo;
}

void setup()
{
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);     // link to the NodeMCU
  Serial.begin(9600);       // USB serial monitor
  analogReadResolution(10); // readings in the range 0-1023
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
}

void loop()
{
  // Soil moisture, inverted as in the original sketch
  int soil = 1023 - analogRead(SOIL_PIN);

  // Pump output: HIGH when the soil reading is 45 or below
  if (soil > SOIL_THRESHOLD)
  {
    digitalWrite(PUMP_PIN, 0);
  }
  else
  {
    digitalWrite(PUMP_PIN, 1);
  }

  int sound = readSoundLevel();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h)) h = NOT_AVAILABLE;
  if (isnan(t)) t = NOT_AVAILABLE;

  // Human-readable output on the USB serial monitor
  Serial.print(F("Soil_Moisture_Level: "));
  Serial.print(soil);
  Serial.print(F("  Sound_Level: "));
  Serial.print(sound);
  Serial.print(F("  Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.println(t);

  // Machine-readable line for the NodeMCU
  char msg[64];
  snprintf(msg, sizeof(msg), "S,%d,%.1f,%.1f,%d\n", soil, t, h, sound);
  mySerial.print(msg);

  delay(1000 - SOUND_WINDOW_MS);
}
