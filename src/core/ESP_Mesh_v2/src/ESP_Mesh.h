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
#include "ESP_Mesh_Connection.h"

/*
  This will be the single manager class to handle all connection instances
  This is a static class with no instantiatable members
*/
class ESP_Mesh_Master_class
{
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
  static void addConnection(uint32_t node_id, void (*recvCallback)(uint32_t node_id, String msg), bool is_master = false);

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
  static painlessMesh *_meshPointer;

  /* callback function for painlessMesh
     this can simply invoke the callback functions from every connection instance */
  static void _onDataRecv(uint32_t nodeId, String msg);

  /* contains all class pointers for each slave device: */
  static linkedList _classList;
};
#endif