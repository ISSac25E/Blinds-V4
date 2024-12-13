#include <painlessMesh.h>

#define MESH_SSID "ESP_NET"
#define MESH_PASSWORD "pass_8888"
#define MESH_PORT 5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

void setup()
{
  Serial.begin(115200);
  Serial.println("init");
  pinMode(A0, INPUT);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
}

void loop()
{
  mesh.update();

  static uint32_t timer = 0;
  if (millis() - timer >= 100)
  {
    String msg = String(map(analogRead(A0), 0, 1023, 0, 5000));
    Serial.print(msg);
        mesh.sendBroadcast(msg);
    timer = millis();
  }
}