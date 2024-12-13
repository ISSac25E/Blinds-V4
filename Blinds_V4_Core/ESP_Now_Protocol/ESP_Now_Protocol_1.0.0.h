/*
  v1.0.0
    Reliably connect to target device and send messages back and forth
    There will be a dedicated master device and slave device. The master device will initiate and maintain the connection.
    After connection, this will function as a normal full duplex communication

    A packet can be sent with or without acknowledgment. This is determined by the sender.
    If sent with acknowledgment, a packet id will be required.

    Simple Packet:
      A simple packet is sent without any acknowledgment or need for connection

    Persistent Packet:
      A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)
      A persistent packet will attempt to send in between connections but will be dropped if PID's mismatch
      (PID mismatch signals packet already received or device reset)

    simple topology of communication:
      Master --> Beacon Packet (simple invitation to connect)
      Slave --> Connection Request (send current expected packet id [1-255, 0 if no packet id])
      Master --> Connection Response (Simple ack of connection request. Send Master's current expected packet id [1-255, 0 if no packet id])
      Master --> Keep Alive (ACK)
      Master --> Keep Alive (ACK)
      Master --> Keep Alive (ACK)

    Slave and Master will only accept the packet id that they are expecting.
    Any other packet id will be considered duplicate.
    A packet ACK will include the next expected packet id. This will be an opportunity to detect packet misalignment
*/
/*
  each packet will follow these general guidelines:
    the first byte is the command byte:
    [00000000]


    The first 4 bits are the packet command:
      [0000xxxx]
      0 = null
      1 = beacon packet
      2 = connection request
      3 = null (was connection response)
      4 = PID realignment
      5 = Keep Alive Packet
      6 = simple packet
      7 = persistent packet

    The 7th bit is the ACK bit
      [x0000000]
      0 = normal packet
      1 = ack packet (this will also set the command bytes to signify which packet is being acknowledged)

    beacon packet[1]:
      only master send this packet. 1 byte, nothing else needed.
      [00000001]

    connection request[2]:
      This is only sent by the slave. 2 bytes. [Command packet][Expected slave PID]
      [00000010][PID]

    connection request ack[2]:
      This is only sent by the master. ACK Packet. 2 bytes. [Command packet][Expected master PID]
      [10000010][PID]

    PID realignment [4]:
      This can be sent by either device. 1 byte. [Command packet]
      [00000100]

    PID realignment ack[4]:
      This can be sent by either device. 2 byte. [Command packet][PID]
      [10000100][PID]

    Keep Alive Packet[5]:
      This is only sent by the master. 1 byte. [Command packet]
      [00000100]

    Keep Alive Packet ack[5]:
      This is only sent by the slave. ACK Packet. 2 byte. [Command packet][expected slave PID]
      [10001001][PID]

    simple packet [6]:
      This can be sent by either master or slave. minimume 2 bytes. [Command packet][simple PID][PayLoad]
      This PID is different. It can be any number. It expires fairly quickly. This is only meant to prevent duplication in the event of a Mesh System (To be  developed)
      [00000110][PID][PayLoad]

    persistent packet [7]:
      This can be sent by either master or slave. minimume 2 bytes. [Command packet][PID][PayLoad]
      [00000111][PID][PayLoad]

    persistent packet ack[7]:
      This can be sent by either master or slave. 2 bytes. [Command packet][PID]
      [10000111][PID]

  */

#ifndef ESP_Now_Protocol_h
#define ESP_Now_Protocol_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "../LinkedList/LinkedList_1.0.0.h"

#ifndef ESP_Now_Protocol_PacketBuffer
#define ESP_Now_Protocol_PacketBuffer 3
#endif

class ESP_Now_Master;
class ESP_Now_Slave;
class class_ESP_Now_Master_SubClass;

class ESP_Now_utility
{
  friend ESP_Now_Master;
  friend ESP_Now_Slave;
  friend class_ESP_Now_Master_SubClass;

private:
  /*
      _compMac:
        compare two mac addresses
        returns false if mismatch
        returns true if match
    */
  static bool _compMac(uint8_t *mac0, uint8_t *mac1)
  {
    for (uint8_t x = 0; x < 6; x++)
      if (mac0[x] != mac0[x])
        return false;
    return true;
  }

  class par_bool
  {
  public:
    par_bool(bool var = false)
    {
      _var = var;
      _prevar = var;
    }
    bool var()
    {
      return _var;
    }
    bool var(bool n_var)
    {
      _var = n_var;
      return _var;
    }
    bool change()
    {
      if (_var != _prevar)
      {
        _prevar = _var;
        return true;
      }
      return false;
    }

  private:
    bool _var;
    bool _prevar;
  };
};

