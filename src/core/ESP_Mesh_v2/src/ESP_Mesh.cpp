#include "ESP_Mesh.h"

// initialize pointer so there are no invalid dereferencing:
painlessMesh *ESP_Mesh_Master_class::_meshPointer = nullptr;

void ESP_Mesh_Master_class::init(painlessMesh *meshPointer)
{
  /*
    reset all connections
  */
  ESP_Mesh_Master_class::removeAll();

  ESP_Mesh_Master_class::_meshPointer = meshPointer;
}

void ESP_Mesh_Master_class::run()
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;

  /*
    - run all connections instances in the linked list
  */
}

void ESP_Mesh_Master_class::addConnection(uint32_t node_id, void (*recvCallback)(uint32_t node_id, String msg), bool is_master/*  = false */)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;

  /*
    - check that not headless and master
    - check if existing connection exists. If so, remove all and reinitialize connection
  */
}

void ESP_Mesh_Master_class::removeConnectionIndex(uint16_t connection_index)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;

  /*
    - find connection at index of list and remove (if exists)
  */
}

void ESP_Mesh_Master_class::removeConnection(uint32_t node_id)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;

  /*
    - find all connections with this node id
      exempt headless connections?
  */
}

void ESP_Mesh_Master_class::removeAll()
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;

  /*
    - remove all connections
  */
}

ESP_Mesh_Connection *ESP_Mesh_Master_class::getConnectionIndex(uint16_t conenction_index)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return nullptr;

  /*
    - get connection at index
  */
    return nullptr;
}

ESP_Mesh_Connection *ESP_Mesh_Master_class::getConnection(uint32_t node_id)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return nullptr;

  /*
    - get first connection with matched target id
  */
    return nullptr;
}

linkedList *ESP_Mesh_Master_class::getConnections()
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return nullptr;

  /*
    - return linkedlist with all connections
  */
    return nullptr;
}

void ESP_Mesh_Master_class::_onDataRecv(uint32_t node_id, String msg)
{
  if (!ESP_Mesh_Master_class::_meshPointer) // cant be nullptr
    return;
  
  /*
    - search through all connections and forward to all with matching node id
      if headless connections, forward all messages only if disconnected
  */
}