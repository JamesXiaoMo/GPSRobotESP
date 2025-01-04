#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --------------------- 以下全局参数 ---------------------
// BLE全局参数
#define SERVICE_UUID "66de1d0a-0dd9-44f0-8fa1-85f39668a27c"
#define CHARACTERISTIC_UUID "6f7e38d5-3acd-42f2-99aa-adf6a410ff92"
BLECharacteristic *pCharacteristic;
// WiFi全局参数
#define CONNECT_WIFI_TIMEOUT 10000
bool isGetWiFiData = false;
std::string WIFIDATA;
// 电机全局参数
int DCMOTOR_SPEED, StepperMotor_SPEED;
// WS2812全局参数
CRGB leds[1];
WiFiServer server(7769);
// -------------------------------------------------------

// --------------------- 以下全局函数 ---------------------
void TCPTask(void *pvParameters);

void BLETask(void *pvParameters);

void WiFiConnect(){
  isGetWiFiData = false;
  std::string SSID, PWD;
  size_t delimiterPos = WIFIDATA.find('|');
  if (delimiterPos != std::string::npos){
    SSID = WIFIDATA.substr(0, delimiterPos);
    PWD = WIFIDATA.substr(delimiterPos + 1);
  }
  else{
    Serial.println("Invalid WiFiData format!");
    return;
  }
  Serial.print("SSID: ");
  Serial.println(SSID.c_str());
  Serial.print("Password: ");
  Serial.println(PWD.c_str());
  WiFi.begin(SSID.c_str(), PWD.c_str());
  // 连接 Wi-Fi
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < CONNECT_WIFI_TIMEOUT){
    delay(1000);
    Serial.print("Connecting to Wi-Fi");
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    // 开启TCP服务
    server.begin();
    Serial.print("TCP server started on ");
    Serial.println("7769");
  }
  else{
    Serial.println("\nFailed to connect to Wi-Fi");
  }
}

class MyCallbacks : public BLECharacteristicCallbacks
{
  std::string rxValueAll;
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.find("^") != std::string::npos){
      WIFIDATA.clear();
      rxValueAll.clear();
      // Serial.println("Start");
    }
    else if (rxValue.find("$") != std::string::npos){
      WIFIDATA = rxValueAll;
      rxValueAll.clear();
      isGetWiFiData = true;
      //Serial.println("End");
    }
    else{
      rxValueAll += rxValue;
    }
    // Serial.println("Received data via BLE:");
    // Serial.println(rxValue.c_str());
  }
};

class MyServerCallbacks : public BLEServerCallbacks{
  void onConnect(BLEServer *pServer){
    Serial.println("BLE Device connected");
  }
  void onDisconnect(BLEServer *pServer){
    Serial.println("BLE Device disconnected");
  }
};
// -------------------------------------------------------
 // 初始化BLE
void BLETask(void *pvParameters){
  Serial.println("BLE Task running on core " + String(xPortGetCoreID()));
  BLEDevice::init("ESP32S3_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE is ready to receive data");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  vTaskDelete(NULL);
}

void TCPTask(void *pvParameters){
  Serial.println("WiFi Task running on core " + String(xPortGetCoreID()));
  while (true){
    if (isGetWiFiData){
    WiFiConnect();
    }
    while (WiFi.status() == WL_CONNECTED){
      WiFiClient client = server.available();
      if (client){
        client.setNoDelay(true);
        Serial.println("New client connected");
        while (client.connected()){
          if (client.available()){
            String data = client.readStringUntil('\n');
            if (data == "Connect"){
              client.println("ConnectOK");
              leds[0] = CRGB::Green;
              FastLED.show();
            }else if (data == "RSSI"){
              client.println("+" + String(WiFi.RSSI()));
            }else if (data.charAt(0) == '@'){
              size_t numOfPos = 2;
              int pos[numOfPos] = {-1, -1};
              for (int i = 0; i < data.length(); i++){
                if (data.charAt(i) == '|'){
                  for (int j = 0; j < numOfPos; j++){
                    if (pos[j] == -1){
                      pos[j] = i;
                      break;
                    }
                  }
                }
              }
              DCMOTOR_SPEED = data.substring(1, pos[0]).toInt();
              StepperMotor_SPEED = data.substring(pos[0] + 1, pos[1]).toInt();
              Serial.println("DCMOTOR_SPEED: " + String(DCMOTOR_SPEED));
              Serial.println("StepperMotor_SPEED: " + String(StepperMotor_SPEED));
            }
          }
        }
      }else{
        leds[0] = CRGB::Yellow;
        FastLED.show();
      }
    }
    Serial.println("Waiting for WiFi connection...");
    delay(1000);
  }
}

void setup(){
  Serial.begin(115200);
  // 初始化WS2812
  FastLED.addLeds<WS2812, 48, GRB>(leds, 1);
  FastLED.setBrightness(100);
  leds[0] = CRGB::Red;
  FastLED.show();

  xTaskCreatePinnedToCore(
    BLETask,
    "BLETask",
    4096,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    TCPTask,
    "TCPTask",
    8192,
    NULL,
    1,
    NULL,
    1
  );
}

void loop(){
  delay(1000);
}