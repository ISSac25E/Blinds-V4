// see readme

/*
  changelog
    TODO
*/

#ifndef ESP_Mesh_h
#define ESP_Mesh_h

// external dependencies:
#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

// local dependencies
#include "../../LinkedList/LinkedList.h"

// internal dependencies
#include "ESP_Mesh_Utility.h"
#include "ESP_Mesh_Connection.h"

#ifndef painlessMesh_MOD_mesh_h_176
#error "painlessMesh_MOD_mesh_h_176 not implemented"
#endif
#ifndef painlessMesh_MOD_mesh_h_189
#error "painlessMesh_MOD_mesh_h_189 not implemented"
#endif

/*
  This will be the single manager class to handle all connection instances
  This is a static class with no instantiatable members
*/
class ESP_Mesh
{
  // this is for the JSON object access
  friend class ESP_Mesh_Connection;

public:
  /*
    give an instance of the painless mesh for data transmission
  */
  static void init(painlessMesh *);

  /*
    run all connections instances and clean up tasks
  */
  static void run();

  /*
    create a new connection.
    set target device id and packet receive callback

    set as master or slave(default)

    setting node_id as 0 and slave will create a headless slave
  */
  static void addConnection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *), bool is_master = false);

  /*
    remove a connection at index of connection list
  */
  static void removeConnectionIndex(uint16_t connection_index);

  /*
    remove specific connection node id
  */
  static void removeConnection(uint32_t node_id);

  /*
    close and delete all connections
  */
  static void removeAll();

  /*
    get conenction at index

    null if invalid
  */
  static ESP_Mesh_Connection *getConnectionIndex(uint16_t conenction_index);

  /*
    get first instance of connection with specific node id

    null if invalid
  */
  static ESP_Mesh_Connection *getConnection(uint32_t node_id);

  /*
    returns the list of all connections
  */
  static linkedList *getConnections();

private:
  /* resource lock to create thread safety */
  static ESP_Mesh_util::rscSync _resourceLock;

  /* contains all class pointers for each slave device: */
  static linkedList _classList;

  static void _runBeacon();

  /* callback function for painlessMesh
     this can simply invoke the callback functions from every connection instance */
  static void _onDataRecv(uint32_t nodeId, String msg);
};
#endif