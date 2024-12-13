/*
  simple collection of useful tools
*/
#ifndef ESP_Mesh_util_h
#define ESP_Mesh_util_h

class ESP_Mesh_util
{
public:
  /*
    encode a byte array into a hexadecimal format. This is a non standard hexadecimal format simply for the purpose of faster conversion
    offset_start_byte
  */
  static inline void encode_base16(char *c, uint16_t offset_start_byte, const uint8_t *b, uint16_t b_len)
  {
    uint32_t char_index = (offset_start_byte * 2);
    uint32_t byte_index = 0;

    for (uint32_t x = 0; x < b_len; x++)
    {
      c[char_index++] = (b[byte_index] & B1111) + '0';
      c[char_index++] = (b[byte_index] >> 4) + '0';

      byte_index++;
    }

    c[char_index] = '\0';
  }
  static inline void encode_base16(char *c, const uint8_t *b, uint16_t b_len)
  {
    encode_base16(c, 0, b, b_len);
  }

  static inline uint16_t decode_base16(const char *c, uint8_t *b)
  {
    uint16_t char_index = 0;
    uint16_t byte_index = 0;

    for (;;)
    {
      uint8_t t_c[2] = {c[char_index] - '0', c[char_index + 1] - '0'};
      if (t_c[0] <= B1111 && t_c[1] <= B1111)
      {
        b[byte_index] = t_c[0];
        b[byte_index++] |= (t_c[1] << 4);

        char_index += 2;
      }
      else
        return byte_index;
    }
  }

  static inline void encode_base64(char *c, const uint8_t *b, uint32_t len)
  {
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
      {
        c[char_index] = '\0';
        return;
      }
    }
  }

  /*
    decode_base64:
      takes base64 alphanumerical char array and decodes into a byte array
      this will search for a terminating character \0
      make sure the byte array has sufficient memory allocated

      This offset starting from '0' of the ascii table
  */
  static inline uint32_t decode_base64(const char *c, uint8_t *b)
  {
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
    isAck:
      checks the last bit of the first byte
      [x0000000]...[]

      input: (uint8_t*) data array
      returns: true if ack packet, false if not
  */
  static inline bool isAck(uint8_t *d)
  {
    return d[0] & B10000000;
  }

  /*
    setCmdPck:
      takes a 4-bit command byte and ack bit and packages into a byte

      input: (uint8_t) 4-bit command, (bool) ack bit
      returns: (uint8_t) command byte
  */
  static inline uint8_t setCmdPck(uint8_t cmd, bool ack)
  {
    return ((cmd & B00001111) | (ack << 7));
  }
};

/*
  maybe this is a good idea for a call back function status?
*/
// class ESP_Now_Status
// {
// };

#endif // #ifndef ESP_Mesh_util_h