/*
  changes from v1.0.0 to v1.0.1
    - provide the ability to upto 16bit amount of bytes

  Purpose of this class is to create a dynamic list where each item can be dynamically created, deleted, and rearranged regardless of position in memory
  List Chart Overview:

    [Head] -> Node_0[Contents][Tail] -> Node_1[Contents][Tail] -> Node_2[Contents][Tail] -> Node_3[Contents][Tail]...

  Each Node can be created, deleted, and rearranged dynamically.

  Each node consists of the contents and a tail which points to the next node in the chain

  Each node can be targetted by their relative location in the chain(e.g. Node0 Node1, Node2...)
*/

// .h

class linkedList
{
public:
  /*
    addNode():
      will add a single node to the very end of the chain
      node will be empty by default

      input: (uint8_t*) uint8_t array of contents, (uint8_t) length of uint8_t array
      returns, (bool) true if node was successfully allocated, false otherwise
  */
  bool addNode(uint8_t *, uint16_t);
  /*
    deleteNode():
      will delete a single node at the very end of the chain
      method will automatically free memory from node AND char*(contents) if it has been used

      If no nodes are present in the chain, method will automatically exit without error
  */
  void deleteNode();

  /*
      addNode():
        will add a single node at designated input
        All nodes in front of new node will shift up a number
        chart overview:

          [Head] -> [Node_0] -> [Node_1] -> [Node_2]

          Create new node at location 1:

          [Head] -> [Node_0] -> [Node_1(New Node)] -> [Node_2(originally Node_1)] -> [Node_3(originally Node_2)]


        If inputted number exceeds the number of nodes in the chain by more than 1,
        the new node will simply be added at the very end(similar to "addNextNode()")

        inputs, (int) location where to add new node, (uint8_t*) uint8_t array of contents, (uint8_t) length of uint8_t array
        returns, (bool) true if node was successfully allocated, false otherwise

  */
  bool addNode(int, uint8_t *, uint16_t);
  /*
      deleteNode():
        will delete a single node at designated input
        All nodes in front of new node will shift down a number
        chart overview:

          [Head] -> [Node_0] -> [Node_1] -> [Node_2] -> [Node_3]

          Delete node at location 1:

          [Head] -> [Node_0] -> [Node_1(originally Node_2)] -> [Node_2(originally Node_3)]


        If inputted number exceeds the number of nodes in the chain,
        method will not delete any nodes and will return

        inputs, (uint16_t) location where to delete node
        returns, (void)

  */
  void deleteNode(uint16_t);

  /*
    nodeCount():
      method will return the number of nodes in the chain

    chart overview:

      [Head] -> [Node_0] -> [Node_1]
      returns: 2

      [Head]
      returns: 0

    returns, (uint16_t) number of nodes in chain
  */
  uint16_t nodeCount();

  /*
    setNode():
      change the size of contents stored inside a node

      inputs, (int) location of target node, (uint8_t*) uint8_t array of contents, (uint8_t) length of uint8_t array
      returns, (bool) true if node was successfully allocated, false otherwise
  */
  bool setNode(int, uint8_t *, uint16_t);

  /*
    nodePnt(int):
      method returns uint8_t pointer(char *) of the node at designated input
      This pointer is essentially the contents of your node

      method will return NULL if node does not exist

      Example use case:
        cout << linkedList.nodePnt(2)[0] << endl;

      inputs, (int) location of target node
      returns, (uint8_t *) uint8_t array pointer(the contents of the node)
  */
  uint8_t *getNode(uint16_t);
  /*
    byteLength():
      method will return the length of uint8_t array at target node
      if node does not exists, -1 will be returned

      inputs, (uint16_t) location of target node
      returns, (int32_t) number of bytes used at target node, -1 if node does not exist

      Example use case:
        for(uint8_t x = 0; x < linkedList.nodeBytes(0); x++)
          cout << linkedList.getNode(0)[x] << ',';
  */
  int32_t nodeBytes(uint16_t);

private:
  // main structure for each node:
  struct _node
  {
    uint8_t *item;
    struct _node *tail = nullptr;
  };

  // node head, points to very first node:
  _node *_head = nullptr;
};

// .cpp

bool linkedList::addNode(uint8_t *arr, uint16_t len)
{
  return this->addNode(this->nodeCount(), arr, len);
}

