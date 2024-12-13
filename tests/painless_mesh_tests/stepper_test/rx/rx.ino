#include <painlessMesh.h>
#include "C:\Users\AVG\Documents\Electrical_Main\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\StepperDriver\esp8266\StepperDriver_2.1.3.h"

#define MESH_SSID "ESP_NET"
#define MESH_PASSWORD "pass_8888"
#define MESH_PORT 5555

#define STEP_PIN 5 // D1
#define DIR_PIN 4  // D2

void stepperCallBack(bool, bool);
// Prototypes
// void sendMessage();
void receivedCallback(uint32_t from, String &msg);
// void newConnectionCallback(uint32_t nodeId);
// void changedConnectionCallback();
// void nodeTimeAdjustedCallback(int32_t offset);
// void delayReceivedCallback(uint32_t from, int32_t delay);

StepperDriver stepper(stepperCallBack);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;


void setup()
{
  Serial.begin(115200);

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  stepper.setMaxSpeed(500);
  stepper.setAccel(800);

  mesh.onReceive(&receivedCallback);
}

void loop()
{

  mesh.update();
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  stepper.moveTo(msg.toInt());
}

void stepperCallBack(bool par, bool dir)
{
  digitalWrite(DIR_PIN, dir);
  digitalWrite(STEP_PIN, par);
}