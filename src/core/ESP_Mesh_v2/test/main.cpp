#include <Arduino.h>
#include "../src/ESP_Mesh.h"

linkedList ll;
ESP_Mesh_Master_class mesh;

void setup()
{
  ll.addNode<int>(0, 45);
  mesh.myfunct();
}

void loop()
{
}