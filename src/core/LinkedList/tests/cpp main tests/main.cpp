#include <iostream>
using namespace std;
int main()
{
  union
  {
    uint8_t *pointer;
    uint8_t pointerByte[sizeof(uint8_t *)];
  } _pointer;

  uint8_t x[] = {4, 3, 7, 5, 4, 56};
  _pointer.pointer = x;

  cout << sizeof(_pointer.pointerByte) << endl;
  for (uint8_t y = 0; y < sizeof(_pointer.pointerByte); y++)
    cout << (int)_pointer.pointerByte[y] << ',';
  cout<< endl << (uintptr_t)_pointer.pointer << endl;
  


} // namespace std
