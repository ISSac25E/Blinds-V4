// #include <ESP8266WiFi.h>
// #include <user_interface.h>  // Include this header for promiscuous mode

#include "user_interface.h"  // ESP8266 Non-OS SDK header for promiscuous mode

void wifi_promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
  Serial.println("wifi_promiscuous_cb_t");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("init");
}

void loop()
{

}