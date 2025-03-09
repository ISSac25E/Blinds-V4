#include <Arduino.h>
#include "../src/ESP_Mesh.h"

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

#define MESH_PREFIX "ESP_Mesh_Net"
#define MESH_PASSWORD "MeshPass"
#define MESH_PORT 5555

/*
  bucket struct to publish and subscribe to
*/
struct device_info
{
  bool init = false;
  char c[10];
};

struct simple_LED
{
  bool state = false;
};

device_info myinfo;

void callBack(const char *msg, ESP_Mesh_Connection *connection)
{
  Serial.println("Received message: ");
  Serial.println(msg);
  Serial.print("From node: ");
  Serial.println(connection->getRemoteId(), HEX);
}

void reScanConnections()
{
  /*
    scan for new connections and add them
    don't call too often not to overwhelm
  */

  std::list<uint32_t> nodes = mesh.getNodeList();

  for (auto node : nodes)
  {
    ESP_Mesh_Connection *connection = ESP_Mesh::getConnection(node);
    if (!connection)
    {
      connection = ESP_Mesh::addConnection(node, callBack);
      Serial.print("New node added: 0x");
      Serial.println(node, HEX);

      /*
        do bucket initialization
      */
      connection->bucket_publish(0, &myinfo, sizeof(device_info));
      connection->bucket_subscribe(0);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("init");

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onChangedConnections(reScanConnections);

  // ESP_Mesh::onNewConnection(newConnCallback);
  ESP_Mesh::init(&mesh);

  myinfo.init = true;
  myinfo.c[0] = 'H';
  myinfo.c[1] = 'e';
  myinfo.c[2] = 'l';
  myinfo.c[3] = 'l';
  myinfo.c[4] = 'o';
  myinfo.c[5] = 0;
}

void loop()
{
  mesh.update();
  ESP_Mesh::run();

  /*
    check buckets and connections status changes
  */
  static uint32_t bucket_timer = 0;
  if (millis() - bucket_timer >= 100)
  {
    bucket_timer = millis();

    linkedList *connections = ESP_Mesh::getConnections();
    for (linkedList::Iterator connection = connections->begin(); connection != connections->end(); ++connection)
    {
      connection.getNodeData<ESP_Mesh_Connection>()->checkBucketPublish();
    }
  }
}
