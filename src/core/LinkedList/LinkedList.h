#ifndef linkedList_h
#define linkedList_h

/*
  changes from v1.0.3 to v1.0.4
    - convert to a .h and .cpp file structure

  Purpose of this class is to create a dynamic list where each item can be dynamically created, deleted, and rearranged regardless of position in memory
  List Chart Overview:

    [Head] -> Node_0[Contents][Tail] -> Node_1[Contents][Tail] -> Node_2[Contents][Tail] -> Node_3[Contents][Tail]...

  Each Node can be created, deleted, and rearranged dynamically.

  Each node consists of the contents and a tail which points to the next node in the chain

  Each node can be targetted by their relative location in the chain(e.g. Node0 Node1, Node2...)
*/

#include <Arduino.h>
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

#include "LinkedList_impl.hpp"

#endif