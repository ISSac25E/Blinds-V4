v2.0.0 (atmega238p)
changes:
  - implement method to check heartbeats if module is alive and working
    If module stops sending heartbeats, "TuyaWifi.mcu_get_wifi_work_state()" will equal "WIFI_STATE_UNKNOWN"
    HeartBeat is expected around every 15s
  - change "WIFI_SATE_UNKNOW" to "WIFI_STATE_UNKNOWN"

v2.1.0 (ESP8266)
changes:
  - same features as v2.0.0 (atmega238p). Now compatible with esp8266