class class_ESP_Now_Master_SubClass
{
  friend ESP_Now_Master;

public:
  void init()
  {
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    _instancePointer = this;
    esp_now_register_send_cb(__onDataSend__);
    esp_now_register_recv_cb(__onDataRecv__);
  }

private:
  static class_ESP_Now_Master_SubClass *_instancePointer;
  static void __onDataRecv__(uint8_t *, uint8_t *, uint8_t);
  static void __onDataSend__(uint8_t *, uint8_t);

  void _addClass(ESP_Now_Master *ec)
  {
    if (__getClassIndex((void *)ec) == -1)
    {
      __classPointer.p = ec;
      __classList.addNode(__classPointer.b, sizeof(ESP_Now_Master *));
    }
  }
  void _deleteClass(ESP_Now_Master *ec)
  {
    if (__getClassIndex((void *)ec) != -1)
    {
      __classList.deleteNode(__getClassIndex((void *)ec));
    }
  }

  /*
    __getClassIndex
      returns index of node if class pointer is found, -1 otherwise
  */
  int16_t __getClassIndex(void *ptr)
  {
    for (uint16_t x = 0; x < __classList.nodeCount(); x++)
    {
      for (uint8_t y = 0; y < sizeof(void *); y++)
      {
        __classPointer.b[y] = __classList.getNode(x)[y];
      }

      if ((void *)__classPointer.p == ptr)
        return x;
    }
    return -1;
  }

  linkedList __classList;

  union
  {
    ESP_Now_Master *p;
    uint8_t b[sizeof(ESP_Now_Master *)];
  } __classPointer;
};
class_ESP_Now_Master_SubClass *class_ESP_Now_Master_SubClass::_instancePointer = nullptr;
class_ESP_Now_Master_SubClass ESP_Now_Master_SubClass;

class ESP_Now_Master
{
  friend class_ESP_Now_Master_SubClass;

public:
  /*
    ESP_Now_Master()
      input: target slave mac address, packet receive callback
      callback input: bytes, length, slave handler
  */
  ESP_Now_Master(uint8_t *, void (*)(uint8_t *, uint8_t, ESP_Now_Master *));
  ~ESP_Now_Master();

  void init()
  {
    ESP_Now_Master_SubClass.init();
  }

  /*
    run():
      handle connection and packets
  */
  void run();

  /*
    sendSimple():
      sends a one time message. Low priority message, no ACK, no PID

      input: bytes, length
  */
  // void sendSimple(uint8_t *, uint8_t);
  /*
    sendSimple():
      sends a persistent message. sends with ACK requirements and PID.
      If device is disconnected or disconnects in the process of sending, will continue to persist regardless.

      if multiple persistent packets are queued, will store in small internal buffer. Oldest message will be deleted.
      A PID misalignment will drop all packets.

    input: bytes, length
  */
  // void sendPersistent(uint8_t *, uint8_t);

  // bool isConnected();

private:
  const uint32_t BEACON_PACKET_POLL = 5000;
  const uint32_t KEEP_ALIVE_PACKET_POLL = 5000;
  const uint32_t DISCONNECT_TIMEOUT = 8000;
  bool _connected = false;
  /*
    This signifies that the slave's PID cannot be verified
    If a packet is dropped or connection is reset, a PID realignment will be required before anymore persistent packets can be sent over
  */
  bool _packetRealignment = true;

  /*
    since this is a master device, the slave PID is what we think the slave expects
    The Master PID is our PID
  */
  uint8_t _slavePID = 0;
  uint8_t _masterPID = 0;

  uint32_t _beaconPacketTimer = millis();
  uint32_t _keepAlivePacketTimer = millis();
  uint32_t _keepAlivePacketCompare = KEEP_ALIVE_PACKET_POLL;
  uint8_t _slaveMac[6];
  void (*_recvCallback)(uint8_t *, uint8_t, ESP_Now_Master *);

  void _onDataRecv(uint8_t *, uint8_t *, uint8_t);
  void _onDataSend(uint8_t *, uint8_t);

  /*
    this will contain all the queued, persistent packets waiting to be sent out
      list topology:
        [packet timeout in 100s of milliseconds. 0 will make this a persistent packet(2 bytes)][recorded millis if timeout used (4 bytes)][Linked PID. 0 if Message isn't being sent yet][packet to be sent]
  */
  linkedList _packetQueue;
};

