//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include <Arduino.h>
#include <painlessMesh.h>

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain
void checkMem();    // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);
Task taskCheckMem(TASK_SECOND * 1, TASK_FOREVER, &checkMem);

std::list<uint32_t> node_list;

void checkMem()
{
  Serial.print(ESP.getFreeHeap());
  Serial.print(",");
  Serial.println(ESP.getMaxFreeBlockSize());
  taskCheckMem.setInterval((TASK_MILLISECOND * 500));
}

void sendMessage()
{
  // String msg = "Hello World using String";
  // mesh.sendBroadcast(msg);

  char c_msg_tx[] = "Hello World using char array";
  std::list<uint32_t> node_list = mesh.getNodeList();
  for (std::list<uint32_t>::iterator it = node_list.begin(); it != node_list.end(); ++it)
    mesh.sendSingle(*it, c_msg_tx);
  taskSendMessage.setInterval((TASK_SECOND * 1));
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  for (std::list<uint32_t>::iterator it = node_list.begin(); it != node_list.end(); ++it)
    if (*it == nodeId)
      return;
  node_list.push_back(nodeId);

  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  // Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
  Serial.begin(115200);
  // Serial.println("\nStarting PainlessMesh Node New");
  // Serial.println("free_heap, largest_block");

  // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE); // all types on
  // mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  // mesh.onChangedConnections(&changedConnectionCallback);
  // mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(taskCheckMem);
  taskCheckMem.enable();
}

void loop()
{
  // it will run the user scheduler as well
  mesh.update();
  yield();
}
