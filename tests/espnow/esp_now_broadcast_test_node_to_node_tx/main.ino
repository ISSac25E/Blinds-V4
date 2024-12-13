#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x42, 0x4C, 0x4E, 0x44, 0xBE, 0xEF};

// Variable to store if sending data was successful
String success;

// Callback when data is sent
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

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
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
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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
  static uint32_t tim = millis();
  if (millis() - tim >= 1000)
  {
    tim = millis();
    Serial.println("sending...");
    char c[] = "test";
    esp_now_send(broadcastAddress, (uint8_t*)c, sizeof(c));
  }
}
