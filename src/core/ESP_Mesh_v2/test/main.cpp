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
// struct myInfo
// {
//   bool init = false;
//   char c[10];
// };

void callBack(const char *msg, ESP_Mesh_Connection *connection)
{
  Serial.println("Received message:");
  Serial.println(msg);
  Serial.print("From node: ");
  Serial.println(connection->getRemoteId(), HEX);
}

void handleConnections()
{
  /*
    scan for new connections and add them
    don't do too often not to overwhelm
  */
  static uint32_t lastScan_timer = 0;

  if (millis() - lastScan_timer >= 15000)
  {
    lastScan_timer = millis();

    std::list<uint32_t> nodes = mesh.getNodeList();

    for (auto node : nodes)
    {
      ESP_Mesh_Connection *connection = ESP_Mesh::getConnection(node);
      if (!connection)
      {
        ESP_Mesh::addConnection(node, callBack);
        Serial.print("New node: 0x");
        Serial.print(node, HEX);
        Serial.print(" ");

        /*
          do bucket initialization
        */
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  ESP_Mesh::init(&mesh);
}

void loop()
{
  mesh.update();
  ESP_Mesh::run();
  handleConnections();

  /*
    check buckets and connections status changes
  */
}
