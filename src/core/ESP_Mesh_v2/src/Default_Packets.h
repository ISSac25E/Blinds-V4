#ifndef DefaultPacekts_h
#define DefaultPacekts_h

#include <Arduino.h>

namespace DefaultPacket
{
  const char Beacon[] PROGMEM = "{\"type\":1}";
  const char HeartBeat[] PROGMEM = "{\"type\":3}";
  const char HeartBeat_ack[] PROGMEM = "{\"type\":3,\"ack\":true}";
};

#endif