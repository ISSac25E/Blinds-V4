// see readme

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ESP_Now_Utility.h"
#include "../../LinkedList/LinkedList_1.0.3.h"

//

// .h

//

class ESP_Now_Master_SubClass; // forward declaration

/*
  ESP_Now_Master_class
    This class should only have the single instances that is given
    It manages all slave devices and can dynamically add and remove devices as needed
*/
class ESP_Now_Master_class
{
public:
  /*
    init():
      this will initialize all necessary aspects of esp now communication
  */
  void init();

  /*
    run():
      handles all instances
  */
  void run();

  /*
    addSlave():
      will create new instance of ESP_Now_Master_SubClass
      inputs: slave mac address (uint8_t*), packet receive callback (void(*)(uint8_t*, uint8_t, ESP_Now_Master_SubClass*))
  */
  void addSlave(uint8_t *slaveMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Master_SubClass *));

  /*
    removeSlave():
      removes specific slave at index
      inputs: index(uint16_t)
  */
  void removeSlave(uint16_t);

  /*
    removeAll():
      removes all instances of slave devices
  */
  void removeAll();

  /*
    getSlave():
      input index and get pointer
      use pointer in invoke methods from slave controller

      inputs: slave index (uint16_t)
      returns: slave pointer at index (ESP_Now_Master_SubClass*)
      will return nullptr if no index found!
  */
  ESP_Now_Master_SubClass *getSlave(uint16_t);

  /*
    get number of classes created:
  */
  uint16_t getSlaveCount()
  {
    return __classList.nodeCount();
  }

private:
  /* for use with statice callback functions */
  static ESP_Now_Master_class *_instancePointer;

  /* callback functions for ESP_Now
      these can simply invoke the callback functions from every slave instance,
      but since that can cut into the processing a lot and is simply unneeded until extenders are implemented,
      let these filter packets by mac addresses */
  static void __onDataRecv__(uint8_t *mac, uint8_t *data, uint8_t len);
  static void __onDataSend__(uint8_t *mac, uint8_t status);

  /* contains all class pointers for each slave device: */
  linkedList __classList;
};
// init static class pointer:
ESP_Now_Master_class *ESP_Now_Master_class::_instancePointer = nullptr;
ESP_Now_Master_class ESP_Now_Master; // << make single static instance

/*
  ESP_Now_Master_SubClass:
    main class which handles connection and communication between slave and master device
*/
class ESP_Now_Master_SubClass
{
  friend ESP_Now_Master_class; // allow the main class to control private functions as it is the only one with access to espnow

public:
  ESP_Now_Master_SubClass(uint8_t *slaveMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Master_SubClass *))
  {
    for (uint8_t x = 0; x < 6; x++)
      _remote_mac[x] = slaveMac[x]; // copy mac address

    _recvCallback = recvCallback;
  }

  /*
    attempt to store packet into the queue if a slot is available

    since esp now is limited to 250 bytes, this funct will limit it to 200 bytes to allow for overhead

    inputs: (uint8_t*) data array, (uint8_t) data length, (uint8_t) packet bucket for ID'ing, (uint32_t) timeout (0 = no timeout)
    returns: (bool) true if packet stored in queue, false other wise
  */
  bool sendPersistentPacket(uint8_t *data, uint8_t len, uint8_t bucket, uint32_t timeout);

  /*
    send a simple packet, regardless of connection status
  */
  int sendSimplePacket(uint8_t *data, uint8_t len);
  void run();

  bool isConnected() { return _connected; }
  void getRemoteMac(uint8_t *mac)
  {
    for (uint8_t x = 0; x < 6; x++)
      mac[x] = _remote_mac[x];
  }

