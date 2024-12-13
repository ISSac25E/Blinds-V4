#include "painlessMesh.h"
#include "C:\Users\AVG\Documents\Electrical_Main\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP_Mesh\ESP_Mesh_Master_1.0.0.h"

void callBack(uint8_t *, uint8_t, ESP_Mesh_Master_SubClass *);

uint32_t slave_id = 0xC0FF7032;

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

#define MESH_PREFIX "ESP_Mesh_Net"
#define MESH_PASSWORD "MeshPass"
#define MESH_PORT 5555

void setup()
{
  Serial.begin(115200);
  // mesh.setDebugMsgTypes(0); // set before init() so that you can see startup messages
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  Serial.print("Master Node id (0x");
  Serial.print(mesh.getNodeId(), HEX);
  Serial.println(")");

  ESP_Now_Master.init(&mesh);
  ESP_Now_Master.addSlave(slave_id, callBack);
}

void loop()
{
  mesh.update();
  ESP_Now_Master.run();

  {
    static bool connected = true;
    if (connected != ESP_Now_Master.getSlave(0)->isConnected())
    {
      connected = !connected;
      Serial.println(connected ? "connected" : "disconnected");
    }
  }
  if (Serial.available())
  {
    String s = Serial.readString();
    s = s.substring(0, s.length() - 1);
    // has a slight delay but its fine
    if (s.length() <= 100)
    {
      Serial.print("TX to (");

      Serial.print("0x");
      Serial.print(ESP_Now_Master.getSlave(0)->getRemoteId(), HEX);

      Serial.print("): \"");

      Serial.print(s);
      Serial.println("\"");

      uint8_t data[s.length()];
      for (uint8_t x = 0; x < s.length(); x++)
      {
        data[x] = s[x];
      }

      ESP_Now_Master.getSlave(0)->sendPersistentPacket(data, sizeof(data), 0 /*data bucket*/, 0 /*timeout (ms)*/);
    }
    else
    {
      Serial.println("ERROR: msg to long");
    }
  }
}

void callBack(uint8_t *data, uint8_t len, ESP_Mesh_Master_SubClass *pnt)
{
  Serial.print("RX from (");

  Serial.print("0x");
  Serial.print(pnt->getRemoteId(), HEX);

  Serial.print("): \"");
  for (uint8_t x = 0; x < len; x++)
  {
    Serial.print((char)data[x]);
  }
  Serial.println("\"");
}