void class_ESP_Now_Master_SubClass::__onDataRecv__(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (_instancePointer == nullptr)
    return;

  for (uint8_t x = 0; x < ESP_Now_Master_SubClass.__classList.nodeCount(); x++)
  {
    // convert bytes to pointer:
    for (uint8_t y = 0; y < sizeof(void *); y++)
      ESP_Now_Master_SubClass.__classPointer.b[y] = ESP_Now_Master_SubClass.__classList.getNode(x)[y];

    // if mac addresses compare, forward to correct instance:
    if (ESP_Now_utility::_compMac(ESP_Now_Master_SubClass.__classPointer.p->_slaveMac, mac))
      ESP_Now_Master_SubClass.__classPointer.p->_onDataRecv(mac, data, len);
  }
}
void class_ESP_Now_Master_SubClass::__onDataSend__(uint8_t *mac, uint8_t status)
{
  if (_instancePointer == nullptr)
    return;

  for (uint8_t x = 0; x < ESP_Now_Master_SubClass.__classList.nodeCount(); x++)
  {
    // convert bytes to pointer:
    for (uint8_t y = 0; y < sizeof(void *); y++)
      ESP_Now_Master_SubClass.__classPointer.b[y] = ESP_Now_Master_SubClass.__classList.getNode(x)[y];

    // if mac addresses compare, forward to correct instance:
    if (ESP_Now_utility::_compMac(ESP_Now_Master_SubClass.__classPointer.p->_slaveMac, mac))
      ESP_Now_Master_SubClass.__classPointer.p->_onDataSend(mac, status);
  }
}

ESP_Now_Master::ESP_Now_Master(uint8_t *mac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Master *))
{
  for (uint8_t x = 0; x < 6; x++)
    _slaveMac[x] = mac[x];

  _recvCallback = recvCallback;
  ESP_Now_Master_SubClass._addClass(this);
}

ESP_Now_Master::~ESP_Now_Master()
{
  ESP_Now_Master_SubClass._deleteClass(this);
}

void ESP_Now_Master::run()
{
  /*
    connection handler:
  */
  {
    if (!_connected)
    {
      // beacon packet timer. every 10 seconds:
      if (millis() - _beaconPacketTimer >= BEACON_PACKET_POLL)
      {
        // construct and send beacon packet
        uint8_t packet[] = {B00000001};
        Serial.println("tx: beacon Packet");
        esp_now_send((uint8_t *)_slaveMac, (uint8_t *)packet, 1);
        _beaconPacketTimer = millis();
      }
    }
    else
    {
      if (millis() - _keepAlivePacketTimer >= _keepAlivePacketCompare)
      {
        // construct and send keep alive packet
        uint8_t packet[] = {B00000101};
        Serial.print("tx: keep alive packet ");
        for (uint8_t x = 0; x < 6; x++)
        {
          Serial.print(_slaveMac[x]);
          Serial.print(",");
        }
        Serial.println();
        esp_now_send((uint8_t *)_slaveMac, (uint8_t *)packet, 1);

        _keepAlivePacketCompare = (millis() - _keepAlivePacketTimer) + 100;
      }

      // watchdog for last successful alive-packet-ack from slave:
      if (millis() - _keepAlivePacketTimer >= DISCONNECT_TIMEOUT)
      {
        _connected = false;
        _packetRealignment = true;
        _beaconPacketTimer = millis() + BEACON_PACKET_POLL; // << send imediate beacon packet
      }
    }
  }
}

void ESP_Now_Master::_onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (ESP_Now_utility::_compMac(_slaveMac, mac) && len)
  {
    switch (data[0] & B00001111)
    {
    case 1: // beacon packet
      // master does not expect anything
      break;
    case 2: // connection packet
      // master only expects the initial packet and sends the ack packet
      if (!(data[0] & B10000000) && len == 2) // cant be ack packet, must be two bytes
      {
        Serial.println("rx: connect packet");
        _connected = true; // set to connected
        _packetRealignment = false;
        _slavePID = data[1];
        _keepAlivePacketTimer = millis();
        _keepAlivePacketCompare = KEEP_ALIVE_PACKET_POLL;
        /*
          return acknowledgment:
        */
        uint8_t returnPacket[] = {B10000010, _masterPID};
        Serial.println("tx: connect packet ack");
        esp_now_send((uint8_t *)_slaveMac, (uint8_t *)returnPacket, 2);
      }
      break;
    case 4: // PID realignment
      // master or slave can initiate this packet
      break;
    case 5: // keep alive packet
      // master only expects ack packets for this
      if ((data[0] & B10000000) && len == 1)
      {
        Serial.println("rx: keep alive packet ack");
        _keepAlivePacketTimer = millis();
        _keepAlivePacketCompare = KEEP_ALIVE_PACKET_POLL;
      }
      break;
    case 6: // simple payload packet
      // master and slave can send this
      break;
    case 7: // persistent payload packet
      // master and slave can send this
      break;
    }
  }
}
void ESP_Now_Master::_onDataSend(uint8_t *, uint8_t)
{
}

