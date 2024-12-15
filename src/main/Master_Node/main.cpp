#include <Arduino.h>
#include <my_wierd_header.h>

void setup()
{
  Serial.begin(115200);

  my_test my_test_obj;
  my_test_obj.print();
}

void loop()
{
}