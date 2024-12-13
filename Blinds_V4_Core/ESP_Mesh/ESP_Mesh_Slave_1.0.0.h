// see readme

#ifndef ESP_Mesh_Slave_h
#define ESP_Mesh_Slave_h

#include "Arduino.h"
#include <painlessMesh.h>

#include "ESP_Mesh_Utillity_1.0.0.h"
#include "../LinkedList/LinkedList_1.0.3.h"

//

// .h

//

class ESP_Mesh_Slave_SubClass; // forward declaration

class ESP_Mesh_Slave_class
{
public:
  // /*
  //   give an instance of the esp mesh
  // */
  // ESP_Mesh_Slave_class(painlessMesh *);

  void init(painlessMesh *);

  /*
    run():
      handles all instances
  */
  void run();

  /*
    addMaster():
      will create new instance of ESP_Mesh_Slave_SubClass
      inputs: slave mac address (uint8_t*), packet receive callback (void(*)(uint8_t*, uint8_t, ESP_Mesh_Slave_SubClass*))
  */
  void addMaster(uint32_t master_id, void (*recvCallback)(uint8_t *, uint8_t, ESP_Mesh_Slave_SubClass *));

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
      returns: master pointer at index (ESP_Mesh_Slave_SubClass*)
      will return nullptr if no index found!
  */
  ESP_Mesh_Slave_SubClass *getMaster(uint16_t);

  /*
    get number of classes created:
  */
  uint16_t getMasterCount()
  {
    return __classList.nodeCount();
  }

private:
  /* for use with statice callback functions */
  static ESP_Mesh_Slave_class *_instancePointer;
  painlessMesh *_meshPointer;

  /* callback function for painlessMesh
      this can simply invoke the callback functions from every master instance */
  static void __onDataRecv__(uint32_t node_id, String msg);

  /* contains all class pointers for each master device: */
  linkedList __classList;
};
// init static class pointer:
ESP_Mesh_Slave_class *ESP_Mesh_Slave_class::_instancePointer = nullptr;
ESP_Mesh_Slave_class ESP_Now_Slave; // << make single static instance

class ESP_Mesh_Slave_SubClass
{
  friend ESP_Mesh_Slave_class;

public:
  ESP_Mesh_Slave_SubClass(uint32_t master_id, painlessMesh *mesh, void (*recvCallback)(uint8_t *, uint8_t, ESP_Mesh_Slave_SubClass *))
  {
    _meshPointer = mesh;
    if (!master_id)
    {
      _headless_master = true;
    }
    else
    {
      _headless_master = false;
      _remote_id = master_id;
    }

    _recvCallback = recvCallback;
  }

  /*
    attempt to store packet into the queue if a slot is available

    the bucket will allow for id'ing the packet later. Say a configuration packet is sent as bucket 3,
      then the configuration changes before the packet has a chance to send.
      Instead of sending a redundant packet, all bucket-3 packets can be cleared and the new packet queued

    inputs: (uint8_t*) data array, (uint8_t) data length, (uint8_t) packet bucket for ID'ing, (uint32_t) timeout (0 = no timeout)
    returns: (bool) true if packet stored in queue, false otherwise
  */
  bool sendPersistentPacket(uint8_t *data, uint8_t len, uint8_t bucket, uint32_t timeout);

  /*
    send a simple packet, regardless of connection status
    *unless headless master mode is enabled, then a master needs to be established before sending
  */
  int sendSimplePacket(uint8_t *data, uint8_t len);
  void run();

  bool isConnected() { return _connected; }
  uint32_t getRemoteId()
  {
    if (!_headless_master || _connected)
      return _remote_id;
    else
      return 0;
  }

private:
  // target remote node id:
  uint32_t _remote_id;

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
  void _onDataRecv(uint32_t node_id, uint8_t *data, uint8_t len)
  {
    _connection.onDataRecv(node_id, data, len);
    _packet.onDataRecv(node_id, data, len);
  }

