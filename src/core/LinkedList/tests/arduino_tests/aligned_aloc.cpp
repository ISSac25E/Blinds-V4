#include <Arduino.h>
#include "../../LinkedList.h"

linkedList myList;

void setup()
{
  struct super_struct
  {
    super_struct()
    {
      Serial.println("super_struct constructor");
    }
    ~super_struct()
    {
      Serial.println("super_struct deconstructor");
    }
    void addChar(const char *new_c)
    {
      int index = 0;
      while (new_c[index] != '\0')
      {
        c[index] = new_c[index];
        index++;
      }
      c[index] = '\0';
    }
    float x;
    char c[];
  };

  delay(500);

  Serial.begin(115200);
  Serial.printf("\nSize of super_struct: %d\n", sizeof(super_struct));

  char c[] = "hello";

  myList.addNode<super_struct>(0, sizeof(c));
  super_struct *myStruct = (myList.getNodeData<super_struct>(0));
  new (myStruct) super_struct{}; // call constructor manually

  myStruct->addChar(c);

  Serial.printf("node size: %u\n", myList.getNodeSize(0));
  myStruct->x = 123.45;
  Serial.printf("myStruct->x: %f\n", myStruct->x);

  myList.deleteNode<super_struct>(0);
}

void loop()
{
}