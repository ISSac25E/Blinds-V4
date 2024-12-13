#include "iostream"
using namespace std;
#include "../../../../LinkedList/LinkedList_1.0.2.h"

/*
  sketchy sketchy sketchy, but I want to see this work :)

  uint8_t packet topology:
    [PID(1B)][BUCKET(1B)][TIMEOUT(4B)][TIMER*(4B)][DATA(...)]
*/
class packet_struct
{
public:
  uint8_t getPID()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    if (arrLen < 1)
    {
      return 0;
    }

    else
      return arrPnt[0];
  }
  uint8_t getBucket()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    if (arrLen < 2)
    {
      return 0;
    }

    else
      return arrPnt[1];
  }
  uint32_t getTimeout()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    /*
      less than 6 bytes automatically means either no packet or not enough bytes for a timeout to be present
    */
    if (arrLen < 6)
    {
      return 0;
    }

    return ((uint32_t)arrPnt[2] | ((uint32_t)arrPnt[3] << (1 * 8)) | ((uint32_t)arrPnt[4] << (2 * 8)) | ((uint32_t)arrPnt[5] << (3 * 8)));
  }
  uint32_t getTimer()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    /*
      less than 10 byte automatically means either no packet or not enough bytes for a timer to be present
    */
    if (arrLen < 10)
    {
      return 0;
    }

    uint32_t timeout = this->getTimeout();

    if (timeout)
    {
      return ((uint32_t)arrPnt[6] | ((uint32_t)arrPnt[7] << (1 * 8)) | ((uint32_t)arrPnt[8] << (2 * 8)) | ((uint32_t)arrPnt[9] << (3 * 8)));
    }
    else
    {
      return 0;
    }
  }
  uint16_t dataSize()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    if (this->getTimeout())
    {
      if (arrLen < 10)
        return 0;
      else
        return arrLen - 10;
    }
    else
    {
      if (arrLen < 6)
        return 0;
      else
        return arrLen - 6;
    }
  }
  uint8_t *getData()
  {
    // hijack the 'this' pointer and get array and length info:
    uint8_t *arrPnt = (uint8_t *)this;
    uint16_t arrLen = (uint16_t)arrPnt[-2] | ((uint16_t)arrPnt[-1] << 8);

    if (this->getTimeout())
    {
      if (arrLen < 10)
        return nullptr;
      else
        return &arrPnt[10]; // 11th spot, if datasize is zero, it should not be used!
    }
    else
    {
      if (arrLen < 6)
        return nullptr;
      else
        return &arrPnt[6]; // 7th spot, if datasize is zero, it should not be used!
    }
  }

  // overload funct:
  static void storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint8_t *data, uint8_t len)
  {
    packet_struct::storePacket(ll, PID, BUCKET, 0, data, len);
  }

  static void storePacket(linkedList *ll, uint8_t PID, uint8_t BUCKET, uint32_t timeout, uint8_t *data, uint8_t len)
  {
    uint16_t pckgLen = 6 + (timeout ? 4 : 0) + len;
    uint8_t pckg[pckgLen];
    uint16_t dataIndex = 6; // 10 if timeout

    pckg[0] = PID;
    pckg[1] = BUCKET;

    pckg[2] = timeout;
    pckg[3] = timeout >> (8 * 1);
    pckg[4] = timeout >> (8 * 2);
    pckg[5] = timeout >> (8 * 3);

    if (timeout)
    {
      dataIndex += 4;
      uint32_t tim = 0xABCDEFA;

      pckg[6] = tim;
      pckg[7] = tim >> (8 * 1);
      pckg[8] = tim >> (8 * 2);
      pckg[9] = tim >> (8 * 3);
    }

    for (uint8_t x = 0; x < len; x++)
      pckg[dataIndex + x] = data[x];

    ll->addNode(pckg, pckgLen);
  }
};

void printarr(uint8_t *arr, int32_t len)
{
  cout << "[" << (int)len << "] = {";
  for (int32_t x = 0; x < len && len >= 0; x++)
  {
    cout << (int)arr[x] << (x < len - 1 ? ", " : "");
  }
  cout << "}" << endl;
}
void printLL(linkedList *ll)
{
  cout << "List Size [" << ll->nodeCount() << "]" << endl;
  for (int x = 0; x < ll->nodeCount(); x++)
  {
    printarr(ll->getNode(x), (int)ll->nodeBytes(x));
    fflush(stdout);
  }
  cout << endl;
  fflush(stdout);
}

void printPackets(linkedList *ll)
{
  cout << "Packet Count [" << ll->nodeCount() << "]" << endl;
  for (int32_t x = 0; x < ll->nodeCount(); x++)
  {
    cout << '\t';
    packet_struct *pck = reinterpret_cast<packet_struct *>(ll->getNode(x));

    cout << "PID: " << (int)pck->getPID()
         << " BUCKET: " << (int)pck->getBucket()
         << " TIMEOUT: " << pck->getTimeout();

    if (pck->getTimeout())
    {
      cout << " TIMER: " << hex << pck->getTimer() << dec;
    }

    cout << " data";
    printarr(pck->getData(), pck->dataSize());
  }
  cout << endl;
}

uint8_t a1[] = {30, 74, 155, 181, 246};
uint8_t a2[] = {21, 100, 118, 153, 195};
uint8_t a3[] = {8, 28, 64, 85, 222};

uint8_t b1[] = {3, 40, 45, 50, 68, 73, 79, 116, 129, 177, 206, 208, 216, 221, 244};
uint8_t b2[] = {8, 30, 103, 126, 135, 142, 151, 166, 175, 187, 199, 207, 227, 229, 231};
uint8_t b3[] = {31, 46, 58, 81, 96, 102, 132, 170, 174, 181, 195, 215, 225, 249, 250};

uint8_t c1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254};

uint8_t d1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
                0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254};

int main()
{
  linkedList PacketList;

  cout << "init" << endl;
  packet_struct::storePacket(&PacketList, 43, 1, 2000, a1, sizeof(a1));
  packet_struct::storePacket(&PacketList, 44, 1, 500, a2, sizeof(a2));
  packet_struct::storePacket(&PacketList, 45, 2, 6000, b1, sizeof(b1));
  packet_struct::storePacket(&PacketList, 46, 2, b3, sizeof(b3));
  PacketList.deleteNode(1);
  printPackets(&PacketList);
  while (PacketList.nodeCount())
    PacketList.deleteNode();
  packet_struct::storePacket(&PacketList, 46, 2, b3, sizeof(b3));
  packet_struct::storePacket(&PacketList, 45, 2, 6000, b1, sizeof(b1));
  packet_struct::storePacket(&PacketList, 43, 1, 2000, a1, sizeof(a1));
  packet_struct::storePacket(&PacketList, 44, 1, 500, a2, sizeof(a2));
  printPackets(&PacketList);

  return 0;
}