#include "LinkedList.h"

// .cpp

/* initialize 'base_node' struct */
linkedList::base_node::base_node() : nextNode(nullptr) {}

/* initialize 'node' struct */
linkedList::node::node() : length(0) {}

linkedList::~linkedList()
{
  /* destroy each node using 'deleteNode' method */
  base_node *temp = &_headNode;

  while (temp->nextNode)
    deleteNode(0);
}

void *linkedList::addNode(uint16_t index, void *data, uint16_t length)
{
  struct template_struct : node
  {
    byte data[0];
  };

  // template_struct *newNode = (template_struct *)aligned_alloc(alignof(template_struct), sizeof(template_struct) + length); // get an undefined reference error here
  template_struct *newNode = (template_struct *)malloc(sizeof(template_struct) + length);

  if (!newNode)
    return nullptr;

  newNode->length = length;

  if (data)
    memcpy(newNode->data, data, length); // << write to the bytes trailing the node

  base_node *storeNode = &_headNode;

  // locate target location
  for (uint16_t nodeCnt = 0; nodeCnt < index && storeNode->nextNode; nodeCnt++)
    storeNode = storeNode->nextNode;

  // relink list with new node:
  newNode->nextNode = storeNode->nextNode;
  storeNode->nextNode = newNode;
  return (newNode->data);
}

void linkedList::deleteNode(uint16_t index)
{
  base_node *temp = &_headNode;

  /* located target node: */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode) // validate if is valid node
  {
    void *deleteNode = /* reinterpret_cast<void *> */ (temp->nextNode); // get target node as byte array to delete instead of object.
    temp->nextNode = temp->nextNode->nextNode;                          // relink list

    free(deleteNode); // << delete as void ptr
  }
}

void *linkedList::getNodeData(uint16_t index)
{
  struct template_struct : node
  {
    byte data[0];
  };

  base_node *temp = &_headNode;

  /* find target node */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode)
  {
    /* retrieve data as a byte-array */
    template_struct *returnNode = reinterpret_cast<template_struct *>(temp->nextNode);

    // return as intended data type
    return reinterpret_cast<void *>(returnNode->data);
  }
  return nullptr; // index out of bounds
}

int32_t linkedList::getNodeSize(uint16_t index)
{
  base_node *temp = &_headNode;

  /* located target node: */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode) // validate if is valid node
  {
    return temp->nextNode->length; // return stored length
  }
  return -1; // return invalid node
}

uint16_t linkedList::nodeCount()
{
  base_node *temp = &_headNode;
  uint16_t nodeCnt = 0;

  /*
    run through entire linked list, counting each node
  */
  while (temp->nextNode)
  {
    temp = temp->nextNode;
    nodeCnt++;
  }

  return nodeCnt;
}
