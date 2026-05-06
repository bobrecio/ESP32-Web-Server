#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define RGB_PIN 48
#define NUM_LEDS 1

Adafruit_NeoPixel rgb(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

const char *SETTINGS_FILE = "/settings.json";

String deviceName = "ESP32-S3 Web Server";
bool statusLedEnabled = true;

int redVal = 0;
int greenVal = 255;
int blueVal = 0;
int brightnessVal = 25;

void sendJson(JsonDocument &doc, int statusCode = 200)
{
  String response;
  serializeJson(doc, response);
  server.send(statusCode, "application/json", response);
}

String formatBytes(size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  if (bytes < 1024 * 1024)
    return String(bytes / 1024.0, 2) + " KB";
  return String(bytes / 1024.0 / 1024.0, 2) + " MB";
}

String contentTypeForPath(const String &path)
{
  if (path.endsWith(".html"))
    return "text/html";
  if (path.endsWith(".css"))
    return "text/css";
  if (path.endsWith(".js"))
    return "application/javascript";
  if (path.endsWith(".json"))
    return "application/json";
  if (path.endsWith(".png"))
    return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
    return "image/jpeg";
  if (path.endsWith(".svg"))
    return "image/svg+xml";
  if (path.endsWith(".ico"))
    return "image/x-icon";
  return "text/plain";
}

void saveSettings()
{
  StaticJsonDocument<256> doc;

  doc["deviceName"] = deviceName;
  doc["statusLedEnabled"] = statusLedEnabled;
  doc["r"] = redVal;
  doc["g"] = greenVal;
  doc["b"] = blueVal;
  doc["brightness"] = brightnessVal;

  File file = LittleFS.open(SETTINGS_FILE, "w");

  if (!file)
  {
    Serial.println("Failed to open settings file for writing");
    return;
  }

  serializeJsonPretty(doc, file);
  file.close();

  Serial.println("Settings saved");
}

void loadSettings()
{
  if (!LittleFS.exists(SETTINGS_FILE))
  {
    Serial.println("No settings file found. Using defaults.");
    saveSettings();
    return;
  }

  File file = LittleFS.open(SETTINGS_FILE, "r");

  if (!file)
  {
    Serial.println("Failed to open settings file");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.println("Failed to parse settings file. Using defaults.");
    return;
  }

  deviceName = doc["deviceName"] | "ESP32-S3 Web Server";
  statusLedEnabled = doc["statusLedEnabled"] | true;
  redVal = doc["r"] | 0;
  greenVal = doc["g"] | 255;
  blueVal = doc["b"] | 0;
  brightnessVal = doc["brightness"] | 25;

  Serial.println("Settings loaded");
}

void handleSettings()
{
  StaticJsonDocument<256> doc;

  doc["deviceName"] = deviceName;
  doc["statusLedEnabled"] = statusLedEnabled;
  doc["r"] = redVal;
  doc["g"] = greenVal;
  doc["b"] = blueVal;
  doc["brightness"] = brightnessVal;

  sendJson(doc);
}

void setRgb(int r, int g, int b, int brightness, bool persist = false)
{
  redVal = constrain(r, 0, 255);
  greenVal = constrain(g, 0, 255);
  blueVal = constrain(b, 0, 255);
  brightnessVal = constrain(brightness, 0, 255);

  if (statusLedEnabled)
  {
    rgb.setBrightness(brightnessVal);
    rgb.setPixelColor(0, rgb.Color(redVal, greenVal, blueVal));
  }
  else
  {
    rgb.clear();
  }

  rgb.show();

  Serial.printf("RGB set to R:%d G:%d B:%d Brightness:%d\n",
                redVal, greenVal, blueVal, brightnessVal);

  if (persist)
  {
    saveSettings();
  }
}

void statusBooting()
{
  if (statusLedEnabled)
    setRgb(0, 0, 255, 25, false);
}

void statusReady()
{
  if (statusLedEnabled)
    setRgb(redVal, greenVal, blueVal, brightnessVal, false);
}

void statusError()
{
  if (statusLedEnabled)
    setRgb(255, 0, 0, 25, false);
}

void handleColor()
{
  StaticJsonDocument<256> doc;

  int r = server.hasArg("r") ? server.arg("r").toInt() : redVal;
  int g = server.hasArg("g") ? server.arg("g").toInt() : greenVal;
  int b = server.hasArg("b") ? server.arg("b").toInt() : blueVal;
  int brightness = server.hasArg("brightness") ? server.arg("brightness").toInt() : brightnessVal;

  setRgb(r, g, b, brightness, true);

  doc["r"] = redVal;
  doc["g"] = greenVal;
  doc["b"] = blueVal;
  doc["brightness"] = brightnessVal;

  sendJson(doc);
}

void handleStatus()
{
  StaticJsonDocument<256> doc;

  doc["ip"] = WiFi.localIP().toString();
  doc["r"] = redVal;
  doc["g"] = greenVal;
  doc["b"] = blueVal;
  doc["brightness"] = brightnessVal;
  doc["uptimeMs"] = millis();
  doc["rssi"] = WiFi.RSSI();

  sendJson(doc);
}

void handleInfo()
{
  StaticJsonDocument<512> doc;

  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();

  doc["chipModel"] = ESP.getChipModel();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["sketchSize"] = ESP.getSketchSize();
  doc["freeSketchSpace"] = ESP.getFreeSketchSpace();
  doc["littleFsTotal"] = total;
  doc["littleFsUsed"] = used;
  doc["littleFsFree"] = total - used;
  doc["uptimeMs"] = millis();
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();

  sendJson(doc);
}

void handleFiles()
{
  StaticJsonDocument<1024> doc;
  JsonArray files = doc.to<JsonArray>();

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    JsonObject item = files.createNestedObject();

    item["name"] = file.name();
    item["size"] = file.size();
    item["sizeFormatted"] = formatBytes(file.size());

    file = root.openNextFile();
  }

  sendJson(doc);
}

