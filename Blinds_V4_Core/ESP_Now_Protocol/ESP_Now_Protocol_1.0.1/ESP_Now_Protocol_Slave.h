// see readme

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ESP_Now_Utility.h"
#include "../../LinkedList/LinkedList_1.0.3.h"

//

// .h

//

class ESP_Now_Slave_SubClass; // forward declaration

class ESP_Now_Slave_class
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
    addMaster():
      will create new instance of ESP_Now_Slave_SubClass
      inputs: slave mac address (uint8_t*), packet receive callback (void(*)(uint8_t*, uint8_t, ESP_Now_Slave_SubClass*))
  */
  void addMaster(uint8_t *slaveMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave_SubClass *));

  /*
    removeMaster():
      removes specific master at index
      inputs: index(uint16_t)
  */
  void removeMaster(uint16_t);

  /*
    removeAll():
      removes all instances of master devices
  */
  void removeAll();

  /*
    getMaster():
      input index and get pointer
      use pointer in invoke methods from master controller

      inputs: master index (uint16_t)
      returns: master pointer at index (ESP_Now_Slave_SubClass*)
      will return nullptr if no index found!
  */
  ESP_Now_Slave_SubClass *getMaster(uint16_t);

  /*
    get number of classes created:
  */
  uint16_t getMasterCount()
  {
    return __classList.nodeCount();
  }

private:
  /* for use with statice callback functions */
  static ESP_Now_Slave_class *_instancePointer;

  /* callback functions for ESP_Now
       these can simply invoke the callback functions from every slave instance,
       but since that can cut into the processing a lot and is simply unneeded until extenders are implemented,
       let these filter packets by mac addresses */
  static void __onDataRecv__(uint8_t *mac, uint8_t *data, uint8_t len);
  static void __onDataSend__(uint8_t *mac, uint8_t status);

  /* contains all class pointers for each master device: */
  linkedList __classList;
};
// init static class pointer:
ESP_Now_Slave_class *ESP_Now_Slave_class::_instancePointer = nullptr;
ESP_Now_Slave_class ESP_Now_Slave; // << make single static instance

class ESP_Now_Slave_SubClass
{
  friend ESP_Now_Slave_class;

public:
  ESP_Now_Slave_SubClass(uint8_t *masterMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave_SubClass *))
  {
    if (!masterMac)
    {
      _headless_master = true;
    }
    else
    {
      _headless_master = false;
      for (uint8_t x = 0; x < 6; x++)
        _remote_mac[x] = masterMac[x]; // copy mac address
    }

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
    *unless headless master mode is enabled, then a master needs to be established before sending
  */
  int sendSimplePacket(uint8_t *data, uint8_t len);
  void run();

  bool isConnected() { return _connected; }
  void getRemoteMac(uint8_t *mac)
  {
    if (!_headless_master || _connected)
      for (uint8_t x = 0; x < 6; x++)
        mac[x] = _remote_mac[x];
    else
      for (uint8_t x = 0; x < 6; x++)
        mac[x] = 0;
  }

private:
  // target master remote mac address:
  uint8_t _remote_mac[6];

  bool _headless_master;

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
  public:
    const uint32_t DISCONNECT_TIMEOUT = 8000;
    const uint32_t REALIGNMENT_PACKET_POLL = 300;

    connection_class(ESP_Now_Slave_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len);

    /*
      timers:
        _disconnectTimer is interesting, it is only updated during conenction and keep alive packets
        while it makes perfect sense to update this timer during packet transfers or PID realignments,
        this timer is ONLY updated during those two events for simplicity sake.
        So even if both devices are sending numerous data packets, keep alive packets are still required just as frequently
        think of it as two separate mechanisms that work independently
    */
    uint32_t _disconnectTimer = 0;
    uint32_t _realignmentTimer = 0;

    /*
      timer compare value:
    */
    uint32_t _keepAliveCompare = 0;

    ESP_Now_Slave_SubClass *_pointer;
  };
  connection_class _connection{this};

  class packet_class
  {
  public:
    packet_class(ESP_Now_Slave_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len);

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
    ESP_Now_Slave_SubClass *_pointer;
  };
  packet_class _packet{this};

  // callback fro new packets:
  void (*_recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave_SubClass *) = nullptr;
};

