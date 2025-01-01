#include <Arduino.h>
#include <WiFi.h>

int dc_motor_speed = 0;
int stepping_motor_speed = 0;

const char* ssid = "Jameswu_2.4G";
const char* pwd = "20030521";

WiFiServer server(7769);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.print("TCP server started on ");
  Serial.println("7769");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
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
  }
}