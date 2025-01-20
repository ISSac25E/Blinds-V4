template <typename T>
T *linkedList::addNode(uint16_t index, uint16_t extra_length)
{
  struct template_struct : node
  {
    T data;
  };

  // allocate memory
  // Use alignof to determine the alignment of the struct. technically, malloc alone is safe enough to allocate as it does so with the largest alignment possible
  // template_struct *newNode = (template_struct *)aligned_alloc(alignof(template_struct), sizeof(template_struct) + extra_length); // get an undefined reference error here
  template_struct *newNode = (template_struct *)malloc(sizeof(template_struct) + extra_length);

  if (!newNode)
    return nullptr;

  newNode->length = sizeof(T) + extra_length;

  base_node *storeNode = &_headNode;

  // locate target location
  for (uint16_t nodeCnt = 0; nodeCnt < index && storeNode->nextNode; nodeCnt++)
    storeNode = storeNode->nextNode;

  // relink list with new node:
  newNode->nextNode = storeNode->nextNode;
  storeNode->nextNode = newNode;
  return &(newNode->data);
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

    deleteNode->data.~T();
    free(deleteNode); // << delete
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
    /* retrieve data as a object */
    template_struct *returnNode = reinterpret_cast<template_struct *>(temp->nextNode);

    // return as intended data type
    return &(returnNode->data);
  }
  return nullptr; // index out of bounds
}