private:
  // target remote mac address:
  uint8_t _remote_mac[6];

  /*
    pid's:
      _local_PID: local PID
      _remote_PID: remote PID
  */
  uint8_t _local_PID = 0;
  uint8_t _remote_PID = 0;

  // connection stats. used and modified by _connection_class and _packet_class
  bool _connected = false;

  /*
    This signifies that the remote PID cannot be verified
    If a packet is dropped or connection is reset,
    a PID realignment will be required before sending anymore persistent packets can be attempted
  */
  bool _packetRealignment = true;

  /* callback functions for ESP Now main class to invoke: */
  void _onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
  {
    _connection.onDataRecv(mac, data, len);
    _packet.onDataRecv(mac, data, len);
  }
  void _onDataSend(uint8_t *mac, uint8_t status) {}

  class connection_class
  {
    // friend class ESP_Now_Master_SubClass;

  public:
    const uint32_t BEACON_PACKET_POLL = 10000;
    const uint32_t KEEP_ALIVE_PACKET_POLL = 5000;
    const uint32_t DISCONNECT_TIMEOUT = 8000; // must be greater than 'KEEP_ALIVE_PACKET_POLL'
    const uint32_t REALIGNMENT_PACKET_POLL = 300;

    // public:
    connection_class(ESP_Now_Master_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len);

    // private:
    /*
        timers:
    */
    uint32_t _beaconTimer = 0;
    uint32_t _disconnectTimer = 0;
    uint32_t _realignmentTimer = 0;

    /*
      timer compare value:
    */
    uint32_t _keepAliveCompare = 0;

    ESP_Now_Master_SubClass *_pointer;
  };
  connection_class _connection{this};

  class packet_class
  {
    // friend class ESP_Now_Master_SubClass;

    // private:
  public:
    packet_class(ESP_Now_Master_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len);

    /*
      set functions to send simple and persistent packets
      minimal error handling
    */

    //  private:
    const uint8_t _MAX_PACKET = 10;        // declare max number of persistent packets allowed to hold
    const uint8_t _SEND_PACKET_POLL = 300; // how often a missed packet will attempt to send

    linkedList _packetBuffer;
    /*
      Improved, safer version of the packet structure
      Utility tool. Used to package up a packet or un-package it. All static methods

      uint8_t packet topology:
        [PID(1Byte)][BUCKET(1Byte)][TIMEOUT(4Byte)][TIMER*(4Byte)][DATA(>=0Byte)]

      *TIMER bytes only exist if the TIMEOUT bytes are set. If TIMEOUT bytes equate to 0, no TIMER exists
      input array pointer and array length
    */
    class packet_struct
    {
    public:
      static uint8_t getPID(uint8_t *arr, uint16_t len);
      static void setPID(uint8_t *arr, uint16_t len, uint8_t PID);
      static uint8_t getBucket(uint8_t *arr, uint16_t len);
      static uint32_t getTimeout(uint8_t *arr, uint16_t len);
      static uint32_t getTimer(uint8_t *arr, uint16_t len);
      static uint16_t dataSize(uint8_t *arr, uint16_t len);
      static uint8_t *getData(uint8_t *arr, uint16_t len);

      // overload funct:
      static void storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint8_t *data, uint8_t len)
      {
        packet_struct::storePacket(ll, PID, BUCKET, 0, data, len);
      }

      static void storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint32_t timeout, uint8_t *data, uint8_t len);
    };

    uint32_t _sendPacketTimer = 0;

    void _checkPacketTimers(bool checkMinimume = false);
    ESP_Now_Master_SubClass *_pointer;
  };
  packet_class _packet{this};

  // callback fro new packets:
  void (*_recvCallback)(uint8_t *, uint8_t, ESP_Now_Master_SubClass *) = nullptr;
};

//

// .cpp

//

/*
  ESP_Now_Master_class
*/
void ESP_Now_Master_class::init()
{
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  _instancePointer = this;
  esp_now_register_recv_cb(__onDataRecv__);
  esp_now_register_send_cb(__onDataSend__);
}

void ESP_Now_Master_class::run()
{
  for (uint8_t x = 0; x < __classList.nodeCount(); x++)
    getSlave(x)->run();
}

void ESP_Now_Master_class::addSlave(uint8_t *slaveMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Master_SubClass *))
{
  // create new instance:
  ESP_Now_Master_SubClass *newSlave = new ESP_Now_Master_SubClass(slaveMac, recvCallback);
  // ESP_Now_Master_SubClass newSlave(slaveMac, recvCallback);

  // store new instance:
  __classList.appendNode<ESP_Now_Master_SubClass *>(newSlave);
}

