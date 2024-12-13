#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP_Now_Protocol\tests\test_1.0.0\include.h"

void callBack(uint8_t *, uint8_t, ESP_Now_Master *);

uint8_t mac[] = {0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x01};
ESP_Now_Master slave1(mac, callBack);

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  slave1.init();
}

void loop()
{
  slave1.run();
}

void callBack(uint8_t *, uint8_t, ESP_Now_Master *)
{
}