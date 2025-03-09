/*
  simple collection of useful tools
*/
#ifndef ESP_Mesh_util_h
#define ESP_Mesh_util_h

#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

/* for atomics: */
#include <atomic>

class ESP_Mesh;
class ESP_Mesh_Connection;

class ESP_Mesh_util
{

  friend class ESP_Mesh;
  friend class ESP_Mesh_Connection;

private:
  /*
    encode base64
    encode a byte array into a NON-STANDARD base-64 foramt

    exact space needed for char*:
      ((len / 3) * 4) + (((len % 3) + 1) * (bool)(len % 3)) + 1

    simple and but less precision:
      ((len / 3) + 1) * 4 + 1
  */
  static inline uint32_t encode_base64(char *c, const void *void_data, uint32_t len)
  {
    if (!c || !void_data)
      return 0;

    const byte *b = static_cast<const byte *>(void_data);
    uint32_t byte_index = 0;
    uint32_t char_index = 0;

    for (;;)
    {
      uint32_t triplets = 0;
      int8_t y;
      for (y = 0; y < 3; y++)
      {
        if (byte_index >= len)
          break;
        triplets |= ((uint32_t)b[byte_index++] << (y * 8));
      }
      if (y > 0)
        for (int8_t x = 0; x <= y; x++)
          c[char_index++] = ((triplets >> (6 * x)) & B111111) + '0';

      if (y < 3)
        break;
    }

    c[char_index++] = '\0';
    return char_index;
  }

  /*
    decode base64
    takes base64 alphanumerical char array and decodes into a byte array
    this will search for a terminating character \0
    make sure the byte array has sufficient memory allocated

    This offset starting from '0' of the ascii table

    returns total number of bytes decoded

    exact space needed for void*:
      (((sizeof(c) - 1) >> 2) * 3) + ((bool)((sizeof(c) - 1) & B11) * (((sizeof(c) - 1) & B11) - 1))
  */
  static inline uint32_t decode_base64(const char *c, void *void_data)
  {
    if (!c || !void_data)
      return 0;

    byte *b = static_cast<byte *>(void_data);
    uint32_t byte_index = 0;
    uint32_t char_index = 0;

    for (;;)
    {
      uint32_t triplets = 0;
      int8_t y;
      for (y = 0; y < 4; y++)
      {
        if (c[char_index] < '0' || c[char_index] > ('0' + B111111))
          break;
        triplets |= ((uint32_t)(c[char_index] - '0') << (y * 6));
        char_index++;
      }
      for (int8_t x = 0; x < y - 1; x++)
        b[byte_index++] = triplets >> (8 * x);

      if (y < 4)
        return byte_index;
    }
  }

  /*
    calculates required buffer size
  */
  static inline uint32_t decode_base64_size(const char *c)
  {
    if (!c)
      return 0;

    uint32_t byte_index = 0;
    uint32_t char_index = 0;

    for (;;)
    {
      int8_t y;
      for (y = 0; y < 4; y++)
      {
        if (c[char_index] < '0' || c[char_index] > ('0' + B111111))
          break;
        char_index++;
      }

      if (y > 1)
        byte_index += (y - 1);

      if (y < 4)
        return byte_index;
    }
  }

  /*
    this will return the absolute different between these two numbers with byte overflow
    is start and end are exactly the same, 0 will be returned*/
  static inline uint8_t byte_diff(uint8_t start, uint8_t end)
  {
    return end - start;
  }

  static painlessMesh *meshPointer;

  // static instances of json docs for receiving and transmitting JSON data
  static JsonDocument jsonDocRx;
  static JsonDocument jsonDocTx;

  /*
    this is used to lock and unlock resources
    better to have the esp crash than to attempt using corrupt data
  */
  class rscSync
  {
  public:
    rscSync()
    {
      _resourceLock.clear(); // Initially unlocked (clear sets the flag to false)
    }

    void lock()
    {
      if (!_resourceLock.test_and_set(std::memory_order_acquire))
        return; // Successfully acquired the lock (previous value was false)

      // If the resource is already locked, force crash
      abort();
    }

    void unlock()
    {
      if (_resourceLock.test_and_set(std::memory_order_acquire))
      {
        _resourceLock.clear(std::memory_order_release); // Set flag back to false (unlocked)
        return;                                         // Successfully released the lock
      }

      // If the resource is already unlocked or corrupted, force crash
      abort();
    }

  private:
    std::atomic_flag _resourceLock ATOMIC_FLAG_INIT; // Atomic flag to hold the lock status
  };
};
#endif // #ifndef ESP_Mesh_util_h