/*
  simple collection of useful tools
*/

class ESP_Now_util
{
public:
  /*
    compMac:
      compare two mac addresses
      returns false if mismatch
      returns true if match
  */
  static bool compMac(uint8_t *mac0, uint8_t *mac1)
  {
    for (uint8_t x = 0; x < 6; x++)
      if (mac0[x] != mac0[x])
        return false;
    return true;
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