void ESP_Now_Master_class::removeSlave(uint16_t index)
{
  if (index < __classList.nodeCount())
  {
    // free memory and remove from list:
    delete getSlave(index);
    __classList.deleteNode(index);
  }
}

void ESP_Now_Master_class::removeAll()
{
  // simple while loop to delete all instances:
  while (__classList.nodeCount())
    removeSlave(0);
}

ESP_Now_Master_SubClass *ESP_Now_Master_class::getSlave(uint16_t index)
{
  if (__classList.getNodeSize(index) < 0)
    return nullptr;

  return *(__classList.getNodeData<ESP_Now_Master_SubClass *>(index));
}

void ESP_Now_Master_class::__onDataRecv__(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (_instancePointer == nullptr)
    return;

  // go through each instance and find a matching mac address:
  for (uint8_t x = 0; x < _instancePointer->__classList.nodeCount(); x++)
  {
    ESP_Now_Master_SubClass *p = _instancePointer->getSlave(x);

    // check if mac addresses match:
    if (ESP_Now_util::compMac(mac, p->_remote_mac))
      p->_onDataRecv(mac, data, len);
  }
}
void ESP_Now_Master_class::__onDataSend__(uint8_t *mac, uint8_t status)
{
  if (_instancePointer == nullptr)
    return;

  // go through each instance and find a matching mac address:
  for (uint8_t x = 0; x < _instancePointer->__classList.nodeCount(); x++)
  {
    ESP_Now_Master_SubClass *p = _instancePointer->getSlave(x);

    // check if mac addresses match:
    if (ESP_Now_util::compMac(mac, p->_remote_mac))
      p->_onDataSend(mac, status);
  }
}

//

// .cpp

//

/*
  ESP_Now_Master_SubClass
*/
bool ESP_Now_Master_SubClass::sendPersistentPacket(uint8_t *data, uint8_t len, uint8_t bucket, uint32_t timeout)
{
  if (len > 200)
    return false;

  if (_packet._packetBuffer.nodeCount() >= _packet._MAX_PACKET)
    _packet._checkPacketTimers();

  if (_packet._packetBuffer.nodeCount() < _packet._MAX_PACKET)
  {
    // add new packet to the tail of
    packet_class::packet_struct::storePacket(&_packet._packetBuffer, 0 /*PID = 0*/, bucket, timeout, data, len);
    return true;
  }
  return false;
}

int ESP_Now_Master_SubClass::sendSimplePacket(uint8_t *data, uint8_t len)
{
  return esp_now_send(_remote_mac, data, len);
}

void ESP_Now_Master_SubClass::run()
{
  _connection.run();
  _packet.run();
}