class ESP_Now_Slave
{
public:
  /*
    ESP_Now_Slave()
      input: packet receive callback
      callback input: bytes, length, master handler
  */
  ESP_Now_Slave(void (*)(uint8_t *, uint8_t, ESP_Now_Slave *));

  void init()
  {
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    _instancePointer = this;
    esp_now_register_send_cb(_onDataSend);
    esp_now_register_recv_cb(_onDataRecv);
  }

  void run();

private:
  const uint32_t DISCONNECT_TIMEOUT = 8000;

  static ESP_Now_Slave *_instancePointer;
  static void _onDataRecv(uint8_t *, uint8_t *, uint8_t);
  static void _onDataSend(uint8_t *, uint8_t);

  bool _connected = false;
  /*
    This signifies that the master's PID cannot be verified
    If a packet is dropped or connection is reset, a PID realignment will be required before anymore persistent packets can be sent over
  */
  bool _packetRealignment = true;
  uint32_t _disconnectTimer = millis();
  uint8_t _masterMac[6];
  /*
    since this is a slave device, the master PID is what we think the master expects
    The slave PID is our PID
  */
  uint8_t _slavePID = 0;
  uint8_t _masterPID = 0;

  void (*_recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave *);

  /*
    this will contain all the queued, persistent packets waiting to be sent out
      list topology:
        [packet timeout in 100s of milliseconds. 0 will make this a persistent packet(2 bytes)][recorded millis if timeout used (4 bytes)][Linked PID. 0 if Message isn't being sent yet][packet to be sent]
  */
  linkedList _packetQueue;
};
ESP_Now_Slave *ESP_Now_Slave::_instancePointer = nullptr;

ESP_Now_Slave::ESP_Now_Slave(void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave *))
{
  _recvCallback = recvCallback;
}

void ESP_Now_Slave::_onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (_instancePointer == nullptr)
    return;

  if (_instancePointer->_connected)
  {
    if (ESP_Now_utility::_compMac((uint8_t *)_instancePointer->_masterMac, mac) && len)
    {
      switch (data[0] & B00001111)
      {
      case 1: // beacon packet
        // slave does not expect anything when connected
        break;
      case 2: // connection packet
        // slave doesn't expect anything while connected
        break;
      case 4: // PID realignment
        // master or slave can initiate this packet
        break;
      case 5: // keep alive packet
        // slave expects non-ack packets for this
        if (!(data[0] & B10000000) && len == 1)
        {
          Serial.println("rx: keep alive packet");
          _instancePointer->_disconnectTimer = millis();
          /*
            return acknowledgment:
          */
          uint8_t returnPacket[] = {B10000101};
          Serial.println("tx: keep alive packet ack");
          esp_now_send((uint8_t *)_instancePointer->_masterMac, (uint8_t *)returnPacket, 1);
        }
        break;
      case 6: // simple payload packet
        // master and slave can send this
        break;
      case 7: // persistent payload packet
        // master and slave can send this
        break;
      }
    }
  }
  else
  {
    if (len)
    {
      switch (data[0] & B00001111)
      {
      case 1: // beacon packet
        if (!(data[0] & B10000000) && len == 1)
        {
          Serial.println("rx: beacon packet");
          for (uint8_t x = 0; x < 6; x++)
            _instancePointer->_masterMac[x] = mac[x];
          /*
            send request packet:
          */
          uint8_t returnPacket[] = {B00000010, _instancePointer->_slavePID};
          Serial.println("tx: connect request");
          esp_now_send((uint8_t *)_instancePointer->_masterMac, (uint8_t *)returnPacket, 2);
        }
        break;
      case 2: // connection packet
              // slave only expects ack packet from master:
        if ((data[0] & B10000000) && len == 2)
        {
          Serial.println("rx: connect packet ack");
          _instancePointer->_masterPID = data[1];

          _instancePointer->_connected = true;
          _instancePointer->_packetRealignment = false;
          _instancePointer->_disconnectTimer = millis();  // you've got to be kidding me :(
        }
        break;
      case 4: // PID realignment
        // master or slave can initiate this packet
        break;
      case 5: // keep alive packet
        // slave does not expect keep-alive packets when disconnected
        break;
      case 6: // simple payload packet
        // master and slave can send this
        break;
      case 7: // persistent payload packet
        // master and slave can send this
        break;
      }
    }
  }
}

void ESP_Now_Slave::_onDataSend(uint8_t *mac, uint8_t status)
{
  if (_instancePointer == nullptr)
    return;
}

void ESP_Now_Slave::run()
{
  /*
    connection handler:
  */
  {
    if (!_connected)
    {
      // just wait. Slave doesn't need to do much
    }
    else
    {
      if (millis() - _disconnectTimer >= DISCONNECT_TIMEOUT)
      {
        _connected = false;
        _packetRealignment = true;
      }
    }
  }
}

#endif