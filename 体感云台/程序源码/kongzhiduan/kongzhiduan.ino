#include <esp_now.h>
#include <WiFi.h>
#include <MPU6050_tockn.h>
#include <Wire.h>

MPU6050 mpu(Wire);

// 发送的数据结构
typedef struct struct_message {
  int xAngle;
  int yAngle;
} struct_message;

struct_message sendData;

// 接收端 MAC 地址（替换为你的实际地址）
uint8_t receiverMAC[] = {0xC8, 0x2E, 0x18, 0xF7, 0x3E, 0xB0};

// 初始偏移
float xOffset = 0, yOffset = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets(true);
  Serial.println("MPU6050 ready");

  WiFi.mode(WIFI_STA);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Add Peer Failed");
      return;
    }
  }
}

void loop() {
  mpu.update();

  float x = mpu.getAngleX() - xOffset;
  float y = mpu.getAngleY() - yOffset;

  sendData.xAngle = constrain(map(x, -45, 45, 0, 180), 0, 180);
  sendData.yAngle = constrain(map(y, -45, 45, 0, 180), 0, 180);

  Serial.print("Sending X: ");
  Serial.print(sendData.xAngle);
  Serial.print(" | Y: ");
  Serial.println(sendData.yAngle);

  esp_now_send(receiverMAC, (uint8_t*)&sendData, sizeof(sendData));

  delay(50);
}
