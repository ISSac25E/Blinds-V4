#include "Arduino.h"
#include "../../LinkedList.h"

class myClass
{
public:
  myClass(int in)
  {
    Serial.print("Constructor called! ");
    Serial.println(in);
  }

  ~myClass()
  {
    Serial.println("Deconstructor called!");
  }

  void printVars()
  {
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.println(z);
  }

private:
  int x = 157;
  int y = -6821;
  float z = 8.98342;
};

// Arrays for testing
uint8_t a1[] = {30, 74, 155, 181, 246};
uint8_t a2[] = {21, 100, 118, 153, 195};
uint8_t a3[] = {8, 28, 64, 85, 222};

uint8_t b1[] = {3, 40, 45, 50, 68, 73, 79, 116, 129, 177, 206, 208, 216, 221, 244};
uint8_t b2[] = {8, 30, 103, 126, 135, 142, 151, 166, 175, 187, 199, 207, 227, 229, 231};
uint8_t b3[] = {31, 46, 58, 81, 96, 102, 132, 170, 174, 181, 195, 215, 225, 249, 250};

uint8_t c1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254};

uint8_t d1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
                0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254};

// Function to print the array for debugging
void printarr(uint8_t *arr, int32_t len)
{
  Serial.print("[");
  Serial.print(len);
  Serial.print("] = {");
  for (int32_t x = 0; x < len; x++)
  {
    Serial.print(arr[x]);
    if (x < len - 1)
    {
      Serial.print(", ");
    }
  }
  Serial.println("}");
}

// Function to print the linked list for debugging
void printLL(linkedList *ll)
{
  Serial.print("List Size [");
  Serial.print(ll->nodeCount());
  Serial.println("]");

  for (auto &item : (*ll))
  {
    if (item.getNodeSize() > 100)
      Serial.print("delete >> ");

    printarr(static_cast<uint8_t *>(item.getNodeData()), item.getNodeSize());

    if (item.getNodeSize() > 100)
    {
      item.deleteNode();
      item.deleteNode();
      item.deleteNode();
    }
  }

  // for (auto it = ll->begin(); it != ll->end(); ++it)
  // {
  //   if (it.getNodeSize() > 100)
  //     Serial.print("delete >> ");

  //   printarr(static_cast<uint8_t *>(it.getNodeData()), it.getNodeSize());

  //   if (it.getNodeSize() > 100)
  //   {
  //     it.deleteNode();
  //     it.deleteNode();
  //     it.deleteNode();
  //   }
  // }
  Serial.println();
}

void setup()
{
  Serial.begin(115200); // Initialize serial communication
  Serial.println();

  linkedList ll; // Create an instance of linkedList
  linkedList class_list;
  linkedList my_list;

  class_list.addNode<myClass>(0);
  class_list.getNodeData<myClass>(0)->printVars();
  class_list.deleteNode<myClass>(0);

  my_list.addNode<float>(0);
  Serial.println(*(my_list.getNodeData<float>(0)));

  // class_list.addNode(0, nullptr, sizeof(myClass)); // allocate
  // new (class_list.getNodeData(0)) myClass(); // initialize
  // // reinterpret_cast<myClass*>(class_list.getNodeData(0))->printVars();
  // class_list.deleteNode(0);


  printLL(&ll); // Print the initial empty list

  // Adding nodes to the linked list
  ll.addNode(-1, a1, sizeof(a1));
  ll.addNode(-1, b1, sizeof(b1));
  printLL(&ll); // Print the list after adding nodes

  // Adding more nodes to the list
  ll.addNode(-1, c1, sizeof(c1));
  ll.addNode(-1, b3, sizeof(b3));

  ll.addNode(-1, d1, sizeof(d1));

  ll.addNode(-1, b2, sizeof(b2));

  // Delete node at index 1
  // ll.deleteNode(1);

  printLL(&ll); // Print the list after modifications
  printLL(&ll); // Print the list after modifications
}

void loop()
{
  // Nothing to do in loop, since we are only running test in setup()
}
