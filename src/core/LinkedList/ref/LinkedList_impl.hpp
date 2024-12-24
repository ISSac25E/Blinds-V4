template <typename T>
T *linkedList::getNodeData(uint16_t index)
{
  base_node *temp = &_headNode;

  /* find target node */
  for (uint16_t nodeCnt = 0; nodeCnt < index && temp->nextNode; nodeCnt++)
    temp = temp->nextNode;

  if (temp->nextNode)
  {
    /* retrieve data as a byte-array */
    uint8_t *byteData = reinterpret_cast<uint8_t *>(temp->nextNode);
    byteData += sizeof(node); // offset data to exclude length and nextnode info

    // return as intended data type
    return reinterpret_cast<T *>(byteData);
  }
  return nullptr; // index out of bounds
}

template <typename T>
bool linkedList::addNode(uint16_t index, T *data, uint16_t length)
{
  if (length <= 0)
    return false; // invalid data length!

  uint8_t *newByteNode = new (std::nothrow) uint8_t[sizeof(node) + length]; // allocate required memory as a byte-array
  if (!newByteNode)
    return false;

  node *newNode = reinterpret_cast<node *>(newByteNode); // convert to node to set length info and next node info
  newNode->length = length;

  memcpy(newByteNode + sizeof(node), data, length); // << write to the bytes trailing the node

  base_node *storeNode = &_headNode;

  // locate target location
  for (uint16_t nodeCnt = 0; nodeCnt < index && storeNode->nextNode; nodeCnt++)
    storeNode = storeNode->nextNode;

  // relink list with new node:
  newNode->nextNode = storeNode->nextNode;
  storeNode->nextNode = newNode;
  return true;
}