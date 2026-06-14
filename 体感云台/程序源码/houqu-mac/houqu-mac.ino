#include <WiFi.h>

void setup() {
  Serial.begin(115200);

  // 关键一步：初始化为 WiFi STA 模式
  WiFi.mode(WIFI_STA);

  delay(100);  // 给 WiFi 子系统一点时间

  String mac = WiFi.macAddress();  // 这时才能正确获取
  Serial.print("uint8_t receiverMAC[] = {");

  for (int i = 0; i < 17; i += 3) {
    Serial.print("0x");
    Serial.print(mac[i]);
    Serial.print(mac[i+1]);
    if (i < 15) Serial.print(", ");
  }

  Serial.println("};");
}

void loop() {
  // Nothing
}