  class connection_class
  {
  public:
    const uint32_t DISCONNECT_TIMEOUT = 8000;
    const uint32_t REALIGNMENT_PACKET_POLL = 300;

    connection_class(ESP_Mesh_Slave_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint32_t node_id, uint8_t *data, uint8_t len);

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

    ESP_Mesh_Slave_SubClass *_pointer;
  };
  connection_class _connection{this};

  class packet_class
  {
  public:
    packet_class(ESP_Mesh_Slave_SubClass *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint32_t node_id, uint8_t *data, uint8_t len);

    const uint8_t _MAX_PACKET = 10;        // declare max number of persistent packets allowed to hold
    const uint8_t _SEND_PACKET_POLL = 300; // how often a missed packet will attempt to send

    linkedList _packetBuffer;

    /*
      this is stored into a linked list and pulled when ready to send out
      this is strictly for persistent packets
    */
    struct packet_struct
    {
      uint8_t PID = 0;
      uint8_t bucket = 0;
      uint32_t timeout = 0;
      uint32_t timer = 0;
      uint8_t msg[0];
    };

    uint32_t _sendPacketTimer = 0;

    void _checkPacketTimers(bool checkMinimume = false);
    ESP_Mesh_Slave_SubClass *_pointer;
  };
  packet_class _packet{this};

  // callback fro new packets:
  void (*_recvCallback)(uint8_t *, uint8_t, ESP_Mesh_Slave_SubClass *) = nullptr;
  painlessMesh *_meshPointer;
};

//

// .cpp

//

/*
  ESP_Mesh_Slave_class
*/
void ESP_Mesh_Slave_class::init(painlessMesh *m)
{
  _meshPointer = m;
  _instancePointer = this;

  _meshPointer->onReceive(&__onDataRecv__);
}
void ESP_Mesh_Slave_class::run()
{
  for (uint8_t x = 0; x < __classList.nodeCount(); x++)
    getMaster(x)->run();
}

void ESP_Mesh_Slave_class::addMaster(uint32_t master_id, void (*recvCallback)(uint8_t *, uint8_t, ESP_Mesh_Slave_SubClass *))
{
  // create new instance:
  ESP_Mesh_Slave_SubClass *newMaster = new ESP_Mesh_Slave_SubClass(master_id, _meshPointer, recvCallback);

  if (newMaster == nullptr)
    return;

  // store new instance:
  __classList.appendNode<ESP_Mesh_Slave_SubClass *>(newMaster);
}

void ESP_Mesh_Slave_class::removeMaster(uint16_t index)
{
  if (index < __classList.nodeCount())
  {
    // free memory and remove from list:
    delete getMaster(index);
    __classList.deleteNode(index);
  }
}

void ESP_Mesh_Slave_class::removeAll()
{
  // simple while loop to delete all instances:
  while (__classList.nodeCount())
    removeMaster(0);
}

ESP_Mesh_Slave_SubClass *ESP_Mesh_Slave_class::getMaster(uint16_t index)
{
  if (__classList.getNodeSize(index) < 0)
    return nullptr;

  return *(__classList.getNodeData<ESP_Mesh_Slave_SubClass *>(index));
}

void ESP_Mesh_Slave_class::__onDataRecv__(uint32_t node_id, String msg)
{
  if (_instancePointer == nullptr)
    return;

  /*
    convert base16 data into byte array
  */
  uint8_t msg_arr[(msg.length() / 2)];
  uint16_t arr_len = ESP_Mesh_util::decode_base16(msg.c_str(), msg_arr);

  // go through each instance and find a matching mac address:
  for (uint8_t x = 0; x < _instancePointer->__classList.nodeCount(); x++)
  {
    ESP_Mesh_Slave_SubClass *p = _instancePointer->getMaster(x);

    // check if mac addresses match:
    if ((node_id == p->_remote_id && (!p->_headless_master || p->_connected)) || (p->_headless_master && !p->_connected))
      p->_onDataRecv(node_id, msg_arr, arr_len);
  }
}

//

// .cpp

//

