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

void linkedList::deleteNode(uint16_t index)
{
  base_node *temp = &_headNode;

  /* located target node: */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode) // validate if is valid node
  {
    uint8_t *deleteNode = reinterpret_cast<uint8_t *>(temp->nextNode); // get target node as byte array to delete instead of struct (originally allocated as byte array)
    temp->nextNode = temp->nextNode->nextNode;                         // relink list

    delete[] deleteNode; // << delete as list
  }
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
