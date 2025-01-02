#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "66de1d0a-0dd9-44f0-8fa1-85f39668a27c"
#define CHARACTERISTIC_UUID "6f7e38d5-3acd-42f2-99aa-adf6a410ff92"

#define CONNECT_TIMEOUT 10000

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

bool isGetWiFiData = false;
std::string WIFIDATA;
const unsigned long connectTimeout = 10000;

CRGB leds[1];

WiFiServer server(7769);

void WiFiConnect()
{
  Serial.println(WIFIDATA.c_str());
  isGetWiFiData = false;
  std::string SSID, PWD;
  size_t delimiterPos = WIFIDATA.find('|');
  if (delimiterPos != std::string::npos)
  {
    SSID = WIFIDATA.substr(0, delimiterPos);
    PWD = WIFIDATA.substr(delimiterPos + 1);
  }
  else
  {
    Serial.println("Invalid WiFiData format!");
    return;
  }
  Serial.print("SSID: ");
  Serial.println(SSID.c_str());
  Serial.print("Password: ");
  Serial.println(PWD.c_str());

  // 开始连接 Wi-Fi
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < CONNECT_TIMEOUT)
  {
    WiFi.begin(SSID.c_str(), PWD.c_str());
    delay(1000);
    Serial.print("Connecting to Wi-Fi...");
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.print("TCP server started on ");
    Serial.println("7769");
  }
  else
  {
    Serial.println("\nFailed to connect to Wi-Fi");
  }
}

class MyCallbacks : public BLECharacteristicCallbacks
{
  std::string rxValueAll;
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.find("^") != std::string::npos)
    {
      WIFIDATA.clear();
      rxValueAll.clear();
      Serial.println("Start");
    }
    else if (rxValue.find("$") != std::string::npos)
    {
      WIFIDATA = rxValueAll;
      rxValueAll.clear();
      isGetWiFiData = true;
      Serial.println("End");
    }
    else
    {
      rxValueAll += rxValue;
    }
    Serial.println("Received data via BLE:");
    Serial.println(rxValue.c_str());
  }
};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

void setup()
{
  Serial.begin(115200);
  BLEDevice::init("ESP32S3_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  // 设置回调
  pCharacteristic->setCallbacks(new MyCallbacks());

  // 添加描述
  pCharacteristic->addDescriptor(new BLE2902());

  // 开启服务
  pService->start();

  // 开启广播
  pServer->getAdvertising()->start();
  Serial.println("BLE is ready to receive data");

  FastLED.addLeds<WS2812, 48, GRB>(leds, 1);
  FastLED.setBrightness(100);

  leds[0] = CRGB::Red;
  FastLED.show();
}

void loop()
{
  if (isGetWiFiData)
  {
    WiFiConnect();
  }
  if (WiFi.status() == WL_CONNECTED)
  {

    while (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient client = server.available();
      if (client)
      {
        Serial.println("New client connected");
        while (client.connected())
        {
          if (client.available())
          {
            String data = client.readStringUntil('\n'); // 读取一行数据
            // Serial.println("<<< " + data);
            if (data == "Connect")
            {
              client.println("ConnectOK");
              leds[0] = CRGB::Green;
              FastLED.show();
              // Serial.println(">>> ConnectOK");
            }
            else if (data == "RSSI")
            {
              client.println("+" + String(WiFi.RSSI()));
            }
          }
        }
      }
      else
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          leds[0] = CRGB::Yellow;
          FastLED.show();
        }
        else
        {
          leds[0] = CRGB::Red;
          FastLED.show();
        }
      }
    }
  }
  else
  {
    Serial.println("Waiting for WiFi connection...");
    delay(1000);
  }
}