/*
  ESP_Mesh_Slave_SubClass
*/
bool ESP_Mesh_Slave_SubClass::sendPersistentPacket(uint8_t *data, uint8_t len, uint8_t bucket, uint32_t timeout)
{
  if (_packet._packetBuffer.nodeCount() >= _packet._MAX_PACKET)
    _packet._checkPacketTimers(); // attempt to clear any timedout packets

  if (_packet._packetBuffer.nodeCount() < _packet._MAX_PACKET)
  {
    // construct the packet:
    byte b[sizeof(packet_class::packet_struct) + len];
    packet_class::packet_struct *p = reinterpret_cast<packet_class::packet_struct *>(b);
    p->bucket = bucket;
    p->timeout = timeout;
    p->timer = millis();
    p->PID = 0;
    memcpy(p->msg, data, len);

    // add new packet to the tail of
    _packet._packetBuffer.appendNode<packet_class::packet_struct>(p, sizeof(b));
    return true;
  }
  return false;
}

int ESP_Mesh_Slave_SubClass::sendSimplePacket(uint8_t *data, uint8_t len)
{
  if (_connected || !_headless_master)
  {
    /*
      format packet correctly
    */
    uint8_t pck[2 + len];
    pck[0] = ESP_Mesh_util::setCmdPck(6, false);
    pck[1] = 0;
    memcpy(&(pck[2]), data, len);

    char send_c[(sizeof(pck) * 2) + 1];
    ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
    return _meshPointer->sendSingle(_remote_id, send_c);
  }
  return false;
}

void ESP_Mesh_Slave_SubClass::run()
{
  _connection.run();
  _packet.run();
}

/*
  ESP_Mesh_Slave_SubClass::connection_class
*/
void ESP_Mesh_Slave_SubClass::connection_class::run()
{
  if (_pointer->_connected)
  {
    /*
      run packet realignment requests
    */
    if (_pointer->_packetRealignment)
      if (millis() - _realignmentTimer >= REALIGNMENT_PACKET_POLL)
      {
        uint8_t pck[] = {ESP_Mesh_util::setCmdPck(4, false)};
        char send_c[(sizeof(pck) * 2) + 1];
        ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
        _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);

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

void ESP_Mesh_Slave_SubClass::connection_class::onDataRecv(uint32_t node_id, uint8_t *data, uint8_t len)
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
          uint8_t pck[] = {ESP_Mesh_util::setCmdPck(2, false), _pointer->_local_PID};
          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(node_id, send_c);
        }
        break;
      case 2:
        /*
          connection packet:
            slave only expects connection ack packets

            save master's id
        */
        if (len == 2 && ESP_Mesh_util::isAck(data))
        {
          /*
            store master id
          */
          _pointer->_remote_id = node_id;

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
    else if (node_id == _pointer->_remote_id)
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
          uint8_t pck[] = {ESP_Mesh_util::setCmdPck(2, false), _pointer->_local_PID};
          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);
        }
        break;
      case 2:
        /*
          connection packet:
            slave only expects connection ack packets
        */
        if (len == 2 && !(_pointer->_connected) && ESP_Mesh_util::isAck(data))
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
        if (len == 1 && !ESP_Mesh_util::isAck(data))
        {
          /*
            PID alignment request
            send ack packet back
          */
          // if (_pointer->_connected)  // not really necessary
          // {
          uint8_t pck[] = {ESP_Mesh_util::setCmdPck(4, true), _pointer->_local_PID};
          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);
          // }
        }
        else if (len == 2 && ESP_Mesh_util::isAck(data))
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
        if (len == 1 && !ESP_Mesh_util::isAck(data))
        {
          // reset necessary timers:
          _disconnectTimer = millis();

          /*
            send ack packet
          */
          uint8_t pck[] = {ESP_Mesh_util::setCmdPck(5, true)};
          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);
        }
        break;
      }
    }
  }
}

//

