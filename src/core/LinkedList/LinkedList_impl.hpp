template <typename T>
bool linkedList::addNode(uint16_t index)
{
  struct template_struct : node
  {
    T data;
  };

  // allocate memory
  template_struct *newNode = new (std::nothrow) template_struct;

  if (!newNode)
    return false;

  newNode->length = sizeof(T);
  
  base_node *storeNode = &_headNode;

  // locate target location
  for (uint16_t nodeCnt = 0; nodeCnt < index && storeNode->nextNode; nodeCnt++)
    storeNode = storeNode->nextNode;

  // relink list with new node:
  newNode->nextNode = storeNode->nextNode;
  storeNode->nextNode = newNode;
  return true;
}

template <typename T>
void linkedList::deleteNode(uint16_t index)
{
  struct template_struct : node
  {
    T data;
  };

  base_node *temp = &_headNode;

  /* located target node: */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode) // validate if is valid node
  {
    template_struct *deleteNode = reinterpret_cast<template_struct *>(temp->nextNode);
    temp->nextNode = temp->nextNode->nextNode; // relink list

    delete deleteNode; // << delete as object. calls deconstructor if any
  }
}

template <typename T>
T *linkedList::getNodeData(uint16_t index)
{
  struct template_struct : node
  {
    T data;
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
    return &(returnNode->data);
  }
  return nullptr; // index out of bounds
}