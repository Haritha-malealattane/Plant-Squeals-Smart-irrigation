/*
 * Smart irrigation: soil-moisture based pump control with DHT11 readings
 * Board: Raspberry Pi Pico / Pico W (Arduino IDE with the RP2040 core)
 *
 * Source: the code listing in Chapter 5 of the project report.
 * Only comments, spacing and the two lines noted below were changed.
 * (In the report's text, the "#define DHTTYPE" line and the "DHT dht(...)"
 * line had been merged onto one line; they are separated again here.)
 *
 * What it does every second:
 *   1. Reads the analog soil-moisture sensor and inverts the value (1023 - raw).
 *   2. Sends the soil reading to the NodeMCU over software serial, which uploads
 *      it to ThingSpeak.
 *   3. Drives the pump output pin: HIGH when the soil reading is 45 or below,
 *      LOW when it is above 45.
 *   4. Reads humidity and temperature from the DHT11 and prints them to the
 *      USB serial monitor.
 */
#include <SoftwareSerial.h>

#define rxPin 17
#define txPin 16
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);

#include "DHT.h"

#define DHTPIN 26
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);   // link to the NodeMCU
  Serial.begin(9600);     // USB serial monitor
  dht.begin();
  pinMode(15, OUTPUT);    // pump control output
}

void loop()
{
  // Soil moisture (analog), inverted so the value moves the other way
  int soil = analogRead(A1);
  soil = 1023 - soil;
  Serial.print("Soil_Moisture_Level: ");
  Serial.print(soil);
  Serial.println("");
  mySerial.print("Soil_Moisture_Level: ");
  mySerial.print(soil);

  // Pump control: output HIGH when the soil reading is 45 or below
  if (soil > 45)
  {
    digitalWrite(15, 0);
  }
  else
  {
    digitalWrite(15, 1);
  }

  // Temperature and humidity
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("% Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.println("");
  Serial.println("*********************************");
  delay(1000);
}
