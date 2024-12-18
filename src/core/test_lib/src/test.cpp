#include "test.h"

test my_test;

test::test() {
  Serial.begin(115200);
}

void test::print() {
  Serial.println("print");
}