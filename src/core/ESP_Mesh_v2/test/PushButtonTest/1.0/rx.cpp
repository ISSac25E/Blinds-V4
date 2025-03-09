#include <Arduino.h>
#include "../../../src/ESP_Mesh.h"

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

#define MESH_PREFIX "ESP_Mesh_Net"
#define MESH_PASSWORD "MeshPass"
#define MESH_PORT 5555

// Define LED and pushbutton pins
#define STATUS_LED 2
#define STATUS_BUTTON 0

/*
  bucket structs to publish and subscribe to
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

simple_LED led_tx;

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
      connection = ESP_Mesh::addConnection(node, nullptr);
      Serial.print("New node added: 0x");
      Serial.println(node, HEX);

      /*
        do bucket initialization
      */
      // connection->bucket_publish(0, &myinfo, sizeof(device_info));
      // connection->bucket_subscribe(0);

      connection->bucket_publish(4, &led_tx, sizeof(simple_LED));
      connection->bucket_subscribe(3);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("init PushButton Test RX");

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onChangedConnections(reScanConnections);
  mesh.setRoot(true);

  // Print this node's info
  Serial.print("This node ID: 0x");
  Serial.println(mesh.getNodeId(), HEX);

  // ESP_Mesh::onNewConnection(newConnCallback);
  ESP_Mesh::init(&mesh);

  pinMode(STATUS_LED, OUTPUT);
  pinMode(STATUS_BUTTON, INPUT_PULLUP);
}

void loop()
{
  ESP_Mesh::run();
  ESP_Mesh::checkBucketPublish();

  // debouncing
  static unsigned long button = 0;
  if (millis() - button >= 10)
  {
    button = millis();
    led_tx.state = digitalRead(STATUS_BUTTON);
  }

  for (auto it = ESP_Mesh::getConnections()->begin(); it != ESP_Mesh::getConnections()->end(); ++it)
  {
    if (it.getNodeData<ESP_Mesh_Connection>()->getSubscribedBucketSize(3) == sizeof(simple_LED))
      digitalWrite(STATUS_LED, static_cast<simple_LED *>(it.getNodeData<ESP_Mesh_Connection>()->getSubscribedBucket(3))->state);
  }
}