//

// .cpp

//

/*
  ESP_Now_Slave_class
*/
void ESP_Now_Slave_class::init()
{
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  _instancePointer = this;
  esp_now_register_recv_cb(__onDataRecv__);
  esp_now_register_send_cb(__onDataSend__);
}

void ESP_Now_Slave_class::run()
{
  for (uint8_t x = 0; x < __classList.nodeCount(); x++)
    getMaster(x)->run();
}

void ESP_Now_Slave_class::addMaster(uint8_t *masterMac, void (*recvCallback)(uint8_t *, uint8_t, ESP_Now_Slave_SubClass *))
{
  // create new instance:
  ESP_Now_Slave_SubClass *newMaster = new ESP_Now_Slave_SubClass(masterMac, recvCallback);

  if (newMaster == nullptr)
    return;

  // store new instance:
  __classList.appendNode<ESP_Now_Slave_SubClass *>(newMaster);
}

void ESP_Now_Slave_class::removeMaster(uint16_t index)
{
  if (index < __classList.nodeCount())
  {
    // free memory and remove from list:
    delete getMaster(index);
    __classList.deleteNode(index);
  }
}

void ESP_Now_Slave_class::removeAll()
{
  // simple while loop to delete all instances:
  while (__classList.nodeCount())
    removeMaster(0);
}

ESP_Now_Slave_SubClass *ESP_Now_Slave_class::getMaster(uint16_t index)
{
  if (__classList.getNodeSize(index) < 0)
    return nullptr;

  return *(__classList.getNodeData<ESP_Now_Slave_SubClass *>(index));
}

void ESP_Now_Slave_class::__onDataRecv__(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (_instancePointer == nullptr)
    return;

  // go through each instance and find a matching mac address:
  for (uint8_t x = 0; x < _instancePointer->__classList.nodeCount(); x++)
  {
    ESP_Now_Slave_SubClass *p = _instancePointer->getMaster(x);

    // check if mac addresses match:
    if (ESP_Now_util::compMac(mac, p->_remote_mac) || (p->_headless_master && !p->_connected))
      p->_onDataRecv(mac, data, len);
  }
}
void ESP_Now_Slave_class::__onDataSend__(uint8_t *mac, uint8_t status)
{
  if (_instancePointer == nullptr)
    return;

  // go through each instance and find a matching mac address:
  for (uint8_t x = 0; x < _instancePointer->__classList.nodeCount(); x++)
  {
    ESP_Now_Slave_SubClass *p = _instancePointer->getMaster(x);

    // check if mac addresses match:
    if (ESP_Now_util::compMac(mac, p->_remote_mac))
      p->_onDataSend(mac, status);
  }
}

//

// .cpp

//

/*
  ESP_Now_Slave_SubClass
*/
bool ESP_Now_Slave_SubClass::sendPersistentPacket(uint8_t *data, uint8_t len, uint8_t bucket, uint32_t timeout)
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

int ESP_Now_Slave_SubClass::sendSimplePacket(uint8_t *data, uint8_t len)
{
  if (_connected || !_headless_master)
    return esp_now_send(_remote_mac, data, len);
  return 1;
}

void ESP_Now_Slave_SubClass::run()
{
  _connection.run();
  _packet.run();
}

/*
  ESP_Now_Slave_SubClass::connection_class
*/
void ESP_Now_Slave_SubClass::connection_class::run()
{
  if (_pointer->_connected)
  {
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
    }
  }
  else
  {
    /*
      if disconnected, all the slave device can do is wait for a beacon from a master(handled by recv-Callback).

      technically, it is possible for the slave to preemptively attempt a connection to the master if the mac address is known
      for simplicity sake, we will let the beacon mechanism do its job
    */
  }
}