/*
  ESP_Now_Master_SubClass::connection_class
*/
void ESP_Now_Master_SubClass::connection_class::run()
{
  if (_pointer->_connected)
  {
    /*
      when connected, run keep alive packets and watchdog timer
    */
    if (millis() - _disconnectTimer >= _keepAliveCompare)
    {
      uint8_t pck[] = {ESP_Now_util::setCmdPck(5, false)};
      esp_now_send(_pointer->_remote_mac, (uint8_t *)pck, sizeof(pck));

      _keepAliveCompare = (millis() - _disconnectTimer) + 500; // << begin rapidly requesting keep alive packets if no response
    }

    /*
      run packet realignment requests
    */
    if (_pointer->_packetRealignment)
      if (millis() - _realignmentTimer >= REALIGNMENT_PACKET_POLL)
      {
        uint8_t pck[] = {ESP_Now_util::setCmdPck(4, false)};
        esp_now_send(_pointer->_remote_mac, (uint8_t *)pck, sizeof(pck));

        _realignmentTimer = millis();
      }

    // watchdog disconnect timer:
    if (millis() - _disconnectTimer >= DISCONNECT_TIMEOUT)
    {
      _pointer->_connected = false;
      _pointer->_packetRealignment = true;
      _beaconTimer = millis() + BEACON_PACKET_POLL;
    }
  }
  else
  {
    /*
      when disconnected, run beacon packets
    */
    if (millis() - _beaconTimer >= BEACON_PACKET_POLL)
    {
      // construct and send beacon packet:
      uint8_t pck[] = {B00000001};
      esp_now_send(_pointer->_remote_mac, (uint8_t *)pck, sizeof(pck));
      _beaconTimer = millis();
    }
  }
}
void ESP_Now_Master_SubClass::connection_class::onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  // verify mac and command byte length:
  if (ESP_Now_util::compMac(mac, _pointer->_remote_mac) && len)
  {
    switch (data[0] & B00001111)
    {
    case 2:
      /*
        connection packet:
          master expects a connection request from slave
          master sends ack packet back
      */
      if (len == 2 && !ESP_Now_util::isAck(data))
      {
        /*
          device connected and ready
          set associated vars:
        */
        _pointer->_connected = true;
        _pointer->_packetRealignment = false;
        _pointer->_remote_PID = data[1];

        /*
          reset timers:
        */
        _disconnectTimer = millis();
        _keepAliveCompare = KEEP_ALIVE_PACKET_POLL;

        /*
          send ack packet
        */
        uint8_t pck[] = {ESP_Now_util::setCmdPck(2, true), _pointer->_local_PID};
        esp_now_send((uint8_t *)_pointer->_remote_mac, (uint8_t *)pck, sizeof(pck));
      }
      break;
    case 4:
      /*
        PID alignment packet
        expecting request and response
      */
      if (len == 1 && !ESP_Now_util::isAck(data))
      {
        /*
          PID alignment request
          send ack packet back
        */
        // if (_pointer->_connected)  // not really necessary
        // {
        uint8_t pck[] = {ESP_Now_util::setCmdPck(4, true), _pointer->_local_PID};
        esp_now_send((uint8_t *)_pointer->_remote_mac, (uint8_t *)pck, sizeof(pck));
        // }
      }
      else if (len == 2 && ESP_Now_util::isAck(data))
      {
        /*
          PID alignment response
          set associated vars:
        */
        if (_pointer->_connected)
        {
          _pointer->_packetRealignment = false;
          _pointer->_remote_PID = data[1];
        }
      }
      break;
    case 5:
      /*
        keep alive packet:
          master only expects ack from slave
      */
      if (len == 1 && ESP_Now_util::isAck(data))
      {
        // reset necessary timers:
        _disconnectTimer = millis();
        _keepAliveCompare = KEEP_ALIVE_PACKET_POLL;
      }
      break;
    default:
      break;
    }
  }
}

/*
  ESP_Now_Master_SubClass::packet_class::packet_struct
*/
uint8_t ESP_Now_Master_SubClass::packet_class::packet_struct::getPID(uint8_t *arr, uint16_t len)
{
  if (len < 1)
  {
    return 0;
  }

  else
    return arr[0];
}
void ESP_Now_Master_SubClass::packet_class::packet_struct::setPID(uint8_t *arr, uint16_t len, uint8_t PID)
{
  if (len < 1)
    return;

  arr[0] = PID;
}
uint8_t ESP_Now_Master_SubClass::packet_class::packet_struct::getBucket(uint8_t *arr, uint16_t len)
{
  if (len < 2)
    return 0;

  return arr[1];
}
uint32_t ESP_Now_Master_SubClass::packet_class::packet_struct::getTimeout(uint8_t *arr, uint16_t len)
{
  /*
    less than 6 bytes automatically means either no packet or not enough bytes for a timeout to be present
  */
  if (len < 6)
    return 0;

  return ((uint32_t)arr[2] | ((uint32_t)arr[3] << (1 * 8)) | ((uint32_t)arr[4] << (2 * 8)) | ((uint32_t)arr[5] << (3 * 8)));
}
uint32_t ESP_Now_Master_SubClass::packet_class::packet_struct::getTimer(uint8_t *arr, uint16_t len)
{
  /*
    less than 10 byte automatically means either no packet or not enough bytes for a timer to be present
  */
  if (len < 10)
    return 0;

  if (packet_struct::getTimeout(arr, len))
    return ((uint32_t)arr[6] | ((uint32_t)arr[7] << (1 * 8)) | ((uint32_t)arr[8] << (2 * 8)) | ((uint32_t)arr[9] << (3 * 8)));
  else
    return 0;
}
uint16_t ESP_Now_Master_SubClass::packet_class::packet_struct::dataSize(uint8_t *arr, uint16_t len)
{
  if (packet_struct::getTimeout(arr, len))
  {
    if (len < 10)
      return 0;
    else
      return len - 10;
  }
  else
  {
    if (len < 6)
      return 0;
    else
      return len - 6;
  }
}
uint8_t *ESP_Now_Master_SubClass::packet_class::packet_struct::getData(uint8_t *arr, uint16_t len)
{
  if (packet_struct::getTimeout(arr, len))
  {
    if (len < 10)
      return nullptr;
    else
      return &arr[10]; // 11th spot, if datasize is zero, it should not be used!
  }
  else
  {
    if (len < 6)
      return nullptr;
    else
      return &arr[6]; // 7th spot, if datasize is zero, it should not be used!
  }
}

