#include "ESP_Mesh.h"

// initialize pointer so there are no invalid dereferencing:
linkedList ESP_Mesh::_classList;
ESP_Mesh_util::rscSync ESP_Mesh::_resourceLock; // TODO: implement

void ESP_Mesh::init(painlessMesh *m)
{
  /*
    reset all connections
  */
  removeAll();

  ESP_Mesh_util::meshPointer = m;
  if (ESP_Mesh_util::meshPointer)
    ESP_Mesh_util::meshPointer->onReceive(&_onDataRecv);
}

void ESP_Mesh::run()
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  for (auto &connection : _classList)
    connection.getNodeData<ESP_Mesh_Connection>()->run();
}

void ESP_Mesh::addConnection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *), bool is_master /*  = false */)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  /*
    delete any existing connections
  */
  removeConnection(node_id);

  /*
    create new connection at a new node
  */
  if (!_classList.addNode<ESP_Mesh_Connection>(0))
    return;

  /*
    initialize new connection using placement new
  */
  new (_classList.getNodeData<ESP_Mesh_Connection>(0)) ESP_Mesh_Connection(node_id, recvCallback, is_master);
}

void ESP_Mesh::removeConnectionIndex(uint16_t connection_index)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  _classList.deleteNode<ESP_Mesh_Connection>(connection_index); // delete node, calls deconstructor
}

void ESP_Mesh::removeConnection(uint32_t node_id)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  // search for matching node id
  for (auto &connection : _classList)
  {
    if (connection.getNodeData<ESP_Mesh_Connection>()->_remote_node_id == node_id)
    {
      connection.deleteNode<ESP_Mesh_Connection>(); // delete node, calls deconstructor
    }
  }
}

void ESP_Mesh::removeAll()
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  for (auto &connection : _classList)
    connection.deleteNode<ESP_Mesh_Connection>(); // delete node, calls deconstructor
}

ESP_Mesh_Connection *ESP_Mesh::getConnectionIndex(uint16_t connection_index)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return nullptr;

  return _classList.getNodeData<ESP_Mesh_Connection>(connection_index);
}

ESP_Mesh_Connection *ESP_Mesh::getConnection(uint32_t node_id)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return nullptr;

  // get the first connection with match node id
  for (auto &connection : _classList)
  {
    if (connection.getNodeData<ESP_Mesh_Connection>()->_remote_node_id == node_id)
      return connection.getNodeData<ESP_Mesh_Connection>();
  }
  return nullptr;
}

linkedList *ESP_Mesh::getConnections()
{
  /*
    return linkedlist with all connections
  */
  return &_classList;
}

void ESP_Mesh::_onDataRecv(uint32_t node_id, String msg)
{
  if (!ESP_Mesh_util::meshPointer) // cant be nullptr
    return;

  /*
    parse json:
  */
  if (deserializeJson(ESP_Mesh_util::jsonDocRx, msg))
  {
    // invalid json
    return;
  }

  for (auto &connection : _classList)
  {
    if (connection.getNodeData<ESP_Mesh_Connection>()->_remote_node_id == node_id)
    {
      // forward message to connection instance
      connection.getNodeData<ESP_Mesh_Connection>()->_onDataRecv(node_id);
    }
  }
}