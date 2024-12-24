## changes from v1.0.3

### 12-22-2024
- make linked list class iterable to improve search efficiency
- add functionality to delete node while iterating.\
  all methods in the iterator class are designed for efficiency to prevent redundant traversal
#### example use case:
```
for (auto it = ll.begin(); it != ll.end(); ++it)
{
  static_cast<data *>(it.getNodeData()); // get data
  // or
  it.getNodeData<data>(); // get data using template

  it.getNodeSize(); // get node size
  it.deleteNode(); // deletes current node (can only be done once, others will be ignored), (*it) points to prev node now
}
```
or
```
for (auto &item : ll) // CRITICAL, must use '&' otherwise a copy of the iterator object will be created instead of a reference, corrupting data and causing a potential crash
{
  static_cast<data *>(item.getNodeData()); // get data
  item.getNodeSize(); // get node size
  item.deleteNode(); // deletes current node (can only be done once, others will be ignored), (*it) points to prev node now
}
```


### 12-21-2024
- convert to a .h and .cpp file structure