#include "C:\Users\AVG\Documents\Electrical_Main\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP_Now_Protocol\ESP_Now_Protocol_1.0.1\ESP_Now_Protocol_Master.h"

void callBack(uint8_t *, uint8_t, ESP_Now_Master_SubClass *);

uint8_t mac[] = {0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x01};

void setup()
{
  Serial.begin(115200);
  ESP_Now_Master.addSlave(mac, callBack);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  ESP_Now_Master.init();
}

void loop()
{
  ESP_Now_Master.run();

  {
    static bool connected = true;
    if (connected != ESP_Now_Master.getSlave(0)->isConnected())
    {
      connected = !connected;
      Serial.println(connected ? "connected" : "disconnected");
    }
  }
  if (Serial.available())
  {
    String s = Serial.readString();
    s = s.substring(0, s.length() - 1);
    // has a slight delay but its fine
    if (s.length() <= 100)
    {
      Serial.print("TX to (");
      for (uint8_t x = 0; x < 6; x++)
      {
        Serial.print("0x");
        Serial.print(mac[x], HEX);

        if (x < 6 - 1)
          Serial.print(",");
      }
      Serial.print("): \"");

      // for (uint8_t x = 0; x < s.length() - 1; x++)
      // {
      //   Serial.print("0x");
      //   Serial.print(s[x], HEX);

      //   if (x < s.length() - 1)
      //     Serial.print(", ");
      // }
      // Serial.print("} \"");
      Serial.print(s);
      Serial.println("\"");

      uint8_t data[s.length()];
      for (uint8_t x = 0; x < s.length(); x++)
      {
        data[x] = s[x];
      }

      ESP_Now_Master.getSlave(0)->sendPersistentPacket(data, sizeof(data), 0 /*data bucket*/, 0 /*timeout (ms)*/);
    }
    else
    {
      Serial.println("ERROR: msg to long");
    }
  }
}

void callBack(uint8_t *data, uint8_t len, ESP_Now_Master_SubClass *pnt)
{
  Serial.print("RX from (");

  uint8_t getMac[6];
  pnt->getRemoteMac(getMac);

  for (uint8_t x = 0; x < 6; x++)
  {
    Serial.print("0x");
    Serial.print(getMac[x], HEX);

    if (x < 6 - 1)
      Serial.print(", ");
  }
  Serial.print("): \"");
  for (uint8_t x = 0; x < len; x++)
  {
    Serial.print((char)data[x]);
  }
  Serial.println("\"");
}