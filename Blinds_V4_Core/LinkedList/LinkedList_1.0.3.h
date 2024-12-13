#ifndef linkedList_h
#define linkedList_h

/*
  changes from v1.0.2 to v1.0.3
    - use function templates to handle type casting by default

  Purpose of this class is to create a dynamic list where each item can be dynamically created, deleted, and rearranged regardless of position in memory
  List Chart Overview:

    [Head] -> Node_0[Contents][Tail] -> Node_1[Contents][Tail] -> Node_2[Contents][Tail] -> Node_3[Contents][Tail]...

  Each Node can be created, deleted, and rearranged dynamically.

  Each node consists of the contents and a tail which points to the next node in the chain

  Each node can be targetted by their relative location in the chain(e.g. Node0 Node1, Node2...)
*/

#include <new>

// .h
class linkedList
{
public:
  /*
    safely deallocates entire list.
  */
  ~linkedList();

  template <typename T>
  bool appendNode(T data)
  {
    return addNode<T>(nodeCount(), &data, sizeof(T));
  }
  template <typename T>
  bool appendNode(T *data, uint16_t length)
  {
    return addNode<T>(nodeCount(), data, length);
  }

  /*
    will add a single node at designated input

    If inputted number exceeds the number of nodes in the chain by more than 1,
    the new node will simply be added at the very end

    inputs: (uint16_t) location where to add new node, (T*)or(T) data to be added, (int16_t) data length (MUST BE POSITIVE NON-ZERO)
    returns: (bool) true if node was successfully allocated and proper input size, false otherwise
  */
  template <typename T>
  bool addNode(uint16_t index, T data)
  {
    return addNode<T>(index, &data, sizeof(T));
  }
  template <typename T>
  bool addNode(uint16_t index, T *data, uint16_t length);

  /*
    will delete a single node at designated input

    If inputted number exceeds the number of nodes in the chain,
    method will not delete any nodes and will return

    inputs: (uint16_t) location where to delete node
    returns: (void)

  */
  void deleteNode(uint16_t index);

  /*
    method returns type T of the node at designated index
    This pointer is essentially the contents of your node

    method will return null if node does not exist

    inputs: (uint16_t) location of target node
    returns: (T*) uint8_t array pointer(the contents of the node)
  */
  template <typename T>
  T *getNodeData(uint16_t index);

  /*
    get node size at specified index
    will return -1 if invalid index

    inputs: (uint16_t) node index
    outputs: (int32_t) node byte size, -1 if invalid index
  */
  int32_t getNodeSize(uint16_t index);

  /*
    traverses the list and returns number of nodes
  */
  uint16_t nodeCount();

private:
  struct node;
  struct base_node
  {
    node *nextNode;
    base_node();
  };
  struct node : base_node
  {
    uint16_t length;
    node();
  };

  base_node _headNode;
};

// .cpp

/* initialize 'base_node' struct */
linkedList::base_node::base_node() : nextNode(nullptr) {}

/* initialize 'node' struct */
linkedList::node::node() : length(0) {}

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
    byteData += sizeof(node); // offset data to exclude length info

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

#endif