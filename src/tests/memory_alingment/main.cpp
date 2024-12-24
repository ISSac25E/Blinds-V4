#include "../../core/LinkedList/LinkedList.h"
/*
  this will demonstrate how dangerous misaligned memory can be.
  This will store a floating point in two methods and attempt to access it. One of which will MOST LIKELY cause a crash
*/

linkedList list;

void setup()
{
  Serial.begin(115200);
  Serial.println("\ninit\n");

  Serial.println("initializing correctly in 3s...");
  delay(3000);

  list.addNode<float>(0);
  *(list.getNodeData<float>(0)) = 123.45; 
  Serial.println(*(list.getNodeData<float>(0)));
  Serial.println();

  Serial.println("initializing incorrectly in 3s...");
  delay(3000);

  list.addNode(0, sizeof(float)); // << this is fine. You can store any data anywhere. When you dereference it as a data type, then you get serious issues
  *(reinterpret_cast<float*>(list.getNodeData(0))) = 123.45; // this can crash 3 in 4 chances
  Serial.println(*(reinterpret_cast<float*>(list.getNodeData(0))) ); // this can crash 3 in 4 chances
  Serial.println();
}

void loop()
{
}
