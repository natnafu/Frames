#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <math.h>
#include <Ticker.h>
#include <SPI.h>
#include <WiFiUdp.h>

#include "creds.h"

// creds.h holds wifi creds and BLYNK token, add to gitignore
const char* ssid = STASSID;
const char* password = STAPSK;

// ESP32 things
#define PIN D2
#define NUM_PIXELS 200

// Blynk things
#define BLYNK_PRINT Serial

// LED things
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

Ticker blinker;
volatile uint32_t timer = 0;

double brightness = 1.0;

struct color {
  double speed;
  int waveln;
  int phase;
  int pwr;
};

color red = {0.04096, 4,  0, 0};
color grn = {0.0512,  9,  0, 0};
color blu = {0.0512,  1,  0, 0};

// sets red speed
BLYNK_WRITE(V0) {
  red.speed = (param[0].asDouble() * 256.0) / 500.0;
}
// sets green speed
BLYNK_WRITE(V1) {
  grn.speed = (param[0].asDouble() * 256.0) / 500.0;
}
// sets blue speed
BLYNK_WRITE(V2) {
  blu.speed = (param[0].asDouble() * 256.0) / 500.0;
}

// sets red wave length
BLYNK_WRITE(V3) {
  red.waveln = 256.0 / param[0].asInt();
}
// sets green frequency
BLYNK_WRITE(V4) {
  grn.waveln = 256.0 / param[0].asInt();
}
// sets blue frequency
BLYNK_WRITE(V5) {
  blu.waveln = 256.0 / param[0].asInt();
}

// sets red power
BLYNK_WRITE(V6) {
  red.pwr = param[0].asInt();
}
// sets green power
BLYNK_WRITE(V7) {
  grn.pwr = param[0].asInt();
}
// sets blue power
BLYNK_WRITE(V8) {
  blu.pwr = param[0].asInt();
}

// Sync devices
BLYNK_WRITE(V9) {
  if (param[0].asInt()) timer = 0;
}

// Set brightness
BLYNK_WRITE(V10) {
  brightness = param[0].asDouble();
}

// Increases timer by 1ms (assuming 1ms ticker)
void ms_step() {
  timer++;
}

// 8bit sine wave approx
byte cos8(int x) {
  return (cos((x/127.5) * M_PI) * 127.5) + 127.5;
}

void setup() {
  Serial.begin(115200);

  // LEDS
  pixels.begin();
  pixels.show();

  // OTA things
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Blynk things
  Blynk.begin(TOKEN, STASSID, STAPSK);
  blinker.attach_ms(1, ms_step);
}

void loop() {
  ArduinoOTA.handle();
  uint32_t t = timer;
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint32_t r = brightness * (red.pwr * cos8((uint32_t ((red.speed * t) + (i*red.waveln))) % 256));
    uint32_t g = brightness * (grn.pwr * cos8((uint32_t ((grn.speed * t) + (i*grn.waveln))) % 256));
    uint32_t b = brightness * (blu.pwr * cos8((uint32_t ((blu.speed * t) + (i*blu.waveln))) % 256));
    pixels.setPixelColor(i, pixels.Color(r,g,b));
  }
  pixels.show();
  Blynk.run();
}