void ESP_Now_Slave_SubClass::connection_class::onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  // verify mac and command byte length:
  if (len)
  {
    if (_pointer->_headless_master && !_pointer->_connected)
    {
      /*
        this will capture all packets regardless of mac address if slave is in headless mode
          and disconnected
      */
      switch (data[0] & B00001111)
      {
      case 1:
        /*
          beacon packet:

          send the connection request,
          do nothing else
        */
        if (len == 1 /*i dont think we care about ack for this one*/)
        {
          uint8_t pck[] = {ESP_Now_util::setCmdPck(2, false), _pointer->_local_PID};
          esp_now_send(mac, pck, sizeof(pck));
        }
        break;
      case 2:
        /*
          connection packet:
            slave only expects connection ack packets

            save master's mac
        */
        if (len == 2 && ESP_Now_util::isAck(data))
        {
          /*
            store master mac address
          */
          for (uint8_t x = 0; x < 6; x++)
            _pointer->_remote_mac[x] = mac[x]; // copy mac address

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
        }
        break;
      };
    }
    else if (ESP_Now_util::compMac(mac, _pointer->_remote_mac))
    {
      /*
        this will only capture packets from the designated mac address
          regardless of connection state
      */
      switch (data[0] & B00001111)
      {
      case 1:
        /*
          beacon packet:

          check if disconnected
          send the connection request,
          do nothing else
        */
        if (len == 1 && !(_pointer->_connected) /*i dont think we care about ack for this one*/)
        {
          uint8_t pck[] = {ESP_Now_util::setCmdPck(2, false), _pointer->_local_PID};
          esp_now_send(mac, pck, sizeof(pck));
        }
        break;
      case 2:
        /*
          connection packet:
            slave only expects connection ack packets
        */
        if (len == 2 && !(_pointer->_connected) && ESP_Now_util::isAck(data))
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
            slave only expects requests from master
        */
        if (len == 1 && !ESP_Now_util::isAck(data))
        {
          // reset necessary timers:
          _disconnectTimer = millis();

          /*
            send ack packet
          */
          uint8_t pck[] = {ESP_Now_util::setCmdPck(5, true)};
          esp_now_send(_pointer->_remote_mac, pck, sizeof(pck));
        }
        break;
      }
    }
  }
}

/*
  ESP_Now_Slave_SubClass::packet_class::packet_struct
*/
uint8_t ESP_Now_Slave_SubClass::packet_class::packet_struct::getPID(uint8_t *arr, uint16_t len)
{
  if (len < 1)
  {
    return 0;
  }

  else
    return arr[0];
}
void ESP_Now_Slave_SubClass::packet_class::packet_struct::setPID(uint8_t *arr, uint16_t len, uint8_t PID)
{
  if (len < 1)
    return;

  arr[0] = PID;
}
uint8_t ESP_Now_Slave_SubClass::packet_class::packet_struct::getBucket(uint8_t *arr, uint16_t len)
{
  if (len < 2)
    return 0;

  return arr[1];
}
uint32_t ESP_Now_Slave_SubClass::packet_class::packet_struct::getTimeout(uint8_t *arr, uint16_t len)
{
  /*
    less than 6 bytes automatically means either no packet or not enough bytes for a timeout to be present
  */
  if (len < 6)
    return 0;

  return ((uint32_t)arr[2] | ((uint32_t)arr[3] << (1 * 8)) | ((uint32_t)arr[4] << (2 * 8)) | ((uint32_t)arr[5] << (3 * 8)));
}
uint32_t ESP_Now_Slave_SubClass::packet_class::packet_struct::getTimer(uint8_t *arr, uint16_t len)
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
uint16_t ESP_Now_Slave_SubClass::packet_class::packet_struct::dataSize(uint8_t *arr, uint16_t len)
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
uint8_t *ESP_Now_Slave_SubClass::packet_class::packet_struct::getData(uint8_t *arr, uint16_t len)
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
void ESP_Now_Slave_SubClass::packet_class::packet_struct::storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint32_t timeout, uint8_t *data, uint8_t len)
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

//

/*
  ESP_Now_Slave_SubClass::packet_class
*/
void ESP_Now_Slave_SubClass::packet_class::run()
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
void ESP_Now_Slave_SubClass::packet_class::onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
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

void ESP_Now_Slave_SubClass::packet_class::_checkPacketTimers(bool checkMinimume /*  = false */)
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