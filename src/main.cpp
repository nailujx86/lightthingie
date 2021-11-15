#include <Arduino.h>
#include "fauxmoESP.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <WiFiUdp.h>
#include <LittleFS.h>

#include "wifi_secrets.h"
#include "otahandler.h"

#define STRIP0 D1
#define STRIP1 D2
#define DIMPIN D7
#define PWMPIN D5

AsyncWebServer server(80);
fauxmoESP fauxmo;
ADC_MODE(ADC_VCC);

int dimcounter = 0;
int lightmode = 0;
int lightfun = 0;
int dimval = 512;
int dimdir = 1;
int speed = 1;
void ICACHE_RAM_ATTR pwmTick();
String templateProcessor(const String &var);

void setup()
{
  Serial.begin(115200);
  Serial.println(F("lightthingie booting.."));
  WiFi.mode(WIFI_STA);
  WiFi.hostname("Lightthingie");
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println(F("Connection Failed! Please check your WiFi Settings.\nRebooting now!"));
    delay(500);
    ESP.restart();
  }
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("lightthingie"))
  {
    Serial.println(F("MDNS set up!"));
  }

  setupOTA(PWMPIN);
  startOTA();
  LittleFS.begin();

  server.on("/", [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/web/index.html", String(), false, templateProcessor);
  });
  server.on("/settings", [](AsyncWebServerRequest *request) {
    if (request->hasParam("brightness", true))
    {
      dimval = request->getParam("brightness", true)->value().toInt();
      analogWrite(DIMPIN, dimval);
    }
    if (request->hasParam("speed", true))
    {
      speed = request->getParam("speed", true)->value().toInt();
    }
    if (request->hasParam("animated", true))
    {
      if (request->getParam("animated", true)->value() == "on")
      {
        lightfun = 1;
      }
    }
    else
    {
      lightfun = 0;
    }
    if (request->hasParam("mode", true))
    {
      lightmode = request->getParam("mode", true)->value().toInt();
    }
    request->redirect("/");
  });
  server.serveStatic("/font", LittleFS, "/web/font").setCacheControl("max-age=900");
  server.serveStatic("/", LittleFS, "/web");
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data)))
      return;
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body))
      return;
    request->send(404, "text/plain", "Not found");
  });

  fauxmo.createServer(false);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  fauxmo.addDevice("Lichterkette");

  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    int brightness = value;
    if (state == false) brightness = 0;
    dimval = brightness * 4;
    analogWrite(DIMPIN, brightness * 4);
  });
  /*espalexa.addDevice(
      "Lichterkette", [](uint8_t brightness) {
        dimval = brightness * 4;
        analogWrite(DIMPIN, brightness * 4);
      },
      128);*/
  /*espalexa.addDevice("Lichterkette - Modus", [](uint8_t brightness) {
    if (!brightness)
    {
      lightfun = 0;
      lightmode = 0;
    }
    else if (brightness < 127)
    {
      lightmode = 0;
      lightfun = 1;
    }
    else
    {
      lightmode = 1;
      lightfun = 1;
    }
  });
  espalexa.addDevice("Lichterkette - Speed", [](uint8_t brightness) {
    speed = espalexa.toPercent(brightness) / 5;
  });*/
  server.begin();

  /// OUTPUT SETUP
  analogWriteFreq(1000);
  pinMode(STRIP0, OUTPUT);
  pinMode(STRIP1, OUTPUT);
  dimcounter = 0;
  delay(10);
  analogWrite(DIMPIN, dimval);
  analogWrite(PWMPIN, 511);
  attachInterrupt(PWMPIN, &pwmTick, CHANGE);
}

void pwmTick()
{
  if (dimcounter == 1)
  {
    digitalWrite(STRIP0, 1);
    digitalWrite(STRIP1, 0);
    dimcounter++;
  }
  else if (dimcounter == 2)
  {
    digitalWrite(STRIP0, 0);
    digitalWrite(STRIP1, 1);
    dimcounter = lightmode;
  }
  else
  {
    digitalWrite(STRIP0, 0);
    digitalWrite(STRIP1, 0);
    dimcounter++;
  }
}

void loop()
{
  if (dimval == 0)
  {
    delay(100);
  }
  else
  {
    delay(10);
  }
  if (lightfun)
  {
    if (dimval > 1000)
    {
      dimdir = 0;
    }
    else if (dimval < 400)
    {
      dimdir = 1;
    }
    if (dimdir)
    {
      dimval += speed;
    }
    else
    {
      dimval -= speed;
    }
    analogWrite(DIMPIN, dimval);
  }
  fauxmo.handle();
  ArduinoOTA.handle();
}

String templateProcessor(const String &var)
{
  FSInfo fs_info;
  LittleFS.info(fs_info);
  if (var == "CPUFREQ")
  {
    return String(ESP.getCpuFreqMHz()) + "MHz";
  }
  else if (var == "FREEHEAP")
  {
    return String(ESP.getFreeHeap()) + "B";
  }
  else if (var == "VCC")
  {
    return String((float)ESP.getVcc() / 1000, 3) + "V";
  }
  else if (var == "RESETINFO")
  {
    return String(ESP.getResetInfo());
  }
  else if (var == "RESETREASON")
  {
    return String(ESP.getResetReason());
  }
  else if (var == "FLASHSIZE")
  {
    return String(ESP.getFlashChipSize()) + "B";
  }
  else if (var == "FLASHREALSIZE")
  {
    return String(ESP.getFlashChipRealSize()) + "B";
  }
  else if (var == "FREESKETCHSPACE")
  {
    return String(ESP.getFreeSketchSpace()) + "B";
  }
  else if (var == "SKETCHSIZE")
  {
    return String(ESP.getSketchSize()) + "B";
  }
  else if (var == "SKETCHMD5")
  {
    return String(ESP.getSketchMD5());
  }
  else if (var == "FSTOTAL")
  {
    return String(fs_info.totalBytes) + "B";
  }
  else if (var == "FSUSED")
  {
    return String(fs_info.usedBytes) + "B";
  }
  else if (var == "DIMVAL")
  {
    return String(dimval);
  }
  else if (var == "LIGHTFUN")
  {
    return String(lightfun ? "checked" : "");
  }
  else if (var == "LIGHTMODE")
  {
    return String(lightmode);
  }
  else if (var == "SPEED")
  {
    return String(speed);
  }
  return "";
}