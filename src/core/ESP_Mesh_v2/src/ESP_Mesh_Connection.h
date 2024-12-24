#ifndef ESP_Mesh_Connection_h
#define ESP_Mesh_Connection_h

#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

class ESP_Mesh_Master_class; // forward declaration

/*
  a connection instance.
  each instance can be setup as a master or slave device
*/
class ESP_Mesh_Connection
{
  friend class ESP_Mesh_Master_class;

public:
  /*
    This is where the behavior of the connection will be determined
    set node_id to zero to setup as a headless slave. is_master must be false(default)
  */
  ESP_Mesh_Connection(painlessMesh *meshPointer, uint32_t node_id, void (*recvCallback)(uint32_t node_id, String msg), bool is_master = false);

private:
  // determines if this connection instance behaves as master or slave 
  bool _is_master;

  /* 
    headless mode can only be used as a slave device.
    headless master would mean broadcasting a beacon.
    A different mechanism is needed for headless master
  */
  bool _headless_mode;

  // remote node to target
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


  /*
    TODO: should master class format incoming packet as JSON?
  */
  static void _onDataRecv(uint32_t nodeId, String msg);
};

#endif