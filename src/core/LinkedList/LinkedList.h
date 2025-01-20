#ifndef linkedList_h
#define linkedList_h

/*
  changes from v1.0.3
    - convert to a .h and .cpp file structure

    - implement iterator class for linked list for efficient traversal

    - modify to allow for safe aligned allocation of data along with raw allocation

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
    This will NOT call deconstructor's
  */
  ~linkedList();

  /*
    will add a single node at designated input

    If inputted number exceeds the number of nodes in the chain by more than 1,
    the new node will simply be added at the very end

    Two main methods of adding a node.
     - As a type (object or struct) (with optional extra raw memory for flex structs)
     - As raw memory

    Adding as a type (using a template) will NOT initialize the object but align properly with memory. Using the template method will allow the option to allocate with added memory
    Adding as a raw data type will give more flexibility but may be misaligned. data must be copied before manipulation on large (> 1-byte) datatypes
  */
  void *addNode(uint16_t index, void *data, uint16_t length);
  inline void *addNode(uint16_t index, uint16_t length)
  {
    return addNode(index, nullptr, length);
  }

  template <typename T>
  T *addNode(uint16_t index, uint16_t extra_length = 0);

  /*
    will delete a single node at designated input

    If inputted number exceeds the number of nodes in the chain,
    method will not delete any nodes and will return

    inputs: (uint16_t) location where to delete node
    returns: (void)

  */
  void deleteNode(uint16_t index);

  /*
    delete with typename will call the deconstructor
  */
  template <typename T>
  void deleteNode(uint16_t index);

  /*
    method returns type T of the node at designated index
    This pointer is essentially the contents of your node

    method will return null if node does not exist

    inputs: (uint16_t) location of target node
    returns: (T*) uint8_t array pointer(the contents of the node)
  */
  void *getNodeData(uint16_t index);

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

  /*
    create an iterable linkedlist for fast searching
  */
public:
  class Iterator
  {
  public:
    Iterator(base_node *base)
    {
      _prev_node = reinterpret_cast<node *>(base);
      if (_prev_node)
      {
        _current_node = _prev_node->nextNode;
      }
      else
      {
        _current_node = nullptr;
      }
    }

    template <typename T>
    T *getNodeData()
    {
      struct template_struct : node
      {
        T data;
      };

      if (!_current_node)
        return nullptr;

      template_struct *returnNode = reinterpret_cast<template_struct *>(_current_node);
      return &(returnNode->data);
    }

    void *getNodeData()
    {
      struct template_struct : node
      {
        byte data[0];
      };

      if (!_current_node)
        return nullptr;

      return reinterpret_cast<void *>(reinterpret_cast<template_struct *>(_current_node)->data);
    }

    /*
      return iterator reference to retain full functionality
    */
    Iterator &operator*()
    {
      return *this;
    }

    /*
      get node size
      will return -1 if no node

      inputs: (uint16_t) node index
      outputs: (int32_t) node byte size, -1 if invalid node
    */
    int32_t getNodeSize()
    {
      if (_current_node)
      {
        return _current_node->length;
      }
      return -1;
    }

    Iterator &operator++()
    {
      if (!_current_node)
        return *this;

      _prev_node = _current_node; // this allows for safe node deletions
      _current_node = _current_node->nextNode;

      return *this;
    }

    bool operator!=(const Iterator &other) const
    {
      return _current_node != other._current_node;
    }

    /*
      this will safely delete the node and continue iterating
    */
    void deleteNode()
    {
      // "_current_node == _prev_node->nextNode" prevents from attempting to delete a node twice. Can't traverse backwards!!
      if (!_current_node || _current_node == _prev_node)
        return; // nothing to delete here

      // temporarily store next node to re-link list
      node *next_node = _current_node->nextNode;

      free(_current_node); // << delete

      // re-link:
      _prev_node->nextNode = next_node;
      _current_node = _prev_node; // this will allow for continued traversal
    }

    template <typename T>
    void deleteNode()
    {
      struct template_struct : node
      {
        T data;
      };

      if (!_current_node || _current_node == _prev_node)
        return;

      // temporarily store next node to re-link list
      node *next_node = _current_node->nextNode;

      reinterpret_cast<template_struct*>(_current_node)->data.~T();
      free(_current_node); // << delete

      // re-link:
      _prev_node->nextNode = next_node;
      _current_node = _prev_node; // this will allow for continued traversal
    }

  private:
    node *_current_node;

    /*
      prev_node is used to have more advanced features like deleting nodes while traversing
    */
    node *_prev_node;
  };

  Iterator begin()
  {
    return Iterator(&_headNode);
  }

  Iterator end()
  {
    return Iterator(nullptr);
  }
};

#include "LinkedList_impl.hpp"

#endif