// overload funct:
void ESP_Now_Master_SubClass::packet_class::packet_struct::storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint32_t timeout, uint8_t *data, uint8_t len)
{
  uint16_t pckgLen = 6 + (timeout ? 4 : 0) + len;
  uint8_t pckg[pckgLen];
  uint16_t dataIndex = 6 + (timeout ? 4 : 0); // 10 if timeout

  pckg[0] = PID;
  pckg[1] = BUCKET;

  pckg[2] = timeout;
  pckg[3] = timeout >> (8 * 1);
  pckg[4] = timeout >> (8 * 2);
  pckg[5] = timeout >> (8 * 3);

  if (timeout)
  {
    uint32_t tim = millis();

    pckg[6] = tim;
    pckg[7] = tim >> (8 * 1);
    pckg[8] = tim >> (8 * 2);
    pckg[9] = tim >> (8 * 3);
  }

  for (uint8_t x = 0; x < len; x++)
    pckg[dataIndex + x] = data[x];

  ll->appendNode<uint8_t>(pckg, pckgLen);
}

/*
  ESP_Now_Master_SubClass::packet_class
*/
void ESP_Now_Master_SubClass::packet_class::run()
{
  /*
    check for nodes:
      no nodes means no packets to send:
  */
  if (_packetBuffer.nodeCount())
  {
    /*
      check if head packet timer has timed-out:
    */
    if (packet_struct::getTimeout(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)) &&
        millis() - packet_struct::getTimer(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)) >= packet_struct::getTimeout(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)))
    {
      // timeout reached, remove packet:
      if (packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)) != 0)
      {
        // packet was actively being sent, realignment needed:
        _pointer->_packetRealignment = true;
      }
      _packetBuffer.deleteNode(0);
      // TODO: Flag?
    }
    else
    {
      // packet at the head of the node isn't timed out yet
      //  check if a current packet is/was sending:
      if (packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)) != 0)
      {
        // currently sending...
        if (_pointer->_connected && !(_pointer->_packetRealignment))
        {
          // check if PIDs don't align:
          if (_pointer->_remote_PID != packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)))
          {
            if (!_pointer->_remote_PID)
            {
              // device rebooted?
              // flag?
            }
            else
            {
              // message received
            }
            _packetBuffer.deleteNode(0); // either ways, get rid of this packet
          }
          else
          {
            // PID's still align, send another attempt if needed:
            if (millis() - _sendPacketTimer >= _SEND_PACKET_POLL)
            {
              _sendPacketTimer = millis();

              /*
                construct the packet:
              */
              uint8_t pck[2 + packet_struct::dataSize(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0))];
              pck[0] = ESP_Now_util::setCmdPck(7, false);
              pck[1] = packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0));
              for (uint8_t x = 0; x < packet_struct::dataSize(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)); x++)
                pck[2 + x] = packet_struct::getData(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0))[x];

              esp_now_send(_pointer->_remote_mac, pck, sizeof(pck));
            }
          }
        }
      }
      else
      {
        // not sending...
        if (_pointer->_connected && !(_pointer->_packetRealignment))
        {
          // start sending next packet:
          // once a pid is set, cannot be undone:
          packet_struct::setPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0), (_pointer->_remote_PID ? _pointer->_remote_PID : ++(_pointer->_remote_PID)));
          _sendPacketTimer = millis();

          /*
            construct the packet:
          */
          uint8_t pck[2 + packet_struct::dataSize(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0))];
          pck[0] = ESP_Now_util::setCmdPck(7, false);
          pck[1] = packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0));
          for (uint8_t x = 0; x < packet_struct::dataSize(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0)); x++)
            pck[2 + x] = packet_struct::getData(_packetBuffer.getNodeData<uint8_t>(0), _packetBuffer.getNodeSize(0))[x];

          esp_now_send(_pointer->_remote_mac, pck, sizeof(pck));
        }
      }
    }
  }
}
void ESP_Now_Master_SubClass::packet_class::onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  // verify mac and command byte length:
  if (ESP_Now_util::compMac(mac, _pointer->_remote_mac) && len)
  {
    switch (data[0] & B00001111)
    {
    case 6:
      /*
        simple packet
        no acknowledgment, duplicates allowed. PID byte for future mesh system (can ignore)
      */
      if (!ESP_Now_util::isAck(data) && len > 1)
      {
        _pointer->_recvCallback(&data[2], len - 2, _pointer);
      }
      break;
    case 7:
      /*
        persistent packet Command
        this can either be an ack or new packet
      */
      if (ESP_Now_util::isAck(data))
      {
        /*
          acknowledging a packet we sent

          we have two options here, help out the program a little and clear out currently sending message out of the queue,
          or update the remote id and let the main loop clean up itself (slower but safer)

          going for safer route
        */
        if (_pointer->_connected && len == 2)
        {
          _pointer->_remote_PID = data[1];
          _pointer->_packetRealignment = false; // why not
        }
      }
      else
      {
        /*
          new packet received, this is much easier to handle compared to sending a packet.
          make sure connected, send response. If PID's match, update PID and call callBack
        */
        if (_pointer->_connected && len > 1)
        {
          bool msgReceived = false;
          if (!_pointer->_local_PID || (_pointer->_local_PID && _pointer->_local_PID == data[1]))
          {
            // new message received!
            _pointer->_local_PID = data[1] + 1;
            if (!_pointer->_local_PID)
              _pointer->_local_PID++;

            msgReceived = true;
          }

          uint8_t pck[2] = {ESP_Now_util::setCmdPck(7, true), _pointer->_local_PID};
          esp_now_send(_pointer->_remote_mac, pck, sizeof(pck));

          if (msgReceived)
            _pointer->_recvCallback(&data[2], len - 2, _pointer);
        }
      }
      break;
    }
  }
}

void ESP_Now_Master_SubClass::packet_class::_checkPacketTimers(bool checkMinimume /* = false */)
{
  /*
      check for packet timeouts:
      unkown how much this might slow down the program:
      starting from back of list because packet list might shrink as searching
  */
  if (_packetBuffer.nodeCount())
    for (int16_t x = _packetBuffer.nodeCount() - 1; x >= 0; x--)
    {
      if (packet_struct::getTimeout(_packetBuffer.getNodeData<uint8_t>(x), _packetBuffer.getNodeSize(x)))
        if (millis() - packet_struct::getTimer(_packetBuffer.getNodeData<uint8_t>(x), _packetBuffer.getNodeSize(x)) >= packet_struct::getTimeout(_packetBuffer.getNodeData<uint8_t>(x), _packetBuffer.getNodeSize(x)))
        {
          // timeout reached, remove packet:
          if (packet_struct::getPID(_packetBuffer.getNodeData<uint8_t>(x), _packetBuffer.getNodeSize(x)) != 0)
          {
            // packet was actively being sent, realignment needed:
            _pointer->_packetRealignment = true;
          }
          _packetBuffer.deleteNode(x);
          // TODO: Flag?
          if (checkMinimume)
            return;
        }
    }
}