void handleReboot()
{
  server.send(200, "application/json", "{\"message\":\"Rebooting...\"}");
  delay(500);
  ESP.restart();
}

void handleFileRequest()
{
  String path = server.uri();
  if (path == "/")
  {
    path = "/index.html";
  }

  File file = LittleFS.open(path, "r");

  if (!file)
  {
    server.send(404, "text/plain", "File Not Found");
    return;
  }

  server.streamFile(file, contentTypeForPath(path));
  file.close();
}

void handleUpdateSettings()
{
  if (server.hasArg("deviceName"))
  {
    deviceName = server.arg("deviceName");
  }

  if (server.hasArg("statusLedEnabled"))
  {
    String value = server.arg("statusLedEnabled");
    statusLedEnabled = value == "true" || value == "1" || value == "on";
  }

  saveSettings();
  statusReady();

  StaticJsonDocument<256> doc;
  doc["message"] = "Settings updated";
  doc["deviceName"] = deviceName;
  doc["statusLedEnabled"] = statusLedEnabled;

  sendJson(doc);
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting ESP32-S3 Web Server...");

  rgb.begin();
  rgb.clear();
  rgb.show();
  statusBooting();

  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");
    statusError();
  }
  else
  {
    Serial.println("LittleFS mounted");
    loadSettings();
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. Open: http://");
  Serial.println(WiFi.localIP());

  server.on("/api/color", handleColor);
  server.on("/api/status", handleStatus);
  server.on("/api/info", handleInfo);
  server.on("/api/files", handleFiles);
  server.on("/api/reboot", handleReboot);
  server.on("/api/settings", HTTP_GET, handleSettings);
  server.on("/api/settings", HTTP_POST, handleUpdateSettings);
  server.onNotFound(handleFileRequest);

  server.begin();
  Serial.println("Web server started.");
  statusReady();
}

void loop()
{
  server.handleClient();
}