/*
  ESP_Mesh_Slave_SubClass::packet_class
*/
void ESP_Mesh_Slave_SubClass::packet_class::run()
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
    if (_packetBuffer.getNodeData<packet_struct>(0)->timeout &&
        millis() - _packetBuffer.getNodeData<packet_struct>(0)->timer >= _packetBuffer.getNodeData<packet_struct>(0)->timeout)
    {
      // timeout reached, remove packet:
      if (_packetBuffer.getNodeData<packet_struct>(0)->PID != 0)
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
      if (_packetBuffer.getNodeData<packet_struct>(0)->PID != 0)
      {
        // currently sending...
        if (_pointer->_connected && !(_pointer->_packetRealignment))
        {
          // check if PIDs don't align:
          if (_pointer->_remote_PID != _packetBuffer.getNodeData<packet_struct>(0)->PID)
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
              uint8_t pck[2 + _packetBuffer.getNodeSize(0) - sizeof(packet_struct)];
              pck[0] = ESP_Mesh_util::setCmdPck(7, false);
              pck[1] = _packetBuffer.getNodeData<packet_struct>(0)->PID;
              memcpy(&(pck[2]), _packetBuffer.getNodeData<packet_struct>(0)->msg, sizeof(pck) - 2);

              char send_c[(sizeof(pck) * 2) + 1];
              ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
              _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);
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
          _packetBuffer.getNodeData<packet_struct>(0)->PID = (_pointer->_remote_PID ? _pointer->_remote_PID : ++(_pointer->_remote_PID));
          _sendPacketTimer = millis();

          /*
            construct the packet:
          */
          uint8_t pck[2 + _packetBuffer.getNodeSize(0) - sizeof(packet_struct)];
          pck[0] = ESP_Mesh_util::setCmdPck(7, false);
          pck[1] = _packetBuffer.getNodeData<packet_struct>(0)->PID;
          memcpy(&(pck[2]), _packetBuffer.getNodeData<packet_struct>(0)->msg, sizeof(pck) - 2);

          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);
        }
      }
    }
  }
}
void ESP_Mesh_Slave_SubClass::packet_class::onDataRecv(uint32_t node_id, uint8_t *data, uint8_t len)
{
  // verify mac and command byte length:
  if (node_id == _pointer->_remote_id && len)
  {
    switch (data[0] & B00001111)
    {
    case 6:
      /*
        simple packet
        no acknowledgment, duplicates allowed. PID byte for future mesh system (can ignore)
      */
      if (!ESP_Mesh_util::isAck(data) && len > 1)
      {
        _pointer->_recvCallback(&data[2], len - 2, _pointer);
      }
      break;
    case 7:
      /*
        persistent packet Command
        this can either be an ack or new packet
      */
      if (ESP_Mesh_util::isAck(data))
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

          uint8_t pck[2] = {ESP_Mesh_util::setCmdPck(7, true), _pointer->_local_PID};

          char send_c[(sizeof(pck) * 2) + 1];
          ESP_Mesh_util::encode_base16(send_c, pck, sizeof(pck));
          _pointer->_meshPointer->sendSingle(_pointer->_remote_id, send_c);

          if (msgReceived)
            _pointer->_recvCallback(&data[2], len - 2, _pointer);
        }
      }
      break;
    }
  }
}

void ESP_Mesh_Slave_SubClass::packet_class::_checkPacketTimers(bool checkMinimume /* = false */)
{
  /*
      check for packet timeouts:
      unkown how much this might slow down the program:
      starting from back of list because packet list might shrink as searching
  */
  if (_packetBuffer.nodeCount())
    for (int16_t x = _packetBuffer.nodeCount() - 1; x >= 0; x--)
    {
      if (_packetBuffer.getNodeData<packet_struct>(0)->timeout)
        if (millis() - _packetBuffer.getNodeData<packet_struct>(0)->timer >= _packetBuffer.getNodeData<packet_struct>(0)->timeout)
        {
          // timeout reached, remove packet:
          if (_packetBuffer.getNodeData<packet_struct>(0)->PID != 0)
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

#endif // #ifndef ESP_Mesh_Slave_h
