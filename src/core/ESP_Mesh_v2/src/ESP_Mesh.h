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

// #ifndef painlessMesh_MOD_configuration_h_20
// #error "painlessMesh_MOD_configuration_h_20 not implemented"
// #endif
// #ifndef painlessMesh_MOD_configuration_h_23
// #error "painlessMesh_MOD_configuration_h_23 not implemented"
// #endif
// #ifndef painlessMesh_MOD_configuration_h_25
// #error "painlessMesh_MOD_configuration_h_25 not implemented"
// #endif
// #ifndef painlessMesh_MOD_configuration_h_28
// #error "painlessMesh_MOD_configuration_h_28 not implemented"
// #endif

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
    setup a callback for new unexpected connections
  */
  static void onNewConnection(ESP_Mesh_Connection *(*newConnCallback)(uint32_t node_id))
  {
    _newConnCallback = newConnCallback;
  }

  /*
    run all connections instances and clean up tasks
  */
  static void run();

  /*
    create a new connection.
    set target device id and packet receive callback

    returns newly created instance
  */
  static ESP_Mesh_Connection *addConnection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *));

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

  /*
    runs bucket publish on all connections instances
  */
  static void checkBucketPublish();

private:
  /* resource lock to create thread safety */
  static ESP_Mesh_util::rscSync _resourceLock;

  /* contains all class pointers for each slave device: */
  static linkedList _classList;
  static ESP_Mesh_Connection *(*_newConnCallback)(uint32_t node_id);

  /* callback function for painlessMesh
     this can simply invoke the callback functions from every connection instance */
  static void _onDataRecv(uint32_t nodeId, String msg);
};
#endif