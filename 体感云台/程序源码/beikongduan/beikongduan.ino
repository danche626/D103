#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

Servo servoX;
Servo servoY;

int lastX = 90;
int lastY = 90;

const int MIN_ANGLE = 10;
const int MAX_ANGLE = 170;
const int ANGLE_TOLERANCE = 2;

typedef struct struct_message {
  int x;
  int y;
} struct_message;

struct_message incomingData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  int newX = constrain(incomingData.x, MIN_ANGLE, MAX_ANGLE);
  int newY = constrain(incomingData.y, MIN_ANGLE, MAX_ANGLE);

  Serial.print("Received X: ");
  Serial.print(newX);
  Serial.print(" | Y: ");
  Serial.println(newY);

  if (abs(newX - lastX) > ANGLE_TOLERANCE) {
    servoX.write(newX);
    lastX = newX;
  }

  if (abs(newY - lastY) > ANGLE_TOLERANCE) {
    servoY.write(newY);
    lastY = newY;
  }
}

void setup() {
  Serial.begin(115200);

  servoX.attach(13);  // 舵机1控制引脚
  servoY.attach(12);  // 舵机2控制引脚

  servoX.write(90);   // 初始化到中间位置
  servoY.write(90);
  delay(500);         // 延时给舵机转动时间

  WiFi.mode(WIFI_STA);
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW 初始化失败");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // 空循环，任务由回调函数处理
}
