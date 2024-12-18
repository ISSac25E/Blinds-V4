// see readme

/*
  changelog
    changes from v1
      TODO
*/

#ifndef ESP_Mesh_h

// external dependencies:
#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

// internal dependencies
#include "../LinkedList/LinkedList_1.0.3.h"

//////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// .h /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

/*
  // forward declaration
*/
class ESP_Mesh_SubClass;
class ESP_Mesh_SubClass_Outgoing;

/*
  This class has one instance that will be created for the user
  This instance will manage all incoming and outgoing connections dynamically
*/

class ESP_Mesh_class
{
  friend class ESP_Mesh_SubClass_Outgoing;

public:
  /*
    give an initialized instance to esp mesh
  */
  void init(painlessMesh *);

  /*
    handles all instances and maintains connections
  */
  void run();

  /*
    add a target connection to static list
    Won't add if id already exists in target connections
  */
  void addTargetConnection(uint32_t target_id, void (*recvCallBack)(String, ESP_Mesh_SubClass *));

  /*
    disconnect and remove a specific connection
    dynamic connections may reappear again shortly after
  */
  void removeConnection(uint16_t index);

  /*
    remove all connection instances
    dynamic connections may reaper shortly after
  */
  void removeAll();

  /*
    get connection at specified index.
    will return nullptr if index doesn't exist
  */
  ESP_Mesh_SubClass *getConnection(uint16_t);

  // /*
  //   get number of connections opened
  // */
  // uint32_t getConnectionCount();

private:
  /* for use with static callback functions */
  static ESP_Mesh_class *_instancePointer;
  painlessMesh *_meshPointer;

  /*
    callback function for painlessMesh
    this will simply invoke the callback function from every connection instance
  */
  static void __onDataRecv__(uint32_t nodeId, String msg);

  /* contains all class pointers for each connection instance */
  linkedList __classList;
};
// init static class pointer:
ESP_Mesh_class *ESP_Mesh_class::_instancePointer = nullptr;
ESP_Mesh_class ESP_Mesh; // << make single static instance

/*
  this is the base class for each connection
*/
class ESP_Mesh_SubClass
{
  friend class ESP_Mesh_class;             // allow the main class to control private functions
  friend class ESP_Mesh_SubClass_Outgoing; // derived class

public:
  ESP_Mesh_SubClass();

  /*
    send a simple packet, regardless of connection status
  */
  bool sendSimplePacket(String);
  void run();

  virtual ~ESP_Mesh_SubClass() {}

private:
  // target remote node id:
  uint32_t _remote_id;

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

  // call back fro new packets:
  void (*_recvCallback)(String, ESP_Mesh_SubClass *) = nullptr;
};

class ESP_Mesh_SubClass_Outgoing : public ESP_Mesh_SubClass
{
  friend class ESP_Mesh_class;

public:
  ESP_Mesh_SubClass_Outgoing(uint32_t target_id, void (*recvCallback)(String, ESP_Mesh_SubClass *))
  {
    _remote_id = target_id;

    _recvCallback = recvCallback;
  }

private:
};

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////// .cpp ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

void ESP_Mesh_class::init(painlessMesh *m)
{
  _meshPointer = m;
  _instancePointer = this;

  _meshPointer->onReceive(&__onDataRecv__);
}

void ESP_Mesh_class::run()
{
  for (uint16_t x = 0; x < __classList.nodeCount(); x++)
    getConnection(x)->run();
}

void ESP_Mesh_class::addTargetConnection(uint32_t target_id, void (*recvCallback)(String, ESP_Mesh_SubClass *))
{
  /*
    search for existing outgoing connection with this id
  */
  for (uint16_t x = 0; x < __classList.nodeCount(); x++)
  {
    ESP_Mesh_SubClass *pnt = *(__classList.getNodeData<ESP_Mesh_SubClass *>(x));
    /*
      must equal the target id and be an outgoing connection
    */
    if (pnt->_remote_id == target_id && dynamic_cast<ESP_Mesh_SubClass_Outgoing *>(pnt))
    {
      removeConnection(x);
      // break; // no break, remove all instances to be sure. There should be no more than one
    }
  }

  /*
    create new connection
  */
  ESP_Mesh_SubClass_Outgoing *newTarget = new ESP_Mesh_SubClass_Outgoing(target_id, recvCallback);

  if (newTarget == nullptr)
    return;

  __classList.appendNode<ESP_Mesh_SubClass *>(newTarget);
}

void ESP_Mesh_class::removeConnection(uint16_t index)
{
  ESP_Mesh_SubClass *target = *(__classList.getNodeData<ESP_Mesh_SubClass *>(index));

  if (target == nullptr)
    return;

#error TODO: deconstruct properly
}

void ESP_Mesh_class::removeAll()
{
  while (__classList.nodeCount())
    removeConnection(0);
}

void ESP_Mesh_class::__onDataRecv__(uint32_t node_id, String msg)
{
  if (_instancePointer == nullptr)
    return;

  DynamicJsonDocument inputParse(1024);

  if (deserializeJson(inputParse, msg))
  {
    
  }
}

#endif