void linkedList::deleteNode()
{
  _node **pnt = &_head;

  // Check that "_head" has a node. If not, chain is completely empty. No nodes to delete, just exit
  if (*pnt != nullptr)
  {
    // search for last node:
    while ((*pnt)->tail != nullptr)
    {
      pnt = &(*pnt)->tail;
    }

    // we know (*pnt)->tail is null, make sure (*pnt)->item has no memory allocated:
    if ((*pnt)->item != nullptr)
      free((*pnt)->item);

    // free up node
    free(*pnt);

    // make sure tail of previous node is NULL:
    *pnt = nullptr;
  }
}

bool linkedList::addNode(int node, uint8_t *arr, uint16_t len)
{
  _node **pnt = &_head;

  // run through the nodes until target node reached.
  // if not enough nodes exist, just create a new node at the end:
  for (int x = 0; x < node; x++)
  {
    // not enough nodes, simply break and add a new node at the end:
    if (*pnt == nullptr)
      break;
    pnt = &((*pnt)->tail);
  }
  // allocate memory for new node:
  _node *tmpPnt = (_node *)malloc(sizeof(_node));

  if (tmpPnt == nullptr)
    return false; // malloc failed!

  // allocate items memory:
  tmpPnt->item = (uint8_t *)malloc(len + 2);
  if (tmpPnt->item == nullptr)
  {
    free(tmpPnt);
    return false; // malloc failed!
  }
  else
  {
    // copy all contents over:
    tmpPnt->item[0] = len;
    tmpPnt->item[1] = len >> 8;
    for (uint8_t x = 0; x < len; x++)
      tmpPnt->item[2 + x] = arr[x];
  }

  // init variables in new node. Link it with the other existing nodes too
  (tmpPnt)->tail = *pnt; // link to next node in line
  *pnt = tmpPnt;         // link to previous node in line

  return true;
}
void linkedList::deleteNode(uint16_t node)
{
  _node **pnt = &_head;

  // if "_head" == NULL, return. No nodes in chain:
  if (*pnt != nullptr)
  {
    // locate specified node. If specified node does not exist/is outside the chain, do not delete any nodes, simply return:
    for (uint16_t x = 0; x < node; x++)
    {
      if ((*pnt)->tail == nullptr)
        return; // node does not exist, return
      pnt = &((*pnt)->tail);
    }
    _node *tmpPnt = (*pnt)->tail; // save the address of the NEXT node, will be needed later

    // free up the memory for the char pointer if it exists:
    if ((*pnt)->item != nullptr)
      free((*pnt)->item);

    free(*pnt);      // free memory for this node/delete node:
    (*pnt) = tmpPnt; // give the address of the next node to the tail of the previous node
  }
}

uint16_t linkedList::nodeCount()
{
  int nodeCount = 0;
  _node *pnt = _head;
  // check if any nodes exist, count through each node:
  while (pnt != nullptr)
  {
    nodeCount++;
    pnt = pnt->tail;
  }
  return nodeCount; // return final count
}

bool linkedList::setNode(int node, uint8_t *arr, uint16_t len)
{
  // lazy but also safer:
  return this->addNode(node, arr, len); // add node first incase the contents are being used from current node
  this->deleteNode(node + 1);
}
uint8_t *linkedList::getNode(uint16_t node)
{
  _node *pnt = _head;
  // go through each node until specified node is located, return NULL if node does not exist:
  for (int x = 0; x < node; x++)
  {
    pnt = pnt->tail;
    if (pnt == nullptr)
      return nullptr; // node does not exist, return NULL
  }
  // Final check just incase "node" == 0 and "_head" is also NULL:
  if (pnt == nullptr)
    return nullptr;

  // return char pointer(may be NULL if not allocated yet):
  return &(pnt->item[2]); // data contents start after the length declaration (pnt->item[0] and [1])
}
int32_t linkedList::nodeBytes(uint16_t node)
{
  _node *pnt = _head;
  // go through each node until specified node is located, return NULL if node does not exist:
  for (int x = 0; x < node; x++)
  {
    pnt = pnt->tail;
    if (pnt == nullptr)
      return -1; // node does not exist, return NULL
  }
  // Final check just incase "node" == 0 and "_head" is also NULL:
  if (pnt == nullptr)
    return -1;

  return pnt->item[0] & pnt->item[1] << 8; // return data length
}