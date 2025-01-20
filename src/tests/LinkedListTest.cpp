/*
  AI generated test
*/

#include <Arduino.h>
#include <unity.h>
#include "LinkedList.h"

void test_addNode() {
    linkedList list;
    TEST_ASSERT_TRUE(list.addNode(0, 10));
    TEST_ASSERT_EQUAL(1, list.nodeCount());
    TEST_ASSERT_EQUAL(10, list.getNodeSize(0));
}

void test_deleteNode() {
    linkedList list;
    list.addNode(0, 10);
    list.deleteNode(0);
    TEST_ASSERT_EQUAL(0, list.nodeCount());
}

void test_getNodeData() {
    linkedList list;
    int data = 123;
    list.addNode(0, &data, sizeof(data));
    int* retrievedData = static_cast<int*>(list.getNodeData(0));
    TEST_ASSERT_NOT_NULL(retrievedData);
    TEST_ASSERT_EQUAL(data, *retrievedData);
}

void test_nodeCount() {
    linkedList list;
    list.addNode(0, 10);
    list.addNode(1, 20);
    TEST_ASSERT_EQUAL(2, list.nodeCount());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_addNode);
    RUN_TEST(test_deleteNode);
    RUN_TEST(test_getNodeData);
    RUN_TEST(test_nodeCount);
    UNITY_END();
}

void loop() {
    // Empty loop
}
