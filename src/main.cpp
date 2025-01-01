#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID        "204a2353-1b42-4aee-ba54-140ba9f4cde9"
#define CHARACTERISTIC_UUID "98303f4b-96ae-4b27-869d-19ac4443c63f"

CRGB leds[1];

int dc_motor_speed = 0;
int stepping_motor_speed = 0;

const char* ssid = "Jameswu_2.4G";
const char* pwd = "20030521";

WiFiServer server(7769);

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812, 8, GRB>(leds, 1);
  FastLED.setBrightness(100);
  leds[0] = CRGB::Red;
  FastLED.show();

  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  leds[0] = CRGB::Yellow;
  FastLED.show();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.print("TCP server started on ");
  Serial.println("7769");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    leds[0] = CRGB::Green;
    FastLED.show();
    Serial.println("New client connected " + client.remoteIP().toString() + ":" + client.remotePort());
    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');  // 读取一行数据
        //Serial.println("<<< " + data);
        if (data == "Connect"){
          client.println("ConnectOK");
          //Serial.println(">>> ConnectOK");
        }else if (data == "RSSI"){
          client.println("+" + String(WiFi.RSSI()));
        }
      }
    }
  }else{
    leds[0] = CRGB::Orange;
    FastLED.show();
  }
}