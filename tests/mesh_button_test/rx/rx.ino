#include <ESP8266WiFi.h>
#include <espnow.h>

#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP_BUTTON_INTERFACE\EspButtonInterface_v1.1.0.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\InputMacro\InputMacro_1.0.1.h"

#define BUTTON_PIN 0 // builtin button
#define LED_PIN LED_BUILTIN

uint8_t new_mac[] = {0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x02};

uint8_t master_mac[6] = {};
bool masterFound = false;

EspPinDriver buttonPin(BUTTON_PIN);
InputMacro buttonMacro(true);

bool dpid_button_value = false;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0)
  {
    Serial.println("Delivery success");
  }
  else
  {
    Serial.println("Delivery fail");
  }
}

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
  if (!masterFound)
  {
    Serial.println("master found");
    masterFound = true;
    for (uint8_t x = 0; x < 6; x++)
      master_mac[x] = mac[x];
  }
  Serial.print("Bytes received {");
  for (uint8_t x = 0; x < 6; x++)
  {
    Serial.print(mac[x], HEX);
    if (x < 6 - 1)
      Serial.print(':');
  }
  Serial.print("}: ");

  for (uint8_t x = 0; x < len; x++)
  {
    Serial.print(incomingData[x], HEX);
    if (x < 6 - 1)
      Serial.print(' ');
  }
  Serial.println();
  Serial.println();

  // set led value:
  dpid_button_value = incomingData[0];
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("Old MAC Address:  ");
  Serial.println(WiFi.macAddress());
  wifi_set_macaddr(STATION_IF, &new_mac[0]);

  Serial.print("New MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != 0 || esp_now_set_self_role(ESP_NOW_ROLE_COMBO) != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  // esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  delay(10);
  digitalWrite(LED_PIN, !dpid_button_value);
  pinMode(LED_PIN, OUTPUT);

  if (buttonMacro(buttonPin.run()))
  {
    if (!buttonMacro) // << state change false
    {
      dpid_button_value = !dpid_button_value;
      if (masterFound)
      {
        esp_now_send(master_mac, (uint8_t *)&dpid_button_value, 1);
        Serial.println("sending data");
      }
    }
  }
  // static uint32_t tim = millis();
  // if (millis() - tim >= 1000)
  // {
  //   tim = millis();
  //   Serial.println("sending...");
  //   char c[] = "test";
  //   esp_now_send(broadcastAddress, (uint8_t*)c, sizeof(c));
  // }
}
