/*
 * NodeMCU (ESP8266) -> ThingSpeak uploader
 *
 * ADDED AFTER THE ORIGINAL SUBMISSION. The report describes the NodeMCU uploading
 * sensor data to ThingSpeak but does not include its code.
 *
 * Reads the line sent by pico_sensor_node.ino:
 *     S,<soil>,<temperature>,<humidity>,<sound>\n
 * and uploads the latest values every 20 seconds (a free ThingSpeak account
 * allows one update per 15 seconds).
 *
 * ThingSpeak channel fields (same order as the dashboard in the report):
 *     field1 = temperature, field2 = humidity, field3 = sound, field4 = soil moisture
 *
 * Not yet run on the prototype hardware: check the pins in docs/wiring.md first.
 */
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "secrets.h"            // copy secrets.example.h to secrets.h and edit it

#define PICO_RX_PIN D5          // NodeMCU RX  <- Pico TX (GP16)
#define PICO_TX_PIN D6          // NodeMCU TX  -> Pico RX (GP17), not used yet
#define UPLOAD_INTERVAL_MS 20000UL
#define MISSING_LIMIT -900.0    // anything below this means "reading failed"

SoftwareSerial picoSerial(PICO_RX_PIN, PICO_TX_PIN);

int   soil  = -999;
float temp  = -999.0;
float hum   = -999.0;
int   sound = -999;
bool  haveData = false;
unsigned long lastUpload = 0;
char line[64];

// Parse "S,<soil>,<temperature>,<humidity>,<sound>". Returns false if malformed.
bool parseLine(char *s)
{
  if (s[0] != 'S' || s[1] != ',') return false;

  char *tok = strtok(s + 2, ",");
  if (tok == NULL) return false;
  int newSoil = atoi(tok);

  tok = strtok(NULL, ",");
  if (tok == NULL) return false;
  float newTemp = atof(tok);

  tok = strtok(NULL, ",");
  if (tok == NULL) return false;
  float newHum = atof(tok);

  tok = strtok(NULL, ",\r\n");
  if (tok == NULL) return false;
  int newSound = atoi(tok);

  soil = newSoil;
  temp = newTemp;
  hum = newHum;
  sound = newSound;
  haveData = true;
  return true;
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000UL)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println(" connected");
  else Serial.println(" failed, will retry");
}

// Send the latest values to ThingSpeak. Failed readings are left out of the update.
bool uploadToThingSpeak()
{
  WiFiClient client;
  if (!client.connect("api.thingspeak.com", 80))
  {
    Serial.println("ThingSpeak: connection failed");
    return false;
  }

  char path[200];
  int n = snprintf(path, sizeof(path), "/update?api_key=%s", THINGSPEAK_WRITE_KEY);
  if (temp > MISSING_LIMIT)  n += snprintf(path + n, sizeof(path) - n, "&field1=%.1f", temp);
  if (hum > MISSING_LIMIT)   n += snprintf(path + n, sizeof(path) - n, "&field2=%.1f", hum);
  if (sound > MISSING_LIMIT) n += snprintf(path + n, sizeof(path) - n, "&field3=%d", sound);
  if (soil > MISSING_LIMIT)  n += snprintf(path + n, sizeof(path) - n, "&field4=%d", soil);

  client.print("GET ");
  client.print(path);
  client.print(" HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\n\r\n");

  // Read the reply; the last line is the new entry number ("0" means the update failed)
  char reply[256];
  int len = 0;
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < 5000UL)
  {
    while (client.available() && len < (int)sizeof(reply) - 1)
    {
      reply[len++] = (char)client.read();
    }
    delay(10);
  }
  reply[len] = '\0';
  client.stop();

  char *lastLine = strrchr(reply, '\n');
  lastLine = (lastLine != NULL) ? lastLine + 1 : reply;
  Serial.print("ThingSpeak entry: ");
  Serial.println(lastLine);
  return atoi(lastLine) > 0;
}

void setup()
{
  Serial.begin(115200);
  picoSerial.begin(9600);
  picoSerial.setTimeout(200);
  connectWiFi();
}

void loop()
{
  // 1. Read any complete lines sent by the Pico
  while (picoSerial.available())
  {
    int len = picoSerial.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';
    if (!parseLine(line))
    {
      Serial.print("Ignored: ");
      Serial.println(line);
    }
  }

  // 2. Keep Wi-Fi connected
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  // 3. Upload the latest values on a fixed interval
  if (haveData && WiFi.status() == WL_CONNECTED && millis() - lastUpload >= UPLOAD_INTERVAL_MS)
  {
    lastUpload = millis();
    uploadToThingSpeak();
  }
}
